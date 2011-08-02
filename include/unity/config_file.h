#ifndef INCLUDED_CONFIG_FILE_H
#define INCLUDED_CONFIG_FILE_H

#include "unity/string.h"
#include "unity/hash_table.h"
#include "unity/space.h"

class SpanCache : public SymbolCache
{
public:
    SpanCache(Span span) : _span(span) { }
    Span span() const { return _span; }
private:
    Span _span;
};

Span spanOf(Symbol symbol) { return symbol.cache<SpanCache>()->span(); }

SpanCache* newSpan(Span span) { return new SpanCache(span); }

SpanCache* newSpan(Location start, Location end)
{
    return newSpan(Span(start, end));
}

SpanCache* newSpan(Symbol symbol) { return newSpan(spanOf(symbol)); }

class ConfigFile
{
public:
    void addOption(String name, Symbol type, Symbol defaultValue = Symbol())
    {
        _options.add(name, Symbol(atomOption, type, defaultValue));
    }
    Symbol parseIdentifier(CharacterSource* source)
    {
        CharacterSource s = *source;
        Location startLocation = s.location();
        int startOffset;
        Span startSpan;
        Span endSpan;
        int c = s.get(&startSpan);
        if (c < 'a' || c > 'z')
            return Symbol();
        CharacterSource s2;
        do {
            s2 = s;
            c = s.get(&endSpan);
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c == '_')
                continue;
            break;
        } while (true);
        int endOffset = s2.offset();
        Location endLocation = s2.location();
        Space::parse(&s2);
        String name = s2.subString(startOffset, endOffset);
        *source = s2;
        return Symbol(atomIdentifier, name, newSpan(startSpan + endSpan));
    }
    Symbol valueFromIdentifier(Symbol identifier)
    {
        String name = identifier[1].string();
        if (!_options.hasKey(name))
            spanOf(identifier).throwError(String("Unknown identifier ") + name);
        Symbol option = _options[name];
        return Symbol(atomValue, option[2].symbol(), option[1].symbol(),
            newSpan(identifier));
    }
    Symbol parseTypeIdentifier(CharacterSource* source)
    {
        CharacterSource s = *source;
        Location startLocation = s.location();
        int startOffset;
        Span startSpan;
        Span endSpan;
        int c = s.get(&startSpan);
        if (c < 'A' || c > 'Z')
            return Symbol();
        CharacterSource s2;
        do {
            s2 = s;
            c = s.get(&endSpan);
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c == '_')
                continue;
            break;
        } while (true);
        int endOffset = s2.offset();
        Location endLocation = s2.location();
        Space::parse(&s2);
        String name = s2.subString(startOffset, endOffset);
        *source = s2;
        return Symbol(atomIdentifier, name);
    }
    
    void throwError(CharacterSource* source)
    {
        static String expected("Expected expression");
        source->location().throwError(expected);
    }
    
    Symbol combine(Symbol left, Symbol right)
    {
        if (left.valid())
            return Symbol(atomValue, left[1].string() + right[1].string(),
                Symbol(atomString), newSpan(spanOf(left) + spanOf(right)));
        return right;
    }
    
    int parseHexadecimalCharacter(CharacterSource* source, Span* span)
    {
        CharacterSource s = *source;
        int c = s.get(span);
        if (c >= '0' && c <= '9') {
            *source = s;
            return c - '0';
        }
        if (c >= 'A' && c <= 'F') {
            *source = s;
            return c + 10 - 'A';
        }
        if (c >= 'a' && c <= 'f') {
            *source = s;
            return c + 10 - 'a';
        }
        return -1;
    }
    
    Symbol parseDoubleQuotedString(CharacterSource* source)
    {
        static String endOfFile("End of file in string");
        static String endOfLine("End of line in string");
        static String printableCharacter("printable character");
        static String escapedCharacter("escaped character");
        static String hexadecimalDigit("hexadecimal digit");
        static String toString("toString");
        Span span;
        Span startSpan;
        if (!source->parse('"', &startSpan))
            return Symbol();
        Span stringStartSpan = startSpan;
        Span stringEndSpan = startSpan;
        int startOffset = source->offset();
        int endOffset;
        String insert(empty);
        int n;
        int nn;
        String string(empty);
        Symbol expression;
        Symbol part;
        do {
            CharacterSource s = *source;
            endOffset = s.offset();
            int c = s.get(&span);
            if (c < 0x20 && c != 10) {
                if (c == -1)
                    source->location().throwError(endOfFile);
                source->throwUnexpected(printableCharacter,
                    String::hexadecimal(c, 2));
            }
            *source = s;
            switch (c) {
                case '"':
                    string += s.subString(startOffset, endOffset);
                    Space::parse(source);
                    return combine(expression, Symbol(atomValue, string,
                        newSpan(stringStartSpan + span)));
                case '\\':
                    string += s.subString(startOffset, endOffset);
                    c = s.get(&stringEndSpan);
                    if (c < 0x20) {
                        if (c == 10)
                            source->location().throwError(endOfLine);
                        if (c == -1)
                            source->location().throwError(endOfFile);
                        source->throwUnexpected(escapedCharacter,
                            String::hexadecimal(c, 2));
                    }
                    *source = s;
                    switch (c) {
                        case 'n':
                            insert = newLine;
                            break;
                        case 't':
                            insert = tab;
                            break;
                        case '$':
                            insert = dollar;
                            break;
                        case '"':
                            insert = doubleQuote;
                            break;
                        case '\'':
                            insert = singleQuote;
                            break;
                        case '`':
                            insert = backQuote;
                            break;
                        case 'U':
                            source->assert('+', &stringEndSpan);
                            n = 0;
                            for (int i = 0; i < 4; ++i) {
                                nn = parseHexadecimalCharacter(source,
                                    &stringEndSpan);
                                if (nn == -1) {
                                    s = *source;
                                    source->throwUnexpected(hexadecimalDigit,
                                        String::codePoint(s.get()));
                                }
                                n = (n << 4) | nn;
                            }
                            nn = parseHexadecimalCharacter(source,
                                &stringEndSpan);
                            if (nn != -1) {
                                n = (n << 4) | nn;
                                nn = parseHexadecimalCharacter(source,
                                    &stringEndSpan);
                                if (nn != -1)
                                    n = (n << 4) | nn;
                            }
                            insert = String::codePoint(n);
                            break;
                        default:
                            source->throwUnexpected(escapedCharacter,
                                String::codePoint(c));
                    }
                    string += insert;
                    startOffset = source->offset();
                    break;
                case '$':
                    part = parseIdentifier(source);
                    if (!part.valid()) {
                        if (Space::parseCharacter(source, '(', &span)) {
                            part = parseExpression(source);
                            source->assert(')', &span);
                        }
                        else
                            source->location().throwError(String(
                                "Expected identifier or parenthesized "
                                    "expression"));
                    }
                    if (part[2].atom() == atomInteger)
                        part = Symbol(atomValue,
                            String::decimal(part[1].integer()),
                            Symbol(atomString), newSpan(part));
                    else
                        if (part[2].atom() != atomString)
                            source->location().throwError(
                                String("Don't know how to convert type ") +
                                typeToString(part[2].symbol()) +
                                String(" to a string"));
                    string += s.subString(startOffset, endOffset);
                    startOffset = source->offset();
                    expression = combine(expression,
                        Symbol(atomValue, string, Symbol(atomString),
                        newSpan(stringStartSpan + stringEndSpan)));
                    string = empty;
                    expression = combine(expression, part);
                    break;
                default:
                    stringEndSpan = span;
            }
        } while (true);
    }
    
    Symbol parseInteger(CharacterSource* source)
    {
        CharacterSource s = *source;
        int n = 0;
        Span span;
        int c = s.get(&span);
        if (c < '0' || c > '9')
            return Symbol();
        do {
            n = n*10 + c - '0';
            *source = s;
            Span span2;
            c = s.get(&span2);
            if (c < '0' || c > '9') {
                Space::parse(source);
                return Symbol(atomValue, n, Symbol(atomInteger),
                    newSpan(span));
            }
            span += span2;
        } while (true);
    }
    
    Symbol parseExpressionElement(CharacterSource* source)
    {
        Location location = source->location();
        Symbol e = parseDoubleQuotedString(source);
        if (e.valid())
            return e;
        e = parseInteger(source);
        if (e.valid())
            return e;
        e = parseIdentifier(source);
        if (e.valid()) {
            String s = e[1].string();
            static String trueKeyword("true");
            if (s == trueKeyword)
                return Symbol(atomValue, Symbol(atomTrue), Symbol(atomBoolean),
                    newSpan(e));
            static String falseKeyword("false");
            if (s == falseKeyword)
                return Symbol(atomValue, Symbol(atomFalse),
                    Symbol(atomBoolean), newSpan(e));
            // TODO: Check for enum constants
            return valueFromIdentifier(e);
        }
        // TODO: Check for structure constructor calls
        Span span;
        if (Space::parseCharacter(source, '(', &span)) {
            e = parseExpression(source);
            Space::assertCharacter(source, ')', &span);
            return e;
        }
        return Symbol();
    }
    
    Symbol parseUnaryExpression(CharacterSource* source)
    {
        Span span;
        if (Space::parseCharacter(source, '-', &span)) {
            Symbol e = parseUnaryExpression(source);
            if (e.atom() != atomInteger)
                throw Exception(String("Only numbers can be negated"));
            return Symbol(atomInteger, -e[1].integer());
        }
        return parseExpressionElement(source);
    }
    
    Symbol parseMultiplicativeExpression(CharacterSource* source)
    {
        Symbol e = parseUnaryExpression(source);
        if (!e.valid())
            return Symbol();
        do {
            Span span;
            if (Space::parseCharacter(source, '*', &span)) {
                Symbol e2 = parseUnaryExpression(source);
                if (!e2.valid())
                    throwError(source);
                bool okay = false;
                if (e[2].atom() == atomInteger) {
                    if (e2[2].atom() == atomInteger) {
                        e = Symbol(atomValue, e[1].integer() * e2[1].integer(),
                            Symbol(atomInteger),
                            newSpan(spanOf(e) + spanOf(e2)));
                        okay = true;
                    }
                    else
                        if (e2[2].atom() == atomString) {
                            e = Symbol(atomValue,
                                e2[1].string() * e[1].integer(),
                                Symbol(atomString),
                                newSpan(spanOf(e) + spanOf(e2)));
                            okay = true;
                        }
                }
                else
                    if (e[2].atom() == atomString) {
                        if (e2[2].atom() == atomInteger) {
                            e = Symbol(atomValue,
                                e[1].string() * e2[1].integer(),
                                Symbol(atomString),
                                newSpan(spanOf(e) + spanOf(e2)));
                            okay = true;
                        }
                    }
                if (!okay) 
                    span.throwError(
                        String("Don't know how to multiply type ") +
                        typeToString(e[2].symbol()) +
                        String(" and type ") +
                        typeToString(e2[2].symbol()) +
                        String::codePoint('.'));
                continue;
            }
            if (Space::parseCharacter(source, '/', &span)) {
                Symbol e2 = parseUnaryExpression(source);
                if (!e2.valid())
                    throwError(source);
                if (e[2].atom() == atomInteger && e2[2].atom() == atomInteger)
                    e = Symbol(atomValue, e[1].integer() / e2[1].integer(),
                        Symbol(atomInteger), newSpan(spanOf(e) + spanOf(e2)));
                else
                    span.throwError(
                        String("Don't know how to divide type ") +
                        typeToString(e[2].symbol()) +
                        String(" by type ") +
                        typeToString(e2[2].symbol()) +
                        String::codePoint('.'));
                continue;
            }
            return e;
        } while (true);
    }
    
    Symbol parseExpression(CharacterSource* source)
    {
        Symbol e = parseMultiplicativeExpression(source);
        if (!e.valid())
            throwError(source);
        do {
            Span span;
            if (Space::parseCharacter(source, '+', &span)) {
                Symbol e2 = parseMultiplicativeExpression(source);
                if (!e2.valid())
                    throwError(source);
                if (e[2].symbol().atom() == atomInteger &&
                    e2[2].symbol().atom() == atomInteger)
                    e = Symbol(atomValue, e[1].integer() + e2[1].integer(),
                        Symbol(atomInteger), newSpan(spanOf(e) + spanOf(e2)));
                else
                    if (e[2].atom() == atomString &&
                        e2[2].atom() == atomString)
                        e = Symbol(atomValue, e[1].string() + e2[1].string(),
                            Symbol(atomString),
                            newSpan(spanOf(e) + spanOf(e2)));
                    else
                        span.throwError(String("Don't know how to add type ") +
                            typeToString(e2[2].symbol()) +
                            String(" to type ") + typeToString(e[2].symbol()) +
                            String::codePoint('.'));
                continue;
            }
            if (Space::parseCharacter(source, '-', &span)) {
                Symbol e2 = parseMultiplicativeExpression(source);
                if (!e2.valid())
                    throwError(source);
                if (e[2].atom() == atomInteger && e2[2].atom() == atomInteger)
                    e = Symbol(atomValue,
                        e[1].integer() - e2[1].integer(), Symbol(atomInteger),
                        newSpan(spanOf(e) + spanOf(e2)));
                else
                    span.throwError(
                        String("Don't know how to subtract type ") +
                        typeToString(e2[2].symbol()) + String(" from type ") +
                        typeToString(e[2].symbol()) + String::codePoint('.'));
                continue;
            }
            return e;
        } while (true);
    }
    String typeToString(Symbol type)
    {
        return atomToString(type.atom());
    }
    void parseAssignment(CharacterSource* source)
    {
        Symbol identifier = parseIdentifier(source);
        Span span;
        String name = identifier[1].string();
        if (!_options.hasKey(name))
            span.throwError(String("Unknown identifier ") + name);
        Space::assertCharacter(source, '=', &span);
        Symbol e = parseExpression(source);
        Symbol expectedType = _options[name][1].symbol();
        Symbol observedType = e[2].symbol();
        if (observedType != expectedType)
            spanOf(e).throwError(String("Expected an expression of type ") +
                typeToString(expectedType) +
                String(" but found one of type ") +
                typeToString(observedType));
        Space::assertCharacter(source, ';', &span);
        _options[name][2] = e;
    }
    void load(File file)
    {
        String contents = file.contents();
        CharacterSource source(contents, file.path());
        Space::parse(&source);
        do {
            CharacterSource s = source;
            if (s.get() == -1)
                break;
            parseAssignment(&source);
        } while (true);
        for (HashTable<String, Symbol>::Iterator i = _options.begin();
            i != _options.end(); ++i) {
            if (!i.value()[2].valid())
                throw Exception(file.messagePath() + colonSpace + i.key() +
                    String(" not defined and no default is available."));
        }
    }
    Atom getAtom(String name)
    {
        return getSymbol(name).atom();
    }
    String getString(String name)
    {
        return getSymbol(name)[1].string();
    }
    Symbol getSymbol(String name)
    {
        return _options[name][2].symbol();
    }
    bool getBoolean(String name)
    {
        return getAtom(name) != atomFalse;
    }
    int getInteger(String name)
    {
        return getSymbol(name)[1].integer();
    }
private:
    HashTable<String, Symbol> _options;
};

#endif // INCLUDED_CONFIG_FILE_H
