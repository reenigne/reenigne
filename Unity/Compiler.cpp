class Program
{
public:
    void add(Symbol basicBlock) { _basicBlocks.add(basicBlock); }
private:
    SymbolList _basicBlocks;
};

class Compiler
{
public:
    Compiler(Program* program) : _program(program) { }
    void compileFunction(Symbol functionDefinitionStatement)
    {
        if (functionDefinitionStatement.cache<FunctionDefinitionCache>()->getCompilingFlag()) {
            static String error("Function called during its own compilation");  // TODO: Give more details about what's being evaluated and how that came to call this
            spanOf(functionDefinitionStatement).end().throwError(error);
        }
        functionDefinitionStatement.cache<FunctionDefinitionCache>()->setCompilingFlag(true);
        _epilogueStack.push(Symbol::newLabel());
        //Symbol type = typeOf(functionDefinitionStatement);
        //Symbol returnType = type[1].symbol();
        //_returnTypeStack.push(returnType);
        int stackAdjust = offsetOf(functionDefinitionStatement);
        if (stackAdjust != 0)
            addAdjustStackPointer(-stackAdjust);
        _stackOffset = 0;
        compileStatementSequence(functionDefinitionStatement[4].array());
        Symbol type = typeOf(functionDefinitionStatement);
        Symbol returnType = type[1].symbol();
        int returnTypeSize = (sizeOf(returnType) + 3) & -4;
        int parametersSize = 0;
        SymbolArray parameterTypes = type[2].array();
        for (int i = 0; i < parameterTypes.count(); ++i)
            parametersSize += (sizeOf(parameterTypes[i]) + 3) & -4;
        if (_reachable && returnType.atom() != atomVoid) {
            static String error("Control reaches end of non-Void function");  // TODO: Give more details about how it got there
            spanOf(functionDefinitionStatement).end().throwError(error);
        }
        addLabel(_epilogueStack.pop());
        addLoadWordFromStackRelativeAddress(returnTypeSize + stackAdjust);
        addMoveBlock(0, stackAdjust + 4 + parametersSize, 1 + returnTypeSize/4);
        if (stackAdjust != 0)
            addAdjustStackPointer(stackAdjust);
        add(Symbol(atomReturn));
        //_returnTypeStack.pop();
        functionDefinitionStatement.cache<FunctionDefinitionCache>()->setCompilingFlag(false);
    }
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
        _program->add(block);
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
                    addAdjustStackPointer((sizeOf(typeOf(expression)) + 3) & -4);
                }
                break;            
            case atomFunctionDefinitionStatement:
                {
                    Compiler compiler(_program);
                    compiler.compileFunction(statement);
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
            case atomCompoundStatement:
                compileStatementSequence(statement[1].array());
                break;
            case atomTypeAliasStatement:
                break;
            case atomNothingStatement:
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
                compileExpression(statement[1].symbol());
                addGoto(_epilogueStack.top());
                break;
            case atomIncludeStatement:
                // TODO
                break;
            case atomBreakStatement:
                {
                    int n = 0;
                    Symbol tail = statement[1].symbol();
                    bool isContinue = false;
                    while (tail.valid()) {
                        isContinue = (tail.atom() == atomContinueStatement);
                        ++n;
                        tail = tail[1].symbol();
                    }
                    if (isContinue)
                        addGoto(_breakContinueStack.fromTop(n)._continueLabel);
                    else
                        addGoto(_breakContinueStack.fromTop(n)._breakLabel);
                }
                break;
            case atomContinueStatement:
                addGoto(_breakContinueStack.top()._continueLabel);
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
            case atomEmit:
                {
                    SymbolArray array = evaluate<SymbolArray>(statement[1].symbol());
                    for (int i = 0; i < array.count(); ++i)
                        add(array[i]);
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
            case atomShiftLeft:
            case atomShiftRight:
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
        _reachable = false;
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
        _program->add(block);
        _basicBlock = SymbolList();
        _label = label;
        _atBlockStart = true;
        _reachable = true;
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

    Program* _program;
    SymbolList _basicBlock;
    int _label;
    bool _blockEnds;
    bool _atBlockStart;
    bool _reachable;
    int _stackOffset;
    HashTable<int, int> _blockStackOffsets;
//    Stack<Symbol> _returnTypeStack;
    Stack<int> _epilogueStack;
    
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
