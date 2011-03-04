Symbol parseTypeIdentifier(CharacterSource* source)
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
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')
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
    return Symbol(atomTypeIdentifier, name,
        new IdentifierCache(Span(location, endLocation)));
}

Symbol parseClassTypeSpecifier(CharacterSource* source)
{
    static String keyword("Class");
    Location start = source->location();
    Span span;
    if (!Space::parseKeyword(source, keyword, span))
        return Symbol();
    Space::assertCharacter(source, '{');
    // TODO: Parse class contents
    Location end = Space::assertCharacter(source, '}');
    return Symbol(atomClass, new TypeCache(Span(start, end), -1, -1,
        Symbol::newLabel()));
}

Symbol parseTypeOfTypeSpecifier(CharacterSource* source);

Symbol parseFundamentalTypeSpecifier(CharacterSource* source)
{
    Location start = source->location();
    Symbol typeSpecifier = parseTypeIdentifier(source);
    if (typeSpecifier.valid()) {
        String s = typeSpecifier[1].string();
        Span span = spanOf(typeSpecifier);
        static String autoKeyword("Auto");
        if (s == autoKeyword)
            return Symbol(atomAuto, newSpan(span));
        static String bitKeyword("Bit");
        if (s == bitKeyword)
            return Symbol(atomBit, new TypeCache(span, 1, 1));
        static String booleanKeyword("Boolean");
        if (s == booleanKeyword)
            return Symbol(atomBoolean, new TypeCache(span, 1, 1));
        static String byteKeyword("Byte");
        if (s == byteKeyword)
            return Symbol(atomByte, new TypeCache(span, 1, 1));
        static String characterKeyword("Character");
        if (s == characterKeyword)
            return Symbol(atomCharacter, new TypeCache(span, 4, 4));
        static String intKeyword("Int");
        if (s == intKeyword)
            return Symbol(atomInt, new TypeCache(span, 4, 4));
        static String stringKeyword("String");
        if (s == stringKeyword)
            return Symbol(atomString, new TypeCache(span, 4, 4));
        static String uIntKeyword("UInt");
        if (s == uIntKeyword)
            return Symbol(atomUInt, new TypeCache(span, 4, 4));
        static String voidKeyword("Void");
        if (s == voidKeyword)
            return Symbol(atomVoid, new TypeCache(span, 0, 0));
        static String wordKeyword("Word");
        if (s == wordKeyword)
            return Symbol(atomVoid, new TypeCache(span, 4, 4));

        return typeSpecifier;
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

SymbolArray parseTypeSpecifierList(CharacterSource* source)
{
    SymbolList list;
    Symbol typeSpecifier = parseTypeSpecifier(source);
    if (!typeSpecifier.valid())
        return list;
    list.add(typeSpecifier);
    Span span;
    while (Space::parseCharacter(source, ',', span)) {
        typeSpecifier = parseTypeSpecifier(source);
        if (!typeSpecifier.valid()) {
            static String error("Type specifier expected");
            source->location().throwError(error);
        }
        list.add(typeSpecifier);
    }
    return list;
}

Symbol parseTypeSpecifier(CharacterSource* source)
{
    Symbol typeSpecifier = parseFundamentalTypeSpecifier(source);
    if (!typeSpecifier.valid())
        return Symbol();
    do {
        Span span;
        if (Space::parseCharacter(source, '*', span)) {
            typeSpecifier = Symbol(atomPointer, typeSpecifier,
                new TypeCache(Span(spanOf(typeSpecifier).start(), span.end()), 4, 4));
            continue;
        }
        if (Space::parseCharacter(source, '(', span)) {
            SymbolArray typeListSpecifier = parseTypeSpecifierList(source);
            Location end = Space::assertCharacter(source, ')');
            typeSpecifier = Symbol(atomFunction, typeSpecifier,
                typeListSpecifier, new TypeCache(Span(spanOf(typeSpecifier).start(), end), 0, 0));
            continue;
        }
    } while (true);
    return typeSpecifier;
}

Symbol parseTypeOfTypeSpecifier(CharacterSource* source)
{
    Span span;
    static String keyword("TypeOf");
    if (!Space::parseKeyword(source, keyword, span))
        return Symbol();
    Space::assertCharacter(source, '(');
    Symbol expression = parseExpressionOrFail(source);
    Location end = Space::assertCharacter(source, ')');
    return Symbol(atomTypeOf, expression, newSpan(span.start(), end));
}

SymbolArray parseTemplateArgumentList(CharacterSource* source)
{
    Span span;
    if (!Space::parseCharacter(source, '<', span))
        return SymbolArray();
    SymbolArray array = parseTypeSpecifierList(source);
    if (array.count() == 0)
        return SymbolArray();
    if (!Space::parseCharacter(source, '>', span))
        return SymbolArray();
    return array;
}
