class Compiler
{
public:
    void compileFunction(Symbol functionDefinitionStatement)
    {
        int stackAdjust = offsetOf(functionDefinitionStatement);
        if (stackAdjust != 0) {
            add(Symbol(atomStackPointer));
            add(Symbol(atomIntegerConstant, -stackAdjust));
            add(Symbol(atomAdd));
            add(Symbol(atomSetStackPointer));
        }
        _stackOffset = 0;
        compileStatementSequence(functionDefinitionStatement[4].array());
        if (stackAdjust != 0) {
            add(Symbol(atomStackPointer));
            add(Symbol(atomIntegerConstant, stackAdjust));
            add(Symbol(atomAdd));
            add(Symbol(atomSetStackPointer));
        }
        add(Symbol(atomReturn));
    }
    SymbolList compiledProgram() const { return _compiledProgram; }
private:
    void compileStatementSequence(SymbolArray program)
    {
        for (int i = 0; i < program.count(); ++i)
            compileStatement(program[i]);
    }
    void finishBasicBlock(int nextLabel)
    {
        Symbol block(atomBasicBlock, SymbolArray(_basicBlock), _label, nextLabel);
        block.setLabel(_label);
        _compiledProgram.add(block);
        _basicBlock = SymbolList();
        _label = nextLabel;
    }
    void compileStatement(Symbol statement)
    {
        switch (statement.atom()) {
            case atomExpressionStatement:
                compileExpression(statement[1].symbol());
                break;            
            case atomFunctionDefinitionStatement:
                {
                    Compiler compiler;
                    compiler.compileFunction(statement);
                    _compiledProgram.add(compiler.compiledProgram());
                }
                break;
            case atomFromStatement:
                // TODO
                break;
            case atomVariableDefinitionStatement:
                addAddressOf(statement[2].symbol());
                compileExpression(statement[3].symbol());
                add(Symbol(atomStore));
                break;
            case atomAssignmentStatement:
                addAddressOf(statement[1].symbol());
                compileExpression(statement[2].symbol());
                add(Symbol(atomStore));
                break;
            case atomAddAssignmentStatement:
                addAddressOf(statement[1].symbol());
                add(Symbol(atomDuplicate));
                add(Symbol(atomDereference));
                compileExpression(statement[2].symbol());
                add(Symbol(atomAdd));
                add(Symbol(atomStore));
                break;
            case atomSubtractAssignmentStatement:
                addAddressOf(statement[1].symbol());
                add(Symbol(atomDuplicate));
                add(Symbol(atomDereference));
                compileExpression(statement[2].symbol());
                add(Symbol(atomSubtract));
                add(Symbol(atomStore));
                break;
            case atomMultiplyAssignmentStatement:
                addAddressOf(statement[1].symbol());
                add(Symbol(atomDuplicate));
                add(Symbol(atomDereference));
                compileExpression(statement[2].symbol());
                add(Symbol(atomMultiply));
                add(Symbol(atomStore));
                break;
            case atomDivideAssignmentStatement:
                addAddressOf(statement[1].symbol());
                add(Symbol(atomDuplicate));
                add(Symbol(atomDereference));
                compileExpression(statement[2].symbol());
                add(Symbol(atomDivide));
                add(Symbol(atomStore));
                break;
            case atomModuloAssignmentStatement:
                addAddressOf(statement[1].symbol());
                add(Symbol(atomDuplicate));
                add(Symbol(atomDereference));
                compileExpression(statement[2].symbol());
                add(Symbol(atomModulo));
                add(Symbol(atomStore));
                break;
            case atomShiftLeftAssignmentStatement:
                addAddressOf(statement[1].symbol());
                add(Symbol(atomDuplicate));
                add(Symbol(atomDereference));
                compileExpression(statement[2].symbol());
                add(Symbol(atomLeftShift));
                add(Symbol(atomStore));
                break;
            case atomShiftRightAssignmentStatement:
                addAddressOf(statement[1].symbol());
                add(Symbol(atomDuplicate));
                add(Symbol(atomDereference));
                compileExpression(statement[2].symbol());
                add(Symbol(atomRightShift));
                add(Symbol(atomStore));
                break;
            case atomAndAssignmentStatement:
                addAddressOf(statement[1].symbol());
                add(Symbol(atomDuplicate));
                add(Symbol(atomDereference));
                compileExpression(statement[2].symbol());
                add(Symbol(atomBitwiseAnd));
                add(Symbol(atomStore));
                break;
            case atomOrAssignmentStatement:
                addAddressOf(statement[1].symbol());
                add(Symbol(atomDuplicate));
                add(Symbol(atomDereference));
                compileExpression(statement[2].symbol());
                add(Symbol(atomBitwiseOr));
                add(Symbol(atomStore));
                break;
            case atomXorAssignmentStatement:
                addAddressOf(statement[1].symbol());
                add(Symbol(atomDuplicate));
                add(Symbol(atomDereference));
                compileExpression(statement[2].symbol());
                add(Symbol(atomBitwiseXor));
                add(Symbol(atomStore));
                break;
            case atomPowerAssignmentStatement:
                addAddressOf(statement[1].symbol());
                add(Symbol(atomDuplicate));
                add(Symbol(atomDereference));
                compileExpression(statement[2].symbol());
                add(Symbol(atomPower));
                add(Symbol(atomStore));
                break;
            case atomCompoundStatement:
                compileStatementSequence(statement[1].array());
                break;
            case atomTypeAliasStatement:
                break;
            case atomNothingStatement:
                break;
            case atomIncrementStatement:
                addAddressOf(statement[1].symbol());
                add(Symbol(atomDuplicate));
                add(Symbol(atomDereference));
                add(Symbol(atomIntegerConstant, 1));
                add(Symbol(atomAdd));
                add(Symbol(atomStore));
                break;
            case atomDecrementStatement:
                addAddressOf(statement[1].symbol());
                add(Symbol(atomDuplicate));
                add(Symbol(atomDereference));
                add(Symbol(atomIntegerConstant, 1));
                add(Symbol(atomSubtract));
                add(Symbol(atomStore));
                break;
            case atomIfStatement:
                {
                    int falseClause = Symbol::newLabel();
                    int done = Symbol::newLabel();
                    compileExpression(statement[1].symbol());
                    add(Symbol(atomNot));
                    addJumpIfTrue(falseClause);
                    compileStatement(statement[2].symbol());
                    addGoto(done);
                    addLabel(falseClause);
                    compileStatement(statement[3].symbol());
                    addLabel(done);
                }
                break;
            case atomSwitchStatement:
            case atomReturnStatement:
            case atomIncludeStatement:
            case atomBreakStatement:
            case atomContinueStatement:
                // TODO
                break;
            case atomForeverStatement:
                {
                    int done = Symbol::newLabel();
                    int start = getLabel();
                    _breakContinueStack.push(BreakContinueStackEntry(done, start));
                    compileStatement(statement[1].symbol());
                    addGoto(start);
                    addLabel(done);
                    _breakContinueStack.pop();
                }
                break;
            case atomWhileStatement:
                {
                    int done = Symbol::newLabel();
                    int final = Symbol::newLabel();
                    int start = getLabel();
                    _breakContinueStack.push(BreakContinueStackEntry(final, start));
                    compileStatement(statement[1].symbol());
                    compileExpression(statement[2].symbol());
                    add(Symbol(atomNot));
                    addJumpIfTrue(done);
                    compileStatement(statement[3].symbol());
                    addGoto(start);
                    _breakContinueStack.pop();
                    addLabel(done);
                    compileStatement(statement[4].symbol());
                }
                break;
            case atomUntilStatement:
                {
                    int done = Symbol::newLabel();
                    int final = Symbol::newLabel();
                    int start = getLabel();
                    _breakContinueStack.push(BreakContinueStackEntry(final, start));
                    compileStatement(statement[1].symbol());
                    compileExpression(statement[2].symbol());
                    addJumpIfTrue(done);
                    compileStatement(statement[3].symbol());
                    addGoto(start);
                    _breakContinueStack.pop();
                    addLabel(done);
                    compileStatement(statement[4].symbol());
                }
                break;
            case atomForStatement:
                {
                    compileStatement(statement[1].symbol());
                    int done = Symbol::newLabel();
                    int final = Symbol::newLabel();
                    int start = getLabel();
                    _breakContinueStack.push(BreakContinueStackEntry(final, start));
                    compileExpression(statement[2].symbol());
                    add(Symbol(atomNot));
                    addJumpIfTrue(done);
                    compileStatement(statement[3].symbol());
                    compileStatement(statement[4].symbol());
                    addGoto(start);
                    _breakContinueStack.pop();
                    addLabel(done);
                    compileStatement(statement[5].symbol());
                }
                break;
        }
    }
    // Add instructions to push the value of expression onto the stack.
    void compileExpression(Symbol expression)
    {
        switch (expression.atom()) {
            case atomLogicalOr:
                {
                    int pushRight = Symbol::newLabel();
                    int pushTrue = Symbol::newLabel();
                    int done = Symbol::newLabel();
                    compileExpression(expression[1].symbol());
                    addJumpIfTrue(pushTrue);
                    addLabel(pushRight);
                    compileExpression(expression[2].symbol());
                    addGoto(done);
                    addLabel(pushTrue);
                    add(Symbol(atomTrue));
                    addLabel(done);
                }
                break;
            case atomLogicalAnd:
                {
                    int pushRight = Symbol::newLabel();
                    int pushFalse = Symbol::newLabel();
                    int done = Symbol::newLabel();
                    compileExpression(expression[1].symbol());
                    add(Symbol(atomNot));
                    addJumpIfTrue(pushFalse);
                    compileExpression(expression[2].symbol());
                    addGoto(done);
                    addLabel(pushFalse);
                    add(Symbol(atomFalse));
                    addLabel(done);
                }
                break;
            case atomDot:
                // TODO
                break;
            case atomDereference:
                addAddressOf(expression[1].symbol());
                add(Symbol(atomDereference));
                break;
            case atomAddressOf:
                addAddressOf(expression[1].symbol());
                break;
            case atomFunctionCall:
                {
                    _stackOffsetStack.push(_stackOffset);
                    SymbolArray arguments = expression[2].array();
                    for (int i = arguments.count() - 1; i >= 0; --i)
                        compileExpression(arguments[i]);
                    compileExpression(expression[1].symbol());
                    add(Symbol(atomCall));
                }
                break;
            case atomIntegerConstant:
            case atomStringConstant:
                add(expression);
                break;
            case atomIdentifier:
                {
                    Symbol definition = Symbol::labelled(labelOf(expression));
                    addAddressOf(definition);
                    add(Symbol(atomDereference));
                }
                break;
            case atomTrue:
            case atomFalse:
            case atomNull:
                add(expression);
                break;
        }
    }
    void addAddressOf(Symbol symbol)
    {
        int offset = offsetOf(symbol);
        add(Symbol(atomStackPointer));  // TODO: there might be temporaries on the stack - keep track and correct for this
        add(Symbol(atomIntegerConstant, offset + _stackOffset - 4));
        add(Symbol(atomAdd));
    }
    void add(Symbol symbol)
    {
        _basicBlock.add(symbol);
        _blockEnds = false;
        _atBlockStart = false;
        int adjust = 0;
        switch (symbol.atom()) {
            case atomBitwiseOr:
            case atomBitwiseXor:
            case atomBitwiseAnd:
            case atomEqualTo:
            case atomNotEqualTo:
            case atomLessThanOrEqualTo:
            case atomGreaterThanOrEqualTo:
            case atomLessThan:
            case atomGreaterThan:
            case atomLeftShift:
            case atomRightShift:
            case atomAdd:
            case atomSubtract:
            case atomMultiply:
            case atomDivide:
            case atomModulo:
            case atomPower:
            case atomGoto:
            case atomDrop:
            case atomStringConcatenate:
            case atomStringEqualTo:
            case atomStringNotEqualTo:
            case atomStringLessThanOrEqualTo:
            case atomStringGreaterThanOrEqualTo:
            case atomStringLessThan:
            case atomStringGreaterThan:
            case atomStringIntegerMultiply:
            case atomPrintFunction:
                adjust = -4;
                break;

            case atomNot:
            case atomNegative:
            case atomDereference:
                adjust = 0;
                break;

            case atomStringConstant:
            case atomIntegerConstant:
            case atomTrue:
            case atomFalse:
            case atomNull:
            case atomStackPointer:
            case atomDuplicate:
                adjust = 4;
                break;

            case atomCall:
                adjust = _stackOffsetStack.pop() - _stackOffset;
                break;

            case atomJumpIfTrue:
            case atomStore:
                adjust = -8;
                break;

        }
        _stackOffset += adjust;
    }
    void addGoto(int destination)
    {
        add(Symbol(atomIntegerConstant, destination));
        add(Symbol(atomGoto));
        _blockEnds = true;
    }
    void addJumpIfTrue(int destination)
    {
        add(Symbol(atomIntegerConstant, destination));
        add(Symbol(atomJumpIfTrue));
    }
    void addLabel(int label)
    {
        int follows = label;
        if (_blockEnds)
            follows = -1;
        Symbol block(atomBasicBlock, SymbolArray(_basicBlock), _label, follows);
        block.setLabel(_label);
        _compiledProgram.add(block);
        _basicBlock = SymbolList();
        _label = label;
        _atBlockStart = true;
    }
    int getLabel()
    {
        if (!_atBlockStart)
            addLabel(Symbol::newLabel());
        return _label;
    }

    SymbolList _compiledProgram;
    SymbolList _basicBlock;
    int _label;
    bool _blockEnds;
    bool _atBlockStart;
    int _stackOffset;
    Stack<int> _stackOffsetStack;
    
    class BreakContinueStackEntry
    {
    public:
        BreakContinueStackEntry(int breakLabel, int continueLabel)
          : _breakLabel(breakLabel), _continueLabel(continueLabel) { }
        int _breakLabel;
        int _continueLabel;
    };
    Stack<BreakContinueStackEntry> _breakContinueStack;
};
