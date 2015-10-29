#include "alfe/main.h"

#ifndef INCLUDED_TYPE_H
#define INCLUDED_TYPE_H

#include "alfe/any.h"
#include "alfe/hash_table.h"
#include "alfe/value.h"
#include "alfe/nullary.h"
#include "alfe/kind.h"
#include "alfe/assert.h"
#include "alfe/identifier.h"
#include "alfe/vectors.h"
#include "alfe/rational.h"
#include "alfe/body_with_array.h"

template<class T> class TemplateTemplate;
typedef TemplateTemplate<void> Template;

template<class T> class TypeTemplate;
typedef TypeTemplate<void> Type;

template<class T> class TypedValueTemplate;
typedef TypedValueTemplate<void> TypedValue;

template<class T> class TycoTemplate;
typedef TycoTemplate<void> Tyco;

template<class T> class IdentifierTemplate;
typedef IdentifierTemplate<void> Identifier;

template<class T> class LValueTypeTemplate;
typedef LValueTypeTemplate<void> LValueType;

template<class T> class StructuredTypeTemplate;
typedef StructuredTypeTemplate<void> StructuredType;

template<class T> class TycoTemplate : public ConstHandle
{
public:
    TycoTemplate() { }
    String toString() const { return body()->toString(); }
    bool operator==(const Tyco& other) const
    {
        if (body() == other.body())
            return true;
        return body()->equals(other.body());
    }
    bool operator!=(const Tyco& other) const { return !operator==(other); }
    int hash() const { return body()->hash(); }
    Kind kind() const { return body()->kind(); }
protected:
    class Body : public ConstHandle::Body
    {
    public:
        // Tyco
        virtual String toString() const = 0;
        virtual bool equals(const Body* other) const { return this == other; }
        virtual Kind kind() const = 0;
        virtual int hash() const { return reinterpret_cast<intptr_t>(this); }

        // Type
        virtual TypedValueTemplate<T> tryConvert(
            const TypedValueTemplate<T>& value, String* reason) const = 0;
        virtual TypedValueTemplate<T> tryConvertTo(const Type& to,
            const TypedValue& value, String* reason) const = 0;
        virtual bool has(IdentifierTemplate<T> memberName) const = 0;

        // Template
        virtual Tyco instantiate(const Tyco& argument) const = 0;
    };
    TycoTemplate(const Body* body) : ConstHandle(body) { }

    friend class TemplateTemplate<void>;
    friend class EnumerationType;
    template<class U> friend class StructuredTypeTemplate;
public:
    const Body* body() const { return as<Body>(); }
};

template<class T> class StructureTemplate;
typedef StructureTemplate<void> Structure;

template<class T> class StructureTemplate
{
public:
    template<class U> U get(Identifier identifier)
    {
        return getValue(identifier).template value<U>();
    }
    virtual TypedValue getValue(Identifier identifier) = 0;
    virtual void set(Identifier identifier, TypedValue value) = 0;
};

template<class T> class LValueTemplate;
typedef LValueTemplate<void> LValue;

template<class T> class LValueTemplate
{
public:
    LValueTemplate(Structure* structure, Identifier identifier)
      : _structure(structure), _identifier(identifier) { }
    TypedValueTemplate<T> rValue() const
    {
        return _structure->getValue(_identifier);
    }
    void set(TypedValueTemplate<T> value) const
    {
        _structure->set(_identifier, value);
    }
private:
    Structure* _structure;
    Identifier _identifier;
};

template<class T> class TypeTemplate : public Tyco
{
public:
    TypeTemplate() { }
    TypeTemplate(const Tyco& tyco) : Tyco(tyco) { }

    TypedValueTemplate<T> tryConvert(const TypedValue& value, String* reason)
        const
    {
        return body()->tryConvert(value, reason);
    }
    TypedValueTemplate<T> tryConvertTo(const Type& to, const TypedValue& value,
        String* reason) const
    {
        return body()->tryConvertTo(to, value, reason);
    }
    bool has(IdentifierTemplate<T> memberName) const
    {
        return body()->has(memberName);
    }
    TypeTemplate(const Body* body) : Tyco(body) { }
    const Body* body() const { return as<Body>(); }
    Type rValue() const
    {
        if (LValueTypeTemplate<T>(*this).valid())
            return LValueTypeTemplate<T>(*this).inner();
        return *this;
    }
protected:
    class Body : public Tyco::Body
    {
    public:
        Kind kind() const { return TypeKind(); }
        TypedValueTemplate<T> tryConvert(const TypedValueTemplate<T>& value,
            String* reason) const
        {
            if (this == value.type().body())
                return value;
            return TypedValueTemplate<T>();
        }
        TypedValueTemplate<T> tryConvertTo(const Type& to,
            const TypedValue& value, String* reason) const
        {
            if (this == to.body())
                return value;
            return TypedValueTemplate<T>();
        }
        virtual bool has(IdentifierTemplate<T> memberName) const
        {
            return false;
        }
        Tyco instantiate(const Tyco& argument) const
        {
            throw Exception(String("Cannot instantiate ") + toString() +
                " because it is not a template.");
        }
    };

    friend class TemplateTemplate<void>;
};

template<class T> class LValueTypeTemplate : public Type
{
public:
    LValueTypeTemplate(const Tyco& other) : Type(other) {}
    static LValueType wrap(const Type& inner)
    {
        if (LValueType(inner).valid())
            return inner;
        return LValueType(new Body(inner));
    }
    Type inner() const { return body()->inner(); }
private:
    LValueTypeTemplate(const Body* body) : Type(body) { }

    class Body : public Type::Body
    {
    public:
        Body(Type inner) : _inner(inner) {}
        Type inner() const { return _inner; }
        String toString() const
        {
            return String("LValue<") + _inner.toString() + ">";
        }
    private:
        Type _inner;
    };

    const Body* body() const { return as<Body>(); }
};

template<class T> Type typeFromCompileTimeType() { return T::type(); }

template<class T> class TypedValueTemplate
{
public:
    TypedValueTemplate() { }
    TypedValueTemplate(Type type, Any defaultValue = Any(), Span span = Span())
      : _type(type), _value(defaultValue), _span(span) { }
    template<class U> TypedValueTemplate(const U& value, Span span = Span())
      : _type(typeFromCompileTimeType<U>()), _value(value), _span(span) { }
    Type type() const { return _type; }
    Any value() const { return _value; }
    template<class U> U value() const { return _value.value<U>(); }
    template<> Vector value<Vector>() const
    {
        Array<Any> sizeArray = value<List<Any>>();
        return Vector(sizeArray[0].value<int>(), sizeArray[1].value<int>());
    }
    void setValue(Any value) { _value = value; }
    Span span() const { return _span; }
    bool valid() const { return _value.valid(); }
    TypedValue convertTo(const Type& to) const
    {
        String reason;
        TypedValue v = tryConvertTo(to, &reason);
        if (!v.valid())
            span().throwError(reason);
        return v;
    }
    TypedValue tryConvertTo(const Type& to, String* why) const
    {
        String reason;
        TypedValue v = to.tryConvert(*this, &reason);
        if (v.valid())
            return v;
        String reasonTo;
        v = _type.tryConvertTo(to, *this, &reasonTo);
        if (v.valid())
            return v;
        String r = "No conversion";
        String f = _type.toString();
        if (f != "")
            r += String(" from type ") + f;
        r += String(" to type ") + to.toString() + String(" is available");
        if (reason.empty())
            reason = reasonTo;
        if (reason.empty())
            r += ".";
        else
            r += String(": ") + reason;
        *why = r;
        return TypedValue();
    }
    TypedValue rValue() const
    {
        LValueType lValueType(_type);
        if (lValueType.valid()) {
            return TypedValue(lValueType.inner(), value<LValue>().rValue(),
                _span);
        }
        return *this;
    }
private:
    Type _type;
    Any _value;
    Span _span;
};

template<class T> class TemplateTemplate : public Tyco
{
public:
    Tyco instantiate(const Tyco& argument) const
    {
        return body()->instantiate(argument);
    }
protected:
    class Body : public Tyco::Body
    {
    public:
        Tyco instantiate(const Tyco& argument) const
        {
            if (_instantiations.hasKey(argument))
                return _instantiations[argument];

            Kind k = kind();
            Kind resultKind = k.instantiate(argument.kind());
            if (!resultKind.valid()) {
                throw Exception(String("Cannot use ") + argument.toString() +
                    " (kind " + argument.kind().toString() +
                    ") to instantiate " + toString() +
                    " because it requires a type constructor of kind " +
                    k.toString());
            }
            TemplateKind tk = k;
            Tyco t = partialInstantiate(tk.rest() == TypeKind(), argument);
            _instantiations.add(argument, t);
            return t;
        }
        virtual Tyco partialInstantiate(bool final, Tyco argument) const
        {
            if (final)
                return finalInstantiate(this, argument);
            return new PartialBody(this, this, argument);
        }
        virtual Type finalInstantiate(const Body* parent, Tyco
            argument) const = 0;
        TypedValue tryConvert(const TypedValue& value, String* reason) const
        {
            assert(false);
            return TypedValue();
        }
        TypedValue tryConvertTo(const Type& to, const TypedValue& value,
            String* reason) const
        {
            assert(false);
            return TypedValue();
        }
        bool has(IdentifierTemplate<T> memberName) const
        {
            assert(false);
            return false;
        }
    private:
        mutable HashTable<Tyco, Tyco> _instantiations;
    };
    class PartialBody : public Body
    {
    public:
        PartialBody(const Body* root, const Body* parent, Tyco argument)
          : _root(root), _parent(parent), _argument(argument) { }

        String toString() const
        {
            return _argument.toString() + "<" + toString2() + ">";
        }
        String toString2() const
        {
            auto p = dynamic_cast<const PartialBody*>(_parent);
            String s;
            if (p != 0)
                s = p->toString2() + ", ";
            return s + _argument.toString();
        }
        Kind kind() const
        {
            return _parent->kind().instantiate(_argument.kind());
        }
        Type finalInstantiate(const Body* parent, Tyco argument)
            const
        {
            assert(false);
            return Type();
        }

        Tyco partialInstantiate(bool final, Tyco argument) const
        {
            if (final)
                return _root->finalInstantiate(this, argument);
            return new PartialBody(_root, this, argument);
        }
        bool equals(const Tyco::Body* other) const
        {
            auto o = other->as<PartialBody>();
            return o != 0 && _parent->equals(o->_parent) &&
                _argument == o->_argument;
        }
        int hash() const
        {
            return (_parent->hash()*67 + 5)*67 + _argument.hash();
        }
        const Body* parent() const { return _parent; }
        Tyco argument() const { return _argument; }
    private:
        const Body* _root;
        const Body* _parent;
        Tyco _argument;
    };
    TemplateTemplate(const Body* body) : Tyco(body) { }
};

class ArrayType : public Type
{
public:
    ArrayType(const Type& type) : Type(type.as<Body>()) { }
    ArrayType(const Type& contained, const Type& indexer)
      : Type(new Body(contained, indexer)) { }
    ArrayType(const Body* body) : Type(body) { }
    Type contained() const { return body()->contained(); }
    Type indexer() const { return body()->indexer(); }

    class Body : public Type::Body
    {
    public:
        Body(const Type &contained, const Type& indexer)
          : _contained(contained), _indexer(indexer) { }
        String toString() const
        {
            return _contained.toString() + "[" + _indexer.toString() + "]";
        }
        bool equals(const Tyco::Body* other) const
        {
            auto o = other->as<Body>();
            if (o == 0)
                return false;
            return _contained == o->_contained && _indexer == o->_indexer;
        }
        int hash() const
        {
            return (_contained.hash()*67 + 3)*67 + _indexer.hash();
        }
        Type contained() const { return _contained; }
        Type indexer() const { return _indexer; }
    private:
        Type _contained;
        Type _indexer;
    };
private:
    const Body* body() const { return as<Body>(); }
};

class ArrayTemplate : public NamedNullary<Template, ArrayTemplate>
{
public:
    static String name() { return "Array"; }

    class Body : public NamedNullary::Body
    {
    public:
        Kind kind() const
        {
            return TemplateKind(TypeKind(),
                TemplateKind(TypeKind(), TypeKind()));
        }
        Type finalInstantiate(const Template::Body* parent,
            Tyco argument) const
        {
            return ArrayType(
                dynamic_cast<const Template::PartialBody*>(parent)->
                    argument(), argument);
        }
    };
};

template<> Nullary<Template, ArrayTemplate>
    Nullary<Template, ArrayTemplate>::_instance;

class SequenceType : public Type
{
public:
    SequenceType(const Type& contained) : Type(new Body(contained)) { }
    Type contained() const { return body()->contained(); }
private:
    class Body : public Type::Body
    {
    public:
        Body(const Type &contained) : _contained(contained) { }
        String toString() const
        {
            return _contained.toString() + "[]";
        }
        bool equals(const Type::Body* other) const
        {
            auto o = other->as<Body>();
            if (o == 0)
                return false;
            return _contained == o->_contained;
        }
        int hash() const { return _contained.hash()*67 + 4; }
        Type contained() const { return _contained; }
    private:
        Type _contained;
    };
    const Body* body() const { return as<Body>(); }
};

class SequenceTemplate : public NamedNullary<Template, SequenceTemplate>
{
public:
    static String name() { return "Sequence"; }

    class Body : public NamedNullary::Body
    {
    public:
        Kind kind() const { return TemplateKind(TypeKind(), TypeKind()); }
        Type finalInstantiate(const Template::Body* parent,
            Tyco argument) const
        {
            return SequenceType(argument);
        }
    };
};

template<> Nullary<Template, SequenceTemplate>
    Nullary<Template, SequenceTemplate>::_instance;

template<class T> class TupleTycoTemplate;
typedef TupleTycoTemplate<void> TupleTyco;

template<class T> class TupleTycoTemplate
  : public NamedNullary<Tyco, TupleTyco>
{
public:
    TupleTycoTemplate() : NamedNullary(instance()) { }
    TupleTycoTemplate(const Tyco& other) : NamedNullary(other.as<Body>()) { }
    static String name() { return "Tuple"; }
    bool isUnit() { return body() == 0; }
    Tyco instantiate(const Tyco& argument) const
    {
        return _body->instantiate(argument);
    }
    Type lastMember()
    {
        const NonUnitBody* i = body();
        if (i == 0)
            return Type();
        return i->contained();
    }
    TupleTyco firstMembers()
    {
        const NonUnitBody* i = body();
        if (i == 0)
            return TupleTyco();
        return i->parent();
    }
    class Body : public NamedNullary::Body
    {
    public:
        // Tyco
        String toString() const
        {
            bool needComma = false;
            return "(" + toString2(&needComma) + ")";
        }
        virtual String toString2(bool* needComma) const { return ""; }
        bool equals(const Body* other) const
        {
            return this == other;
        }
        Kind kind() const { return VariadicTemplateKind(); }
        int hash() const { return reinterpret_cast<intptr_t>(this); }

        // Type
        TypedValue tryConvert(const TypedValue& value, String* reason) const
        {
            if (this == value.type().body())
                return value;
            return TypedValue();
        }
        TypedValue tryConvertTo(const Type& to, const TypedValue& value,
            String* reason) const
        {
            if (this == to.body())
                return value;
            return TypedValue();
        }
        bool has(IdentifierTemplate<T> memberName) const { return false; }

        // Template
        Tyco instantiate(const Tyco& argument) const
        {
            if (_instantiations.hasKey(argument))
                return _instantiations[argument];

            if (argument.kind() != TypeKind()) {
                throw Exception(String("Cannot use ") + argument.toString() +
                    " (kind " + argument.kind().toString() +
                    ") to instantiate Tuple because it requires a type");
            }

            TupleTyco t(new NonUnitBody(this, argument));
            _instantiations.add(argument, t);
            return t;
        }
    private:
        mutable HashTable<Tyco, Tyco> _instantiations;
    };
    TupleTycoTemplate(const Body* body) : NamedNullary(body) { }
private:

    class NonUnitBody : public Body
    {
    public:
        NonUnitBody(const Body* parent, Type contained)
          : _parent(parent), _contained(contained) { }
        String toString2(bool* needComma) const
        {
            String s = _parent->toString2(needComma);
            if (*needComma)
                s += ", ";
            *needComma = true;
            return s + _contained.toString();
        }
        bool equals(const Body* other) const
        {
            auto o = other->as<NonUnitBody>();
            return _parent->equals(o->_parent) && _contained == o->_contained;
        }
        int hash() const
        {
            return (_parent->hash()*67 + 6)*67 + _contained.hash();
        }

        // Type
        TypedValue tryConvert(const TypedValue& value, String* reason) const
        {
            if (_parent == TupleTyco().body())
                return _contained.tryConvert(value, reason);
            if (this == value.type().body())
                return value;
            return TypedValue();
        }
        TypedValue tryConvertTo(const Type& to, const TypedValue& value,
            String* reason) const
        {
            if (_parent == TupleTyco().body())
                return _contained.tryConvertTo(to, value, reason);
            if (this == to.body())
                return value;
            return TypedValue();
        }
        bool has(IdentifierTemplate<T> memberName) const
        {
            CharacterSource s(memberName.name());
            Rational r;
            if (!Space::parseNumber(&s, &r))
                return false;
            if (r.denominator != 1)
                return false;
            int n = r.numerator;
            if (s.get() != -1)
                return false;
            const NonUnitBody* p = this;
            do {
                if (n == 1)
                    return true;
                --n;
                p = dynamic_cast<const NonUnitBody*>(p->_parent);
                if (p == 0)
                    return false;
            } while (true);
        }
        const Body* parent() const { return _parent; }
        Type contained() const { return _contained; }
    private:
        const Body* _parent;
        Type _contained;
    };
private:
    const NonUnitBody* body() const { return as<NonUnitBody>(); }
};

template<> Nullary<Tyco, TupleTyco> Nullary<Tyco, TupleTyco>::_instance;

class PointerType : public Type
{
public:
    PointerType(const Type& referent) : Type(new Body(referent)) { }
private:
    class Body : public Type::Body
    {
    public:
        Body(const Type &referent) : _referent(referent) { }
        String toString() const { return _referent.toString() + "*"; }
        bool equals(const Type::Body* other) const
        {
            auto o = other->as<Body>();
            if (o == 0)
                return false;
            return _referent == o->_referent;
        }
        int hash() const { return _referent.hash()*67 + 1; }
    private:
        Type _referent;
    };
};

class PointerTemplate : public NamedNullary<Template, PointerTemplate>
{
public:
    static String name() { return "Pointer"; }

    class Body : public NamedNullary::Body
    {
    public:
        Kind kind() const { return TemplateKind(TypeKind(), TypeKind()); }
        Type finalInstantiate(const Template::Body* parent,
            Tyco argument) const
        {
            return PointerType(argument);
        }
    };
};

template<> Nullary<Template, PointerTemplate>
    Nullary<Template, PointerTemplate>::_instance;

template<class T> class FunctionTycoTemplate;
typedef FunctionTycoTemplate<void> FunctionTyco;

template<class T> class FunctionTemplateTemplate;
typedef FunctionTemplateTemplate<void> FunctionTemplate;

template<class T> class FunctionTycoTemplate : public Tyco
{
public:
    FunctionTycoTemplate(const Tyco& t) : Tyco(t) { }

    static FunctionTyco nullary(const Type& returnType)
    {
        return FunctionTyco(new NullaryBody(returnType));
    }
    FunctionTycoTemplate(Type returnType, Type argumentType)
      : Tyco(FunctionTyco(FunctionTemplateTemplate<T>().
            instantiate(returnType)).
            instantiate(argumentType).body()) { }
    FunctionTycoTemplate(Type returnType, Type argumentType1,
        Type argumentType2)
      : Tyco(FunctionTyco(FunctionTyco(FunctionTemplateTemplate<T>().
            instantiate(returnType)).instantiate(argumentType1)).
            instantiate(argumentType2).body()) { }
    int argumentsHash() const { return body()->argumentsHash(); }
    bool argumentsMatch(const List<TypedValue>& arguments) const
    {
        return body()->argumentsMatch(arguments.begin());
    }
    Tyco instantiate(const Tyco& argument) const
    {
        return body()->instantiate(argument);
    }
private:
    FunctionTycoTemplate(const Body* body) : Tyco(body) { }
    class Body : public Tyco::Body
    {
    public:
        String toString() const
        {
            bool needComma = false;
            return toString2(&needComma) + ")";
        }
        virtual String toString2(bool* needComma) const = 0;
        Kind kind() const { return VariadicTemplateKind(); }
        TypedValue tryConvert(const TypedValue& value, String* reason) const
        {
            if (this == value.type().body())
                return value;
            return TypedValue();
        }
        TypedValue tryConvertTo(const Type& to, const TypedValue& value,
            String* reason) const
        {
            if (this == to.body())
                return value;
            return TypedValue();
        }
        virtual bool has(IdentifierTemplate<T> memberName) const
        {
            return false;
        }
        // Template
        Tyco instantiate(const Tyco& argument) const
        {
            if (_instantiations.hasKey(argument))
                return _instantiations[argument];

            if (argument.kind() != TypeKind()) {
                throw Exception(String("Cannot use ") + argument.toString() +
                    " (kind " + argument.kind().toString() +
                    ") to instantiate Function because it requires a type");
            }

            FunctionTyco t(new ArgumentBody(this, argument));
            _instantiations.add(argument, t);
            return t;
        }
        virtual int argumentsHash() const = 0;
        virtual bool argumentsMatch(List<Type>::Iterator i)
    private:
        mutable HashTable<Tyco, Tyco> _instantiations;
    };
    class NullaryBody : public Body
    {
    public:
        NullaryBody(const Type& returnType) : _returnType(returnType) { }
        String toString2(bool* needComma) const
        {
            return _returnType.toString() + "(";
        }
        bool equals(const Tyco::Body* other) const
        {
            auto o = other->as<NullaryBody>();
            if (o == 0)
                return false;
            return (_returnType != o->_returnType);
        }
        int hash() const { return _returnType.hash()*67 + 2; }
        int argumentsHash() const { return 2; }
        bool argumentsMatch(List<Type>::Iterator i) const { return i.end(); }
    private:
        Type _returnType;
    };
    class ArgumentBody : public Body
    {
    public:
        ArgumentBody(const Body* parent, const Type& argumentType)
          : _parent(parent), _argumentType(argumentType) { }
        String toString2(bool* needComma) const
        {
            String s = _parent->toString2(needComma);
            if (*needComma)
                s += ", ";
            *needComma = true;
            return s + _argumentType.toString();
        }
        bool equals(const Tyco::Body* other) const
        {
            auto o = other->as<ArgumentBody>();
            if (o == 0)
                return false;
            return _parent->equals(o->_parent) &&
                _argumentType == o->_argumentType;
        }
        int hash() const { return _parent->hash()*67 + _argumentType.hash(); }
        int argumentsHash() const
        {
            return _parent->argumentsHash()*67 + _argumentType.hash();
        }
        bool argumentsMatch(List<Type>::Iterator i) const
        {
            if (*i != _argumentType)
                return false;
            ++i;
            return _parent->argumentsMatch(i);
        }
    private:
        const Body* _parent;
        Type _argumentType;
    };
    const Body* body() const { return as<Body>(); }
};

template<class T> class FunctionTemplateTemplate
  : public NamedNullary<Template, FunctionTemplate>
{
public:
    static String name() { return "Function"; }

    class Body : public NamedNullary::Body
    {
    public:
        virtual Tyco partialInstantiate(bool final, Tyco argument) const
        {
            return FunctionTyco::nullary(argument);
        }
        Kind kind() const
        {
            return TemplateKind(TypeKind(), VariadicTemplateKind());
        }
        Type finalInstantiate(const Template::Body* parent,
            Tyco argument) const
        {
            assert(false);
            return Type();
        }
    };
};

template<> Nullary<Template, FunctionTemplate>
    Nullary<Template, FunctionTemplate>::_instance;

class EnumerationType : public Type
{
public:
    class Value
    {
    public:
        template<class T> Value(String name, const T& value)
            : _name(name), _value(value)
        { }
        String name() const { return _name; }
        template<class T> T value() const { return _value.value<T>(); }
        Any value() const { return _value; }
    private:
        String _name;
        Any _value;
    };

    EnumerationType(const Type& other) : Type(other.as<Body>()) { }
    EnumerationType(String name, List<Value> values) 
      : Type(new Body(name, values)) { }
    const Array<Value>* values() const { return as<Body>()->values(); }
private:
    class Body : public Type::Body
    {
    public:
        Body(String name, List<Value> values)
          : _name(name), _values(values) { }
        String toString() const { return _name; }
        const Array<Value>* values() const { return &_values; }
    private:
        String _name;
        Array<Value> _values;
    };
};

class LessThanType : public Type
{
public:
    LessThanType(int n) : Type(new Body(n)) { }
private:
    class Body : public Type::Body
    {
    public:
        Body(int n) : _n(n) { }
        String toString() const { return decimal(_n); }

        bool equals(const Type::Body* other) const
        {
            auto o = other->as<Body>();
            if (o == 0)
                return false;
            return _n == o->_n;
        }
        int hash() const { return 7*67 + _n; }

    private:
        int _n;
    };
};

// StructuredType is the type of "{...}" literals, not the base type for all
// types which have members. The ALFE compiler will need a more complicated
// body of structures, including using the same conversions at
// compile-time as at run-time. Also we don't want to have to override
// conversion functions in children just to avoid unwanted conversions

template<class T> class StructuredTypeTemplate : public Type
{
public:
    class Member
    {
    public:
        Member(String name, Type type) : _name(name), _default(type) { }
        Member(String name, TypedValue defaultValue)
          : _name(name), _default(defaultValue) { }
        template<class U> Member(String name, const U& defaultValue)
          : _name(name), _default(defaultValue) { }
        String name() const { return _name; }
        Type type() const { return _default.type(); }
        TypedValue defaultValue() const { return _default; }
        bool hasDefault() const { return _default.valid(); }
        bool operator==(const Member& other) const
        {
            return _name == other._name && type() == other.type();
        }
        bool operator!=(const Member& other) const
        {
            return !operator==(other);
        }
    private:
        String _name;
        TypedValue _default;
    };

    template<class MemberT> static Member member(String name)
    {
        return Member(name, typeFromCompileTimeType<MemberT>());
    }

    StructuredTypeTemplate() { }
    StructuredTypeTemplate(const Type& other) : Type(other) { }
    StructuredTypeTemplate(String name, List<Member> members)
      : Type(new Body(name, members)) { }
    const HashTable<Identifier, int>* names() const
    {
        return body()->names();
    }
    const Array<Member>* members() const
    {
        return body()->members();
    }
    static TypedValue empty()
    {
        return TypedValue(StructuredType(String(),
            List<StructuredType::Member>()),
            Value<HashTable<Identifier, TypedValue>>());
    }
protected:
    class Body : public Type::Body
    {
    public:
        Body(String name, List<Member> members)
          : _name(name), _members(members)
        {
            int n = 0;
            for (auto i = members.begin(); i != members.end(); ++i) {
                _names.add(i->name(), n);
                ++n;
            }
        }
        String toString() const { return _name; }
        const HashTable<Identifier, int>* names() const { return &_names; }
        const Array<Member>* members() const { return &_members; }

        TypedValue tryConvertTo(const Type& to, const TypedValue& value,
            String* why) const
        {
            const Body* toBody = to.as<Body>();
            if (toBody != 0) {
                auto input =
                    value.value<Value<HashTable<Identifier, TypedValue>>>();
                Value<HashTable<Identifier, TypedValue>> output;

                // First take all named members in the RHS and assign them to
                // the corresponding named members in the LHS.
                int count = _members.count();
                int toCount = toBody->_members.count();
                Array<bool> assigned(toCount);
                for (int i = 0; i < toCount; ++i)
                    assigned[i] = false;
                for (int i = 0; i < count; ++i) {
                    const Member* m = &_members[i];
                    String name = m->name();
                    if (name.empty())
                        continue;
                    // If a member doesn't exist, fail conversion.
                    if (!toBody->_names.hasKey(name)) {
                        *why = String("The target type has no member named ") +
                            name;
                        return TypedValue();
                    }
                    int j = toBody->_names[name];
                    if (assigned[j]) {
                        *why = String("The source type has more than one "
                            "member named ") + name;
                        return TypedValue();
                    }
                    // If one of the child conversions fails, fail.
                    TypedValue v = tryConvertHelper((*input)[name],
                        &toBody->_members[j], why);
                    if (!v.valid())
                        return TypedValue();
                    (*output)[name] = v;
                    assigned[j] = true;
                }
                // Then take all unnamed arguments in the RHS and in LTR order
                // and assign them to unassigned members in the LHS, again in
                // LTR order.
                int j = 0;
                for (int i = 0; i < count; ++i) {
                    const Member* m = &_members[i];
                    if (!m->name().empty())
                        continue;
                    String fromName = String::Decimal(i);
                    while (assigned[j] && j < toCount)
                        ++j;
                    if (j >= toCount) {
                        *why = "The source type has too many members";
                        return TypedValue();
                    }
                    const Member* toMember = &toBody->_members[j];
                    ++j;
                    TypedValueTemplate<T> v = tryConvertHelper(
                        (*input)[Identifier(String::Decimal(i))], toMember,
                        why);
                    if (!v.valid())
                        return TypedValue();
                    (*output)[toMember->name()] = v;
                }
                // Make sure any unassigned members have defaults.
                for (;j < toCount; ++j) {
                    if (assigned[j])
                        continue;
                    const Member* toMember = &toBody->_members[j];
                    if (!toMember->hasDefault()) {
                        *why = String("No default value is available for "
                            "target type member ") + toMember->name();
                        return TypedValue();
                    }
                    else
                        (*output)[toMember->name()] = toMember->defaultValue();
                }
                return TypedValue(Type(this), output, value.span());
            }
            ArrayType toArray = to;
            if (toArray.valid()) {
                Type contained = toArray.contained();
                auto input =
                    value.value<Value<HashTable<Identifier, TypedValue>>>();
                List<TypedValue> results;
                for (int i = 0; i < input->count(); ++i) {
                    String name = String::Decimal(i);
                    if (!input->hasKey(name)) {
                        *why = String("Array cannot be initialized with a "
                            "structured value containing named members");
                        return TypedValue();
                    }
                    String reason;
                    TypedValue v =
                        (*input)[name].tryConvertTo(contained, &reason);
                    if (!v.valid()) {
                        *why = String("Cannot convert child member ") + name;
                        if (!reason.empty())
                            *why += String(": ") + reason;
                        return TypedValue();
                    }
                    results.add(v);
                }
                return TypedValue(to, results, value.span());
            }
            TupleTyco toTuple = to;
            if (toTuple.valid()) {
                auto input =
                    value.value<Value<HashTable<Identifier, TypedValue>>>();
                List<TypedValue> results;
                int count = _members.count();
                for (int i = input->count() - 1; i >= 0; --i) {
                    String name = String::Decimal(i);
                    if (!input->hasKey(name)) {
                        *why = String("Tuple cannot be initialized with a "
                            "structured value containing named members");
                        return TypedValue();
                    }
                    if (toTuple.isUnit())
                        return String("Tuple type does not have enough members"
                            " to be initialized with this structured value.");
                    String reason;
                    TypedValue v = (*input)[name].
                        tryConvertTo(toTuple.lastMember(), &reason);
                    if (!v.valid()) {
                        *why = String("Cannot convert child member ") + name;
                        if (!reason.empty())
                            *why += String(": ") + reason;
                        return TypedValue();
                    }
                    results.add(v);
                    toTuple = toTuple.firstMembers();
                }
            }

            return TypedValue();
        }
        bool has(IdentifierTemplate<T> memberName) const
        {
            return _names.hasKey(memberName);
        }
    private:
        TypedValue tryConvertHelper(const TypedValue& value, const Member* to,
            String* why) const
        {
            String reason;
            TypedValue v = value.tryConvertTo(to->type(), &reason);
            if (!v.valid()) {
                *why = String("Cannot convert child member ") + to->name();
                if (!reason.empty())
                    *why += String(": ") + reason;
                return TypedValue();
            }
            return v;
        }

        String _name;
        HashTable<Identifier, int> _names;
        Array<Member> _members;
    };
    const Body* body() const { return as<Body>(); }

    friend class Body;
};

class StringType : public NamedNullary<Type, StringType>
{
public:
    static String name() { return "String"; }
};

template<> Nullary<Type, StringType> Nullary<Type, StringType>::_instance;

class IntegerType : public NamedNullary<Type, IntegerType>
{
public:
    static String name() { return "Integer"; }
};

template<> Nullary<Type, IntegerType> Nullary<Type, IntegerType>::_instance;

class BooleanType : public NamedNullary<Type, BooleanType>
{
public:
    static String name() { return "Boolean"; }
};

template<> Nullary<Type, BooleanType> Nullary<Type, BooleanType>::_instance;

class ObjectType : public NamedNullary<Type, ObjectType>
{
public:
    static String name() { return "Object"; }
};

template<> Nullary<Type, ObjectType> Nullary<Type, ObjectType>::_instance;

class LabelType : public NamedNullary<Type, LabelType>
{
public:
    static String name() { return "Label"; }
};

template<> Nullary<Type, LabelType> Nullary<Type, LabelType>::_instance;

class VoidType : public NamedNullary<Type, VoidType>
{
public:
    static String name() { return "Void"; }
};

template<> Nullary<Type, VoidType> Nullary<Type, VoidType>::_instance;

class DoubleType : public NamedNullary<Type, DoubleType>
{
public:
    static String name() { return "Double"; }
};

template<> Nullary<Type, DoubleType> Nullary<Type, DoubleType>::_instance;

class RationalType : public NamedNullary<Type, RationalType>
{
public:
    static String name() { return "Rational"; }
    class Body : public NamedNullary<Type, RationalType>::Body
    {
    public:
        TypedValue tryConvertTo(const Type& to, const TypedValue& value,
            String* reason) const
        {
            if (this == to.body())
                return value;
            Rational r = value.value<Rational>();
            if (to == DoubleType())
                return r.value<double>();
            if (to == IntegerType()) {
                if (r.denominator == 1)
                    return r.numerator;
                *reason = String("Value is not an integer");
            }
            return TypedValue();
        }
    };
};

template<> Nullary<Type, RationalType> Nullary<Type, RationalType>::_instance;

// ConcreteType is a bit strange. It's really a family of types, but these
// types cannot be instantiated via the usual template syntax. The normal
// constructor takes no arguments, but constructs a different dimension each
// time, so care must be taken to keep track of instantiations and use the
// correct one.
class ConcreteType : public Type
{
    ConcreteType() : Type(Body::create(_bases)) { ++_bases; }

private:
    class BodyHead : public Type::Body
    {
    };
    class Body : public BodyWithArray<BodyHead, int>
    {
    public:
        Body()
        {
            for (int i = 0; i < _size; ++i)
                (*this)[i] = 0;
        }
        String toString() const { return "Concrete"; }
        bool equals(Type::Body* other) const
        {
            throw NotYetImplementedException();
        }
        int hash() const
        {
            throw NotYetImplementedException();
        }
        TypedValue tryConvertTo(const Type& to, const TypedValue& value,
            String* reason) const
        {
            if (this == to.body())
                return value;
            Concrete c = value.value<Concrete>();
            if (!c.dimensionless()) {
                *reason = String("Value is denominate");
                return TypedValue();
            }
            Rational r = c.value();
            if (to == DoubleType())
                return r.value<double>();
            if (to == RationalType())
                return r;
            if (to == IntegerType()) {
                if (r.denominator == 1)
                    return r.numerator;
                *reason = String("Value is not an integer");
            }
            return TypedValue();
        }
    private:
        int get(int i) const
        {
            if (i >= _size)
                return 0;
            return (*this)[i];
        }
        Body* copy(int minSize) const
        {
            int s = max(minSize, _size);
            Body* b = create(s);
            for (int i = 0; i < s; ++i)
                b->set(i, get(i));
            return b;
        }
        void set(int i, int v) { (*this)[i] = v; }
        int size() const { return _size; }

        friend class Concrete;
    };
    static int _bases;
    friend class Concrete;
};

int ConcreteType::_bases = 0;

class VectorType : public NamedNullary<StructuredType, VectorType>
{
public:
    class Body : public StructuredType::Body
    {
    public:
        Body() : StructuredType::Body("Vector", members()) { }
    private:
        List<StructuredType::Member> members()
        {
            List<StructuredType::Member> vectorMembers;
            vectorMembers.add(StructuredType::member<int>("x"));
            vectorMembers.add(StructuredType::member<int>("y"));
            return vectorMembers;
        }
    };
    friend class NamedNullary<StructuredType, VectorType>;
};

template<> Nullary<StructuredType, VectorType>
    Nullary<StructuredType, VectorType>::_instance;

template<> Type typeFromCompileTimeType<int>() { return IntegerType(); }
template<> Type typeFromCompileTimeType<String>() { return StringType(); }
template<> Type typeFromCompileTimeType<bool>() { return BooleanType(); }
template<> Type typeFromCompileTimeType<Vector>() { return VectorType(); }
template<> Type typeFromCompileTimeType<Rational>() { return RationalType(); }
template<> Type typeFromCompileTimeType<double>() { return DoubleType(); }

#endif // INCLUDED_TYPE_H
