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


class Symbol;

class SymbolEntry
{
public:
    SymbolEntry(int value) : _implementation(new IntegerImplementation(value)) { }
    SymbolEntry(String value) : _implementation(new StringImplementation(value)) { }
    bool operator==(const SymbolEntry& other) const
    {
        return _implementation->equals(other._implementation);
    }
    bool operator!=(const SymbolEntry& other) const
    {
        return !_implementation->equals(other._implementation);
    }
    int integer() const { return dynamic_cast<const IntegerImplementation*>(implementation())->value(); }
    String string() const { return dynamic_cast<const StringImplementation*>(implementation())->value(); }
    SymbolList list() const { return static_cast<SymbolList>(_implementation); }
    Symbol symbol() const { return static_cast<Symbol>(_implementation); }
    Atom atom() const { return symbol().atom(); }
    Symbol head() const { return list().head(); }
    SymbolList tail() const { return list().tail(); }
    bool valid() const { return _implementation.valid(); }
    String toString() const { return _implementation->toString(); }
    Symbol label() const { return Symbol::_labelled[integer()]; }
protected:
    class Implementation : public ReferenceCounted
    {
    public:
        virtual bool equals(const Implementation* other) const = 0;
        virtual String toString() const = 0;
    };
    class IntegerImplementation : public Implementation
    {
    public:
        IntegerImplementation(int value) : _value(value) { }
        bool equals(const SymbolEntry::Implementation* other) const
        {
            const IntegerImplementation* o = dynamic_cast<const IntegerImplementation*>(other);
            if (o == 0)
                return false;
            return _value == o->_value;
        }
        String toString() const { return String::decimal(_value); }
        int value() const { return _value; }
    private:
        int _value;
    };
    class StringImplementation : public Implementation
    {
    public:
        StringImplementation(String value) : _value(value) { }
        bool equals(const SymbolEntry::Implementation* other) const
        {
            const StringImplementation* o = dynamic_cast<const StringImplementation*>(other);
            if (o == 0)
                return false;
            return _value == o->_value;
        }
        String toString() const { return quote(_value); }
        String value() const { return _value; }
    private:
        String _value;
    };
    SymbolEntry(Implementation* implementation) : _implementation(implementation) { }
    //SymbolEntry(Reference<Implementation> implementation) : _implementation(implementation) { }
    const Implementation* implementation() const { return _implementation; }
private:
    Reference<Implementation> _implementation;
};

String openParenthesis("(");
String closeParenthesis(")");
String space(" ");

class Symbol : public SymbolEntry
{
public:
    Symbol(Atom atom) : SymbolEntry(new Implementation0(atom)) { }
    Symbol(Atom atom, SymbolEntry symbol1) : SymbolEntry(new Implementation1(atom, symbol1)) { }
    Symbol(Atom atom, SymbolEntry symbol1, SymbolEntry symbol2) : SymbolEntry(new Implementation2(atom, symbol1, symbol2)) { }
    Symbol(Atom atom, SymbolEntry symbol1, SymbolEntry symbol2, SymbolEntry symbol3) : SymbolEntry(new Implementation3(atom, symbol1, symbol2, symbol3)) { }
    Atom atom() const { return dynamic_cast<const Implementation0*>(implementation())->atom(); }
    SymbolEntry entry1() const { return dynamic_cast<const Implementation1*>(implementation())->entry1(); }
    SymbolEntry entry2() const { return dynamic_cast<const Implementation2*>(implementation())->entry2(); }
    SymbolEntry entry3() const { return dynamic_cast<const Implementation3*>(implementation())->entry3(); }
private:
    class Implementation0 : public SymbolEntry::Implementation
    {
    public:
        Implementation0(Atom atom) : _atom(atom) { }
        bool equals(const SymbolEntry::Implementation* other) const
        {
            const Implementation0* o = dynamic_cast<const Implementation0*>(other);
            if (o == 0)
                return false;
            return _atom == o->_atom;
        }
        String toString() const { return openParenthesis + toString2() + closeParenthesis; }
        Atom atom() const { return _atom; }
    protected:
        String toString2() const { return atomToString(_atom); }
        Atom _atom;
    };
    class Implementation1 : public Implementation0
    {
    public:
        Implementation1(Atom atom, SymbolEntry entry1) : Implementation0(atom), _entry1(entry1) { }
        bool equals(const SymbolEntry::Implementation* other) const
        {
            const Implementation1* o = dynamic_cast<const Implementation1*>(other);
            if (o == 0)
                return false;
            return _atom == o->_atom && _entry1 == o->_entry1;
        }
        String toString() const { return openParenthesis + toString2() + closeParenthesis; }
        SymbolEntry entry1() const { return _entry1; }
    protected:
        String toString2() const { return Implementation0::toString2() + space + _entry1.toString(); }
        SymbolEntry _entry1;
    };
    class Implementation2 : public Implementation1
    {
    public:
        Implementation2(Atom atom, SymbolEntry entry1, SymbolEntry entry2) : Implementation1(atom, entry1), _entry2(entry2) { }
        bool equals(const SymbolEntry::Implementation* other) const
        {
            const Implementation2* o = dynamic_cast<const Implementation2*>(other);
            if (o == 0)
                return false;
            return _atom == o->_atom && _entry1 == o->_entry1 && _entry2 == o->_entry2;
        }
        String toString() const { return openParenthesis + toString2() + closeParenthesis; }
        SymbolEntry entry2() const { return _entry1; }
    protected:
        String toString2() const { return Implementation1::toString2() + space + _entry2.toString(); }
        SymbolEntry _entry2;
    };
    class Implementation3 : public Implementation2
    {
    public:
        Implementation3(Atom atom, SymbolEntry entry1, SymbolEntry entry2, SymbolEntry entry3) : Implementation2(atom, entry1, entry2), _entry3(entry3) { }
        bool equals(const SymbolEntry::Implementation* other) const
        {
            const Implementation3* o = dynamic_cast<const Implementation3*>(other);
            if (o == 0)
                return false;
            return _atom == o->_atom && _entry1 == o->_entry1 && _entry2 == o->_entry2 && _entry3 == o->_entry3;
        }
        String toString() const { return openParenthesis + toString2() + closeParenthesis; }
        SymbolEntry entry3() const { return _entry1; }
    protected:
        String toString2() const { return Implementation2::toString2() + space + _entry2.toString(); }
        SymbolEntry _entry3;
    };

    Symbol(Reference<Symbol::Implementation> implementation) : SymbolEntry(implementation) { }
    friend class SymbolEntry;
    static GrowableArray<int, Symbol*> _labelled;
};

GrowableArray<Symbol*> Symbol::_labelled;

class SymbolList : public SymbolEntry
{
public:
    SymbolList() : SymbolEntry(emptyList()) { }
    SymbolList(Symbol head) : SymbolEntry(new Implementation(head, SymbolList())) { }
    SymbolList(Symbol head, SymbolList tail) : SymbolEntry(new Implementation(head, tail)) { }
    bool isEmpty() const { return dynamic_cast<const ListImplementation*>(implementation())->isEmpty(); }
    Symbol head() const { return dynamic_cast<const Implementation*>(implementation())->head(); }
    SymbolList tail() const { return dynamic_cast<const Implementation*>(implementation())->tail(); }
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
        Implementation(Symbol head, SymbolList tail) : _head(head), _tail(tail) { }
        bool equals(const SymbolEntry::Implementation* other) const
        {
            const Implementation* o = dynamic_cast<const Implementation*>(other);
            if (o == 0)
                return false;
            return _head == o->_head && _tail == o->_tail;
        }
        bool isEmpty() const { return false; }
        Symbol head() const { return _head; }
        SymbolList tail() const { return _tail; }
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
        bool equals(const SymbolEntry::Implementation* other) const
        {
            return static_cast<const SymbolEntry::Implementation*>(this) == other;
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
//    SymbolList(Reference<Symbol::Implementation> implementation) : SymbolEntry(implementation) { }
    friend class Symbol;
    String toString2() { return dynamic_cast<const ListImplementation*>(implementation())->toString2(); } 
};

Reference<SymbolList::ListImplementation> SymbolList::_emptyList;

