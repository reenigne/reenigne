#include "alfe/main.h"

#ifndef INCLUDED_CONCRETE_FUNCTIONS_H
#define INCLUDED_CONCRETE_FUNCTIONS_H

#include "alfe/function.h"
#include "alfe/concrete.h"

class AddConcreteConcrete : public Nullary<Function, AddConcreteConcrete>
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

class SubtractConcreteConcrete
  : public Nullary<Function, SubtractConcreteConcrete>
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

class MultiplyConcreteConcrete
    : public Nullary<Function, MultiplyConcreteConcrete>
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

class MultiplyConcreteInteger
    : public Nullary<Function, MultiplyConcreteInteger>
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

class MultiplyConcreteRational
    : public Nullary<Function, MultiplyConcreteRational>
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
            return TypedValue(l * i->value<Rational>());
        }
        Identifier identifier() const { return OperatorStar(); }
        TypedValue typedValue() const
        {
            return TypedValue(
                FunctionTyco(ConcreteType(), ConcreteType(), RationalType()),
                this);
        }
    };
};

class MultiplyIntegerConcrete
    : public Nullary<Function, MultiplyIntegerConcrete>
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

class MultiplyRationalConcrete
    : public Nullary<Function, MultiplyRationalConcrete>
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
            return TypedValue(l * i->value<Concrete>());
        }
        Identifier identifier() const { return OperatorStar(); }
        TypedValue typedValue() const
        {
            return TypedValue(
                FunctionTyco(ConcreteType(), RationalType(), ConcreteType()),
                this);
        }
    };
};

class DivideConcreteConcrete : public Nullary<Function, DivideConcreteConcrete>
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

class DivideConcreteInteger : public Nullary<Function, DivideConcreteInteger>
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

class DivideConcreteRational : public Nullary<Function, DivideConcreteRational>
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
            return TypedValue(l / i->value<Rational>());
        }
        Identifier identifier() const { return OperatorDivide(); }
        TypedValue typedValue() const
        {
            return TypedValue(
                FunctionTyco(ConcreteType(), ConcreteType(), RationalType()),
                this);
        }
    };
};

class DivideRationalConcrete : public Nullary<Function, DivideRationalConcrete>
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

#endif // INCLUDED_CONCRETE_FUNCTIONS_H