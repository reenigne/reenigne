template<class T> class StatementTemplate : public ReferenceCounted
{
public:
    static Reference<StatementTemplate> parse(CharacterSource* source, Scope* scope)
    {
        Reference<Statement> s = FunctionCallStatement::parse(source, scope);
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
        return 0;
    }
    virtual void resolveTypes() = 0;
    virtual void compile() = 0;
    virtual void run(Stack<Value>* stack) = 0;
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
    void run(Stack<Value>* stack)
    {
        for (int i = 0; i < _statements.count(); ++i)
            _statements[i]->run(stack);
    }
private:
    StatementSequence() { }
    Array<Reference<Statement> > _statements;
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

        Reference<Identifier> identifier = Identifier::parse(source, inner);
        static String from("from");
        if (identifier.valid() && identifier->name() == from) {
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
    void run(Stack<Value>* stack) { }
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

class FunctionCallStatement : public Statement
{
public:
    static Reference<FunctionCallStatement> parse(CharacterSource* source, Scope* scope)
    {
        CharacterSource s = *source;
        DiagnosticLocation location = s.location();
        Reference<Identifier> functionName = Identifier::parse(&s, scope);
        if (!functionName.valid())
            return 0;
        if (!Space::parseCharacter(&s, '('))
            return 0;
        int n = 0;
        Stack<Reference<Expression> > stack;
        *source = s;
        if (!Space::parseCharacter(source, ')')) {
            do {
                Reference<Expression> e = Expression::parse(source, scope);
                if (!e.valid()) {
                    static String expression("Expected expression");
                    source->location().throwError(expression);
                }
                stack.push(e);
                ++n;
                if (Space::parseCharacter(source, ')'))
                    break;
                Space::assertCharacter(source, ',');
            } while (true);
        }
        Space::assertCharacter(source, ';');
        Reference<FunctionCallStatement> functionCall = new FunctionCallStatement(scope, functionName, n, location);
        stack.toArray(&functionCall->_arguments);
        return functionCall;
    }
    void resolveTypes() { }
    void compile()
    {
        TypeList argumentTypes;
        for (int i = 0; i < _arguments.count(); ++i) {
            _arguments[i]->compile();
            argumentTypes.push(_arguments[i]->type());
        }
        argumentTypes.finalize();
        _function = _scope->resolveFunction(_functionName, argumentTypes, _location);
    }
    void run(Stack<Value>* stack)
    {
        for (int i = _arguments.count() - 1; i >= 0; --i)
            _arguments[i]->push(stack);
        _function->call(stack);
    }
private:
    FunctionCallStatement(Scope* scope, Reference<Identifier> functionName, int n, DiagnosticLocation location)
      : _scope(scope), _functionName(functionName), _location(location)
    {
        _arguments.allocate(n);
    }

    Scope* _scope;
    Reference<Identifier> _functionName;
    FunctionDeclarationStatement* _function;
    Array<Reference<Expression> > _arguments;
    DiagnosticLocation _location;
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
        _variable = _scope->outer()->addVariable(_identifier->name(), _typeSpecifier->type(), _location);
    }
    void run(Stack<Value>* stack)
    {
        if (_initializer.valid()) {
            _initializer->push(stack);
            _variable->setValue(stack->pop());
        }
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
        static String subtractAssignment("+=");
        static String multiplyAssignment("*=");
        static String divideAssignment("/=");
        static String moduloAssignment("%=");
        static String shiftLeftAssignment("<<=");
        static String shiftRightAssignment(">>=");
        static String andAssignment("&=");
        static String orAssignment("|=");
        static String xorAssignment("~=");
        static String powerAssignment("^=");

        if (!Space::parseCharacter(&s, '='))
            type = 1;
        else if (!Space::parseOperator(&s, addAssignment))
            type = 2;
        else if (!Space::parseOperator(&s, subtractAssignment))
            type = 3;
        else if (!Space::parseOperator(&s, multiplyAssignment))
            type = 4;
        else if (!Space::parseOperator(&s, divideAssignment))
            type = 5;
        else if (!Space::parseOperator(&s, moduloAssignment))
            type = 6;
        else if (!Space::parseOperator(&s, shiftLeftAssignment))
            type = 7;
        else if (!Space::parseOperator(&s, shiftRightAssignment))
            type = 8;
        else if (!Space::parseOperator(&s, andAssignment))
            type = 9;
        else if (!Space::parseOperator(&s, orAssignment))
            type = 10;
        else if (!Space::parseOperator(&s, xorAssignment))
            type = 11;
        else if (!Space::parseOperator(&s, powerAssignment))
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
    void resolveTypes()
    {
        Type l = _lValue->type();
        Type r = _value->type();
        if (l != r) {
            static String error1("Cannot convert ");
            static String error2(" to ");
            _location.throwError(error1 + l.toString() + error2 + r.toString());
        }
    }
    void compile()
    {
        if (!_lValue->isLValue()) {
            static String error("LValue required");
            _location.throwError(error);
        }
    }
    void run(Stack<Value>* stack)
    {
        _value->push(stack);
        _variable->setValue(stack->pop());
    }
private:
    AssignmentStatement(Scope* scope, Reference<Expression> lValue, Reference<Expression> value, DiagnosticLocation location)
      : _scope(scope), _lValue(lValue), _value(value), _location(location)
    { }
    Scope* _scope;
    Reference<Expression> _lValue;
    Reference<Expression> _value;
    DiagnosticLocation _location;
    Reference<Variable> _variable;
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
    void run(Stack<Value>* stack)
    {
        _sequence->run(stack);
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
    void run(Stack<Value>* stack) { }
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
        Reference<Identifier> identifier = Identifier::parse(source, scope);
        static String include("include");
        if (!identifier.valid() || identifier->name() != include)
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
    void run(Stack<Value>* stack) { }
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
        Reference<Identifier> identifier = Identifier::parse(source, scope);
        static String nothing("nothing");
        if (!identifier.valid() || identifier->name() != nothing)
            return 0;
        Space::assertCharacter(source, ';');
        return new NothingStatement();
    }
    void resolveTypes() { }
    void compile() { }
    void run(Stack<Value>* stack) { }
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
    void run(Stack<Value>* stack)
    { 
        _lValue->push(stack);
        _lValue->setValue(stack, Value(stack->pop().getInt() + 1));
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
    void run(Stack<Value>* stack)
    { 
        _lValue->push(stack);
        _lValue->setValue(stack, Value(stack->pop().getInt() - 1));
    }
private:
    DecrementStatement(Scope* scope, Reference<Expression> lValue, DiagnosticLocation location)
      : _scope(scope), _lValue(lValue), _location(location)
    { }
    Scope* _scope;
    Reference<Expression> _lValue;
    DiagnosticLocation _location;
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

