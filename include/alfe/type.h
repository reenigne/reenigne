#include "alfe/main.h"

#ifndef INCLUDED_TYPE_H
#define INCLUDED_TYPE_H

#include "alfe/any.h"
#include "alfe/hash_table.h"
#include "alfe/value.h"
#include "alfe/nullary.h"
#include "alfe/kind.h"
#include "alfe/assert.h"
#include "alfe/function.h"

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

template<class T> class TycoTemplate
{
public:
    TycoTemplate() { }
    String toString() const { return _implementation->toString(); }
    bool valid() const { return _implementation.valid(); }
    bool operator==(const Tyco& other) const
    {
        if (_implementation == other._implementation)
            return true;
        return _implementation->equals(other._implementation);
    }
    bool operator!=(const Tyco& other) const { return !operator==(other); }
    int hash() const { return _implementation->hash(); }
    Kind kind() const { return _implementation->kind(); }
    template<class K> bool is() const
    {
        return _implementation.template referent<K::Implementation>() != 0;
    }
    template<class K> K as() const
    {
        return K(_implementation.template referent<K::Implementation>());
    }
protected:
    class Implementation : public ReferenceCounted
    {
    public:
        // Tyco
        virtual String toString() const = 0;
        virtual bool equals(const Implementation* other) const
        {
            return this == other;
        }
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
    TycoTemplate(const Implementation* implementation)
      : _implementation(implementation) { }
    ConstReference<Implementation> _implementation;

    friend class TemplateTemplate<void>;
    friend class EnumerationType;
    template<class U> friend class StructuredTypeTemplate;
public:
    const Implementation* implementation() const
    {
        return _implementation.template referent<Implementation>();
    }
};

template<class T> class TypeTemplate : public Tyco
{
public:
    TypeTemplate() { }
    TypeTemplate(const Tyco& tyco) : Tyco(tyco) { }

    TypedValueTemplate<T> tryConvert(const TypedValue& value, String* reason)
        const
    {
        return _implementation->tryConvert(value, reason);
    }
    TypedValueTemplate<T> tryConvertTo(const Type& to, const TypedValue& value,
        String* reason) const
    {
        return _implementation->tryConvertTo(to, value, reason);
    }
    bool has(IdentifierTemplate<T> memberName) const
    {
        return _implementation->has(memberName);
    }
    TypeTemplate(const Implementation* implementation)
      : Tyco(implementation) { }
    const Implementation* implementation() const
    {
        return _implementation.referent<Implementation>();
    }
protected:
    class Implementation : public Tyco::Implementation
    {
    public:
        Kind kind() const { return TypeKind(); }
        TypedValueTemplate<T> tryConvert(const TypedValueTemplate<T>& value,
            String* reason) const
        {
            if (this == value.type().implementation())
                return value;
            return TypedValueTemplate<T>();
        }
        TypedValueTemplate<T> tryConvertTo(const Type& to,
            const TypedValue& value, String* reason) const
        {
            if (this == to.implementation())
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

class StringType : public Nullary<Type, StringType>
{
 public:
    static String name() { return "String"; }
};

template<> Nullary<Type, StringType> Nullary<Type, StringType>::_instance;

class IntegerType : public Nullary<Type, IntegerType>
{
 public:
    static String name() { return "Integer"; }
};

template<> Nullary<Type, IntegerType>  Nullary<Type, IntegerType>::_instance;

class BooleanType : public  Nullary<Type, BooleanType>
{
 public:
    static String name() { return "Boolean"; }
};

template<> Nullary<Type, BooleanType> Nullary<Type, BooleanType>::_instance;

class ObjectType : public Nullary<Type, ObjectType>
{
 public:
    static String name() { return "Object"; }
};

template<> Nullary<Type, ObjectType> Nullary<Type, ObjectType>::_instance;

class LabelType : public Nullary<Type, LabelType>
{
 public:
    static String name() { return "Label"; }
};

template<> Nullary<Type, LabelType> Nullary<Type, LabelType>::_instance;

class VoidType : public Nullary<Type, VoidType>
{
 public:
    static String name() { return "Void"; }
};

template<> Nullary<Type, VoidType> Nullary<Type, VoidType>::_instance;

template<class T> Type typeFromCompileTimeType()
{
    throw Exception("Don't know this type.");
}
template<> Type typeFromCompileTimeType<int>() { return IntegerType(); }
template<> Type typeFromCompileTimeType<String>() { return StringType(); }
template<> Type typeFromCompileTimeType<bool>() { return BooleanType(); }

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
        return _implementation->instantiate(argument);
    }
protected:
    class Implementation : public Tyco::Implementation
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
            return new PartialImplementation(this, this, argument);
        }
        virtual Type finalInstantiate(const Implementation* parent, Tyco
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
    class PartialImplementation : public Implementation
    {
    public:
        PartialImplementation(const Implementation* root,
            const Implementation* parent, Tyco argument)
          : _root(root), _parent(parent), _argument(argument) { }

        String toString() const
        {
            return _argument.toString() + "<" + toString2() + ">";
        }
        String toString2() const
        {
            auto p = dynamic_cast<const PartialImplementation*>(_parent);
            String s;
            if (p != 0)
                s = p->toString2() + ", ";
            return s + _argument.toString();
        }
        Kind kind() const
        {
            return _parent->kind().instantiate(_argument.kind());
        }
        Type finalInstantiate(const Implementation* parent, Tyco argument)
            const
        {
            assert(false);
            return Type();
        }

        Tyco partialInstantiate(bool final, Tyco argument) const
        {
            if (final)
                return _root->finalInstantiate(this, argument);
            return new PartialImplementation(_root, this, argument);
        }
        bool equals(const Tyco::Implementation* other) const
        {
            const PartialImplementation* o =
                dynamic_cast<const PartialImplementation*>(other);
            return o != 0 && _parent->equals(o->_parent) &&
                _argument == o->_argument;
        }
        int hash() const
        {
            return (_parent->hash()*67 + 5)*67 + _argument.hash();
        }
        const Implementation* parent() const { return _parent; }
        Tyco argument() const { return _argument; }
    private:
        const Implementation* _root;
        const Implementation* _parent;
        Tyco _argument;
    };
    TemplateTemplate(const Implementation* implementation)
      : Tyco(implementation) { }
};

class ArrayType : public Type
{
public:
    ArrayType(const Type& contained, const Type& indexer)
      : Type(new Implementation(contained, indexer)) { }
    ArrayType(const Implementation* implementation) : Type(implementation) { }
    Type contained() const { return implementation()->contained(); }
    Type indexer() const { return implementation()->indexer(); }

    class Implementation : public Type::Implementation
    {
    public:
        Implementation(const Type &contained, const Type& indexer)
          : _contained(contained), _indexer(indexer) { }
        String toString() const
        {
            return _contained.toString() + "[" + _indexer.toString() + "]";
        }
        bool equals(const Tyco::Implementation* other) const
        {
            const Implementation* o =
                dynamic_cast<const Implementation*>(other);
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
    const Implementation* implementation() const
    {
        return _implementation.referent<Implementation>();
    }
};

class ArrayTemplate : public Nullary<Template, ArrayTemplate>
{
public:
    static String name() { return "Array"; }

    class Implementation : public Nullary::Implementation
    {
    public:
        Kind kind() const
        {
            return TemplateKind(TypeKind(),
                TemplateKind(TypeKind(), TypeKind()));
        }
        Type finalInstantiate(const Template::Implementation* parent,
            Tyco argument) const
        {
            return ArrayType(
                dynamic_cast<const Template::PartialImplementation*>(parent)->
                    argument(), argument);
        }
    };
};

template<> Nullary<Template, ArrayTemplate>
    Nullary<Template, ArrayTemplate>::_instance;

class SequenceType : public Type
{
public:
    SequenceType(const Type& contained)
      : Type(new Implementation(contained)) { }
    Type contained() const { return implementation()->contained(); }
private:
    class Implementation : public Type::Implementation
    {
    public:
        Implementation(const Type &contained) : _contained(contained) { }
        String toString() const
        {
            return _contained.toString() + "[]";
        }
        bool equals(const Type::Implementation* other) const
        {
            const Implementation* o =
                dynamic_cast<const Implementation*>(other);
            if (o == 0)
                return false;
            return _contained == o->_contained;
        }
        int hash() const { return _contained.hash()*67 + 4; }
        Type contained() const { return _contained; }
    private:
        Type _contained;
    };
    const Implementation* implementation() const
    {
        return _implementation.referent<Implementation>();
    }
};

class SequenceTemplate : public Nullary<Template, SequenceTemplate>
{
public:
    static String name() { return "Sequence"; }

    class Implementation : public Nullary::Implementation
    {
    public:
        Kind kind() const { return TemplateKind(TypeKind(), TypeKind()); }
        Type finalInstantiate(const Template::Implementation* parent,
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

template<class T> class TupleTycoTemplate : public Tyco
{
public:
    static String name() { return "Tuple"; }
    TupleTycoTemplate() : Tyco(instance()) { }
    bool isUnit() { return implementation() == 0; }
    Tyco instantiate(const Tyco& argument) const
    {
        return _implementation->instantiate(argument);
    }
    Type lastMember()
    {
        const NonUnitImplementation* i = implementation();
        if (i == 0)
            return Type();
        return i->contained();
    }
    TupleTyco firstMembers()
    {
        const NonUnitImplementation* i = implementation();
        if (i == 0)
            return TupleTyco();
        return i->parent();
    }
    class Implementation : public Tyco::Implementation
    {
    public:
        // Tyco
        String toString() const
        {
            bool needComma = false;
            return "(" + toString2(&needComma) + ")";
        }
        virtual String toString2(bool* needComma) const { return ""; }
        bool equals(const Implementation* other) const
        {
            return this == other;
        }
        Kind kind() const { return VariadicTemplateKind(); }
        int hash() const { return reinterpret_cast<intptr_t>(this); }

        // Type
        TypedValue tryConvert(const TypedValue& value, String* reason) const
        {
            if (this == value.type().implementation())
                return value;
            return TypedValue();
        }
        TypedValue tryConvertTo(const Type& to, const TypedValue& value,
            String* reason) const
        {
            if (this == to.implementation())
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

            TupleTyco t(new NonUnitImplementation(this, argument));
            _instantiations.add(argument, t);
            return t;
        }
    private:
        mutable HashTable<Tyco, Tyco> _instantiations;
    };
    TupleTycoTemplate(const Implementation* implementation)
      : Tyco(implementation) { }
private:

    class NonUnitImplementation : public Implementation
    {
    public:
        NonUnitImplementation(const Implementation* parent, Type contained)
          : _parent(parent), _contained(contained) { }
        String toString2(bool* needComma) const
        {
            String s = _parent->toString2(needComma);
            if (*needComma)
                s += ", ";
            *needComma = true;
            return s + _contained.toString();
        }
        bool equals(const Implementation* other) const
        {
            const NonUnitImplementation* o =
                dynamic_cast<const NonUnitImplementation*>(other);
            return _parent->equals(o->_parent) && _contained == o->_contained;
        }
        int hash() const
        {
            return (_parent->hash()*67 + 6)*67 + _contained.hash();
        }

        // Type
        TypedValue tryConvert(const TypedValue& value, String* reason) const
        {
            if (_parent == TupleTyco().implementation())
                return _contained.tryConvert(value, reason);
            if (this == value.type().implementation())
                return value;
            return TypedValue();
        }
        TypedValue tryConvertTo(const Type& to, const TypedValue& value,
            String* reason) const
        {
            if (_parent == TupleTyco().implementation())
                return _contained.tryConvertTo(to, value, reason);
            if (this == to.implementation())
                return value;
            return TypedValue();
        }
        bool has(IdentifierTemplate<T> memberName) const
        {
            CharacterSource s(memberName.name());
            int n;
            if (!Space::parseInteger(&s, &n))
                return false;
            if (s.get() != -1)
                return false;
            const NonUnitImplementation* p = this;
            do {
                if (n == 1)
                    return true;
                --n;
                p = dynamic_cast<const NonUnitImplementation*>(p->_parent);
                if (p == 0)
                    return false;
            } while (true);
        }
        const Implementation* parent() const { return _parent; }
        Type contained() const { return _contained; }
    private:
        const Implementation* _parent;
        Type _contained;
    };
private:
    static TupleTyco _instance;
    static TupleTyco instance()
    {
        if (!_instance.valid())
            _instance = new Implementation();
        return _instance;
    }

    const NonUnitImplementation* implementation() const
    {
        return _implementation.referent<NonUnitImplementation>();
    }
};

template<> TupleTyco TupleTyco::_instance;

class PointerType : public Type
{
public:
    PointerType(const Type& referent) : Type(new Implementation(referent)) { }
private:
    class Implementation : public Type::Implementation
    {
    public:
        Implementation(const Type &referent) : _referent(referent) { }
        String toString() const { return _referent.toString() + "*"; }
        bool equals(const Type::Implementation* other) const
        {
            const Implementation* o =
                dynamic_cast<const Implementation*>(other);
            if (o == 0)
                return false;
            return _referent == o->_referent;
        }
        int hash() const { return _referent.hash()*67 + 1; }
    private:
        Type _referent;
    };
};

class PointerTemplate : public Nullary<Template, PointerTemplate>
{
public:
    static String name() { return "Pointer"; }

    class Implementation : public Nullary::Implementation
    {
    public:
        Kind kind() const { return TemplateKind(TypeKind(), TypeKind()); }
        Type finalInstantiate(const Template::Implementation* parent,
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
        return FunctionTyco(new NullaryImplementation(returnType));
    }
    FunctionTycoTemplate(Type returnType, Type argumentType)
      : Tyco(FunctionTyco(FunctionTemplateTemplate<T>().
            instantiate(returnType)).
            instantiate(argumentType).implementation()) { }
    FunctionTycoTemplate(Type returnType, Type argumentType1,
        Type argumentType2)
      : Tyco(FunctionTyco(FunctionTyco(FunctionTemplateTemplate<T>().
            instantiate(returnType)).instantiate(argumentType1)).
            instantiate(argumentType2).implementation()) { }
    bool valid() const { return implementation() != 0; }
    bool matches(List<Type> argumentTypes)
    {
        List<Type>::Iterator i = argumentTypes.begin();
        if (!implementation()->matches(&i))
            return false;
        return i == argumentTypes.end();
    }
    Tyco instantiate(const Tyco& argument) const
    {
        return _implementation->instantiate(argument);
    }
private:
    FunctionTycoTemplate(const Implementation* implementation)
      : Tyco(implementation) { }
    class Implementation : public Tyco::Implementation
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
            if (this == value.type().implementation())
                return value;
            return TypedValue();
        }
        TypedValue tryConvertTo(const Type& to, const TypedValue& value,
            String* reason) const
        {
            if (this == to.implementation())
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

            FunctionTyco t(new ArgumentImplementation(this, argument));
            _instantiations.add(argument, t);
            return t;
        }
        virtual bool matches(List<Type>::Iterator* i) const = 0;
    private:
        mutable HashTable<Tyco, Tyco> _instantiations;
    };
    class NullaryImplementation : public Implementation
    {
    public:
        NullaryImplementation(const Type& returnType)
          : _returnType(returnType) { }
        String toString2(bool* needComma) const
        {
            return _returnType.toString() + "(";
        }
        bool equals(const Tyco::Implementation* other) const
        {
            const NullaryImplementation* o =
                dynamic_cast<const NullaryImplementation*>(other);
            if (o == 0)
                return false;
            return (_returnType != o->_returnType);
        }
        int hash() const { return _returnType.hash()*67 + 2; }
        bool matches(List<Type>::Iterator* i) const
        {
            return true;
        }
    private:
        Type _returnType;
    };
    class ArgumentImplementation : public Implementation
    {
    public:
        ArgumentImplementation(const Implementation* parent,
            const Type& argumentType)
          : _parent(parent), _argumentType(argumentType) { }
        String toString2(bool* needComma) const
        {
            String s = _parent->toString2(needComma);
            if (*needComma)
                s += ", ";
            *needComma = true;
            return s + _argumentType.toString();
        }
        bool equals(const Tyco::Implementation* other) const
        {
            const ArgumentImplementation* o =
                dynamic_cast<const ArgumentImplementation*>(other);
            if (o == 0)
                return false;
            return _parent->equals(o->_parent) &&
                _argumentType == o->_argumentType;
        }
        int hash() const { return _parent->hash()*67 + _argumentType.hash(); }
        bool matches(List<Type>::Iterator* i)
            const
        {
            if (!_parent->matches(i))
                return false;
            if (**i != _argumentType)
                return false;
            ++*i;
            return true;
        }
    private:
        const Implementation* _parent;
        Type _argumentType;
    };
    const Implementation* implementation() const
    {
        return _implementation.referent<Implementation>();
    }
};

template<class T> class FunctionTemplateTemplate
  : public Nullary<Template, FunctionTemplate>
{
public:
    static String name() { return "Pointer"; }

    class Implementation : public Nullary::Implementation
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
        Type finalInstantiate(const Template::Implementation* parent,
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

    EnumerationType(const Type& other)
      : Type(other._implementation.referent<Implementation>()) { }
    EnumerationType(String name, List<Value> values)
      : Type(new Implementation(name, values)) { }
    const Array<Value>* values() const
    {
        return _implementation.referent<Implementation>()->values();
    }
private:
    class Implementation : public Type::Implementation
    {
    public:
        Implementation(String name, List<Value> values)
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
    LessThanType(int n) : Type(new Implementation(n)) { }
private:
    class Implementation : public Type::Implementation
    {
    public:
        Implementation(int n) : _n(n) { }
        String toString() const { return decimal(_n); }

        bool equals(const Type::Implementation* other) const
        {
            const Implementation* o =
                dynamic_cast<const Implementation*>(other);
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
// implementation of structures, including using the same conversions at
// compile-time as at run-time. Also we don't want to have to override
// conversion functions in children just to avoid unwanted conversions

template<class T> class StructuredTypeTemplate;
typedef StructuredTypeTemplate<void> StructuredType;

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

    StructuredTypeTemplate() { }
    StructuredTypeTemplate(const Type& other)
      : Type(other._implementation.referent<Implementation>()) { }
    StructuredTypeTemplate(String name, List<Member> members)
      : Type(new Implementation(name, members)) { }
    const HashTable<Identifier, int>* names() const
    {
        return implementation()->names();
    }
    const Array<Member>* members() const
    {
        return implementation()->members();
    }
    static TypedValue empty()
    {
        return TypedValue(StructuredType(String(),
            List<StructuredType::Member>()),
            Value<HashTable<Identifier, TypedValue>>());
    }
private:
    class Implementation : public Type::Implementation
    {
    public:
        Implementation(String name, List<Member> members)
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
            const Implementation* toImplementation =
                to._implementation.referent<Implementation>();
            if (toImplementation != 0) {
                auto input =
                    value.value<Value<HashTable<Identifier, TypedValue>>>();
                Value<HashTable<Identifier, TypedValue>> output;

                // First take all named members in the RHS and assign them to
                // the corresponding named members in the LHS.
                int count = _members.count();
                int toCount = toImplementation->_members.count();
                Array<bool> assigned(toCount);
                for (int i = 0; i < toCount; ++i)
                    assigned[i] = false;
                for (int i = 0; i < count; ++i) {
                    const Member* m = &_members[i];
                    String name = m->name();
                    if (name.empty())
                        continue;
                    // If a member doesn't exist, fail conversion.
                    if (!toImplementation->_names.hasKey(name)) {
                        *why = String("The target type has no member named ") +
                            name;
                        return TypedValue();
                    }
                    int j = toImplementation->_names[name];
                    if (assigned[j]) {
                        *why = String("The source type has more than one "
                            "member named ") + name;
                        return TypedValue();
                    }
                    // If one of the child conversions fails, fail.
                    TypedValue v = tryConvertHelper((*input)[name],
                        &toImplementation->_members[j], why);
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
                    const Member* toMember = &toImplementation->_members[j];
                    ++j;
                    TypedValue v =
                        tryConvertHelper((*input)[String::Decimal(i)],
                        toMember, why);
                    if (!v.valid())
                        return TypedValue();
                    (*output)[toMember->name()] = v;
                }
                // Make sure any unassigned members have defaults.
                for (;j < toCount; ++j) {
                    if (assigned[j])
                        continue;
                    const Member* toMember = &toImplementation->_members[j];
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
            ArrayType toArray = to.as<ArrayType>();
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
            TupleTyco toTuple = to.as<TupleTyco>();
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
    const Implementation* implementation() const
    {
        return _implementation.referent<Implementation>();
    }

    friend class Implementation;
};

#include "alfe/identifier.h"

#endif // INCLUDED_TYPE_H
