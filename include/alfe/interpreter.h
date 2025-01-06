#include "alfe/code.h"

#ifndef INCLUDED_INTERPRETER_H
#define INCLUDED_INTERPRETER_H

class Interpreter : Uncopyable
{
public:
    void interpret(Code code)
    {
        Interpret interpret;
        code.walk(&interpret);
    }

    //template<class U> U evaluate(String text, const U& def)
    //{
    //    CharacterSource s(text);
    //    Value c;
    //    try {
    //        Expression e = Expression::parse(&s);
    //        if (!e.valid())
    //            return def;
    //        e.resolve(&_scope);
    //        Value v = e.evaluate(this);
    //        if (!v.valid())
    //            return def;
    //        c = v.convertTo(typeFromCompileTimeType<U>());
    //        if (!c.valid())
    //            return def;
    //    }
    //    catch (...) {
    //        return def;
    //    }
    //    return c.template value<U>();
    //}
private:
    class Interpret : public CodeWalker
    {
    public:
        Interpret() : _valuesOnStack(0) { }
        Result visit(Annotation c)
        {
            FunctionDefinitionCodeStatement fdcs = c;
            if (fdcs.valid())
                return Result::advance;
            FunctionDefinitionFromStatement fdfs = c;
            if (fdfs.valid())
                return Result::advance;
            TycoDefinitionStatement tds = c;
            if (tds.valid())
                return Result::advance;
            ConditionalStatement cs = c;
            if (cs.valid()) {
                cs.condition().walk(this);
                if (pop().value<bool>())
                    cs.trueStatement().walk(this);
                else
                    cs.falseStatement().walk(this);
                return Result::advance;
            }
            //ForeverStatement fes = c;
            //if (fes.valid()) {
            //    // TODO: handle break and continue
            //    while (true)
            //        fes.code().walk(this);
            //}
            //WhileStatement ws = c;
            //if (ws.valid()) {
            //    // TODO: handle break and continue
            //    do {
            //        ws.doStatement().walk(this);
            //        ws.condition().walk(this);
            //        if (!pop().value<bool>())
            //            break;
            //        ws.statement().walk(this);
            //    } while (true);
            //    ws.doneStatement().walk(this);
            //    return Result::advance;
            //}
            //ForStatement fs = c;
            //if (fs.valid()) {
            //    // TODO: handle break and continue
            //    fs.preStatement().walk(this);
            //    do {
            //        fs.condition().walk(this);
            //        if (!pop().value<bool>())
            //            break;
            //        fs.statement().walk(this);
            //        fs.postStatement().walk(this);
            //    } while (true);
            //    fs.doneStatement().walk(this);
            //    return Result::advance;
            //}
            VariableDefinitionStatement vds = c;
            if (vds.valid()) {
                // TODO
                return Result::advance;
            }


            return Result::recurse;
        }
        Result visit(ParseTreeObject o)
        {
            DotExpression de = o;
            if (de.valid()) {
                de.left().walk(this);
                Value e = pop();
                Identifier i = de.right();
                LValueType lValueType(e.type());
                Span s = de.span();
                if (!lValueType.valid()) {
                    if (!e.type().member(i).valid()) {
                        s.throwError("Expression has no member named " +
                            i.toString());
                    }
                    auto m = e.template value<HashTable<Identifier, Value>>();
                    e = m[i];
                    e = Value(e.type(), e.value(), s);
                }
                else {
                    Type t = lValueType.inner().member(i);
                    if (!t.valid()) {
                        s.throwError("Expression has no member named " +
                            i.toString());
                    }
                    e = Value(LValueType::wrap(t),
                        e.template value<LValue>().member(i), s);
                }
                push(e);
                return Result::advance;
            }
            NumericLiteral nl = o;
            if (nl.valid()) {
                Rational r = nl.value();
                if (r.denominator == 1)
                    push(r.numerator);
                else
                    push(r);
                return Result::advance;
            }
            FunctionCallExpression fce = o;
            if (fce.valid()) {
                Value l;
                Funco resolvedFunco = fce.resolvedFunco();
                Expression function = fce.function();
                if (!resolvedFunco.valid()) {
                    function.walk(this);
                    l = pop().rValue();
                }
                List<Value> arguments;
                for (auto p : fce.arguments()) {
                    p.walk(this);
                    arguments.add(pop().rValue());
                }
                if (resolvedFunco.valid()) {
                    // Convert arguments. TODO: share this logic with
                    // OverloadedFunctionSet::Body::evaluate()?

                    List<Value> convertedArguments;
                    if (Function(resolvedFunco).valid()) {
                        List<Tyco> parameterTycos =
                            resolvedFunco.parameterTycos();
                        auto ii = parameterTycos.begin();
                        for (auto a : arguments) {
                            Type type = *ii;
                            if (!type.valid()) {
                                a.span().throwError("Function parameter's "
                                    "type constructor is not a type.");
                            }
                            convertedArguments.add(a.convertTo(type));
                            ++ii;
                        }
                    }
                    else {
                        // Funcos that are not functions don't get their arguments
                        // converted.
                        convertedArguments = arguments;
                    }

                    push(resolvedFunco.evaluate(convertedArguments,
                        fce.span()));
                    return Result::advance;
                }
                Type lType = l.type();
                if (lType == FuncoType()) {
                    push (l.template value<OverloadedFunctionSet>().evaluate(
                        arguments, o.span()));
                    return Result::advance;
                }
                // What we have on the left isn't a function, try to call its
                // operator() method instead.
                Identifier i = Identifier(OperatorFunctionCall());
                if (!lType.member(i).valid())
                    o.span().throwError("Expression is not a function.");
                if (!LValueType(lType).valid()) {
                    auto m = l.template value<HashTable<Identifier, Value>>();
                    l = m[i];
                    l = Value(l.type(), l.value(), o.span());
                }
                else {
                    Structure* p = l.template
                        value<LValue>().rValue().template value<Structure*>();
                    l = Value(LValueType::wrap(p->getValue(i).type()),
                        LValue(p, i), o.span());
                }
                List<Value> convertedArguments;
                Function f = l.template value<Function>();
                List<Tyco> parameterTycos = f.parameterTycos();
                auto ii = parameterTycos.begin();
                for (auto a : arguments) {
                    Type type = *ii;
                    if (!type.valid()) {
                        a.span().throwError("Function parameter's type "
                            "constructor is not a type.");
                    }
                    convertedArguments.add(a.convertTo(type));
                    ++ii;
                }
                push(f.evaluate(convertedArguments, o.span()));
                return Result::advance;
            }
            return Result::recurse;
        }
        Result visit(Tyco t)
        {
            return Result::recurse;
        }
    private:
        Value pop()
        {
            Value t = _stack[_valuesOnStack - 1];
            --_valuesOnStack;
            return t;
        }
        void push(Value t)
        {
            _stack.ensure(_valuesOnStack + 1);
            _stack[_valuesOnStack] = t;
            ++_valuesOnStack;
        }

        int _valuesOnStack;
        Array<Value> _stack;
    };
};

#endif // INCLUDED_INTERPRETER_H
