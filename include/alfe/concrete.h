#include "alfe/main.h"

#ifndef INCLUDED_CONCRETE_H
#define INCLUDED_CONCRETE_H

#include "alfe/body_with_array.h"
#include "alfe/rational.h"

class UnitMismatchException : public Exception
{
public:
    UnitMismatchException() : Exception("Unit mismatch") { }
};

class Concrete
{
public:
    Concrete() : Handle(Body::create(_bases + 1)) { ++_bases; }

    const Concrete& operator+=(const Concrete& other)
    {
        check(other);
        v() += other.v();
        return *this;
    }
    const Concrete& operator-=(const Concrete& other)
    {
        check(other);
        v() -= other.v();
        return *this;
    }
    Concrete operator+(const Concrete& other)
    {
        check(other);
        Body* b = body()->copy(other.body()->size());
        b->_value = v() + other.v();
        return Concrete(b);
    }
    Concrete operator-(const Concrete& other)
    {
        check(other);
        Body* b = body()->copy(other.body()->size());
        b->_value = v() - other.v();
        return Concrete(b);
    }
    Concrete operator*(const Concrete& other)
    {
        Body* b = body()->copy(other.body()->size());
        for (int i = 0; i < b->size(); ++i)
            b->set(i, body()->get(i) + other.body()->get(i));
        b->_value = v() * other.v();
        return Concrete(b);
    }
    Concrete operator/(const Concrete& other)
    {
        Body* b = body()->copy(other.body()->size());
        for (int i = 0; i < b->size(); ++i)
            b->set(i, body()->get(i) - other.body()->get(i));
        b->_value = v() / other.v();
        return Concrete(b);
    }
    const Concrete& operator*=(const Concrete& other)
    {
        Body* b = body();
        int s = b->size();
        const Body* o = other.as<Body>();
        if (s >= other.body()->size()) {
            for (int i = 0; i < s; ++i)
                b->set(i, b->get(i) + o->get(i));
            v() *= other.v();
        }
        else
            *this = *this * other;
    }
    const Concrete& operator/=(const Concrete& other)
    {
        Body* b = body();
        int s = b->size();
        const Body* o = other.as<Body>();
        if (s >= other.body()->size()) {
            for (int i = 0; i < s; ++i)
                b->set(i, b->get(i) - o->get(i));
            v() /= other.v();
        }
        else
            *this = *this / other;
    }
    template<class T> const Concrete& operator*=(const T& other)
    {
        v() *= other;
        return *this;
    }
    template<class T> const Concrete& operator/=(const T& other)
    {
        v() /= other;
        return *this;
    }
    template<class T> const Concrete& operator*(const T& other)
    {
        Body* b = body()->copy(0);
        b->_value = v() * other;
        return Concrete(b);
    }
    template<class T> const Concrete& operator/(const T& other)
    {
        Body* b = body()->copy(0);
        b->_value = v() / other;
        return Concrete(b);
    }
    Rational value() const
    {
        if (!dimensionless())
            throw UnitMismatchException();
        return v();
    }
    bool operator<(const Concrete& other) const
    {
        check(other);
        return v() < other.v();
    }
    bool operator>(const Concrete& other) const { return other < *this; }
    bool operator<=(const Concrete& other) const { return !(other > *this); }
    bool operator>=(const Concrete& other) const { return !(other < *this); }
    bool operator==(const Concrete& other) const
    {
        if (!commensurable(other))
            return false;
        return v() == other.v();
    }
    bool operator!=(const Concrete& other) const { return !(other == *this); }
    bool commensurable(const Concrete& other) const
    {
        const Body* o = other.as<Body>();
        for (int i = 0; i < max(body()->size(), o->size()); ++i)
            if (body()->get(i) != o->get(i))
                return false;
        return true;
    }
    bool dimensionless() const
    {
        for (int i = 0; i < body()->size(); ++i)
            if (body()->get(i) != 0)
                return false;
        return true;
    }
    Concrete reciprocal() const
    {
        Body* b = body()->copy(0);
        for (int i = 0; i < size(); ++i)
            b->set(i, -body()->get(i));
        b->_value = 1 / v();
        return Concrete(b);
    }

private:
    ConcreteType::Body* body() { return _type.as<ConcreteType::Body>(); }
    const ConcreteType::Body* body() const { return as<ConcreteType::Body>(); }

    void check(const Concrete& other) const
    {
        if (!commensurable(other))
            throw UnitMismatchException();
    }
    const Rational& v() const { return body()->_value; }
    Rational& v() { return body()->_value; }
    int size() const { return body()->size(); }

    ConcreteType _type;
    Rational _value;
};

template<class T> Concrete operator*(const T& x, const Concrete& y)
{
    return y*x;
}

template<class T> Concrete operator/(const T& x, const Concrete& y)
{
    return x*y.reciprocal();
}

#endif // INCLUDED_CONCRETE_H