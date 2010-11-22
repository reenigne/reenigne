class VoidType;

class TypeIdentifier : public ReferenceCounted
{
public:
    static Reference<TypeIdentifier> parse(CharacterSource* source, Context* context)
    {
        CharacterSource s = *source;
        DiagnosticLocation location = s.location();
        int start = s.offset();
        int c = s.get();
        if (c < 'A' || c > 'Z')
            return 0;
        do {
            *source = s;
            c = s.get();
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')
                continue;
            break;
        } while (true);
        int end = source->offset();
        Space::parse(source, context);
        return new TypeIdentifier(s, s.subString(start, end), location);
    }
    String name() const { return _name; }
private:
    TypeIdentifier(String name, DiagnosticLocation location)
      : _name(name), _location(location)
    { }
    String _name;
    DiagnosticLocation _location;
};

template<class T> class TypeTemplate
{
public:
    static Type parse(CharacterSource* source, Context* context)
    {
        Reference<TypeIdentifier> identifier = TypeIdentifier::parse(source, context);
        if (!identifier.valid())
            return Type();
        Type type = SimpleType(identifier);
        do {
            CharacterSource s = *source;
            int c = s.get();
            if (c == '*') {
                *source = s;
                Space::parse(source, context);
                type = PointerType(type);
                continue;
            }
            if (c == '(') {
                *source = s;
                Space::parse(source, context);
                TypeList typeList = TypeList::parse(source, context);
                type = FunctionType(type, typeList);
                continue;
            }
            break;
        } while (true);
        return type;
    }
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
    String toString() const { return _implementation->toString(); }
    bool valid() const { return _implementation.valid(); }
protected:
    class Implementation : public ReferenceCounted
    {
    public:
        virtual bool equals(const Implementation* other) = 0;
        virtual int hash() const = 0;
        virtual String toString() const = 0;
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
        String toString() const
        {
            static String s("Void");
            return s;
        }
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
        String toString() const
        {
            static String s("Int");
            return s;
        }
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
    };
};

Reference<Type::Implementation> StringType::_implementation;

Reference<Type::Implementation> VoidType::_implementation;

class SimpleType : public Type
{
public:
    SimpleType(Reference<TypeIdentifier> identifier) : Type(new Implementation(identifier)) { }
private:
    class Implementation : public Type::Implementation
    {
    public:
        Implementation(Reference<TypeIdentifier> identifier) : _identifier(identifier) { }
        bool equals(const Type::Implemntation* otherBase)
        {
            const Implementation* other = dynamic_cast<const Implementation*>(otherBase);
            if (other == 0)
                return false;
            return _identifier->name() == other->_identifier->name();
        }
        int hash() const { return _identifier->name().hash(); }
        String toString() const
        {
            return _identifier->name();
        }
    private:
        Reference<TypeIdentifier> _identifier;
    };
};

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
    private:
        Type _returnType;
        TypeList _argumentTypes;
    };
public:
    FunctionType(Type returnType, TypeList argumentTypes)
      : Type(new Implementation(returnType, argumentTypes))
    { }
};


