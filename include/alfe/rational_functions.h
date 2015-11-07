#include "alfe/main.h"

#ifndef INCLUDED_RATIONAL_FUNCTIONS_H
#define INCLUDED_RATIONAL_FUNCTIONS_H

#include "alfe/function.h"
#include "alfe/rational.h"

class AddRationalRational : public Nullary<Function, AddRationalRational>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            Rational l = i->value<Rational>();
            ++i;
            return Value(l + i->value<Rational>());
        }
        Identifier identifier() const { return OperatorPlus(); }
        FunctionTyco tyco() const
        {
            return
                FunctionTyco(RationalType(), RationalType(), RationalType());
        }
    };
};

template<> Nullary<Function, AddRationalRational>
    Nullary<Function, AddRationalRational>::_instance;

class AddRationalInteger : public Nullary<Function, AddRationalInteger>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            Rational l = i->value<Rational>();
            ++i;
            return Value(l + i->value<int>());
        }
        Identifier identifier() const { return OperatorPlus(); }
        FunctionTyco tyco() const
        {
            return FunctionTyco(RationalType(), RationalType(), IntegerType());
        }
    };
};

template<> Nullary<Function, AddRationalInteger>
    Nullary<Function, AddRationalInteger>::_instance;

class AddIntegerRational : public Nullary<Function, AddIntegerRational>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            int l = i->value<int>();
            ++i;
            return Value(l + i->value<Rational>());
        }
        Identifier identifier() const { return OperatorPlus(); }
        FunctionTyco tyco() const
        {
            return FunctionTyco(RationalType(), IntegerType(), RationalType());
        }
    };
};

template<> Nullary<Function, AddIntegerRational>
    Nullary<Function, AddIntegerRational>::_instance;

class SubtractRationalRational
  : public Nullary<Function, SubtractRationalRational>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            Rational l = i->value<Rational>();
            ++i;
            return Value(l - i->value<Rational>());
        }
        Identifier identifier() const { return OperatorMinus(); }
        FunctionTyco tyco() const
        {
            return
                FunctionTyco(RationalType(), RationalType(), RationalType());
        }
    };
};

template<> Nullary<Function, SubtractRationalRational>
    Nullary<Function, SubtractRationalRational>::_instance;

class SubtractRationalInteger
  : public Nullary<Function, SubtractRationalInteger>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            Rational l = i->value<Rational>();
            ++i;
            return Value(l - i->value<int>());
        }
        Identifier identifier() const { return OperatorMinus(); }
        FunctionTyco tyco() const
        {
            return FunctionTyco(RationalType(), RationalType(), IntegerType());
        }
    };
};

template<> Nullary<Function, SubtractRationalInteger>
    Nullary<Function, SubtractRationalInteger>::_instance;

class SubtractIntegerRational
  : public Nullary<Function, SubtractIntegerRational>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            int l = i->value<int>();
            ++i;
            return Value(l - i->value<Rational>());
        }
        Identifier identifier() const { return OperatorMinus(); }
        FunctionTyco tyco() const
        {
            return FunctionTyco(RationalType(), IntegerType(), RationalType());
        }
    };
};

template<> Nullary<Function, SubtractIntegerRational>
    Nullary<Function, SubtractIntegerRational>::_instance;

class MultiplyRationalRational
  : public Nullary<Function, MultiplyRationalRational>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            Rational l = i->value<Rational>();
            ++i;
            return Value(l * i->value<Rational>());
        }
        Identifier identifier() const { return OperatorStar(); }
        FunctionTyco tyco() const
        {
            return
                FunctionTyco(RationalType(), RationalType(), RationalType());
        }
    };
};

template<> Nullary<Function, MultiplyRationalRational>
    Nullary<Function, MultiplyRationalRational>::_instance;

class MultiplyRationalInteger
    : public Nullary<Function, MultiplyRationalInteger>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            Rational l = i->value<Rational>();
            ++i;
            return Value(l * i->value<int>());
        }
        Identifier identifier() const { return OperatorStar(); }
        FunctionTyco tyco() const
        {
            return FunctionTyco(RationalType(), RationalType(), IntegerType());
        }
    };
};

template<> Nullary<Function, MultiplyRationalInteger>
    Nullary<Function, MultiplyRationalInteger>::_instance;

class MultiplyIntegerRational
    : public Nullary<Function, MultiplyIntegerRational>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            int l = i->value<int>();
            ++i;
            return Value(l * i->value<Rational>());
        }
        Identifier identifier() const { return OperatorStar(); }
        FunctionTyco tyco() const
        {
            return FunctionTyco(RationalType(), IntegerType(), RationalType());
        }
    };
};

template<> Nullary<Function, MultiplyIntegerRational>
    Nullary<Function, MultiplyIntegerRational>::_instance;

class DivideRationalRational : public Nullary<Function, DivideRationalRational>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            Rational l = i->value<Rational>();
            ++i;
            return Value(l / i->value<Rational>());
        }
        Identifier identifier() const { return OperatorDivide(); }
        FunctionTyco tyco() const
        {
            return
                FunctionTyco(RationalType(), RationalType(), RationalType());
        }
    };
};

template<> Nullary<Function, DivideRationalRational>
    Nullary<Function, DivideRationalRational>::_instance;

class DivideRationalInteger : public Nullary<Function, DivideRationalInteger>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            Rational l = i->value<Rational>();
            ++i;
            return Value(l / i->value<int>());
        }
        Identifier identifier() const { return OperatorDivide(); }
        FunctionTyco tyco() const
        {
            return FunctionTyco(RationalType(), RationalType(), IntegerType());
        }
    };
};

template<> Nullary<Function, DivideRationalInteger>
    Nullary<Function, DivideRationalInteger>::_instance;

class DivideIntegerRational : public Nullary<Function, DivideIntegerRational>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            int l = i->value<int>();
            ++i;
            return Value(l / i->value<Rational>());
        }
        Identifier identifier() const { return OperatorDivide(); }
        FunctionTyco tyco() const
        {
            return FunctionTyco(RationalType(), IntegerType(), RationalType());
        }
    };
};

template<> Nullary<Function, DivideIntegerRational>
    Nullary<Function, DivideIntegerRational>::_instance;

class DivideIntegerInteger : public Nullary<Function, DivideIntegerInteger>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            int l = i->value<int>();
            ++i;
            return Value(Rational(l, i->value<int>()));
        }
        Identifier identifier() const { return OperatorDivide(); }
        FunctionTyco tyco() const
        {
            return FunctionTyco(RationalType(), IntegerType(), IntegerType());
        }
    };
};

template<> Nullary<Function, DivideIntegerInteger>
    Nullary<Function, DivideIntegerInteger>::_instance;

#endif // INCLUDED_RATIONAL_FUNCTIONS_H
