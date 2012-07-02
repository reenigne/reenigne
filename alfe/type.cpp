template<class T> class ExpressionTemplate;
typedef ExpressionTemplate<void> Expression;

template<class T> class TypeConstructorSpecifierTemplate;
typedef TypeConstructorSpecifierTemplate<void> TypeConstructorSpecifier;

template<class T> class TypeSpecifierTemplate;
typedef TypeSpecifierTemplate<void> TypeSpecifier;

template<class T> class TypeIdentifierTemplate;
typedef TypeIdentifierTemplate<void> TypeIdentifier;

//TypeConstructorSpecifier :=
//    TypeConstructorIdentifier ("<" TypeConstructorSpecifier \ "," ">")*
//  | TypeConstructorSpecifier "*"
//  | TypeConstructorSpecifier "(" 
//    [(TypeConstructorSpecifier [Identifier] \ ","] ")"
//  | "Class" "{" ClassDefinition "}"
//  | "TypeOf" "(" Expression ")"
template<class T> class TypeConstructorSpecifierTemplate
{
public:
    static TypeConstructorSpecifier parse(CharacterSource* source)
    {
        TypeConstructorSpecifier typeSpecifier = parseFundamental(source);
        if (!typeSpecifier.valid())
            return TypeConstructorSpecifier();
        do {
            Span span;
            if (Space::parseCharacter(source, '*', &span)) {
                typeSpecifier = TypeSpecifier(
                    new TypeSpecifier::PointerImplementation(typeSpecifier,
                        typeSpecifier.span() + span));
                continue;
            }
            if (Space::parseCharacter(source, '(', &span)) {

                List<TypeConstructorSpecifier> typeListSpecifier =
                    parseList(source);
                Space::assertCharacter(source, ')', &span);
                typeSpecifier = TypeSpecifier(
                    new TypeSpecifier::FunctionImplementation(typeSpecifier,
                        typeListSpecifier, typeSpecifier.span() + span));
                continue;
            }
        } while (true);
        return typeSpecifier;
    }

    bool valid() const { return _implementation.valid(); }
    Span span() const { return _implementation->span(); }
protected:
    class Implementation : public ReferenceCounted
    {
    public:
        Implementation(const Span& span) : _span(span) { }
        Span span() const { return _span; }
    private:
        Span _span;
    };
    class InstantiationImplementation : public Implementation
    {
    public:
        InstantiationImplementation(const TypeIdentifier& typeIdentifier,
            const List<TypeConstructorSpecifier>& argumentTypeSpecifiers,
            const Span& span)
          : Implementation(span), _typeIdentifier(typeIdentifier),
            _argumentTypeSpecifiers(argumentTypeSpecifiers)
        { }
    private:
        TypeIdentifier _typeIdentifier;
        Array<TypeConstructorSpecifier> _argumentTypeSpecifiers;
    };

    TypeConstructorSpecifierTemplate() { }
    TypeConstructorSpecifierTemplate(const Implementation* implementation)
      : _implementation(implementation) { }
    template<class T> const T* implementation()
    {
        return _implementation.referent<T>();
    }
private:
    TypeConstructorSpecifier parseFundamental(CharacterSource* source)
    {
        CharacterSource s2 = *source;
        TypeIdentifier typeIdentifier = TypeConstructorIdentifier::parse(&s2);
        if (typeIdentifier.valid()) {
            String s = typeIdentifier.name();
            Span span = typeIdentifier.span();
            List<TypeConstructorSpecifier> templateArguments;
            while (Space::parseCharacter(&s2, '<', &span)) {
                do {
                    TypeConstructorSpecifier templateArgument = parse(&s2);
                    if (!templateArgument.valid())
                        return TypeConstructorSpecifier();
                    templateArguments.add(templateArgument);
                } while (Space::parseCharacter(&s2, ',', &span));
                if (!Space::parseCharacter(&s2, '>', &span))
                    return TypeConstructorSpecifier();
            }
            *source = s2;
            return new InstantiationImplementation(typeIdentifier,
                    templateArguments, typeIdentifier.span() + span);
        }
        TypeSpecifier typeSpecifier = ClassTypeSpecifier::parse(source);
        if (typeSpecifier.valid())
            return typeSpecifier;
        typeSpecifier = TypeOfTypeSpecifier::parse(source);
        if (typeSpecifier.valid())
            return typeSpecifier;
        return TypeConstructorSpecifier();
    }
    List<TypeConstructorSpecifier> parseList(CharacterSource* source)
    {
        List<TypeConstructorSpecifier> list;
        TypeConstructorSpecifier argument = parse(source);
        if (!argument.valid())
            return list;
        list.add(argument);
        Span span;
        while (Space::parseCharacter(source, ',', &span)) {
            argument = parse(source);
            if (!argument.valid())
                source->location().throwError("Type specifier expected");
            list.add(argument);
        }
        return list;
    }
    ConstReference<Implementation> _implementation;
};

template<class T> class TypeSpecifierTemplate : public TypeConstructorSpecifier
{
private:
    class PointerImplementation
      : public TypeConstructorSpecifier::Implementation
    {
    public:
        PointerImplementation(const TypeConstructorSpecifier& referent,
            const Span& span)
          : Implementation(span), _referent(referent) { }
    private:
        TypeConstructorSpecifier _referent;
    };
    class FunctionImplementation
      : public TypeConstructorSpecifier::Implementation
    {
    public:
        FunctionImplementation(
            const TypeConstructorSpecifier& returnTypeSpecifier,
            const List<TypeConstructorSpecifier>& argumentTypeSpecifiers,
            const Span& span)
          : Implementation(span), _returnType(returnType),
            _argumentTypeSpecifiers(argumentTypeSpecifiers)
        { }
    private:
        TypeConstructorSpecifier _returnTypeSpecifier;
        Array<TypeConstructorSpecifier> _argumentTypeSpecifiers;
    };

    template<class T> friend class TypeConstructorSpecifierTemplate;
};

template<class T> class TypeIdentifierTemplate
  : public TypeConstructorSpecifier
{
public:
    static TypeIdentifier parse(CharacterSource* source)
    {
        CharacterSource s = *source;
        Location location = s.location();
        int start = s.offset();
        int c = s.get();
        if (c < 'A' || c > 'Z')
            return TypeIdentifier();
        CharacterSource s2;
        do {
            s2 = s;
            c = s.get();
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c == '_')
                continue;
            break;
        } while (true);
        int end = s2.offset();
        Location endLocation = s2.location();
        Space::parse(&s2);
        String name = s2.subString(start, end);
        static String keywords[] = {
            "Class",
            "Complex",
            "DInt",
            "DUInt",
            "DWord",
            "Fixed",
            "Float",
            "HInt",
            "HUInt",
            "HWord",
            "Integer",
            "QInt",
            "QUInt",
            "QWord",
            "Rational",
            "TypeOf",
            "Unsigned",
            "WordString"};
        for (int i = 0; i < sizeof(keywords)/sizeof(keywords[0]); ++i)
            if (name == keywords[i])
                return TypeIdentifier();
        *source = s2;
        return TypeIdentifier(name, Span(location, endLocation));
    }
    String name() const { return implementation<Implementation  >()->name(); }
private:
    class Implementation : public TypeConstructorSpecifier::Implementation
    {
    public:
        Implementation(const String& name, const Span& span)
          : TypeConstructorSpecifier::Implementation(span), _name(name) { }
        String name() const { return _name; }
    private:
        String _name;
    };
    TypeIdentifierTemplate() { }
    TypeIdentifierTemplate(const String& name, const Span& span)
      : TypeConstructorSpecifier(new Implementation(name, span)) { }
};

class ClassTypeSpecifier : public TypeConstructorSpecifier
{
public:
    static ClassTypeSpecifier parse(CharacterSource* source)
    {
        Location start = source->location();
        Span span;
        if (!Space::parseKeyword(source, "Class", &span))
            return ClassTypeSpecifier();
        Span span2;
        Space::assertCharacter(source, '{', &span2);
        // TODO: Parse class contents
        Space::assertCharacter(source, '}', &span2);
        return ClassTypeSpecifier(span + span2);
    }
private:
    class Implementation : public TypeConstructorSpecifier::Implementation
    {
    public:
        Implementation(const Span& span)
          : TypeConstructorSpecifier::Implementation(span) { }
    };
    ClassTypeSpecifier() { } 
    ClassTypeSpecifier(const Span& span)
      : TypeConstructorSpecifier(new Implementation(span)) { }
};

//Symbol parseTypeOfTypeSpecifier(CharacterSource* source);
//
//Symbol parseTypeConstructorSpecifier(CharacterSource* source);

Expression parseExpressionOrFail(CharacterSource* source);

template<class T> class TypeOfTypeSpecifierTemplate;
typedef TypeOfTypeSpecifierTemplate<void> TypeOfTypeSpecifier;

template<class T> class TypeOfTypeSpecifierTemplate : public TypeSpecifier
{
public:
    TypeOfTypeSpecifier parse(CharacterSource* source)
    {
        Span span;
        if (!Space::parseKeyword(source, "TypeOf", &span))
            return TypeOfTypeSpecifier();
        Span span2;
        Space::assertCharacter(source, '(', &span2);
        Expression expression = parseExpressionOrFail(source);
        Space::assertCharacter(source, ')', &span2);
        return TypeOfTypeSpecifier(expression, span + span2);
    }
private:
    class Implementation : public TypeSpecifier::Implementation
    {
    public:
        Implementation(const Expression& expression, const Span& span)
          : TypeSpecifier::Implementation(span), _expression(expression) { }
    private:
        Expression _expression;
    };
    TypeOfTypeSpecifierTemplate() { }
    TypeOfTypeSpecifierTemplate(const Expression& expression,
        const Span& span)
      : TypeSpecifier(new Implementation(expression, span)) { }
};

// KindSpecifier :=
//     ("<" [([TypeConstructorIdentifier] KindSpecifier) \ ","] ">")*
class KindSpecifier
{
public:
    static KindSpecifier parse(CharacterSource* source)
    {
        Span span;
        if (!Space::parseCharacter(source, '<', &span))
            return KindSpecifier(Kind::type,
                Span(source->location(), source->location()));
        Span span2;
        Kind templateKind = parseTemplate(source, &span2);
        return KindSpecifier(templateKind, span + span2);
    }
private:
    static TemplateKind parseTemplate(CharacterSource* source, Span* span)
    {
        // A type identifier is allowed here for documentation purposes only -
        // it isn't used for anything, so we immediately throw it away. It
        // doesn't even need to be resolved.
        TypeIdentifier::parse(source);

        KindSpecifier left = parse(source);
        if (Space::parseCharacter(source, '>', span)) {
            if (!Space::parseCharacter(source, '<', span))
                return TemplateKind(left._kind, Kind::type);
        }
        else
            Space::assertCharacter(source, ',', span);
        return TemplateKind(left._kind, parseTemplate(source, span));
    }

    Kind _kind;
    Span _span;

    KindSpecifier(const Kind& kind, const Span& span)
      : _kind(kind), _span(span) { }
};


////SpecializedTypeConstructorSpecifier :=
////    TypeConstructorSpecifier
////  | "@" TypeConstructorIdentifier
////  | SpecializedTypeConstructorSpecifier "*"
////  | SpecializedTypeConstructorSpecifier "(" [SpecializedTypeConstructorSpecifier \ ","] ")"
//Symbol parseFundamentalSpecializedTypeConstructorSpecifier(CharacterSource* source)
//{
//    Symbol specializedTypeConstructorSpecifier = parseTypeConstructorSpecifier(source);
//    if (specializedTypeConstructorSpecifier.valid())
//        return specializedTypeConstructorSpecifier;
//    Span span;
//    if (!Space::parseCharacter(source, '@', &span))
//        source->location().throwError(
//            "Expected @ or type constructor specifier");
//    Symbol typeConstructorIdentifier = parseTypeConstructorIdentifier(source);
//    if (!typeConstructorIdentifier.valid())
//        source->location().throwError("Expected type constructor identifier");
//    return Symbol(atomTemplateParameter, typeConstructorIdentifier,
//        newSpan(span + spanOf(typeConstructorIdentifier)));
//}
//
//Symbol parseSpecializedTypeConstructorSpecifier(CharacterSource* source);
//
//SymbolArray parseSpecializedTypeConstructorSpecifierList(CharacterSource* source)
//{
//    SymbolList list;
//    Symbol typeSpecifier = parseSpecializedTypeConstructorSpecifier(source);
//    if (!typeSpecifier.valid())
//        return list;
//    list.add(typeSpecifier);
//    Span span;
//    while (Space::parseCharacter(source, ',', &span)) {
//        typeSpecifier = parseSpecializedTypeConstructorSpecifier(source);
//        if (!typeSpecifier.valid())
//            source->location().throwError(
//                "(Specialized) type specifier expected");
//        list.add(typeSpecifier);
//    }
//    return list;
//}
//
//Symbol parseSpecializedTypeConstructorSpecifier(CharacterSource* source)
//{
//    Symbol typeSpecifier = parseFundamentalTypeConstructorSpecifier(source);
//    if (!typeSpecifier.valid())
//        return Symbol();
//    do {
//        Span span;
//        if (Space::parseCharacter(source, '*', &span)) {
//            typeSpecifier = Symbol(atomPointer, typeSpecifier,
//                new TypeCache(spanOf(typeSpecifier) + span, 4, 4));
//            continue;
//        }
//        if (Space::parseCharacter(source, '(', &span)) {
//            SymbolArray typeListSpecifier =
//                parseSpecializedTypeConstructorSpecifierList(source);
//            Space::assertCharacter(source, ')', &span);
//            typeSpecifier = Symbol(atomFunction, typeSpecifier,
//                typeListSpecifier, new TypeCache(spanOf(typeSpecifier) + span, 0, 0));
//            continue;
//        }
//    } while (true);
//    return typeSpecifier;
//}
//
////TemplateParameter :=
////    "@" TypeConstructorIdentifier KindSpecifier
////  | SpecializedTypeConstructorSpecifier
//Symbol parseTemplateParameter(CharacterSource* source)
//{
//    Span span;
//    if (Space::parseCharacter(source, '@', &span)) {
//        Symbol typeConstructorIdentifier = parseTypeConstructorIdentifier(source);
//        if (!typeConstructorIdentifier.valid())
//            source->location().throwError(
//                "Expected type constructor identifier");
//        Symbol kindSpecifier = parseKindSpecifier(source);
//        return Symbol(atomTemplateParameter, typeConstructorIdentifier,
//            kindSpecifier, newSpan(span + spanOf(kindSpecifier)));
//    }
//    return parseSpecializedTypeConstructorSpecifier(source);
//}
//
////TypeConstructorSignifier :=
////    TypeConstructorIdentifier ("<" TemplateParameter \ "*" ">")*
//Symbol parseTypeConstructorSignifier(CharacterSource* source)
//{
//    CharacterSource s2 = *source;
//    Symbol typeConstructorIdentifier = parseTypeConstructorIdentifier(source);
//    if (!typeConstructorIdentifier.valid())
//        return Symbol();
//    SymbolList templateParameters;
//    Span span;
//    while (Space::parseCharacter(&s2, '<', &span)) {
//        do {
//            Symbol templateParameter = parseTemplateParameter(&s2);
//            if (!templateParameter.valid())
//                return Symbol();
//            templateParameters.add(templateParameter);
//        } while (Space::parseCharacter(&s2, ',', &span));
//        if (!Space::parseCharacter(&s2, '>', &span))
//            return Symbol();
//    }
//    return Symbol(atomTypeConstructorSignifier, typeConstructorIdentifier,
//        SymbolArray(templateParameters),
//        newSpan(spanOf(typeConstructorIdentifier) + span));
//}
//
//String typeToString(Symbol type)
//{
//    switch (type.atom()) {
//        case atomFunction:
//            {
//                String s = typeToString(type[1].symbol()) + "(";
//                SymbolArray array = type[2].array();
//                bool hasArguments = false;
//                for (int i = 0; i < array.count(); ++i) {
//                    if (hasArguments)
//                        s += ", ";
//                    s += typeToString(array[i]);
//                    hasArguments = true;
//                }
//                return s + ")";
//            }
//        case atomPointer:
//            return typeToString(type[1].symbol()) + "*";
//        default:
//            return atomToString(type.atom());
//    }
//}
//
//String typesToString(SymbolArray array)
//{
//    String s("(");
//    for (int i = 0; i < array.count(); ++i) {
//        if (i != 0)
//            s += ", ";
//        s += typeToString(array[i]);
//    }
//    return s + ")";
//}
