#include "alfe/main.h"

#ifndef INCLUDED_TYPE_SPECIFIER_H
#define INCLUDED_TYPE_SPECIFIER_H

#include "alfe/parse_tree_object.h"

template<class T> class ExpressionTemplate;
typedef ExpressionTemplate<void> Expression;

template<class T> class IdentifierTemplate;
typedef IdentifierTemplate<void> Identifier;

template<class T> class TycoSpecifierTemplate;
typedef TycoSpecifierTemplate<void> TycoSpecifier;

template<class T> class TypeSpecifierTemplate;
typedef TypeSpecifierTemplate<void> TypeSpecifier;

template<class T> class TycoIdentifierTemplate;
typedef TycoIdentifierTemplate<void> TycoIdentifier;

template<class T> class TemplateArgumentsTemplate;
typedef TemplateArgumentsTemplate<void> TemplateArguments;

template<class T> class ClassTycoSpecifierTemplate;
typedef ClassTycoSpecifierTemplate<void> ClassTycoSpecifier;

template<class T> class TypeOfTypeSpecifierTemplate;
typedef TypeOfTypeSpecifierTemplate<void> TypeOfTypeSpecifier;

template<class T> class TypeParameterTemplate;
typedef TypeParameterTemplate<void> TypeParameter;

//TycoSpecifier :=
//    TycoIdentifier ("<" TycoSpecifier \ "," ">")*
//  | TycoSpecifier "*"
//  | TycoSpecifier "("
//    [(TycoSpecifier [Identifier] \ ","] ")"
//  | "Class" "{" ClassDefinition "}"
//  | "TypeOf" "(" Expression ")"
template<class T> class TycoSpecifierTemplate : public ParseTreeObject
{
public:
    static TycoSpecifier parse(CharacterSource* source)
    {
        TycoSpecifier tycoSpecifier = parseFundamental(source);
        if (!tycoSpecifier.valid())
            return TycoSpecifier();
        do {
            Span span;
            if (Space::parseCharacter(source, '*', &span)) {
                tycoSpecifier = new typename TypeSpecifierTemplate<T>::
                    PointerBody(tycoSpecifier,
                    tycoSpecifier.span() + span);
                continue;
            }
            CharacterSource s2 = *source;
            if (Space::parseCharacter(&s2, '(', &span)) {
                List<TycoSpecifier> typeListSpecifier = parseList(&s2);
                if (!Space::parseCharacter(&s2, ')', &span))
                    return tycoSpecifier;
                *source = s2;
                tycoSpecifier = new typename TypeSpecifierTemplate<T>::
                    FunctionBody(tycoSpecifier,
                    typeListSpecifier, tycoSpecifier.span() + span);
                continue;
            }
            break;
        } while (true);
        return tycoSpecifier;
    }
protected:
    TycoSpecifierTemplate() { }
    TycoSpecifierTemplate(const Body* body) : ParseTreeObject(body) { }

    class Body : public ParseTreeObject::Body
    {
    public:
        Body(const Span& span) : ParseTreeObject::Body(span) { }
    };
    class InstantiationBody : public Body
    {
    public:
        InstantiationBody(const TycoIdentifier& tycoIdentifier,
            const TemplateArguments& arguments, const Span& span)
          : Body(span), _tycoIdentifier(tycoIdentifier), _arguments(arguments)
        { }
    private:
        TycoIdentifierTemplate<T> _tycoIdentifier;
        TemplateArgumentsTemplate<T> _arguments;
    };

    const Body* body() { return as<Body>(); }
private:
    static TycoSpecifier parseFundamental(CharacterSource* source)
    {
        TycoIdentifierTemplate<T> tycoIdentifier =
            TycoIdentifierTemplate<T>::parse(source);
        if (tycoIdentifier.valid()) {
            String s = tycoIdentifier.name();
            Span span = tycoIdentifier.span();
            TemplateArgumentsTemplate<T> arguments =
                TemplateArgumentsTemplate<T>::parse(source);
            if (arguments.valid())
                return new InstantiationBody(tycoIdentifier,
                    arguments, span + arguments.span());
            return tycoIdentifier;
        }
        TycoSpecifier tycoSpecifier =
            ClassTycoSpecifierTemplate<T>::parse(source);
        if (tycoSpecifier.valid())
            return tycoSpecifier;
        tycoSpecifier = TypeOfTypeSpecifierTemplate<T>::parse(source);
        if (tycoSpecifier.valid())
            return tycoSpecifier;
        return TycoSpecifier();
    }
    static List<TycoSpecifier> parseList(CharacterSource* source)
    {
        List<TycoSpecifier> list;
        TycoSpecifier argument = parse(source);
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
};

template<class T> class TemplateArgumentsTemplate : public ParseTreeObject
{
public:
    static TemplateArguments parse(CharacterSource* source)
    {
        CharacterSource s2 = *source;
        List<TycoSpecifier> arguments;
        Span span;
        Span span2;
        while (Space::parseCharacter(&s2, '<', &span2)) {
            span += span2;
            do {
                TycoSpecifier argument = TycoSpecifier::parse(&s2);
                if (!argument.valid())
                    return TemplateArguments();
                arguments.add(argument);
            } while (Space::parseCharacter(&s2, ',', &span2));
            if (!Space::parseCharacter(&s2, '>', &span2))
                return TemplateArguments();
        }
        *source = s2;
        if (arguments.count() == 0)
            return TemplateArguments();
        return new Body(arguments, span + span2);
    }
    int count() const
    {
        const Body* body = as<Body>();
        if (body != 0)
            return body->count();
        return 0;
    }

    class Body : public ParseTreeObject::Body
    {
    public:
        Body(const List<TycoSpecifier>& arguments, const Span& span)
          : ParseTreeObject::Body(span), _arguments(arguments) { }
        int count() const { return _arguments.count(); }
    private:
        List<TycoSpecifier> _arguments;
    };
private:
    TemplateArgumentsTemplate() { }
    TemplateArgumentsTemplate(const Body* body)
        : ParseTreeObject(body) { }
};

template<class T> class TypeSpecifierTemplate : public TycoSpecifier
{
public:
    TypeSpecifierTemplate() { }
    TypeSpecifierTemplate(const Body* body)
      : TycoSpecifier(body) { }

private:
    class PointerBody : public TycoSpecifier::Body
    {
    public:
        PointerBody(const TycoSpecifier& referent, const Span& span)
          : Body(span), _referent(referent) { }
    private:
        TycoSpecifier _referent;
    };
    class FunctionBody : public TycoSpecifier::Body
    {
    public:
        FunctionBody(const TycoSpecifier& returnType,
            const List<TycoSpecifier>& argumentTypes, const Span& span)
          : Body(span), _returnType(returnType),
            _argumentTypes(argumentTypes)
        { }
    private:
        TycoSpecifier _returnType;
        List<TycoSpecifier> _argumentTypes;
    };

    template<class U> friend class TycoSpecifierTemplate;
};

template<class T> class TycoIdentifierTemplate : public TycoSpecifier
{
public:
    TycoIdentifierTemplate(const String& name)
      : TycoSpecifier(new Body(name, Span())) { }
    TycoIdentifierTemplate(const TycoSpecifier& t) : TycoSpecifier(t) { }
    static TycoIdentifier parse(CharacterSource* source)
    {
        CharacterSource s = *source;
        Location location = s.location();
        int start = s.offset();
        int c = s.get();
        if (c < 'A' || c > 'Z')
            return TycoIdentifier();
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
//            "Complex",
//            "DInt",
//            "DUInt",
//            "DWord",
//            "Fixed",
//            "Float",
//            "HInt",
//            "HUInt",
//            "HWord",
//            "Integer",
//            "QInt",
//            "QUInt",
//            "QWord",
//            "Rational",
            "TypeOf"
//            "Unsigned",
//            "WordString"
        };
        for (int i = 0; i < sizeof(keywords)/sizeof(keywords[0]); ++i)
            if (name == keywords[i])
                return TycoIdentifier();
        *source = s2;
        return new Body(name, Span(location, endLocation));
    }
    int hash() const { return name().hash(); }
    bool operator==(const TycoIdentifier& other) const
    {
        return name() == other.name();
    }

    class Body : public TycoSpecifier::Body
    {
    public:
        Body(const String& name, const Span& span)
          : TycoSpecifier::Body(span), _name(name) { }
        String name() const { return _name; }
    private:
        String _name;
    };
    String name() const { return as<Body>()->name(); }

    TycoIdentifierTemplate() { }
private:
    TycoIdentifierTemplate(const Body* body) : TycoSpecifier(body) { }
};

template<class T> class ClassTycoSpecifierTemplate : public TycoSpecifier
{
public:
    static ClassTycoSpecifier parse(CharacterSource* source)
    {
        Location start = source->location();
        Span span;
        if (!Space::parseKeyword(source, "Class", &span))
            return ClassTycoSpecifier();
        Span span2;
        Space::assertCharacter(source, '{', &span2);
        // TODO: Parse class contents
        Space::assertCharacter(source, '}', &span2);
        return ClassTycoSpecifier(span + span2);
    }
private:
    class Body : public TycoSpecifier::Body
    {
    public:
        Body(const Span& span)
          : TycoSpecifier::Body(span) { }
    };
    ClassTycoSpecifierTemplate() { }
    ClassTycoSpecifierTemplate(const Span& span)
      : TycoSpecifier(new Body(span)) { }
};

template<class T> class TypeOfTypeSpecifierTemplate : public TypeSpecifier
{
public:
    static TypeOfTypeSpecifier parse(CharacterSource* source)
    {
        Span span;
        if (!Space::parseKeyword(source, "TypeOf", &span))
            return TypeOfTypeSpecifier();
        Span span2;
        Space::assertCharacter(source, '(', &span2);
        Expression expression = Expression::parseOrFail(source);
        Space::assertCharacter(source, ')', &span2);
        return new Body(expression, span + span2);
    }
    TypeOfTypeSpecifierTemplate() { }
    TypeOfTypeSpecifierTemplate(const Body* body)
      : TypeSpecifier(body) { }
private:
    class Body : public TypeSpecifier::Body
    {
    public:
        Body(const Expression& expression, const Span& span)
          : TypeSpecifier::Body(span), _expression(expression) { }
    private:
        Expression _expression;
    };
};

template<class T> class TemplateParametersTemplate;
typedef TemplateParametersTemplate<void> TemplateParameters;

template<class T> class TemplateParameterTemplate;
typedef TemplateParameterTemplate<void> TemplateParameter;

//TemplateParameter =
//    TycoSpecifier
//  | "@" TycoIdentifier TemplateParameters
//  | TemplateParameter "*"
//  | TemplateParameter "(" TemplateParameter \ "," ")"
//  ;
template<class T> class TemplateParameterTemplate : public ParseTreeObject
{
public:
    static TemplateParameter parse(CharacterSource* source)
    {
        TemplateParameter parameter = parseFundamental(source);
        if (!parameter.valid())
            return parameter;
        do {
            Span span;
            if (Space::parseCharacter(source, '*', &span)) {
                parameter = new typename TypeParameterTemplate<T>::
                    PointerBody(parameter, parameter.span() + span);
                continue;
            }
            if (Space::parseCharacter(source, '(', &span)) {
                List<TemplateParameter> parameters = parseList(source);
                Space::assertCharacter(source, ')', &span);
                parameter = new typename TypeParameterTemplate<T>::
                    FunctionBody(parameter, parameters,
                    parameter.span() + span);
                continue;
            }
        } while (true);
        return parameter;
    }
protected:
    class Body : public ParseTreeObject::Body
    {
    public:
        Body(const Span& span)
          : ParseTreeObject::Body(span) { }
    };
private:
    static List<TemplateParameter> parseList(CharacterSource* source)
    {
        List<TemplateParameter> list;
        TemplateParameter parameter = parse(source);
        if (!parameter.valid())
            return list;
        list.add(parameter);
        Span span;
        while (Space::parseCharacter(source, ',', &span)) {
            parameter = parse(source);
            if (!parameter.valid())
                source->location().throwError("Template parameter "
                    "expected");
            list.add(parameter);
        }
        return list;
    }
    static TemplateParameter parseFundamental(CharacterSource* source)
    {
        TycoSpecifier tycoSpecifier = TycoSpecifier::parse(source);
        if (tycoSpecifier.valid())
            return new TycoSpecifierBody(tycoSpecifier);
        Span span;
        if (Space::parseCharacter(source, '@', &span)) {
            TycoIdentifier tycoIdentifier = TycoIdentifier::parse(source);
            if (!tycoIdentifier.valid())
                source->location().throwError(
                    "Expected type constructor identifier");
            TemplateParametersTemplate<T> parameters =
                TemplateParametersTemplate<T>::parse(source);
            return new BoundVariableBody(tycoIdentifier,
                parameters, span + parameters.span());
        }
        source->location().throwError("Expected template parameter");
        // Not reachable
        return 0;
    }

    TemplateParameterTemplate(const Body* body)
      : ParseTreeObject(body) { }

    class TycoSpecifierBody : public Body
    {
    public:
        TycoSpecifierBody(const TycoSpecifier& tycoSpecifier)
            : Body(tycoSpecifier.span()),
            _tycoSpecifier(tycoSpecifier) { }
    private:
        TycoSpecifier _tycoSpecifier;
    };
    class BoundVariableBody : public Body
    {
    public:
        BoundVariableBody(const TycoIdentifier& tycoIdentifier,
            const TemplateParameters& parameters, const Span& span)
          : Body(span), _tycoIdentifier(tycoIdentifier),
            _parameters(parameters) { }
    private:
        TycoIdentifier _tycoIdentifier;
        TemplateParametersTemplate<T> _parameters;
    };
};

template<class T> class TypeParameterTemplate : public TemplateParameter
{
public:
    class PointerBody : public TemplateParameter::Body
    {
    public:
        PointerBody(const TemplateParameter& parameter,
            const Span& span)
          : Body(span), _parameter(parameter) { }
    private:
        TemplateParameter _parameter;
    };
    class FunctionBody : public TemplateParameter::Body
    {
    public:
        FunctionBody(const TemplateParameter& parameter,
            const List<TemplateParameter>& parameters, const Span& span)
          : Body(span), _parameter(parameter),
            _parameters(parameters) { }
    private:
        TemplateParameter _parameter;
        List<TemplateParameter> _parameters;
    };
};

// TemplateParameters = ("<" TemplateParameter \ "," ">")*;
template<class T> class TemplateParametersTemplate : public ParseTreeObject
{
public:
    TemplateParametersTemplate() { }
    static TemplateParameters parse(CharacterSource* source)
    {
        CharacterSource s2 = *source;
        List<TemplateParameter> parameters;
        Span span, span2;
        while (Space::parseCharacter(&s2, '<', &span)) {
            do {
                TemplateParameter parameter = TemplateParameter::parse(&s2);
                if (!parameter.valid())
                    return TemplateParameters();
                parameters.add(parameter);
            } while (Space::parseCharacter(&s2, ',', &span2));
            if (!Space::parseCharacter(&s2, '>', &span2))
                return TemplateParameters();
        }
        *source = s2;
        return new Body(parameters, span + span2);
    }
private:
    TemplateParametersTemplate(const Body* body)
      : ParseTreeObject(body) { }

    class Body : public ParseTreeObject::Body
    {
    public:
        Body(const List<TemplateParameter>& parameters,
            const Span& span)
          : ParseTreeObject::Body(span), _parameters(parameters) { }
    private:
        List<TemplateParameter> _parameters;
    };
};

//TycoSignifier := TycoIdentifier TemplateParameters
class TycoSignifier : public ParseTreeObject
{
public:
    TycoSignifier(const TycoIdentifier& identifier,
        const TemplateParameters& parameters = TemplateParameters())
      : ParseTreeObject(new Body(identifier, parameters, Span())) { }
    static TycoSignifier parse(CharacterSource* source)
    {
        CharacterSource s2 = *source;
        TycoIdentifier identifier = TycoIdentifier::parse(source);
        if (!identifier.valid())
            return TycoSignifier();
        TemplateParameters parameters = TemplateParameters::parse(source);
        return new Body(identifier, parameters,
            identifier.span() + parameters.span());
    }
private:
    TycoSignifier() { }
    TycoSignifier(const Body* body)
      : ParseTreeObject(body) { }

    class Body : public ParseTreeObject::Body
    {
    public:
        Body(const TycoIdentifier& identifier,
            const TemplateParameters& parameters, const Span& span)
          : ParseTreeObject::Body(span), _identifier(identifier),
            _parameters(parameters) { }
    private:
        TycoIdentifier _identifier;
        TemplateParameters _parameters;
    };
};

class BuiltInTycoSpecifier : public TycoSpecifier
{
public:
    BuiltInTycoSpecifier(const Kind& kind)
      : TycoSpecifier(new Body(kind)) { }
private:
    class Body : public TycoSpecifier::Body
    {
    public:
        Body(const Kind& kind)
          : TycoSpecifier::Body(Span()), _kind(kind) { }
    private:
        Kind _kind;
    };
};

#endif // INCLUDED_TYPE_SPECIFIER_H
