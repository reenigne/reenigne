#include <stdint.h>

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
    static String doubleQuote("\"");
    CodePointSource s(string);
    String r = doubleQuote;
    int start = 0;
    int end;
    do {
        int c = s.get();
        if (c == -1) {
            end = s.offset();
            r += s.subString(start, end);
            return r + doubleQuote;
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

int quotedLength(String stinrg)
{
    static String doubleQuote("\"");
    CodePointSource s(string);
    int r = 2;
    do {
        int c = s.get();
        if (c == -1)
            return r;
        if (c == '"' || c == '\\')
            ++r;
        ++r;
    } while (true);
}

int decimalLength(int v)
{
    int l = 1;
    if (v < 0) {
        ++l;
        v = -v;
    }
    while (v > 9) {
        ++l;
        v /= 10;
    }
    return l;
}

String openParenthesis("(");
String closeParenthesis(")");
String space(" ");
String newLine = String::codePoint(10);


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
    SymbolList list() const { return SymbolList(_implementation); }
    Symbol symbol() const { return Symbol(_implementation); }
    Atom atom() const { return symbol().atom(); }
    Symbol head() const { return list().head(); }
    SymbolList tail() const { return list().tail(); }
    bool valid() const { return _implementation.valid(); }
    String toString(int width = 80, int spacesPerIndent = 2, bool ownLine = true, int indent = 0) const
    {
        int x = 0;
        return _implementation->toString(width, spacesPerIndent, ownLine, indent, x);
    }
    Symbol target() const { return Symbol::_labelled[integer()].symbol(); }
protected:
    class Implementation : public ReferenceCounted
    {
    public:
        virtual bool equals(const Implementation* other) const = 0;
        virtual int length(int max) const = 0;
        virtual String toString(int width, int spacesPerIndent, bool ownLine, int indent, int& x) const = 0;
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
        String toString(int width, int spacesPerIndent, bool ownLine, int indent, int& x) const
        { 
            static String space(" ");
            static String newLine = String::codePoint(10);
            int l = decimalLength(_value);
            if (!ownLine && (width - x) > 1 + l) {
                x += l + 1;
                return space + String::decimal(_value);
            }
            x = indent + l;
            if (!ownLine)
                return newLine + String::padding(indent) + String::decimal(_value);
            return String::padding(indent) + String::decimal(_value);
        }
        int length(int max) const { return decimalLength(_value); }
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
        String toString(int width, int spacesPerIndent, bool ownLine, int indent, int& x) const
        {
            int l = quotedLength(_value);
            if (!ownLine && (width - x) > 1 + l) {
                x += l + 1;
                return space + quote(_value);
            }
            x = indent + l;
            if (!ownLine)
                return newLine + String::padding(indent) + quote(_value);
            return String::padding(indent) + quote(_value);
        }
        int length(int max) const { return quotedLength(_value); }
        String value() const { return _value; }
    private:
        String _value;
    };
    SymbolEntry(Implementation* implementation) : _implementation(implementation) { }
    const Implementation* implementation() const { return _implementation; }
    Implementation* implementation() { return _implementation; }
private:
    Reference<Implementation> _implementation;
    friend class Symbol;
    friend class SymbolList;
};

class SymbolLabelTarget
{
public:
    SymbolLabelTarget() : _pointer(0) { }
    SymbolLabelTarget(Symbol* symbol) : _pointer(symbol) { }
    SymbolLabelTarget(SymbolLabelTarget* target) : _pointer(reinterpret_cast<char*>(target) + 1) { }
    Symbol symbol() const { return Symbol(static_cast<Symbol::Implementation0*>(_pointer)); }
    SymbolLabelTarget* target() const { return reinterpret_cast<SymbolLabelTarget*>(static_cast<char*>(_pointer) - 1); }
    bool isSymbol() const { return (reinterpret_cast<uintptr_t>(_pointer) & 1) == 0; }
private:
    void* _pointer;
};

class Symbol : public SymbolEntry
{
public:
    Symbol(int label, Atom atom) : SymbolEntry(new Implementation0(label, atom)) { addToTable(); }
    Symbol(int label, Atom atom, SymbolEntry symbol1) : SymbolEntry(new Implementation1(label, atom, symbol1)) { addToTable(); }
    Symbol(int label, Atom atom, SymbolEntry symbol1, SymbolEntry symbol2) : SymbolEntry(new Implementation2(label, atom, symbol1, symbol2)) { addToTable(); }
    Symbol(int label, Atom atom, SymbolEntry symbol1, SymbolEntry symbol2, SymbolEntry symbol3) : SymbolEntry(new Implementation3(label, atom, symbol1, symbol2, symbol3)) { addToTable(); }
    Atom atom() const { return dynamic_cast<const Implementation0*>(implementation())->atom(); }
    SymbolEntry entry1() const { return dynamic_cast<const Implementation1*>(implementation())->entry1(); }
    SymbolEntry entry2() const { return dynamic_cast<const Implementation2*>(implementation())->entry2(); }
    SymbolEntry entry3() const { return dynamic_cast<const Implementation3*>(implementation())->entry3(); }
    int label() const { return dynamic_cast<const Implementation0*>(implementation())->label(); }
private:
    class Implementation0 : public SymbolEntry::Implementation
    {
    public:
        Implementation0(int label, Atom atom) : _label(label), _atom(atom) { }
        bool equals(const SymbolEntry::Implementation* other) const
        {
            const Implementation0* o = dynamic_cast<const Implementation0*>(other);
            if (o == 0)
                return false;
            return _atom == o->_atom;
        }
        int length(int max) const
        {
            int r = 2 + atomToString(_atom).length();
            if (_label != -1)
                r += 1 + decimalLength(_label);
            return r;
        }
        String toString(int width, int spacesPerIndent, bool ownLine, int indent, int& x) const
        {
            String s = openParenthesis;
            if (_label != -1) {                                                        
                static String colon(":");
                s = String::decimal(_label) + colon + s;
            }
            return s + toString2() + closeParenthesis;
        }
        Atom atom() const { return _atom; }
        int label() const { return _label; }
        void removeLabel() { _label = -1; }
    protected:
        ~Implementation0()
        {
            if (_label == -1)
                return;
            _labelled[_label] = SymbolLabelTarget(&_labelled[_nextFreeLabel]);
            _nextFreeLabel = _label;
        }
        virtual String toString2() const { return atomToString(_atom); }
        int _label;
        Atom _atom;
    };
    class Implementation1 : public Implementation0
    {
    public:
        Implementation1(int label, Atom atom, SymbolEntry entry1) : Implementation0(label, atom), _entry1(entry1) { }
        bool equals(const SymbolEntry::Implementation* other) const
        {
            const Implementation1* o = dynamic_cast<const Implementation1*>(other);
            if (o == 0)
                return false;
            return _atom == o->_atom && _entry1 == o->_entry1;
        }
        SymbolEntry entry1() const { return _entry1; }
        int length(int max) const
        {
            int r = Implementation0::length(max);
            if (r < max)
                r += 1 + _entry1.implementation()->length(max - r);
            return r;
        }
    protected:
        String toString2() const { return Implementation0::toString2() + space + _entry1.toString(); }
        SymbolEntry _entry1;
    };
    class Implementation2 : public Implementation1
    {
    public:
        Implementation2(int label, Atom atom, SymbolEntry entry1, SymbolEntry entry2) : Implementation1(label, atom, entry1), _entry2(entry2) { }
        bool equals(const SymbolEntry::Implementation* other) const
        {
            const Implementation2* o = dynamic_cast<const Implementation2*>(other);
            if (o == 0)
                return false;
            return _atom == o->_atom && _entry1 == o->_entry1 && _entry2 == o->_entry2;
        }
        SymbolEntry entry2() const { return _entry1; }
        int length(int max) const
        {
            int r = Implementation1::length(max);
            if (r < max)
                r += 1 + _entry2.implementation()->length(max - r);
            return r;
        }
    protected:
        String toString2() const { return Implementation1::toString2() + space + _entry2.toString(); }
        SymbolEntry _entry2;
    };
    class Implementation3 : public Implementation2
    {
    public:
        Implementation3(int label, Atom atom, SymbolEntry entry1, SymbolEntry entry2, SymbolEntry entry3) : Implementation2(label, atom, entry1, entry2), _entry3(entry3) { }
        bool equals(const SymbolEntry::Implementation* other) const
        {
            const Implementation3* o = dynamic_cast<const Implementation3*>(other);
            if (o == 0)
                return false;
            return _atom == o->_atom && _entry1 == o->_entry1 && _entry2 == o->_entry2 && _entry3 == o->_entry3;
        }
        SymbolEntry entry3() const { return _entry1; }
        int length(int max) const
        {
            int r = Implementation2::length(max);
            if (r < max)
                r += 1 + _entry3.implementation()->length(max - r);
            return r;
        }
    protected:
        String toString2() const { return Implementation2::toString2() + space + _entry2.toString(); }
        SymbolEntry _entry3;
    };

    static int labelFromTarget(SymbolLabelTarget* target)
    {
        return (target - &_labelled[0])/sizeof(SymbolLabelTarget*);
    }

    static int newLabel()
    {
        int l = _nextFreeLabel;
        if (l == _labelled.count()) {
            ++_nextFreeLabel;
            _labelled.append(SymbolLabelTarget());
        }
        else
            _nextFreeLabel = labelFromTarget(_labelled[l].target());
        return l;
    }

    void addToTable()
    {
        int l = label();
        if (_labelled[l].isSymbol())
            _labelled[l].symbol().removeLabel();
        _labelled[l] = SymbolLabelTarget(this);
    }

    void removeLabel()
    {
        dynamic_cast<Implementation0*>(implementation())->removeLabel();
    }

    Symbol(Reference<Symbol::Implementation0> implementation) : SymbolEntry(implementation) { }
    friend class SymbolEntry;
    friend class SymbolLabelTarget;
    static GrowableArray<SymbolLabelTarget> _labelled;
    static int _nextFreeLabel;
};

GrowableArray<SymbolLabelTarget> Symbol::_labelled;
int Symbol::_nextFreeLabel = 0;

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
        int length(int max) const { return 2 + length2(max - 2); }
        virtual int length2(int max) const = 0;
    };
    class Implementation : public ListImplementation
    {
    public:
        Implementation(Symbol head, Reference<ListImplementation> tail) : _head(head), _tail(tail) { }
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
        String toString(int width, int spacesPerIndent, bool ownLine, int indent, int& x) const
        {
            static String openBracket("[");
            static String closeBracket("]");

            int l = length2(width - indent);
            String s;
            if (!ownLine) {
                if ((width - x) > 1 + l) {
                    x += l + 1;
                    return space + openBracket + _head.toString() + _tail->toString2() + closeBracket;
                }
                s = newLine;
                x = indent;
            }
            if ((width - x) > l)
                return s + openBracket + _head.toString() + _tail->toString2() + closeBracket;
            do {


                
            //if (fits entirely on the line)
            //    output as single line
            //else
            //    add sub-elements to line until it is full, then start a new line at the same indent level, until we're done.

            x = indent + l;
            if (!ownLine)
                return newLine + String::padding(indent) + String::decimal(_value);
            return String::padding(indent) + String::decimal(_value);


            return openBracket + _head.toString() + _tail->toString2() + closeBracket;
        }
        String toString2() const
        {
            static String space(" ");
            return space + _head.toString() + _tail->toString2();
        }
        int length2(int max) const
        {
            int r = _head.implementation()->length(max);
            if (r < max && dynamic_cast<const Implementation*>(static_cast<const ListImplementation*>(_tail)) != 0)
                r += 1 + _tail->length(max - r);
            return r;
        }
    private:
        Symbol _head;
        Reference<ListImplementation> _tail;
    };
    class EmptyImplementation : public ListImplementation
    {
    public:
        bool equals(const SymbolEntry::Implementation* other) const
        {
            return static_cast<const SymbolEntry::Implementation*>(this) == other;
        }
        bool isEmpty() const { return true; }
        String toString(int width, int spacesPerIndent, bool ownLine, int indent, int& x) const
        {
            static String s("[]");
            if (!ownLine && (width - x) > 3) {
                x += 3;
                return space + s;
            }
            x = indent + 2;
            if (!ownLine)
                return newLine + String::padding(indent) + s;
            return String::padding(indent) + s;
        }
        String toString2() const
        {
            static String s("");
            return s;
        }
        int length2(int max) const { return 0; }
    };
    static Reference<Implementation> emptyList()
    {
        if (!_emptyList.valid())
            _emptyList = new EmptyImplementation;
        return _emptyList;
    }
    static Reference<ListImplementation> _emptyList;
    friend class SymbolEntry;
    String toString2() { return dynamic_cast<const ListImplementation*>(implementation())->toString2(); } 
    SymbolList(Reference<ListImplementation> implementation) : SymbolEntry(implementation) { }
};

Reference<SymbolList::ListImplementation> SymbolList::_emptyList;
