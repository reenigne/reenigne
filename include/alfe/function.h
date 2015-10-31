#include "alfe/main.h"

#ifndef INCLUDED_FUNCTION_H
#define INCLUDED_FUNCTION_H

#include "alfe/type.h"
#include "alfe/identifier.h"

class Funco : public Handle
{
};

class Function : public Funco
{
public:
    Function(Body* body) : Funco(body) { }
    TypedValue evaluate(List<TypedValue> arguments) const
    {
        return body()->evaluate(arguments);
    }
    Identifier identifier() const { return body()->identifier(); }
    TypedValue typedValue() const { return body()->typedValue(); }
protected:
    class Body : public Handle::Body
    {
    public:
        virtual TypedValue evaluate(List<TypedValue> arguments) const = 0;
        virtual Identifier identifier() const = 0;
        virtual TypedValue typedValue() const = 0;
    };
    const Body* body() const { return as<Body>(); }
};

class OverloadFunction : public Funco
{
public:
    //OverloadFunction(Identifier identifier)
    //  : Function(new Body(identifier)) { }
//    void add(Function function) { body()->add(function); }
//private:
//    class Key : public ConstHandle
//    {
//    public:
//        Key(Function function) : ConstHandle(new FunctionBody(function)) { }
//        Key(List<TypedValue> arguments)
//          : ConstHandle(new ArgumentsBody(arguments)) { }
//    private:
//        class Body : public ConstHandle::Body
//        {
//        public:
//            virtual bool equals(const Body* other) const = 0;
//        };
//        class FunctionBody : public Body
//        {
//        public:
//            FunctionBody(Function function) : _function(function) { }
//            //Hash hash() const { return Body::hash().mixin(_function.hash()); }
//            bool equals(const Body* other) const
//            {
//                auto o = other->as<ArgumentsBody>();
//                if (o != 0)
//                    return FunctionTyco(_function.typedValue().type()).
//                        argumentsMatch(o->_arguments);
//                // We shouldn't be adding two functions with matching parameter
//                // list types to the same overload, let alone two functions
//                // with the same types.
//                auto f = other->as<FunctionBody>();
//                bool r = (_function.typedValue().type() ==
//                    f->_function.typedValue().type());
//                assert(!r);
//                return r;
//            }
//            Function _function;
//        };
//        class ArgumentsBody : public Body
//        {
//        public:
//            ArgumentsBody(List<TypedValue> arguments)
//              : _arguments(arguments) { }
//            //int hash() const
//            //{
//            //    throw NotYetImplementedException();
//            //}
//            // In the implementation of HashTable, the table key is always on
//            // the left of the == so we won't be using this.
//            bool equals(const Body* other) const { return false; }
//            List<TypedValue> _arguments;
//        };
//        const Body* body() const { return as<Body>(); }
//    };
//    class Body : public Function::Body
//    {
//    public:
//        Body(Identifier identifier) : _identifier(identifier) { }
//        TypedValue evaluate(List<TypedValue> arguments) const
//        {
//            return _functions[Key(arguments)].evaluate(arguments);
//        }
//        Identifier identifier() const { return _identifier; }
//        TypedValue typedValue() const
//        {
//            // The type doesn't matter here, it just needs to be a FunctionTyco
//            // so that FunctionCallExpression::FunctionCallBody::evaluate()
//            // recognizes it as a function.
//            return TypedValue(FunctionTyco::nullary(IntegerType()), this);
//        }
//        void add(Function function)
//        {
//            _functions.add(Key(function), function);
//        }
//    private:
//        Identifier _identifier;
//        HashTable<Key, Function> _functions;
//    };
//    Body* body() { return as<Body>(); }
};

#endif // INCLUDED_FUNCTION_H
