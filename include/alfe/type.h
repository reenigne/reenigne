#include "alfe/main.h"

#ifndef INCLUDED_TYPE_H
#define INCLUDED_TYPE_H

template<class T> class TemplateT;
typedef TemplateT<void> Template;

template<class T> class TypeT;
typedef TypeT<void> Type;

template<class T> class PointerTypeT;
typedef PointerTypeT<void> PointerType;

template<class T> class FunctionTypeT;
typedef FunctionTypeT<void> FunctionType;

template<class T> class FunctionTemplateT;
typedef FunctionTemplateT<void> FunctionTemplate;

template<class T> class VoidTypeT;
typedef VoidTypeT<void> VoidType;

template<class T> class TupleTycoT;
typedef TupleTycoT<void> TupleTyco;

template<class T> class StringTypeT;
typedef StringTypeT<void> StringType;

template<class T> class IntegerTypeT;
typedef IntegerTypeT<void> IntegerType;

template<class T> class RationalTypeT;
typedef RationalTypeT<void> RationalType;

#include "alfe/any.h"
#include "alfe/hash_table.h"
#include "alfe/nullary.h"
#include "alfe/kind.h"
#include "alfe/assert.h"
#include "alfe/identifier.h"
#include "alfe/vectors.h"
#include "alfe/rational.h"
#include "alfe/reference.h"
#include "alfe/statement.h"
#include <type_traits>

template<class T> class ValueT;
typedef ValueT<void> Value;

template<class T> class TycoT;
typedef TycoT<void> Tyco;

template<class T> class IdentifierT;
typedef IdentifierT<void> Identifier;

template<class T> class LValueTypeT;
typedef LValueTypeT<void> LValueType;

template<class T> class StructuredTypeT;
typedef StructuredTypeT<void> StructuredType;

template<class T> class BooleanTypeT;
typedef BooleanTypeT<void> BooleanType;

template<class T> class TycoT : public Handle
{
public:
    TycoT() { }
    TycoT(const Handle& other) : Handle(to<Body>(other)) { }
    String toString() { return body()->toString(); }
    Kind kind() { return body()->kind(); }
    CodeWalker::Result walk(CodeWalker* walker)
    {
        return body()->walk(walker);
    }
protected:
    class Body : public Handle::Body
    {
    public:
        virtual String toString() = 0;
        virtual Kind kind() = 0;
        Tyco tyco() { return handle<Tyco>(); }
        virtual CodeWalker::Result walk(CodeWalker* walker)
        {
            return walker->visit(tyco());
        }
    };
private:
    Body* body() { return as<Body>(); }

    friend class TemplateT<void>;
    template<class U> friend class EnumerationType;
    template<class U> friend class StructuredTypeT;
};

template<class T> class StructureT;
typedef StructureT<void> Structure;

class StructureOwner
{
public:
    void addOwned(Reference<Structure> structure) { _owned.add(structure); }
private:
    List<Reference<Structure>> _owned;
};

template<class T> class StructureT
{
public:
    template<class U> U get(Identifier identifier) const
    {
        Value v = getValue(identifier);
        StructuredType t(v.type().rValue());
        if (t.valid())
            return t.rValueFromLValue(v).template value<U>();
        return v.template value<U>();
    }
    virtual ValueT<T> getValue(Identifier identifier) const
    {
        return _values[identifier];
    }
    bool has(Identifier identifier) const
    {
        return _values.hasKey(identifier);
    }
    virtual void set(Identifier identifier, ValueT<T> value, Span span)
    {
        _values[identifier] = value;
    }
    HashTable<Identifier, Value>::Iterator begin() const
    {
        return _values.begin();
    }
    HashTable<Identifier, Value>::Iterator end() const
    {
        return _values.end();
    }
private:
    HashTable<Identifier, Value> _values;
};

template<class T> class LValueT;
typedef LValueT<void> LValue;

template<class T> class LValueT
{
public:
    LValueT(Structure* structure, Identifier identifier)
      : _structure(structure), _identifier(identifier) { }
    ValueT<T> rValue() const
    {
        return _structure->getValue(_identifier);
    }
    void set(ValueT<T> value, Span span) const
    {
        _structure->set(_identifier, value, span);
    }
    bool operator==(const LValue& other) const
    {
        return _structure == other._structure &&
            _identifier == other._identifier;
    }
    LValueT member(Identifier identifier)
    {
        return LValueT(_structure->getValue(_identifier).value<Structure*>(),
            identifier);
    }
private:
    Structure* _structure;
    Identifier _identifier;
};

template<class T> class TypeT : public Tyco
{
public:
    TypeT() { }
    TypeT(const Handle& other) : Tyco(other) { }
    TypeT(const Tyco& other) : Tyco(to<Body>(other)) { }

    Type member(IdentifierT<T> i) { return body()->member(i); }
    Type rValue() const
    {
        if (LValueTypeT<T>(*this).valid())
            return LValueTypeT<T>(*this).inner();
        return *this;
    }
    // All measurements in characters (== bytes, no unicode support yet).
    // "width" is maximum total width of file not including line terminator
    // (e.g. 79 characters).
    // "used" is the number of characters that are already used on the left
    // (including indentation).
    // "indent" is the number of spaces to indent on any new lines. If 0 then
    // we'll exit with "*" if the result doesn't fit in the line.
    // "delta" is the number of spaces by which the indent should be increased
    // when going in a level.
    // We will leave enough space at the end for a trailing comma.
    String serialize(void* p, int width, int used, int indent, int delta)
    {
        return body()->serialize(p, width, used, indent, delta);
    }
    void deserialize(const Value& value, void* p)
    {
        body()->deserialize(value, p);
    }
    int size() { return body()->size(); }
    ValueT<T> value(void* p) { return body()->value(p); }
    ValueT<T> simplify(const Value& value)
    {
        return body()->simplify(value);
    }
    bool canConvertFrom(Type other, String* reason = 0)
    {
        if (*this == other)
            return true;
        if (other == StructuredTypeT<T>::empty().type() &&
            body()->defaultValue().valid())
            return true;
        String reasonFrom;
        if (body()->canConvertFrom(other, &reasonFrom))
            return true;
        String reasonTo;
        if (other.body()->canConvertTo(*this, &reasonTo))
            return true;
        if (reason != 0) {
            if (reasonFrom != "")
                *reason = reasonFrom;
            else
                *reason = reasonTo;
        }
        return false;
    }
    bool canConvertTo(Type other, String* reason = 0)
    {
        return other.canConvertFrom(*this, reason);
    }
    ValueT<T> convert(const ValueT<T>& value)
    {
        if (*this == value.type())
            return value;
        if (value == StructuredTypeT<T>::empty()) {
            ValueT<T> v = body()->defaultValue();
            if (v.valid())
                return v;
        }
        String reasonFrom;
        if (body()->canConvertFrom(value.type(), &reasonFrom))
            return body()->convert(value);
        return value.type().body()->convertTo(*this, value);
    }
    ValueT<T> convertTo(Type to, const ValueT<T>& value) const
    {
        assert(*this == value.type());
        return to.convert(value);
    }
    ValueT<T> defaultValue() { return body()->defaultValue(); }
protected:
    class Body : public Tyco::Body
    {
    public:
        Kind kind() { return TypeKind(); }
        virtual Type member(IdentifierT<T> i) { return Type(); }
        virtual String serialize(void* p, int width, int used, int indent,
            int delta)
        {
            return "";
        }
        virtual void deserialize(const Value& value, void* p) { }
        virtual int size() { return 0; }
        virtual ValueT<T> defaultValue() { return Value(); }
        virtual ValueT<T> value(void* p) { return Value(); }
        Type type() { return tyco(); }
        virtual ValueT<T> simplify(const Value& value)
        {
            return value;
        }
        virtual bool canConvertFrom(const Type& other, String* reason)
        {
            return false;
        }
        virtual bool canConvertTo(const Type& other, String* reason)
        {
            return false;
        }
        virtual ValueT<T> convert(const ValueT<T>& value)
        {
            return ValueT<T>();
        }
        virtual ValueT<T> convertTo(const Type& to, const Value& value)
        {
            return ValueT<T>();
        }
        virtual ValueT<T> valueFromAny(Any a, Structure* owner)
        {
            return ValueT<T>(type(), a);
        }
        virtual Any anyFromValue(ValueT<T> v) { return v.value(); }
    };
    Body* body() { return as<Body>(); }

    friend class TemplateT<void>;
};

template<class T> class LValueTypeT : public Type
{
public:
    LValueTypeT(const Type& type) : Type(to<Body>(type)) { }
    static LValueType wrap(const Type& inner)
    {
        if (LValueType(inner).valid())
            return inner;
        return create<Body>(inner);
    }
    Type inner() { return body()->inner(); }
private:
    class Body : public Type::Body
    {
    public:
        Body(Type inner) : _inner(inner) { }
        Type inner() { return _inner; }
        String toString() { return "LValue<" + _inner.toString() + ">"; }
        CodeWalker::Result walk(CodeWalker* walker)
        {
            auto r = walker->visit(tyco());
            if (r == CodeWalker::Result::recurse) {
                if (_inner.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
            }
            return r;
        }
    private:
        Type _inner;
    };

    Body* body() { return as<Body>(); }
    LValueTypeT(const Handle& type) : Type(to<Body>(type)) { }
};

template<typename T> class HasType
{
    template <typename U, Type (U::*)() const> struct Check;
    template <typename U> static char func(Check<U, &U::type> *) { }
    template <typename U> static int func(...) { }
public:
    typedef HasType type;
    enum { value = sizeof(func<T>(0)) == sizeof(char) };
};

template<class T> Type typeFromCompileTimeType() { return T::type(); }
// TODO: For this to work with string literals, need to use class template
// specialization, not function template specialization.
//template<size_t N> Type typeFromCompileTimeType<const char (&)[N]>() { return StringType(); }
template<class T, std::enable_if_t<HasType<T>::value>* = nullptr>
    Type typeFromValue(const T& value)
{
    return value.type();
}
template<class T, typename = void, std::enable_if_t<!HasType<T>::value>* =
    nullptr>
    Type typeFromValue(const T&)
{
    return typeFromCompileTimeType<T>();
}

template<class T> class ValueT
{
public:
    ValueT() { }
    template<class U, std::enable_if_t<std::is_base_of<Type, U>::value>* =
        nullptr>
        ValueT(U type, Any any, Span span = Span())
      : _type(type), _any(any), _span(span) { }
    template<class U, std::enable_if_t<std::is_base_of<Type, U>::value>* =
        nullptr>
        ValueT(U type, Span span = Span())
      : _type(type), _span(span)
    {
        _any = StructuredTypeT<T>::empty().convertTo(type).value();
    }
    template<class U, std::enable_if_t<!std::is_base_of<Type, U>::value>* =
        nullptr> ValueT(const U& value, Span span = Span())
      : _type(typeFromValue(value)), _any(value), _span(span) { }
    Type type() const { return _type; }
    Any value() const { return _any; }
    bool operator==(const Value& other) const
    {
        return _type == other._type && _any == other._any;
    }
    bool operator!=(const Value& other) const { return !(*this == other); }
    template<class U> U value() const { return _any.value<U>(); }
    Span span() const { return _span; }
    bool valid() const { return _type.valid(); }
    Value convertTo(const Type& to)
    {
        String reason;
        Value v = tryConvertTo(to, &reason);
        if (!v.valid())
            span().throwError(reason);
        return v;
    }
    Value tryConvertTo(Type to, String* why)
    {
        String reason;
        if (to.canConvertFrom(_type, &reason))
            return to.convert(*this);
        String r = "No conversion";
        String f = _type.toString();
        if (f != "")
            r += " from type " + f;
        r += " to type " + to.toString() + " is available";
        if (reason.empty())
            r += ".";
        else
            r += ": " + reason;
        *why = r;
        return Value();
    }
    Value rValue() const
    {
        LValueType lValueType(_type);
        if (lValueType.valid()) {
            Value r = value<LValue>().rValue();
            return Value(r.type(), r.value(), _span);
        }
        return *this;
    }
    Value simplify() { return _type.simplify(*this); }
private:
    Type _type;
    Any _any;
    Span _span;
};

template<class T> class TemplateT : public Tyco
{
public:
    TemplateT(const Handle& other) : Tyco(other) { }
    Tyco instantiate(const Tyco& argument)
    {
        return body()->instantiate(argument);
    }
    Tyco argument() const { return as<PartialBody>()->argument(); }
protected:
    class Body : public Tyco::Body
    {
    public:
        virtual Tyco instantiate(Tyco argument)
        {
            if (_instantiations.hasKey(argument))
                return _instantiations[argument];

            Kind k = kind();
            Kind resultKind = k.instantiate(argument.kind());
            if (!resultKind.valid()) {
                throw Exception("Cannot use " + argument.toString() +
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
        virtual Tyco partialInstantiate(bool final, Tyco argument)
        {
            if (final)
                return finalInstantiate(tyco(), argument);
            return create<PartialBody>(tyco(), tyco(), argument);
        }
        virtual Type finalInstantiate(Template parent, Tyco argument) = 0;
    private:
        mutable HashTable<Tyco, Tyco> _instantiations;
    };
    class PartialBody : public Body
    {
    public:
        PartialBody(Template root, Template parent, Tyco argument)
          : _root(root), _parent(parent), _argument(argument) { }

        String toString()
        {
            return _argument.toString() + "<" + toString2() + ">";
        }
        String toString2()
        {
            auto p = _parent.to<PartialBody>();
            String s;
            if (p != 0)
                s = p->toString2() + ", ";
            return s + _argument.toString();
        }
        Kind kind()
        {
            return _parent.kind().instantiate(_argument.kind());
        }
        Type finalInstantiate(Template parent, Tyco argument)
        {
            assert(false);
            return Type();
        }

        Tyco partialInstantiate(bool final, Tyco argument)
        {
            if (final)
                return _root.body()->finalInstantiate(this->tyco(), argument);
            return create<PartialBody>(_root, this->tyco(), argument);
        }
        bool equals(HandleBase::Body* other)
        {
            auto o = other->to<PartialBody>();
            return o != 0 && Template(_parent) == Template(o->_parent) &&
                _argument == o->_argument;
        }
        Hash hash() { return Body::hash().mixin(_argument.hash()); }
        const Body* parent() { return _parent; }
        Tyco argument() { return _argument; }
    private:
        TemplateT<T> _root;
        TemplateT<T> _parent;
        Tyco _argument;
    };
    Body* body() const { return as<Body>(); }
};

class LessThanType : public Type
{
public:
    LessThanType(Type type) : Type(to<Body>(type)) { }
    LessThanType(int n) : Type(create<Body>(n)) { }
    int n() const { return body()->_n; }
private:
    class Body : public Type::Body
    {
    public:
        Body(int n) : _n(n) { }
        String toString() { return decimal(_n); }

        bool equals(HandleBase::Body* other)
        {
            auto o = other->toConst<Body>();
            return o != 0 && _n == o->_n;
        }
        Hash hash() { return Type::Body::hash().mixin(_n); }
        int _n;
    };
    const Body* body() const { return as<Body>(); }
};

template<class T> class StringTypeT : public NamedNullary<Type, StringType>
{
public:
    static String name() { return "String"; }
    class Body : public NamedNullary<Type, StringType>::Body
    {
    public:
        String serialize(void* p, int width, int used, int indent, int delta)
        {
            String r = "\"";
            String s = *static_cast<String*>(p);
            for (int i = 0; i < s.length(); ++i) {
                Byte b = s[i];
                if (b == '\\' || b == '\"')
                    r += "\\";
                r += String::Byte(b);
            }
            return r + "\"";
        }
        void deserialize(const Value& value, void* p)
        {
            *static_cast<String*>(p) = value.value<String>();
        }
        int size() { return sizeof(String); }
        Value defaultValue() { return String(); }
        Value value(void* p) { return *static_cast<String*>(p); }
    };
};

template<class T> class IntegerTypeT : public NamedNullary<Type, IntegerType>
{
public:
    IntegerTypeT() { }
    IntegerTypeT(const Handle& other) : NamedNullary(other) { }
    static String name() { return "Integer"; }
    class Body : public NamedNullary<Type, IntegerType>::Body
    {
    public:
        String serialize(void* p, int width, int used, int indent, int delta)
        {
            return decimal(*static_cast<int*>(p));
        }
        void deserialize(const Value& value, void* p)
        {
            *static_cast<int*>(p) = value.value<int>();
        }
        int size() { return sizeof(int); }
        Value defaultValue() { return 0; }
        Value value(void* p) { return *static_cast<int*>(p); }
    };
};

template<class T> class BooleanTypeT
  : public NamedNullary<Type, BooleanTypeT<T>>
{
public:
    static String name() { return "Boolean"; }
    class Body : public NamedNullary<Type, BooleanType>::Body
    {
    public:
        String serialize(void* p, int width, int used, int indent, int delta)
        {
            return String::Boolean(*static_cast<bool*>(p));
        }
        void deserialize(const Value& value, void* p)
        {
            *static_cast<bool*>(p) = value.value<bool>();
        }
        int size() { return sizeof(bool); }
        Value defaultValue() { return false; }
        Value value(void* p) { return *static_cast<bool*>(p); }
    };
};

class ObjectType : public NamedNullary<Type, ObjectType>
{
public:
    static String name() { return "Object"; }
};

class LabelType : public NamedNullary<Type, LabelType>
{
protected:
    LabelType(const Handle& other) : NamedNullary(other) { }
public:
    static String name() { return "Label"; }
};

class SpecificLabelType : public LabelType
{
public:
    SpecificLabelType() : LabelType(create<Body>()) { }
private:
    class Body : public Type::Body
    {
    public:
        Body() { }
        bool equals(HandleBase::Body* other)
        {
            return other->to<Body>() == this;
        }
        Hash hash()
        {
            return Type::Body::hash().mixin(
                (UInt32)reinterpret_cast<uintptr_t>(this));
        }
    };
};

template<class T> class VoidTypeT : public NamedNullary<Type, VoidType>
{
public:
    static String name() { return "Void"; }
};

class DoubleType : public NamedNullary<Type, DoubleType>
{
public:
    static String name() { return "Double"; }
    class Body : public NamedNullary<Type, DoubleType>::Body
    {
    public:
        bool canConvertFrom(const Type& from, String* reason)
        {
            return from == IntegerType();
        }
        Value convert(const Value& value)
        {
            return Value(DoubleType(), static_cast<double>(value.value<int>()),
                value.span());
        }
    };
};

class ByteType : public NamedNullary<Type, ByteType>
{
public:
    static String name() { return "Byte"; }
    class Body : public NamedNullary<Type, ByteType>::Body
    {
    public:
        String serialize(void* p, int width, int used, int indent, int delta)
        {
            return hex(*static_cast<Byte*>(p), 2);
        }
        void deserialize(const Value& value, void* p)
        {
            *static_cast<Byte*>(p) = value.value<int>();
        }
        int size() { return sizeof(Byte); }
        Value defaultValue() { return 0; }
        Value value(void* p)
        {
            return Value(ByteType(), static_cast<int>(*static_cast<Byte*>(p)));
        }
    };
};

class WordType : public NamedNullary<Type, WordType>
{
public:
    static String name() { return "Word"; }
    class Body : public NamedNullary<Type, WordType>::Body
    {
    public:
        String serialize(void* p, int width, int used, int indent, int delta)
        {
            return hex(*static_cast<Word*>(p), 4);
        }
        void deserialize(const Value& value, void* p)
        {
            *static_cast<Word*>(p) = value.value<int>();
        }
        int size() { return sizeof(Word); }
        Value defaultValue() { return 0; }
        Value value(void* p)
        {
            return Value(WordType(), static_cast<int>(*static_cast<Word*>(p)));
        }
    };
};

class DWordType : public NamedNullary<Type, DWordType>
{
public:
    static String name() { return "DWord"; }
    class Body : public NamedNullary<Type, DWordType>::Body
    {
    public:
        String serialize(void* p, int width, int used, int indent, int delta)
        {
            return hex(*static_cast<DWord*>(p), 4);
        }
        void deserialize(const Value& value, void* p)
        {
            *static_cast<DWord*>(p) = value.value<int>();
        }
        int size() { return sizeof(DWord); }
        Value defaultValue() { return 0; }
        Value value(void* p)
        {
            return Value(DWordType(),
                static_cast<int>(*static_cast<DWord*>(p)));
        }
    };
};

template<class T> class RationalTypeT : public NamedNullary<Type, RationalType>
{
public:
    static String name() { return "Rational"; }
    class Body : public NamedNullary<Type, RationalType>::Body
    {
    public:
        bool canConvertFrom(const Type& from, String* reason)
        {
            return from == IntegerType();
        }
        bool canConvertTo(const Type& to, String* reason)
        {
            // Eventually we may want something more sophisticated here, since
            // in general the conversion from Rational to Double would be
            // lossy.
            return to == DoubleType();
        }
        Value convert(const Value& value)
        {
            return Value(RationalType(), Rational(value.value<int>()),
                value.span());
        }
        Value convertTo(const Type& to, const Value& value)
        {
            return Value(DoubleType(), value.value<Rational>().value<double>(),
                value.span());
        }
        Value simplify(const Value& value)
        {
            Rational r = value.value<Rational>();
            if (r.denominator == 1)
                return Value(IntegerType(), r.numerator, value.span());
            return value;
        }
        String serialize(void* p, int width, int used, int indent, int delta)
        {
            auto r = static_cast<Rational*>(p);
            return String(decimal(r->numerator)) + "/" +
                decimal(r->denominator);
        }
        void deserialize(const Value& value, void* p)
        {
            *static_cast<Rational*>(p) = value.value<Rational>();
        }
        int size() { return sizeof(Rational); }
        Value defaultValue()
        {
            return Value(RationalType(), Rational(0));
        }
        Value value(void* p)
        {
            return Value(RationalType(), *static_cast<Rational*>(p));
        }
    };
};

class ArrayType : public Type
{
public:
    ArrayType(const Type& type) : Type(to<Body>(type)) { }
    ArrayType(const Type& contained, const Type& indexer)
      : Type(create<Body>(contained, indexer)) { }
    ArrayType(const Type& contained, int size)
      : Type(create<Body>(contained, LessThanType(size))) { }
    Type contained() { return body()->contained(); }
    Type indexer() { return body()->indexer(); }
protected:
    class Body : public Type::Body
    {
    public:
        Body(const Type &contained, const Type& indexer)
          : _contained(contained), _indexer(indexer) { }
        String toString()
        {
            return _contained.toString() + "[" + _indexer.toString() + "]";
        }
        bool equals(HandleBase::Body* other)
        {
            auto o = other->toConst<Body>();
            return o != 0 && _contained == o->_contained &&
                _indexer == o->_indexer;
        }
        Hash hash()
        {
            return Type::Body::hash().mixin(_contained.hash()).
                mixin(_indexer.hash());
        }
        Type contained() { return _contained; }
        Type indexer() { return _indexer; }
        String serialize(void* p, int width, int used, int indent, int delta)
        {
            int n;
            char* pc0 = pointerAndCount(p, &n);
            char* pc = pc0;
            int size = _contained.size();
            Value d(_contained);
            do {
                if (d !=
                    _contained.value(static_cast<void*>(pc + (n - 1)*size)))
                    break;
                --n;
            } while (n > 0);
            // First try putting everything on one line
            String s("{ ");
            bool needComma = false;
            bool separate = false;
            used += 5;
            void* pp = p;
            for (int i = 0; i < n; ++i) {
                if (used > width) {
                    separate = true;
                    break;
                }
                int u = used + (needComma ? 2 : 0);
                String v = _contained.serialize(pp, width, u, 0, 0);
                if (v == "*") {
                    separate = true;
                    s = "";
                    break;
                }
                if (needComma)
                    s += ", ";
                needComma = true;
                s += v;
                used = u + v.length();
                pc += size;
                pp = static_cast<void*>(pc);
            }
            if (s == "{ ")
                return "{ }";
            if (!separate && used <= width)
                return s + " }";
            else
                s = "";
            if (s == "" && indent == 0)
                return "*";
            // It doesn't all fit on one line, put each member on a separate
            // line.
            s = "{\n";
            needComma = false;
            pc = pc0;
            for (int i = 0; i < n; ++i) {
                int u = indent + delta;
                String v = "{ }";
                if (_contained.value(p) != d) {
                    v = _contained.serialize(p, width, u, indent + delta,
                        delta);
                }
                if (needComma)
                    s += ",\n";
                needComma = true;
                s += String(" ")*indent + v;
                pc += size;
                p = static_cast<void*>(pc);
            }
            return s + " }";
        }
        void deserialize(const Value& value, void* p)
        {
            LessThanType l(_indexer);
            if (!l.valid()) {
                throw Exception(
                    "Don't know how many elements to deserialize.");
            }
            int n = l.n();
            auto v = value.value<List<Value>>();
            char* pc = static_cast<char*>(p);
            int size = _contained.size();
            for (auto vv : v) {
                if (n == 0)
                    break;
                _contained.deserialize(vv, p);
                pc += size;
                p = static_cast<void*>(pc);
                --n;
            }
            for (int i = 0; i < n; ++i) {
                _contained.deserialize(Value(_contained), p);
                pc += size;
                p = static_cast<void*>(pc);
            }
        }
        Value value(void* p)
        {
            int n;
            char* pc = pointerAndCount(p, &n);
            List<Value> v;
            int size = _contained.size();
            for (int i = 0; i < n; ++i) {
                v.add(_contained.value(static_cast<void*>(pc)));
                pc += size;
            }
            return Value(type(), v);
        }
        CodeWalker::Result walk(CodeWalker* walker)
        {
            auto r = walker->visit(tyco());
            if (r == CodeWalker::Result::recurse) {
                if (_contained.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
                if (_indexer.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
            }
            return r;
        }
    protected:
        virtual int elementCount(void* p) { unknownCount(); return 0; }
        virtual void* elementData(void* p) { return 0; }
    private:
        char* pointerAndCount(void* p, int *n)
        {
            LessThanType l(_indexer);
            if (l.valid()) {
                *n = l.n();
                return static_cast<char*>(p);
            }
            if (_indexer == IntegerType()) {
                *n = elementCount(p);
                return static_cast<char*>(elementData(p));
            }
            unknownCount();
            return 0;
        }
        void unknownCount()
        {
            throw Exception("Don't know how many elements to serialize.");
        }
        Type _contained;
        Type _indexer;
    };
private:
    Body* body() { return as<Body>(); }
};

template<class T> class ArrayPersistType : public ArrayType
{
public:
    ArrayPersistType(const Type& type) : ArrayType(type) { }
    ArrayPersistType(const Type& contained, const Type& indexer)
      : ArrayType(contained, indexer) { }
    ArrayPersistType(const Type& contained, int size)
      : ArrayType(contained, size) { }
protected:
    class Body : public ArrayType::Body
    {
    protected:
        virtual int elementCount(void* p)
        {
            return static_cast<Array<T>*>(p)->count();
        }
        virtual void* elementData(void* p)
        {
            return static_cast<void*>(&(*static_cast<Array<T>*>(p))[0]);
        }
    };
};

class ArrayTemplate : public NamedNullary<Template, ArrayTemplate>
{
public:
    static String name() { return "Array"; }

    class Body : public NamedNullary::Body
    {
    public:
        Kind kind()
        {
            return TemplateKind(TypeKind(),
                TemplateKind(TypeKind(), TypeKind()));
        }
        Type finalInstantiate(Template parent, Tyco argument)
        {
            return ArrayType(parent.argument(), argument);
        }
    };
};

class SequenceType : public Type
{
public:
    SequenceType(const Type& contained) : Type(create<Body>(contained)) { }
    Type contained() { return body()->contained(); }
private:
    class Body : public Type::Body
    {
    public:
        Body(const Type &contained) : _contained(contained) { }
        String toString()
        {
            return _contained.toString() + "[]";
        }
        bool equals(HandleBase::Body* other)
        {
            auto o = other->toConst<Body>();
            return o != 0 && _contained == o->_contained;
        }
        Hash hash()
        {
            return Type::Body::hash().mixin(_contained.hash());
        }
        Type contained() { return _contained; }
        CodeWalker::Result walk(CodeWalker* walker)
        {
            auto r = walker->visit(tyco());
            if (r == CodeWalker::Result::recurse) {
                if (_contained.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
            }
            return r;
        }
    private:
        Type _contained;
    };
    Body* body() { return as<Body>(); }
};

class SequenceTemplate : public NamedNullary<Template, SequenceTemplate>
{
public:
    static String name() { return "Sequence"; }

    class Body : public NamedNullary::Body
    {
    public:
        Kind kind() { return TemplateKind(TypeKind(), TypeKind()); }
        Type finalInstantiate(Template parent, Tyco argument)
        {
            return SequenceType(argument);
        }
    };
};

template<class T> class TupleTycoT : public NamedNullary<Tyco, TupleTyco>
{
public:
    TupleTycoT() : NamedNullary(instance()) { }
    static String name() { return "Tuple"; }
    bool isUnit() { return *this == TupleTyco(); }
    Tyco instantiate(const Tyco& argument)
    {
        return body()->instantiate(argument);
    }
    Type lastMember()
    {
        auto i = to<NonUnitBody>();
        if (i == 0)
            return Type();
        return i->contained();
    }
    TupleTyco firstMembers()
    {
        auto i = to<NonUnitBody>();
        if (i == 0)
            return TupleTyco();
        return i->parent();
    }
    class Body : public NamedNullary::Body
    {
    public:
        // Tyco
        String toString()
        {
            bool needComma = false;
            return "(" + toString2(&needComma) + ")";
        }
        virtual String toString2(bool* needComma) { return ""; }
        Kind kind() { return VariadicTemplateKind(); }

        // Template
        Tyco instantiate(Tyco argument)
        {
            if (_instantiations.hasKey(argument))
                return _instantiations[argument];

            if (argument.kind() != TypeKind()) {
                throw Exception(String("Cannot use ") + argument.toString() +
                    " (kind " + argument.kind().toString() +
                    ") to instantiate Tuple because it requires a type");
            }

            TupleTyco t(create<NonUnitBody>(handle<TupleTyco>(), argument));
            _instantiations.add(argument, t);
            return t;
        }
    private:
        mutable HashTable<Tyco, Tyco> _instantiations;
    };
    TupleTycoT(const Handle& other) : NamedNullary(other) { }
private:

    class NonUnitBody : public Body
    {
    public:
        NonUnitBody(TupleTyco parent, Type contained)
          : _parent(parent), _contained(contained) { }
        String toString2(bool* needComma)
        {
            String s = _parent.toString2(needComma);
            if (*needComma)
                s += ", ";
            *needComma = true;
            return s + _contained.toString();
        }
        bool equals(HandleBase::Body* other)
        {
            auto o = other->to<NonUnitBody>();
            return o != 0 && _parent == o->_parent &&
                _contained == o->_contained;
        }
        Hash hash()
        {
            return Body::hash().mixin(_parent.hash()).mixin(_contained.hash());
        }

        // Type
        bool canConvertFrom(const Type& from, String* reason)
        {
            if (_parent == TupleTyco())
                return _contained.canConvertFrom(from, reason);
            return false;
        }
        bool canConvertTo(const Type& to, String* reason)
        {
            if (_parent == TupleTyco())
                return _contained.canConvertTo(to, reason);
            return false;
        }
        Value convert(const Value& value)
        {
            return _contained.convert(value);
        }
        Value convertTo(const Type& to, const Value& value)
        {
            return _contained.convertTo(to, value);
        }
        Type member(IdentifierT<T> i)
        {
            CharacterSource s(i.name());
            Rational r;
            if (!Space::parseNumber(&s, &r))
                return Type();
            if (r.denominator != 1)
                return Type();
            int n = r.numerator;
            if (s.get() != -1)
                return Type();
            TupleTyco p(this);
            do {
                if (p.isUnit())
                    return Type();
                if (n == 1)
                    return p.as<NonUnitBody>()->contained();
                --n;
                p = p.parent();
            } while (true);
        }
        Type contained() { return _contained; }
        TupleTyco parent() { return _parent; }
        CodeWalker::Result walk(CodeWalker* walker)
        {
            auto r = walker->visit(tyco());
            if (r == CodeWalker::Result::recurse) {
                if (_parent.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
                if (_contained.walker(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
            }
            return r;
        }
    private:
        TupleTycoT<T> _parent;
        Type _contained;
    };
private:
    String toString2(bool* needComma) { return body()->toString2(needComma); }
    Body* body() { return as<Body>(); }
    TupleTyco parent() const { return as<NonUnitBody>()->parent(); }
    friend class Body;
    friend class NonUnitBody;
};

template<class T> class PointerTypeT : public Type
{
public:
    PointerTypeT(const Type& referent) : Type(create<Body>(referent)) { }
private:
    class Body : public Type::Body
    {
    public:
        Body(const Type &referent) : _referent(referent) { }
        String toString() { return _referent.toString() + "*"; }
        bool equals(HandleBase::Body* other)
        {
            auto o = other->to<Body>();
            return o != 0 && _referent == o->_referent;
        }
        Hash hash()
        {
            return Type::Body::hash().mixin(_referent.hash());
        }
        CodeWalker::Result walk(CodeWalker* walker)
        {
            auto r = walker->visit(tyco());
            if (r == CodeWalker::Result::recurse) {
                if (_referent.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
            }
            return r;
        }
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
        Kind kind() { return TemplateKind(TypeKind(), TypeKind()); }
        Type finalInstantiate(Template parent, Tyco argument)
        {
            return PointerType(argument);
        }
    };
};

template<class T> class FunctionTypeT : public Tyco
{
public:
    FunctionTypeT(const Tyco& t) : Tyco(t) { }

    static FunctionType nullary(const Type& returnType)
    {
        return create<NullaryBody>(returnType);
    }
    FunctionTypeT(Type returnType, Type argumentType)
      : Tyco(FunctionType(
            FunctionTemplateT<T>().instantiate(returnType)).
            instantiate(argumentType)) { }
    FunctionTypeT(Type returnType, Type argumentType1,
        Type argumentType2)
      : Tyco(FunctionType(FunctionType(FunctionTemplateT<T>().
            instantiate(returnType)).instantiate(argumentType1)).
            instantiate(argumentType2)) { }
    bool argumentsMatch(List<Type>::Iterator argumentTypes) const
    {
        return body()->argumentsMatch(&argumentTypes) && argumentTypes.end();
    }
    Tyco instantiate(const Tyco& argument)
    {
        return body()->instantiate(argument);
    }
    bool isNullary() const { return body()->isNullary(); }
    FunctionType parent() const { return body()->parent(); }
    void addParameterTycos(List<Tyco>* list) const
    {
        body()->addParameterTycos(list);
    }
    Type returnType() const { return body()->returnType(); }
private:
    class Body : public Tyco::Body
    {
    public:
        String toString()
        {
            bool needComma = false;
            return toString2(&needComma) + ")";
        }
        virtual String toString2(bool* needComma) = 0;
        Kind kind() { return VariadicTemplateKind(); }
        // Template
        Tyco instantiate(Tyco argument)
        {
            if (_instantiations.hasKey(argument))
                return _instantiations[argument];

            if (argument.kind() != TypeKind()) {
                throw Exception("Cannot use " + argument.toString() +
                    " (kind " + argument.kind().toString() +
                    ") to instantiate Function because it requires a type");
            }

            FunctionType t(create<ArgumentBody>(tyco(), argument));
            _instantiations.add(argument, t);
            return t;
        }
        virtual bool argumentsMatch(List<Type>::Iterator* i) = 0;
        virtual void addParameterTycos(List<Tyco>* list) = 0;
        virtual Type returnType() = 0;
    private:
        mutable HashTable<Tyco, Tyco> _instantiations;
    };
    class NullaryBody : public Body
    {
    public:
        NullaryBody(const Type& returnType) : _returnType(returnType) { }
        String toString2(bool* needComma)
        {
            return _returnType.toString() + "(";
        }
        bool equals(HandleBase::Body* other)
        {
            auto o = other->to<NullaryBody>();
            return o != 0 && _returnType != o->_returnType;
        }
        Hash hash() { return Body::hash().mixin(_returnType.hash()); }
        bool argumentsMatch(List<Type>::Iterator* i) { return true; }
        virtual void addParameterTycos(List<Tyco>* list) { }
        Type returnType() { return _returnType; }
        CodeWalker::Result walk(CodeWalker* walker)
        {
            auto r = walker->visit(tyco());
            if (r == CodeWalker::Result::recurse) {
                if (_returnType.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
            }
            return r;
        }
    private:
        Type _returnType;
    };
    class ArgumentBody : public Body
    {
    public:
        ArgumentBody(FunctionType parent, const Type& argumentType)
          : _parent(parent), _argumentType(argumentType) { }
        String toString2(bool* needComma)
        {
            String s = _parent.toString2(needComma);
            if (*needComma)
                s += ", ";
            *needComma = true;
            return s + _argumentType.toString();
        }
        bool equals(HandleBase::Body* other)
        {
            auto o = other->to<ArgumentBody>();
            return o != 0 && _parent == o->_parent &&
                _argumentType == o->_argumentType;
        }
        Hash hash()
        {
            return Body::hash().mixin(_parent.hash()).
                mixin(_argumentType.hash());
        }
        bool argumentsMatch(List<Type>::Iterator* i)
        {
            if (!_parent.body()->argumentsMatch(i))
                return false;
            if (i->end())
                return false;
            if (!(*i)->canConvertTo(_argumentType))
                return false;
            ++*i;
            return true;
        }
        virtual void addParameterTycos(List<Tyco>* list)
        {
            _parent.addParameterTycos(list);
            list->add(_argumentType);
        }
        Type returnType() { return _parent.returnType(); }
        CodeWalker::Result walk(CodeWalker* walker)
        {
            auto r = walker->visit(tyco());
            if (r == CodeWalker::Result::recurse) {
                if (_parent.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
                if (_argumentType.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
            }
            return r;
        }
    private:
        FunctionTypeT<T> _parent;
        Type _argumentType;
    };
    Body* body() const { return as<Body>(); }
    String toString2(bool* needComma) const
    {
        return body()->toString2(needComma);
    }
    FunctionTypeT(const Handle& t) : Tyco(t) { }
};

template<class T> class FunctionTemplateT
  : public NamedNullary<Template, FunctionTemplate>
{
public:
    static String name() { return "Function"; }

    class Body : public NamedNullary::Body
    {
    public:
        virtual Tyco partialInstantiate(bool final, Tyco argument)
        {
            return FunctionType::nullary(argument);
        }
        Kind kind()
        {
            return TemplateKind(TypeKind(), VariadicTemplateKind());
        }
        Type finalInstantiate(Template parent, Tyco argument)
        {
            assert(false);
            return Type();
        }
    };
};

template<class T = int> class EnumerationType : public Type
{
public:
    class Helper;
    EnumerationType(String name, const Helper& helper, String context = "")
      : Type(create<Body>(name, helper, context)) { }
protected:
    class Body : public Type::Body
    {
    public:
        Body(String name, const Helper& helper, String context)
          : _name(name), _helper(helper), _context(context) { }
        String toString() { return _name; }
        String serialize(void* p, int width, int used, int indent, int delta)
        {
            return _context + _name + "." +
                _helper._tToString[*static_cast<T*>(p) + 1];
        }
        void deserialize(const Value& value, void* p)
        {
            *static_cast<T*>(p) = value.value<T>();
        }
        int size() { return sizeof(T); }
        Value defaultValue() { return Value(type(), static_cast<T>(0)); }
        Value value(void* p)
        {
            return Value(type(), *static_cast<T*>(p));
        }
        bool equals(HandleBase::Body* other)
        {
            auto o = other->to<Body>();
            return o != 0 && _context == o->_context && _name == o->_name &&
                _helper == o->_helper;
        }
    private:
        String _context;
        String _name;
        const Helper _helper;
    };
public:
    class Helper
    {
    public:
        void add(const T& t, String i)
        {
            _stringToT.add(i, t);
            _tToString.add(static_cast<int>(t) + 1, i);
        }
        bool operator==(const Helper& other) const
        {
            return _stringToT == other._stringToT &&
                _tToString == other._tToString;
        }
    private:
        HashTable<String, T> _stringToT;
        HashTable<int, String> _tToString;
        friend class Body;
    };
};

// StructuredType is the type of "{...}" literals, not the base type for all
// types which have members. The ALFE compiler will need a more complicated
// body of structures, including using the same conversions at
// compile-time as at run-time. Also we don't want to have to override
// conversion functions in children just to avoid unwanted conversions

template<class T> class StructuredTypeT : public Type
{
public:
    class Member
    {
    public:
        Member() { }
        Member(String name, Type type) : _name(name), _default(type) { }
        Member(String name, Value defaultValue)
          : _name(name), _default(defaultValue) { }
        template<class U> Member(String name, const U& defaultValue)
          : _name(name), _default(defaultValue) { }
        String name() const { return _name; }
        Type type() const { return _default.type(); }
        Value defaultValue() const { return _default; }
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
        Value _default;
    };

    template<class MemberT> static Member member(String name)
    {
        return Member(name, typeFromCompileTimeType<MemberT>());
    }

    StructuredTypeT() { }
    StructuredTypeT(const Tyco& tyco) : Type(to<Body>(tyco)) { }
    StructuredTypeT(const Handle& other) : Type(other) { }
    StructuredTypeT(String name, List<Member> members)
      : Type(create<Body>(name, members)) { }
    const HashTable<Identifier, int> names() { return body()->names(); }
    const Array<Member> members() { return body()->members(); }
    static Value empty()
    {
        static Value e = Value(StructuredType(String(),
            List<StructuredType::Member>()), HashTable<Identifier, Value>());
        return e;
    }
    // This is a bit of a hack. In order to allow access to members (e.g. "x"
    // and "y" of a Vector) from a ConfigFile, we need to convert the Vector to
    // a Structure, and that Structure needs to be owned by something. Hence
    // the ConfigFile needs to have a StructureOwner to own those Structure
    // objects.
    Value lValueFromRValue(Any rValue, StructureOwner* owner)
    {
        return body()->lValueFromRValue(rValue, owner);
    }
    Any rValueFromLValue(Value lValue)
    {
        return body()->rValueFromLValue(lValue);
    }
    Value constructValue(Value value)
    {
        return body()->constructValue(value);
    }
    void setLValue(LValue l, Value rValue)
    {
        return body()->setLValue(l, rValue);
    }
    Type member(Identifier i) { return body()->member(i); }
    //Scope* scope() { return body()->scope(); }
protected:
    class Body : public Type::Body
    {
    public:
        Body(String name, List<Member> members)
          : _name(name), _members(members)
        {
            int n = 0;
            for (auto i : members) {
                String name = i.name();B
                _names.add(name, n);
                Identifier identifier(name);
                _scope.addObject(identifier,
                    VariableDefinition(i.type(), identifier));
                ++n;
            }
        }
        String toString() { return _name; }
        const HashTable<Identifier, int> names() { return _names; }
        const Array<Member> members() { return _members; }

        bool canConvertTo(const Type& to, String* why)
        {
            StructuredType s(to);
            if (s.valid()) {
                // First take all named members in the RHS and assign them to
                // the corresponding named members in the LHS.
                int count = _members.count();
                int toCount = s.members().count();
                Array<bool> assigned(toCount);
                for (int i = 0; i < toCount; ++i)
                    assigned[i] = false;
                for (int i = 0; i < count; ++i) {
                    const Member* m = &_members[i];
                    String name = m->name();
                    if (name.empty())
                        continue;
                    // If a member doesn't exist, fail conversion.
                    if (!s.names().hasKey(name)) {
                        *why = "The target type has no member named " + name;
                        return false;
                    }
                    int j = s.names()[name];
                    if (assigned[j]) {
                        *why =
                            "The source type has more than one member named " +
                            name;
                        return false;
                    }
                    // If one of the child conversions fails, fail.
                    if (!canConvertHelper(member(name), &s.members()[j],
                        why))
                        return false;
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
                    while (assigned[j] && j < toCount)
                        ++j;
                    if (j >= toCount) {
                        *why = "The source type has too many members";
                        return false;
                    }
                    const Member* toMember = &s.members()[j];
                    ++j;
                    if (!canConvertHelper(m->type(), toMember, why))
                        return false;
                }
                // Make sure any unassigned members have defaults.
                for (;j < toCount; ++j) {
                    if (assigned[j])
                        continue;
                    const Member* toMember = &s.members()[j];
                    if (!toMember->hasDefault()) {
                        *why = "No default value is available for target type "
                            "member " + toMember->name();
                        return false;
                    }
                }
                return true;
            }
            ArrayType toArray = to;
            if (toArray.valid()) {
                Type contained = toArray.contained();
                LessThanType l(toArray.indexer());
                if (l.valid() && _members.count() > l.n()) {
                    *why = "The source type has too many members";
                    return false;
                }
                for (int i = 0; i < _members.count(); ++i) {
                    if (!_members[i].name().empty()) {
                        *why = "Array cannot be initialized with a structured "
                            "value containing named members";
                        return false;
                    }
                    String reason;
                    if (!_members[i].type().canConvertTo(contained, &reason)) {
                        *why = "Cannot convert child member " + decimal(i);
                        if (!reason.empty())
                            *why += ": " + reason;
                        return false;
                    }
                }
                return true;
            }
            TupleTyco toTuple = to;
            if (toTuple.valid()) {
                int count = _members.count();
                for (int i = _members.count() - 1; i >= 0; --i) {
                    if (!_members[i].name().empty()) {
                        *why = "Tuple cannot be initialized with a structured "
                            "value containing named members";
                        return false;
                    }
                    if (toTuple.isUnit()) {
                        *why = "Tuple type does not have enough members to be "
                            "initialized with this structured value.";
                        return false;
                    }
                    String reason;
                    if (!_members[i].type().
                        canConvertTo(toTuple.lastMember(), &reason)) {
                        *why = "Cannot convert child member " + decimal(i);
                        if (!reason.empty())
                            *why += ": " + reason;
                        return false;
                    }
                    toTuple = toTuple.firstMembers();
                }
                return true;
            }
            return false;
        }
        Value convertTo(const Type& to, const Value& value)
        {
            StructuredType s(to);
            if (s.valid()) {
                auto input = value.value<HashTable<Identifier, Value>>();
                HashTable<Identifier, Value> output;

                // First take all named members in the RHS and assign them to
                // the corresponding named members in the LHS.
                int count = _members.count();
                int toCount = s.members().count();
                Array<bool> assigned(toCount);
                for (int i = 0; i < toCount; ++i)
                    assigned[i] = false;
                for (int i = 0; i < count; ++i) {
                    const Member* m = &_members[i];
                    String name = m->name();
                    if (name.empty())
                        continue;
                    int j = s.names()[name];
                    output[name] =
                        input[name].convertTo(s.members()[j].type());
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
                    while (assigned[j] && j < toCount)
                        ++j;
                    const Member* toMember = &s.members()[j];
                    ++j;
                    output[toMember->name()] =
                        input[Identifier(String::Decimal(i))].
                        convertTo(toMember->type());
                }
                // Make sure any unassigned members have defaults.
                for (; j < toCount; ++j) {
                    if (assigned[j])
                        continue;
                    const Member* toMember = &s.members()[j];
                    output[toMember->name()] = toMember->defaultValue();
                }
                return Value(type(), output, value.span());
            }
            ArrayType toArray = to;
            if (toArray.valid()) {
                Type contained = toArray.contained();
                auto input = value.value<HashTable<Identifier, Value>>();
                List<Value> results;
                int i;
                for (i = 0; i < input.count(); ++i) {
                    String name = decimal(i);
                    Value v = input[name].convertTo(contained);
                    results.add(v);
                }
                LessThanType l(toArray.indexer());
                if (l.valid()) {
                    for (; i < l.n(); ++i)
                        results.add(contained.defaultValue());
                }
                return Value(to, results, value.span());
            }
            TupleTyco toTuple = to;
            if (toTuple.valid()) {
                auto input = value.value<HashTable<Identifier, Value>>();
                List<Value> results;
                int count = _members.count();
                for (int i = input.count() - 1; i >= 0; --i) {
                    String name = decimal(i);
                    Value v = input[name].convertTo(toTuple.lastMember());
                    results.add(v);
                    toTuple = toTuple.firstMembers();
                }
                return Value(to, results, value.span());
            }
            assert(false);
            return Value();
        }
        Value defaultValue()
        {
            HashTable<Identifier, Value> values;
            for (auto i : _members)
                values.add(i.name(), i.defaultValue());
            return Value(type(), values);
        }

        Type member(IdentifierT<T> i)
        {
            if (!_names.hasKey(i))
                return Type();
            return _members[_names[i]].type();
        }
        bool equals(HandleBase::Body* other)
        {
            auto o = other->to<Body>();
            if (o == 0)
                return false;
            return _name == o->_name && _names == o->_names &&
                _members == o->_members;
        }
        virtual Value lValueFromRValue(Any rValue, StructureOwner* owner)
        {
            return Value(type(), rValue);
        }
        virtual Any rValueFromLValue(Value lValue)
        {
            return lValue.value();
        }
        virtual Value constructValue(Value value) { return value; }
        void setLValue(LValue l, Value rValue) { }
        //Scope* scope() { return &_scope; }
    private:
        bool canConvertHelper(Type type, const Member* to, String* why)
        {
            String reason;
            if (!type.canConvertTo(to->type(), &reason)) {
                *why = "Cannot convert child member " + to->name();
                if (!reason.empty())
                    *why += ": " + reason;
                return false;
            }
            return true;
        }

        String _name;
        HashTable<Identifier, int> _names;
        Array<Member> _members;
        //Scope _scope;
    };
    Body* body() { return as<Body>(); }
    const Body* body() const { return as<Body>(); }

    friend class Body;
};

template<class T> class Vector2Type
  : public NamedNullary<StructuredType, Vector2Type<T>>
{
public:
    class Body : public StructuredType::Body
    {
    public:
        Body() : StructuredType::Body("Vector2<" +
            typeFromCompileTimeType<T>().toString() + ">", members()) { }
        Value lValueFromRValue(Any rValue, StructureOwner* owner)
        {
            auto r = Reference<Structure>::create<Structure>();
            owner->addOwned(r);
            auto v = rValue.value<Vector2<T>>();
            r->set("x", v.x, Span());
            r->set("y", v.y, Span());
            return Value(LValueType::wrap(type()), &*r);
        }
        void setLValue(LValue l, Value rValue)
        {
            auto v = rValue.value<Vector2<T>>();
            l.member("x").set(v.x, Span());
            l.member("y").set(v.y, Span());
        }
        Any rValueFromLValue(Value lValue)
        {
            auto s = lValue.value<Structure*>();
            return Vector2<T>(s->get<T>("x"), s->get<T>("y"));
        }
        virtual Value constructValue(Value value)
        {
            auto s = value.value<List<Any>>();
            auto i = s.begin();
            Vector2<T> v;
            v.x = i->value<T>();
            ++i;
            v.y = i->value<T>();
            return Value(v, value.span());
        }
    private:
        List<StructuredType::Member> members()
        {
            List<StructuredType::Member> vectorMembers;
            vectorMembers.add(StructuredType::member<T>("x"));
            vectorMembers.add(StructuredType::member<T>("y"));
            return vectorMembers;
        }
    };
    friend class NamedNullary<StructuredType, Vector2Type<T>>;
};

typedef Vector2Type<int> VectorType;

template<> Type typeFromCompileTimeType<int>() { return IntegerType(); }
template<> Type typeFromCompileTimeType<String>() { return StringType(); }
template<> Type typeFromCompileTimeType<bool>() { return BooleanType(); }
// If C++ allowed partial specialization of function templates, we could do
// this:
//template<class T> Type typeFromCompileTimeType<Vector2<T>>()
//{
//    return Vector2Type<T>();
//}
// But it doesn't, so we have to enumerate each type of Vector2 we want here:
template<> Type typeFromCompileTimeType<Vector>() { return VectorType(); }
template<> Type typeFromCompileTimeType<Vector2<double>>()
{
    return Vector2Type<double>();
}
template<> Type typeFromCompileTimeType<Rational>() { return RationalType(); }
template<> Type typeFromCompileTimeType<double>() { return DoubleType(); }
template<> Type typeFromCompileTimeType<Byte>() { return ByteType(); }
template<> Type typeFromCompileTimeType<Word>() { return WordType(); }
template<> Type typeFromCompileTimeType<DWord>() { return DWordType(); }

#endif // INCLUDED_TYPE_H
