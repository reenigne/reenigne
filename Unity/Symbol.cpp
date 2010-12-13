#include <stdint.h>

enum Atom
{
    atomAuto,
    atomBit,
    atomBoolean,
    atomByte,
    atomCharacter,
    atomClass,
    atomFunction,
    atomInt,
    atomPointer,
    atomString,
    atomTypeIdentifier,
    atomTypeOf,
    atomUInt,
    atomVoid,
    atomWord,

    atomLogicalOr,
    atomLogicalAnd,
    atomDot,

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
    atomNot,
    atomPositive,
    atomNegative,
    atomDereference,
    atomAddressOf,
    atomPower,
    atomFunctionCall,

    atomStringConstant,
    atomIdentifier,
    atomIntegerConstant,
    atomTrue,
    atomFalse,
    atomNull,

    atomLocation,
    atomSpan,
    
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

            _table[atomLocation] = String("location");
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
            r += backslash;
        }
    } while (true);
}

int quotedLength(String string)
{
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



template<class T> class SymbolEntryTemplate;
typedef SymbolEntryTemplate<void> SymbolEntry;

template<class T> class SymbolListTemplate;
typedef SymbolListTemplate<void> SymbolList;

template<class T> class SymbolTemplate;
typedef SymbolTemplate<void> Symbol;

template<class T> class SymbolEntryTemplate
{
public:
    SymbolEntryTemplate(int value) : _implementation(new IntegerImplementation(value)) { }
    SymbolEntryTemplate(String value) : _implementation(new StringImplementation(value)) { }
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
    SymbolListTemplate<T> list() const { return SymbolList(_implementation); }
    SymbolTemplate<T> symbol() const { return Symbol(_implementation); }
    Atom atom() const { return symbol().atom(); }
    SymbolTemplate<T> head() const { return list().head(); }
    SymbolListTemplate<T> tail() const { return list().tail(); }
    bool valid() const { return _implementation.valid(); }
    SymbolTemplate<T> target() const { return Symbol::_labelled[integer()].symbol(); }
    int length(int max) const { return implementation()->length(max); }
    String toString(int width, int spacesPerIndex, int indent, int& x, bool& more) const { return _implementation->toString(width, spacesPerIndex, indent, x, more); }
protected:
    class Implementation : public ReferenceCounted
    {
    public:
        virtual bool equals(const Implementation* other) const = 0;
        virtual int length(int max) const = 0;
        virtual String toString(int width, int spacesPerIndex, int indent, int& x, bool& more) const = 0;
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
        String toString(int width, int spacesPerIndex, int indent, int& x, bool& more) const
        { 
            x += decimalLength(_value);
            more = true;
            return String::decimal(_value);
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
        String toString(int width, int spacesPerIndex, int indent, int& x, bool& more) const
        { 
            x += quotedLength(_value);
            more = true;
            return quote(_value);
        }
        int length(int max) const { return quotedLength(_value); }
        String value() const { return _value; }
    private:
        String _value;
    };
    SymbolEntryTemplate(Implementation* implementation) : _implementation(implementation) { }
    const Implementation* implementation() const { return _implementation; }
    Implementation* implementation() { return _implementation; }
private:
    Reference<Implementation> _implementation;
    template<class T> friend class SymbolTemplate;
    template<class T> friend class SymbolListTemplate;
};

template<class T> class SymbolTemplate : public SymbolEntryTemplate<T>
{
public:
    SymbolTemplate(int label, Atom atom)
      : SymbolEntry(new Implementation0(label, atom)) { addToTable(); }
    SymbolTemplate(int label, Atom atom, SymbolEntry symbol1)
      : SymbolEntry(new Implementation1(label, atom, symbol1)) { addToTable(); }
    SymbolTemplate(int label, Atom atom, SymbolEntry symbol1, SymbolEntry symbol2)
      : SymbolEntry(new Implementation2(label, atom, symbol1, symbol2)) { addToTable(); }
    SymbolTemplate(int label, Atom atom, SymbolEntry symbol1, SymbolEntry symbol2, SymbolEntry symbol3)
      : SymbolEntry(new Implementation3(label, atom, symbol1, symbol2, symbol3)) { addToTable(); }
    SymbolTemplate(int label, Atom atom, SymbolEntry symbol1, SymbolEntry symbol2, SymbolEntry symbol3, SymbolEntry symbol4)
      : SymbolEntry(new Implementation4(label, atom, symbol1, symbol2, symbol3, symbol4)) { addToTable(); }
    SymbolTemplate(int label, Atom atom, SymbolEntry symbol1, SymbolEntry symbol2, SymbolEntry symbol3, SymbolEntry symbol4, SymbolEntry symbol5)
      : SymbolEntry(new Implementation5(label, atom, symbol1, symbol2, symbol3, symbol4, symbol5)) { addToTable(); }
    SymbolTemplate() { }
    SymbolTemplate(Atom atom)
      : SymbolEntry(new Implementation0(label, atom)) { addToTable(); }
    SymbolTemplate(Atom atom, SymbolEntry symbol1)
      : SymbolEntry(new Implementation1(-1, atom, symbol1)) { addToTable(); }
    SymbolTemplate(Atom atom, SymbolEntry symbol1, SymbolEntry symbol2)
      : SymbolEntry(new Implementation2(-1, atom, symbol1, symbol2)) { addToTable(); }
    SymbolTemplate(Atom atom, SymbolEntry symbol1, SymbolEntry symbol2, SymbolEntry symbol3)
      : SymbolEntry(new Implementation3(-1, atom, symbol1, symbol2, symbol3)) { addToTable(); }
    SymbolTemplate(Atom atom, SymbolEntry symbol1, SymbolEntry symbol2, SymbolEntry symbol3, SymbolEntry symbol4)
      : SymbolEntry(new Implementation4(-1, atom, symbol1, symbol2, symbol3, symbol4)) { addToTable(); }
    SymbolTemplate(Atom atom, SymbolEntry symbol1, SymbolEntry symbol2, SymbolEntry symbol3, SymbolEntry symbol4, SymbolEntry symbol5)
      : SymbolEntry(new Implementation5(-1, atom, symbol1, symbol2, symbol3, symbol4, symbol5)) { addToTable(); }

    Atom atom() const { return dynamic_cast<const Implementation0*>(implementation())->atom(); }
    SymbolEntry entry1() const { return dynamic_cast<const Implementation1*>(implementation())->entry1(); }
    SymbolEntry entry2() const { return dynamic_cast<const Implementation2*>(implementation())->entry2(); }
    SymbolEntry entry3() const { return dynamic_cast<const Implementation3*>(implementation())->entry3(); }
    SymbolEntry entry4() const { return dynamic_cast<const Implementation4*>(implementation())->entry4(); }
    SymbolEntry entry5() const { return dynamic_cast<const Implementation5*>(implementation())->entry5(); }
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
        String toString(int width, int spacesPerIndent, int indent, int& x, bool& more) const
        {
            ++x;
            more = true;
            String s = openParenthesis;
            if (_label != -1) {                                                        
                s = String::decimal(_label) + colon + s;
                x += 1 + decimalLength(_label);
            }
            String a = atomToString(_atom);
            x += a.length();
            s += a;
            bool canInlineNext = true;
            more = true;
            const Implementation1* implementation1 = dynamic_cast<const Implementation1*>(this);
            if (implementation1 == 0) {
                ++x;
                return s + closeParenthesis;
            }
            SymbolEntry entry1 = implementation1->entry1();
            if (canInlineNext && x + 1 + entry1.length(width - x) <= width - 1) {
                // Fits on the line - inline it
                s += space;
                ++x;
            }
            else {
                // Doesn't fit on the line - put it on its own line.
                s += newLine + String::padding(indent + 2);
                x = indent + 2;
                more = false;
            }
            s += entry1.toString(width, spacesPerIndent, indent + 2, x, canInlineNext);

            const Implementation2* implementation2 = dynamic_cast<const Implementation2*>(this);
            if (implementation2 == 0) {
                ++x;
                return s + closeParenthesis;
            }
            SymbolEntry entry2 = implementation2->entry2();
            if (canInlineNext && x + 1 + entry2.length(width - x) <= width - 1) {
                // Fits on the line - inline it
                s += space;
                ++x;
            }
            else {
                // Doesn't fit on the line - put it on its own line.
                s += newLine + String::padding(indent + 2);
                x = indent + 2;
                more = false;
            }
            s += entry2.toString(width, spacesPerIndent, indent + 2, x, canInlineNext);

            const Implementation3* implementation3 = dynamic_cast<const Implementation3*>(this);
            if (implementation3 == 0) {
                ++x;
                return s + closeParenthesis;
            }
            SymbolEntry entry3 = implementation3->entry3();
            if (canInlineNext && x + 1 + entry3.length(width - x) <= width - 1) {
                // Fits on the line - inline it
                s += space;
                ++x;
            }
            else {
                // Doesn't fit on the line - put it on its own line.
                s += newLine + String::padding(indent + 2);
                x = indent + 2;
                more = false;
            }
            s += entry3.toString(width, spacesPerIndent, indent + 2, x, canInlineNext);

            const Implementation4* implementation4 = dynamic_cast<const Implementation4*>(this);
            if (implementation4 == 0) {
                ++x;
                return s + closeParenthesis;
            }
            SymbolEntry entry4 = implementation4->entry4();
            if (canInlineNext && x + 1 + entry4.length(width - x) <= width - 1) {
                // Fits on the line - inline it
                s += space;
                ++x;
            }
            else {
                // Doesn't fit on the line - put it on its own line.
                s += newLine + String::padding(indent + 2);
                x = indent + 2;
                more = false;
            }
            s += entry4.toString(width, spacesPerIndent, indent + 2, x, canInlineNext);

            const Implementation5* implementation5 = dynamic_cast<const Implementation5*>(this);
            if (implementation5 == 0) {
                ++x;
                return s + closeParenthesis;
            }
            SymbolEntry entry5 = implementation5->entry5();
            if (canInlineNext && x + 1 + entry5.length(width - x) <= width - 1) {
                // Fits on the line - inline it
                s += space;
                ++x;
            }
            else {
                // Doesn't fit on the line - put it on its own line.
                s += newLine + String::padding(indent + 2);
                x = indent + 2;
                more = false;
            }
            s += entry5.toString(width, spacesPerIndent, indent + 2, x, canInlineNext);

            ++x;
            return s + closeParenthesis;
        }
        Atom atom() const { return _atom; }
        int label() const { return _label; }
        void removeLabel() { _label = -1; }
    protected:
        ~Implementation0()
        {
            if (_label == -1)
                return;
            _labelled[_label] = LabelTarget(&_labelled[_nextFreeLabel]);
            _nextFreeLabel = _label;
        }
        int _label;
        Atom _atom;
    };
    class Implementation1 : public Implementation0
    {
    public:
        Implementation1(int label, Atom atom, SymbolEntry entry1)
          : Implementation0(label, atom), _entry1(entry1) { }
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
        SymbolEntry _entry1;
    };
    class Implementation2 : public Implementation1
    {
    public:
        Implementation2(int label, Atom atom, SymbolEntry entry1, SymbolEntry entry2)
          : Implementation1(label, atom, entry1), _entry2(entry2) { }
        bool equals(const SymbolEntry::Implementation* other) const
        {
            const Implementation2* o = dynamic_cast<const Implementation2*>(other);
            if (o == 0)
                return false;
            return _atom == o->_atom && _entry1 == o->_entry1 && _entry2 == o->_entry2;
        }
        SymbolEntry entry2() const { return _entry2; }
        int length(int max) const
        {
            int r = Implementation1::length(max);
            if (r < max)
                r += 1 + _entry2.implementation()->length(max - r);
            return r;
        }
    protected:
        SymbolEntry _entry2;
    };
    class Implementation3 : public Implementation2
    {
    public:
        Implementation3(int label, Atom atom, SymbolEntry entry1, SymbolEntry entry2, SymbolEntry entry3)
          : Implementation2(label, atom, entry1, entry2), _entry3(entry3) { }
        bool equals(const SymbolEntry::Implementation* other) const
        {
            const Implementation3* o = dynamic_cast<const Implementation3*>(other);
            if (o == 0)
                return false;
            return _atom == o->_atom && _entry1 == o->_entry1 && _entry2 == o->_entry2 && _entry3 == o->_entry3;
        }
        SymbolEntry entry3() const { return _entry3; }
        int length(int max) const
        {
            int r = Implementation2::length(max);
            if (r < max)
                r += 1 + _entry3.implementation()->length(max - r);
            return r;
        }
    protected:
        SymbolEntry _entry3;
    };
    class Implementation4 : public Implementation3
    {
    public:
        Implementation4(int label, Atom atom, SymbolEntry entry1, SymbolEntry entry2, SymbolEntry entry3, SymbolEntry entry4)
          : Implementation3(label, atom, entry1, entry2, entry3), _entry4(entry4) { }
        bool equals(const SymbolEntry::Implementation* other) const
        {
            const Implementation4* o = dynamic_cast<const Implementation4*>(other);
            if (o == 0)
                return false;
            return _atom == o->_atom && _entry1 == o->_entry1 && _entry2 == o->_entry2 && _entry3 == o->_entry3 && _entry4 == o->_entry4;
        }
        SymbolEntry entry4() const { return _entry4; }
        int length(int max) const
        {
            int r = Implementation3::length(max);
            if (r < max)
                r += 1 + _entry4.implementation()->length(max - r);
            return r;
        }
    protected:
        SymbolEntry _entry4;
    };
    class Implementation5 : public Implementation4
    {
    public:
        Implementation5(int label, Atom atom, SymbolEntry entry1, SymbolEntry entry2, SymbolEntry entry3, SymbolEntry entry4, SymbolEntry entry5)
          : Implementation3(label, atom, entry1, entry2, entry3, entry4), _entry5(entry5) { }
        bool equals(const SymbolEntry::Implementation* other) const
        {
            const Implementation5* o = dynamic_cast<const Implementation5*>(other);
            if (o == 0)
                return false;
            return _atom == o->_atom && _entry1 == o->_entry1 && _entry2 == o->_entry2 && _entry3 == o->_entry3 && _entry4 == o->_entry4 && _entry5 == o->_entry5;
        }
        SymbolEntry entry5() const { return _entry5; }
        int length(int max) const
        {
            int r = Implementation4::length(max);
            if (r < max)
                r += 1 + _entry5.implementation()->length(max - r);
            return r;
        }
    protected:
        SymbolEntry _entry5;
    };

    class LabelTarget
    {
    public:
        LabelTarget() : _pointer(0) { }
        LabelTarget(Symbol* symbol) : _pointer(symbol) { }
        LabelTarget(LabelTarget* target) : _pointer(reinterpret_cast<char*>(target) + 1) { }
        Symbol symbol() const { return Symbol(static_cast<Implementation0*>(_pointer)); }
        LabelTarget* target() const { return reinterpret_cast<LabelTarget*>(static_cast<char*>(_pointer) - 1); }
        bool isSymbol() const { return (reinterpret_cast<uintptr_t>(_pointer) & 1) == 0; }
    private:
        void* _pointer;
    };

    static int labelFromTarget(LabelTarget* target)
    {
        return (target - &_labelled[0])/sizeof(LabelTarget*);
    }

public:
    static int newLabel()
    {
        int l = _nextFreeLabel;
        if (l == _labelled.count()) {
            ++_nextFreeLabel;
            _labelled.append(LabelTarget());
        }
        else
            _nextFreeLabel = labelFromTarget(_labelled[l].target());
        return l;
    }

private:
    void addToTable()
    {
        int l = label();
        if (_labelled[l].isSymbol())
            _labelled[l].symbol().removeLabel();
        _labelled[l] = LabelTarget(this);
    }

    void removeLabel()
    {
        dynamic_cast<Implementation0*>(implementation())->removeLabel();
    }

    SymbolTemplate(Reference<Symbol::Implementation0> implementation) : SymbolEntry(implementation) { }

    template<class T> friend class SymbolEntryTemplate;
    static GrowableArray<LabelTarget> _labelled;
    static int _nextFreeLabel;
};

GrowableArray<Symbol::LabelTarget> Symbol::_labelled;
int Symbol::_nextFreeLabel = 0;

template<class T> class SymbolListTemplate : public SymbolEntry
{
public:
    SymbolListTemplate() : SymbolEntry(emptyList()) { }
    SymbolListTemplate(Symbol head) : SymbolEntry(new Implementation(head, emptyList())) { }
    SymbolListTemplate(Symbol head, SymbolList tail) : SymbolEntry(new Implementation(head, Reference<ListImplementation>(_implementation))) { }
    bool isEmpty() const { return dynamic_cast<const ListImplementation*>(implementation())->isEmpty(); }
    Symbol head() const { return dynamic_cast<const Implementation*>(implementation())->head(); }
    SymbolList tail() const { return dynamic_cast<const Implementation*>(implementation())->tail(); }
private:
    class ListImplementation : public Symbol::Implementation
    {
    public:
        virtual bool isEmpty() const = 0;
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
        String toString(int width, int spacesPerIndent, int indent, int& x, bool& more) const
        {
            ++x;
            bool canInlineNext;
            String s = openBracket + _head.toString(width, spacesPerIndent, indent + 2, x, canInlineNext);

            const Implementation* tail = this;
            more = true;
            do {
                tail = dynamic_cast<const Implementation*>(static_cast<const ListImplementation*>(tail->_tail));
                if (tail == 0) {
                    ++x;
                    return s + closeBracket;
                }
                Symbol next = tail->_head;
                if (canInlineNext && x + 1 + next.length(width - x) <= width - 1) {
                    // Fits on the line - inline it
                    s += space;
                    ++x;
                }
                else {
                    // Doesn't fit on the line - put it on its own line.
                    s += newLine + String::padding(indent + 2);
                    x = indent + 2;
                    more = false;
                }
                s += next.toString(width, spacesPerIndent, indent + 2, x, canInlineNext);
            } while (true);
        }
        int length2(int max) const
        {
            int r = _head.implementation()->length(max);
            if (r < max && dynamic_cast<const Implementation*>(static_cast<const ListImplementation*>(_tail)) != 0)
                r += 1 + _tail->length2(max - r);
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
        String toString(int width, int spacesPerIndent, int indent, int& x, bool& more) const
        {
            x += 2;
            more = true;
            static String s("[]");
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
    template<class T> friend class SymbolEntryTemplate;
    SymbolListTemplate(Reference<ListImplementation> implementation) : SymbolEntry(implementation) { }
};

Reference<SymbolList::ListImplementation> SymbolList::_emptyList;

