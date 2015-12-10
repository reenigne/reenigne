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
        virtual int compareTo(Funco other) const
        {
            FunctionTyco f = tyco();
            FunctionTyco b = other.tyco();
            int r = 0;
            while (!f.isNullary()) {
                Type fType = f.lastArgumentType();
                Type bType = b.lastArgumentType();
                if (!fType.canConvertTo(bType))
                    r |= 2;
                if (!bType.canConvertTo(fType))
                    r |= 1;
            }
            assert(b.isNullary());
            return r;
        }
        virtual String toString() const { return tyco().toString(); }
    };
    const Body* body() const { return as<Body>(); }
public:
    Funco() { }
    Funco(const Handle& other) : Handle(other) { }
    Value evaluate(List<Value> arguments, Span span) const
    {
        return body()->evaluate(arguments, span);
    }
    Identifier identifier() const { return body()->identifier(); }
    bool argumentsMatch(List<Type> argumentTypes) const
    {
        return body()->argumentsMatch(argumentTypes);
    }
    int compareTo(Funco other) const { return body()->compareTo(other); }
    String toString() const { return body()->toString(); }
    FunctionTyco tyco() const { return body()->tyco(); }
};

class Function : public Funco
{
public:
    Function(const Handle& other) : Funco(other) { }
protected:
    class Body : public Funco::Body
    {
    public:
        bool argumentsMatch(List<Type> argumentTypes) const
        {
            return tyco().argumentsMatch(argumentTypes.begin());
        }
    };
};

template<class T> class OverloadedFunctionSetT : public Handle
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
            List<Funco> bestCandidates;
            for (auto f : _funcos) {
                if (!f.argumentsMatch(argumentTypes))
                    continue;
                List<Funco> newBestCandidates;
                bool newBest = true;
                for (auto b : bestCandidates) {
                    int r = f.compareTo(b);
                    if (r == 2) {
                        // b better than f
                        newBest = false;
                        break;
                    }
                    if (r != 1)
                        newBestCandidates.add(b);
                }
                if (newBest) {
                    bestCandidates = newBestCandidates;
                    bestCandidates.add(f);
                }
            }
            for (auto f : bestCandidates) {
                for (auto b : bestCandidates) {
                    int r = f.compareTo(b);
                    if (r == 3) {
                        span.throwError("Ambiguous function call of " +
                            _identifier.name() + " with argument types " +
                            argumentTypesString(argumentTypes) +
                            ". Could be " + b.toString() + " or " +
                            f.toString() + ".");
                    }
                }
            }
            if (bestCandidates.count() == 0) {
                span.throwError("No matches for function " +
                    _identifier.name() + " with argument types " +
                    argumentTypesString(argumentTypes) + ".");
            }
            // We have a choice of possible funcos here. Logically they should
            // be equivalent, but some may be more optimal. For now we'll just
            // choose the first one, but later we may want to try to figure out
            // which one is most optimal.
            auto i = bestCandidates.begin();
            return i->evaluate(arguments, span);
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

    OverloadedFunctionSetT(Identifier identifier)
      : Handle(create<Body>(identifier)) { }
    void add(Funco funco) { body()->add(funco); }
    static Type type() { return Type(); }
    Value evaluate(List<Value> arguments, Span span) const
    {
        return body()->evaluate(arguments, span);
    }
};

#endif // INCLUDED_FUNCTION_H
