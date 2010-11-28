class VoidType;

template<class T> class TypeTemplate;

typedef TypeTemplate<void> Type;

template<class T> class TypeTemplate
{
public:
    TypeTemplate() { }
    bool operator==(const TypeTemplate& other)
    {
        return _implementation->equals(other._implementation);
    }
    bool operator!=(const TypeTemplate& other)
    {
        return !_implementation->equals(other._implementation);
    }
    int hash() const { return _implementation->hash(); }
    String toString() const { return _implementation->toString(); }
    bool valid() const { return _implementation.valid(); }
    Type referentType() const { return _implementation->referentType(); }
    Type returnType() const { return _implementation->returnType(); }
protected:
    class Implementation : public ReferenceCounted
    {
    public:
        virtual bool equals(const Implementation* other) = 0;
        virtual int hash() const = 0;
        virtual String toString() const = 0;
        virtual Type referentType() const = 0;
        virtual Type returnType() const = 0;
    };
    TypeTemplate(Reference<Implementation> implementation)
      : _implementation(implementation)
    { }
private:
    Reference<Implementation> _implementation;
};

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
        String toString() const
        {
            static String s("Void");
            return s;
        }
        Type referentType() const { return Type(); }
        Type returnType() const { return Type(); }
    };
    friend class TypeTemplate<void>;
};

Reference<Type::Implementation> VoidType::_implementation;

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
        String toString() const
        {
            static String s("Int");
            return s;
        }
        Type referentType() const { return Type(); }
        Type returnType() const { return Type(); }
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
        String toString() const
        {
            static String s("String");
            return s;
        }
        Type referentType() const { return Type(); }
        Type returnType() const { return Type(); }
    };
};

Reference<Type::Implementation> StringType::_implementation;

class BooleanType : public Type
{
public:
    BooleanType() : Type(implementation()) { }
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
        int hash() const { return 5; }
        String toString() const
        {
            static String s("Boolean");
            return s;
        }
        Type referentType() const { return Type(); }
        Type returnType() const { return Type(); }
    };
};

Reference<Type::Implementation> BooleanType::_implementation;

class BitType : public Type
{
public:
    BitType() : Type(implementation()) { }
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
        int hash() const { return 6; }
        String toString() const
        {
            static String s("Bit");
            return s;
        }
        Type referentType() const { return Type(); }
        Type returnType() const { return Type(); }
    };
};

Reference<Type::Implementation> BitType::_implementation;

class ByteType : public Type
{
public:
    ByteType() : Type(implementation()) { }
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
        int hash() const { return 7; }
        String toString() const
        {
            static String s("Byte");
            return s;
        }
        Type referentType() const { return Type(); }
        Type returnType() const { return Type(); }
    };
};

Reference<Type::Implementation> ByteType::_implementation;

class CharacterType : public Type
{
public:
    CharacterType() : Type(implementation()) { }
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
        int hash() const { return 8; }
        String toString() const
        {
            static String s("Character");
            return s;
        }
        Type referentType() const { return Type(); }
        Type returnType() const { return Type(); }
    };
};

Reference<Type::Implementation> CharacterType::_implementation;

class UIntType : public Type
{
public:
    UIntType() : Type(implementation()) { }
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
        int hash() const { return 9; }
        String toString() const
        {
            static String s("UInt");
            return s;
        }
        Type referentType() const { return Type(); }
        Type returnType() const { return Type(); }
    };
};

Reference<Type::Implementation> UIntType::_implementation;

class WordType : public Type
{
public:
    WordType() : Type(implementation()) { }
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
        int hash() const { return 10; }
        String toString() const
        {
            static String s("Word");
            return s;
        }
        Type referentType() const { return Type(); }
        Type returnType() const { return Type(); }
    };
};

Reference<Type::Implementation> WordType::_implementation;

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
        String toString() const
        {
            static String s("*");
            return _referentType.toString() + s;
        }
        Type referentType() const { return _referentType; }
        Type returnType() const { return Type(); }
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
    String toString() const { return _implementation->toString(); }
    bool valid() const { return _implementation.valid(); }
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
        String toString() const
        {
            String s;
            for (int i = 0; i < _array.count(); ++i) {
                if (i > 0) {
                    static String comma(", ");
                    s += comma;
                }
                s += _array[i].toString();
            }
            return s;
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
        String toString() const
        {
            static String open("(");
            static String close(")");
            return _returnType.toString() + open + _argumentTypes.toString() + close;
        }
        Type referentType() const { return Type(); }
        Type returnType() const { return _returnType; }
    private:
        Type _returnType;
        TypeList _argumentTypes;
    };
public:
    FunctionType(Type returnType, TypeList argumentTypes)
      : Type(new Implementation(returnType, argumentTypes))
    { }
};

class ClassType : public Type
{
    class Implementation : public Type::Implementation
    {
    public:
        Implementation()
        { }
        bool equals(const Type::Implementation* otherBase)
        {
            const Implementation* other = dynamic_cast<const Implementation*>(otherBase);
            if (other == 0)
                return false;
            return this == other;
        }
        int hash() const
        {
            return reinterpret_cast<int>(this);
        }
        String toString() const
        {
            static String c("Class");
            return c + String::hexadecimal(reinterpret_cast<UInt32>(this), 8);
        }
        Type referentType() const { return Type(); }
        Type returnType() const { return Type(); }
    };
public:
    ClassType() : Type(new Implementation) { }
};