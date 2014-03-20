#include "alfe/main.h"

#ifndef INCLUDED_KIND_H
#define INCLUDED_KIND_H

#include "alfe/nullary.h"
#include "alfe/string.h"
#include "alfe/reference.h"

// Kind constructors. See:
//   http://www.reenigne.org/blog/templates-and-kinds-in-alfe
//   http://www.reenigne.org/blog/variadic-templates-in-alfe
// for more information.

class Kind
{
public:
    String toString() const { return _implementation->toString(); }
    bool valid() const { return _implementation.valid(); }
    bool operator==(const Kind& other) const
    {
        if (_implementation == other._implementation)
            return true;
        return _implementation->equals(other._implementation);
    }
    bool operator!=(const Kind& other) const { return !operator==(other); }
    int hash() const { return _implementation->hash(); }
    template<class T> bool is() const
    {
        return _implementation.referent<T::Implementation>() != 0;
    }
    template<class T> T as() const
    {
        return T(_implementation.referent<T::Implementation>());
    }
protected:
    class Implementation : public ReferenceCounted
    {
    public:
        virtual String toString() const = 0;
        virtual bool equals(const Implementation* other) const
        {
            return this == other;
        }
        virtual int hash() const { return reinterpret_cast<intptr_t>(this); }
    };
    Kind(const Implementation* implementation)
      : _implementation(implementation) { }
private:
    ConstReference<Implementation> _implementation;
    friend class TemplateKind;
};

// TypeKind is the kind of tycos that describe the types of variables, values
// and expressions.
class TypeKind : public Nullary<Kind, TypeKind>
{
public:
    static String name() { return String(); }
};

Nullary<Kind, TypeKind> Nullary<Kind, TypeKind>::_instance;

// WildKind is a placeholder kind which can be substituted for the kind of any
// tyco. It cannot be the kind of a real tyco - it is just used for pattern
// matching.
class WildKind : public Nullary<Kind, WildKind>
{
public:
    static String name() { return "$"; }
};

Nullary<Kind, WildKind> Nullary<Kind, WildKind>::_instance;

// VariadicKind is the kind of a sequence of type constructors of kind inner.
// It cannot be the kind of a real tyco - it is just used for pattern matching.
class VariadicKind : public Kind
{
public:
    VariadicKind(const Kind& inner) : Kind(new Implementation(inner)) { }
    Kind inner() const { return implementation()->inner(); }
private:
    class Implementation : public Kind::Implementation
    {
    public:
        Implementation(const Kind& inner) : _inner(inner) { }
        String toString() ocnst { return _inner.toString() + "..."; }
        bool equals(const Implementation* other) const
        {
            const Implementation* o =
                dynamic_cast<const Implementation*>(other);
            if (o == 0)
                return false;
            return _inner == o->_inner;
        }
        int hash() const { return _inner.hash()*67 + 1; }
        Kind inner() const { return _inner; }
    private:
        Kind _inner;
    };
    const Implementation* implementation() const
    {
        return _implementation.referent<Implementation>();
    }
};

Nullary<Kind, VariadicKind> Nullary<Kind, VariadicKind>::_instance;

// TemplateKind(first, rest) is the kind of a template that yields a tyco of
// kind "rest" when instantiated with a tyco of kind "first".
class TemplateKind : public Kind
{
public:
    // Pass in firstParameterKind and the Kind of the result is
    // restParameterKind.
    TemplateKind(const Kind& firstParameterKind, const Kind& restParameterKind)
      : Kind(new Implementation(firstParameterKind, restParameterKind)) { }
    TemplateKind(const Kind& kind) : Kind(kind) { }
    Kind first() const { return implementation()->first(); }
    Kind rest() const { return implementation()->rest(); }
protected:
    class Implementation : public Kind::Implementation
    {
    public:
        Implementation(const Kind& firstParameterKind,
            const Kind& restParameterKind)
          : _firstParameterKind(firstParameterKind),
            _restParameterKind(restParameterKind) { }
        String toString() const
        {
            String s("<");
            TemplateKind k(this);
            bool needComma = false;
            do {
                if (needComma)
                    s += ", ";
                s += k.first().toString();
                k = k.rest();
                needComma = true;
            } while (k != TypeKind());
            return s + ">";
        }
        bool equals(const Kind::Implementation* other) const
        {
            const Implementation* o =
                dynamic_cast<const Implementation*>(other);
            if (o == 0)
                return false;
            return _firstParameterKind == o->_firstParameterKind &&
                _restParameterKind == o->_restParameterKind;
        }
        int hash() const
        {
            return (_firstParameterKind.hash()*67 + 2)*67 +
                _restParameterKind.hash();
        }
        Kind first() const { return _firstParameterKind; }
        Kind rest() const { return _restParameterKind; }
    private:
        Kind _firstParameterKind;
        Kind _restParameterKind;
    };
    const Implementation* implementation() const
    {
        return _implementation.referent<Implementation>();
    }
};

class VariadicTemplateKind : public Nullary<TemplateKind, VariadicTemplateKind>
{
public:
    class Implementation : public TemplateKind::Implementation
    {
    public:
        Implementation()
          : TemplateKind::Implementation(
            VariadicKind(TypeKind()), TypeKind()) { }
    };
};

// VariadicTemplateKind is the kind of a template that yields a type for any
// sequence of types. It is the kind of Tuple.
Nullary<Kind, VariadicTemplateKind>
    Nullary<Kind, VariadicTemplateKind>::_instance;

#endif // INCLUDED_KIND_H
