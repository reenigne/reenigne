String typeToString(Symbol type)
{
    switch (type.atom()) {
        case atomFunction:
            {
                String s = typeToString(type[1].symbol()) + openParenthesis;
                SymbolArray array = type[2].array();
                bool hasArguments = false;
                for (int i = 0; i < array.count(); ++i) {
                    if (hasArguments)
                        s += commaSpace;
                    s += typeToString(array[i]);
                    hasArguments = true;
                }
                return s + closeParenthesis;
            }
        case atomPointer:
            return typeToString(type[1].symbol()) + asterisk;
        default:
            return atomToString(type.atom());
    }
}

String typesToString(SymbolArray array)
{
    String s = openParenthesis;
    for (int i = 0; i < array.count(); ++i) {
        if (i != 0)
            s += commaSpace;
        s += typeToString(array[i]);
    }
    return s + closeParenthesis;
}