#include "alfe/main.h"

#ifndef INCLUDED_CONFIG_FILE_H
#define INCLUDED_CONFIG_FILE_H

#include "alfe/hash_table.h"
#include "alfe/space.h"
#include "alfe/any.h"
#include "alfe/type.h"
#include "alfe/value.h"
#include "alfe/set.h"
#include "alfe/expression.h"

class ConfigFile : public Structure
{
public:
    ConfigFile() : _context(this)
    {
        FunctionTyco iii =
            FunctionTyco(IntegerType(), IntegerType(), IntegerType());
        //addFunction(OperatorPlus(), iii, );
    }
    void addOption(String name, Type type)
    {
        _options.add(name, TypedValue(type));
    }
    template<class T> void addDefaultOption(String name, Type type,
        const T& defaultValue)
    {
        _options.add(name, TypedValue(type, Any(defaultValue)));
    }
    template<class T> void addDefaultOption(String name, const T& defaultValue)
    {
        _options.add(name, TypedValue(defaultValue));
    }
    void addType(Type type) { _types.add(type.toString(), type); }
    void addFunction(Identifier identifier, FunctionTyco type,
        Function* function)
    {
        //if (_options.hasKey(identifier))

    }

    void load(File file)
    {
        _file = file;
        String contents = file.contents();
        CharacterSource source(contents, file.path());
        Space::parse(&source);
        do {
            CharacterSource s = source;
            if (s.get() == -1)
                break;
            s = source;
            Span span;
            if (Space::parseKeyword(&s, "include", &span)) {
                TypedValue e = Expression::parse(&s).evaluate(&_context).
                    convertTo(StringType());
                Space::assertCharacter(&s, ';', &span);
                load(e.value<String>());
                source = s;
                continue;
            }
            TycoSpecifier tycoSpecifier = TycoSpecifier::parse(&s);
            if (tycoSpecifier.valid()) {
                TycoIdentifier identifier(tycoSpecifier);
                if (!identifier.valid()) {
                    tycoSpecifier.span().throwError(
                        "Don't understand this type specifier");
                }
                String name = identifier.name();
                if (!_types.hasKey(name))
                    identifier.span().throwError("Unknown type " + name);
                Type type = _types[name];
                Identifier objectIdentifier = Identifier::parse(&s);
                if (objectIdentifier.isOperator()) {
                    identifier.span().throwError("Cannot create an object "
                        "with operator name");
                }
                String objectName = objectIdentifier.name();
                if (_options.hasKey(name))
                    objectIdentifier.span().throwError(name +
                        " already exists");
                TypedValue value = TypedValue(
                    StructuredType(String(), List<StructuredType::Member>()),
                    Value<HashTable<Identifier, TypedValue>>());
                if (Space::parseCharacter(&s, '=', &span))
                    value = Expression::parse(&s).evaluate(&_context);
                Space::assertCharacter(&s, ';', &span);
                source = s;
                value = value.convertTo(type);
                if (type.has(Identifier("*"))) {
                    // This special member is how ConfigFile tells created
                    // objects their names so that they can responsible for
                    // persistence so that this functionality doesn't need to
                    // be in ConfigFile.
                    // Using an actual identifier here would lead to collisions
                    // with real members. ALFE persistence is done by types 
                    // knowing how to persist themselves.
                    // I also don't want to use the empty string, since I might
                    // want to use that as the connector name for
                    // single-connector components.
                    value.value<Structure*>()->set(Identifier("*"), name);
                }

                continue;
            }
            Identifier identifier = Identifier::parse(&s);
            if (!identifier.valid()) {
                s.location().throwError("Expected an include statement, an "
                    "object creation statement or an assignment statement.");
            }
            String name = identifier.name();
            TypedValue left =
                Expression::parseDot(&source).evaluate(&_context);
            Space::assertCharacter(&source, '=', &span);
            TypedValue loadedExpression =
                Expression::parse(&source).evaluate(&_context);
            LValueType lValueType(left.type());
            if (!lValueType.valid())
                left.span().throwError("LValue required");
            Type type = lValueType.inner();
            LValue p = left.value<LValue>();
            TypedValue e = loadedExpression.rValue().convertTo(type);
            Space::assertCharacter(&source, ';', &span);
            p.set(e);
            _options[name].setValue(e.value());
        } while (true);
        for (auto i = _options.begin(); i != _options.end(); ++i) {
            if (!i.value().valid())
                throw Exception(file.path() + ": " + i.key().name() +
                    " not defined and no default is available.");
        }
    }

    TypedValue getValue(Identifier identifier)
    {
        return _options[identifier].rValue();
    }
    void set(Identifier identifier, TypedValue value)
    {
        _options[identifier] = value;
    }
    File file() const { return _file; }

    TypedValue valueOfIdentifier(Identifier i)
    {
        String s = i.name();
        if (_enumeratedValues.hasKey(s)) {
            TypedValue value = _enumeratedValues[s];
            return TypedValue(value.type(), value.value(), i.span());
        }
        if (!_options.hasKey(s))
            i.span().throwError("Unknown identifier " + s);
        return TypedValue(LValueType::wrap(_options[s].type()),
            LValue(this, s), i.span());
    }
    Tyco resolveTycoIdentifier(TycoIdentifier i)
    {
        String s = i.name();
        if (!_types.hasKey(s))
            return Tyco();
        return _types[s];
    }
private:

    class EvaluationContext : public ::EvaluationContext
    {
    public:
        EvaluationContext(ConfigFile* configFile) : _configFile(configFile) { }
        TypedValue valueOfIdentifier(Identifier i)
        {
            return _configFile->valueOfIdentifier(i);
        }
        Tyco resolveTycoIdentifier(TycoIdentifier i)
        {
            return _configFile->resolveTycoIdentifier(i);
        }
    private:
        ConfigFile* _configFile;
    };

    HashTable<Identifier, TypedValue> _options;
    HashTable<Identifier, TypedValue> _enumeratedValues;
    HashTable<TycoIdentifier, Type> _types;
    File _file;
    EvaluationContext _context;
};

#endif // INCLUDED_CONFIG_FILE_H
