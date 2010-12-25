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
    atomTypeAliasStatement,
    atomNothingStatement,
    atomIncrementStatement,
    atomDecrementStatement,
    atomIfStatement,
    atomSwitchStatement,
    atomReturnStatement,
    atomIncludeStatement,
    atomBreakStatement,
    atomContinueStatement,
    atomForeverStatement,
    atomWhileStatement,
    atomUntilStatement,
    atomForStatement,

    atomCase,
    atomDefaultCase,

    atomPrintFunction,

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
            _table[atomFunction] = String("Function");                               // returnType     argumentTypes
            _table[atomInt] = String("Int");
            _table[atomPointer] = String("Pointer");                                 // referentType
            _table[atomString] = String("String");
            _table[atomTypeIdentifier] = String("TypeIdentifier");                   // name
            _table[atomTypeOf] = String("TypeOf");                                   // expression
            _table[atomUInt] = String("UInt");                     
            _table[atomVoid] = String("Void");
            _table[atomWord] = String("Word");

            _table[atomLogicalOr] = String("||");                                    // leftExpression rightExpression
            _table[atomLogicalAnd] = String("&&");                                   // leftExpression rightExpression
            _table[atomDot] = String(".");                                           // leftExpression rightExpression

            _table[atomBitwiseOr] = String("|");                                     // leftExpression rightExpression
            _table[atomBitwiseXor] = String("~");                                    // leftExpression rightExpression
            _table[atomBitwiseAnd] = String("&");                                    // leftExpression rightExpression
            _table[atomEqualTo] = String("==");                                      // leftExpression rightExpression
            _table[atomNotEqualTo] = String("!=");                                   // leftExpression rightExpression
            _table[atomLessThanOrEqualTo] = String("<=");                            // leftExpression rightExpression
            _table[atomGreaterThanOrEqualTo] = String(">=");                         // leftExpression rightExpression
            _table[atomLessThan] = String("<");                                      // leftExpression rightExpression
            _table[atomGreaterThan] = String(">");                                   // leftExpression rightExpression
            _table[atomLeftShift] = String("<<");                                    // leftExpression rightExpression
            _table[atomRightShift] = String(">>");                                   // leftExpression rightExpression
            _table[atomAdd] = String("+");                                           // leftExpression rightExpression
            _table[atomSubtract] = String("-");                                      // leftExpression rightExpression
            _table[atomMultiply] = String("*");                                      // leftExpression rightExpression
            _table[atomDivide] = String("/");                                        // leftExpression rightExpression
            _table[atomModulo] = String("%");                                        // leftExpression rightExpression
            _table[atomNot] = String("!");                                           // expression
            _table[atomPositive] = String("u+");                                     // expression
            _table[atomNegative] = String("u-");                                     // expression
            _table[atomDereference] = String("u*");                                  // expression
            _table[atomAddressOf] = String("u&");                                    // expression
            _table[atomPower] = String("^");                                         // leftExpression rightExpression
            _table[atomFunctionCall] = String("call");                               // expression     arguments

            _table[atomStringConstant] = String("string");                           // string
            _table[atomIdentifier] = String("identifier");                           // name
            _table[atomIntegerConstant] = String("integer");                         // value
            _table[atomTrue] = String("true");
            _table[atomFalse] = String("false");
            _table[atomNull] = String("null");

            _table[atomParameter] = String("parameter");                             // typeSpecifier  name

            _table[atomExpressionStatement] = String("expression");                  // expression
            _table[atomFunctionDefinitionStatement] = String("functionDefinition");  // returnType     name            parameters     statement
            _table[atomFromStatement] = String("from");                              // dllExpression
            _table[atomVariableDefinitionStatement] = String("variableDefinition");  // typeSpecifier  identifier      initializer
            _table[atomAssignmentStatement] = String("=");                           // leftExpression rightExpression
            _table[atomAddAssignmentStatement] = String("+=");                       // leftExpression rightExpression
            _table[atomSubtractAssignmentStatement] = String("-=");                  // leftExpression rightExpression
            _table[atomMultiplyAssignmentStatement] = String("*=");                  // leftExpression rightExpression
            _table[atomDivideAssignmentStatement] = String("/=");                    // leftExpression rightExpression
            _table[atomModuloAssignmentStatement] = String("%=");                    // leftExpression rightExpression
            _table[atomShiftLeftAssignmentStatement] = String("<<=");                // leftExpression rightExpression
            _table[atomShiftRightAssignmentStatement] = String(">>=");               // leftExpression rightExpression
            _table[atomAndAssignmentStatement] = String("&=");                       // leftExpression rightExpression
            _table[atomOrAssignmentStatement] = String("|=");                        // leftExpression rightExpression
            _table[atomXorAssignmentStatement] = String("~=");                       // leftExpression rightExpression
            _table[atomPowerAssignmentStatement] = String("^=");                     // leftExpression rightExpression
            _table[atomCompoundStatement] = String("compound");                      // statements
            _table[atomTypeAliasStatement] = String("type");                         // typeIdentifier typeSpecifier
            _table[atomNothingStatement] = String("nothing");
            _table[atomIncrementStatement] = String("++");                           // expression
            _table[atomDecrementStatement] = String("--");                           // expression
            _table[atomIfStatement] = String("if");                                  // condition      trueStatement   falseStatement
            _table[atomSwitchStatement] = String("switch");                          // expression     defaultCase     cases
            _table[atomReturnStatement] = String("return");                          // expression
            _table[atomIncludeStatement] = String("include");                        // expression
            _table[atomBreakStatement] = String("break");                            // statement
            _table[atomContinueStatement] = String("continue");
            _table[atomForeverStatement] = String("forever");                        // statement
            _table[atomWhileStatement] = String("while");                            // doStatement    condition       statement      doneStatement
            _table[atomUntilStatement] = String("until");                            // doStatement    condition       statement      doneStatement
            _table[atomForStatement] = String("for");                                // preStatement   expression      postStatement  statement     doneStatement

            _table[atomCase] = String("case");                                       // expressions    statement
            _table[atomDefaultCase] = String("default");                             // statement

            _table[atomPrintFunction] = String("print");                             // returnType     name            parameters
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

template<class T> class SymbolTemplate;
typedef SymbolTemplate<void> Symbol;

template<class T> class SymbolArrayTemplate;
typedef SymbolArrayTemplate<void> SymbolArray;

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
    SymbolArrayTemplate<T> array() const { return SymbolArray(_implementation); }
    SymbolTemplate<T> symbol() const { return Symbol(_implementation); }
    Atom atom() const { return symbol().atom(); }
    bool valid() const { return _implementation.valid(); }
    SymbolTemplate<T> target() const { return Symbol::_labelled[integer()].symbol(); }
    int length(int max) const { return implementation()->length(max); }
    String toString(int width, int spacesPerIndex, int indent, int& x, bool& more) const { return _implementation->toString(width, spacesPerIndex, indent, x, more); }
    int hash() const { return implementation()->hash(); }
    bool isSymbol() const { return _implementation()->isSymbol(); }
    bool isArray() const { return _implementation()->isArray(); }
protected:
    class Implementation : public ReferenceCounted
    {
    public:
        virtual bool equals(const Implementation* other) const = 0;
        virtual int length(int max) const = 0;
        virtual String toString(int width, int spacesPerIndex, int indent, int& x, bool& more) const = 0;
        virtual int hash() const = 0;
        virtual bool isSymbol() const = 0;
        virtual bool isArray() const = 0;
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
        bool isSymbol() const { return false; }
        bool isArray() const { return false; }
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
        bool isSymbol() const { return false; }
        bool isArray() const { return false; }
    private:
        String _value;
    };
    SymbolEntryTemplate(Implementation* implementation) : _implementation(implementation) { }
    const Implementation* implementation() const { return _implementation; }
    Implementation* implementation() { return _implementation; }
private:
    Reference<Implementation> _implementation;
    template<class T> friend class SymbolTemplate;
    template<class T> friend class SymbolArrayTemplate;
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
    SymbolTemplate() { }
    SymbolTemplate(Atom atom, Span span = Span())
      : SymbolEntry(new Implementation(-1, atom, span, 0)) { }
    SymbolTemplate(Atom atom, SymbolEntry symbol1, Span span = Span())
      : SymbolEntry(new Implementation(-1, atom, span, SymbolTail(symbol1, 0))) { }
    SymbolTemplate(Atom atom, SymbolEntry symbol1, SymbolEntry symbol2, Span span = Span())
      : SymbolEntry(new Implementation(-1, atom, span, SymbolTail(symbol1, SymbolEntry(symbol2, 0)))) { }
    SymbolTemplate(Atom atom, SymbolEntry symbol1, SymbolEntry symbol2, SymbolEntry symbol3, Span span = Span())
      : SymbolEntry(new Implementation(-1, atom, span, SymbolTail(symbol1, SymbolTail(symbol2, SymbolTail(symbol3, 0))))) { }
    SymbolTemplate(Atom atom, SymbolEntry symbol1, SymbolEntry symbol2, SymbolEntry symbol3, SymbolEntry symbol4, Span span = Span())
      : SymbolEntry(new Implementation(-1, atom, span, SymbolTail(symbol1, SymbolTail(symbol2, SymbolTail(symbol3, SymbolTail(symbol4, 0)))))) { }
    SymbolTemplate(Atom atom, SymbolEntry symbol1, SymbolEntry symbol2, SymbolEntry symbol3, SymbolEntry symbol4, SymbolEntry symbol5, Span span = Span())
      : SymbolEntry(new Implementation(-1, atom, span, SymbolTail(symbol1, SymbolTail(symbol2, SymbolTail(symbol3, SymbolTail(symbol4, SymbolTail(symbol5, 0))))))) { }

    SymbolTemplate(Atom atom, const SymbolTail* tail, Span span)
      : SymbolEntry(new Implementation(-1, atom, span, tail)) { }

    Atom atom() const { return implementation()->atom(); }
    SymbolEntry operator[](int n) const
    {
        SymbolTail* t = tail();
        while (n > 1) {
            --n;
            t = t->tail();
        }
        return _tail.head();

    }

    const SymbolTail* tail() const { return implementation()->tail(); }

    class Cache : public ReferenceCounted
    {
    public:
        Cache() : _label(-1) { }
        int label() const { return _label; }
        Span span() const { return _span; }
        Symbol type() const { return _type; }
        void setLabel(int label) { _label = label; }
        void setSpan(Span span) { _span = span; }
        void setType(Symbol type) { _type = type; }
    private:
        int _label;
        Span _span;
        Symbol _type;
    };

    Cache* cache() { return implementation()->cache(); }
    int label() const { return implementation()->label(); }
    Span span() const { return implementation()->span(); }

    void setCache(Reference<Cache> cache) { implementation()->setCache(cache); }
    void setLabel(int label) { implementation()->setLabel(label); }
    void setLabelTarget(int label) { implementation()->setLabelTarget(label); }
    void setSpan(Span span) { implementation()->setSpan(label); }
    void setType(Symbol type) { implementation()->setType(type); }

    static int newLabel()
    {
        int l = _labelled.count();
        _labelled.append(0);
        return l;
    }

private:
    class Implementation : public SymbolEntry::Implementation
    {
    public:
        Implementation(int label, Atom atom, Span span, const SymbolTail* tail)
          : _label(label), _atom(atom), _tail(tail)
        {
            setSpan(span);
        }
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
            if (isTarget())
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
            if (isTarget()) {
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

        Cache* cache()
        {
            if (!_cache.valid())
                _cache = new Cache;
            return _cache;
        }
        int label() const { return cache()->label(); }
        Span span() const { return cache()->span(); }
        Symbol type() const { return cache()->type(); }

        void setCache(Reference<Cache> cache) { _cache = cache; }
        void setLabel(int label) { cache()->setLabel(label); }
        void setLabelTarget(int lable) { cache()->setLabel(label); _labelled[label] = this; }
        void setSpan(Span span) { cache()->setSpan(span); }
        void setType(Symbol type) { cache()->setType(type); }

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
        bool isSymbol() const { return true; }
        bool isArray() const { return false; }

    private:
        bool isTarget() const
        { 
            int l = cache()->label();
            return l > 0 && _labelled[l] == this;
        }

        Atom _atom;
        Reference<SymbolTail> _tail;
        Reference<Cache> _cache;
    };

    const Implementation* implementation() const { return dynamic_cast<const Implementation*>(implementation()); }
    Implementation* implementation() { return dynamic_cast<Implementation*>(implementation()); }

    template<class T> friend class SymbolEntryTemplate;
    static GrowableArray<Symbol::Implementation*> _labelled;
};

GrowableArray<Symbol::Implementation*> Symbol::_labelled;

class SymbolList
{
public:
    SymbolList() : _count(0) { }
    void add(Symbol symbol)
    {
        _first = new Implementation(symbol, _first);
        ++_count;
    }
private:
    class Implementation : public ReferenceCounted
    {
    public:
        Implementation(Symbol symbol, Reference<Implementation> next)
          : _symbol(symbol), _next(next) { }
        Symbol symbol() const { return _symbol; }
        Reference<Implementation> next() const { return _next; }
    private:
        Symbol _symbol;
        Reference<Implementation> _next;
    };
    Reference<Implementation> _first;
    int _count;

    void copyTo(Array<Symbol>* symbols)
    {
        symbols->allocate(_count);
        Reference<Implementation> implementation = _first;
        for (int i = _count - 1; i >= 0; --i) {
            (*symbols)[i] = implementation->symbol();
            implementation = implementation->next();
        }
    }

    template<class T> friend class SymbolArrayTemplate;
};

template<class T> class SymbolArrayTemplate : public SymbolEntry
{
public:
    SymbolArrayTemplate() : SymbolEntry(_empty) { }
    SymbolArrayTemplate(Symbol s1) : SymbolEntry(new Implementation(s)) { }
    SymbolArrayTemplate(Symbol s1, Symbol s2) : SymbolEntry(new Implementation(s1, s2)) { }
    SymbolArrayTemplate(SymbolList list) : SymbolEntry(new Implementation(list)) { }
    int count() const { return dynamic_cast<const Implementation*>(implementation())->count(); }
    Symbol operator[](int i) { return (*dynamic_cast<const Implementation*>(implementation()))[i]; }
private:
    class Implementation : public SymbolEntry::Implementation
    {
    public:
        Implementation(SymbolList list) { list.copyTo(&_symbols); }
        Implementation()
        {
            _symbols.allocate(0);
        }
        Implementation(Symbol s0)
        {
            _symbols.allocate(1);
            _symbols[0] = s0;
        }
        Implementation(Symbol s0, Symbol s1)
        {
            _symbols.allocate(2);
            _symbols[0] = s0;
            _symbols[1] = s1;
        }
        bool equals(const SymbolEntry::Implementation* other) const
        {
            const Implementation* o = dynamic_cast<const Implementation*>(other);
            if (o == 0)
                return false;
            int n = _symbols.count();
            if (n != o->_symbols.count())
                return false;
            for (int i = 0; i < n; ++i)
                if (_symbols[i] != o->_symbols[i])
                    return false;
            return true;
        }
        int hash() const
        {
            int h = 0;
            for (int i = 0; i < _symbols.count(); ++i)
                h = h * 67 + _symbols[i].hash() - 113;
            return h;
        }
        int length(int max) const
        {
            int r = 2;
            for (int i = 0; i < _symbols.count(); ++i) {
                if (r > max)
                    break;
                if (i != 0)
                    ++r;
                r += _symbols[i].length(max - r);
            }
            return r;
        }
        int count() const { return _symbols.count(); }
        Symbol operator[](int i) const { return _symbols[i]; }
        String toString(int width, int spacesPerIndent, int indent, int& x, bool& more) const
        {
            ++x;
            int n = _symbols.count();
            more = true;
            if (n == 0) {
                ++x;
                static String s("[]");
                return s;
            }
             
            bool canInlineNext;
            String s = openBracket + _symbols[0].toString(width, spacesPerIndent, indent + 2, x, canInlineNext);
            
            for (int i = 1; i < n; ++i) {
                Symbol symbol = _symbols[i];
                if (canInlineNext && x + 1 + symbol.length(width - x) <= width - 1) {
                    // Fits on the line - inline it
                    s += space;
                    ++x;
                }
                else {
                    // Doesn't fit on the line - put it on its own line.
                    s += newLine + space*(indent + 2);
                    x = indent + 2;
                    more = false;
                }
                s += symbol.toString(width, spacesPerIndent, indent + 2, x, canInlineNext);
            }
            ++x;
            return s + closeBracket;
        }
        bool isSymbol() const { return false; }
        bool isArray() const { return true; }
    private:
        Array<Symbol> _symbols;
    };
    static Reference<Implementation> _empty;
};

Reference<SymbolArray::Implementation> SymbolArray::_empty = new SymbolArray::Implementation();
