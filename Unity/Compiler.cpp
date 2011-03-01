class Compiler
{
public:
    void compileFunction(Symbol functionDefinitionStatement)
    {
        int stackAdjust = offsetOf(functionDefinitionStatement);
        if (stackAdjust != 0)
            addAdjustStackPointer(-stackAdjust);
        _stackOffset = 0;
        compileStatementSequence(functionDefinitionStatement[4].array());
        // Before: returnValue localVariables returnAddress parameters
        // After: returnAddress returnValue
        Symbol type = typeOf(functionDefinitionStatement);
        Symbol returnType = type[1].symbol();
        int returnTypeSize = (sizeOf(returnType) + 3) & -4;
        int parametersSize = 0;
        SymbolArray parameterTypes = type[2].array();
        for (int i = 0; i < parameterTypes.count(); ++i)
            parametersSize += (sizeOf(parameterTypes[i]) + 3) & -4;
        addLoadWordFromStackRelativeAddress(returnTypeSize + stackAdjust);
        addMoveBlock(0, stackAdjust + 4 + parametersSize, 1 + returnTypeSize/4);        
        if (stackAdjust != 0)
            addAdjustStackPointer(stackAdjust);
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
                {
                    Symbol expression = statement[1].symbol();
                    compileExpression(expression);
                    // Pop (unused) return value from stack
                    addAdjustStackPointer((sizeOf(typeOf(expression)) + 3) & -4);
                }
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
                    SymbolArray arguments = expression[2].array();
                    for (int i = arguments.count() - 1; i >= 0; --i)
                        compileExpression(arguments[i]);
                    Symbol function = expression[1].symbol();
                    compileExpression(function);
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
    void addPushStackRelativeAddress(int offset)
    {
        add(Symbol(atomStackPointer));
        add(Symbol(atomIntegerConstant, offset));
        add(Symbol(atomAdd));
    }
    void addLoadWordFromStackRelativeAddress(int offset)
    {
        addPushStackRelativeAddress(offset);
        add(Symbol(atomDereference));
    }
    void addStoreWordToStackRelativeAddress(int offset)
    {
        addPushStackRelativeAddress(offset);
        addLoadWordFromStackRelativeAddress(4);
        add(Symbol(atomStore));
        add(Symbol(atomDrop));
    }
    void addMoveWord(int fromOffset, int toOffset)
    {
        if (fromOffset != toOffset) {
            addLoadWordFromStackRelativeAddress(fromOffset);
            addStoreWordToStackRelativeAddress(toOffset + 4);
        }
    }
    void addMoveBlock(int fromOffset, int toOffset, int words)
    {
        for (int i = 0; i < words; ++i) {
            addMoveWord(fromOffset, toOffset);
            fromOffset += 4;
            toOffset += 4;
        }
    }
    void addAdjustStackPointer(int offset)
    {
        if (offset == 12) {
            offset -= 4;
            add(Symbol(atomDrop));
        }
        if (offset == 8) {
            offset -= 4;
            add(Symbol(atomDrop));
        }
        if (offset == 4) {
            offset -= 4;
            add(Symbol(atomDrop));
        }
        if (offset == 0)
            return;
        add(Symbol(atomStackPointer));
        add(Symbol(atomIntegerConstant, offset));
        add(Symbol(atomAdd));
        add(Symbol(atomSetStackPointer));
    }
    void addAddressOf(Symbol symbol)
    {
        Symbol definition = Symbol::labelled(labelOf(symbol));
        addPushStackRelativeAddress(offsetOf(definition) + _stackOffset - 4);
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
                adjust = -4;
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
        checkBlockStackOffset(destination);
        _blockEnds = true;
    }
    void addJumpIfTrue(int destination)
    {
        add(Symbol(atomIntegerConstant, destination));
        add(Symbol(atomJumpIfTrue));
        checkBlockStackOffset(destination);
    }
    void addLabel(int label)
    {
        int follows = label;
        if (_blockEnds)
            follows = -1;
        else
            checkBlockStackOffset(follows);
        Symbol block(atomBasicBlock, SymbolArray(_basicBlock), _label, follows);
        checkBlockStackOffset(_label);
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
    void checkBlockStackOffset(int label)
    {
        if (_blockStackOffsets.hasKey(label)) {
            int stackOffset = _blockStackOffsets[label];
            if (stackOffset != _stackOffset) {
                static String error("Stack offset mismatch. Expected ");
                static String error2(" found ");
                throw Exception(error + String::decimal(stackOffset) + error2 + String::decimal(_stackOffset));
            }
        }
        else
            _blockStackOffsets.add(label, _stackOffset);
    }

    SymbolList _compiledProgram;
    SymbolList _basicBlock;
    int _label;
    bool _blockEnds;
    bool _atBlockStart;
    int _stackOffset;
    HashTable<int, int> _blockStackOffsets;
    
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
