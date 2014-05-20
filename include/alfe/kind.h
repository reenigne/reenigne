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
    Kind() { }
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
    Kind instantiate(Kind argument) const
    {
        return _implementation->instantiate(argument);
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
        virtual Kind instantiate(Kind argument) const = 0;
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

    class Implementation : public Nullary::Implementation
    {
    public:
        Kind instantiate(Kind argument) const { return Kind(); }
    };
};

template<> Nullary<Kind, TypeKind> Nullary<Kind, TypeKind>::_instance;

// VariadicTemplateKind is the kind of a template with a variable number of
// arguments that are not kind-checked before use.
class VariadicTemplateKind : public Nullary<Kind, VariadicTemplateKind>
{
public:
    static String name() { return "<...>"; }

    class Implementation : public Nullary::Implementation
    {
    public:
        Kind instantiate(Kind argument) const
        {
            return VariadicTemplateKind();
        }
    };
};

template<> Nullary<Kind, VariadicTemplateKind>
    Nullary<Kind, VariadicTemplateKind>::_instance;

// TemplateKind(first, rest) is the kind of a template that yields a tyco of
// kind "rest" when instantiated with a tyco of kind "first".
class TemplateKind : public Kind
{
public:
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
        String toString() const { return "<" + toString2(); }
        String toString2() const
        {
            Kind k(this);
            bool needComma = false;
            String s;
            do {
                if (needComma)
                    s += ", ";
                if (k == VariadicTemplateKind())
                    return s + "...>";
                if (k == TypeKind())
                    return s + ">";
                TemplateKind t(this);
                s += t.first().toString();
                k = t.rest();
                needComma = true;
            } while (true);
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
        Kind instantiate(Kind argument) const
        {
            // A tyco of kind VariadicTemplateKind can act as a type or a
            // template of any kind so (for the purposes of initial kind
            // checking) such a tyco can be passed to any template. Note that
            // the tyco will probably perform its own kind checking when
            // instantiated, so that there will be a suitable error when, say,
            // a Tuple is passed to a template of kind <<<>>>.

            if (argument == _firstParameterKind ||
                argument == VariadicTemplateKind())
                return _restParameterKind;
            return Kind();
        }
    private:
        Kind _firstParameterKind;
        Kind _restParameterKind;
    };
    const Implementation* implementation() const
    {
        return _implementation.referent<Implementation>();
    }
};

#endif // INCLUDED_KIND_H
