class ExtricationStatement;

template<class T> class StatementTemplate : public ReferenceCounted
{
public:
    static Reference<StatementTemplate> parse(CharacterSource* source, Scope* scope)
    {
        Reference<Statement> s = ExpressionStatement::parse(source, scope);
        if (s.valid())
            return s;
        s = FunctionDeclarationStatement::parse(source, scope);
        if (s.valid())
            return s;
        s = VariableDeclarationStatement::parse(source, scope);
        if (s.valid())
            return s;
        s = AssignmentStatement::parse(source, scope);
        if (s.valid())
            return s;
        s = CompoundStatement::parse(source, scope);
        if (s.valid())
            return s;
        s = TypeAliasStatement::parse(source, scope);
        if (s.valid())
            return s;
        s = NothingStatement::parse(source, scope);
        if (s.valid())
            return s;
        s = IncrementStatement::parse(source, scope);
        if (s.valid())
            return s;
        s = ConditionalStatement::parse(source, scope);
        if (s.valid())
            return s;
        return 0;
    }
    virtual void resolveTypes() = 0;
    virtual void compile() = 0;
    virtual ExtricationStatement* run(Stack<Value>* stack) = 0;
};

typedef StatementTemplate<void> Statement;

class StatementSequence : public ReferenceCounted
{
public:
    static Reference<StatementSequence> parse(CharacterSource* source, Scope* scope)
    {
        Stack<Reference<Statement> > statements;
        do {
            Reference<Statement> statement = Statement::parse(source, scope);
            if (!statement.valid())
                break;
            statements.push(statement);
        } while (true);
        Reference<StatementSequence> statementSequence = new StatementSequence;
        statements.toArray(&statementSequence->_statements);
        return statementSequence;
    }
    void resolveTypes()
    {
        for (int i = 0; i < _statements.count(); ++i)
            _statements[i]->resolveTypes();
    }
    void compile()
    {
        for (int i = 0; i < _statements.count(); ++i)
            _statements[i]->compile();
    }
    ExtricationStatement* run(Stack<Value>* stack)
    {
        for (int i = 0; i < _statements.count(); ++i) {
            ExtricationStatement* statement = _statements[i]->run(stack);
            if (statement != 0)
                return statement;
        }
        return 0;
    }
private:
    StatementSequence() { }
    Array<Reference<Statement> > _statements;
};

class ExtricationStatement : public Statement
{
};

class ReturnStatement : public ExtricationStatement
{
public:
    static Reference<ReturnStatement> parse(CharacterSource* source, Scope* scope)
    {
        static String keyword("return");
        if (!Space::parseKeyword(source, keyword))
            return 0;
        Reference<Expression> expression = Expression::parse(source, scope);
        Space::assertCharacter(source, ';');
        return new ReturnStatement(expression);
    }
    void resolveTypes() { }
    void compile()
    {
        if (_expression.valid())
            _expression->compile();
    }
    ExtricationStatement* run(Stack<Value>* stack)
    {
        if (_expression.valid())
            _expression->push(stack);
        return this;
    }
private:
    ReturnStatement(Reference<Expression> expression) : _expression(expression)
    { }
    Reference<Expression> _expression;
};

class FunctionDeclarationStatement : public Statement
{
public:
    static Reference<FunctionDeclarationStatement> parse(CharacterSource* source, Scope* scope)
    {
        DiagnosticLocation location = source->location();
        CharacterSource s = *source;
        Reference<TypeSpecifier> returnTypeSpecifier = TypeSpecifier::parse(&s, scope);
        if (!returnTypeSpecifier.valid())
            return 0;
        Reference<Identifier> name = Identifier::parse(&s, scope);
        if (!name.valid())
            return 0;
        if (!Space::parseCharacter(&s, '('))
            return 0;
        Reference<Scope> inner = new Scope(scope, true);
        *source = s;
        Stack<Reference<Argument> > stack;
        do {
            Reference<Argument> argument = Argument::parse(source, inner);
            stack.push(argument);
            if (Space::parseCharacter(source, ')'))
                break;
            Space::assertCharacter(source, ',');
        } while (true);

        static String from("from");
        if (Space::parseKeyword(source, from)) {
            Reference<Expression> dll = Expression::parse(source, inner);
            if (dll.valid())
                return new FunctionDeclarationStatement(inner, returnTypeSpecifier, name, &stack, dll, location);
            static String error("Expected expression");
            source->location().throwError(error);
        }
        Reference<Statement> statement = Statement::parse(source, inner);
        if (!statement.valid()) {
            static String error("Expected statement");
            source->location().throwError(error);
        }
        return new FunctionDeclarationStatement(inner, returnTypeSpecifier, name, &stack, statement, location);
    }
    void resolveTypes()
    {
        _returnType = _returnTypeSpecifier->type();
        TypeList typeList;
        for (int i = 0; i < _arguments.count(); ++i) {
            _arguments[i]->addToScope(_scope);
            typeList.push(_arguments[i]->type());
        }
        typeList.finalize();
        _scope->outer()->addFunction(_name->name(), typeList, this, _location);
    }
    void compile()
    {
        if (_statement.valid())
            _statement->compile();
    }
    ExtricationStatement* run(Stack<Value>* stack) { return 0; }
    virtual Type returnType() const { return _returnTypeSpecifier->type(); }
    virtual void call(Stack<Value>* stack)
    {
        if (_statement.valid()) {
            for (int i = 0; i < _arguments.count(); ++i)
                _arguments[i]->setValue(stack->pop(), _scope);
            _statement->run(stack);
        }
    }
protected:
    FunctionDeclarationStatement() { }
private:
    class Argument : public ReferenceCounted
    {
    public:
        static Reference<Argument> parse(CharacterSource* source, Scope* scope)
        {
            DiagnosticLocation location = source->location();
            Reference<TypeSpecifier> typeSpecifier = TypeSpecifier::parse(source, scope);
            if (!typeSpecifier.valid()) {
                static String error("Expected type specifier");
                source->location().throwError(error);
            }
            Reference<Identifier> name = Identifier::parse(source, scope);
            if (!name.valid()) {
                static String error("Expected identifier");
                source->location().throwError(error);
            }
            return new Argument(typeSpecifier, name, location);
        }
        Type type() { return _typeSpecifier->type(); }
        void addToScope(Scope* scope)
        {
            scope->addVariable(_name->name(), type(), _location); 
        }
        void setValue(Value value, Scope* scope)
        {
            Reference<Variable> variable = scope->resolveSymbol(_name->name(), _location);
            variable->setValue(value);
        }
    private:
        Argument(Reference<TypeSpecifier> typeSpecifier, Reference<Identifier> name, DiagnosticLocation location)
          : _typeSpecifier(typeSpecifier), _name(name), _location(location)
        { }
        Reference<TypeSpecifier> _typeSpecifier;
        Reference<Identifier> _name;
        DiagnosticLocation _location;
    };
    FunctionDeclarationStatement(Reference<Scope> scope, Reference<TypeSpecifier> returnTypeSpecifier, Reference<Identifier> name, Stack<Reference<Argument> >* stack, Reference<Expression> dll, DiagnosticLocation location)
      : _scope(scope), _returnTypeSpecifier(returnTypeSpecifier), _name(name), _dll(dll), _location(location)
    {
        stack->toArray(&_arguments);
    }
    FunctionDeclarationStatement(Reference<Scope> scope, Reference<TypeSpecifier> returnTypeSpecifier, Reference<Identifier> name, Stack<Reference<Argument> >* stack, Reference<Statement> statement, DiagnosticLocation location)
      : _scope(scope), _returnTypeSpecifier(returnTypeSpecifier), _name(name), _statement(statement), _location(location)
    {
        stack->toArray(&_arguments);
    }

    Reference<Scope> _scope;
    Reference<TypeSpecifier> _returnTypeSpecifier;
    Reference<Identifier> _name;
    Array<Reference<Argument> > _arguments;
    Reference<Expression> _dll;
    Reference<Statement> _statement;
    Type _returnType;
    DiagnosticLocation _location;
};

class ExpressionStatement : public Statement
{
public:
    static Reference<ExpressionStatement> parse(CharacterSource* source, Scope* scope)
    {
        CharacterSource s = *source;
        Reference<Expression> expression = Expression::parse(&s, scope);
        if (!expression.valid())
            return 0;
        if (!Space::parseCharacter(&s, ';'))
            return 0;
        *source = s;
        FunctionCallExpression* functionCallExpression = dynamic_cast<FunctionCallExpression*>(static_cast<Expression*>(expression));
        if (functionCallExpression == 0) {
            static String error("Statement has no effect");
            source->location().throwError(error);
        }
        return new ExpressionStatement(expression);
    }
    void resolveTypes() { }
    void compile()
    {
        _expression->compile();
    }
    ExtricationStatement* run(Stack<Value>* stack)
    {
        _expression->push(stack);
        if (_expression->type() != VoidType())
            stack->pop();
        return 0;
    }
private:
    ExpressionStatement(Reference<Expression> expression)
      : _expression(expression)
    { }
    Reference<Expression> _expression;
};

class VariableDeclarationStatement : public Statement
{
public:
    static Reference<VariableDeclarationStatement> parse(CharacterSource* source, Scope* scope)
    {
        Reference<Scope> inner = new Scope(scope);
        DiagnosticLocation location = source->location();
        CharacterSource s = *source;
        Reference<TypeSpecifier> typeSpecifier = TypeSpecifier::parse(&s, inner);
        if (!typeSpecifier.valid())
            return 0;
        Reference<Identifier> identifier = Identifier::parse(&s, inner);
        if (!identifier.valid())
            return 0;
        *source = s;
        Reference<Expression> initializer;
        if (Space::parseCharacter(source, '=')) {
            initializer = Expression::parse(source, inner);
            if (!initializer.valid()) {
                static String expression("Expected expression");
                source->location().throwError(expression);
            }
        }
        Space::assertCharacter(source, ';');
        return new VariableDeclarationStatement(inner, typeSpecifier, identifier, initializer, location);
    }
    void resolveTypes()
    {
        _typeSpecifier->type();
    }
    void compile()
    {
        Type type = _typeSpecifier->type();
        AutoTypeSpecifier* autoTypeSpecifier = dynamic_cast<AutoTypeSpecifier*>(static_cast<TypeSpecifier*>(_typeSpecifier));
        if (autoTypeSpecifier != 0)
            type = _initializer->type();
        _initializer->compile();
        _variable = _scope->outer()->addVariable(_identifier->name(), type, _location);
    }
    ExtricationStatement* run(Stack<Value>* stack)
    {
        if (_initializer.valid()) {
            _initializer->push(stack);
            _variable->setValue(stack->pop());
        }
        return 0;
    }
private:
    VariableDeclarationStatement(Reference<Scope> scope, Reference<TypeSpecifier> typeSpecifier, Reference<Identifier> identifier, Reference<Expression> initializer, DiagnosticLocation location)
      : _scope(scope), _typeSpecifier(typeSpecifier), _identifier(identifier), _initializer(initializer), _location(location)
    { }
    Reference<Scope> _scope;
    Reference<TypeSpecifier> _typeSpecifier;
    Reference<Identifier> _identifier;
    Reference<Expression> _initializer;
    DiagnosticLocation _location;
    Reference<Variable> _variable;
};

class AssignmentStatement : public Statement
{
public:
    static Reference<AssignmentStatement> parse(CharacterSource* source, Scope* scope)
    {
        DiagnosticLocation location = source->location();
        CharacterSource s = *source;
        Reference<Expression> lValue = Expression::parse(&s, scope);
        DiagnosticLocation operatorLocation = s.location();
        if (!lValue.valid())
            return 0;
        int type = 0;
        static String addAssignment("+=");
        static String subtractAssignment("-=");
        static String multiplyAssignment("*=");
        static String divideAssignment("/=");
        static String moduloAssignment("%=");
        static String shiftLeftAssignment("<<=");
        static String shiftRightAssignment(">>=");
        static String andAssignment("&=");
        static String orAssignment("|=");
        static String xorAssignment("~=");
        static String powerAssignment("^=");

        if (Space::parseCharacter(&s, '='))
            type = 1;
        else if (Space::parseOperator(&s, addAssignment))
            type = 2;
        else if (Space::parseOperator(&s, subtractAssignment))
            type = 3;
        else if (Space::parseOperator(&s, multiplyAssignment))
            type = 4;
        else if (Space::parseOperator(&s, divideAssignment))
            type = 5;
        else if (Space::parseOperator(&s, moduloAssignment))
            type = 6;
        else if (Space::parseOperator(&s, shiftLeftAssignment))
            type = 7;
        else if (Space::parseOperator(&s, shiftRightAssignment))
            type = 8;
        else if (Space::parseOperator(&s, andAssignment))
            type = 9;
        else if (Space::parseOperator(&s, orAssignment))
            type = 10;
        else if (Space::parseOperator(&s, xorAssignment))
            type = 11;
        else if (Space::parseOperator(&s, powerAssignment))
            type = 12;
        if (type == 0)
            return 0;

        *source = s;
        Reference<Expression> e = Expression::parse(source, scope);
        if (!e.valid()) {
            static String expression("Expected expression");
            source->location().throwError(expression);
        }
        Space::assertCharacter(source, ';');
        switch (type) {
            case 2:
                e = new AddExpression(lValue, e, operatorLocation);
                break;
            case 3:
                e = new SubtractExpression(lValue, e, operatorLocation);
                break;
            case 4:
                e = new MultiplyExpression(lValue, e, operatorLocation);
                break;
            case 5:
                e = new DivideExpression(lValue, e, operatorLocation);
                break;
            case 6:
                e = new ModuloExpression(lValue, e, operatorLocation);
                break;
            case 7:
                e = new ShiftLeftExpression(lValue, e, operatorLocation);
                break;
            case 8:
                e = new ShiftRightExpression(lValue, e, operatorLocation);
                break;
            case 9:
                e = new BitwiseAndExpression(lValue, e, operatorLocation);
                break;
            case 10:
                e = new BitwiseOrExpression(lValue, e, operatorLocation);
                break;
            case 11:
                e = new BitwiseXorExpression(lValue, e, operatorLocation);
                break;
            case 12:
                e = new PowerExpression(lValue, e, operatorLocation);
                break;
        }
        return new AssignmentStatement(scope, lValue, e, location);
    }
    void resolveTypes() { }
    void compile()
    {
        _lValue->compile();
        _value->compile();
        if (!_lValue->isLValue()) {
            static String error("LValue required");
            _location.throwError(error);
        }
        Type l = _lValue->type();
        Type r = _value->type();
        if (l != r) {
            static String error1("Cannot convert ");
            static String error2(" to ");
            _location.throwError(error1 + l.toString() + error2 + r.toString());
        }
    }
    ExtricationStatement* run(Stack<Value>* stack)
    {
        _value->push(stack);
        _lValue->variable(stack)->setValue(stack->pop());
        return 0;
    }
private:
    AssignmentStatement(Scope* scope, Reference<Expression> lValue, Reference<Expression> value, DiagnosticLocation location)
      : _scope(scope), _lValue(lValue), _value(value), _location(location)
    { }
    Scope* _scope;
    Reference<Expression> _lValue;
    Reference<Expression> _value;
    DiagnosticLocation _location;
};

class PrintFunction : public FunctionDeclarationStatement
{
public:
    PrintFunction() : _consoleOutput(Handle::consoleOutput())
    { }
    void resolveTypes() { }
    void compile() { }
    void call(Stack<Value>* stack)
    {
        stack->pop().getString().write(_consoleOutput);
    }
    Type returnType() const { return VoidType(); }
private:
    Handle _consoleOutput;
};

class CompoundStatement : public Statement
{
public:
    static Reference<CompoundStatement> parse(CharacterSource* source, Scope* scope)
    {
        if (!Space::parseCharacter(source, '{'))
            return 0;
        Reference<Scope> inner = new Scope(scope, true);
        Reference<StatementSequence> sequence = StatementSequence::parse(source, inner);
        Space::assertCharacter(source, '}');
        return new CompoundStatement(inner, sequence);
    }
    void resolveTypes()
    {
        _sequence->resolveTypes();
    }
    void compile()
    {
        _sequence->compile();
    }
    ExtricationStatement* run(Stack<Value>* stack)
    {
        return _sequence->run(stack);
    }
private:
    CompoundStatement(Reference<Scope> scope, Reference<StatementSequence> sequence)
      : _scope(scope), _sequence(sequence)
    { }
    Reference<Scope> _scope;
    Reference<StatementSequence> _sequence;
};

class TypeDefinitionStatement : public Statement
{
public:
    void resolveTypes() { }
    void compile() { }
    ExtricationStatement* run(Stack<Value>* stack) { return 0; }
    virtual Type type() = 0;
};

class TypeAliasStatement : public TypeDefinitionStatement
{
public:
    static Reference<TypeAliasStatement> parse(CharacterSource* source, Scope* scope)
    {
        CharacterSource s = *source;
        CharacterSource s2 = s;
        Reference<TypeIdentifier> typeIdentifier = TypeIdentifier::parse(&s, scope);
        if (!typeIdentifier.valid())
            return 0;
        if (!Space::parseCharacter(&s, '='))
            return 0;
        *source = s;
        Reference<TypeSpecifier> typeSpecifier = TypeSpecifier::parse(source, scope);
        Space::assertCharacter(source, ';');
        Reference<TypeAliasStatement> typeAliasStatement = new TypeAliasStatement(scope, typeIdentifier, typeSpecifier);
        scope->addType(typeIdentifier->name(), typeAliasStatement, s2.location());
        return typeAliasStatement;
    }
    void resolveTypes()
    {
        _typeIdentifier->type();
    }
    Type type() { return _typeSpecifier->type(); }
private:
    TypeAliasStatement(Scope* scope, Reference<TypeIdentifier> typeIdentifier, Reference<TypeSpecifier> typeSpecifier)
      : _scope(scope), _typeIdentifier(typeIdentifier), _typeSpecifier(typeSpecifier)
    { }
    Scope* _scope;
    Reference<TypeIdentifier> _typeIdentifier;
    Reference<TypeSpecifier> _typeSpecifier;
};

class IncludeStatement : public Statement
{
public:
    static Reference<IncludeStatement> parse(CharacterSource* source, Scope* scope)
    {
        static String include("include");
        if (!Space::parseKeyword(source, include))
            return 0;
        Reference<Expression> expression = Expression::parse(source, scope);
        if (!expression.valid()) {
            static String error("Expected expression");
            source->location().throwError(error);
        }
        Space::assertCharacter(source, ';');
        return new IncludeStatement(scope, expression);
    }
    void resolveTypes() { }
    void compile() { }
    ExtricationStatement* run(Stack<Value>* stack) { return 0; }
private:
    IncludeStatement(Scope* scope, Reference<Expression> expression)
      : _scope(scope), _expression(expression)
    { }
    Scope* _scope;
    Reference<Expression> _expression;
};

class NothingStatement : public Statement
{
public:
    static Reference<NothingStatement> parse(CharacterSource* source, Scope* scope)
    {
        static String nothing("nothing");
        if (!Space::parseKeyword(source, nothing))
            return 0;
        Space::assertCharacter(source, ';');
        return new NothingStatement();
    }
    void resolveTypes() { }
    void compile() { }
    ExtricationStatement* run(Stack<Value>* stack) { return 0; }
};

class BreakStatement;

class ContinueStatement;

template<class T> class BreakOrContinueStatementTemplate;

typedef BreakOrContinueStatementTemplate<void> BreakOrContinueStatement;

template<class T> class BreakOrContinueStatementTemplate : public ExtricationStatement
{
public:
    static Reference<BreakOrContinueStatement> parse(CharacterSource* source, Scope* scope)
    {
        Reference<BreakStatement> breakStatement = BreakStatement::parse(source, scope);
        if (breakStatement.valid())
            return breakStatement;
        return ContinueStatement::parse(source, scope);
    }
    void resolveTypes() { }
    void compile() { }
    ExtricationStatement* run(Stack<Value>* stack) { return this; }
};

class BreakStatement : public BreakOrContinueStatement
{
public:
    static Reference<BreakStatement> parse(CharacterSource* source, Scope* scope)
    {
        static String keyword("break");
        if (!Space::parseKeyword(source, keyword))
            return 0;
        Reference<BreakOrContinueStatement> statement = BreakOrContinueStatement::parse(source, scope);
        if (!statement.valid())
            Space::assertCharacter(source, ';');
        return new BreakStatement(statement);
    }
    BreakOrContinueStatement* nextAction() { return _statement; }
private:
    BreakStatement(Reference<BreakOrContinueStatement> statement) : _statement(statement) { }
    Reference<BreakOrContinueStatement> _statement;
};

class ContinueStatement : public BreakOrContinueStatement
{
public:
    static Reference<ContinueStatement> parse(CharacterSource* source, Scope* scope)
    {
        static String keyword("continue");
        if (!Space::parseKeyword(source, keyword))
            return 0;
        Space::assertCharacter(source, ';');
        return new ContinueStatement();
    }
};

class ForeverStatement : public Statement
{
public:
    static Reference<ForeverStatement> parse(CharacterSource* source, Scope* scope)
    {
        static String forever("forever");
        if (!Space::parseKeyword(source, forever))
            return 0;
        Reference<Statement> statement = Statement::parse(source, scope);
        if (!statement.valid()) {
            static String error("Expected statement");
            source->location().throwError(error);
        }
        return new ForeverStatement(statement);
    }
    void resolveTypes() { _statement->resolveTypes(); }
    void compile() { _statement->compile(); }
    ExtricationStatement* run(Stack<Value>* stack)
    {
        do {
            ExtricationStatement* statement = _statement->run(stack);
            BreakStatement* breakStatement = dynamic_cast<BreakStatement*>(statement);
            if (breakStatement != 0)
                return breakStatement->nextAction();
            ContinueStatement* continueStatement = dynamic_cast<ContinueStatement*>(statement);
            if (continueStatement != 0)
                continue;
            return statement;  // must be a throw or return statement
        } while (true);
    }
public:
    ForeverStatement(Reference<Statement> statement)
      : _statement(statement)
    { }
    Reference<Statement> _statement;
};

class IncrementStatement : public Statement
{
public:
    static Reference<IncrementStatement> parse(CharacterSource* source, Scope* scope)
    {
        static String incrementOperator("++");
        if (!Space::parseOperator(source, incrementOperator))
            return 0;
        DiagnosticLocation location = source->location();
        CharacterSource s = *source;
        Reference<Expression> lValue = Expression::parse(&s, scope);
        *source = s;
        Space::assertCharacter(source, ';');
        return new IncrementStatement(scope, lValue, location);
    }
    void resolveTypes() { _lValue->type(); }
    void compile()
    { 
        _lValue->compile();
        if (_lValue->isLValue()) {
            static String error("LValue required");
            _location.throwError(error);
        }
    }
    ExtricationStatement* run(Stack<Value>* stack)
    { 
        _lValue->push(stack);
        _lValue->variable(stack)->setValue(Value(stack->pop().getInt() + 1));
        return 0;
    }
private:
    IncrementStatement(Scope* scope, Reference<Expression> lValue, DiagnosticLocation location)
      : _scope(scope), _lValue(lValue), _location(location)
    { }
    Scope* _scope;
    Reference<Expression> _lValue;
    DiagnosticLocation _location;
};

class DecrementStatement : public Statement
{
public:
    static Reference<DecrementStatement> parse(CharacterSource* source, Scope* scope)
    {
        static String decrementOperator("--");
        if (!Space::parseOperator(source, decrementOperator))
            return 0;
        DiagnosticLocation location = source->location();
        CharacterSource s = *source;
        Reference<Expression> lValue = Expression::parse(&s, scope);
        *source = s;
        Space::assertCharacter(source, ';');
        return new DecrementStatement(scope, lValue, location);
    }
    void resolveTypes() { _lValue->type(); }
    void compile()
    { 
        _lValue->compile();
        if (_lValue->isLValue()) {
            static String error("LValue required");
            _location.throwError(error);
        }
    }
    ExtricationStatement* run(Stack<Value>* stack)
    { 
        _lValue->push(stack);
        _lValue->variable(stack)->setValue(Value(stack->pop().getInt() - 1));
        return 0;
    }
private:
    DecrementStatement(Scope* scope, Reference<Expression> lValue, DiagnosticLocation location)
      : _scope(scope), _lValue(lValue), _location(location)
    { }
    Scope* _scope;
    Reference<Expression> _lValue;
    DiagnosticLocation _location;
};

class ConditionalStatement : public Statement
{
public:
    static Reference<ConditionalStatement> parse(CharacterSource* source, Scope* scope)
    {
        static String ifKeyword("if");
        static String unlessKeyword("unless");
        if (Space::parseKeyword(source, ifKeyword))
            return parse(source, scope, false);
        if (Space::parseKeyword(source, unlessKeyword))
            return parse(source, scope, true);
        return 0;
    }
    void resolveTypes()
    {
        if (_trueStatement.valid())
            _trueStatement->resolveTypes();
        if (_falseStatement.valid())
            _falseStatement->resolveTypes();
    }
    void compile()
    {
        _condition->compile();
        if (_condition->type() != BooleanType()) {
            static String error("Boolean expression expected");
            _location.throwError(error);
        }
        if (_trueStatement.valid())
            _trueStatement->compile();
        if (_falseStatement.valid())
            _falseStatement->compile();
    }
    ExtricationStatement* run(Stack<Value>* stack)
    {
        _condition->push(stack);
        bool result = (stack->pop().getInt() != 0);
        if (result) {
            if (_trueStatement.valid())
                return _trueStatement->run(stack);
        }
        else
            if (_falseStatement.valid())
                return _falseStatement->run(stack);
        return 0;
    }
private:
    ConditionalStatement(DiagnosticLocation location, Reference<Expression> condition, Reference<Statement> trueStatement, Reference<Statement> falseStatement)
        : _location(location), _condition(condition), _trueStatement(trueStatement), _falseStatement(falseStatement)
    { }
    static Reference<ConditionalStatement> parse(CharacterSource* source, Scope* scope, bool unlessStatement)
    {
        static String elseKeyword("else");
        static String elseIfKeyword("elseIf");
        static String elseUnlessKeyword("elseUnless");
        Space::assertCharacter(source, '(');
        DiagnosticLocation location = source->location();
        Reference<Expression> condition = Expression::parse(source, scope);
        if (!condition.valid()) {
            static String error("Expected expression");
            source->location().throwError(error);
        }
        Space::assertCharacter(source, ')');
        Reference<Statement> conditionedStatement = Statement::parse(source, scope);
        static String expectedStatement("Expected statement");
        if (!conditionedStatement.valid())
            source->location().throwError(expectedStatement);
        Reference<Statement> elseStatement;
        if (Space::parseKeyword(source, elseKeyword)) {
            elseStatement = Statement::parse(source, scope);
            if (!elseStatement.valid())
                source->location().throwError(expectedStatement);
        }
        else
            if (Space::parseKeyword(source, elseIfKeyword))
                elseStatement = parse(source, scope, false);
            else
                if (Space::parseKeyword(source, elseUnlessKeyword))
                    elseStatement = parse(source, scope, true);
        if (unlessStatement)
            return new ConditionalStatement(location, condition, elseStatement, conditionedStatement);
        return new ConditionalStatement(location, condition, conditionedStatement, elseStatement);
    }

    DiagnosticLocation _location;
    Reference<Expression> _condition;
    Reference<Statement> _trueStatement;
    Reference<Statement> _falseStatement;
};

class StringTypeDefinitionStatement : public TypeDefinitionStatement
{
public:
    Type type() { return StringType(); }
};

class IntTypeDefinitionStatement : public TypeDefinitionStatement
{
public:
    Type type() { return IntType(); }
};

class VoidTypeDefinitionStatement : public TypeDefinitionStatement
{
public:
    Type type() { return VoidType(); }
};

class BooleanTypeDefinitionStatement : public TypeDefinitionStatement
{
public:
    Type type() { return BooleanType(); }
};

