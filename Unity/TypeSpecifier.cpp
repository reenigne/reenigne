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
    return Symbol(atomTypeIdentifier, name, Span(location, endLocation));
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
    return Symbol(atomClass, Span(start, end));
}

Symbol parseTypeOfTypeSpecifier(CharacterSource* source);

Symbol parseFundamentalTypeSpecifier(CharacterSource* source)
{
    Location start = source->location();
    Symbol typeSpecifier = parseTypeIdentifier(source);
    if (typeSpecifier.valid()) {
        String s = typeSpecifier[1].string();
        static String autoKeyword("Auto");
        if (s == autoKeyword)
            return Symbol(atomAuto, typeSpecifier.span());
        static String bitKeyword("Bit");
        if (s == bitKeyword)
            return Symbol(atomBit, typeSpecifier.span());
        static String booleanKeyword("Boolean");
        if (s == booleanKeyword)
            return Symbol(atomBoolean, typeSpecifier.span());
        static String byteKeyword("Byte");
        if (s == byteKeyword)
            return Symbol(atomByte, typeSpecifier.span());
        static String characterKeyword("Character");
        if (s == characterKeyword)
            return Symbol(atomCharacter, typeSpecifier.span());
        static String intKeyword("Int");
        if (s == intKeyword)
            return Symbol(atomInt, typeSpecifier.span());
        static String stringKeyword("String");
        if (s == stringKeyword)
            return Symbol(atomString, typeSpecifier.span());
        static String uIntKeyword("UInt");
        if (s == uIntKeyword)
            return Symbol(atomUInt, typeSpecifier.span());
        static String voidKeyword("Void");
        if (s == voidKeyword)
            return Symbol(atomVoid, typeSpecifier.span());
        static String wordKeyword("Word");
        if (s == wordKeyword)
            return Symbol(atomVoid, typeSpecifier.span());

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
            typeSpecifier = Symbol(atomPointer, Span(typeSpecifier.span().start(), span.end()), typeSpecifier);
            continue;
        }
        if (Space::parseCharacter(source, '(', span)) {
            SymbolArray typeListSpecifier = parseTypeSpecifierList(source);
            Location end = Space::assertCharacter(source, ')');
            typeSpecifier = Symbol(atomFunction, Span(typeSpecifier.span().start(), end), typeSpecifier, typeListSpecifier);
            continue;
        }
    } while (true);
    return typeSpecifier;
}

#include "Expression.cpp"

Symbol parseTypeOfTypeSpecifier(CharacterSource* source)
{
    Span span;
    static String keyword("TypeOf");
    if (!Space::parseKeyword(source, keyword, span))
        return Symbol();
    Space::assertCharacter(source, '(');
    Symbol expression = parseExpressionOrFail(source);
    Location end = Space::assertCharacter(source, ')');
    return Symbol(atomTypeOf, Span(span.start(), end), expression);
}
