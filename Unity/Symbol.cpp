enum SymbolType
{
    symbolInteger,
    symbolString,
    symbolList,
    symbolTuple
};

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
    IntegerSymbol integer() const { return static_cast<IntegerSymbol>(_implementation); }
    StringSymbol string() const { return static_cast<StringSymbol>(_implementation); }
    ListSymbol list() const { return static_cast<ListSymbol>(_implementation); }
    TupleSymbol tuple() const { return static_cast<TupleSymbol>(_implementation); }
    bool valid() const { return _implementation.valid(); }
    String toString() const { return _implementation->toString(); }
    SymbolType type() const { return _implementation->type(); }
protected:
    class Implementation : public ReferenceCounted
    {
    public:
        virtual bool equals(const Implementation* other) const = 0;
        virtual String toString() const = 0;
        virtual SymbolType type() const = 0;
    };
    Symbol(Implementation* implementation) : _implementation(implementation) { }
    Symbol(Reference<Implementation> implementation) : _implementation(implementation) { }
    const Implementation* implementation() const { return _implementation; }
private:
    Reference<Implementation> _implementation;
};

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
        SymbolType type() const { return symbolInteger; }
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
        SymbolType type() const { return symbolString; }
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
        SymbolType type() const { return symbolList; }
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
        SymbolType type() const { return symbolTuple; }
    private:
        Atom _atom;
        Array<Symbol> _array;
    };
    TupleSymbol(Reference<Symbol::Implementation> implementation) : Symbol(implementation) { }
    friend class Symbol;
};
