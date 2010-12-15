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

    atomParameter,

    atomExpressionStatement,
    atomFunctionDefinitionStatement,
    atomFromStatement,
    atomVariableDefinitionStatement,
    atomAssignmentStatement,
    atomAddAssignmentStatement,
    atomSubtractAssignmentStatement,
    atomMultiplyAssignmentStatement,
    atomDivideAssignmentStatement,
    atomModuloAssignmentStatement,
    atomShiftLeftAssignmentStatement,
    atomShiftRightAssignmentStatement,
    atomAndAssignmentStatement,
    atomOrAssignmentStatement,
    atomXorAssignmentStatement,
    atomPowerAssignmentStatement,
    atomCompoundStatement,

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
            _table[atomAuto] = String("Auto");
            _table[atomBit] = String("Bit");
            _table[atomBoolean] = String("Boolean");
            _table[atomByte] = String("Byte");
            _table[atomCharacter] = String("Character");
            _table[atomClass] = String("Class");
            _table[atomFunction] = String("Function");
            _table[atomInt] = String("Int");
            _table[atomPointer] = String("Pointer");
            _table[atomString] = String("String");
            _table[atomTypeIdentifier] = String("TypeIdentifier");
            _table[atomTypeOf] = String("TypeOf");
            _table[atomUInt] = String("UInt");
            _table[atomVoid] = String("Void");
            _table[atomWord] = String("Word");

            _table[atomLogicalOr] = String("||");
            _table[atomLogicalAnd] = String("&&");
            _table[atomDot] = String(".");

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
            _table[atomNot] = String("!");
            _table[atomPositive] = String("u+");
            _table[atomNegative] = String("u-");
            _table[atomDereference] = String("u*");
            _table[atomAddressOf] = String("u&");
            _table[atomPower] = String("^");
            _table[atomFunctionCall] = String("call");

            _table[atomStringConstant] = String("string");
            _table[atomIdentifier] = String("identifier");
            _table[atomIntegerConstant] = String("integer");
            _table[atomTrue] = String("true");
            _table[atomFalse] = String("false");
            _table[atomNull] = String("null");

            _table[atomParameter] = String("parameter");

            _table[atomExpressionStatement] = String("expression");
            _table[atomFunctionDefinitionStatement] = String("functionDefinition");
            _table[atomFromStatement] = String("from");
            _table[atomVariableDefinitionStatement] = String("variableDefinition");
            _table[atomAssignmentStatement] = String("=");
            _table[atomAddAssignmentStatement] = String("+=");
            _table[atomSubtractAssignmentStatement] = String("-=");
            _table[atomMultiplyAssignmentStatement] = String("*=");
            _table[atomDivideAssignmentStatement] = String("/=");
            _table[atomModuloAssignmentStatement] = String("%=");
            _table[atomShiftLeftAssignmentStatement] = String("<<=");
            _table[atomShiftRightAssignmentStatement] = String(">>=");
            _table[atomAndAssignmentStatement] = String("&=");
            _table[atomOrAssignmentStatement] = String("|=");
            _table[atomXorAssignmentStatement] = String("~=");
            _table[atomPowerAssignmentStatement] = String("^=");
            _table[atomCompoundStatement] = String("compound");

            _table[atomLocation] = String("location");
            _table[atomSpan] = String("span");
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
    int hash() const { return implementation()->hash(); }
protected:
    class Implementation : public ReferenceCounted
    {
    public:
        virtual bool equals(const Implementation* other) const = 0;
        virtual int length(int max) const = 0;
        virtual String toString(int width, int spacesPerIndex, int indent, int& x, bool& more) const = 0;
        int hash() const = 0;
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
        int hash() const { return _value; }
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
        int hash() const { return _value.hash(); }
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

class SymbolTail : public ReferenceCounted
{
public:
    SymbolTail(SymbolEntry head) : _head(head) { }
    SymbolTail(SymbolEntry head, SymbolTail* tail) : _head(head), _tail(tail) { }
    SymbolEntry head() const { return _head; }
    const SymbolTail* tail() const { return _tail; }
    bool equals(const SymbolTail* other) const
    { 
        if (this == other)
            return true;
        if (other == 0)
            return false;
        if (_head != other->_head)
            return false;
        if (_tail.valid())
            return _tail->equals(other->_tail);
        return !other->_tail.valid();
    }
    int length(int max)
    {
        int r = _head.length(max);
        if (r < max && _tail.valid())
            r += _tail->length(max - r);
        return r;
    }
private:
    SymbolEntry _head;
    Reference<SymbolTail> _tail;
};

template<class T> class SymbolTemplate : public SymbolEntryTemplate<T>
{
public:
    SymbolTemplate(int label, Atom atom, DiagnosticSpan span)
      : SymbolEntry(new Implementation(label, atom, span, 0)) { addToTable(); }
    SymbolTemplate(int label, Atom atom, DiagnosticSpan span, SymbolEntry symbol1)
      : SymbolEntry(new Implementation(label, atom, span, SymbolTail(symbol1, 0))) { addToTable(); }
    SymbolTemplate(int label, Atom atom, DiagnosticSpan span, SymbolEntry symbol1, SymbolEntry symbol2)
      : SymbolEntry(new Implementation(label, atom, span, SymbolTail(symbol1, SymbolTail(symbol2, 0)))) { addToTable(); }
    SymbolTemplate(int label, Atom atom, DiagnosticSpan span, SymbolEntry symbol1, SymbolEntry symbol2, SymbolEntry symbol3)
      : SymbolEntry(new Implementation(label, atom, span, SymbolTail(symbol1, SymbolTail(symbol2, SymbolTail(symbol3, 0))))) { addToTable(); }
    SymbolTemplate(int label, Atom atom, DiagnosticSpan span, SymbolEntry symbol1, SymbolEntry symbol2, SymbolEntry symbol3, SymbolEntry symbol4)
      : SymbolEntry(new Implementation(label, atom, span, SymbolTail(symbol1, SymbolTail(symbol2, SymbolTail(symbol3, SymbolTail(symbol4, 0)))))) { addToTable(); }
    SymbolTemplate(int label, Atom atom, DiagnosticSpan span, SymbolEntry symbol1, SymbolEntry symbol2, SymbolEntry symbol3, SymbolEntry symbol4, SymbolEntry symbol5)
      : SymbolEntry(new Implementation(label, atom, span, SymbolTail(symbol1, SymbolTail(symbol2, SymbolTail(symbol3, SymbolTail(symbol4, SymbolTail(symbol5, 0))))))) { addToTable(); }
    SymbolTemplate() { }
    SymbolTemplate(Atom atom, DiagnosticSpan span)
      : SymbolEntry(new Implementation(-1, atom, span, 0)) { }
    SymbolTemplate(Atom atom, DiagnosticSpan span, SymbolEntry symbol1)
      : SymbolEntry(new Implementation(-1, atom, span, SymbolTail(symbol1, 0))) { }
    SymbolTemplate(Atom atom, DiagnosticSpan span, SymbolEntry symbol1, SymbolEntry symbol2)
      : SymbolEntry(new Implementation(-1, atom, span, SymbolTail(symbol1, SymbolEntry(symbol2, 0)))) { }
    SymbolTemplate(Atom atom, DiagnosticSpan span, SymbolEntry symbol1, SymbolEntry symbol2, SymbolEntry symbol3)
      : SymbolEntry(new Implementation(-1, atom, span, SymbolTail(symbol1, SymbolTail(symbol2, SymbolTail(symbol3, 0))))) { }
    SymbolTemplate(Atom atom, DiagnosticSpan span, SymbolEntry symbol1, SymbolEntry symbol2, SymbolEntry symbol3, SymbolEntry symbol4)
      : SymbolEntry(new Implementation(-1, atom, span, SymbolTail(symbol1, SymbolTail(symbol2, SymbolTail(symbol3, SymbolTail(symbol4, 0)))))) { }
    SymbolTemplate(Atom atom, DiagnosticSpan span, SymbolEntry symbol1, SymbolEntry symbol2, SymbolEntry symbol3, SymbolEntry symbol4, SymbolEntry symbol5)
      : SymbolEntry(new Implementation(-1, atom, span, SymbolTail(symbol1, SymbolTail(symbol2, SymbolTail(symbol3, SymbolTail(symbol4, SymbolTail(symbol5, 0))))))) { }

    SymbolTemplate(Atom atom, DiagnosticSpan span, const SymbolTail* tail)
      : SymbolEntry(new Implementation(-1, atom, span, tail)) { }

    Atom atom() const { return dynamic_cast<const Implementation*>(implementation())->atom(); }
    DiagnosticSpan span() const { return dynamic_cast<const Implementation*>(implementation())->span(); }
    SymbolEntry operator[](int n) const
    {
        SymbolTail* t = tail();
        while (n > 1) {
            --n;
            t = t->tail();
        }
        return _tail.head();
    }
    int label() const { return dynamic_cast<const Implementation*>(implementation())->label(); }
    const SymbolTail* tail() const { return dynamic_cast<const Implementation*>(implementation())->tail(); }
private:
    class Implementation : public SymbolEntry::Implementation
    {
    public:
        Implementation(int label, Atom atom, DiagnosticSpan span, const SymbolTail* tail)
          : _label(label), _atom(atom), _span(span), _tail(tail)
        { }
        bool equals(const SymbolEntry::Implementation* other) const
        {
            const Implementation* o = dynamic_cast<const Implementation*>(other);
            if (o == 0)
                return false;
            return _atom == o->_atom && _tail->equals(other->_tail);
        }
        int length(int max) const
        {
            int r = 2 + atomToString(_atom).length();
            if (_label != -1)
                r += 1 + decimalLength(_label);
            if (r < max && _tail.valid()) {
                ++r;
                r += _tail->length(max - r);
            }
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

            SymbolTail* tail = _tail;
            while (tail != 0) {
                SymbolEntry entry = tail->head();
                if (canInlineNext && x + 1 + entry.length(width - x) <= width - 1) {
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
                s += entry.toString(width, spacesPerIndent, indent + 2, x, canInlineNext);
                tail = tail->tail();
            }
            ++x;
            return s + closeParenthesis;
        }

        Atom atom() const { return _atom; }
        int label() const { return _label; }
        DiagnosticSpan span() const { return _span; }
        void removeLabel() { _label = -1; }
        int hash() const
        { 
            int h = atom();
            SymbolTail* t = _tail;
            while (t != 0) {
                h = h * 67 + t->head().hash() - 113;
                t = t->_tail;
            }
            return h; 
        }
    private:
        ~Implementation()
        {
            if (_label == -1)
                return;
            _labelled[_label] = LabelTarget(&_labelled[_nextFreeLabel]);
            _nextFreeLabel = _label;
        }
        int _label;
        Atom _atom;
        DiagnosticSpan _span;
        Reference<SymbolTail> _tail;
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
    int hash() const { return dynamic_cast<const Implementation*>(implementation())->hash(); }

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
        int hash() const
        {
            return _head.hash() * 67 + _tail->hash() - 113;
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
        int hash() const { return atomLast; }
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
