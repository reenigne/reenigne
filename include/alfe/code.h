#include "alfe/main.h"

#ifndef INCLUDED_CODE_H
#define INCLUDED_CODE_H

#include "alfe/expression.h"

class CodeList;

class Code
{
public:
    Code(const Code& other) : _body(other._body) { }
    Code prev() const { return body()->_prev; }
    Code next() const { return body()->_next; }

    // Unlike Handle/ConstHandle, Code equality is just pointer equality.
    bool operator==(Code other) const { return body() == other.body(); }
    bool operator!=(Code other) const { return body() != other.body(); }

    // Moves all the code from list to immediately before this. If this is a
    // CodeList, list will be inserted at the end.
    Code insert(CodeList list) const
    {
        Code o = list;
        Body* c = o.body();
        Body* f = o.next().body();
        if (f != c) {
            Body* l = o.prev().body();
            Body* n = body();
            Body* p = prev().body();
            f->_prev = p;
            l->_next = n;
            p->_next = f;
            n->_prev = l;
            c->_next = c;
            c->_prev = c;
        }
        return *this;
    }
    template<typename T, typename... Args> Code insert(Args&&... args) const
    {
        Body* b = new T::Body(std::forward<Args>(args)...);
        Body* n = body();
        Body* p = prev().body();
        b->_next = n;
        b->_prev = p;
        n->_prev = b;
        p->_next = b;
        return *this;
    }
    CodeList split(Code start) const
    {
        Body* s = start.body();
        Body* e = body();
        Body* l = s->_prev;
        Body* p = e->_prev;
        CodeList r;
        Code o = r;
        Body* b = o.body();
        b->_next = s;
        s->_prev = b;
        b->_prev = p;
        p->_next = b;
        l->_next = e;
        e->_prev = l;
        return r;
    }
    bool walk(const std::function<bool(Code)>& function)
    {
        return body()->walk(function);
    }
    bool isSentinel() const { return as<CodeList::Body>() != 0; }
    bool valid() const { return _body != 0; }
protected:
    class Body : Uncopyable
    {
    protected:
        Body() : _prev(this), _next(this) { }
        Code code() { return Code(this); }
        virtual bool walk(const std::function<bool(Code)>& function)
        {
            return function(code());
        }
    private:
        template<class T> T* as() { return static_cast<T*>(this); }
        template<class T> T* to() { return dynamic_cast<T*>(this); }

        Body* _prev;
        Body* _next;

        friend class Code;
    };
    Body* body() const { return _body; }
    template<class T> T* as() const { return _body->as<T>(); }
    template<class T> T* to() const { return _body->to<T>(); }

private:
    Code(Body* body) : _body(body) { }
    Body* _body;

    friend class CodeList;
    friend class Statement;
};

class CodeList : public Code
{
public:
    CodeList() : Code(new Body()) { }
    ~CodeList() { reset(); }
    CodeList(const CodeList& other) : Code(other) { body()->acquire(); }
    Code begin() const { return next(); }
    Code end() const { return *this; }
private:
    class Body : public Code::Body
    {
    protected:
        virtual ~Body() { }
    private:
        Body() : _count(1) { }
        void release()
        {
            --_count;
            if (_count != 0)
                return;
            Code e = code();
            Code c = e.next();
            while (c != e) {
                Code n = c.next();
                delete c.body();
                c = n;
            }
            delete this;
        }
        bool walk(const std::function<bool(Code)>& function)
        {
            Code e = code();
            Code c = e.next();
            while (c != e) {
                if (!c.walk(function))
                    return false;
                c = c.next();
            }
            return true;
        }
        void acquire() { ++_count; }
        int _count;

        friend class CodeList;
    };
    Body* body() const { return as<Body>(); }
    void reset() const { body()->release(); }

    friend class Code;
};

class Annotation
{
public:
    Annotation() : _annotation(0) { }
    virtual ~Annotation() { }
private:
    Annotation* _annotation;

    friend class Statement;
    friend class Statement::Body;
};

class SpanAnnotation : public Annotation
{
public:
    SpanAnnotation(Span span) : _span(span) { }
    Span span() const { return _span; }
private:
    Span _span;
};

class Statement : public Code
{
public:
    Statement(Code code) : Code(code.to<Body>()) { }
    bool walkAnnotations(const std::function<bool(Annotation*)>& function)
        const
    {
        return body()->walkAnnotations(function);
    }
    template<typename T, typename... Args> T* annotate(Args&&... args) const
    {
        T* t = new T(std::forward<Args>(args)...);
        Annotation* a = static_cast<Annotation*>(t);
        Annotation* o = &body()->_annotation;
        a->_annotation = o->_annotation;
        o->_annotation = a;
        return a;
    }
    template<class T> T* getAnnotation() const
    {
        Annotation* a = body()->_annotation._annotation;
        while (a != 0) {
            T* t = dynamic_cast<T*>(a);
            if (t != 0)
                return t;
        }
        return 0;
    }
    Span span() const
    {
        SpanAnnotation* a = getAnnotation<SpanAnnotation>();
        if (a == 0)
            return Span();
        return a->span();
    }
protected:
    class Body : public Code::Body
    {
    protected:
        Body(Span span) { statement().annotate<SpanAnnotation>(span); }
        ~Body()
        {
            Annotation* a = _annotation._annotation;
            while (a != 0) {
                Annotation* n = a->_annotation;
                delete a;
                a = n;
            }
        }
        Statement statement() { return Statement(this); }
    private:
        bool walkAnnotations(const std::function<bool(Annotation*)>& function)
        {
            Annotation* a = _annotation._annotation;
            while (a != 0) {
                if (!function(a))
                    return false;
                a = a->_annotation;
            }
        }
        Annotation _annotation;

        friend class Statement;
    };
    Statement(Body* body) : Code(body) { }
private:
    Body* body() const { return as<Body>(); }
};

class ExpressionStatement : public Statement
{
public:
    Expression expression() const { return body()->expression(); }
private:
    class Body : public Statement::Body
    {
    public:
        Body(const Expression& expression, const Span& span)
          : Statement::Body(span), _expression(expression) { }
        Expression expression() const { return _expression; }
    private:
        Expression _expression;
    };
    Body* body() const { return as<Body>(); }
};

class FunctionDefinitionStatement : public Statement
{
public:
    TycoSpecifier returnTypeSpecifier() const
    {
        return body()->_returnTypeSpecifier;
    }
    Identifier name() const { return body()->_name; }
    List<VariableDefinition> parameterList() const
    {
        return body()->_parameterList;
    }
protected:
    class Body : public Statement::Body
    {
    public:
        Body(const TycoSpecifier& returnTypeSpecifier, const Identifier& name,
            const List<VariableDefinition>& parameterList, const Span& span)
          : Statement::Body(span),
            _returnTypeSpecifier(returnTypeSpecifier), _name(name),
            _parameterList(parameterList) { }
    private:
        TycoSpecifier _returnTypeSpecifier;
        Identifier _name;
        List<VariableDefinition> _parameterList;

        friend class FunctionDefinitionStatement;
    };
private:
    Body* body() const { return as<Body>(); }
};

class FunctionDefinitionCodeStatement : public FunctionDefinitionStatement
{
public:
    Code code() const { return body()->_body; }
private:
    class Body : public FunctionDefinitionStatement::Body
    {
    public:
        Body(const TycoSpecifier& returnTypeSpecifier, const Identifier& name,
            const List<VariableDefinition>& parameterList, CodeList body,
            const Span& span)
          : FunctionDefinitionStatement::Body(returnTypeSpecifier, name,
              parameterList, span),
            _body(body) { }
        bool walk(const std::function<bool(Code)>& function)
        {
            if (!function(code()))
                return false;
            return _body.walk(function);
        }
    private:
        CodeList _body;

        friend class FunctionDefinitionCodeStatement;
    };
    Body* body() const { return as<Body>(); }
};

class FunctionDefinitionFromStatement : public FunctionDefinitionStatement
{
public:
    Expression from() const { return body()->_from; }
private:
    class Body : public FunctionDefinitionStatement::Body
    {
    public:
        Body(const TycoSpecifier& returnTypeSpecifier, const Identifier& name,
            const List<VariableDefinition>& parameterList, Expression from,
            const Span& span)
            : FunctionDefinitionStatement::Body(returnTypeSpecifier, name,
                parameterList, span),
            _from(from) { }
    private:
        Expression _from;

        friend class FunctionDefinitionFromStatement;
    };
    Body* body() const { return as<Body>(); }
};

// TycoDefinitionStatement := TycoSignifier "=" TycoSpecifier ";"
class TycoDefinitionStatement : public Statement
{
public:
    TycoSignifier tycoSignifier() const { return body()->_tycoSignifier; }
    TycoSpecifier tycoSpecifier() const { return body()->_tycoSpecifier; }
private:
    class Body : public Statement::Body
    {
    public:
        Body(const TycoSignifier& tycoSignifier,
            const TycoSpecifier& tycoSpecifier, const Span& span)
          : Statement::Body(span), _tycoSignifier(tycoSignifier),
            _tycoSpecifier(tycoSpecifier) { }
    private:
        TycoSignifier _tycoSignifier;
        TycoSpecifier _tycoSpecifier;

        friend class TycoDefinitionStatement;
    };
    Body* body() const { return as<Body>(); }
};

// ConditionalStatement = (`if` | `unless`) ConditionedStatement
//   ((`elseIf` | `elseUnless`) ConditionedStatement)* [`else` Statement];
// ConditionedStatement = "(" Expression ")" Statement;
class ConditionalStatement : public Statement
{
public:
    Expression condition() const { return body()->_condition; }
    Code trueStatement() const { return body()->_trueStatement; }
    Code falseStatement() const { return body()->_falseStatement; }
private:
    class Body : public Statement::Body
    {
    public:
        Body(const Expression& condition, CodeList trueStatement,
            CodeList falseStatement, const Span& span)
          : Statement::Body(span), _condition(condition),
            _trueStatement(trueStatement), _falseStatement(falseStatement) { }
        bool walk(const std::function<bool(Code)>& function)
        {
            if (!function(code()))
                return false;
            if (!_trueStatement.walk(function))
                return false;
            return _falseStatement.walk(function);
        }
    private:
        Expression _condition;
        CodeList _trueStatement;
        CodeList _falseStatement;

        friend class ConditionalStatement;
    };
    Body* body() const { return as<Body>(); }
};

class SwitchStatement : public Statement
{
public:
    class Case : public ParseTreeObject
    {
    public:
        bool isDefault() const { return body()->isDefault(); }
        Code code() const { return body()->_code; }
        List<Expression> expressions()
        {
            ValueBody* v = as<ValueBody>();
            if (v == 0)
                return List<Expression>();
            return v->_expressions;
        }
        Case() { }
        Case(CodeList code, const Span& span)
          : ParseTreeObject(create<DefaultBody>(code, span)) { }
        Case(const List<Expression>& expressions, CodeList code,
            const Span& span)
          : ParseTreeObject(create<ValueBody>(expressions, code, span)) { }

        class Body : public ParseTreeObject::Body
        {
        public:
            Body(CodeList code, const Span& span)
              : ParseTreeObject::Body(span), _code(code) { }
            virtual bool isDefault() const = 0;
        private:
            CodeList _code;

            friend class Case;
        };
    private:
        Case(Handle other) : ParseTreeObject(other) { }

        Body* body() { return as<Body>(); }
        const Body* body() const { return as<Body>(); }

        class DefaultBody : public Body
        {
        public:
            DefaultBody(CodeList code, const Span& span)
              : Body(code, span) { }
            bool isDefault() const { return true; }
        };
        class ValueBody : public Body
        {
        public:
            ValueBody(const List<Expression>& expressions,
                CodeList code, const Span& span)
              : Body(code, span), _expressions(expressions) { }
            bool isDefault() const { return false; }
        private:
            List<Expression> _expressions;

            friend class Case;
        };
    };

    Expression expression() const { return body()->_expression; }
    Case defaultCase() const { return body()->_defaultCase; }
    List<Case> cases() const { return body()->_cases; }
    bool walkCases(std::function<bool(Case)>& function) const
    {
        for (auto c : cases()) {
            if (!function(c))
                return false;
        }
        return function(defaultCase());
    }

private:
    class Body : public Statement::Body
    {
    public:
        Body(const Expression& expression, const Case& defaultCase,
            const List<Case>& cases, const Span& span)
          : Statement::Body(span), _expression(expression),
            _defaultCase(defaultCase), _cases(cases) { }
    private:
        Expression _expression;
        Case _defaultCase;
        List<Case> _cases;

        friend class SwitchStatement;
    };

    Body* body() const { return as<Body>(); }
};

class ReturnStatement : public Statement
{
public:
    Expression expression() const { return body()->_expression; }
private:
    class Body : public Statement::Body
    {
    public:
        Body(const Expression& expression, const Span& span)
          : Statement::Body(span), _expression(expression) { }
    private:
        Expression _expression;

        friend class ReturnStatement;
    };

    Body* body() const { return as<Body>(); }
};

class BreakOrContinueStatement : public Statement
{
public:
    int breakCount() const { return body()->_breakCount; }
    bool hasContinue() const { return body()->_hasContinue; }
private:
    class Body : public Statement::Body
    {
    public:
        Body(const Span& span, int breakCount, bool hasContinue)
          : Statement::Body(span), _breakCount(breakCount),
            _hasContinue(hasContinue)
        { }
    private:
        int _breakCount;
        bool _hasContinue;

        friend class BreakOrContinueStatement;
    };

    Body* body() const { return as<Body>(); }
};

class ForeverStatement : public Statement
{
public:
    Code code() const { return body()->_code; }
private:
    class Body : public Statement::Body
    {
    public:
        Body(CodeList code, const Span& span)
          : Statement::Body(span), _code(code) { }
    private:
        CodeList _code;

        friend class ForeverStatement;
    };

    Body* body() const { return as<Body>(); }
};

class WhileStatement : public Statement
{
public:
    Code doStatement() const { return body()->_doStatement; }
    Expression condition() const { return body()->_condition; }
    Code statement() const { return body()->_statement; }
    Code doneStatement() const { return body()->_doneStatement; }
private:
    class Body : public Statement::Body
    {
    public:
        Body(CodeList doStatement, const Expression& condition,
            CodeList statement, CodeList doneStatement,
            const Span& span)
          : Statement::Body(span), _doStatement(doStatement),
            _condition(condition), _statement(statement),
            _doneStatement(doneStatement) { }
    private:
        CodeList _doStatement;
        Expression _condition;
        CodeList _statement;
        CodeList _doneStatement;

        friend class WhileStatement;
    };

    Body* body() const { return as<Body>(); }
};

class ForStatement : public Statement
{
public:
    Code preStatement() const { return body()->_preStatement; }
    Expression condition() const { return body()->_condition; }
    Code postStatement() const { return body()->_postStatement; }
    Code statement() const { return body()->_statement; }
    Code doneStatement() const { return body()->_doneStatement; }
private:
    class Body : public Statement::Body
    {
    public:
        Body(CodeList preStatement, const Expression& condition,
            CodeList postStatement, CodeList statement,
            CodeList doneStatement, const Span& span)
          : Statement::Body(span), _preStatement(preStatement),
            _condition(condition), _postStatement(postStatement),
            _statement(statement), _doneStatement(doneStatement) { }
    private:
        CodeList _preStatement;
        Expression _condition;
        CodeList _postStatement;
        CodeList _statement;
        CodeList _doneStatement;

        friend class ForStatement;
    };

    Body* body() const { return as<Body>(); }
};

class LabelStatement : public Statement
{
public:
    Identifier identifier() const { return body()->_identifier; }
private:
    class Body : public Statement::Body
    {
    public:
        Body(const Identifier& identifier, const Span& span)
          : Statement::Body(span), _identifier(identifier) { }
    private:
        Identifier _identifier;

        friend class LabelStatement;
    };

    Body* body() const { return as<Body>(); }
};

class GotoStatement : public Statement
{
public:
    Expression expression() const { return body()->_expression; }
private:
    class Body : public Statement::Body
    {
    public:
        Body(const Expression& expression, const Span& span)
            : Statement::Body(span), _expression(expression) { }
        void resolve(Scope* scope) { _expression.resolve(scope); }
    private:
        Expression _expression;

        friend class GotoStatement;
    };

    Body* body() const { return as<Body>(); }
};


#endif // INCLUDED_CODE_H
