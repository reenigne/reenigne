//template<class T> class TypeSpecifierTemplate;
//typedef TypeSpecifierTemplate<void> TypeSpecifier;
//
//template<class T> class TypeSpecifierTemplate : public ReferenceCounted
//{
//public:
//    static Reference<TypeSpecifier> parse(CharacterSource* source, Scope* scope)
//    {
//        Reference<TypeSpecifier> typeSpecifier = parseFundamental(source, scope);
//        if (!typeSpecifier.valid())
//            return 0;
//        do {
//            if (Space::parseCharacter(source, '*')) {
//                typeSpecifier = new PointerTypeSpecifier(typeSpecifier);
//                continue;
//            }
//            if (Space::parseCharacter(source, '(')) {
//                Reference<TypeListSpecifier> typeListSpecifier = TypeListSpecifier::parse(source, scope);
//                typeSpecifier = new FunctionTypeSpecifier(typeSpecifier, typeListSpecifier);
//                continue;
//            }
//            break;
//        } while (true);
//        return typeSpecifier;
//    }
//    virtual Symbol type() = 0;
//private:
//    static Reference<TypeSpecifier> parseFundamental(CharacterSource* source, Scope* scope)
//    {
//        Reference<TypeSpecifier> typeSpecifier = TypeIdentifier::parse(source, scope);
//        if (typeSpecifier.valid())
//            return typeSpecifier;
//        typeSpecifier = AutoTypeSpecifier::parse(source, scope);
//        if (typeSpecifier.valid())
//            return typeSpecifier;
//        typeSpecifier = BitTypeSpecifier::parse(source, scope);
//        if (typeSpecifier.valid())
//            return typeSpecifier;
//        typeSpecifier = BooleanTypeSpecifier::parse(source, scope);
//        if (typeSpecifier.valid())
//            return typeSpecifier;
//        typeSpecifier = ByteTypeSpecifier::parse(source, scope);
//        if (typeSpecifier.valid())
//            return typeSpecifier;
//        typeSpecifier = CharacterTypeSpecifier::parse(source, scope);
//        if (typeSpecifier.valid())
//            return typeSpecifier;
//        typeSpecifier = ClassTypeSpecifier::parse(source, scope);
//        if (typeSpecifier.valid())
//            return typeSpecifier;
//        typeSpecifier = IntTypeSpecifier::parse(source, scope);
//        if (typeSpecifier.valid())
//            return typeSpecifier;
//        typeSpecifier = StringTypeSpecifier::parse(source, scope);
//        if (typeSpecifier.valid())
//            return typeSpecifier;
//        typeSpecifier = TypeOfTypeSpecifier::parse(source, scope);
//        if (typeSpecifier.valid())
//            return typeSpecifier;
//        typeSpecifier = UIntTypeSpecifier::parse(source, scope);
//        if (typeSpecifier.valid())
//            return typeSpecifier;
//        typeSpecifier = VoidTypeSpecifier::parse(source, scope);
//        if (typeSpecifier.valid())
//            return typeSpecifier;
//        typeSpecifier = WordTypeSpecifier::parse(source, scope);
//        if (typeSpecifier.valid())
//            return typeSpecifier;
//        return 0;
//    }
//};
//
//template<class T> class TypeIdentifierTemplate;
//typedef TypeIdentifierTemplate<void> TypeIdentifier;
//template<class T> class TypeIdentifierTemplate : public TypeSpecifier
//{
//public:
//    static Reference<TypeIdentifier> parse(CharacterSource* source, Scope* scope)
//    {
//        CharacterSource s = *source;
//        DiagnosticLocation location = s.location();
//        int start = s.offset();
//        int c = s.get();
//        if (c < 'A' || c > 'Z')
//            return 0;
//        CharacterSource s2;
//        do {
//            s2 = s;
//            c = s.get();
//            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')
//                continue;
//            break;
//        } while (true);
//        int end = s2.offset();
//        Space::parse(&s2);
//        String name = s2.subString(start, end);
//        static String keywords[] = {
//            String("Auto"),
//            String("Bit"),
//            String("Boolean"),
//            String("Byte"),
//            String("Character"),
//            String("Class"),
//            String("Complex"),
//            String("DInt"),
//            String("DUInt"),
//            String("DWord"),
//            String("Fixed"),
//            String("Float"),
//            String("HInt"),
//            String("HUInt"),
//            String("HWord"),
//            String("Int"),
//            String("Integer"),
//            String("QInt"),
//            String("QUInt"),
//            String("QWord"),
//            String("Rational"),
//            String("String"),
//            String("TypeOf"),
//            String("UInt"),
//            String("Unsigned"),
//            String("Word"),
//            String("WordString")
//        };
//        for (int i = 0; i < sizeof(keywords)/sizeof(keywords[0]); ++i)
//            if (name == keywords[i])
//                return 0;
//        *source = s2;
//        return new TypeIdentifier(scope, name, location);
//    }
//
//    String name() const { return _name; }
//    Symbol type()
//    {
//        if (!_type.valid()) {
//            if (_resolving) {
//                static String error("A type cannot be defined in terms of itself");
//                _location.throwError(error);
//            }
//            _resolving = true;
//            _type = _scope->resolveType(_name, _location)->type();
//            _resolving = false;
//        }
//        return _type;
//    }
//private:
//    TypeIdentifierTemplate(Scope* scope, String name, DiagnosticLocation location)
//      : _scope(scope), _name(name), _location(location), _resolving(false)
//    { }
//    Scope* _scope;
//    String _name;
//    DiagnosticLocation _location;
//    bool _resolving;
//    Symbol _type;
//};
//
//class PointerTypeSpecifier : public TypeSpecifier
//{
//public:
//    PointerTypeSpecifier(Reference<TypeSpecifier> referentTypeSpecifier)
//      : _referentTypeSpecifier(referentTypeSpecifier)
//    { }
//    Symbol type()
//    {
//        if (!_type.valid())
//            _type = Symbol(atomPointer, _referentTypeSpecifier->type());
//        return _type;
//    }
//private:
//    Reference<TypeSpecifier> _referentTypeSpecifier;
//    Symbol _type;
//};
//
//class TypeListSpecifier : public ReferenceCounted
//{
//public:
//    static Reference<TypeListSpecifier> parse(CharacterSource* source, Scope* scope)
//    {
//        Stack<Reference<TypeSpecifier> > stack;
//        do {
//            Reference<TypeSpecifier> typeSpecifier = TypeSpecifier::parse(source, scope);
//            if (!typeSpecifier.valid()) {
//                static String error("Type specifier expected");
//                source->location().throwError(error);
//            }
//            stack.push(typeSpecifier);
//            if (Space::parseCharacter(source, ','))
//                continue;
//            break;
//        } while (true);
//        return new TypeListSpecifier(&stack);
//    }
//    TypeList typeList()
//    {
//        if (!_typeList.valid()) {
//            for (int i = 0; i < _array.count(); ++i)
//                _typeList.push(_array[i]->type());
//            _typeList.finalize();
//        }
//        return _typeList;
//    }
//private:
//    TypeListSpecifier(Stack<Reference<TypeSpecifier> >* stack)
//    {
//        stack->toArray(&_array);
//    }
//    Array<Reference<TypeSpecifier> > _array;
//    TypeList _typeList;
//};
//
//class FunctionTypeSpecifier : public TypeSpecifier
//{
//public:
//    FunctionTypeSpecifier(Reference<TypeSpecifier> returnTypeSpecifier, Reference<TypeListSpecifier> argumentTypeListSpecifier)
//      : _returnTypeSpecifier(returnTypeSpecifier), _argumentTypeListSpecifier(argumentTypeListSpecifier)
//    { }
//    Type type()
//    {
//        if (!_type.valid())
//            _type = FunctionType(_returnTypeSpecifier->type(), _argumentTypeListSpecifier->typeList());
//        return _type;
//    }
//private:
//    Reference<TypeSpecifier> _returnTypeSpecifier;
//    Reference<TypeListSpecifier> _argumentTypeListSpecifier;
//    Type _type;
//};
//
//class AutoTypeSpecifier : public TypeSpecifier
//{
//public:
//    static Reference<AutoTypeSpecifier> parse(CharacterSource* source, Scope* scope)
//    {
//        static String keyword("Auto");
//        if (Space::parseKeyword(source, keyword))
//            return new AutoTypeSpecifier;
//        return 0;
//    }
//    Type type() { return VoidType(); }
//};
//
//class BitTypeSpecifier : public TypeSpecifier
//{
//public:
//    static Reference<BitTypeSpecifier> parse(CharacterSource* source, Scope* scope)
//    {
//        static String keyword("Bit");
//        if (Space::parseKeyword(source, keyword))
//            return new BitTypeSpecifier;
//        return 0;
//    }
//    Type type() { return BitType(); }
//};
//
//class BooleanTypeSpecifier : public TypeSpecifier
//{
//public:
//    static Reference<BooleanTypeSpecifier> parse(CharacterSource* source, Scope* scope)
//    {
//        static String keyword("Boolean");
//        if (Space::parseKeyword(source, keyword))
//            return new BooleanTypeSpecifier;
//        return 0;
//    }
//    Type type() { return BooleanType(); }
//};
//
//class ByteTypeSpecifier : public TypeSpecifier
//{
//public:
//    static Reference<ByteTypeSpecifier> parse(CharacterSource* source, Scope* scope)
//    {
//        static String keyword("Byte");
//        if (Space::parseKeyword(source, keyword))
//            return new ByteTypeSpecifier;
//        return 0;
//    }
//    Type type() { return ByteType(); }
//};
//
//class CharacterTypeSpecifier : public TypeSpecifier
//{
//public:
//    static Reference<CharacterTypeSpecifier> parse(CharacterSource* source, Scope* scope)
//    {
//        static String keyword("Character");
//        if (Space::parseKeyword(source, keyword))
//            return new CharacterTypeSpecifier;
//        return 0;
//    }
//    Type type() { return CharacterType(); }
//};
//
//class IntTypeSpecifier : public TypeSpecifier
//{
//public:
//    static Reference<IntTypeSpecifier> parse(CharacterSource* source, Scope* scope)
//    {
//        static String keyword("Int");
//        if (Space::parseKeyword(source, keyword))
//            return new IntTypeSpecifier;
//        return 0;
//    }
//    Type type() { return IntType(); }
//};
//
//class StringTypeSpecifier : public TypeSpecifier
//{
//public:
//    static Reference<StringTypeSpecifier> parse(CharacterSource* source, Scope* scope)
//    {
//        static String keyword("String");
//        if (Space::parseKeyword(source, keyword))
//            return new StringTypeSpecifier;
//        return 0;
//    }
//    Type type() { return StringType(); }
//};
//
//class UIntTypeSpecifier : public TypeSpecifier
//{
//public:
//    static Reference<UIntTypeSpecifier> parse(CharacterSource* source, Scope* scope)
//    {
//        static String keyword("UInt");
//        if (Space::parseKeyword(source, keyword))
//            return new UIntTypeSpecifier;
//        return 0;
//    }
//    Type type() { return UIntType(); }
//};
//
//class VoidTypeSpecifier : public TypeSpecifier
//{
//public:
//    static Reference<VoidTypeSpecifier> parse(CharacterSource* source, Scope* scope)
//    {
//        static String keyword("Void");
//        if (Space::parseKeyword(source, keyword))
//            return new VoidTypeSpecifier;
//        return 0;
//    }
//    Type type() { return VoidType(); }
//};
//
//class WordTypeSpecifier : public TypeSpecifier
//{
//public:
//    static Reference<WordTypeSpecifier> parse(CharacterSource* source, Scope* scope)
//    {
//        static String keyword("Word");
//        if (Space::parseKeyword(source, keyword))
//            return new WordTypeSpecifier;
//        return 0;
//    }
//    Type type() { return WordType(); }
//};
//
//class ClassTypeSpecifier : public TypeSpecifier
//{
//public:
//    static Reference<ClassTypeSpecifier> parse(CharacterSource* source, Scope* scope)
//    {
//        static String keyword("Class");
//        if (!Space::parseKeyword(source, keyword))
//            return 0;
//        return new ClassTypeSpecifier;
//    }
//    Type type() { return ClassType(); }
//};

#include "Expression.cpp"

//class TypeOfTypeSpecifier : public TypeSpecifier
//{
//public:
//    static Reference<TypeOfTypeSpecifier> parse(CharacterSource* source, Scope* scope)
//    {
//        static String keyword("TypeOf");
//        if (!Space::parseKeyword(source, keyword))
//            return 0;
//        Space::assertCharacter(source, '(');
//        Reference<Expression> expression = Expression::parse(source, scope);
//        if (!expression.valid()) {
//            static String error("Expression expected");
//            source->location().throwError(error);
//        }
//        Space::assertCharacter(source, ')');
//        return new TypeOfTypeSpecifier(expression);
//    }
//    Type type() { return _expression->type(); }
//private:
//    TypeOfTypeSpecifier(Reference<Expression> expression)
//      : _expression(expression)
//    { }
//    Reference<Expression> _expression;
//};
