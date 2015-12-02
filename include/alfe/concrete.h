#include "alfe/main.h"

#ifndef INCLUDED_CONCRETE_H
#define INCLUDED_CONCRETE_H

#include "alfe/rational.h"
#include "alfe/type.h"

class UnitMismatchException : public Exception
{
public:
    UnitMismatchException() : Exception("Unit mismatch") { }
};

template<class T> class ConcreteTypeTemplate;

template<class T> class ConcreteTemplate
{
public:
    ConcreteTemplate() : _type(ConcreteType()), _value(1) { }
    static ConcreteTemplate zero()
    {
        return ConcreteTemplate(ConcreteTypeTemplate<T>::zero(), 0);
    }

    const ConcreteTemplate& operator+=(const ConcreteTemplate& other)
    {
        check(other);
        _value += other._value;
        return *this;
    }
    const ConcreteTemplate& operator-=(const ConcreteTemplate& other)
    {
        check(other);
        _value -= other._value;
        return *this;
    }
    ConcreteTemplate operator+(const ConcreteTemplate& other)
    {
        check(other);
        if (_value == 0)
            return other;
        return ConcreteTemplate(_type, _value + other._value);
    }
    ConcreteTemplate operator-(const ConcreteTemplate& other)
    {
        check(other);
        if (_value == 0)
            return ConcreteTemplate(other._type, -other._value);
        return ConcreteTemplate(_type, _value - other._value);
    }
    ConcreteTemplate operator*(const ConcreteTemplate& other)
    {
        return ConcreteTemplate(_type + other._type, _value * other._value);
    }
    ConcreteTemplate operator/(const ConcreteTemplate& other)
    {
        return ConcreteTemplate(_type - other._type, _value / other._value);
    }
    const ConcreteTemplate& operator*=(const ConcreteTemplate& other)
    {
        _type += other._type;
        _value *= other._value;
        return *this;
    }
    const ConcreteTemplate& operator/=(const ConcreteTemplate& other)
    {
        _type -= other._type;
        _value /= other._value;
        return *this;
    }
    template<class U> const ConcreteTemplate& operator*=(const U& other)
    {
        _value *= other;
        return *this;
    }
    template<class U> const ConcreteTemplate& operator/=(const U& other)
    {
        _value /= other;
        return *this;
    }
    template<class U> ConcreteTemplate operator*(const U& other) const
    {
        return ConcreteTemplate(_type, _value * other);
    }
    template<class U> ConcreteTemplate operator/(const U& other) const
    {
        return ConcreteTemplate(_type, _value / other);
    }
    Rational value() const
    {
        if (!isAbstract())
            throw UnitMismatchException();
        return _value;
    }
    bool operator<(const ConcreteTemplate& other) const
    {
        check(other);
        return _value < other._value;
    }
    bool operator>(const ConcreteTemplate& other) const
    {
        return other < *this;
    }
    bool operator<=(const ConcreteTemplate& other) const
    {
        return !(other > *this);
    }
    bool operator>=(const ConcreteTemplate& other) const
    {
        return !(other < *this);
    }
    bool operator==(const ConcreteTemplate& other) const
    {
        if (!commensurable(other))
            return false;
        return _value == other._value;
    }
    bool operator!=(const ConcreteTemplate& other) const
    {
        return !(other == *this);
    }
    bool commensurable(const ConcreteTemplate& other) const
    {
        return _value == 0 || other._value == 0 || _type == other._type;
    }
    bool isAbstract() const { return _type.isAbstract() || _value == 0; }
    ConcreteTemplate reciprocal() const
    {
        return ConcreteTemplate(-_type, 1 / _value);
    }
    ConcreteTypeTemplate<T> type() const { return _type; }

private:
    ConcreteTemplate(const ConcreteTypeTemplate<T>& type, const T& value)
      : _type(type), _value(value) { }

    void check(const ConcreteTemplate& other) const
    {
        if (!commensurable(other))
            throw UnitMismatchException();
    }

    ConcreteTypeTemplate<T> _type;
    T _value;
};

template<class T, class U> ConcreteTemplate<U> operator*(const T& x,
    const ConcreteTemplate<U>& y)
{
    return y*x;
}

template<class T, class U> ConcreteTemplate<U> operator/(const T& x,
    const ConcreteTemplate<U>& y)
{
    return x*y.reciprocal();
}

typedef ConcreteTemplate<Rational> Concrete;

#endif // INCLUDED_CONCRETE_H