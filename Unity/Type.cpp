String typeToString(Symbol type)
{
    switch (type.atom()) {
        case atomFunction:
            {
                String s = typeToString(type[1].symbol()) + openParenthesis;
                SymbolArray list = type[2].list();
                bool hasArguments = false;
                while (list.count() != 0) {
                    if (hasArguments)
                        s += commaSpace;
                    s += typeToString(list.head());
                    hasArguments = true;
                    list = list.tail();
                }
                return s + closeParenthesis;
            }
        case atomPointer:
            return typeToString(type[1].symbol()) + asterisk;
        default:
            return atomToString(type.atom());
    }
}
