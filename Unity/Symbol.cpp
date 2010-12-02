enum Atom
{
    atomBit,
    atomBoolean,
    atomByte,
    atomCharacter,
    atomFunction,
    atomInt,
    atomPointer,
    atomString,
    atomUInt,
    atomVoid,
    atomWord,

    atomLogicalOr,
    atomLogicalAnd,
    atomBitwiseOr,
    atomBitwiseXor,
    atomBitwiseAnd,
    atomEqualTo,
    atomNotEqualTo,
    atomLessThanOrEqualTo,
    atomGreaterThanOrEqualTo,
    atomLessThan,
    atomGreaterThan,
    atomLeftShift,
    atomRightShift,
    atomAdd,
    atomSubtract,
    atomMultiply,
    atomDivide,
    atomModulo,
    atomLogicalNot,
    atomBitwiseNot,
    atomPositive,
    atomNegative,
    atomDereference,
    atomAddressOf,
    atomPower,
    
    atomLast
};

String atomToString(Atom atom)
{
    class LookupTable
    {
    public:
        LookupTable()
        {
            _table[atomBit] = String("Bit");
            _table[atomBoolean] = String("Boolean");
            _table[atomByte] = String("Byte");
            _table[atomCharacter] = String("Character");
            _table[atomFunction] = String("Function");
            _table[atomPointer] = String("Pointer");
            _table[atomInt] = String("Int");
            _table[atomString] = String("String");
            _table[atomUInt] = String("UInt");
            _table[atomVoid] = String("Void");
            _table[atomWord] = String("Word");

            _table[atomLogicalOr] = String("||");
            _table[atomLogicalAnd] = String("&&");
            _table[atomBitwiseOr] = String("|");
            _table[atomBitwiseXor] = String("~");
            _table[atomBitwiseAnd] = String("&");
            _table[atomEqualTo] = String("==");
            _table[atomNotEqualTo] = String("!=");
            _table[atomLessThanOrEqualTo] = String("<=");
            _table[atomGreaterThanOrEqualTo] = String(">=");
            _table[atomLessThan] = String("<");
            _table[atomGreaterThan] = String(">");
            _table[atomLeftShift] = String("<<");
            _table[atomRightShift] = String(">>");
            _table[atomAdd] = String("+");
            _table[atomSubtract] = String("-");
            _table[atomMultiply] = String("*");
            _table[atomDivide] = String("/");
            _table[atomModulo] = String("%");
            _table[atomLogicalNot] = String("!");
            _table[atomBitwiseNot] = String("u~");
            _table[atomPositive] = String("u+");
            _table[atomNegative] = String("u-");
            _table[atomDereference] = String("u*");
            _table[atomAddressOf] = String("u&");
            _table[atomPower] = String("^");
        }
        String lookUp(Atom atom) { return _table[atom]; }
    private:
        String _table[atomLast];
    };
    static LookupTable lookupTable;
    return lookupTable.lookUp(atom);
}

String quote(String string)
{
    CodePointSource s(string);
    String r;
    int start = 0;
    int end;
    do {
        int c = s.get();
        if (c == -1) {
            end = s.offset();
            r += s.subString(start, end);
            return r;
        }
        if (c == '"' || c == '\\') {
            end = s.offset();
            r += s.subString(start, end);
            start = end;
            static String backslash("\\");
            r += backslash;
        }
    } while (true);
}

class Symbol
{
public:
    bool operator==(const Symbol& other) const
    {
        return _implementation->equals(other._implementation);
    }
    bool operator!=(const Symbol& other) const
    {
        return !_implementation->equals(other._implementation);
    }
    int integer() const { return static_cast<IntegerSymbol>(_implementation).value(); }
    String string() const { return static_cast<StringSymbol>(_implementation).value(); }
    ListSymbol list() const { return static_cast<ListSymbol>(_implementation); }
    TupleSymbol tuple() const { return static_cast<TupleSymbol>(_implementation); }
    Atom atom() const { return tuple().atom(); }
    Symbol head() const { return list().head(); }
    ListSymbol tail() const { return list().tail(); }
    Reference<ReferenceCounted> reference() const { return static_cast<ReferenceSymbol>(_implementation).value(); }
    bool valid() const { return _implementation.valid(); }
    String toString() const { return _implementation->toString(); }
    TupleSymbol label() const { return _labelled[integer()]; }
protected:
    class Implementation : public ReferenceCounted
    {
    public:
        virtual bool equals(const Implementation* other) const = 0;
        virtual String toString() const = 0;
    };
    Symbol(Implementation* implementation) : _implementation(implementation) { }
    Symbol(Reference<Implementation> implementation) : _implementation(implementation) { }
    const Implementation* implementation() const { return _implementation; }
private:
    Reference<Implementation> _implementation;
    static HashTable<int, TupleSymbol> _labelled;
};

HashTable<int, TupleSymbol> Symbol::_labelled;

class IntegerSymbol : public Symbol
{
public:
    IntegerSymbol(int value) : Symbol(new Implementation(value)) { }
    int value() const { return dynamic_cast<const Implementation*>(implementation())->value(); }
private:
    class Implementation : public Symbol::Implementation
    {
    public:
        Implementation(int value) : _value(value) { }
        bool equals(const Symbol::Implementation* other) const
        {
            const Implementation* o = dynamic_cast<const Implementation*>(other);
            if (o == 0)
                return false;
            return _value == o->_value;
        }
        String toString() const { return String::decimal(_value); }
        int value() const { return _value; }
    private:
        int _value;
    };
    IntegerSymbol(Reference<Symbol::Implementation> implementation) : Symbol(implementation) { }
    friend class Symbol;
};

class StringSymbol : public Symbol
{
public:
    StringSymbol(String value) : Symbol(new Implementation(value)) { }
    String value() const { return dynamic_cast<const Implementation*>(implementation())->value(); }
private:
    class Implementation : public Symbol::Implementation
    {
    public:
        Implementation(String value) : _value(value) { }
        bool equals(const Symbol::Implementation* other) const
        {
            const Implementation* o = dynamic_cast<const Implementation*>(other);
            if (o == 0)
                return false;
            return _value == o->_value;
        }
        String toString() const { return quote(_value); }
        String value() const { return _value; }
    private:
        String _value;
    };
    StringSymbol(Reference<Symbol::Implementation> implementation) : Symbol(implementation) { }
    friend class Symbol;
};

class ListSymbol : public Symbol
{
public:
    ListSymbol() : Symbol(emptyList()) { }
    ListSymbol(Symbol head) : Symbol(new Implementation(head, ListSymbol())) { }
    ListSymbol(Symbol head, ListSymbol tail) : Symbol(new Implementation(head, tail)) { }
    bool isEmpty() const { return dynamic_cast<const ListImplementation*>(implementation())->isEmpty(); }
    Symbol head() const { return dynamic_cast<const Implementation*>(implementation())->head(); }
    ListSymbol tail() const { return dynamic_cast<const Implementation*>(implementation())->tail(); }
private:
    class ListImplementation : public Symbol::Implementation
    {
    public:
        virtual bool isEmpty() const = 0;
        virtual String toString2() const = 0;
    };
    class Implementation : public ListImplementation
    {
    public:
        Implementation(Symbol head, ListSymbol tail) : _head(head), _tail(tail) { }
        bool equals(const Symbol::Implementation* other) const
        {
            const Implementation* o = dynamic_cast<const Implementation*>(other);
            if (o == 0)
                return false;
            return _head == o->_head && _tail == o->_tail;
        }
        bool isEmpty() const { return false; }
        Symbol head() const { return _head; }
        ListSymbol tail() const { return _tail; }
        String toString() const
        { 
            static String openBracket("[");
            static String closeBracket("]");
            return openBracket + _head.toString() + _tail.list().toString2() + closeBracket;
        }
        String toString2() const
        {
            static String space(" ");
            return space + _head.toString() + _tail.list().toString2();
        }
    private:
        Symbol _head;
        Symbol _tail;
    };
    class EmptyImplementation : public ListImplementation
    {
    public:
        bool equals(const Symbol::Implementation* other) const
        {
            return static_cast<const Symbol::Implementation*>(this) == other;
        }
        bool isEmpty() const { return true; }
        String toString() const
        {
            static String s("[]");
            return s;
        }
        String toString2() const
        {
            static String s("");
            return s;
        }
    };
    static Reference<Implementation> emptyList()
    {
        if (!_emptyList.valid())
            _emptyList = new EmptyImplementation;
        return _emptyList;
    }
    static Reference<ListImplementation> _emptyList;
    ListSymbol(Reference<Symbol::Implementation> implementation) : Symbol(implementation) { }
    friend class Symbol;
    String toString2() { return dynamic_cast<const ListImplementation*>(implementation())->toString2(); } 
};

Reference<ListSymbol::ListImplementation> ListSymbol::_emptyList;

class TupleSymbol : public Symbol
{
public:
    TupleSymbol(Atom atom) : Symbol(new Implementation(atom)) { }
    TupleSymbol(Atom atom, Symbol symbol1) : Symbol(new Implementation(atom, symbol1)) { }
    TupleSymbol(Atom atom, Symbol symbol1, Symbol symbol2) : Symbol(new Implementation(atom, symbol1, symbol2)) { }
    TupleSymbol(Atom atom, Symbol symbol1, Symbol symbol2, Symbol symbol3) : Symbol(new Implementation(atom, symbol1, symbol2, symbol3)) { }
    Atom atom() const { return dynamic_cast<const Implementation*>(implementation())->atom(); }
    Symbol symbol1() const { return dynamic_cast<const Implementation*>(implementation())->symbol1(); }
    Symbol symbol2() const { return dynamic_cast<const Implementation*>(implementation())->symbol2(); }
    Symbol symbol3() const { return dynamic_cast<const Implementation*>(implementation())->symbol3(); }
private:
    class Implementation : public Symbol::Implementation
    {
    public:
        Implementation(Atom atom) : _atom(atom) { }
        Implementation(Atom atom, Symbol symbol1) : _atom(atom)
        {
            _array.allocate(1);
            _array[0] = symbol1;
        }
        Implementation(Atom atom, Symbol symbol1, Symbol symbol2) : _atom(atom)
        {
            _array.allocate(2);
            _array[0] = symbol1;
            _array[1] = symbol2;
        }
        Implementation(Atom atom, Symbol symbol1, Symbol symbol2, Symbol symbol3) : _atom(atom)
        {
            _array.allocate(3);
            _array[0] = symbol1;
            _array[1] = symbol2;
            _array[2] = symbol3;
        }
        bool equals(const Symbol::Implementation* other) const
        {
            const Implementation* o = dynamic_cast<const Implementation*>(other);
            if (o == 0)
                return false;
            return _atom == o->_atom && _array == o->_array;
        }
        Atom atom() const { return _atom; }
        Symbol symbol1() const { return _array[0]; }
        Symbol symbol2() const { return _array[1]; }
        Symbol symbol3() const { return _array[2]; }
        String toString() const
        {
            static String openParenthesis("(");
            static String closeParenthesis(")");
            static String space(" ");
            String s = openParenthesis + atomToString(_atom);
            for (int i = 0; i < _array.count(); ++i)
                s += space + _array[i].toString();
            return s + closeParenthesis;
        }
    private:
        Atom _atom;
        Array<Symbol> _array;
    };
    TupleSymbol(Reference<Symbol::Implementation> implementation) : Symbol(implementation) { }
    friend class Symbol;
};

class ReferenceSymbol : public Symbol
{
public:
    ReferenceSymbol(Reference<ReferenceCounted> reference) : Symbol(new Implementation(reference)) { }
    Reference<ReferenceCounted> value() const { return dynamic_cast<const Implementation*>(implementation())->value(); }
private:
    class Implementation : public Symbol::Implementation
    {
    public:
        Implementation(Reference<ReferenceCounted> reference) : _reference(reference) { }
        bool equals(const Symbol::Implementation* other) const
        {
            const Implementation* o = dynamic_cast<const Implementation*>(other);
            if (o == 0)
                return false;
            return _reference == o->_reference;
        }
        Reference<ReferenceCounted> value() const { return _reference; }
        String toString() const
        {
            static String reference("<reference>");
            return reference;
        }
    private:
        Reference<ReferenceCounted> _reference;
    };
    ReferenceSymbol(Reference<Symbol::Implementation> implementation) : Symbol(implementation) { }
    friend class Symbol;
};