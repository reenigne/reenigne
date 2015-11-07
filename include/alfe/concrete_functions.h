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

template<> Nullary<Funco, AddConcreteConcrete>
    Nullary<Funco, AddConcreteConcrete>::_instance;

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

template<> Nullary<Funco, SubtractConcreteConcrete>
    Nullary<Funco, SubtractConcreteConcrete>::_instance;

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

template<> Nullary<Funco, MultiplyConcreteConcrete>
    Nullary<Funco, MultiplyConcreteConcrete>::_instance;

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

template<> Nullary<Funco, MultiplyConcreteAbstract>
    Nullary<Funco, MultiplyConcreteAbstract>::_instance;

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

template<> Nullary<Funco, MultiplyAbstractConcrete>
    Nullary<Funco, MultiplyAbstractConcrete>::_instance;

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

template<> Nullary<Funco, DivideConcreteConcrete>
    Nullary<Funco, DivideConcreteConcrete>::_instance;

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

template<> Nullary<Funco, DivideConcreteAbstract>
    Nullary<Funco, DivideConcreteAbstract>::_instance;

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

template<> Nullary<Funco, DivideAbstractConcrete>
    Nullary<Funco, DivideAbstractConcrete>::_instance;

#endif // INCLUDED_CONCRETE_FUNCTIONS_H
