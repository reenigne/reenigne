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
        List<Tyco> parameterTycos() const
        {
            List<Tyco> r;
            r.add(ConcreteTyco());
            r.add(ConcreteTyco());
            return r;
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
        List<Tyco> parameterTycos() const
        {
            List<Tyco> r;
            r.add(ConcreteTyco());
            r.add(ConcreteTyco());
            return r;
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
        List<Tyco> parameterTycos() const
        {
            List<Tyco> r;
            r.add(ConcreteTyco());
            r.add(ConcreteTyco());
            return r;
        }
    };
};

class MultiplyConcreteRational
  : public Nullary<Funco, MultiplyConcreteRational>
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
            return l * i->convertTo(RationalType()).value<Rational>();
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
            return i->canConvertTo(RationalType());
        }
        List<Tyco> parameterTycos() const
        {
            List<Tyco> r;
            r.add(RationalType());
            r.add(ConcreteTyco());
            return r;
        }
    };
};

class MultiplyRationalConcrete
  : public Nullary<Funco, MultiplyRationalConcrete>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            auto l = i->convertTo(RationalType()).value<Rational>();
            ++i;
            return Value(l * i->value<Concrete>());
        }
        Identifier identifier() const { return OperatorStar(); }
        bool argumentsMatch(List<Type> argumentTypes) const
        {
            if (argumentTypes.count() != 2)
                return false;
            auto i = argumentTypes.begin();
            if (!i->canConvertTo(RationalType()))
                return false;
            ++i;
            return ConcreteType(*i).valid();
        }
        List<Tyco> parameterTycos() const
        {
            List<Tyco> r;
            r.add(ConcreteTyco());
            r.add(RationalType());
            return r;
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
        List<Tyco> parameterTycos() const
        {
            List<Tyco> r;
            r.add(ConcreteTyco());
            r.add(ConcreteTyco());
            return r;
        }
    };
};

class DivideConcreteRational : public Nullary<Funco, DivideConcreteRational>
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
            return l / i->convertTo(RationalType()).value<Rational>();
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
            return i->canConvertTo(RationalType());
        }
        List<Tyco> parameterTycos() const
        {
            List<Tyco> r;
            r.add(RationalType());
            r.add(ConcreteTyco());
            return r;
        }
    };
};

class DivideRationalConcrete : public Nullary<Funco, DivideRationalConcrete>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            auto l = i->convertTo(RationalType()).value<Rational>();
            ++i;
            return Value(l / i->value<Concrete>());
        }
        Identifier identifier() const { return OperatorDivide(); }
        bool argumentsMatch(List<Type> argumentTypes) const
        {
            if (argumentTypes.count() != 2)
                return false;
            auto i = argumentTypes.begin();
            if (!i->canConvertTo(RationalType()))
                return false;
            ++i;
            return ConcreteType(*i).valid();
        }
        List<Tyco> parameterTycos() const
        {
            List<Tyco> r;
            r.add(ConcreteTyco());
            r.add(RationalType());
            return r;
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
        List<Tyco> parameterTycos() const
        {
            List<Tyco> r;
            r.add(IntegerType());
            r.add(ConcreteTyco());
            return r;
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
        List<Tyco> parameterTycos() const
        {
            List<Tyco> r;
            r.add(IntegerType());
            r.add(ConcreteTyco());
            return r;
        }
    };
};

#endif // INCLUDED_CONCRETE_FUNCTIONS_H
