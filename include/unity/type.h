#ifndef INCLUDED_TYPE_H
#define INCLUDED_TYPE_H

#include "unity/string.h"
#include "unity/any.h"

class Type
{
public:
    Type() { }
    String toString() const { return _implementation->name(); }
    bool valid() const { return _implementation.valid(); }
    bool operator==(const Type& other) const
    {
        return _implementation == other._implementation; 
    }
    bool operator!=(const Type& other) const
    {
        return _implementation != other._implementation; 
    }
protected:
    class Implementation : public ReferenceCounted
    {
    public:
        Implementation(const String& name) : _name(name) { }
        String name() const { return _name; }
    private:
        String _name;
    };
private:
    Type(ConstReference<Implementation> implementation)
      : _implementation(implementation) { }
    void setImplementation(Implementation* implementation)
    {
        _implementation = implementation;
    }
    ConstReference<Implementation> _implementation;

    friend class EnumerationType;
    friend class StructuredType;
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
    {
        setImplementation(new Implementation(name, values));
    }
    const Array<Value>* values() const
    {
        return ConstReference<Implementation>(_implementation)->values();
    }
private:
    class Implementation : public Type::Implementation
    {
    public:
        Implementation(String name, List<Value> values)
          : Type::Implementation(name), _values(values) { }
        bool isEnumeration() const { return true; }
        const Array<Value>* values() const { return &_values; }
    private:
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
    {
        setImplementation(new Implementation(name, members));
    }
    const Array<Member>* members() const
    {
        return ConstReference<Implementation>(_implementation)->members(); 
    }
private:
    class Implementation : public Type::Implementation
    {
    public:
        Implementation(String name, List<Member> members)
          : Type::Implementation(name), _members(members) { }
        bool isStrufcture() const { return true; }
        const Array<Member>* members() const { return &_members; }
    private:
        Array<Member> _members;
    };
};

template<class T> class AtomicType : public Type
{
protected:
    AtomicType(String name)
    {
        setImplementation(implementation(name)); 
    }
private:
    static Reference<Implementation> _implementation;
    class Implementation : public Type::Implementation { };
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

#endif // INCLUDED_TYPE_H