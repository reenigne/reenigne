#include "alfe/main.h"

#ifndef INCLUDED_TYPE_H
#define INCLUDED_TYPE_H

#include "alfe/any.h"
#include "alfe/hash_table.h"
#include "alfe/value.h"

class Kind
{
public:
    static Kind type;
    static Kind variadic;
    static Kind variadicTemplate;
    String toString() const { return _implementation->toString(); }
    bool operator==(const Kind& other) const
    {
        if (_implementation == other._implementation)
            return true;
        return _implementation->equals(other._implementation);
    }
    bool operator!=(const Kind& other) const { return !operator==(other); }
protected:
    class Implementation : public ReferenceCounted
    {
    public:
        virtual String toString() const = 0;
        virtual bool equals(const Implementation* other) const
        {
            return false;
        }
    };
    Kind(const Implementation* implementation)
      : _implementation(implementation) { }
private:
    class TypeImplementation : public Implementation
    {
    public:
        String toString() const { return String(); }
    };
    class VariadicImplementation : public Implementation
    {
    public:
        String toString() const { return String("..."); }
    };
    ConstReference<Implementation> _implementation;
    friend class TemplateKind;
};

Kind Kind::type = Kind(new Kind::TypeImplementation);
Kind Kind::variadic = Kind(new Kind::VariadicImplementation);

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
private:
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
            } while (k != Kind::type);
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

Kind Kind::variadicTemplate = TemplateKind(Kind::variadic, Kind::type);

template<class T> class TemplateTemplate;

typedef TemplateTemplate<void> Template;

template<class T> class TypeTemplate;

typedef TypeTemplate<void> Type;

class Tyco
{
public:
    Tyco() { }
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
    bool isInstantiation() const { return _implementation->isInstantiation(); }
    Tyco generatingTemplate() const
    {
        return _implementation->generatingTemplate();
    }
    Tyco templateArgument() const
    {
        return _implementation->templateArgument();
    }
protected:
    class Implementation : public ReferenceCounted
    {
    public:
        virtual String toString() const = 0;
        virtual bool equals(const Implementation* other) const
        {
            return false;
        }
        virtual Kind kind() const = 0;
        virtual int hash() const { return reinterpret_cast<intptr_t>(this); }
        virtual bool isInstantiation() const { return false; }
        virtual Tyco generatingTemplate() const { throw Exception(); }
        virtual Tyco templateArgument() const { throw Exception(); }
    };
    Tyco(const Implementation* implementation)
      : _implementation(implementation) { }
    ConstReference<Implementation> _implementation;

    friend class TemplateTemplate<void>;
    friend class EnumerationType;
    friend class StructuredType;
};

template<class T> class TypedValueTemplate;
typedef TypedValueTemplate<void> TypedValue;

template<class T> class TypeTemplate : public Tyco
{
public:
    TypeTemplate() { }
    TypeTemplate(const Tyco& tyco) : Tyco(tyco) { }

    static Type integer;
    static Type string;
    static Type boolean;
    static Type object;
    static Type label;
    static Type voidType;

    static Type array(const Type& type)
    {
        List<Tyco> arguments;
        arguments.add(type);
        return TemplateTemplate<T>::array.instantiate(arguments);
    }
    static Type tuple(const List<Type>& arguments)
    {
        List<Tyco> a;
        for (auto i = arguments.begin(); i != arguments.end(); ++i)
            a.add(*i);
        return TemplateTemplate<T>::tuple.instantiate(a);
    }
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
    TypeTemplate(const Implementation* implementation)
      : Tyco(implementation) { }
protected:
    class Implementation : public Tyco::Implementation
    {
    public:
        Kind kind() const { return Kind::type; }
        virtual TypedValueTemplate<T> tryConvert(
            const TypedValueTemplate<T>& value, String* reason) const
        { 
            if (this == value.type().implementation())
                return value;
            return TypedValueTemplate<T>();
        }
        virtual TypedValueTemplate<T> tryConvertTo(const Type& to,
            const TypedValue& value, String* reason) const
        { 
            if (this == to.implementation())
                return value;
            return TypedValueTemplate<T>();
        }
    };
    const Implementation* implementation() const
    {
        return _implementation.referent<Implementation>();
    }

    friend class TemplateTemplate<void>;
};

template<class T> Type typeFromCompileTimeType()
{
    throw Exception("Don't know this type.");
}
template<> Type typeFromCompileTimeType<int>()
{
    return Type::integer;
}
template<> Type typeFromCompileTimeType<String>()
{
    return Type::string;
}
template<> Type typeFromCompileTimeType<bool>()
{
    return Type::boolean;
}

class AtomicType : public Type
{
public:
    AtomicType(String name) : Type(new Implementation(name)) { }
    AtomicType(const Tyco& tyco) : Type(tyco) { }
protected:
    AtomicType(const Implementation* implementation) : Type(implementation) { }

    class Implementation : public Type::Implementation
    {
    public:
        Implementation(String name) : _name(name) { }
        String toString() const { return _name; }
    private:
        String _name;
    };
};

template<> Type Type::integer = AtomicType("Integer");
template<> Type Type::string = AtomicType("String");
template<> Type Type::boolean = AtomicType("Boolean");
template<> Type Type::object = AtomicType("Object");
template<> Type Type::label = AtomicType("Label");
template<> Type Type::voidType = AtomicType("Void");

template<class T> class TemplateTemplate : public Tyco
{
public:
    TemplateTemplate(const String& name, const Kind& kind)
      : Tyco(new UninstantiatedImplementation(name, kind)) { }
    Tyco instantiate(const List<Tyco>& arguments) const
    {
        Tyco t = *this;
        for (auto i = arguments.begin(); i != arguments.end(); ++i) {
            ConstReference<Implementation> ti =
                t._implementation.referent<Implementation>();
            if (!ti.valid())
                throw Exception(String("Can't instantiate ") + t.toString());
            t = ti->instantiate(*i);
        }
        return t;
    }

    static Template array;
    static Template tuple;
private:
    class Implementation : public Tyco::Implementation
    {
    public:
        virtual Tyco instantiate(const Tyco& tyco) const = 0;
        Tyco instantiate(const TemplateKind& kind, const Tyco& tyco) const
        {
            if (_instantiations.hasKey(tyco))
                return _instantiations[tyco];
            if (kind.first() != Kind::variadic) {
                if (kind.first() != tyco.kind())
                    throw Exception(String("Can't instantiate ") + toString() +
                        String(" (argument kind ") + kind.first().toString() +
                        String(") with ") + tyco.toString() +
                        String(" (kind ") + tyco.kind().toString());
                Kind rest = kind.rest();
                Tyco instantiation;
                if (rest == Kind::type)
                    instantiation =
                        Type(new InstantiatedImplementation(this, tyco));
                else
                    instantiation = Tyco(
                        new PartiallyInstantiatedImplementation(this, tyco));
                _instantiations.add(tyco, instantiation);
                return instantiation;
            }
            Tyco instantiation(new VariadicImplementation(this, tyco));
            _instantiations.add(tyco, instantiation);
            return instantiation;
        }
        virtual String toString2(bool* needComma) const = 0;
    private:
        mutable HashTable<Tyco, Tyco> _instantiations;
    };
    class UninstantiatedImplementation : public Implementation
    {
    public:
        UninstantiatedImplementation(const String& name, const Kind& kind)
          : _name(name), _kind(kind) { }
        Kind kind() const { return _kind; }
        String toString() const { return _name; }
        Tyco instantiate(const Tyco& tyco) const
        {
            return Implementation::instantiate(_kind, tyco);
        }
    protected:
        String toString2(bool* needComma) const
        {
            *needComma = false;
            return _name + "<";
        }
    private:
        String _name;
        Kind _kind;
    };
    class PartiallyInstantiatedImplementation : public Implementation
    {
    public:
        PartiallyInstantiatedImplementation(const Implementation* parent,
            Tyco argument)
          : _parent(parent), _argument(argument) { }
        Kind kind() const { return TemplateKind(_parent->kind()).rest(); }
        String toString() const
        {
            bool needComma;
            return toString2(&needComma) + ">";
        }
        String toString2(bool* needComma) const
        {
            String s = _parent->toString2(needComma);
            if (*needComma)
                s += ", ";
            s += _argument.toString();
            *needComma = true;
            return s;
        }
        Tyco instantiate(const Tyco& tyco) const
        {
            return Implementation::instantiate(kind(), tyco);
        }
        virtual bool isInstantiation() const { return true; }
        virtual Tyco generatingTemplate() const { return Tyco(_parent); }
        virtual Tyco templateArgument() const { return _argument; }
    private:
        const Implementation* _parent;
        Tyco _argument;
        mutable HashTable<Tyco, Tyco> _instantiations;
    };
    class VariadicImplementation : public PartiallyInstantiatedImplementation
    {
    public:
        VariadicImplementation(const Implementation* parent,
            const Tyco& argument)
          : PartiallyInstantiatedImplementation(parent, argument) { }
        Kind kind() const { return Kind::variadicTemplate; }
    };
    class InstantiatedImplementation : public Type::Implementation
    {
    public:
        InstantiatedImplementation(
            const typename TemplateTemplate::Implementation* parent,
            Tyco argument)
          : _parent(parent), _argument(argument) { }
        String toString() const
        {
            bool needComma;
            String s = _parent->toString2(&needComma);
            if (needComma)
                s += ", ";
            return s + _argument.toString() + ">";
        }

        virtual bool isInstantiation() const { return true; }
        virtual Tyco generatingTemplate() const { return Tyco(_parent); }
        virtual Tyco templateArgument() const { return _argument; }
    private:
        const typename TemplateTemplate::Implementation* _parent;
        Tyco _argument;
    };

    friend class StructuredType;
};

// If we give Array a class of its own, StructuredType will need to be
// modified to use it for conversions.
template<> Template Template::array("Array",
    TemplateKind(Kind::type, Kind::type));
template<> Template Template::tuple("Tuple", Kind::variadicTemplate);

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
            const Template::InstantiatedImplementation* arrayImplementation =
                to._implementation.
                referent<Template::InstantiatedImplementation>();
            if (arrayImplementation != 0 && 
                arrayImplementation->generatingTemplate() == Template::array) {
                Type contained = arrayImplementation->templateArgument();
                if (!contained.valid()) {
                    *why = String("Array instantiated with non-type argument");
                    return TypedValue();
                }
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
