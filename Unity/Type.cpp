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
        Symbol::newLabel()));
}

Symbol parseTypeOfTypeSpecifier(CharacterSource* source);

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

Symbol parseTypeSpecifier(CharacterSource* source);

SymbolArray parseTypeConstructorSpecifierList(CharacterSource* source)
{
    SymbolList list;
    Symbol typeSpecifier = parseTypeConstructorSpecifier(source);
    if (!typeSpecifier.valid())
        return list;
    list.add(typeSpecifier);
    Span span;
    while (Space::parseCharacter(source, ',', &span)) {
        typeSpecifier = parseTypeSpecifier(source);
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

//SymbolArray parseTemplateArgumentList(CharacterSource* source)
//{
//    Span span;
//    if (!Space::parseCharacter(source, '<', &span))
//        return SymbolArray();
//    SymbolArray array = parseTypeSpecifierList(source);
//    if (array.count() == 0)
//        return SymbolArray();
//    if (!Space::parseCharacter(source, '>', &span))
//        return SymbolArray();
//    return array;
//}

//TypeConstructorSignifier := TypeConstructorIdentifier ("<" TemplateParameter \ "*" ">")*     
Symbol parseTypeConstructorSignifier(CharacterSource* source)
{
    CharacterSource s2 = *source;
    Symbol typeConstructorIdentifer = parseTypeConstructorIdentifier(source);
    if (!typeConstructorIdentifer.valid())
        return Symbol();
    SymbolList templateArguments;
    Span span;
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

}

Symbol kindOf(Symbol typeConstructor)
{
    // TODO
}

Symbol parseKindSpecifier(CharaxterSource* source)
{
    Span span;
    if (!Space::parseCharacter(source, '<', &span)) {
        return Symbol(atomTypeKind);
    // TODO
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