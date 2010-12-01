String openParenthesis("(");
String closeParenthesis(")");
String commaSpace(", ");
String asterisk("*");

String typeToString(TupleSymbol type)
{
    switch (type.atom()) {
        case atomFunction:
            {
                String s = typeToString(type.symbol1().tuple()) + openParenthesis;
                ListSymbol list = type.symbol2();
                bool hasArguments = false;
                while (!list.isEmpty()) {
                    if (hasArguments)
                        s += commaSpace;
                    s += typeToString(list.head().tuple());
                    hasArguments = true;
                    list = list.tail();
                }
                return s + closeParenthesis;
            }
        case atomPointer:
            return typeToString(type.symbol1().tuple()) + asterisk;
        default:
            return atomToString(type.atom());
    }
}
