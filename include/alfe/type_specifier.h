#include "alfe/main.h"

#ifndef INCLUDED_TYPE_SPECIFIER_H
#define INCLUDED_TYPE_SPECIFIER_H

#include "alfe/parse_tree_object.h"

template<class T> class ExpressionT;
typedef ExpressionT<void> Expression;

template<class T> class IdentifierT;
typedef IdentifierT<void> Identifier;

template<class T> class TycoSpecifierT;
typedef TycoSpecifierT<void> TycoSpecifier;

template<class T> class TypeSpecifierT;
typedef TypeSpecifierT<void> TypeSpecifier;

template<class T> class TycoIdentifierT;
typedef TycoIdentifierT<void> TycoIdentifier;

template<class T> class TemplateArgumentsT;
typedef TemplateArgumentsT<void> TemplateArguments;

template<class T> class ClassTycoSpecifierT;
typedef ClassTycoSpecifierT<void> ClassTycoSpecifier;

template<class T> class TypeOfTypeSpecifierT;
typedef TypeOfTypeSpecifierT<void> TypeOfTypeSpecifier;

template<class T> class TypeParameterT;
typedef TypeParameterT<void> TypeParameter;

template<class T> class TycoT;
typedef TycoT<void> Tyco;

template<class T> class ScopeT;
typedef ScopeT<void> Scope;

//TycoSpecifier :=
//    TycoIdentifier ("<" TycoSpecifier \ "," ">")*
//  | TycoSpecifier "*"
//  | TycoSpecifier "("
//    [(TycoSpecifier [Identifier] \ ","] ")"
//  | "Class" "{" ClassDefinition "}"
//  | "TypeOf" "(" Expression ")"
template<class T> class TycoSpecifierT : public ParseTreeObject
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
                tycoSpecifier = create<typename
                     TypeSpecifierT<T>::PointerBody>(
                    tycoSpecifier, tycoSpecifier.span() + span);
                continue;
            }
            CharacterSource s2 = *source;
            if (Space::parseCharacter(&s2, '(', &span)) {
                List<TycoSpecifier> typeListSpecifier = parseList(&s2);
                if (!Space::parseCharacter(&s2, ')', &span))
                    return tycoSpecifier;
                *source = s2;
                tycoSpecifier = create<typename
                    TypeSpecifierT<T>::FunctionBody>(
                    tycoSpecifier, typeListSpecifier,
                    tycoSpecifier.span() + span);
                continue;
            }
            break;
        } while (true);
        return tycoSpecifier;
    }
    TycoT<T> resolve(const Scope* scope) const
    {
        return body()->resolve(scope);
    }
    String toString() const { return body()->toString(); }
    TycoSpecifierT() { }
protected:
    TycoSpecifierT(Handle other) : ParseTreeObject(other) { }

    class Body : public ParseTreeObject::Body
    {
    public:
        Body(const Span& span) : ParseTreeObject::Body(span) { }
        virtual Tyco resolve(const Scope* scope) const = 0;
        virtual String toString() const = 0;
    };
    class InstantiationBody : public Body
    {
    public:
        InstantiationBody(const TycoIdentifier& tycoIdentifier,
            const TemplateArguments& arguments, const Span& span)
          : Body(span), _tycoIdentifier(tycoIdentifier), _arguments(arguments)
        { }
        TycoT<T> resolve(const Scope* scope) const
        {
            TycoT<T> t = scope->resolveTycoIdentifier(_tycoIdentifier);
            List<TycoT<T>> a = _arguments.resolve(scope);
            for (auto argument : a) {
                Template te = t;
                assert(te.valid());
                t = te.instantiate(argument);
            }
            return t;
        }
        String toString() const
        {
            return _tycoIdentifier.toString() + "<" + _arguments.toString() +
                ">";
        }
    private:
        TycoIdentifierT<T> _tycoIdentifier;
        TemplateArgumentsT<T> _arguments;
    };

    const Body* body() const { return as<Body>(); }
private:
    static TycoSpecifier parseFundamental(CharacterSource* source)
    {
        TycoIdentifierT<T> tycoIdentifier =
            TycoIdentifierT<T>::parse(source);
        if (tycoIdentifier.valid()) {
            String s = tycoIdentifier.name();
            Span span = tycoIdentifier.span();
            TemplateArgumentsT<T> arguments =
                TemplateArgumentsT<T>::parse(source);
            if (arguments.valid()) {
                return create<InstantiationBody>(tycoIdentifier, arguments,
                    span + arguments.span());
            }
            return tycoIdentifier;
        }
        TycoSpecifier tycoSpecifier =
            ClassTycoSpecifierT<T>::parse(source);
        if (tycoSpecifier.valid())
            return tycoSpecifier;
        tycoSpecifier = TypeOfTypeSpecifierT<T>::parse(source);
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

template<class T> class TemplateArgumentsT : public ParseTreeObject
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
        return create<Body>(arguments, span + span2);
    }
    int count() const
    {
        const Body* body = to<Body>();
        if (body != 0)
            return body->count();
        return 0;
    }
    List<Tyco> resolve(const Scope* scope) const
    {
        return body()->resolve(scope);
    }
    String toString() const { return body()->toString(); }

    class Body : public ParseTreeObject::Body
    {
    public:
        Body(const List<TycoSpecifier>& arguments, const Span& span)
          : ParseTreeObject::Body(span), _arguments(arguments) { }
        int count() const { return _arguments.count(); }
        List<Tyco> resolve(const Scope* scope) const
        {
            List<Tyco> l;
            for (auto s : _arguments)
                l.add(scope->resolveTycoSpecifier(s));
            return l;
        }
        String toString() const
        {
            bool first = true;
            String r;
            for (auto s : _arguments) {
                if (!first)
                    r += ", ";
                first = false;
                r += s.toString();
            }
            return r;
        }
    private:
        List<TycoSpecifier> _arguments;
    };
private:
    const Body* body() const { return as<Body>(); }

    TemplateArgumentsT() { }
    TemplateArgumentsT(Handle other) : ParseTreeObject(other) { }
};

template<class T> class TypeSpecifierT : public TycoSpecifier
{
public:
    TypeSpecifierT() { }
    TypeSpecifierT(Handle other) : TycoSpecifier(other) { }

private:
    class PointerBody : public TycoSpecifier::Body
    {
    public:
        PointerBody(const TycoSpecifier& referent, const Span& span)
          : Body(span), _referent(referent) { }
        TycoT<T> resolve(const Scope* scope) const
        {
            Type referent = scope->resolveTycoSpecifier(_referent);
            return PointerType(referent);
        }
        String toString() const { return _referent.toString() + "*"; }
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
        TycoT<T> resolve(const Scope* scope) const
        {
            Type ret = scope->resolveType(_returnType);
            FunctionType f = FunctionTemplate().instantiate(ret);
            for (auto a : _argumentTypes) {
                Type t = scope->resolveType(a);
                f = f.instantiate(t);
            }
            return f;
        }
        String toString() const
        {
            String r = _returnType.toString() + "(";
            bool first = true;
            for (auto s : _argumentTypes) {
                if (!first)
                    r += ", ";
                first = false;
                r += s.toString();
            }
            return r + ")";
        }
    private:
        TycoSpecifier _returnType;
        List<TycoSpecifier> _argumentTypes;
    };

    template<class U> friend class TycoSpecifierT;
};

template<class T> class TycoIdentifierT : public TycoSpecifier
{
public:
    TycoIdentifierT() { }
    TycoIdentifierT(const String& name)
      : TycoSpecifier(create<Body>(name, Span())) { }
    TycoIdentifierT(const TycoSpecifier& tycoSpecifier)
      : TycoSpecifier(to<Body>(tycoSpecifier)) { }
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
        return create<Body>(name, Span(location, endLocation));
    }
    String name() const { return as<Body>()->name(); }
    TycoIdentifierT(Handle other) : TycoSpecifier(other) { }
protected:
    class Body : public TycoSpecifier::Body
    {
    public:
        Body(const String& name, const Span& span)
          : TycoSpecifier::Body(span), _name(name) { }
        String name() const { return _name; }
        Hash hash() const
        {
            return TycoSpecifier::Body::hash().mixin(_name.hash());
        }
        bool equals(const ConstHandle::Body* other) const
        {
            auto o = other->to<Body>();
            return o != 0 && _name == o->_name;
        }
        TycoT<T> resolve(const Scope* scope) const
        {
            return scope->resolveTycoIdentifier(handle<TycoIdentifier>());
        }
        String toString() const { return _name; }
    private:
        String _name;
    };
};

template<class T> class ClassTycoSpecifierT : public TycoSpecifier
{
public:
    ClassTycoSpecifierT() { }
    ClassTycoSpecifierT(Handle other) : TycoSpecifier(other) { }
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
        return create<Body>(span + span2);
    }
private:
    class Body : public TycoSpecifier::Body
    {
    public:
        Body(const Span& span) : TycoSpecifier::Body(span) { }
        TycoT<T> resolve(const Scope* scope) const
        {
            // TODO
            return TycoT<T>();
        }
        String toString() const
        {
            // TODO
            return String();
        }
    };
};

template<class T> class TypeOfTypeSpecifierT : public TypeSpecifier
{
public:
    TypeOfTypeSpecifierT() { }
    TypeOfTypeSpecifierT(Handle other) : TypeSpecifier(other) { }
    static TypeOfTypeSpecifier parse(CharacterSource* source)
    {
        Span span;
        if (!Space::parseKeyword(source, "TypeOf", &span))
            return TypeOfTypeSpecifier();
        Span span2;
        Space::assertCharacter(source, '(', &span2);
        ExpressionT<T> expression =
            ExpressionT<T>::parseOrFail(source);
        Space::assertCharacter(source, ')', &span2);
        return create<Body>(expression, span + span2);
    }
private:
    class Body : public TypeSpecifier::Body
    {
    public:
        Body(const Expression& expression, const Span& span)
          : TypeSpecifier::Body(span), _expression(expression) { }
        TycoT<T> resolve(const Scope* scope) const
        {
            return _expression.type();
        }
        String toString() const { return _expression.toString(); }
    private:
        ExpressionT<T> _expression;
    };
};

template<class T> class TemplateParametersT;
typedef TemplateParametersT<void> TemplateParameters;

template<class T> class TemplateParameterT;
typedef TemplateParameterT<void> TemplateParameter;

//TemplateParameter =
//    TycoSpecifier
//  | "@" TycoIdentifier TemplateParameters
//  | TemplateParameter "*"
//  | TemplateParameter "(" TemplateParameter \ "," ")"
//  ;
template<class T> class TemplateParameterT : public ParseTreeObject
{
public:
    TemplateParameterT() { }
    TemplateParameterT(Handle other) : ParseTreeObject(other) { }
    static TemplateParameter parse(CharacterSource* source)
    {
        TemplateParameter parameter = parseFundamental(source);
        if (!parameter.valid())
            return parameter;
        do {
            Span span;
            if (Space::parseCharacter(source, '*', &span)) {
                parameter = create<typename
                    TypeParameterT<T>::PointerBody>(
                    parameter, parameter.span() + span);
                continue;
            }
            if (Space::parseCharacter(source, '(', &span)) {
                List<TemplateParameter> parameters = parseList(source);
                Space::assertCharacter(source, ')', &span);
                parameter = create<typename
                    TypeParameterT<T>::FunctionBody>(
                    parameter, parameters, parameter.span() + span);
                continue;
            }
        } while (true);
        return parameter;
    }
protected:
    class Body : public ParseTreeObject::Body
    {
    public:
        Body(const Span& span) : ParseTreeObject::Body(span) { }
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
            return create<TycoSpecifierBody>(tycoSpecifier);
        Span span;
        if (Space::parseCharacter(source, '@', &span)) {
            TycoIdentifier tycoIdentifier = TycoIdentifier::parse(source);
            if (!tycoIdentifier.valid())
                source->location().throwError(
                    "Expected type constructor identifier");
            TemplateParametersT<T> parameters =
                TemplateParametersT<T>::parse(source);
            return create<BoundVariableBody>(tycoIdentifier, parameters,
                span + parameters.span());
        }
        source->location().throwError("Expected template parameter");
        // Not reachable
        return TemplateParameter();
    }

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
        TemplateParametersT<T> _parameters;
    };
};

template<class T> class TypeParameterT : public TemplateParameter
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
template<class T> class TemplateParametersT : public ParseTreeObject
{
public:
    TemplateParametersT() { }
    TemplateParametersT(Handle other) : ParseTreeObject(other) { }
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
        return create<Body>(parameters, span + span2);
    }
private:
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
      : ParseTreeObject(create<Body>(identifier, parameters, Span())) { }
    static TycoSignifier parse(CharacterSource* source)
    {
        CharacterSource s2 = *source;
        TycoIdentifier identifier = TycoIdentifier::parse(source);
        if (!identifier.valid())
            return TycoSignifier();
        TemplateParameters parameters = TemplateParameters::parse(source);
        return create<Body>(identifier, parameters,
            identifier.span() + parameters.span());
    }
private:
    TycoSignifier() { }
    TycoSignifier(Handle other) : ParseTreeObject(other) { }

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

#endif // INCLUDED_TYPE_SPECIFIER_H
