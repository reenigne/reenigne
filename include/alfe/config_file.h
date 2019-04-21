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
#include "alfe/double_functions.h"

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

template<class T> class ConfigFileT : public Structure
{
public:
    ConfigFileT()
    {
        addType(IntegerType());
        addType(BooleanType());
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
        addFunco(AddDoubleDouble());
        addFunco(AddDoubleRational());
        addFunco(AddRationalDouble());
        addFunco(SubtractDoubleDouble());
        addFunco(SubtractDoubleRational());
        addFunco(SubtractRationalDouble());
        addFunco(MultiplyDoubleDouble());
        addFunco(MultiplyDoubleRational());
        addFunco(MultiplyRationalDouble());
        addFunco(DivideDoubleDouble());
        addFunco(DivideDoubleRational());
        addFunco(DivideRationalDouble());
        addFunco(PowerDoubleDouble());
        addFunco(PowerDoubleRational());
        addFunco(PowerRationalDouble());
        addFunco(ShiftLeftDoubleInteger());
        addFunco(ShiftRightDoubleInteger());
        addFunco(NegativeDouble());
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
        StructuredType s(type);
        if (s.valid()) {
            addOption(name,
                s.lValueFromRValue(defaultValue, &_structureOwner));
        }
        else
            addOption(name, Value(type, defaultValue));
    }
    template<class V> ConfigOption<V> addDefaultOption(String name,
        const V& defaultValue)
    {
        addDefaultOption(name, typeFromCompileTimeType<V>(), defaultValue);
        return ConfigOption<V>(this, name);
    }
    void addType(Type type, TycoIdentifier identifier = TycoIdentifier())
    {
        _scope.addType(type, identifier);
    }
private:
    void addOption(String name, Value defaultValue)
    {
        VariableDefinition s(defaultValue.type(), name);
        _scope.addObject(name, s);
        set(name, defaultValue, Span());
    }
public:
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
        loadFromString(file.contents());
    }
    void loadFromString(String contents)
    {
        CharacterSource source(contents, _file);
        Space::parse(&source);
        do {
            CharacterSource s = source;
            if (s.get() == -1)
                break;
            s = source;
            Span span;
            if (Space::parseKeyword(&s, "include", &span)) {
                Expression e = Expression::parse(&s);
                e.resolve(&_scope);
                Value v = e.evaluate().convertTo(StringType());
                Space::assertCharacter(&s, ';', &span);
                load(File(v.value<String>(), _file.parent()));
                source = s;
                continue;
            }
            TycoSpecifier tycoSpecifier = TycoSpecifier::parse(&s);
            if (tycoSpecifier.valid()) {
                Type type = _scope.resolveType(tycoSpecifier);
                Identifier objectIdentifier = Identifier::parse(&s);
                if (objectIdentifier.isOperator()) {
                    objectIdentifier.span().throwError("Cannot create an "
                        "object with operator name");
                }
                String objectName = objectIdentifier.toString();
                if (has(objectIdentifier)) {
                    objectIdentifier.span().throwError(objectName +
                        " already exists");
                }
                Value value = StructuredType::empty();
                if (Space::parseCharacter(&s, '=', &span)) {
                    Expression e = Expression::parse(&s);
                    e.resolve(&_scope);
                    value = e.evaluate();
                }
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
            Expression le = Expression::parseDot(&source);
            le.resolve(&_scope);
            Value left = le.evaluate();
            Space::assertCharacter(&source, '=', &span);
            Expression e = Expression::parse(&source);
            if (!e.valid())
                source.location().throwError("Expected expression.");
            Space::assertCharacter(&source, ';', &span);
            e.resolve(&_scope);
            Value loadedExpression = e.evaluate();
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
            else {
                StructuredType s(type);
                if (s.valid())
                    s.setLValue(p, v.convertTo(type));
                else
                    p.set(v.convertTo(type), span);
            }
        } while (true);
        for (auto i : *this) {
            if (!i.value().valid())
                throw Exception(_file.path() + ": " + i.key().toString() +
                    " not defined and no default is available.");
        }
    }

    File file() const { return _file; }

    template<class U> U evaluate(String text, const U& def)
    {
        CharacterSource s(text);
        Value c;
        try {
            Expression e = Expression::parse(&s);
            if (!e.valid())
                return def;
            e.resolve(this);
            Value v = e.evaluate();
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
    File _file;
    StructureOwner _structureOwner;
    Scope _scope;
};

#endif // INCLUDED_CONFIG_FILE_H
