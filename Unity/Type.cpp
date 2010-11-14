class TypeImplementation : public ReferenceCounted
{
public:
    virtual bool equals(TypeImplementation* other) = 0;
};

class Type
{
public:
protected:
    Type(Reference<TypeImplementation> implementation) : _implementation(implementation) { }
    bool operator==(const Type& other) { return _implementation->equals(other._implementation); }
private:
    Reference<TypeImplementation> _implementation;
};


class IntTypeImplementation : public TypeImplementation
{
public:
    bool equals(TypeImplementation* other)
    {
        return (dynamic_cast<IntTypeImplementation*>(other) != 0);
    }
};

class IntType : public Type
{
public:
    IntType() : Type(implementation()) { }
private:
    static Reference<TypeImplementation> _implementation;
    static Reference<TypeImplementation> implementation()
    {
        if (!_implementation.valid())
            _implementation = new IntTypeImplementation;
        return _implementation;
    }
};


class StringTypeImplementation : public TypeImplementation
{
public:
    bool equals(TypeImplementation* other)
    {
        return (dynamic_cast<StringTypeImplementation*>(other) != 0);
    }
};

class StringType : public Type
{
public:
    StringType() : Type(implementation()) { }
private:
    static Reference<TypeImplementation> _implementation;
    static Reference<TypeImplementation> implementation()
    {
        if (!_implementation.valid())
            _implementation = new StringTypeImplementation;
        return _implementation;
    }
};


class VoidTypeImplementation : public TypeImplementation
{
public:
    bool equals(TypeImplementation* other)
    {
        return (dynamic_cast<VoidTypeImplementation*>(other) != 0);
    }
};

class VoidType : public Type
{
public:
    VoidType() : Type(implementation()) { }
private:
    static Reference<TypeImplementation> _implementation;
    static Reference<TypeImplementation> implementation()
    {
        if (!_implementation.valid())
            _implementation = new VoidTypeImplementation;
        return _implementation;
    }
};


class PointerTypeImplementation : public TypeImplementation
{
public:
    PointerTypeImplementation(Type referentType) : _referentType(referentType) { }
    bool equals(TypeImplementation* other)
    {
        PointerTypeImplementation* otherImplementation = dynamic_cast<PointerTypeImplementation*>(other);
        if (otherImplementation == 0)
            return false;
        return _referentType == other->_referentType;
    }
private:
    Type _referentType;
};

class PointerType : public Type
{
public:
    PointerType(Type referentType) : Type(new PointerTypeImplementation(referentType)) { }
};


class FunctionTypeImplementation : public TypeImplementation
{
    Array<Type>
};

class FunctionType : public Type
{
public:
    FunctionType(Reference<Type> returnType, Stack<Reference<Type> >* argumentTypes)
      : _returnType(returnType)
    {
        argumentTypes->toArray(&_argumentTypes);
    }
    bool equals(Type* other)
    {
        FunctionType* f = dynamic_cast<FunctionType*>(other);
        if (f == 0)
            return false;
        if (!f->_returnType->equals(_returnType))
            return false;
        int n = _argumentTypes.count();
        if (!f->_argumentTypes.count() != n)
            return false;
        for (int i = 0; i < n; ++i)
            if (!f->_argumentTypes[i]->equals(_argumentTypes[i]))
                return false;
        return true;
    }
private:
    Reference<Type> _returnType;
    Array<Reference<Type> > _argumentTypes;
};


