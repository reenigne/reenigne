#ifndef INCLUDED_CONFIG_FILE_H
#define INCLUDED_CONFIG_FILE_H

#include "unity/string.h"
#include "unity/hash_table.h"
#include "unity/space.h"

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
        return Symbol(atomIdentifier, name, newSpan(startSpan + endSpan));
    }
    Symbol parseExpression(CharacterSource* source, Symbol type)
    {
        // TODO: parse, type check and evaluate.
        // TODO: 
    }
    void parseAssignment(CharacterSource* source)
    {
        Symbol identifier = parseIdentifier(source);
        Span span;
        String name = identifier[1].string();
        if (!_options.hasKey(name))
            span.throwError("Unknown identifier " + name);
        Space::assertCharacter(source, '=' &span);
        Symbol e = parseExpression(source, _options[name][1].symbol());
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
        return _options[name][2];
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
