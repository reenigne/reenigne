#ifndef INCLUDED_CONFIG_FILE_H
#define INCLUDED_CONFIG_FILE_H

#include "unity/string.h"
#include "unity/hash_table.h"
#include "unity/space.h"
#include "unity/any.h"
#include "unity/type.h"

class ConfigFile
{
public:
    void addType(Type type)
    {
        String name = type.toString();
        _types.add(name, type);
        EnumerationType t(type);
        if (t.valid()) {
            const Array<EnumerationType::Value>* values = t.values();
            for (int i = 0; i < values->count(); ++i) {
                EnumerationType::Value value = (*values)[i];
                _enumeratedValues.add(value.name(),
                    TypedValue(type, value.value()));
            }
        }
    }
    void addOption(String name, Type type)
    {
        _options.add(name, TypedValue(type));
    }
    template<class T> void addOption(String name, Type type, T defaultValue)
    {
        _options.add(name, TypedValue(type, Any(defaultValue)));
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
            Identifier identifier = parseIdentifier(&source);
            String name = identifier.name();
            if (!_options.hasKey(name))
                identifier.span().throwError(
                    String("Unknown identifier ") + name);
            Span span;
            Space::assertCharacter(&source, '=', &span);
            TypedValue e = parseExpression(&source);
            Type expectedType = _options[name].type();
            Type observedType = e.type();
            if (observedType != expectedType)  // TODO: check for conversions
                e.span().throwError(
                    String("Expected an expression of type ") +
                    expectedType.toString() +
                    String(" but found one of type ") +
                    observedType.toString());
            Space::assertCharacter(&source, ';', &span);
            _options[name].setValue(e.value());
        } while (true);
        for (HashTable<String, TypedValue>::Iterator i = _options.begin();
            i != _options.end(); ++i) {
            if (!i.value().valid())
                throw Exception(file.messagePath() + colonSpace + i.key() +
                    String(" not defined and no default is available."));
        }
    }
    template<class T> T getValue(String name)
    {
        return _options[name].value().value<T>();
    }
private:
    class TypedValue
    {
    public:
        TypedValue() { }
        TypedValue(Type type, Any defaultValue = Any(), Span span = Span())
          : _type(type), _value(defaultValue), _span(span) { }
        Type type() const { return _type; }
        Any value() const { return _value; }
        void setValue(Any value) { _value = value; }
        Span span() const { return _span; }
        bool valid() const { return _type.valid(); }
    private:
        Type _type;
        Any _value;
        Span _span;
    };
    class Identifier
    {
    public:
        Identifier() { }
        Identifier(String name, Span span) : _name(name), _span(span) { }
        String name() const { return _name; }
        Span span() const { return _span; }
        bool valid() const { return !_name.empty(); }
    private:
        String _name;
        Span _span;
    };

    Identifier parseIdentifier(CharacterSource* source)
    {
        CharacterSource s = *source;
        Location startLocation = s.location();
        int startOffset = s.offset();
        Span startSpan;
        Span endSpan;
        int c = s.get(&startSpan);
        if (!(c < 'A' || c > 'Z') && !(c < 'a' || c > 'z'))
            return Identifier();
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
        return Identifier(name, startSpan + endSpan);
    }
    void throwError(CharacterSource* source)
    {
        static String expected("Expected expression");
        source->location().throwError(expected);
    }
    
    TypedValue combine(TypedValue left, TypedValue right)
    {
        if (left.valid())
            return TypedValue(StringType(),
                left.value().value<String>() + right.value().value<String>(),
                left.span() + right.span());
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
    
    TypedValue parseDoubleQuotedString(CharacterSource* source)
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
            return TypedValue(StringType(), String());
        Span stringStartSpan = startSpan;
        Span stringEndSpan = startSpan;
        int startOffset = source->offset();
        int endOffset;
        String insert(empty);
        int n;
        int nn;
        String string(empty);
        TypedValue expression;
        TypedValue part;
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
                    return combine(expression, TypedValue(StringType(), string,
                        stringStartSpan + span));
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
                    {
                        Identifier i = parseIdentifier(source);
                        if (!i.valid()) {
                            if (Space::parseCharacter(source, '(', &span)) {
                                part = parseExpression(source);
                                source->assert(')', &span);
                            }
                            else
                                source->location().throwError(String(
                                    "Expected identifier or parenthesized "
                                        "expression"));
                        }
                        String s = i.name();
                        if (s[0] >= 'a' && s[0] <= 'z')
                            part = valueOfIdentifier(i);
                        else
                            i.span().throwError(
                                String("Expected identifier or parenthesized "
                                        "expression"));
                    }
                    if (part.type() == IntegerType())
                        part = TypedValue(StringType(), 
                            String::decimal(part.value().value<int>()),
                            part.span());
                    else
                        if (part.type() != StringType())
                            source->location().throwError(
                                String("Don't know how to convert type ") +
                                part.type().toString() +
                                String(" to a string"));
                    string += s.subString(startOffset, endOffset);
                    startOffset = source->offset();
                    expression = combine(expression, TypedValue(StringType(),
                        string, stringStartSpan + stringEndSpan));
                    string = empty;
                    expression = combine(expression, part);
                    break;
                default:
                    stringEndSpan = span;
            }
        } while (true);
    }
    
    TypedValue parseInteger(CharacterSource* source)
    {
        CharacterSource s = *source;
        int n = 0;
        Span span;
        int c = s.get(&span);
        if (c < '0' || c > '9')
            return TypedValue();
        do {
            n = n*10 + c - '0';
            *source = s;
            Span span2;
            c = s.get(&span2);
            if (c < '0' || c > '9') {
                Space::parse(source);
                return TypedValue(IntegerType(), n, span);
            }
            span += span2;
        } while (true);
    }
    TypedValue valueOfIdentifier(const Identifier& i)
    {
        String s = i.name();
        static String trueKeyword("true");
        if (s == trueKeyword)
            return TypedValue(BooleanType(), true, i.span());
        static String falseKeyword("false");
        if (s == falseKeyword)
            return TypedValue(BooleanType(), false, i.span());
        if (_enumeratedValues.hasKey(s)) {
            TypedValue value = _enumeratedValues[s];
            return TypedValue(value.type(), value.value(), i.span());
        }
        if (!_options.hasKey(s))
            i.span().throwError(String("Unknown identifier ") + s);
        TypedValue option = _options[s];
        return TypedValue(option.type(), option.value(), i.span()); 
    }
    TypedValue parseExpressionElement(CharacterSource* source)
    {
        Location location = source->location();
        TypedValue e = parseDoubleQuotedString(source);
        if (e.valid())
            return e;
        e = parseInteger(source);
        if (e.valid())
            return e;
        Identifier i = parseIdentifier(source);
        if (i.valid()) {
            String s = i.name();
            if (s[0] >= 'a' && s[0] <= 'z')
                return valueOfIdentifier(i);
            // i is a type identifier
            if (!_types.hasKey(s))
                i.span().throwError(String("Unknown type ") + s);
            StructuredType type = _types[s];
            if (!type.valid())
                i.span().throwError(
                    String("Only structure types can be constructed"));
            const Array<StructuredType::Member>* elements = type.members();
            List<TypedValue> values;
            Span span;
            Space::assertCharacter(source, '(', &span);
            for (int i = 0; i < elements->count(); ++i) {
                StructuredType::Member member = (*elements)[i];
                if (i > 0)
                    Space::assertCharacter(source, ',', &span);
                TypedValue value = parseExpression(source);
                Type expectedType = member.type();
                Type observedType = value.type();
                // TODO: Check for conversions
                if (observedType != expectedType)
                    value.span().throwError(String("Type mismatch: ") + s + 
                        dot + member.name() + String(" has type ") + 
                        expectedType.toString() +
                        String(" but value has type ") +
                        observedType.toString());
                values.add(value);
            }
            Space::assertCharacter(source, ')', &span);
            return TypedValue(type, values, e.span() + span);
        }
        Span span;
        if (Space::parseCharacter(source, '{', &span)) {
            List<TypedValue> values;
            Span span2;
            List<Type> types;
            do {
                TypedValue e = parseExpression(source);
                values.add(e);
                types.add(e.type());
            } while (Space::parseCharacter(source, ',', &span2));
            Space::assertCharacter(source, '}', &span2);
            return TypedValue(TupleType(types), values, span + span2);
        }
        if (Space::parseCharacter(source, '(', &span)) {
            e = parseExpression(source);
            Space::assertCharacter(source, ')', &span);
            return e;
        }
        return TypedValue();
    }
    
    TypedValue parseUnaryExpression(CharacterSource* source)
    {
        Span span;
        if (Space::parseCharacter(source, '-', &span)) {
            TypedValue e = parseUnaryExpression(source);
            if (e.type() != IntegerType())
                throw Exception(String("Only numbers can be negated"));
            return TypedValue(IntegerType(), -e.value().value<int>(),
                span + e.span());
        }
        return parseExpressionElement(source);
    }
    
    TypedValue parseMultiplicativeExpression(CharacterSource* source)
    {
        TypedValue e = parseUnaryExpression(source);
        if (!e.valid())
            return e;
        do {
            Span span;
            if (Space::parseCharacter(source, '*', &span)) {
                TypedValue e2 = parseUnaryExpression(source);
                if (!e2.valid())
                    throwError(source);
                bool okay = false;
                if (e.type() == IntegerType()) {
                    if (e2.type() == IntegerType()) {
                        e = TypedValue(IntegerType(),
                            e.value().value<int>() * e2.value().value<int>(),
                            e.span() + e2.span());
                        okay = true;
                    }
                    else
                        if (e2.type() == StringType()) {
                            e = TypedValue(StringType(),
                                e2.value().value<String>() *
                                    e.value().value<int>(),
                                e.span() + e2.span());
                            okay = true;
                        }
                }
                else
                    if (e.type() == StringType()) {
                        if (e2.type() == IntegerType()) {
                            e = TypedValue(StringType(),
                                e.value().value<String>() *
                                    e2.value().value<int>(),
                                e.span() + e2.span());
                            okay = true;
                        }
                    }
                if (!okay) 
                    span.throwError(
                        String("Don't know how to multiply type ") +
                        e.type().toString() + String(" and type ") +
                        e2.type().toString() + String::codePoint('.'));
                continue;
            }
            if (Space::parseCharacter(source, '/', &span)) {
                TypedValue e2 = parseUnaryExpression(source);
                if (!e2.valid())
                    throwError(source);
                if (e.type() == IntegerType() && e2.type() == IntegerType())
                    e = TypedValue(IntegerType(),
                            e.value().value<int>() / e2.value().value<int>(),
                            e.span() + e2.span());
                else
                    span.throwError(
                        String("Don't know how to divide type ") +
                        e.type().toString() + String(" by type ") +
                        e2.type().toString() + String::codePoint('.'));
                continue;
            }
            return e;
        } while (true);
    }
    
    TypedValue parseExpression(CharacterSource* source)
    {
        TypedValue e = parseMultiplicativeExpression(source);
        if (!e.valid())
            throwError(source);
        do {
            Span span;
            if (Space::parseCharacter(source, '+', &span)) {
                TypedValue e2 = parseMultiplicativeExpression(source);
                if (!e2.valid())
                    throwError(source);
                if (e.type() == IntegerType() && e2.type() == IntegerType())
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

    HashTable<String, TypedValue> _options;
    HashTable<String, TypedValue> _enumeratedValues;
    HashTable<String, Type> _types;
};

#endif // INCLUDED_CONFIG_FILE_H
