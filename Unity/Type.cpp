class VoidType;

template<class T> class TypeTemplate
{
public:
    TypeTemplate() : _implementation(VoidType::implementation()) { }
    bool operator==(const TypeTemplate& other)
    {
        return _implementation->equals(other._implementation);
    }
    bool operator!=(const TypeTemplate& other)
    {
        return !_implementation->equals(other._implementation);
    }
    int hash() const { return _implementation->hash(); }
protected:
    class Implementation : public ReferenceCounted
    {
    public:
        virtual bool equals(const Implementation* other) = 0;
        virtual int hash() const = 0;
    };
    TypeTemplate(Reference<Implementation> implementation)
      : _implementation(implementation)
    { }
private:
    Reference<Implementation> _implementation;
};

typedef TypeTemplate<void> Type;

class VoidType : public Type
{
public:
    VoidType() : Type(implementation()) { }
private:
    static Reference<Type::Implementation> _implementation;
    static Reference<Type::Implementation> implementation()
    {
        if (!_implementation.valid())
            _implementation = new Implementation;
        return _implementation;
    }
    class Implementation : public Type::Implementation
    {
    public:
        bool equals(const Type::Implementation* other)
        {
            return (dynamic_cast<const Implementation*>(other) != 0);
        }
        int hash() const { return 0; }
    };
    friend class TypeTemplate<void>;
};

class IntType : public Type
{
public:
    IntType() : Type(implementation()) { }
private:
    static Reference<Type::Implementation> _implementation;
    static Reference<Type::Implementation> implementation()
    {
        if (!_implementation.valid())
            _implementation = new Implementation;
        return _implementation;
    }
    class Implementation : public Type::Implementation
    {
    public:
        bool equals(const Type::Implementation* other)
        {
            return (dynamic_cast<const Implementation*>(other) != 0);
        }
        int hash() const { return 1; }
    };
};

Reference<Type::Implementation> IntType::_implementation;

class StringType : public Type
{
public:
    StringType() : Type(implementation()) { }
private:
    static Reference<Type::Implementation> _implementation;
    static Reference<Type::Implementation> implementation()
    {
        if (!_implementation.valid())
            _implementation = new Implementation;
        return _implementation;
    }
    class Implementation : public Type::Implementation
    {
    public:
        bool equals(const Type::Implementation* other)
        {
            return (dynamic_cast<const Implementation*>(other) != 0);
        }
        int hash() const { return 2; }
    };
};

Reference<Type::Implementation> StringType::_implementation;

Reference<Type::Implementation> VoidType::_implementation;

class PointerType : public Type
{
public:
    PointerType(Type referentType) : Type(new Implementation(referentType)) { }
private:
    class Implementation : public Type::Implementation
    {
    public:
        Implementation(Type referentType) : _referentType(referentType) { }
        bool equals(const Type::Implementation* otherBase)
        {
            const Implementation* other = dynamic_cast<const Implementation*>(otherBase);
            if (other == 0)
                return false;
            return _referentType == other->_referentType;
        }
        int hash() const { return 3 * 67 + _referentType.hash(); }
    private:
        Type _referentType;
    };
};

class TypeList
{
public:
    TypeList() : _implementation(new Implementation) { }
    void push(Type type)
    {
        _implementation->push(type);
    }
    void finalize()
    {
        _implementation->finalize();
    }
    bool operator==(const TypeList& other) { return _implementation->equals(other._implementation); }
    bool operator!=(const TypeList& other) { return !_implementation->equals(other._implementation); }
    int hash() const { return _implementation->hash(); }
private:
    class Implementation : public ReferenceCounted
    {
    public:
        void push(Type type)
        {
            _stack.push(type);
        }
        void finalize()
        {
            _stack.toArray(&_array);
        }
        bool equals(const Implementation* other)
        {
            int n = _array.count();
            if (n != other->_array.count())
                return false;
            for (int i = 0 ; i < n; ++i)
                if (_array[i] != other->_array[i])
                    return false;
            return true;
        }
        int hash() const
        {
            int h = 0;
            for (int i = 0; i < _array.count(); ++i)
                h = h * 67 + _array[i].hash();
            return h;
        }
    private:
        Stack<Type> _stack;
        Array<Type> _array;
    };
    Reference<Implementation> _implementation;
};

class FunctionType : public Type
{
private:
    class Implementation : public Type::Implementation
    {
    public:
        Implementation(Type returnType, TypeList argumentTypes)
          : _returnType(returnType), _argumentTypes(argumentTypes)
        { }
        bool equals(const Type::Implementation* otherBase)
        {
            const Implementation* other = dynamic_cast<const Implementation*>(otherBase);
            if (other == 0)
                return false;
            return (_returnType == other->_returnType && _argumentTypes == other->_argumentTypes);
        }
        int hash() const
        {
            return (4*67 + _returnType.hash())*67 + _argumentTypes.hash();
        }
    private:
        Type _returnType;
        TypeList _argumentTypes;
    };
public:
    FunctionType(Type returnType, TypeList argumentTypes)
      : Type(new Implementation(returnType, argumentTypes))
    { }
};


