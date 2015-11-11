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
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            Concrete l = i->value<Concrete>();
            ++i;
            return Value(l + i->value<Concrete>());
        }
        Identifier identifier() const { return OperatorPlus(); }
        bool argumentsMatch(List<Type> argumentTypes) const
        {
            if (argumentTypes.count() != 2)
                return false;
            auto i = argumentTypes.begin();
            ConcreteType l(*i);
            if (!l.valid())
                return false;
            ++i;
            return ConcreteType(*i).valid();
        }
        FunctionTyco tyco() const
        {
            return
                FunctionTyco(ConcreteTyco(), ConcreteTyco(), ConcreteTyco());
        }
    };
};

class SubtractConcreteConcrete
  : public Nullary<Funco, SubtractConcreteConcrete>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            Concrete l = i->value<Concrete>();
            ++i;
            return Value(l - i->value<Concrete>());
        }
        Identifier identifier() const { return OperatorMinus(); }
        bool argumentsMatch(List<Type> argumentTypes) const
        {
            if (argumentTypes.count() != 2)
                return false;
            auto i = argumentTypes.begin();
            ConcreteType l(*i);
            if (!l.valid())
                return false;
            ++i;
            return ConcreteType(*i).valid();
        }
        FunctionTyco tyco() const
        {
            return
                FunctionTyco(ConcreteTyco(), ConcreteTyco(), ConcreteTyco());
        }
    };
};

class MultiplyConcreteConcrete
  : public Nullary<Funco, MultiplyConcreteConcrete>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            Concrete l = i->value<Concrete>();
            ++i;
            return Value(l * i->value<Concrete>());
        }
        Identifier identifier() const { return OperatorStar(); }
        bool argumentsMatch(List<Type> argumentTypes) const
        {
            if (argumentTypes.count() != 2)
                return false;
            auto i = argumentTypes.begin();
            ConcreteType l(*i);
            if (!l.valid())
                return false;
            ++i;
            return ConcreteType(*i).valid();
        }
        FunctionTyco tyco() const
        {
            return
                FunctionTyco(ConcreteTyco(), ConcreteTyco(), ConcreteTyco());
        }
    };
};

class MultiplyConcreteAbstract
  : public Nullary<Funco, MultiplyConcreteAbstract>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            Concrete l = i->value<Concrete>();
            ++i;
            if (i->type() == RationalType())
                return Value(l * i->value<Rational>());
            else
                return Value(l * i->value<int>());
        }
        Identifier identifier() const { return OperatorStar(); }
        bool argumentsMatch(List<Type> argumentTypes) const
        {
            if (argumentTypes.count() != 2)
                return false;
            auto i = argumentTypes.begin();
            ConcreteType l(*i);
            if (!l.valid())
                return false;
            ++i;
            return (*i == RationalType() || *i == IntegerType());
        }
        FunctionTyco tyco() const
        {
            return
                FunctionTyco(ConcreteTyco(), ConcreteTyco(), AbstractType());
        }
    };
};

class MultiplyAbstractConcrete
  : public Nullary<Funco, MultiplyAbstractConcrete>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            if (i->type() == RationalType()) {
                Rational l = i->value<Rational>();
                ++i;
                return Value(l * i->value<Concrete>());
            }
            else {
                int l = i->value<int>();
                ++i;
                return Value(l * i->value<Concrete>());
            }
        }
        Identifier identifier() const { return OperatorStar(); }
        bool argumentsMatch(List<Type> argumentTypes) const
        {
            if (argumentTypes.count() != 2)
                return false;
            auto i = argumentTypes.begin();
            if (*i != RationalType() && *i != IntegerType())
                return false;
            ++i;
            return ConcreteType(*i).valid();
        }
        FunctionTyco tyco() const
        {
            return
                FunctionTyco(ConcreteTyco(), AbstractType(), ConcreteTyco());
        }
    };
};

class DivideConcreteConcrete : public Nullary<Funco, DivideConcreteConcrete>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            Concrete l = i->value<Concrete>();
            ++i;
            return Value(l / i->value<Concrete>());
        }
        Identifier identifier() const { return OperatorDivide(); }
        bool argumentsMatch(List<Type> argumentTypes) const
        {
            if (argumentTypes.count() != 2)
                return false;
            auto i = argumentTypes.begin();
            ConcreteType l(*i);
            if (!l.valid())
                return false;
            ++i;
            return ConcreteType(*i).valid();
        }
        FunctionTyco tyco() const
        {
            return
                FunctionTyco(ConcreteTyco(), ConcreteTyco(), ConcreteTyco());
        }
    };
};

class DivideConcreteAbstract : public Nullary<Funco, DivideConcreteAbstract>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            Concrete l = i->value<Concrete>();
            ++i;
            if (i->type() == RationalType())
                return Value(l / i->value<Rational>());
            else
                return Value(l / i->value<int>());

        }
        Identifier identifier() const { return OperatorDivide(); }
        bool argumentsMatch(List<Type> argumentTypes) const
        {
            if (argumentTypes.count() != 2)
                return false;
            auto i = argumentTypes.begin();
            ConcreteType l(*i);
            if (!l.valid())
                return false;
            ++i;
            return (*i == RationalType() || *i == IntegerType());
        }
        FunctionTyco tyco() const
        {
            return
                FunctionTyco(ConcreteTyco(), ConcreteTyco(), AbstractType());
        }
    };
};

class DivideAbstractConcrete : public Nullary<Funco, DivideAbstractConcrete>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            if (i->type() == RationalType()) {
                Rational l = i->value<Rational>();
                ++i;
                return Value(l / i->value<Concrete>());
            }
            else {
                int l = i->value<int>();
                ++i;
                return Value(l / i->value<Concrete>());
            }
        }
        Identifier identifier() const { return OperatorDivide(); }
        bool argumentsMatch(List<Type> argumentTypes) const
        {
            if (argumentTypes.count() != 2)
                return false;
            auto i = argumentTypes.begin();
            if (*i != RationalType() && *i != IntegerType())
                return false;
            ++i;
            return ConcreteType(*i).valid();
        }
        FunctionTyco tyco() const
        {
            return
                FunctionTyco(ConcreteTyco(), AbstractType(), ConcreteTyco());
        }
    };
};

class ShiftLeftConcreteInteger
    : public Nullary<Function, ShiftLeftConcreteInteger>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            auto l = i->value<Concrete>();
            ++i;
            int r = i->value<int>();
            if (r < 0)
                return l*Rational(1, 1 << -r);
            return l*Rational(1 << r, 1);
        }
        Identifier identifier() const { return OperatorShiftLeft(); }
        bool argumentsMatch(List<Type> argumentTypes) const
        {
            if (argumentTypes.count() != 2)
                return false;
            auto i = argumentTypes.begin();
            if (!ConcreteType(*i).valid())
                return false;
            ++i;
            return *i == IntegerType();
        }
        FunctionTyco tyco() const
        {
            return FunctionTyco(ConcreteType(), ConcreteType(), IntegerType());
        }
    };
};

class ShiftRightConcreteInteger
    : public Nullary<Function, ShiftRightConcreteInteger>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            auto l = i->value<Concrete>();
            ++i;
            int r = i->value<int>();
            if (r < 0)
                return l*Rational(1 << -r, 1);
            return l*Rational(1, 1 << r);
        }
        Identifier identifier() const { return OperatorShiftRight(); }
        bool argumentsMatch(List<Type> argumentTypes) const
        {
            if (argumentTypes.count() != 2)
                return false;
            auto i = argumentTypes.begin();
            if (!ConcreteType(*i).valid())
                return false;
            ++i;
            return *i == IntegerType();
        }
        FunctionTyco tyco() const
        {
            return FunctionTyco(ConcreteType(), ConcreteType(), IntegerType());
        }
    };
};

#endif // INCLUDED_CONCRETE_FUNCTIONS_H
