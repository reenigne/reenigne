#include "alfe/main.h"

#ifndef INCLUDED_STATEMENT_H
#define INCLUDED_STATEMENT_H

template<class T> class StatementT;
typedef StatementT<void> Statement;

template<class T> class IdentifierT;
typedef IdentifierT<void> Identifier;

template<class T> class StatementT : public ParseTreeObject
{
public:
    static Statement parse(CharacterSource* source)
    {
        Statement statement = ExpressionStatement::parse(source);
        if (statement.valid())
            return statement;
        statement = FunctionDefinitionStatement::parse(source);
        if (statement.valid())
            return statement;
        statement = ExpressionStatement::parseAssignment(source);
        if (statement.valid())
            return statement;
        statement = CompoundStatement::parse(source);
        if (statement.valid())
            return statement;
        statement = TycoDefinitionStatement::parse(source);
        if (statement.valid())
            return statement;
        statement = NothingStatement::parse(source);
        if (statement.valid())
            return statement;
        statement = IncrementDecrementStatement::parse(source);
        if (statement.valid())
            return statement;
        statement = ConditionalStatement::parse(source);
        if (statement.valid())
            return statement;
        statement = SwitchStatement::parse(source);
        if (statement.valid())
            return statement;
        statement = ReturnStatement::parse(source);
        if (statement.valid())
            return statement;
        statement = IncludeStatement::parse(source);
        if (statement.valid())
            return statement;
        statement = BreakOrContinueStatement::parse(source);
        if (statement.valid())
            return statement;
        statement = ForeverStatement::parse(source);
        if (statement.valid())
            return statement;
        statement = WhileStatement::parse(source);
        if (statement.valid())
            return statement;
        statement = ForStatement::parse(source);
        if (statement.valid())
            return statement;
        statement = LabelStatement::parse(source);
        if (statement.valid())
            return statement;
        return GotoStatement::parse(source);
    }
    static Statement parseOrFail(CharacterSource* source)
    {
        Statement statement = parse(source);
        if (!statement.valid())
            source->location().throwError("Expected statement");
        return statement;
    }
    StatementT() { }
    void resolve(Scope* scope) { body()->resolve(scope); }
protected:
    StatementT(Handle other) : ParseTreeObject(other) { }

    class Body : public ParseTreeObject::Body
    {
    public:
        Body(const Span& span) : ParseTreeObject::Body(span) { }
        virtual void resolve(Scope* scope) = 0;
    };

    Body* body() { return as<Body>(); }
};

class ExpressionStatement : public Statement
{
public:
    ExpressionStatement(Expression expression, Span span)
      : ExpressionStatement(create<Body>(expression, span)) { }

    static ExpressionStatement parse(CharacterSource* source)
    {
        CharacterSource s = *source;
        Expression expression = Expression::parse(&s);
        if (!expression.valid())
            return ExpressionStatement();
        Span span;
        if (!Space::parseCharacter(&s, ';', &span))
            return ExpressionStatement();
        *source = s;
        if (!expression.mightHaveSideEffect())
            source->location().throwError("Statement has no effect");
        return ExpressionStatement(expression, expression.span() + span);
    }

    static ExpressionStatement parseAssignment(CharacterSource* source)
    {
        CharacterSource s = *source;
        Expression left = Expression::parse(&s);
        Location operatorLocation = s.location();
        if (!left.valid())
            return ExpressionStatement();
        Span span;

        static const Operator ops[] = {
            OperatorAssignment(), OperatorAddAssignment(),
            OperatorSubtractAssignment(), OperatorMultiplyAssignment(),
            OperatorDivideAssignment(), OperatorModuloAssignment(),
            OperatorShiftLeftAssignment(), OperatorShiftRightAssignment(),
            OperatorBitwiseAndAssignment(), OperatorBitwiseOrAssignment(),
            OperatorBitwiseXorAssignment(), OperatorPowerAssignment(),
            Operator()};

        const Operator* op;
        for (op = ops; op->valid(); ++op)
            if (Space::parseOperator(&s, op->toString(), &span))
                break;
        if (!op->valid())
            return ExpressionStatement();

        *source = s;
        Expression right = Expression::parseOrFail(source);
        Space::assertCharacter(source, ';', &span);

        return ExpressionStatement(FunctionCallExpression::binary(*op, span,
            FunctionCallExpression::unary(OperatorAmpersand(), Span(), left),
            right), left.span() + span);
    }
private:
    ExpressionStatement() { }
    ExpressionStatement(Handle other) : Statement(other) { }

    class Body : public Statement::Body
    {
    public:
        Body(const Expression& expression, const Span& span)
          : Statement::Body(span), _expression(expression) { }
        void resolve(Scope* scope) { _expression.resolve(scope); }
    private:
        Expression _expression;
    };
};

class FromStatement : public Statement
{
public:
    static FromStatement parse(CharacterSource* source)
    {
        Span span;
        if (!Space::parseKeyword(source, "from", &span))
            return FromStatement();
        Expression dll = Expression::parseOrFail(source);
        Space::assertCharacter(source, ';', &span);
        return create<Body>(dll, span);
    }
private:
    FromStatement() { }
    FromStatement(Handle other) : Statement(other) { }

    class Body : public Statement::Body
    {
    public:
        Body(const Expression& expression, const Span& span)
          : Statement::Body(span), _expression(expression) { }
        void resolve(Scope* scope) { _expression.resolve(scope); }
    private:
        Expression _expression;
    };
};

template<class T> class FunctionDefinitionStatementT;
typedef FunctionDefinitionStatementT<void> FunctionDefinitionStatement;

template<class T> class FunctionDefinitionStatementT : public Statement
{
public:
    class Parameter : public VariableDefinitionT<T>
    {
    public:
        static Parameter parse(CharacterSource* source)
        {
            TycoSpecifier typeSpecifier = TycoSpecifier::parse(source);
            if (!typeSpecifier.valid())
                return Parameter();
            Identifier name = Identifier::parse(source);
            if (!name.valid())
                source->location().throwError("Expected identifier");
            return create<Body>(typeSpecifier, name);
        }
        void resolve(Scope* scope) { body()->resolve(scope); }
    private:
        Parameter() { }
        Parameter(Handle other) : VariableDefinitionStatement(other) { }
        class Body : public VariableDefinitionT<T>::Body
        {
        public:
            Body(const TycoSpecifier& typeSpecifier, const Identifier& name)
              : ParseTreeObject::Body(typeSpecifier.span() + name.span()),
                _typeSpecifier(typeSpecifier), _name(name) { }
        private:
            TycoSpecifier _typeSpecifier;
            IdentifierT<T> _name;
        };
    };

    static FunctionDefinitionStatement parse(CharacterSource* source)
    {
        CharacterSource s = *source;
        TycoSpecifier returnTypeSpecifier = TycoSpecifier::parse(&s);
        if (!returnTypeSpecifier.valid())
            return FunctionDefinitionStatement();
        Identifier name = Identifier::parse(&s);
        if (!name.valid())
            return FunctionDefinitionStatement();
        Span span;
        if (!Space::parseCharacter(&s, '('))
            return FunctionDefinitionStatement();
        *source = s;
        List<Parameter> parameterList = parseParameterList(source);
        Space::assertCharacter(source, ')');
        Statement body = FromStatement::parse(source);
        if (!body.valid())
            body = Statement::parseOrFail(source);
        return create<Body>(returnTypeSpecifier, name, parameterList, body);
    }
private:
    static List<Parameter> parseParameterList(CharacterSource* source)
    {
        List<Parameter> list;
        Parameter parameter = Parameter::parse(source);
        if (!parameter.valid())
            return list;
        list.add(parameter);
        Span span;
        while (Space::parseCharacter(source, ',', &span)) {
            Parameter parameter = Parameter::parse(source);
            if (!parameter.valid())
                source->location().throwError("Expected parameter");
            list.add(parameter);
        }
        return list;
    }

    FunctionDefinitionStatementT() { }
    FunctionDefinitionStatementT(Handle other) : Statement(other) { }

    class Body : public Statement::Body
    {
    public:
        Body(const TycoSpecifier& returnTypeSpecifier, const Identifier& name,
            const List<Parameter>& parameterList, const Statement& body)
          : Statement::Body(returnTypeSpecifier.span() + body.span()),
            _returnTypeSpecifier(returnTypeSpecifier), _name(name),
            _parameterList(parameterList), _body(body) { }
        TypeT<T> type() const
        {
            FunctionType t(_returnTypeSpecifier);
            for (auto p : _parameterList)
                t.instantiate(p.type());
            return t;
        }
        void resolve(Scope* scope)
        {
            _returnTypeSpecifier.resolve(scope);
            for (auto p : _parameterList) {
                p.resolve(scope);
                _scope.addObject(p.name(), p);
            }
            _scope.setParentScope(scope);
            _body.resolve(&_scope);
        }
    private:
        TycoSpecifier _returnTypeSpecifier;
        Identifier _name;
        List<Parameter> _parameterList;
        Statement _body;
        Scope _scope;
    };
};

class StatementSequence : public ParseTreeObject
{
public:
    static StatementSequence parse(CharacterSource* source)
    {
        Span span;
        List<Statement> sequence;
        do {
            Statement statement = Statement::parse(source);
            if (!statement.valid())
                break;
            span += statement.span();
            sequence.add(statement);
        } while (true);
        return create<Body>(sequence, span);
    }
    void resolve(Scope* scope) { body()->resolve(scope); }
private:
    StatementSequence(Handle other) : ParseTreeObject(other) { }

    class Body : public ParseTreeObject::Body
    {
    public:
        Body(const List<Statement>& sequence, const Span& span)
          : ParseTreeObject::Body(span), _sequence(sequence) { }
        void resolve(Scope* scope)
        {
            for (auto s : _sequence)
                s.resolve(scope);
        }
    private:
        List<Statement> _sequence;
    };

    Body* body() { return as<Body>(); }
};

class CompoundStatement : public Statement
{
public:
    static CompoundStatement parse(CharacterSource* source)
    {
        Span span;
        if (!Space::parseCharacter(source, '{', &span))
            return CompoundStatement();
        StatementSequence sequence = StatementSequence::parse(source);
        Space::assertCharacter(source, '}', &span);
        return create<Body>(sequence, span);
    }
private:
    CompoundStatement() { }
    CompoundStatement(Handle other) : Statement(other) { }

    class Body : public Statement::Body
    {
    public:
        Body(const StatementSequence& sequence, const Span& span)
          : Statement::Body(span), _sequence(sequence) { }
        void resolve(Scope* scope) { _sequence.resolve(scope); }
    private:
        StatementSequence _sequence;
    };
};

// TycoDefinitionStatement := TycoSignifier "=" TycoSpecifier ";"
class TycoDefinitionStatement : public Statement
{
public:
    static TycoDefinitionStatement parse(CharacterSource* source)
    {
        CharacterSource s = *source;
        CharacterSource s2 = s;
        TycoSignifier tycoSignifier = TycoSignifier::parse(&s);
        if (!tycoSignifier.valid())
            return TycoDefinitionStatement();
        if (!Space::parseCharacter(&s, '='))
            return TycoDefinitionStatement();
        *source = s;
        TycoSpecifier tycoSpecifier = TycoSpecifier::parse(source);
        Span span;
        Space::assertCharacter(source, ';', &span);
        return create<Body>(tycoSignifier, tycoSpecifier,
            tycoSignifier.span() + span);
    }
private:
    TycoDefinitionStatement() { }
    TycoDefinitionStatement(Handle other) : Statement(other) { }

    class Body : public Statement::Body
    {
    public:
        Body(const TycoSignifier& tycoSignifier,
            const TycoSpecifier& tycoSpecifier, const Span& span)
          : Statement::Body(span), _tycoSignifier(tycoSignifier),
            _tycoSpecifier(tycoSpecifier) { }
        void resolve(Scope* scope)
        {

        }
    private:
        TycoSignifier _tycoSignifier;
        TycoSpecifier _tycoSpecifier;
    };
};

class NothingStatement : public Statement
{
public:
    static NothingStatement parse(CharacterSource* source)
    {
        Span span;
        if (!Space::parseKeyword(source, "nothing", &span))
            return NothingStatement();
        Space::assertCharacter(source, ';', &span);
        return create<Body>(span);
    }
private:
    NothingStatement() { }
    NothingStatement(Handle other) : Statement(other) { }

    class Body : public Statement::Body
    {
    public:
        Body(const Span& span) : Statement::Body(span) { }
        void resolve(Scope* scope) { }
    };
};

class IncrementDecrementStatement : public Statement
{
public:
    static Statement parse(CharacterSource* source)
    {
        Span span;
        Operator o = OperatorIncrement().parse(source, &span);
        if (!o.valid())
            o = OperatorDecrement().parse(source, &span);
        if (!o.valid())
            return Statement();
        Expression lValue = Expression::parse(source);
        Span span2;
        Space::assertCharacter(source, ';', &span2);
        return ExpressionStatement(FunctionCallExpression::unary(o, span,
            FunctionCallExpression::unary(
                OperatorAmpersand(), Span(), lValue)),
            span + span2);
    }
};

// ConditionalStatement = (`if` | `unless`) ConditionedStatement
//   ((`elseIf` | `elseUnless`) ConditionedStatement)* [`else` Statement];
// ConditionedStatement = "(" Expression ")" Statement;
class ConditionalStatement : public Statement
{
public:
    static ConditionalStatement parse(CharacterSource* source)
    {
        Span span;
        if (Space::parseKeyword(source, "if", &span))
            return parse2(source, span, false);
        if (Space::parseKeyword(source, "unless", &span))
            return parse2(source, span, true);
        return ConditionalStatement();
    }
private:
    static ConditionalStatement parse2(CharacterSource* source, Span span,
        bool unlessStatement)
    {
        Space::assertCharacter(source, '(');
        Expression condition = Expression::parseOrFail(source);
        Space::assertCharacter(source, ')');
        Statement statement = Statement::parseOrFail(source);
        span += statement.span();
        Statement elseStatement;
        if (Space::parseKeyword(source, "else")) {
            elseStatement = Statement::parseOrFail(source);
            span += elseStatement.span();
        }
        else
            if (Space::parseKeyword(source, "elseIf")) {
                elseStatement = parse2(source, span, false);
                span += elseStatement.span();
            }
            else
                if (Space::parseKeyword(source, "elseUnless")) {
                    elseStatement = parse2(source, span, true);
                    span += elseStatement.span();
                }
        if (unlessStatement)
            condition = !condition;
        return create<Body>(condition, statement, elseStatement, span);
    }

    ConditionalStatement() { }
    ConditionalStatement(Handle other) : Statement(other) { }

    class Body : public Statement::Body
    {
    public:
        Body(const Expression& condition, const Statement& trueStatement,
            const Statement& falseStatement, const Span& span)
          : Statement::Body(span), _condition(condition),
            _trueStatement(trueStatement), _falseStatement(falseStatement) { }
        void resolve(Scope* scope)
        {
            _condition.resolve(scope);
            _trueStatement.resolve(scope);
            _falseStatement.resolve(scope);
        }
    private:
        Expression _condition;
        Statement _trueStatement;
        Statement _falseStatement;
    };
};

class SwitchStatement : public Statement
{
public:
    static SwitchStatement parse(CharacterSource* source)
    {
        Span span;
        if (!Space::parseKeyword(source, "switch", &span))
            return SwitchStatement();
        Space::assertCharacter(source, '(');
        Expression expression = Expression::parseOrFail(source);
        Space::assertCharacter(source, ')');
        Space::assertCharacter(source, '{');
        Case defaultCase;

        CharacterSource s = *source;
        List<Case> cases;
        do {
            Case c = Case::parse(source);
            if (!c.valid())
                break;
            if (c.isDefault()) {
                if (defaultCase.valid())
                    s.location().throwError(
                        "This switch statement already has a default case");
                defaultCase = c;
            }
            else
                cases.add(c);
        } while (true);
        Space::assertCharacter(source, '}', &span);
        return create<Body>(expression, defaultCase, cases, span);
    }
private:
    SwitchStatement() { }
    SwitchStatement(Handle other) { }

    class Case : public ParseTreeObject
    {
    public:
        static Case parse(CharacterSource* source)
        {
            List<Expression> expressions;
            bool defaultType;
            Span span;
            if (Space::parseKeyword(source, "case", &span)) {
                defaultType = false;
                do {
                    Expression expression = Expression::parseOrFail(source);
                    expressions.add(expression);
                    if (!Space::parseCharacter(source, ','))
                        break;
                } while (true);
            }
            else {
                defaultType = true;
                if (!Space::parseKeyword(source, "default", &span))
                    source->location().throwError("Expected case or default");
            }
            Space::assertCharacter(source, ':');
            Statement statement = Statement::parseOrFail(source);
            span += statement.span();
            if (defaultType)
                return create<DefaultBody>(statement, span);
            return create<ValueBody>(expressions, statement, span);
        }
        bool isDefault() const { return as<Case>()->isDefault(); }
        void resolve(Scope* scope) { body()->resolve(scope); }

        Case() { }

        class Body : public ParseTreeObject::Body
        {
        public:
            Body(const Statement& statement, const Span& span)
              : ParseTreeObject::Body(span), _statement(statement) { }
            virtual bool isDefault() const = 0;
            virtual void resolve(Scope* scope)
            {
                _statement.resolve(scope);
            }
        private:
            Statement _statement;
        };
    private:
        Case(Handle other) : ParseTreeObject(other) { }

        Body* body() { return as<Body>(); }

        class DefaultBody : public Body
        {
        public:
            DefaultBody(const Statement& statement, const Span& span)
              : Body(statement, span) { }
            bool isDefault() const { return true; }
        };
        class ValueBody : public Body
        {
        public:
            ValueBody(const List<Expression>& expressions,
                const Statement& statement, const Span& span)
              : Body(statement, span), _expressions(expressions) { }
            bool isDefault() const { return false; }
            void resolve(Scope* scope)
            {
                for (auto e : _expressions)
                    e.resolve(scope);
                Body::resolve(scope);
            }
        private:
            List<Expression> _expressions;
        };
    };

    class Body : public Statement::Body
    {
    public:
        Body(const Expression& expression, const Case& defaultCase,
            const List<Case>& cases, const Span& span)
          : Statement::Body(span), _expression(expression),
            _defaultCase(defaultCase), _cases(cases) { }
        void resolve(Scope* scope)
        {
            _expression.resolve(scope);
            _defaultCase.resolve(scope);
            for (auto c : _cases)
                c.resolve(scope);
        }
    private:
        Expression _expression;
        Case _defaultCase;
        List<Case> _cases;
    };
};

class ReturnStatement : public Statement
{
public:
    static ReturnStatement parse(CharacterSource* source)
    {
        Span span;
        if (!Space::parseKeyword(source, "return", &span))
            return ReturnStatement();
        Expression expression = Expression::parseOrFail(source);
        Space::assertCharacter(source, ';', &span);
        return create<Body>(expression, span);
    }
private:
    ReturnStatement() { }
    ReturnStatement(Handle other) : Statement(other) { }

    class Body : public Statement::Body
    {
    public:
        Body(const Expression& expression, const Span& span)
          : Statement::Body(span), _expression(expression) { }
        void resolve(Scope* scope) { _expression.resolve(scope); }
    private:
        Expression _expression;
    };
};

class IncludeStatement : public Statement
{
public:
    static IncludeStatement parse(CharacterSource* source)
    {
        Span span;
        if (!Space::parseKeyword(source, "include", &span))
            return IncludeStatement();
        Expression expression = Expression::parseOrFail(source);
        Space::assertCharacter(source, ';', &span);
        return create<Body>(expression, span);
    }
private:
    IncludeStatement() { }
    IncludeStatement(Handle other) : Statement(other) { }

    class Body : public Statement::Body
    {
    public:
        Body(const Expression& expression, const Span& span)
          : Statement::Body(span), _expression(expression) { }
        void resolve(Scope* scope) { _expression.resolve(scope); }
    private:
        Expression _expression;
    };
};

template<class T> class BreakOrContinueStatementT;
typedef BreakOrContinueStatementT<void> BreakOrContinueStatement;

template<class T> class BreakOrContinueStatementT : public Statement
{
public:
    static BreakOrContinueStatement parse(CharacterSource* source)
    {
        BreakOrContinueStatement breakStatement = parseBreak(source);
        if (breakStatement.valid())
            return breakStatement;
        return parseContinue(source);
    }
private:
    BreakOrContinueStatementT() { }
    BreakOrContinueStatementT(Handle other) : Statement(other) { }

    static BreakOrContinueStatement parseBreak(CharacterSource* source)
    {
        Span span;
        if (!Space::parseKeyword(source, "break", &span))
            return BreakOrContinueStatement();
        BreakOrContinueStatement statement = parse(source);
        if (!statement.valid())
            Space::assertCharacter(source, ';', &span);
        else
            span += statement.span();
        return create<BreakBody>(statement, span);
    }

    static BreakOrContinueStatement parseContinue(CharacterSource* source)
    {
        Span span;
        if (!Space::parseKeyword(source, "continue", &span))
            return BreakOrContinueStatement();
        Space::assertCharacter(source, ';', &span);
        return create<ContinueBody>(span);
    }

    class Body : public Statement::Body
    {
    public:
        Body(const Span& span) : Statement::Body(span) { }
        void resolve(Scope* scope) { }
    };

    class BreakBody : public Body
    {
    public:
        BreakBody(const BreakOrContinueStatement& statement, const Span& span)
          : Body(span), _statement(statement) { }
    private:
        BreakOrContinueStatement _statement;
    };

    class ContinueBody : public Body
    {
    public:
        ContinueBody(const Span& span) : Body(span) { }
    };
};

class ForeverStatement : public Statement
{
public:
    static ForeverStatement parse(CharacterSource* source)
    {
        Span span;
        if (!Space::parseKeyword(source, "forever", &span))
            return ForeverStatement();
        Statement statement = Statement::parseOrFail(source);
        return create<Body>(statement, span + statement.span());
    }
private:
    ForeverStatement() { }
    ForeverStatement(Handle other) : Statement(other) { }

    class Body : public Statement::Body
    {
    public:
        Body(const Statement& statement, const Span& span)
          : Statement::Body(span), _statement(statement) { }
        void resolve(Scope* scope) { _statement.resolve(scope); }
    private:
        Statement _statement;
    };
};

class WhileStatement : public Statement
{
public:
    static WhileStatement parse(CharacterSource* source)
    {
        Span span;
        Statement doStatement;
        bool foundDo = false;
        if (Space::parseKeyword(source, "do", &span)) {
            foundDo = true;
            doStatement = Statement::parseOrFail(source);
        }
        bool foundWhile = false;
        bool foundUntil = false;
        if (Space::parseKeyword(source, "while", &span))
            foundWhile = true;
        else
            if (Space::parseKeyword(source, "until", &span))
                foundUntil = true;
        if (!foundWhile && !foundUntil) {
            if (foundDo)
                source->location().throwError("Expected while or until");
            return WhileStatement();
        }
        Space::assertCharacter(source, '(');
        Expression condition = Expression::parse(source);
        Space::assertCharacter(source, ')');
        Statement statement = Statement::parseOrFail(source);
        span += statement.span();
        Statement doneStatement;
        if (Space::parseKeyword(source, "done")) {
            doneStatement = Statement::parseOrFail(source);
            span += doneStatement.span();
        }
        if (foundUntil)
            condition = !condition;
        return create<Body>(doStatement, condition, statement,
            doneStatement, span);
    }
private:
    WhileStatement() { }
    WhileStatement(Handle other) : Statement(other) { }

    class Body : public Statement::Body
    {
    public:
        Body(const Statement& doStatement, const Expression& condition,
            const Statement& statement, const Statement& doneStatement,
            const Span& span)
          : Statement::Body(span), _doStatement(doStatement),
            _condition(condition), _statement(statement),
            _doneStatement(doneStatement) { }
        void resolve(Scope* scope)
        {
            _doStatement.resolve(scope);
            _condition.resolve(scope);
            _statement.resolve(scope);
            _doneStatement.resolve(scope);
        }
    private:
        Statement _doStatement;
        Expression _condition;
        Statement _statement;
        Statement _doneStatement;
    };
};

class ForStatement : public Statement
{
public:
    static ForStatement parse(CharacterSource* source)
    {
        Span span;
        if (!Space::parseKeyword(source, "for", &span))
            return ForStatement();
        Space::assertCharacter(source, '(');
        Statement preStatement = Statement::parse(source);
        if (!preStatement.valid())
            Space::assertCharacter(source, ';');
        Expression expression = Expression::parse(source);
        Space::assertCharacter(source, ';');
        Statement postStatement = Statement::parse(source);
        Space::parseCharacter(source, ')');
        Statement statement = Statement::parseOrFail(source);
        span += statement.span();
        Statement doneStatement;
        if (Space::parseKeyword(source, "done")) {
            doneStatement = Statement::parseOrFail(source);
            span += doneStatement.span();
        }
        return create<Body>(preStatement, expression, postStatement,
            statement, doneStatement, span);
    }
private:
    ForStatement() { }
    ForStatement(Handle other) : Statement(other) { }

    class Body : public Statement::Body
    {
    public:
        Body(const Statement& preStatement, const Expression& condition,
            const Statement& postStatement, const Statement& statement,
            const Statement& doneStatement, const Span& span)
          : Statement::Body(span), _preStatement(preStatement),
            _condition(condition), _postStatement(postStatement),
            _statement(statement), _doneStatement(doneStatement) { }
        void resolve(Scope* scope)
        {
            _preStatement.resolve(scope);
            _condition.resolve(scope);
            _postStatement.resolve(scope);
            _statement.resolve(scope);
            _doneStatement.resolve(scope);
        }
    private:
        Statement _preStatement;
        Expression _condition;
        Statement _postStatement;
        Statement _statement;
        Statement _doneStatement;
    };
};

template<class T> class LabelStatementT;
typedef LabelStatementT<void> LabelStatement;

template<class T> class LabelStatementT : public Statement
{
public:
    static LabelStatement parse(CharacterSource* source)
    {
        CharacterSource s2 = *source;
        Identifier identifier = Identifier::parse(&s2);
        if (!identifier.valid())
            return LabelStatement();
        Span span;
        if (!Space::parseCharacter(&s2, ':', &span))
            return LabelStatement();
        return create<Body>(identifier, identifier.span() + span);
    }
private:
    LabelStatementT() { }
    LabelStatementT(Handle other) : Statement(other) { }

    class Body : public Statement::Body
    {
    public:
        Body(const Identifier& identifier, const Span& span)
          : Statement::Body(span), _identifier(identifier) { }
        void resolve(Scope* scope)
        {

        }
    private:
        IdentifierT<T> _identifier;
    };
};

class GotoStatement : public Statement
{
public:
    static GotoStatement parse(CharacterSource* source)
    {
        Span span;
        if (!Space::parseKeyword(source, "goto", &span))
            return GotoStatement();
        Expression expression = Expression::parseOrFail(source);
        Span span2;
        Space::parseCharacter(source, ';', &span);
        return create<Body>(expression, span);
    }
private:
    GotoStatement() { }
    GotoStatement(Handle other) : Statement(other) { }

    class Body : public Statement::Body
    {
    public:
        Body(const Expression& expression, const Span& span)
          : Statement::Body(span), _expression(expression) { }
        void resolve(Scope* scope) { _expression.resolve(scope); }
    private:
        Expression _expression;
    };
};

#endif // INCLUDED_STATEMENT_H
