#ifndef INCLUDED_TYPE_H
#define INCLUDED_TYPE_H

#include "unity/string.h"
#include "unity/any.h"
#include "unity/hash_table.h"

class Kind
{
public:
    static Kind type;
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
    Kind() : _implementation(new TypeImplementation) { }
    class TypeImplementation : public Implementation
    {
    public:
        String toString() const { return String(); }
    };
    ConstReference<Implementation> _implementation;

    friend class TemplateKind;
};

Kind Kind::type;

class TemplateKind : public Kind
{
public:
    TemplateKind(const Kind& firstParameterKind, const Kind& restParameterKind)
      : Kind(new Implementation(firstParameterKind, restParameterKind)) { }
    TemplateKind(const Kind& kind)
      : Kind(ConstReference<Implementation>(kind._implementation)) { }
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
            String s = lessThan;
            TemplateKind k(this);
            bool needComma = false;
            do {
                if (needComma)
                    s += commaSpace;
                s += k.first().toString();
                k = k.rest();
                needComma = true;
            } while (k != Kind::type);
            return s + greaterThan;
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
        return ConstReference<Implementation>(_implementation);
    }
};

class TypeConstructor
{
public:
    TypeConstructor() { }
    String toString() const { return _implementation->toString(); }
    bool valid() const { return _implementation.valid(); }
    bool operator==(const TypeConstructor& other) const
    { 
        if (_implementation == other._implementation)
            return true;
        return _implementation->equals(other._implementation);
    }
    bool operator!=(const TypeConstructor& other) const
    {
        return !operator==(other);
    }
    int hash() const { return _implementation->hash(); }
    Kind kind() const { return _implementation->kind(); }
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
        virtual int hash() const { return reinterpret_cast<int>(this); }
    };
    TypeConstructor(const Implementation* implementation)
      : _implementation(implementation) { }
    ConstReference<Implementation> _implementation;
};

template<class T> class TypeTemplate;
typedef TypeTemplate<void> Type;
template<class T> class TypeTemplate : public TypeConstructor
{
public:
    TypeTemplate() { }

    static Type integer;
    static Type string;
    static Type boolean;
    static Type object;
protected:
    class Implementation : public TypeConstructor::Implementation
    {
    public:
        Kind kind() const { return Kind::type; }
    };
    TypeTemplate(const Implementation* implementation)
      : TypeConstructor(implementation) { }
private:
    template<class T> class AtomicTypeTemplate : public TypeTemplate<T>
    {
    public:
        AtomicTypeTemplate(String name) : Type(new Implementation(name)) { }
    private:
        class Implementation : public TypeTemplate::Implementation
        { 
        public:
            Implementation(String name) : _name(name) { }
            String toString() const { return _name; }
        private:
            String _name;
        };
    };
    typedef AtomicTypeTemplate<void> AtomicType;

    friend class EnumerationType;
    friend class StructuredType;
    friend class TemplateTypeConstructor;
    friend class TemplateTypeConstructor::Implementation;
    friend class TemplateTypeConstructor::PartiallyInstantiatedImplementation;
};

Type Type::integer = Type::AtomicType("Integer");
Type Type::string = Type::AtomicType("String");
Type Type::boolean = Type::AtomicType("Boolean");
Type Type::object = Type::AtomicType("Object");

class PointerType : public Type
{
public:
    PointerType(const Type& referent) : Type(new Implementation(referent)) { }
private:
    class Implementation : public Type::Implementation
    {
    public:
        Implementation(const Type &referent) : _referent(referent) { }
        String toString() const { return _referent.toString() + asterisk; }
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
            String s = _returnType.toString() + openParenthesis;
            for (int i = 0; i < _parameterTypes.count(); ++i) {
                if (i > 0)
                    s += commaSpace;
                s += _parameterTypes[i].toString();
            }
            return s + closeParenthesis;
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
      : Type(ConstReference<Implementation>(other._implementation)) { }
    EnumerationType(String name, List<Value> values)
      : Type(new Implementation(name, values)) { }
    const Array<Value>* values() const
    {
        return ConstReference<Implementation>(_implementation)->values();
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

class StructuredType : public Type
{
public:
    class Member
    {
    public:
        Member(String name, Type type) : _name(name), _type(type) { }
        String name() const { return _name; }
        Type type() const { return _type; }
    private:
        String _name;
        Type _type;
    };

    StructuredType(const Type& other)
      : Type(ConstReference<Implementation>(other._implementation)) { }
    StructuredType(String name, List<Member> members)
      : Type(new Implementation(name, members)) { }
    const Array<Member>* members() const
    {
        return ConstReference<Implementation>(_implementation)->members(); 
    }
private:
    class Implementation : public Type::Implementation
    {
    public:
        Implementation(String name, List<Member> members)
          : _name(name), _members(members) { }
        String toString() const { return _name; }
        const Array<Member>* members() const { return &_members; }
    private:
        String _name;
        Array<Member> _members;
    };
};

class TupleType : public Type
{
public:
    TupleType(const List<Type>& parameterTypes)
      : Type(new Implementation(parameterTypes)) { }
private:
    class Implementation : public Type::Implementation
    {
    public:
        Implementation(const List<Type>& parameterTypes)
          : _parameterTypes(parameterTypes) { }
        String toString() const
        {
            String s("Tuple<");
            for (int i = 0; i < _parameterTypes.count(); ++i) {
                if (i > 0)
                    s += commaSpace;
                s += _parameterTypes[i].toString();
            }
            return s + greaterThan;
        }
        bool equals(const Type::Implementation* other) const
        {
            const Implementation* o =
                dynamic_cast<const Implementation*>(other);
            if (o == 0)
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
            int h = 3;
            for (int i = 0; i < _parameterTypes.count(); ++i)
                h = h*67 + _parameterTypes[i].hash();
            return h;
        }
    private:
        Array<Type> _parameterTypes;
    };
};

class TemplateTypeConstructor : public TypeConstructor
{
public:
    TemplateTypeConstructor(const String& name, const Kind& kind)
      : TypeConstructor(new UninstantiatedImplementation(name, kind)) { }
    TypeConstructor instantiate(const List<TypeConstructor>& arguments)
    {
    }
private:
    class Implementation : public TypeConstructor::Implementation
    {
    protected:
        TypeConstructor instantiate(const TemplateKind& kind,
            const TypeConstructor& typeConstructor) const
        {
            if (_instantiations.hasKey(typeConstructor))
                return _instantiations[typeConstructor];
            if (kind.first() != typeConstructor.kind())
                throw Exception(String("Can't instantiate ") + toString() +
                    String(" (argument kind ") + kind.first().toString() +
                    String(") with ") + typeConstructor.toString() +
                    String(" (kind ") + typeConstructor.kind().toString());
            Kind rest = kind.rest();
            TypeConstructor instantiation;
            if (rest == Kind::type)
                instantiation = Type(
                    new InstantiatedImplementation(this, typeConstructor));
            else
                instantiation = TypeConstructor(
                    new PartiallyInstantiatedImplementation(this,
                        typeConstructor));
            _instantiations.add(typeConstructor, instantiation);
            return instantiation;
        }
    private:
        mutable HashTable<TypeConstructor, TypeConstructor> _instantiations;
    };
    class UninstantiatedImplementation : public Implementation
    {
    public:
        UninstantiatedImplementation(const String& name, const Kind& kind)
          : _name(name), _kind(kind) { }
        Kind kind() const { return _kind; }
        String toString() const { return _name; }
        TypeConstructor instantiate(const TypeConstructor& typeConstructor)
        {
            return instantiate(_kind, typeConstructor);
        }
    private:
        String _name;
        Kind _kind;
    };
    class PartiallyInstantiatedImplementation : public Implementation
    {
    public:
        PartiallyInstantiatedImplementation(const Implementation* parent,
            TypeConstructor argument)
          : _parent(parent), _argument(argument) { }
        Kind kind() const { return TemplateKind(_parent->kind()).rest(); }
        String toString() const
        {
            //if (_arguments.count() == 0)
            //    return _name;
            //String s = _name + lessThan;
            //for (int i = 0; i < _arguments.count(); ++i) {
            //    if (i > 0)
            //        s += commaSpace;
            //    s += _arguments[i].toString();
            //}
            //return s + greaterThan;
        }
        TypeConstructor instantiate(const TypeConstructor& typeConstructor)
            const
        {
            return instantiate(kind(), typeConstructor);
        }
    private:
        const Implementation* _parent;
        TypeConstructor _argument;
        mutable HashTable<TypeConstructor, TypeConstructor> _instantiations;
    };
    class InstantiatedImplementation : public Type::Implementation
    {
    public:
        InstantiatedTypeImplementation(
            const TemplateTypeConstructor::Implementation* implementation,
            TypeConstructor argument)
          : _implementation(implementation), _argument(argument) { }
        String toString() const
        {
            // TODO
        }
    private:
        const Implementation* _implementation;
        TypeConstructor _argument;
    };
};

#endif // INCLUDED_TYPE_H