#ifndef INCLUDED_TYPE_H
#define INCLUDED_TYPE_H

#include "unity/string.h"
#include "unity/any.h"

class Type
{
public:
    Type() { }
    String toString() const { return _implementation->toString(); }
    bool valid() const { return _implementation.valid(); }
    bool operator==(const Type& other) const
    {
        if (_implementation == other._implementation)
            return true;
        return _implementation->equals(other._implementation);
    }
    bool operator!=(const Type& other) const { return !operator==(other); }
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
    Type(Implementation* implementation) : _implementation(implementation) { }
private:
    Type(ConstReference<Implementation> implementation)
      : _implementation(implementation) { }
    ConstReference<Implementation> _implementation;

    friend class EnumerationType;
    friend class StructuredType;
};

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

template<class T> class AtomicType : public Type
{
protected:
    AtomicType(String name) : Type(implementation(name)) { }
private:
    static Reference<Implementation> _implementation;
    class Implementation : public Type::Implementation
    { 
    public:
        Implementation(String name) : _name(name) { }
        String toString() const { return _name; }
    private:
        String _name;
    };
    static Reference<Implementation> implementation(const String& name)
    {
        if (!_implementation.valid())
            _implementation = new Implementation(name);
        return _implementation;
    }
};

class IntegerType : public AtomicType<IntegerType>
{
public:
    IntegerType() : AtomicType("Integer") { }
};

class BooleanType : public AtomicType<BooleanType>
{
public:
    BooleanType() : AtomicType("Boolean") { }
};

class StringType : public AtomicType<StringType>
{
public:
    StringType() : AtomicType("String") { }
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

        }
    private:
        List<Type> _parameterTypes;
    };
};

#endif // INCLUDED_TYPE_H