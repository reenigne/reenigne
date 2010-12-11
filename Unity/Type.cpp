String typeToString(Symbol type)
{
    switch (type.atom()) {
        case atomFunction:
            {
                String s = typeToString(type.entry1().symbol()) + openParenthesis;
                SymbolList list = type.entry2().list();
                bool hasArguments = false;
                while (!list.isEmpty()) {
                    if (hasArguments)
                        s += commaSpace;
                    s += typeToString(list.head());
                    hasArguments = true;
                    list = list.tail();
                }
                return s + closeParenthesis;
            }
        case atomPointer:
            return typeToString(type.entry1().symbol()) + asterisk;
        default:
            return atomToString(type.atom());
    }
}
