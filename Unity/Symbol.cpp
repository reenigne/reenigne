enum Atom
{
    lastAtom
};

class Symbol
{
public:
    bool operator==(const Symbol& other)
    {
        return _implementation->equals(other->_implementation);
    }
    bool operator!=(const Symbol& other)
    {
        return !_implementation->equals(other->_implementation);
    }
    IntegerSymbol integer() const { return IntegerSymbol(*this); }
    StringSymbol string() const { return StringSymbol(*this); }
    ListSymbol list() const { return ListSymbol(*this); }
    TupleSymbol tuple() const { return TupleSymbol(*this); }
protected:
    class Implementation : public ReferenceCounted
    {
    public:
        virtual bool equals(Implementation* other) = 0;
    };
    Symbol(Implementation* implementation) : _implementation(implementation) { }
    Implementation* implementation() { return _implementation; }
private:
    Reference<Implementation> _implementation;
};

class IntegerSymbol : public Symbol
{
public:
    IntegerSymbol(int value) : Symbol(new Implementation(value)) { }
    int value() const { return dynamic_cast<Implementation*>(implementation())->value(); }
private:
    class Implementation : public Symbol::Implementation
    {
    public:
        Implementation(int value) : _value(value) { }
        bool equals(Symbol::Implementation* other)
        {
            Implementation* o = dynamic_cast<Implementation*>(other);
            if (o == 0)
                return false;
            return _value == o->_value;
        }
        int value() const { return _value; }
    private:
        int _value;
    }
};

class StringSymbol : public Symbol
{
public:
    StringSymbol(String value) : Symbol(new Implementation(value)) { }
    String value() const { return dynamic_cast<Implementation*>(implementation())->value(); }
private:
    class Implementation : public Symbol::Implementation
    {
    public:
        Implementation(String value) : _value(value) { }
        bool equals(Symbol::Implementation* other)
        {
            Implementation* o = dynamic_cast<Implementation*>(other);
            if (o == 0)
                return false;
            return _value == o->_value;
        }
        String value() const { return _value; }
    private:
        String _value;
    }
};

class ListSymbol : public Symbol
{
public:
    ListSymbol() : Symbol(emptyList()) { }
    ListSymbol(Symbol head) : Symbol(new Implementation(head, emptyList())) { }
    ListSymbol(Symbol head, Symbol tail) : Symbol(new Implementation(head, tail)) { }
    bool isEmpty() const { return dynamic_cast<ListImplementation*>(implementation())->isEmpty(); }
    Symbol head() const { return dynamic_cast<Implementation*>(implementation())->head(); }
    ListSymbol tail() const { return ListSymbol(dynamic_cast<Implementation*>(implementation())->tail()); }
private:
    class ListImplementation : public Symbol::Implementation
    {
    public:
        virtual bool isEmpty() const = 0;
    };
    class Implementation : public ListImplementation
    {
    public:
        Implementation(Symbol head, Symbol tail) : _head(head), _tail(tail) { }
        bool equals(Symbol::Implementation* other)
        {
            Implementation* o = dynamic_cast<Implementation*>(other);
            if (o == 0)
                return false;
            return _head == o->_head && _tail == o->_tail;
        }
        bool isEmpty() const { return false; }
    private:
        Symbol _head;
        Symbol _tail;
    };
    class EmptyImplementation : public ListImplementation
    {
    public:
        bool equals(Symbol::Implementation* other)
        {
            return static_cast<Symbol::Implementation*>(this) == other;
        }
        bool isEmpty() const { return true; }
    };
    static Reference<Implementation> emptyList()
    {
        if (!_emptyList.valid())
            _emptyList = new EmptyImplementation;
        return _emptyList;
    }
    static Reference<Implementation> _emptyList;
};

Reference<ListSymbol::Implementation> ListSymbol::_emptyList;

class TupleSymbol : public Symbol
{
public:
    TupleSymbol(Atom atom) : Symbol(new Implementation(atom)) { }
    TupleSymbol(Atom atom, Symbol symbol1) : Symbol(new Implementation(atom, symbol1)) { }
    Atom atom() const { return dynamic_cast<Implementation*>(implementation())->atom(); }
    Symbol symbol1() const { return dynamic_cast<Implementation*>(implementation())->symbol1(); }
private:
    class Implementation : public Symbol::Implementation
    {
    public:
        Implementation(Atom atom) : _atom(atom) { }
        Implementation(Atom atom, Symbol symbol1) : _atom(atom)
        {
            _array.allocate(1);
            _array[0] = symbol;
        }
        bool equals(Symbol::Implementation* other)
        {
            Implementation* o = dynamic_cast<Implementation*>(other);
            if (o == 0)
                return false;
            return _atom == o->_atom && _array == o->_array;
        }
        Atom atom() const { return _atom; }
        Symbol symbol1() const { return _array[0]; }
    private:
        Atom _atom;
        Array<Symbol> _array;
    }
};
