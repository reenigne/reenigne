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
    Symbol parseLValue(CharacterSource* source)
    {
        // TODO: parse
    }
    Symbol parseExpression(CharacterSource* source, Symbol type)
    {
        // TODO: parse, type check and evaluate
    }
    void parseAssignment(CharacterSource* source)
    {
        Symbol identifier = parseLValue(source, &span);
        Span span;
        String name = identifier[1].string();
        if (_options.hasKey(name))
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
        parseAssignment(&source);

        // TODO: Go through all options in _options and check that we have a 
        // value.
        if (!s.valid())
            throw Exception(file.messagePath() + colonSpace + name +
                String(" not defined and no default is available."));
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
