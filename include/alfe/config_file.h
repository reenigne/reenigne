#include "alfe/main.h"

#ifndef INCLUDED_CONFIG_FILE_H
#define INCLUDED_CONFIG_FILE_H

#include "alfe/hash_table.h"
#include "alfe/space.h"
#include "alfe/any.h"
#include "alfe/type.h"
#include "alfe/set.h"
#include "alfe/expression.h"
#include "alfe/function.h"
#include "alfe/integer_functions.h"
#include "alfe/string_functions.h"
#include "alfe/rational_functions.h"
#include "alfe/concrete_functions.h"
#include "alfe/array_functions.h"
#include "alfe/boolean_functions.h"

template<class T> class ConfigFileT;
typedef ConfigFileT<void> ConfigFile;

template<class T, class V> class ConfigOptionT
{
public:
    T get() { return _config->template get<T>(_member); }
private:
    ConfigOptionT(ConfigFile* config, String member)
      : _config(config), _member(member) { }
    ConfigFileT<T>* _config;
    String _member;

    friend class ConfigFileT<T>;
};

template<class V> using ConfigOption = ConfigOptionT<void, V>;

class DispatchFunction : public Function
{
public:
    class Body : public Function::Body
    {
    public:
        virtual Value evaluate(List<Value> arguments) const = 0;
        virtual Identifier identifier() const = 0;
        virtual Value value() const = 0;
    };
};

template<class T> class ConfigFileT : public Structure
{
public:
    ConfigFileT() : _context(this)
    {
        addType(IntegerType());
        addFunco(AddIntegerInteger());
        addFunco(SubtractIntegerInteger());
        addFunco(MultiplyIntegerInteger());
        addFunco(AddStringString());
        addFunco(MultiplyIntegerString());
        addFunco(MultiplyStringInteger());
        addFunco(AddRationalRational());
        addFunco(AddRationalInteger());
        addFunco(AddIntegerRational());
        addFunco(SubtractRationalRational());
        addFunco(SubtractRationalInteger());
        addFunco(SubtractIntegerRational());
        addFunco(MultiplyRationalRational());
        addFunco(MultiplyRationalInteger());
        addFunco(MultiplyIntegerRational());
        addFunco(DivideRationalRational());
        addFunco(DivideRationalInteger());
        addFunco(DivideIntegerRational());
        addFunco(DivideIntegerInteger());
        addFunco(AddConcreteConcrete());
        addFunco(SubtractConcreteConcrete());
        addFunco(MultiplyConcreteConcrete());
        addFunco(MultiplyConcreteRational());
        addFunco(MultiplyRationalConcrete());
        addFunco(DivideConcreteConcrete());
        addFunco(DivideConcreteRational());
        addFunco(DivideRationalConcrete());
        addFunco(ShiftLeftIntegerInteger());
        addFunco(ShiftRightIntegerInteger());
        addFunco(ShiftLeftRationalInteger());
        addFunco(ShiftRightRationalInteger());
        addFunco(ShiftLeftConcreteInteger());
        addFunco(ShiftRightConcreteInteger());
        addFunco(PowerIntegerInteger());
        addFunco(PowerRationalInteger());
        addFunco(NegativeRational());
        addFunco(IndexArray());
        addFunco(LogicalNotBoolean());
        addFunco(LessThanIntegerInteger());
        addFunco(GreaterThanIntegerInteger());
        addFunco(LessThanOrEqualToIntegerInteger());
        addFunco(GreaterThanOrEqualToIntegerInteger());
        addFunco(NegativeInteger());
    }
    template<class V> ConfigOption<V> addOption(String name)
    {
        addOption(name, typeFromCompileTimeType<V>());
        return ConfigOption<V>(this, name);
    }
    void addOption(String name, Type type)
    {
        addOption(name, Value(type));
    }
    template<class V> void addDefaultOption(String name, Type type,
        const V& defaultValue)
    {
        auto r = Reference<Structure>::create<Structure>();
        LValue l(&*r, name);
        addOption(name, Value(LValueType::wrap(type), l));
    }
    template<class V> ConfigOption<V> addDefaultOption(String name,
        const V& defaultValue)
    {
        addOption(name, Value(defaultValue));
        return ConfigOption<V>(this, name);
    }
private:
    void addOption(String name, Value defaultValue)
    {
        set(name, defaultValue, Span());
    }
public:
    void addType(Type type, TycoIdentifier identifier = TycoIdentifier())
    {
        if (!identifier.valid())
            identifier = TycoIdentifier(type.toString());
        _types.add(identifier, type);
    }
    void addFunco(Funco funco)
    {
        Identifier identifier = funco.identifier();
        if (!has(identifier))
            set(identifier, OverloadedFunctionSet(identifier), Span());
        get<OverloadedFunctionSet>(identifier).add(funco);
    }

    void load(File file)
    {
        _file = file;
        String contents = file.contents();
        CharacterSource source(contents, file);
        Space::parse(&source);
        do {
            CharacterSource s = source;
            if (s.get() == -1)
                break;
            s = source;
            Span span;
            if (Space::parseKeyword(&s, "include", &span)) {
                Value e = Expression::parse(&s).evaluate(&_context).
                    convertTo(StringType());
                Space::assertCharacter(&s, ';', &span);
                load(File(e.value<String>(), file.parent()));
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
                if (!_types.hasKey(identifier))
                    identifier.span().throwError("Unknown type " + name);
                Type type = _types[identifier];
                Identifier objectIdentifier = Identifier::parse(&s);
                if (objectIdentifier.isOperator()) {
                    identifier.span().throwError("Cannot create an object "
                        "with operator name");
                }
                String objectName = objectIdentifier.name();
                if (has(objectIdentifier)) {
                    objectIdentifier.span().throwError(objectName +
                        " already exists");
                }
                Value value = StructuredType::empty();
                if (Space::parseCharacter(&s, '=', &span))
                    value = Expression::parse(&s).evaluate(&_context);
                Space::assertCharacter(&s, ';', &span);
                source = s;
                value = value.rValue().convertTo(type);
                span = tycoSpecifier.span() + span;
                if (type.member(Identifier("*")) == StringType()) {
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
                    value.value<Structure*>()->set(Identifier("*"),
                        objectName, span);
                }
                set(objectIdentifier, value, span);

                continue;
            }
            Identifier identifier = Identifier::parse(&s);
            if (!identifier.valid()) {
                s.location().throwError("Expected an include statement, an "
                    "object creation statement or an assignment statement.");
            }
            Value left = Expression::parseDot(&source).evaluate(&_context);
            Space::assertCharacter(&source, '=', &span);
            Expression e = Expression::parse(&source);
            if (!e.valid())
                source.location().throwError("Expected expression.");
            Space::assertCharacter(&source, ';', &span);
            Value loadedExpression = e.evaluate(&_context);
            LValueType lValueType(left.type());
            if (!lValueType.valid())
                left.span().throwError("LValue required");
            Type type = lValueType.inner();
            LValue p = left.value<LValue>();
            Identifier i = Identifier(OperatorAssignment());
            Value v = loadedExpression.rValue();
            span = left.span() + v.span();
            if (type.member(i).valid()) {
                List<Value> arguments;
                arguments.add(left);
                arguments.add(v);
                auto s = p.rValue().value<Structure*>();
                Value f = s->getValue(i);
                if (f.type() == FuncoType())
                    f.value<OverloadedFunctionSet>().evaluate(arguments, span);
                else {
                    List<Type> argumentTypes;
                    argumentTypes.add(PointerType(type));
                    argumentTypes.add(PointerType(v.type()));
                    auto func = f.value<Funco>();
                    if (!func.argumentsMatch(argumentTypes)) {
                        span.throwError("Cannot assign value of type " +
                            v.type().toString() + " to object of type " +
                            type.toString() + ".");
                    }
                    func.evaluate(arguments, span);
                }
            }
            else
                p.set(v.convertTo(type), span);
        } while (true);
        for (auto i : *this) {
            if (!i.value().valid())
                throw Exception(file.path() + ": " + i.key().name() +
                    " not defined and no default is available.");
        }
    }

    File file() const { return _file; }

    Value valueOfIdentifier(Identifier i)
    {
        Span s = i.span();
        if (!has(i))
            s.throwError("Unknown identifier " + i.name());
        return Value(LValueType::wrap(getValue(i).type()), LValue(this, i), s);
    }
    Tyco resolveTycoIdentifier(TycoIdentifier i) const
    {
        String s = i.name();
        if (!_types.hasKey(s))
            return Tyco();
        return _types[s];
    }

    template<class U> U evaluate(String text, const U& def)
    {
        CharacterSource s(text);
        Value c;
        try {
            Expression e = Expression::parse(&s);
            if (!e.valid())
                return def;
            Value v = e.evaluate(&_context);
            if (!v.valid())
                return def;
            c = v.convertTo(typeFromCompileTimeType<U>());
            if (!c.valid())
                return def;
        }
        catch (...) {
            return def;
        }
        return c.template value<U>();
    }
private:

    class EvaluationContext : public ::EvaluationContext
    {
    public:
        EvaluationContext(ConfigFile* configFile) : _configFile(configFile) { }
        Value valueOfIdentifier(Identifier i)
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

    HashTable<TycoIdentifier, Type> _types;
    File _file;
    EvaluationContext _context;
};

#endif // INCLUDED_CONFIG_FILE_H
