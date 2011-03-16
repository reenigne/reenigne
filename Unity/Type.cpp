Symbol parseTypeConstructorIdentifier(CharacterSource* source)
{
    CharacterSource s = *source;
    Location location = s.location();
    int start = s.offset();
    int c = s.get();
    if (c < 'A' || c > 'Z')
        return Symbol();
    CharacterSource s2;
    do {
        s2 = s;
        c = s.get();
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '_')
            continue;
        break;
    } while (true);
    int end = s2.offset();
    Location endLocation = s2.location();
    Space::parse(&s2);
    String name = s2.subString(start, end);
    static String keywords[] = {
        String("Class"),
        String("Complex"),
        String("DInt"),
        String("DUInt"),
        String("DWord"),
        String("Fixed"),
        String("Float"),
        String("HInt"),
        String("HUInt"),
        String("HWord"),
        String("Integer"),
        String("QInt"),
        String("QUInt"),
        String("QWord"),
        String("Rational"),
        String("TypeOf"),
        String("Unsigned"),
        String("WordString")
    };
    for (int i = 0; i < sizeof(keywords)/sizeof(keywords[0]); ++i)
        if (name == keywords[i])
            return Symbol();
    *source = s2;
    return Symbol(atomTypeConstructorIdentifier, name,
        new IdentifierCache(Span(location, endLocation)));
}

Symbol parseClassTypeSpecifier(CharacterSource* source)
{
    static String keyword("Class");
    Location start = source->location();
    Span span;
    if (!Space::parseKeyword(source, keyword, &span))
        return Symbol();
    Span span2;
    Space::assertCharacter(source, '{', &span2);
    // TODO: Parse class contents
    Space::assertCharacter(source, '}', &span2);
    return Symbol(atomClass, new TypeCache(span + span2, -1, -1,
        SymbolLabel()));
}

Symbol parseTypeOfTypeSpecifier(CharacterSource* source);

Symbol parseTypeConstructorSpecifier(CharacterSource* source);

Symbol parseFundamentalTypeConstructorSpecifier(CharacterSource* source)
{
    CharacterSource s2 = *source;
    Symbol typeSpecifier = parseTypeConstructorIdentifier(&s2);
    if (typeSpecifier.valid()) {
        String s = typeSpecifier[1].string();
        Span span = spanOf(typeSpecifier);
        static String autoKeyword("Auto");
        if (s == autoKeyword) {
            *source = s2;
            return Symbol(atomAuto, newSpan(span));
        }
        static String bitKeyword("Bit");
        if (s == bitKeyword) {
            *source = s2;
            return Symbol(atomBit, new TypeCache(span, 1, 1));
        }
        static String booleanKeyword("Boolean");
        if (s == booleanKeyword) {
            *source = s2;
            return Symbol(atomBoolean, new TypeCache(span, 1, 1));
        }
        static String byteKeyword("Byte");
        if (s == byteKeyword) {
            *source = s2;
            return Symbol(atomByte, new TypeCache(span, 1, 1));
        }
        static String characterKeyword("Character");
        if (s == characterKeyword) {
            *source = s2;
            return Symbol(atomCharacter, new TypeCache(span, 4, 4));
        }
        static String intKeyword("Int");
        if (s == intKeyword) {
            *source = s2;
            return Symbol(atomInt, new TypeCache(span, 4, 4));
        }
        static String stringKeyword("String");
        if (s == stringKeyword) {
            *source = s2;
            return Symbol(atomString, new TypeCache(span, 4, 4));
        }
        static String uIntKeyword("UInt");
        if (s == uIntKeyword) {
            *source = s2;
            return Symbol(atomUInt, new TypeCache(span, 4, 4));
        }
        static String voidKeyword("Void");
        if (s == voidKeyword) {
            *source = s2;
            return Symbol(atomVoid, new TypeCache(span, 0, 0));
        }
        static String wordKeyword("Word");
        if (s == wordKeyword) {
            *source = s2;
            return Symbol(atomVoid, new TypeCache(span, 4, 4));
        }
        SymbolList templateArguments;
        while (Space::parseCharacter(&s2, '<', &span)) {
            do {
                Symbol templateArgument = parseTypeConstructorSpecifier(&s2);
                if (!templateArgument.valid())
                    return Symbol();
                templateArguments.add(templateArgument);
            } while (Space::parseCharacter(&s2, ',', &span));
            if (!Space::parseCharacter(&s2, '>', &span))
                return Symbol();
        }
        return Symbol(atomTypeConstructorIdentifier, typeSpecifier,
            SymbolArray(templateArguments),
            newSpan(spanOf(typeSpecifier) + span));
    }
    typeSpecifier = parseClassTypeSpecifier(source);
    if (typeSpecifier.valid())
        return typeSpecifier;
    typeSpecifier = parseTypeOfTypeSpecifier(source);
    if (typeSpecifier.valid())
        return typeSpecifier;
    return Symbol();
}

SymbolArray parseTypeConstructorSpecifierList(CharacterSource* source)
{
    SymbolList list;
    Symbol typeSpecifier = parseTypeConstructorSpecifier(source);
    if (!typeSpecifier.valid())
        return list;
    list.add(typeSpecifier);
    Span span;
    while (Space::parseCharacter(source, ',', &span)) {
        typeSpecifier = parseTypeConstructorSpecifier(source);
        if (!typeSpecifier.valid()) {
            static String error("Type specifier expected");
            source->location().throwError(error);
        }
        list.add(typeSpecifier);
    }
    return list;
}

//TypeConstructorSpecifier :=
//    TypeConstructorIdentifier ("<" TypeConstructorSpecifier \ "," ">")*
//  | TypeConstructorSpecifier "*"
//  | TypeConstructorSpecifier "(" [(TypeConstructorSpecifier [Identifier] \ ","] ")"
//  | "Class" "{" ClassDefinition "}"
//  | "TypeOf" "(" Expression ")"
Symbol parseTypeConstructorSpecifier(CharacterSource* source)
{
    Symbol typeSpecifier = parseFundamentalTypeConstructorSpecifier(source);
    if (!typeSpecifier.valid())
        return Symbol();
    do {
        Span span;
        if (Space::parseCharacter(source, '*', &span)) {
            typeSpecifier = Symbol(atomPointer, typeSpecifier,
                new TypeCache(spanOf(typeSpecifier) + span, 4, 4));
            continue;
        }
        if (Space::parseCharacter(source, '(', &span)) {
            SymbolArray typeListSpecifier = parseTypeConstructorSpecifierList(source);
            Space::assertCharacter(source, ')', &span);
            typeSpecifier = Symbol(atomFunction, typeSpecifier,
                typeListSpecifier, new TypeCache(spanOf(typeSpecifier) + span, 0, 0));
            continue;
        }
    } while (true);
    return typeSpecifier;
}

Symbol parseExpressionOrFail(CharacterSource* source);

Symbol parseTypeOfTypeSpecifier(CharacterSource* source)
{
    Span span;
    static String keyword("TypeOf");
    if (!Space::parseKeyword(source, keyword, &span))
        return Symbol();
    Span span2;
    Space::assertCharacter(source, '(', &span2);
    Symbol expression = parseExpressionOrFail(source);
    Space::assertCharacter(source, ')', &span2);
    return Symbol(atomTypeOf, expression, newSpan(span + span2));
}

//KindSpecifier := ("<" [([TypeConstructorIdentifier] KindSpecifier) \ ","] ">")*
Symbol parseKindSpecifier(CharacterSource* source)
{
    Span span;
    if (!Space::parseCharacter(source, '<', &span))
        return Symbol(atomTypeKind, newSpan(Span(source->location(), source->location())));
    SymbolList kindSpecifierList;
    Span span2;
    do {
        // A type constructor identifier is allowed here for documentation
        // purposes only - it isn't used for anything, so we immediately throw
        // it away. It doesn't even need to be resolved.
        do {
            parseTypeConstructorIdentifier(source);  
            kindSpecifierList.add(parseKindSpecifier(source));
        } while (Space::parseCharacter(source, ',', &span2));
        Space::assertCharacter(source, '>', &span2);
    } while (Space::parseCharacter(source, '<', &span2));
    return Symbol(atomTemplateKind, SymbolArray(kindSpecifierList),
        newSpan(span + span2));
}

//SpecializedTypeConstructorSpecifier :=
//    TypeConstructorSpecifier
//  | "@" TypeConstructorIdentifier
//  | SpecializedTypeConstructorSpecifier "*"
//  | SpecializedTypeConstructorSpecifier "(" [SpecializedTypeConstructorSpecifier \ ","] ")"
Symbol parseFundamentalSpecializedTypeConstructorSpecifier(CharacterSource* source)
{
    Symbol specializedTypeConstructorSpecifier = parseTypeConstructorSpecifier(source);
    if (specializedTypeConstructorSpecifier.valid())
        return specializedTypeConstructorSpecifier;
    Span span;
    if (!Space::parseCharacter(source, '@', &span)) {
        static String error("Expected @ or type constructor specifier");
        source->location().throwError(error);
    }
    Symbol typeConstructorIdentifier = parseTypeConstructorIdentifier(source);
    if (!typeConstructorIdentifier.valid()) {
        static String error("Expected type constructor identifier");
        source->location().throwError(error);
    }
    return Symbol(atomTemplateParameter, typeConstructorIdentifier,
        newSpan(span + spanOf(typeConstructorIdentifier)));
}

Symbol parseSpecializedTypeConstructorSpecifier(CharacterSource* source);

SymbolArray parseSpecializedTypeConstructorSpecifierList(CharacterSource* source)
{
    SymbolList list;
    Symbol typeSpecifier = parseSpecializedTypeConstructorSpecifier(source);
    if (!typeSpecifier.valid())
        return list;
    list.add(typeSpecifier);
    Span span;
    while (Space::parseCharacter(source, ',', &span)) {
        typeSpecifier = parseSpecializedTypeConstructorSpecifier(source);
        if (!typeSpecifier.valid()) {
            static String error("(Specialized) type specifier expected");
            source->location().throwError(error);
        }
        list.add(typeSpecifier);
    }
    return list;
}

Symbol parseSpecializedTypeConstructorSpecifier(CharacterSource* source)
{
    Symbol typeSpecifier = parseFundamentalTypeConstructorSpecifier(source);
    if (!typeSpecifier.valid())
        return Symbol();
    do {
        Span span;
        if (Space::parseCharacter(source, '*', &span)) {
            typeSpecifier = Symbol(atomPointer, typeSpecifier,
                new TypeCache(spanOf(typeSpecifier) + span, 4, 4));
            continue;
        }
        if (Space::parseCharacter(source, '(', &span)) {
            SymbolArray typeListSpecifier =
                parseSpecializedTypeConstructorSpecifierList(source);
            Space::assertCharacter(source, ')', &span);
            typeSpecifier = Symbol(atomFunction, typeSpecifier,
                typeListSpecifier, new TypeCache(spanOf(typeSpecifier) + span, 0, 0));
            continue;
        }
    } while (true);
    return typeSpecifier;
}

//TemplateParameter := 
//    "@" TypeConstructorIdentifier KindSpecifier
//  | SpecializedTypeConstructorSpecifier
Symbol parseTemplateParameter(CharacterSource* source)
{
    Span span;
    if (Space::parseCharacter(source, '@', &span)) {
        Symbol typeConstructorIdentifier = parseTypeConstructorIdentifier(source);
        if (!typeConstructorIdentifier.valid()) {
            static String error("Expected type constructor identifier");
            source->location().throwError(error);
        }
        Symbol kindSpecifier = parseKindSpecifier(source);
        return Symbol(atomTemplateParameter, typeConstructorIdentifier,
            kindSpecifier, newSpan(span + spanOf(kindSpecifier)));
    }
    return parseSpecializedTypeConstructorSpecifier(source);
}

//TypeConstructorSignifier := 
//    TypeConstructorIdentifier ("<" TemplateParameter \ "*" ">")*     
Symbol parseTypeConstructorSignifier(CharacterSource* source)
{
    CharacterSource s2 = *source;
    Symbol typeConstructorIdentifier = parseTypeConstructorIdentifier(source);
    if (!typeConstructorIdentifier.valid())
        return Symbol();
    SymbolList templateParameters;
    Span span;
    while (Space::parseCharacter(&s2, '<', &span)) {
        do {
            Symbol templateParameter = parseTemplateParameter(&s2);
            if (!templateParameter.valid())
                return Symbol();
            templateParameters.add(templateParameter);
        } while (Space::parseCharacter(&s2, ',', &span));
        if (!Space::parseCharacter(&s2, '>', &span))
            return Symbol();
    }
    return Symbol(atomTypeConstructorSignifier, typeConstructorIdentifier,
        SymbolArray(templateParameters),
        newSpan(spanOf(typeConstructorIdentifier) + span));
}

String typeToString(Symbol type)
{
    switch (type.atom()) {
        case atomFunction:
            {
                String s = typeToString(type[1].symbol()) + openParenthesis;
                SymbolArray array = type[2].array();
                bool hasArguments = false;
                for (int i = 0; i < array.count(); ++i) {
                    if (hasArguments)
                        s += commaSpace;
                    s += typeToString(array[i]);
                    hasArguments = true;
                }
                return s + closeParenthesis;
            }
        case atomPointer:
            return typeToString(type[1].symbol()) + asterisk;
        default:
            return atomToString(type.atom());
    }
}

String typesToString(SymbolArray array)
{
    String s = openParenthesis;
    for (int i = 0; i < array.count(); ++i) {
        if (i != 0)
            s += commaSpace;
        s += typeToString(array[i]);
    }
    return s + closeParenthesis;
}