#include "alfe/main.h"

#ifndef INCLUDED_FUNCTION_H
#define INCLUDED_FUNCTION_H

#include "alfe/type.h"
#include "alfe/identifier.h"
#include "alfe/concrete.h"

// This is not a real type - we can't do anything with it. A funco does not
// in general have a type (or even a tyco, because we can't do kind checking).
// However, OverloadedFunctionSet needs to be wrapped in a Value and placed in
// a symbol table (e.g. in ConfigFile), so this is the stub type for that.
class FuncoType : public NamedNullary<Type, FuncoType>
{
public:
    static String name() { return "@FunctionConstructor"; }
};

class Funco : public Handle
{
protected:
    class Body : public Handle::Body
    {
    public:
        virtual FunctionType type() const { return FuncoType(); };
        virtual Value evaluate(List<Value> arguments, Span span) const = 0;
        virtual Identifier identifier() const = 0;
        virtual bool argumentsMatch(List<Type> argumentTypes) const = 0;
        virtual int compareTo(Funco other) const
        {
            List<Tyco> fTycos = parameterTycos();
            List<Tyco> bTycos = other.parameterTycos();
            assert(fTycos.count() == bTycos.count());
            auto bIterator = bTycos.begin();
            int r = 0;
            for (auto fTyco : fTycos) {
                Tyco bTyco = *bIterator;
                Type fType = fTyco;
                Type bType = bTyco;
                if (fType.valid() && bType.valid()) {
                    if (!fType.canConvertTo(bType))
                        r |= 2;
                    if (!bType.canConvertTo(fType))
                        r |= 1;
                }
                else {
                    // This is enough for Berapa's built-in concrete functions,
                    // but eventually we'll want to generalize this.
                    if (bTyco == ConcreteTyco() &&
                        (fTyco == IntegerType() || fTyco == RationalType()))
                        r |= 1;
                    if (fTyco == ConcreteTyco() &&
                        (bTyco == IntegerType() || bTyco == RationalType()))
                        r |= 2;
                }
                ++bIterator;
            }
            return r;
        }
        // For the convenience of FunctionType, parameterTycos() returns a list
        // of tycos in right-to-left order.
        virtual List<Tyco> parameterTycos() const = 0;
        virtual String toString() const { return type().toString(); }
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
    FunctionType type() const { return body()->type(); }
    List<Tyco> parameterTycos() const { return body()->parameterTycos(); }
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
            return type().argumentsMatch(argumentTypes.begin());
        }
        List<Tyco> parameterTycos() const
        {
            List<Tyco> tycos;
            FunctionType t = type();
            while (!t.isNullary()) {
                tycos.add(t.lastArgumentType());
                t = t.parent();
            }
            return tycos;
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
    OverloadedFunctionSetT(Identifier identifier)
      : Handle(create<Body>(identifier)) { }
    void add(Funco funco) { body()->add(funco); }
    static Type type() { return FuncoType(); }
    Value evaluate(List<Value> arguments, Span span) const
    {
        return body()->evaluate(arguments, span);
    }
};

#endif // INCLUDED_FUNCTION_H
