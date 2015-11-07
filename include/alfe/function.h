#include "alfe/main.h"

#ifndef INCLUDED_FUNCTION_H
#define INCLUDED_FUNCTION_H

#include "alfe/type.h"
#include "alfe/identifier.h"

class Funco : public Handle
{
protected:
    class Body : public Handle::Body
    {
    public:
        virtual FunctionTyco tyco() const = 0;
        virtual Value evaluate(List<Value> arguments, Span span) const = 0;
        virtual Identifier identifier() const = 0;
        virtual bool argumentsMatch(List<Type> argumentTypes) const = 0;
        virtual bool betterThan(Funco other) const { return false; }
        virtual String toString() const { return tyco().toString(); }
        // = 0;  // TODO: write properly once we need best-match overloading
    };
    Funco(Body* body) : Handle(body) { }
    const Body* body() const { return as<Body>(); }
public:
    Funco() { }
    Value evaluate(List<Value> arguments, Span span) const
    {
        return body()->evaluate(arguments, span);
    }
    Identifier identifier() const { return body()->identifier(); }
    bool argumentsMatch(List<Type> argumentTypes) const
    {
        return body()->argumentsMatch(argumentTypes);
    }
    bool betterThan(Funco other) const { return body()->betterThan(other); }
    String toString() const { return body()->toString(); }
};

class Function : public Funco
{
protected:
    class Body : public Funco::Body
    {
    public:
        bool argumentsMatch(List<Type> argumentTypes) const
        {
            return tyco().argumentsMatch(argumentTypes.begin());
        }
    };                                                    
    Function(Body* body) : Funco(body) { }
    const Body* body() const { return as<Body>(); }
};

class OverloadedFunctionSet : public Handle
{
    class Body : public Handle::Body
    {
    public:
        Body(Identifier identifier) : _identifier(identifier) { }
        void add(Funco funco) { _funcos.add(funco); }
        Value evaluate(List<Value> arguments, Span span) const
        {
            List<::Type> argumentTypes;
            for (auto i : arguments)
                argumentTypes.add(i.type());
            Funco bestCandidate;
            for (auto f : _funcos) {
                if (!f.argumentsMatch(argumentTypes))
                    continue;
                if (bestCandidate.valid()) {
                    if (bestCandidate.betterThan(f))
                        continue;
                    if (!f.betterThan(bestCandidate)) {
                        String s = "Ambiguous function call of " +
                            _identifier.name() + " with argument types " +
                            argumentTypesString(argumentTypes) +
                            ". Could be " + bestCandidate.toString() +
                            " or " + f.toString() + ".";
                        span.throwError(s);
                    }
                }
                bestCandidate = f;
            }
            if (!bestCandidate.valid()) {
                String s = "No matches for function " +
                    _identifier.name() + " with argument types ";
                bool needComma = false;
                for (auto t : argumentTypes) {
                    if (needComma)
                        s += ", ";
                    needComma = true;
                    s += t.toString();
                }
                s += ".";
                span.throwError(s);
            }
            return bestCandidate.evaluate(arguments, span);
        }
    private:
        String argumentTypesString(List<::Type> argumentTypes) const
        {
            String s;
            bool needComma = false;
            for (auto t : argumentTypes) {
                if (needComma)
                    s += ", ";
                needComma = true;
                s += t.toString();
            }
            return s;
        }

        List<Funco> _funcos;
        Identifier _identifier;
    };
    Body* body() { return as<Body>(); }
    const Body* body() const { return as<Body>(); }
public:
    class Type : public NamedNullary<::Type, Type>
    {
    public:
        static String name() { return "OverloadedFunctionSet::Type"; }
    };

    OverloadedFunctionSet(Identifier identifier)
      : Handle(new Body(identifier)) { }
    void add(Funco funco) { body()->add(funco); }
    static Type type() { return Type(); }
    Value evaluate(List<Value> arguments, Span span) const
    {
        return body()->evaluate(arguments, span);
    }
};

template<> Nullary<Type, OverloadedFunctionSet::Type>
    Nullary<Type, OverloadedFunctionSet::Type>::_instance;

#endif // INCLUDED_FUNCTION_H
