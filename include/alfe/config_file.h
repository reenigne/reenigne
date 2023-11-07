#include "alfe/code.h"

#ifndef INCLUDED_CONFIG_FILE_H
#define INCLUDED_CONFIG_FILE_H

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
        addFunco(FloorRational());
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
    template<class V> ConfigOption<V> addOption(String name,
        Type type = Type(),
        Expression e = Expression())
    {
        if (!type.valid())
            type = typeFromCompileTimeType<V>();
        VariableDefinition v(type, name, e);
        _code.insert<VariableDefinitionStatement>(v, Span());
        return ConfigOption<V>(this, name);
    }

    //void addOption(String name, Type type)
    //{
    //    addOption(name, Value(type));
    //}
    template<class V> ConfigOption<V> addDefaultOption(String name, Type type,
        const V& defaultValue)
    {
        StructuredType s(type);
        Expression e = Expression::from(defaultValue);
        if (s.valid()) {
            return addOption<V>(name,
                s.lValueFromRValue(defaultValue, &_structureOwner), e);
        }
        return addOption<V>(name, type, e);
    }
    template<class V> ConfigOption<V> addDefaultOption(String name,
        const V& defaultValue)
    {
        return addDefaultOption(name, typeFromCompileTimeType<V>(),
            defaultValue);
    }
    void addTyco(Tyco tyco, TycoIdentifier identifier = TycoIdentifier())
    {
        if (!identifier.valid()) {
            // TODO: We might want to avoid going via String here. Should Tyco
            // have an identifier() method?
            identifier = TycoIdentifier(tyco.toString());
        }
        _code.insert<TycoDefinitionStatement>(identifier, type,
            identifier.span());
    }
//private:
//    void addOption(String name, Value defaultValue)
//    {
//        VariableDefinition v(defaultValue.type(), name);
//        _code.insert<VariableDefinitionStatement>(v, Span());
//        set(name, defaultValue, Span());
//    }
//public:
    void addFunco(Funco funco)
    {
        _code.insert<ExternalFuncoDefinitionStatement>(funco);
    }

//    void load(File file)
//    {
//        _file = file;
//        loadFromString(file.contents());
//    }
//    void loadFromString(String contents)
//    {
//        CharacterSource source(contents, _file);
//        Space::parse(&source);
//        do {
//            CharacterSource s = source;
//            if (s.get() == -1)
//                break;
//            s = source;
//            Span span;
//            if (Space::parseKeyword(&s, "include", &span)) {
//                Expression e = Expression::parse(&s);
//                e.resolve(&_scope);
//                Value v = e.evaluate(this).convertTo(StringType());
//                Space::assertCharacter(&s, ';', &span);
//                load(File(v.value<String>(), _file.parent()));
//                source = s;
//                continue;
//            }
//            TycoSpecifier tycoSpecifier = TycoSpecifier::parse(&s);
//            if (tycoSpecifier.valid()) {
//                Type type = _scope.resolveType(tycoSpecifier);
//                Identifier objectIdentifier = Identifier::parse(&s);
//                if (objectIdentifier.isOperator()) {
//                    objectIdentifier.span().throwError("Cannot create an "
//                        "object with operator name");
//                }
//                String objectName = objectIdentifier.toString();
//                if (has(objectIdentifier)) {
//                    objectIdentifier.span().throwError(objectName +
//                        " already exists");
//                }
//                Value value = StructuredType::empty();
//                if (Space::parseCharacter(&s, '=', &span)) {
//                    Expression e = Expression::parse(&s);
//                    e.resolve(&_scope);
//                    value = e.evaluate(this);
//                }
//                Space::assertCharacter(&s, ';', &span);
//                source = s;
//                value = value.rValue().convertTo(type);
//                span = tycoSpecifier.span() + span;
//                if (type.member(Identifier("*")) == StringType()) {
//                    // This special member is how ConfigFile tells created
//                    // objects their names so that they can responsible for
//                    // persistence so that this functionality doesn't need to
//                    // be in ConfigFile.
//                    // Using an actual identifier here would lead to collisions
//                    // with real members. ALFE persistence is done by types
//                    // knowing how to persist themselves.
//                    // I also don't want to use the empty string, since I might
//                    // want to use that as the connector name for
//                    // single-connector components.
//                    value.value<Structure*>()->set(Identifier("*"),
//                        objectName, span);
//                }
//				_scope.addObject(objectIdentifier,
//					VariableDefinition(type, objectIdentifier));
//                set(objectIdentifier, value, span);
//
//                continue;
//            }
//            Identifier identifier = Identifier::parse(&s);
//            if (!identifier.valid()) {
//                s.location().throwError("Expected an include statement, an "
//                    "object creation statement or an assignment statement.");
//            }
//            Expression le = Expression::parseDot(&source);
//            le.resolve(&_scope);
//            Value left = le.evaluate(this);
//            Space::assertCharacter(&source, '=', &span);
//            Expression e = Expression::parse(&source);
//            if (!e.valid())
//                source.location().throwError("Expected expression.");
//            Space::assertCharacter(&source, ';', &span);
//            e.resolve(&_scope);
//            Value loadedExpression = e.evaluate(this);
//            LValueType lValueType(left.type());
//            if (!lValueType.valid())
//                left.span().throwError("LValue required");
//            Type type = lValueType.inner();
//            LValue p = left.value<LValue>();
//            Identifier i = Identifier(OperatorAssignment());
//            Value v = loadedExpression.rValue();
//            span = left.span() + v.span();
//            if (type.member(i).valid()) {
//                List<Value> arguments;
//                arguments.add(left);
//                arguments.add(v);
//                auto s = p.rValue().value<Structure*>();
//                Value f = s->getValue(i);
//                if (f.type() == FuncoType())
//                    f.value<OverloadedFunctionSet>().evaluate(arguments, span);
//                else {
//                    List<Type> argumentTypes;
//                    argumentTypes.add(PointerType(type));
//                    argumentTypes.add(PointerType(v.type()));
//                    auto func = f.value<Funco>();
//                    if (!func.argumentsMatch(argumentTypes)) {
//                        span.throwError("Cannot assign value of type " +
//                            v.type().toString() + " to object of type " +
//                            type.toString() + ".");
//                    }
//                    func.evaluate(arguments, span);
//                }
//            }
//            else {
//                StructuredType s(type);
//                if (s.valid())
//                    s.setLValue(p, v.convertTo(type));
//                else
//                    p.set(v.convertTo(type), span);
//            }
//        } while (true);
//        for (auto i : *this) {
//            if (!i.value().valid())
//                throw Exception(_file.path() + ": " + i.key().toString() +
//                    " not defined and no default is available.");
//        }
//    }

    File file() const { return _file; }

//    template<class U> U evaluate(String text, const U& def)
//    {
//        CharacterSource s(text);
//        Value c;
//        try {
//            Expression e = Expression::parse(&s);
//            if (!e.valid())
//                return def;
//            e.resolve(&_scope);
//            Value v = e.evaluate(this);
//            if (!v.valid())
//                return def;
//            c = v.convertTo(typeFromCompileTimeType<U>());
//            if (!c.valid())
//                return def;
//        }
//        catch (...) {
//            return def;
//        }
//        return c.template value<U>();
//    }
private:
    File _file;
    Code _code;
    StructureOwner _structureOwner;
//    Scope _scope;
};

#endif // INCLUDED_CONFIG_FILE_H
