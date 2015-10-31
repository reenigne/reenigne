#include "alfe/main.h"

#ifndef INCLUDED_CONCRETE_FUNCTIONS_H
#define INCLUDED_CONCRETE_FUNCTIONS_H

#include "alfe/function.h"
#include "alfe/concrete.h"

class AddConcreteConcrete : public Nullary<Funco, AddConcreteConcrete>
{
public:
    class Body : public Nullary::Body
    {
    public:
        TypedValue evaluate(List<TypedValue> arguments) const
        {
            auto i = arguments.begin();
            Concrete l = i->value<Concrete>();
            ++i;
            return TypedValue(l + i->value<Concrete>());
        }
        Identifier identifier() const { return OperatorPlus(); }
        TypedValue typedValue() const
        {
            return TypedValue(
                FunctionTyco(ConcreteType(), ConcreteType(), ConcreteType()),
                this);
        }
    };
};

template<> Nullary<Funco, AddConcreteConcrete>
    Nullary<Funco, AddConcreteConcrete>::_instance;

class SubtractConcreteConcrete
  : public Nullary<Funco, SubtractConcreteConcrete>
{
public:
    class Body : public Nullary::Body
    {
    public:
        TypedValue evaluate(List<TypedValue> arguments) const
        {
            auto i = arguments.begin();
            Concrete l = i->value<Concrete>();
            ++i;
            return TypedValue(l - i->value<Concrete>());
        }
        Identifier identifier() const { return OperatorMinus(); }
        TypedValue typedValue() const
        {
            return TypedValue(
                FunctionTyco(ConcreteType(), ConcreteType(), ConcreteType()),
                this);
        }
    };
};

template<> Nullary<Funco, SubtractConcreteConcrete>
    Nullary<Funco, SubtractConcreteConcrete>::_instance;

class MultiplyConcreteConcrete
  : public Nullary<Funco, MultiplyConcreteConcrete>
{
public:
    class Body : public Nullary::Body
    {
    public:
        TypedValue evaluate(List<TypedValue> arguments) const
        {
            auto i = arguments.begin();
            Concrete l = i->value<Concrete>();
            ++i;
            return TypedValue(l * i->value<Concrete>());
        }
        Identifier identifier() const { return OperatorStar(); }
        TypedValue typedValue() const
        {
            return TypedValue(
                FunctionTyco(ConcreteType(), ConcreteType(), ConcreteType()),
                this);
        }
    };
};

template<> Nullary<Funco, MultiplyConcreteConcrete>
    Nullary<Funco, MultiplyConcreteConcrete>::_instance;

class MultiplyConcreteAbstract
  : public Nullary<Funco, MultiplyConcreteAbstract>
{
public:
    class Body : public Nullary::Body
    {
    public:
        TypedValue evaluate(List<TypedValue> arguments) const
        {
            auto i = arguments.begin();
            Concrete l = i->value<Concrete>();
            ++i;
            return TypedValue(l * i->value<int>());
        }
        Identifier identifier() const { return OperatorStar(); }
        TypedValue typedValue() const
        {
            return TypedValue(
                FunctionTyco(ConcreteType(), ConcreteType(), IntegerType()),
                this);
        }
    };
};

template<> Nullary<Funco, MultiplyConcreteAbstract>
    Nullary<Funco, MultiplyConcreteAbstract>::_instance;

class MultiplyAbstractConcrete
  : public Nullary<Funco, MultiplyAbstractConcrete>
{
public:
    class Body : public Nullary::Body
    {
    public:
        TypedValue evaluate(List<TypedValue> arguments) const
        {
            auto i = arguments.begin();
            int l = i->value<int>();
            ++i;
            return TypedValue(l * i->value<Concrete>());
        }
        Identifier identifier() const { return OperatorStar(); }
        TypedValue typedValue() const
        {
            return TypedValue(
                FunctionTyco(ConcreteType(), IntegerType(), ConcreteType()),
                this);
        }
    };
};

template<> Nullary<Funco, MultiplyAbstractConcrete>
    Nullary<Funco, MultiplyAbstractConcrete>::_instance;

class DivideConcreteConcrete : public Nullary<Funco, DivideConcreteConcrete>
{
public:
    class Body : public Nullary::Body
    {
    public:
        TypedValue evaluate(List<TypedValue> arguments) const
        {
            auto i = arguments.begin();
            Concrete l = i->value<Concrete>();
            ++i;
            return TypedValue(l / i->value<Concrete>());
        }
        Identifier identifier() const { return OperatorDivide(); }
        TypedValue typedValue() const
        {
            return TypedValue(
                FunctionTyco(ConcreteType(), ConcreteType(), ConcreteType()),
                this);
        }
    };
};

template<> Nullary<Funco, DivideConcreteConcrete>
    Nullary<Funco, DivideConcreteConcrete>::_instance;

class DivideConcreteAbstract : public Nullary<Funco, DivideConcreteAbstract>
{
public:
    class Body : public Nullary::Body
    {
    public:
        TypedValue evaluate(List<TypedValue> arguments) const
        {
            auto i = arguments.begin();
            Concrete l = i->value<Concrete>();
            ++i;
            return TypedValue(l / i->value<int>());
        }
        Identifier identifier() const { return OperatorDivide(); }
        TypedValue typedValue() const
        {
            return TypedValue(
                FunctionTyco(ConcreteType(), ConcreteType(), IntegerType()),
                this);
        }
    };
};

template<> Nullary<Funco, DivideConcreteAbstract>
    Nullary<Funco, DivideConcreteAbstract>::_instance;

class DivideAbstractConcrete : public Nullary<Funco, DivideAbstractConcrete>
{
public:
    class Body : public Nullary::Body
    {
    public:
        TypedValue evaluate(List<TypedValue> arguments) const
        {
            auto i = arguments.begin();
            Rational l = i->value<Rational>();
            ++i;
            return TypedValue(l / i->value<Concrete>());
        }
        Identifier identifier() const { return OperatorDivide(); }
        TypedValue typedValue() const
        {
            return TypedValue(
                FunctionTyco(ConcreteType(), RationalType(), ConcreteType()),
                this);
        }
    };
};

template<> Nullary<Funco, DivideAbstractConcrete>
    Nullary<Funco, DivideAbstractConcrete>::_instance;

#endif // INCLUDED_CONCRETE_FUNCTIONS_H