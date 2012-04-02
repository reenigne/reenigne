class TypeConstructorSpecifier
{
public:
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
    TypeConstructorSpecifier(const Implementation* implementation)
      : _implementation(implementation) { }
private:
    ConstReference<Implementation> _implementation;
};

class TypeConstructorIdentifier : public TypeConstructorSpecifier
{
public:
    static TypeConstructorIdentifier parse(CharacterSource* source)
    {
        CharacterSource s = *source;
        Location location = s.location();
        int start = s.offset();
        int c = s.get();
        if (c < 'A' || c > 'Z')
            return TypeConstructorIdentifier();
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
                return TypeConstructorIdentifier();
        *source = s2;
        return TypeConstructorIdentifier(name, Span(location, endLocation));
    }
    bool valid() const { return !_name.empty(); }
    Span span() const { return _span; }
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
    TypeConstructorIdentifier() { }
    TypeConstructorIdentifier(const String& name, const Span& span)
      : _name(name), _span(span) { }

    String _name;
    Span _span;
};

class ClassTypeSpecifier
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
    ClassTypeSpecifier() { } 
    ClassTypeSpecifier(const Span& span) : _span(span) { }

    Span _span;
};

//Symbol parseTypeOfTypeSpecifier(CharacterSource* source);
//
//Symbol parseTypeConstructorSpecifier(CharacterSource* source);

class FundamentalTypeConstructorSpecifier
{
public:
    static FundamentalTypeConstructorSpecifier parse(CharacterSource* source)
    {
        CharacterSource s2 = *source;
        TypeConstructorIdentifier typeSpecifier =
            TypeConstructorIdentifier::parse(&s2);
        if (typeSpecifier.valid()) {
            String s = typeSpecifier.name();
            Span span = typeSpecifier.span();
            List<TypeConstructorSpecifier> templateArguments;
            while (Space::parseCharacter(&s2, '<', &span)) {
                do {
                    TypeConstructorSpecifier templateArgument =
                        TypeConstructorSpecifier::parse(&s2);
                    if (!templateArgument.valid())
                        return Symbol();
                    templateArguments.add(templateArgument);
                } while (Space::parseCharacter(&s2, ',', &span));
                if (!Space::parseCharacter(&s2, '>', &span))
                    return Symbol();
            }
            return Symbol(atomTypeConstructorIdentifier, typeSpecifier,
                SymbolArray(templateArguments),
                newSpan(spanOf(typeSpecifier) + span));
        }
        typeSpecifier = parseClassTypeSpecifier(source);
        if (typeSpecifier.valid())
            return typeSpecifier;
        typeSpecifier = parseTypeOfTypeSpecifier(source);
        if (typeSpecifier.valid())
            return typeSpecifier;
        return Symbol();
    }
private:

};

SymbolArray parseTypeConstructorSpecifierList(CharacterSource* source)
{
    SymbolList list;
    Symbol typeSpecifier = parseTypeConstructorSpecifier(source);
    if (!typeSpecifier.valid())
        return list;
    list.add(typeSpecifier);
    Span span;
    while (Space::parseCharacter(source, ',', &span)) {
        typeSpecifier = parseTypeConstructorSpecifier(source);
        if (!typeSpecifier.valid())
            source->location().throwError("Type specifier expected");
        list.add(typeSpecifier);
    }
    return list;
}

//TypeConstructorSpecifier :=
//    TypeConstructorIdentifier ("<" TypeConstructorSpecifier \ "," ">")*
//  | TypeConstructorSpecifier "*"
//  | TypeConstructorSpecifier "(" [(TypeConstructorSpecifier [Identifier] \ ","] ")"
//  | "Class" "{" ClassDefinition "}"
//  | "TypeOf" "(" Expression ")"
Symbol parseTypeConstructorSpecifier(CharacterSource* source)
{
    Symbol typeSpecifier = parseFundamentalTypeConstructorSpecifier(source);
    if (!typeSpecifier.valid())
        return Symbol();
    do {
        Span span;
        if (Space::parseCharacter(source, '*', &span)) {
            typeSpecifier = Symbol(atomPointer, typeSpecifier,
                new TypeCache(spanOf(typeSpecifier) + span, 4, 4));
            continue;
        }
        if (Space::parseCharacter(source, '(', &span)) {
            SymbolArray typeListSpecifier = parseTypeConstructorSpecifierList(source);
            Space::assertCharacter(source, ')', &span);
            typeSpecifier = Symbol(atomFunction, typeSpecifier,
                typeListSpecifier, new TypeCache(spanOf(typeSpecifier) + span, 0, 0));
            continue;
        }
    } while (true);
    return typeSpecifier;
}

Symbol parseExpressionOrFail(CharacterSource* source);

Symbol parseTypeOfTypeSpecifier(CharacterSource* source)
{
    Span span;
    static String keyword("TypeOf");
    if (!Space::parseKeyword(source, keyword, &span))
        return Symbol();
    Span span2;
    Space::assertCharacter(source, '(', &span2);
    Symbol expression = parseExpressionOrFail(source);
    Space::assertCharacter(source, ')', &span2);
    return Symbol(atomTypeOf, expression, newSpan(span + span2));
}

//KindSpecifier := ("<" [([TypeConstructorIdentifier] KindSpecifier) \ ","] ">")*
Symbol parseKindSpecifier(CharacterSource* source)
{
    Span span;
    if (!Space::parseCharacter(source, '<', &span))
        return Symbol(atomTypeKind, newSpan(Span(source->location(), source->location())));
    SymbolList kindSpecifierList;
    Span span2;
    do {
        // A type constructor identifier is allowed here for documentation
        // purposes only - it isn't used for anything, so we immediately throw
        // it away. It doesn't even need to be resolved.
        do {
            parseTypeConstructorIdentifier(source);
            kindSpecifierList.add(parseKindSpecifier(source));
        } while (Space::parseCharacter(source, ',', &span2));
        Space::assertCharacter(source, '>', &span2);
    } while (Space::parseCharacter(source, '<', &span2));
    return Symbol(atomTemplateKind, SymbolArray(kindSpecifierList),
        newSpan(span + span2));
}

//SpecializedTypeConstructorSpecifier :=
//    TypeConstructorSpecifier
//  | "@" TypeConstructorIdentifier
//  | SpecializedTypeConstructorSpecifier "*"
//  | SpecializedTypeConstructorSpecifier "(" [SpecializedTypeConstructorSpecifier \ ","] ")"
Symbol parseFundamentalSpecializedTypeConstructorSpecifier(CharacterSource* source)
{
    Symbol specializedTypeConstructorSpecifier = parseTypeConstructorSpecifier(source);
    if (specializedTypeConstructorSpecifier.valid())
        return specializedTypeConstructorSpecifier;
    Span span;
    if (!Space::parseCharacter(source, '@', &span))
        source->location().throwError(
            "Expected @ or type constructor specifier");
    Symbol typeConstructorIdentifier = parseTypeConstructorIdentifier(source);
    if (!typeConstructorIdentifier.valid())
        source->location().throwError("Expected type constructor identifier");
    return Symbol(atomTemplateParameter, typeConstructorIdentifier,
        newSpan(span + spanOf(typeConstructorIdentifier)));
}

Symbol parseSpecializedTypeConstructorSpecifier(CharacterSource* source);

SymbolArray parseSpecializedTypeConstructorSpecifierList(CharacterSource* source)
{
    SymbolList list;
    Symbol typeSpecifier = parseSpecializedTypeConstructorSpecifier(source);
    if (!typeSpecifier.valid())
        return list;
    list.add(typeSpecifier);
    Span span;
    while (Space::parseCharacter(source, ',', &span)) {
        typeSpecifier = parseSpecializedTypeConstructorSpecifier(source);
        if (!typeSpecifier.valid())
            source->location().throwError(
                "(Specialized) type specifier expected");
        list.add(typeSpecifier);
    }
    return list;
}

Symbol parseSpecializedTypeConstructorSpecifier(CharacterSource* source)
{
    Symbol typeSpecifier = parseFundamentalTypeConstructorSpecifier(source);
    if (!typeSpecifier.valid())
        return Symbol();
    do {
        Span span;
        if (Space::parseCharacter(source, '*', &span)) {
            typeSpecifier = Symbol(atomPointer, typeSpecifier,
                new TypeCache(spanOf(typeSpecifier) + span, 4, 4));
            continue;
        }
        if (Space::parseCharacter(source, '(', &span)) {
            SymbolArray typeListSpecifier =
                parseSpecializedTypeConstructorSpecifierList(source);
            Space::assertCharacter(source, ')', &span);
            typeSpecifier = Symbol(atomFunction, typeSpecifier,
                typeListSpecifier, new TypeCache(spanOf(typeSpecifier) + span, 0, 0));
            continue;
        }
    } while (true);
    return typeSpecifier;
}

//TemplateParameter :=
//    "@" TypeConstructorIdentifier KindSpecifier
//  | SpecializedTypeConstructorSpecifier
Symbol parseTemplateParameter(CharacterSource* source)
{
    Span span;
    if (Space::parseCharacter(source, '@', &span)) {
        Symbol typeConstructorIdentifier = parseTypeConstructorIdentifier(source);
        if (!typeConstructorIdentifier.valid())
            source->location().throwError(
                "Expected type constructor identifier");
        Symbol kindSpecifier = parseKindSpecifier(source);
        return Symbol(atomTemplateParameter, typeConstructorIdentifier,
            kindSpecifier, newSpan(span + spanOf(kindSpecifier)));
    }
    return parseSpecializedTypeConstructorSpecifier(source);
}

//TypeConstructorSignifier :=
//    TypeConstructorIdentifier ("<" TemplateParameter \ "*" ">")*
Symbol parseTypeConstructorSignifier(CharacterSource* source)
{
    CharacterSource s2 = *source;
    Symbol typeConstructorIdentifier = parseTypeConstructorIdentifier(source);
    if (!typeConstructorIdentifier.valid())
        return Symbol();
    SymbolList templateParameters;
    Span span;
    while (Space::parseCharacter(&s2, '<', &span)) {
        do {
            Symbol templateParameter = parseTemplateParameter(&s2);
            if (!templateParameter.valid())
                return Symbol();
            templateParameters.add(templateParameter);
        } while (Space::parseCharacter(&s2, ',', &span));
        if (!Space::parseCharacter(&s2, '>', &span))
            return Symbol();
    }
    return Symbol(atomTypeConstructorSignifier, typeConstructorIdentifier,
        SymbolArray(templateParameters),
        newSpan(spanOf(typeConstructorIdentifier) + span));
}

String typeToString(Symbol type)
{
    switch (type.atom()) {
        case atomFunction:
            {
                String s = typeToString(type[1].symbol()) + "(";
                SymbolArray array = type[2].array();
                bool hasArguments = false;
                for (int i = 0; i < array.count(); ++i) {
                    if (hasArguments)
                        s += ", ";
                    s += typeToString(array[i]);
                    hasArguments = true;
                }
                return s + ")";
            }
        case atomPointer:
            return typeToString(type[1].symbol()) + "*";
        default:
            return atomToString(type.atom());
    }
}

String typesToString(SymbolArray array)
{
    String s("(");
    for (int i = 0; i < array.count(); ++i) {
        if (i != 0)
            s += ", ";
        s += typeToString(array[i]);
    }
    return s + ")";
}
