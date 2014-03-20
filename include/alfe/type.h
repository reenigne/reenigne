#include "alfe/main.h"

#ifndef INCLUDED_TYPE_H
#define INCLUDED_TYPE_H

#include "alfe/any.h"
#include "alfe/hash_table.h"
#include "alfe/value.h"
#include "alfe/nullary.h"
#include "alfe/kind.h"

template<class T> class TemplateTemplate;
typedef TemplateTemplate<void> Template;

template<class T> class TypeTemplate;
typedef TypeTemplate<void> Type;

template<class T> class TypedValueTemplate;
typedef TypedValueTemplate<void> TypedValue;

template<class T> class TycoTemplate;
typedef TycoTemplate<void> Tyco;

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
        virtual bool has(String memberName) const = 0;

        // Template
        virtual Tyco instantiate(const Tyco& argument) const = 0;
    };
    Tyco(const Implementation* implementation)
      : _implementation(implementation) { }
    ConstReference<Implementation> _implementation;

    friend class TemplateTemplate<void>;
    friend class EnumerationType;
    friend class StructuredType;
};

template<class T> class TypeTemplate : public Tyco
{
public:
    TypeTemplate() { }
    TypeTemplate(const Tyco& tyco) : Tyco(tyco) { }

    TypedValueTemplate<T> tryConvert(const TypedValue& value, String* reason)
        const
    {
        return implementation()->tryConvert(value, reason);
    }
    TypedValueTemplate<T> tryConvertTo(const Type& to, const TypedValue& value,
        String* reason) const
    {
        return implementation()->tryConvertTo(to, value, reason);
    }
    bool has(String memberName) const
    {
        return implementation()->has(memberName);
    }
    TypeTemplate(const Implementation* implementation)
      : Tyco(implementation) { }
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
        virtual bool has(String memberName) const { return false; }
    };
    const Implementation* implementation() const
    {
        return _implementation.referent<Implementation>();
    }

    friend class TemplateTemplate<void>;
};

class StringType : public Nullary<Type, StringType>
{
public:
    static String name() { return "String"; }
};

Nullary<Type, StringType> Nullary<Type, StringType>::_instance;

class IntegerType : public Nullary<Type, IntegerType>
{
public:
    static String name() { return "Integer"; }
};

 Nullary<Type, IntegerType>  Nullary<Type, IntegerType>::_instance;

class BooleanType : public  Nullary<Type, BooleanType>
{
public:
    static String name() { return "Boolean"; }
};

Nullary<Type, BooleanType> Nullary<Type, BooleanType>::_instance;

class ObjectType : public Nullary<Type, ObjectType>
{
public:
    static String name() { return "Object"; }
};

Nullary<Type, ObjectType> Nullary<Type, ObjectType>::_instance;

class LabelType : public Nullary<Type, LabelType>
{
public:
    static String name() { return "Label"; }
};

Nullary<Type, LabelType> Nullary<Type, LabelType>::_instance;

class VoidType : public Nullary<Type, VoidType>
{
public:
    static String name() { return "Void"; }
};

Nullary<Type, VoidType> Nullary<Type, VoidType>::_instance;

template<class T> Type typeFromCompileTimeType()
{
    throw Exception("Don't know this type.");
}
template<> Type typeFromCompileTimeType<int>() { return IntegerType(); }
template<> Type typeFromCompileTimeType<String>() { return StringType(); }
template<> Type typeFromCompileTimeType<bool>() { return BooleanType(); }

template<class T> class TemplateTemplate : public Tyco
{
public:
    Tyco instantiate(const List<Tyco>& arguments) const
    {
        Tyco t(*this);
        for (auto i = arguments.begin(); i != arguments.end(); ++i) {
            if (kind() == TypeKind())
                throw Exception(String("Can't instantiate ") +
                    toString() + " because it's not a template.");
            t = t->instantiate(*i);
        }
        return t;
    }
};

class ArrayType : public Type
{
public:
    ArrayType(const Type& contained, const Type& indexer)
      : Type(new Implementation(contained, indexer)) { }
    Type contained() const { return implementation()->contained(); }
    Type indexer() const { return implementation()->indexer(); }
private:
    class Implementation : public Type::Implementation
    {
    public:
        Implementation(const Type &contained) : _contained(contained) { }
        String toString() const { return _contained.toString() + "[]"; }
        bool equals(const Type::Implementation* other) const
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
    const Implementation* implementation() const
    {
        return _implementation.referent<Implementation>();
    }
};

class ArrayTemplate : public Nullary<Template, ArrayTemplate>
{
public:
    static String name() { return "Array"; }
private:
    class Implementation : public Nullary::Implementation
    {
    public:
        Tyco instantiate(const Tyco& tyco) const
        {
            if (tyco.kind() != TypeKind())
                throw Exception(String("Can't instantiate Array (argument "
                    "kind Type) with ") + tyco.toString() + " (kind " +
                    tyco.kind().toString() + ")");
            return ArrayType(tyco);
        }
        Kind kind() const { return TemplateKind(TypeKind(), TypeKind()); }
    };
};

Nullary<Template, ArrayTemplate> Nullary<Template, ArrayTemplate>::_instance;

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
            return String("[") + _contained.toString() + "]";
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
private:
    class Implementation : public Nullary::Implementation
    {
    public:
        Tyco instantiate(const Tyco& tyco) const
        {
            Type t = tyco.as<Type>();
            if (!t.valid())
                throw Exception(String("Can't instantiate Array (argument "
                    "kind Type) with ") + tyco.toString() + " (kind " +
                    tyco.kind().toString() + ")");
            return SequenceType(t);
        }
        Kind kind() const { return TemplateKind(TypeKind(), TypeKind()); }
    };
};

Nullary<Template, SequenceTemplate>
    Nullary<Template, SequenceTemplate>::_instance;

class TupleType : public Type
{
public:
    TupleType(const List<Type>& arguments)
      : Type(new Implementation(arguments)) { }
private:
    class Implementation : public Type::Implementation
    {
    public:
        Implementation(const List<Type>& arguments) : _arguments(arguments) { }
        String toString() const
        {
            String s = "(";
            bool needComma = false;
            for (auto i = _arguments.begin(); i != _arguments.end(); ++i) {
                if (needComma)
                    s += ", ";
                needComma = true;
                s += i->toString();
            }
            return s + ")";
        }
    private:
        List<Type> _arguments;
    };
};

class TupleTemplate : public Nullary<Template, TupleTemplate>
{
public:
    static String name() { return "Tuple"; }
private:
    class Implementation : public Nullary::Implementation
    {
    public:
        Tyco instantiate(const List<Tyco>& arguments)
        {
            List<Type> a;
            int p = 0;
            for (auto i = arguments.begin(); i != arguments.end(); ++i) {
                Type t = (*i).as<Type>();
                if (!t.valid())
                    throw Exception(String("Can't instantiate argument ") +
                        decimal(p) + " of Tuple with " + i->toString() +
                        " because it is not a type but is of kind " +
                        i->kind().toString() + ".");
                a.add(t);
            }
            return TupleType(a);
        }
        Kind kind() const { return VariadicTemplateKind(); }
    };
};

Nullary<Template, TupleTemplate> Nullary<Template, TupleTemplate>::_instance;

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
private:
    class Implementation : public Nullary::Implementation
    {
    public:
        Tyco instantiate(const Tyco& tyco) const
        {
            if (tyco.kind() != TypeKind())
                throw Exception(String("Can't instantiate Pointer (argument "
                    "kind Type) with ") + tyco.toString() + " (kind " +
                    tyco.kind().toString() + ")");
            return PointerType(tyco);
        }
        Kind kind() const { return TemplateKind(TypeKind(), TypeKind()); }
    };
};

Nullary<Template, PointerTemplate>
    Nullary<Template, PointerTemplate>::_instance;

class FunctionType : public Type
{
public:
    FunctionType(const Type& returnType, const List<Type>& parameterTypes)
      : Type(new Implementation(returnType, parameterTypes)) { }
private:
    class Implementation : public Type::Implementation
    {
    public:
        Implementation(const Type& returnType,
            const List<Type>& parameterTypes)
          : _returnType(returnType), _parameterTypes(parameterTypes) { }
        String toString() const
        {
            String s = _returnType.toString() + "(";
            for (int i = 0; i < _parameterTypes.count(); ++i) {
                if (i > 0)
                    s += ", ";
                s += _parameterTypes[i].toString();
            }
            return s + ")";
        }
        bool equals(const Type::Implementation* other) const
        {
            const Implementation* o =
                dynamic_cast<const Implementation*>(other);
            if (o == 0)
                return false;
            if (_returnType != o->_returnType)
                return false;
            int c = _parameterTypes.count();
            if (c != o->_parameterTypes.count())
                return false;
            for (int i = 0; i < c; ++i)
                if (_parameterTypes[i] != o->_parameterTypes[i])
                    return false;
            return true;
        }
        int hash() const
        {
            int h = _returnType.hash()*67 + 2;
            for (int i = 0; i < _parameterTypes.count(); ++i)
                h = h*67 + _parameterTypes[i].hash();
            return h;
        }
    private:
        Type _returnType;
        Array<Type> _parameterTypes;
    };
};

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
    private:
        int _n;
    };
};

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

// StructuredType is the type of "{...}" literals, not the base type for all
// types which have members. The ALFE compiler will need a more complicated
// implementation of structures, including using the same conversions at
// compile-time as at run-time. Also we don't want to have to override
// conversion functions in children just to avoid unwanted conversions
class StructuredType : public Type
{
public:
    class Member
    {
    public:
        Member(String name, Type type) : _name(name), _default(type) { }
        Member(String name, TypedValue defaultValue)
          : _name(name), _default(defaultValue) { }
        template<class T> Member(String name, const T& defaultValue)
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

    StructuredType() { }
    StructuredType(const Type& other)
      : Type(other._implementation.referent<Implementation>()) { }
    StructuredType(String name, List<Member> members)
      : Type(new Implementation(name, members)) { }
    const HashTable<String, int>* names() const
    {
        return implementation()->names();
    }
    const Array<Member>* members() const
    {
        return implementation()->members();
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
        const HashTable<String, int>* names() const { return &_names; }
        const Array<Member>* members() const { return &_members; }

        TypedValue tryConvertTo(const Type& to, const TypedValue& value,
            String* why) const
        {
            const Implementation* toImplementation =
                to._implementation.referent<Implementation>();
            if (toImplementation != 0) {
                auto input =
                    value.value<Value<HashTable<String, TypedValue>>>();
                Value<HashTable<String, TypedValue>> output;

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
                    value.value<Value<HashTable<String, TypedValue>>>();
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

            return TypedValue();
        }                      
        bool has(String memberName) const { return _names.hasKey(memberName); }

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
        HashTable<String, int> _names;
        Array<Member> _members;
    };
    const Implementation* implementation() const
    {
        return _implementation.referent<Implementation>();
    }

    friend class Implementation;
};

#endif // INCLUDED_TYPE_H
