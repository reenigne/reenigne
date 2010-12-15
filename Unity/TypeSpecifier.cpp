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
    DiagnosticLocation endLocation = s2.location();
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
    return Symbol(atomTypeIdentifier, DiagnosticSpan(location, endLocation), name);
}

Symbol parseClassTypeSpecifier(CharacterSource* source)
{
    static String keyword("Class");
    DiagnosticLocation start = source->location();
    if (!Space::parseKeyword(source, keyword))
        return Symbol();
    Space::assertCharacter(source, '{');
    // TODO: Parse class contents
    DiagnosticLocation end = Space::assertCharacter(source, '}');
    return Symbol(atomClass, DiagnosticSpan(start, end));
}

Symbol parseFundamentalTypeSpecifier(CharacterSource* source)
{
    DiagnosticLocation start = source->location();
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

SymbolList parseTypeListSpecifier2(Symbol typeSpecifier, CharacterSource* source)
{
    DiagnosticSpan span;
    if (!Space::parseCharacter(source, ',', span))
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
        DiagnosticSpan span;
        if (Space::parseCharacter(source, '*', span)) {
            typeSpecifier = Symbol(atomPointer, DiagnosticSpan(typeSpecifier.span().start(), span.end()), typeSpecifier);
            continue;
        }
        if (Space::parseCharacter(source, '(', span)) {
            SymbolList typeListSpecifier = parseTypeListSpecifier(source);
            DiagnosticLocation end = Space::assertCharacter(source, ')');
            typeSpecifier = Symbol(atomFunction, DiagnosticSpan(typeSpecifier.span().start(), end), typeSpecifier, typeListSpecifier);
            continue;
        }
    } while (true);
    return typeSpecifier;
}

#include "Expression.cpp"

Symbol parseTypeOfTypeSpecifier(CharacterSource* source)
{
    DiagnosticSpan span;
    static String keyword("TypeOf");
    if (!Space::parseKeyword(source, keyword, span))
        return Symbol();
    Space::assertCharacter(source, '(');
    Symbol expression = parseExpression(source);
    if (!expression.valid()) {
        static String error("Expression expected");
        source->location().throwError(error);
    }
    DiagnosticLocation end = Space::assertCharacter(source, ')');
    return Symbol(atomTypeOf, DiagnosticSpan(span.start(), end), expression);
}