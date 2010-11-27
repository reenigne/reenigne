template<class T> class TypeIdentifierTemplate;
typedef TypeIdentifierTemplate<void> TypeIdentifier;

template<class T> class TypeSpecifierTemplate : public ReferenceCounted
{
public:
    static Reference<TypeSpecifierTemplate> parse(CharacterSource* source, Scope* scope)
    {
        Reference<TypeSpecifierTemplate> typeSpecifier = TypeIdentifier::parse(source, scope);
        if (!typeSpecifier.valid())
            return 0;
        do {
            if (Space::parseCharacter(source, '*')) {
                typeSpecifier = new PointerTypeSpecifier(typeSpecifier);
                continue;
            }
            if (Space::parseCharacter(source, '(')) {
                Reference<TypeListSpecifier> typeListSpecifier = TypeListSpecifier::parse(source, scope);
                typeSpecifier = new FunctionTypeSpecifier(typeSpecifier, typeListSpecifier);
                continue;
            }
            break;
        } while (true);
        return typeSpecifier;
    }
    virtual Type type() = 0;
};

typedef TypeSpecifierTemplate<void> TypeSpecifier;

template<class T> class TypeIdentifierTemplate : public TypeSpecifier
{
public:
    static Reference<TypeIdentifier> parse(CharacterSource* source, Scope* scope)
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
        Space::parse(source);
        return new TypeIdentifier(scope, s.subString(start, end), location);
    }
    String name() const { return _name; }
    Type type()
    {
        if (!_type.valid()) {
            if (_resolving) {
                static String error("A type cannot be defined in terms of itself");
                _location.throwError(error);
            }
            _resolving = true;
            _type = _scope->resolveType(_name, _location)->type();
            _resolving = false;
        }
        return _type;
    }
private:
    TypeIdentifierTemplate(Scope* scope, String name, DiagnosticLocation location)
      : _scope(scope), _name(name), _location(location), _resolving(false)
    { }
    Scope* _scope;
    String _name;
    DiagnosticLocation _location;
    bool _resolving;
    Type _type;
};

class PointerTypeSpecifier : public TypeSpecifier
{
public:
    PointerTypeSpecifier(Reference<TypeSpecifier> referentTypeSpecifier)
      : _referentTypeSpecifier(referentTypeSpecifier)
    { }
    Type type()
    {
        if (!_type.valid())
            _type = PointerType(_referentTypeSpecifier->type());
        return _type;
    }
private:
    Reference<TypeSpecifier> _referentTypeSpecifier;
    Type _type;
};

class TypeListSpecifier : public ReferenceCounted
{
public:
    static Reference<TypeListSpecifier> parse(CharacterSource* source, Scope* scope)
    {
        Stack<Reference<TypeSpecifier> > stack;
        do {
            Reference<TypeSpecifier> typeSpecifier = TypeSpecifier::parse(source, scope);
            if (!typeSpecifier.valid()) {
                static String error("Type specifier expected");
                source->location().throwError(error);
            }
            stack.push(typeSpecifier);
            if (Space::parseCharacter(source, ','))
                continue;
            break;
        } while (true);
        return new TypeListSpecifier(&stack);
    }
    TypeList typeList()
    {
        if (!_typeList.valid()) {
            for (int i = 0; i < _array.count(); ++i)
                _typeList.push(_array[i]->type());
            _typeList.finalize();
        }
        return _typeList;
    }
private:
    TypeListSpecifier(Stack<Reference<TypeSpecifier> >* stack)
    {
        stack->toArray(&_array);
    }
    Array<Reference<TypeSpecifier> > _array;
    TypeList _typeList;
};

class FunctionTypeSpecifier : public TypeSpecifier
{
public:
    FunctionTypeSpecifier(Reference<TypeSpecifier> returnTypeSpecifier, Reference<TypeListSpecifier> argumentTypeListSpecifier)
      : _returnTypeSpecifier(returnTypeSpecifier), _argumentTypeListSpecifier(argumentTypeListSpecifier)
    { }
    Type type()
    {
        if (!_type.valid())
            _type = FunctionType(_returnTypeSpecifier->type(), _argumentTypeListSpecifier->typeList());
        return _type;
    }
private:
    Reference<TypeSpecifier> _returnTypeSpecifier;
    Reference<TypeListSpecifier> _argumentTypeListSpecifier;
    Type _type;
};


