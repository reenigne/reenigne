Symbol parseTypeIdentifier(CharacterSource* source)
{
    CharacterSource s = *source;
    DiagnosticLocation location = s.location();
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
    Space::parse(&s2);
    String name = s2.subString(start, end);
    static String keywords[] = {
        String("Auto"),
        String("Bit"),
        String("Boolean"),
        String("Byte"),
        String("Character"),
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
        String("Int"),
        String("Integer"),
        String("QInt"),
        String("QUInt"),
        String("QWord"),
        String("Rational"),
        String("String"),
        String("TypeOf"),
        String("UInt"),
        String("Unsigned"),
        String("Word"),
        String("WordString")
    };
    for (int i = 0; i < sizeof(keywords)/sizeof(keywords[0]); ++i)
        if (name == keywords[i])
            return Symbol();
    *source = s2;
    return Symbol(atomTypeIdentifier, name, symbolFromLocation(location));
}

Symbol parseClassTypeSpecifier(CharacterSource* source)
{
    static String keyword("Class");
    if (!Space::parseKeyword(source, keyword))
        return Symbol();
    Space::assertCharacter(source, '{');
    // TODO: Parse class contents
    Space::assertCharacter(source, '}');
    return Symbol(atomClass);
}

Symbol parseFundamentalTypeSpecifier(CharacterSource* source)
{
    Symbol typeSpecifier = parseTypeIdentifier(source);
    if (typeSpecifier.valid())
        return typeSpecifier;
    static String autoKeyword("Auto");
    if (Space::parseKeyword(source, autoKeyword))
        return Symbol(atomAuto);
    static String bitKeyword("Bit");
    if (Space::parseKeyword(source, bitKeyword))
        return Symbol(atomBit);
    static String booleanKeyword("Boolean");
    if (Space::parseKeyword(source, booleanKeyword))
        return Symbol(atomBoolean);
    static String byteKeyword("Byte");
    if (Space::parseKeyword(source, byteKeyword))
        return Symbol(atomByte);
    static String characterKeyword("Character");
    if (Space::parseKeyword(source, characterKeyword))
        return Symbol(atomCharacter);
    typeSpecifier = parseClassTypeSpecifier(source);
    if (typeSpecifier.valid())
        return typeSpecifier;
    static String intKeyword("Int");
    if (Space::parseKeyword(source, intKeyword))
        return Symbol(atomInt);
    static String stringKeyword("String");
    if (Space::parseKeyword(source, stringKeyword))
        return Symbol(atomString);
    typeSpecifier = parseTypeOfTypeSpecifier(source);
    if (typeSpecifier.valid())
        return typeSpecifier;
    static String uIntKeyword("UInt");
    if (Space::parseKeyword(source, uIntKeyword))
        return Symbol(atomUInt);
    static String voidKeyword("Void");
    if (Space::parseKeyword(source, voidKeyword))
        return Symbol(atomVoid);
    static String wordKeyword("Word");
    if (Space::parseKeyword(source, wordKeyword))
        return Symbol(atomVoid);
    return Symbol();
}

SymbolList parseTypeListSpecifier2(Symbol typeSpecifier, CharacterSource* source)
{
    if (!Space::parseCharacter(source, ','))
        return SymbolList(typeSpecifier);
    Symbol typeSpecifier2 = parseTypeSpecifier(source);
    if (!typeSpecifier2.valid()) {
        static String error("Type specifier expected");
        source->location().throwError(error);
    }
    return SymbolList(typeSpecifier, parseTypeListSpecifier2(typeSpecifier2, source));
}

SymbolList parseTypeListSpecifier(CharacterSource* source)
{
    Symbol typeSpecifier = parseTypeSpecifier(source);
    if (!typeSpecifier.valid())
        return SymbolList();
    return parseTypeListSpecifier2(typeSpecifier, source);
}

Symbol parseTypeSpecifier(CharacterSource* source)
{
    Symbol typeSpecifier = parseFundamentalTypeSpecifier(source);
    if (!typeSpecifier.valid())
        return Symbol();
    do {
        if (Space::parseCharacter(source, '*')) {
            typeSpecifier = Symbol(atomPointer, typeSpecifier);
            continue;
        }
        if (Space::parseCharacter(source, '(')) {
            typeSpecifier = Symbol(atomFunction, typeSpecifier, parseTypeListSpecifier(source));
            Space::assertCharacter(source, ')');
            continue;
        }
    } while (true);
    return typeSpecifier;
}

#include "Expression.cpp"

Symbol parseTypeOfTypeSpecifier(CharacterSource* source)
{
    static String keyword("TypeOf");
    if (!Space::parseKeyword(source, keyword))
        return Symbol();
    Space::assertCharacter(source, '(');
    Symbol expression = parseExpression(source);
    if (!expression.valid()) {
        static String error("Expression expected");
        source->location().throwError(error);
    }
    Space::assertCharacter(source, ')');
    return Symbol(atomTypeOf, expression);
}