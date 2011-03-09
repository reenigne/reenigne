void assertTypeBoolean(Symbol expression)
{
    static String error("expression is of type ");
    static String error2(", Boolean expected");
    Symbol type = typeOf(expression);
    if (type.atom() != atomBoolean)
        spanOf(expression).throwError(error + typeToString(type) + error2);
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
                if (parameterTypes != argumentTypes) {
                    static String error("function requires arguments of types ");
                    static String error2(" but passed arguments of types ");
                    spanOf(symbol).throwError(error + typesToString(parameterTypes) + error2 + typesToString(argumentTypes));
                }
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
                if (variableType != initializerType) {
                    static String error("variable declared as type ");
                    static String error2(" but initialized with expression of type ");
                    spanOf(symbol).throwError(error + typeToString(variableType) + error2 + typeToString(initializerType));
                }
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
                        if (type != expressionType) {
                            static String error("can't compare an expression of type ");
                            static String error2(" to an epxression of type ");
                            spanOf(expression).throwError(error + typeToString(type) + error2 + typeToString(expressionType));
                        }
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
                if (type != returnType) {
                    static String error("returning an expression of type ");
                    static String error2(" from a function with return type ");
                    spanOf(symbol).throwError(error + typeToString(type) + error2 + typeToString(returnType));
                }
            }
            break;
        case atomIncludeStatement:
            {
                Symbol expression = symbol[1].symbol();
                Symbol type = typeOf(expression);
                if (type.atom() != atomString) {
                    static String error("argument to include is of type ");
                    static String error2(", expected String");
                    spanOf(expression).throwError(error + typeToString(type) + error2);
                }
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
                if (type.atom() != atomLabel) {
                    static String error("expression is of type ");
                    static String error2(", Label expected");
                    if (type.atom() != atomLabel)
                        spanOf(expression).throwError(error + typeToString(type) + error2);
                }
            }
            break;
    }

    const SymbolTail* tail = symbol.tail();
    while (tail != 0) {
        checkTypes(tail->head(), returnType);
        tail = tail->tail();
    }
}
