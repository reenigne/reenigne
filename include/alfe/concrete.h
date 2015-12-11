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
typedef ConcreteTypeTemplate<Rational> ConcreteType;

template<class T> class ConcreteTemplate
{
public:
    ConcreteTemplate() : _type(ConcreteTypeTemplate<T>()), _value(1) { }
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

class ConcreteKind : public NamedNullary<Kind, ConcreteKind>
{
public:
    static String name() { return "Concrete"; }

    class Body : public NamedNullary::Body
    {
    public:
        Kind instantiate(Kind argument) const { assert(false); return Kind(); }
    };
};

class ConcreteTyco : public NamedNullary<Tyco, ConcreteTyco>
{
public:
    ConcreteTyco() { }
    static String name() { return "Concrete"; }
protected:
    class Body : public NamedNullary<Tyco, ConcreteTyco>::Body
    {
    public:
        Kind kind() const { return ConcreteKind(); }
    };
    friend class Nullary<Tyco, ConcreteTyco>;
};

// ConcreteType is a bit strange. It's really a family of types, but these
// types cannot be instantiated via the usual template syntax. The normal
// constructor takes no arguments, but constructs a different dimension each
// time, so care must be taken to keep track of instantiations and use the
// correct one.
template<class T> class ConcreteTypeTemplate : public Type
{
    class BaseBody : public Type::Body
    {
        typedef Array<int>::Body<BaseBody> Body;
    public:
        String toString() const { return "Concrete"; }
        bool equals(const ConstHandle::Body* other) const
        {
            auto b = other->as<Body>();
            if (b == 0)
                return false;
            for (int i = 0; i < max(elements(), b->elements()); ++i)
                if ((*body())[i] != (*b)[i])
                    return false;
            return true;
        }
        Hash hash() const
        {
            Hash h = Type::Body::hash();
            int i;
            for (i = elements() - 1; i >= 0; --i)
                if ((*body())[i] != 0)
                    break;
            for (; i >= 0; --i)
                h.mixin((*body())[i]);
            return h;
        }
        bool isAbstract() const
        {
            for (int i = 0; i < elements(); ++i)
                if ((*body())[i] != 0)
                    return false;
            return true;
        }
        bool canConvertTo(const Type& to, String* reason) const
        {
            ConcreteTypeTemplate<T> c(to);
            if (c.valid()) {
                *reason = String("Types are not commensurate");
                return false;
            }
            if (!isAbstract()) {
                *reason = String("Type is denominate");
                return false;
            }
            return RationalType().canConvertTo(to);
        }
        Value convertTo(const Type& to, const Value& value) const
        {
            return RationalType().convertTo(to, Value(RationalType(),
                value.value<ConcreteTemplate<T>>().value(), value.span()));
        }
        int elements() const { return body()->size(); }
        Value defaultValue() const { return Concrete::zero(); }
        Value simplify(const Value& value) const
        {
            auto v = value.value<ConcreteTemplate<T>>();
            if (v.isAbstract())
                return Value(RationalType(), v.value(), value.span());
            return value;
        }
    private:
        Body* body() { return as<Body>(); }
        const Body* body() const { return as<Body>(); }
    };
    typedef Array<int>::Body<BaseBody> Body;

    static int _bases;
public:
    ConcreteTypeTemplate()
      : Type(Array<int>::create<ConstHandle, BaseBody>(_bases + 1, _bases + 1))
    {
        for (int i = 0; i < elements(); ++i)
            element(i) = 0;
        element(elements() - 1) = 1;
        ++_bases;
    }
    ConcreteTypeTemplate(const ConstHandle& other) : Type(other) { }
    static ConcreteTypeTemplate zero()
    {
        return Array<int>::create<ConstHandle, BaseBody>(0, 0);
    }
    bool valid() const { return body() != 0; }
    bool isAbstract() const { return body()->isAbstract(); }
    const ConcreteTypeTemplate& operator+=(const ConcreteTypeTemplate& other)
    {
        *this = *this + other;
        return *this;
    }
    const ConcreteTypeTemplate& operator-=(const ConcreteTypeTemplate& other)
    {
        *this = *this - other;
        return *this;
    }
    ConcreteTypeTemplate operator-() const
    {
        ConcreteTypeTemplate t(elements());
        for (int i = 0; i < elements(); ++i)
            t.element(i) = -element(i);
        return t;
    }
    ConcreteTypeTemplate operator+(const ConcreteTypeTemplate& other) const
    {
        ConcreteTypeTemplate t(max(elements(), other.elements()));
        for (int i = 0; i < t.elements(); ++i)
            t.element(i) = element(i) + other.element(i);
        return t;
    }
    ConcreteTypeTemplate operator-(const ConcreteTypeTemplate& other) const
    {
        ConcreteTypeTemplate t(max(elements(), other.elements()));
        for (int i = 0; i < t.elements(); ++i)
            t.element(i) = element(i) - other.element(i);
        return t;
    }
private:
    const Body* body() const { return as<Body>(); }
    Body* body() { return const_cast<Body*>(as<Body>()); }
    ConcreteTypeTemplate(int bases)
      : Type(Array<int>::create<ConstHandle, BaseBody>(bases, bases)) { }
    int elements() const { return body()->elements(); }
    int& element(int i) { return (*body())[i]; }
    int element(int i) const { return i >= elements() ? 0 : (*body())[i]; }
};

template<> int ConcreteTypeTemplate<Rational>::_bases = 0;

template<> Type typeFromValue<Concrete>(const Concrete& c) { return c.type(); }

// We would like to deserialize a concrete value into a Rational so that the
// calling code can handle it directly without units causing any run-time
// overhead. However, we cannot directly extract the rational from a concrete,
// we have to divide by another concrete (the unit) to get an abstract.
// ConcretePersistenceType exists to hold this unit concrete.
class ConcretePersistenceType : public Type
{
public:
    ConcretePersistenceType(Concrete unit) : Type(create<Body>(unit)) { }
private:
    class Body : public Type::Body
    {
    public:
        Body(Concrete unit) : _unit(unit) { }
        virtual bool canConvertFrom(const Type& other, String* reason) const
        {
            return other.canConvertTo(_unit.type(), reason);
        }
        virtual bool canConvertTo(const Type& other, String* reason) const
        {
            return _unit.type().canConvertTo(other, reason);
        }
        virtual Value convert(const Value& value) const
        {
            // First convert to the type of _unit
            Value v = value.convertTo(_unit.type());
            // Then replace the type with our type
            return Value(type(), v.value(), v.span());
        }
        virtual Value convertTo(const Type& to, const Value& value) const
        {
            // Replace the type with the one from _unit
            // Then ask the unit to do the conversion.
            return _unit.type().convertTo(to,
                Value(_unit.type(), value.value(), value.span()));
        }
        void deserialize(const Value& value, void* p) const
        {
            *static_cast<Rational*>(p) =
                (value.value<Concrete>()/_unit).value();
        }
        Value defaultValue() const { return _unit.type().defaultValue(); }
        String toString() const { return _unit.type().toString(); }
        Concrete _unit;
    };
};

#endif // INCLUDED_CONCRETE_H
