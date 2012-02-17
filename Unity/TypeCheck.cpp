void assertTypeBoolean(Symbol expression)
{
    Symbol type = typeOf(expression);
    if (type.atom() != atomBoolean)
        spanOf(expression).throwError("expression is of type " +
            typeToString(type) + ", Boolean expected");
}

void checkTypes(SymbolEntry entry, Symbol returnType)
{
    if (entry.isArray()) {
        SymbolList list;
        SymbolArray array = entry.array();
        for (int i = 0; i < array.count(); ++i)
            checkTypes(array[i], returnType);
    }
    if (!entry.isSymbol())
        return;
    Symbol symbol = entry.symbol();
    switch (symbol.atom()) {
        case atomLogicalOr:
        case atomLogicalAnd:
            assertTypeBoolean(symbol[1].symbol());
            assertTypeBoolean(symbol[2].symbol());
            break;
        case atomFunctionCall:
            {
                Symbol function = symbol[1].symbol();
                SymbolArray parameterTypes = typeOf(function)[2].array();
                SymbolArray argumentTypes = typeOf(symbol[2]).array();
                if (parameterTypes != argumentTypes)
                    spanOf(symbol).throwError(
                        "function requires arguments of types " +
                        typesToString(parameterTypes) +
                        " but passed arguments of types " +
                        typesToString(argumentTypes));
            }
            break;
        case atomFunctionDefinitionStatement:
            checkTypes(symbol[3], returnType);
            checkTypes(symbol[4], symbol[1].symbol());
            return;
        case atomVariableDefinitionStatement:
            {
                Symbol initializerType = typeOf(symbol[3]);
                Symbol variableType = typeOf(symbol[1]);
                if (variableType != initializerType)
                    spanOf(symbol).throwError("variable declared as type " +
                        typeToString(variableType) +
                        " but initialized with expression of type " +
                        typeToString(initializerType));
            }
            break;
        case atomIfStatement:
            assertTypeBoolean(symbol[1].symbol());
            break;
        case atomSwitchStatement:
            {
                Symbol type = typeOf(symbol[1]);
                SymbolArray cases = symbol[2].array();
                for (int i = 0; i < cases.count(); ++i) {
                    Symbol c = cases[i];
                    SymbolArray expressions = c[1].array();
                    for (int j = 0; j < expressions.count(); ++j) {
                        Symbol expression = expressions[j];
                        Symbol expressionType = typeOf(expression);
                        if (type != expressionType)
                            spanOf(expression).throwError(
                                "can't compare an expression of type " +
                                typeToString(type) +
                                " to an expression of type " +
                                typeToString(expressionType));
                    }
                }
            }
            break;
        case atomReturnStatement:
            {
                Symbol expression = symbol[1].symbol();
                Symbol type;
                if (expression.valid())
                    type = typeOf(expression);
                else
                    type = Symbol(atomVoid);
                if (type != returnType)
                    spanOf(symbol).throwError(
                        "returning an expression of type " +
                        typeToString(type) +
                        " from a function with return type " +
                        typeToString(returnType));
            }
            break;
        case atomIncludeStatement:
            {
                Symbol expression = symbol[1].symbol();
                Symbol type = typeOf(expression);
                if (type.atom() != atomString)
                    spanOf(expression).throwError(
                        "argument to include is of type " +
                        typeToString(type) + ", expected String");
            }
            break;
        case atomFromStatement:
            {
                Symbol expression = symbol[1].symbol();
                Symbol type = typeOf(expression);
                if (type.atom() != atomString)
                    spanOf(expression).throwError(
                        "argument to from is of type " + typeToString(type) +
                        ", expected String");
            }
            break;
        case atomWhileStatement:
        case atomUntilStatement:
        case atomForStatement:
            assertTypeBoolean(symbol[2].symbol());
            break;
        case atomGotoStatement:
            {
                Symbol expression = symbol[1].symbol();
                Symbol type = typeOf(expression);
                if (type.atom() != atomLabel)
                    if (type.atom() != atomLabel)
                        spanOf(expression).throwError(
                            "expression is of type " + typeToString(type) +
                            ", Label expected");
            }
            break;
        case atomEmit:
            // TODO: Check that type of argument is Sequence<Compiler.Instruction>
            break;
    }

    const SymbolTail* tail = symbol.tail();
    while (tail != 0) {
        checkTypes(tail->head(), returnType);
        tail = tail->tail();
    }
}
