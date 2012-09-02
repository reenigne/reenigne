template<class T> class StatementTemplate;
typedef StatementTemplate<void> Statement;

template<class T> class StatementTemplate : public ParseTreeObject
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
        statement = VariableDefinitionStatement::parse(source);
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
    StatementTemplate() { }
protected:
    StatementTemplate(const Implementation* implementation)
      : ParseTreeObject(implementation) { }

    class Implementation : public ParseTreeObject::Implementation
    {
    public:
        Implementation(const Span& span)
          : ParseTreeObject::Implementation(span) { }
    };
};

class ExpressionStatement : public Statement
{
public:
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
        if (!expression.is<FunctionCallExpression>())
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
            if (Space::parseOperator(&s, op->name(), &span))
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
    ExpressionStatement(const Expression& expression, const Span& span)
      : Statement(new Implementation(expression, span)) { }
private:
    ExpressionStatement() { }

    class Implementation : public Statement::Implementation
    {
    public:
        Implementation(const Expression& expression, const Span& span)
          : Statement::Implementation(span), _expression(expression) { }
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
        return new Implementation(dll, span);
    }
private:
    FromStatement() { }
    FromStatement(const Implementation* implementation)
      : Statement(implementation) { }

    class Implementation : public Statement::Implementation
    {
    public:
        Implementation(const Expression& expression, const Span& span)
          : Statement::Implementation(span), _expression(expression) { }
    private:
        Expression _expression;
    };
};

class FunctionDefinitionStatement : public Statement
{
public:
    class Parameter : public ParseTreeObject
    {
    public:
        Parameter(const TycoSpecifier& typeSpecifier, const Identifier& name)
          : ParseTreeObject(new Implementation(typeSpecifier, name)) { }

        static Parameter parse(CharacterSource* source)
        {
            TycoSpecifier typeSpecifier = TycoSpecifier::parse(source);
            if (!typeSpecifier.valid())
                return Parameter();
            Identifier name = Identifier::parse(source);
            if (!name.valid())
                source->location().throwError("Expected identifier");
            return Parameter(typeSpecifier, name);
        }
    private:
        Parameter() { }
        class Implementation : public ParseTreeObject::Implementation
        {
        public:
            Implementation(const TycoSpecifier& typeSpecifier,
                const Identifier& name)
              : ParseTreeObject::Implementation(
                    typeSpecifier.span() + name.span()),
                _typeSpecifier(typeSpecifier), _name(name) { }
        private:
            TycoSpecifier _typeSpecifier;
            Identifier _name;
        };
    };

    FunctionDefinitionStatement(const TycoSpecifier& returnTypeSpecifier,
        const Identifier& name, const List<Parameter>& parameterList,
        const Statement& body)
      : Statement(
        new Implementation(returnTypeSpecifier, name, parameterList, body)) { }

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
        return FunctionDefinitionStatement(returnTypeSpecifier, name,
            parameterList, body);
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

    FunctionDefinitionStatement() { }
    FunctionDefinitionStatement(const Implementation* implementation)
      : Statement(implementation) { }

    class Implementation : public Statement::Implementation
    {
    public:
        Implementation(const TycoSpecifier& returnTypeSpecifier,
            const Identifier& name, const List<Parameter>& parameterList,
            const Statement& body)
          : Statement::Implementation(
            returnTypeSpecifier.span() + body.span()),
            _returnTypeSpecifier(returnTypeSpecifier), _name(name),
            _parameterList(parameterList), _body(body) { }
    private:
        TycoSpecifier _returnTypeSpecifier;
        Identifier _name;
        List<Parameter> _parameterList;
        Statement _body;
    };
};

class VariableDefinitionStatement : public Statement
{
public:
    static VariableDefinitionStatement parse(CharacterSource* source)
    {
        CharacterSource s = *source;
        TycoSpecifier typeSpecifier = TycoSpecifier::parse(&s);
        if (!typeSpecifier.valid())
            return VariableDefinitionStatement();
        Identifier identifier = Identifier::parse(&s);
        if (!identifier.valid())
            return VariableDefinitionStatement();
        *source = s;
        Expression initializer;
        if (Space::parseCharacter(source, '='))
            initializer = Expression::parseOrFail(source);
        Span span;
        Space::assertCharacter(source, ';', &span);
        return VariableDefinitionStatement(typeSpecifier, identifier,
            initializer, typeSpecifier.span() + span);
    }
private:
    VariableDefinitionStatement() { }
    VariableDefinitionStatement(const TycoSpecifier& typeSpecifier,
        const Identifier& identifier, const Expression& initializer,
        const Span& span)
      : Statement(new Implementation(typeSpecifier, identifier, initializer,
        span)) { }

    class Implementation : public Statement::Implementation
    {
    public:
        Implementation(const TycoSpecifier& typeSpecifier,
            const Identifier& identifier, const Expression& initializer,
            const Span& span)
          : Statement::Implementation(span), _typeSpecifier(typeSpecifier),
            _identifier(identifier), _initializer(initializer) { }
    private:
        TycoSpecifier _typeSpecifier;
        Identifier _identifier;
        Expression _initializer;
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
        return new Implementation(sequence, span);
    }
private:
    StatementSequence(const Implementation* implementation)
      : ParseTreeObject(implementation) { }

    class Implementation : public ParseTreeObject::Implementation
    {
    public:
        Implementation(const List<Statement>& sequence, const Span& span)
          : ParseTreeObject::Implementation(span), _sequence(sequence) { }
    private:
        List<Statement> _sequence;
    };
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
        return new Implementation(sequence, span);
    }
private:
    CompoundStatement() { }
    CompoundStatement(const Implementation* implementation)
      : Statement(implementation) { } 

    class Implementation : public Statement::Implementation
    {
    public:
        Implementation(const StatementSequence& sequence, const Span& span)
          : Statement::Implementation(span), _sequence(sequence) { }
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
        return new Implementation(tycoSignifier, tycoSpecifier,
            tycoSignifier.span() + span);
    }
    TycoDefinitionStatement(const TycoSignifier& tycoSignifier,
        const TycoSpecifier& tycoSpecifier)
      : Statement(new Implementation(tycoSignifier, tycoSpecifier, Span())) { }
private:
    TycoDefinitionStatement() { }
    TycoDefinitionStatement(const Implementation* implementation)
      : Statement(implementation) { }

    class Implementation : public Statement::Implementation
    {
    public:
        Implementation(const TycoSignifier& tycoSignifier,
            const TycoSpecifier& tycoSpecifier, const Span& span)
          : Statement::Implementation(span), _tycoSignifier(tycoSignifier),
            _tycoSpecifier(tycoSpecifier) { }

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
        return NothingStatement(span);
    }
private:
    NothingStatement() { }
    NothingStatement(const Span& span)
      : Statement(new Implementation(span)) { }

    class Implementation : public Statement::Implementation
    {
    public:
        Implementation(const Span& span) : Statement::Implementation(span) { }
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
        return new Implementation(condition, statement, elseStatement, span);
    }

    ConditionalStatement() { }
    ConditionalStatement(const Implementation* implementation)
      : Statement(implementation) { }

    class Implementation : public Statement::Implementation
    {
    public:
        Implementation(const Expression& condition,
            const Statement& trueStatement, const Statement& falseStatement,
            const Span& span)
          : Statement::Implementation(span), _condition(condition),
            _trueStatement(trueStatement), _falseStatement(falseStatement) { }
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
        return new Implementation(expression, defaultCase, cases, span);
    }
private:
    SwitchStatement() { }
    SwitchStatement(const Implementation* implementation)
      : Statement(implementation) { }

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
                return new DefaultImplementation(statement, span);
            return new ValueImplementation(expressions, statement, span);
        }
        bool isDefault() const { return as<Case>()->isDefault(); }

        Case() { }
        Case(const Implementation* implementation)
          : ParseTreeObject(implementation) { }

        class Implementation : public ParseTreeObject::Implementation
        {
        public:
            Implementation(const Statement& statement, const Span& span)
              : ParseTreeObject::Implementation(span), _statement(statement)
            { }
            virtual bool isDefault() const = 0;
        private:
            Statement _statement;
        };
    private:
        class DefaultImplementation : public Implementation
        {
        public:
            DefaultImplementation(const Statement& statement, const Span& span)
              : Implementation(statement, span) { }
            bool isDefault() const { return true; }
        };
        class ValueImplementation : public Implementation
        {
        public:
            ValueImplementation(const List<Expression>& expressions,
                const Statement& statement, const Span& span)
              : Implementation(statement, span), _expressions(expressions) { }
            bool isDefault() const { return false; }
        private:
            List<Expression> _expressions;
        };
    };

    class Implementation : public Statement::Implementation
    {
    public:
        Implementation(const Expression& expression, const Case& defaultCase,
            const List<Case>& cases, const Span& span)
          : Statement::Implementation(span), _expression(expression),
            _defaultCase(defaultCase), _cases(cases) { }
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
        return new Implementation(expression, span);
    }
private:
    ReturnStatement() { }
    ReturnStatement(const Implementation* implementation)
      : Statement(implementation) { }

    class Implementation : public Statement::Implementation
    {
    public:
        Implementation(const Expression& expression, const Span& span)
          : Statement::Implementation(span), _expression(expression) { }
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
        return new Implementation(expression, span);
    }
private:
    IncludeStatement() { }
    IncludeStatement(const Implementation* implementation)
      : Statement(implementation) { }

    class Implementation : public Statement::Implementation
    {
    public:
        Implementation(const Expression& expression, const Span& span)
          : Statement::Implementation(span), _expression(expression) { }
    private:
        Expression _expression;
    };
};

template<class T> class BreakOrContinueStatementTemplate;
typedef BreakOrContinueStatementTemplate<void> BreakOrContinueStatement;

template<class T> class BreakOrContinueStatementTemplate : public Statement
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
    BreakOrContinueStatementTemplate() { }
    BreakOrContinueStatementTemplate(const Implementation* implementation)
      : Statement(implementation) { }

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
        return new BreakImplementation(statement, span);
    }
    
    static BreakOrContinueStatement parseContinue(CharacterSource* source)
    {
        Span span;
        if (!Space::parseKeyword(source, "continue", &span))
            return BreakOrContinueStatement();
        Space::assertCharacter(source, ';', &span);
        return new ContinueImplementation(span);
    }

    class Implementation : public Statement::Implementation
    {
    public:
        Implementation(const Span& span) : Statement::Implementation(span) { }
    };

    class BreakImplementation : public Implementation
    {
    public:
        BreakImplementation(const BreakOrContinueStatement& statement,
            const Span& span)
          : Implementation(span), _statement(statement) { }
    private:
        BreakOrContinueStatement _statement;
    };

    class ContinueImplementation : public Implementation
    {
    public:
        ContinueImplementation(const Span& span) : Implementation(span) { }
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
        return new Implementation(statement, span + statement.span());
    }
private:
    ForeverStatement() { }
    ForeverStatement(const Implementation* implementation)
      : Statement(implementation) { }

    class Implementation : public Statement::Implementation
    {
    public:
        Implementation(const Statement& statement, const Span& span)
          : Statement::Implementation(span), _statement(statement) { }
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
        return new Implementation(doStatement, condition, statement,
            doneStatement, span);
    }
private:
    WhileStatement() { }
    WhileStatement(const Implementation* implementation)
      : Statement(implementation) { }

    class Implementation : public Statement::Implementation
    {
    public:
        Implementation(const Statement& doStatement,
            const Expression& condition, const Statement& statement,
            const Statement& doneStatement, const Span& span)
          : Statement::Implementation(span), _doStatement(doStatement),
            _condition(condition), _statement(statement),
            _doneStatement(doneStatement) { }
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
        return new Implementation(preStatement, expression, postStatement,
            statement, doneStatement, span);
    }
private:
    ForStatement() { }
    ForStatement(const Implementation* implementation)
      : Statement(implementation) { }

    class Implementation : public Statement::Implementation
    {
    public:
        Implementation(const Statement& preStatement,
            const Expression& condition, const Statement& postStatement,
            const Statement& statement, const Statement& doneStatement,
            const Span& span)
          : Statement::Implementation(span), _preStatement(preStatement),
            _condition(condition), _postStatement(postStatement),
            _statement(statement), _doneStatement(doneStatement) { }
    private:
        Statement _preStatement;
        Expression _condition;
        Statement _postStatement;
        Statement _statement;
        Statement _doneStatement;
    };
};

class LabelStatement : public Statement
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
        return new Implementation(identifier, identifier.span() + span);
    }
private:
    LabelStatement() { }
    LabelStatement(const Implementation* implementation)
      : Statement(implementation) { }

    class Implementation : public Statement::Implementation
    {
    public:
        Implementation(const Identifier& identifier, const Span& span)
          : Statement::Implementation(span), _identifier(identifier) { }
    private:
        Identifier _identifier;
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
        return new Implementation(expression, span);
    }
private:
    GotoStatement() { }
    GotoStatement(const Implementation* implementation)
      : Statement(implementation) { }

    class Implementation : public Statement::Implementation
    {
    public:
        Implementation(const Expression& expression, const Span& span)
          : Statement::Implementation(span), _expression(expression) { }
    private:
        Expression _expression;
    };
};

class RunTimeStack;

class BuiltInStatement : public Statement
{
public:
    BuiltInStatement(void (*execute)(RunTimeStack* stack))
      : Statement(new Implementation(execute)) { }
protected:
    class Implementation : public Statement::Implementation
    {
    public:
        Implementation(void (*execute)(RunTimeStack* stack))
          : Statement::Implementation(Span()), _execute(execute) { }
    private:
        void (*_execute)(RunTimeStack* stack);
    };
};