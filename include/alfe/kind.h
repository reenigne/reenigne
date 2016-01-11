#include "alfe/main.h"

#ifndef INCLUDED_KIND_H
#define INCLUDED_KIND_H

#include "alfe/nullary.h"
#include "alfe/string.h"
#include "alfe/handle.h"

// Kind constructors. See:
//   http://www.reenigne.org/blog/templates-and-kinds-in-alfe
//   http://www.reenigne.org/blog/variadic-templates-in-alfe
// for more information.

class Kind : public ConstHandle
{
public:
    Kind() { }
    Kind(const ConstHandle other) : ConstHandle(other) { }
    String toString() const { return body()->toString(); }
    bool operator!=(const Kind& other) const { return !operator==(other); }
    Kind instantiate(Kind argument) const
    {
        return body()->instantiate(argument);
    }
protected:
    class Body : public ConstHandle::Body
    {
    public:
        virtual String toString() const = 0;
        virtual Kind instantiate(Kind argument) const = 0;
        Kind kind() const { return handle<Kind>(); }
    };
    const Body* body() const { return as<Body>(); }
private:
    friend class TemplateKind;
};

// TypeKind is the kind of tycos that describe the types of variables, values
// and expressions.
class TypeKind : public NamedNullary<Kind, TypeKind>
{
public:
    static String name() { return String(); }

    class Body : public NamedNullary::Body
    {
    public:
        Kind instantiate(Kind argument) const { return Kind(); }
    };
};

// VariadicTemplateKind is the kind of a template with a variable number of
// arguments that are not kind-checked before use.
class VariadicTemplateKind : public NamedNullary<Kind, VariadicTemplateKind>
{
public:
    static String name() { return "<...>"; }

    class Body : public NamedNullary::Body
    {
    public:
        Kind instantiate(Kind argument) const
        {
            return VariadicTemplateKind();
        }
    };
};

// TemplateKind(first, rest) is the kind of a template that yields a tyco of
// kind "rest" when instantiated with a tyco of kind "first".
class TemplateKind : public Kind
{
public:
    TemplateKind(const Kind& firstParameterKind, const Kind& restParameterKind)
      : Kind(create<Body>(firstParameterKind, restParameterKind)) { }
    TemplateKind(const Kind& kind) : Kind(kind) { }
    Kind first() const { return body()->first(); }
    Kind rest() const { return body()->rest(); }
protected:
    class Body : public Kind::Body
    {
    public:
        Body(const Kind& firstParameterKind,
            const Kind& restParameterKind)
          : _firstParameterKind(firstParameterKind),
            _restParameterKind(restParameterKind) { }
        String toString() const { return "<" + toString2(); }
        String toString2() const
        {
            Kind k = kind();
            bool needComma = false;
            String s;
            do {
                if (needComma)
                    s += ", ";
                if (k == VariadicTemplateKind())
                    return s + "...>";
                if (k == TypeKind())
                    return s + ">";
                TemplateKind t = kind();
                s += t.first().toString();
                k = t.rest();
                needComma = true;
            } while (true);
        }
        bool equals(const ConstHandle::Body* other) const
        {
            auto o = other->to<Body>();
            return o != 0 && _firstParameterKind == o->_firstParameterKind &&
                _restParameterKind == o->_restParameterKind;
        }
        Hash hash() const
        {
            return Kind::Body::hash().mixin(_firstParameterKind.hash()).
                mixin((_restParameterKind.hash()));
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
    const Body* body() const { return as<Body>(); }
};

#endif // INCLUDED_KIND_H
