#ifndef INCLUDED_SYMBOL_H
#define INCLUDED_SYMBOL_H

#include <stdint.h>
#include "unity/linked_list.h"

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

template<class T> class SymbolLabelTemplate;
typedef SymbolLabelTemplate<void> SymbolLabel;

template<class T> class SymbolEntryTemplate
{
public:
    SymbolEntryTemplate() { }
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
    SymbolArrayTemplate<T> array() { return SymbolArrayTemplate<T>(_implementation); }
    SymbolTemplate<T> symbol() { return SymbolTemplate<T>(dynamic_cast<Symbol::Implementation*>(implementation())); }
    SymbolLabelTemplate<T> label() { return SymbolLabelTemplate<T>(dynamic_cast<Symbol::Implementation*>(implementation())); }
    Atom atom() const { return symbol().atom(); }
    bool valid() const { return _implementation.valid(); }
    int length(int max) const { return implementation()->length(max); }
    String toString(int width, int spacesPerIndex, int indent, int& x, bool& more) const { return _implementation->toString(width, spacesPerIndex, indent, x, more); }
    int hash() const { return implementation()->hash(); }
    bool isSymbol() const { return _implementation->isSymbol(); }
    bool isArray() const { return _implementation->isArray(); }
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
    SymbolEntry& head() { return _head; }
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
    int length(int max) const
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

class SymbolCache : public ReferenceCounted
{
};

template<class T> class SymbolTemplate : public SymbolEntryTemplate<T>
{
public:
    SymbolTemplate() { }
    SymbolTemplate(Atom atom, SymbolCache* cache = 0)
      : SymbolEntry(new Implementation(atom, cache, 0)) { }
    SymbolTemplate(Atom atom, SymbolEntry symbol1, SymbolCache* cache = 0)
      : SymbolEntry(new Implementation(atom, cache, new SymbolTail(symbol1, 0))) { }
    SymbolTemplate(Atom atom, SymbolEntry symbol1, SymbolEntry symbol2, SymbolCache* cache = 0)
      : SymbolEntry(new Implementation(atom, cache, new SymbolTail(symbol1, new SymbolTail(symbol2, 0)))) { }
    SymbolTemplate(Atom atom, SymbolEntry symbol1, SymbolEntry symbol2, SymbolEntry symbol3, SymbolCache* cache = 0)
      : SymbolEntry(new Implementation(atom, cache, new SymbolTail(symbol1, new SymbolTail(symbol2, new SymbolTail(symbol3, 0))))) { }
    SymbolTemplate(Atom atom, SymbolEntry symbol1, SymbolEntry symbol2, SymbolEntry symbol3, SymbolEntry symbol4, SymbolCache* cache = 0)
      : SymbolEntry(new Implementation(atom, cache, new SymbolTail(symbol1, new SymbolTail(symbol2, new SymbolTail(symbol3, new SymbolTail(symbol4, 0)))))) { }
    SymbolTemplate(Atom atom, SymbolEntry symbol1, SymbolEntry symbol2, SymbolEntry symbol3, SymbolEntry symbol4, SymbolEntry symbol5, SymbolCache* cache = 0)
      : SymbolEntry(new Implementation(atom, cache, new SymbolTail(symbol1, new SymbolTail(symbol2, new SymbolTail(symbol3, new SymbolTail(symbol4, new SymbolTail(symbol5, 0))))))) { }

    SymbolTemplate(Atom atom, const SymbolTail* tail, SymbolCache* cache)
      : SymbolEntry(new Implementation(atom, cache, tail)) { }

    Atom atom() const { return implementation()->atom(); }
    SymbolEntry operator[](int n) const
    {
        const SymbolTail* t = tail();
        while (n > 1) {
            if (t == 0)
                return Symbol();
            --n;
            t = t->tail();
        }
        return t->head();
    }

    SymbolEntry& operator[](int n)
    {
        SymbolTail* t = tail();
        while (n > 1) {
            if (t == 0)
                return Symbol();
            --n;
            t = t->tail();
        }
        return t->head();
    }

    const SymbolTail* tail() const { return implementation()->tail(); }

    template<class U> U* cache()
    {
        return implementation()->cache()->cast<U>();
    }
private:
    SymbolTemplate(Implementation* implementation) : SymbolEntry(implementation) { }

    class Implementation : public SymbolEntry::Implementation
    {
    public:
        Implementation(Atom atom, SymbolCache* cache, const SymbolTail* tail)
          : _atom(atom), _cache(cache), _tail(tail), _labelReferences(0),
          _labelNumber(-1)
        { }
        bool equals(const SymbolEntry::Implementation* other) const
        {
            const Implementation* o = dynamic_cast<const Implementation*>(other);
            if (o == 0)
                return false;
            return _atom == o->_atom && _tail->equals(o->_tail);
        }
        int length(int max) const
        {
            int r = 2 + atomToString(_atom).length();
            if (_labelReferences > 0)
                r += 1 + decimalLength(label());
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
            if (_labelReferences > 0) {
                s = String::decimal(label()) + colon + s;
                x += 1 + decimalLength(label());
            }
            String a = atomToString(_atom);
            x += a.length();
            s += a;
            bool canInlineNext = true;
            more = true;

            const SymbolTail* tail = _tail;
            while (tail != 0) {
                SymbolEntry entry = tail->head();
                if (canInlineNext && x + 1 + entry.length(width - x) <= width - 1) {
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
                s += entry.toString(width, spacesPerIndent, indent + 2, x, canInlineNext);
                tail = tail->tail();
            }
            ++x;
            return s + closeParenthesis;
        }

        Atom atom() const { return _atom; }

        SymbolCache* cache() { return _cache; }
        const SymbolTail* tail() const { return _tail; }

        void setCache(Reference<ReferenceCounted> cache) { _cache = cache; }

        int hash() const
        {
            int h = atom();
            const SymbolTail* t = _tail;
            while (t != 0) {
                h = h * 67 + t->head().hash() - 113;
                t = t->tail();
            }
            return h;
        }
        bool isSymbol() const { return true; }
        bool isArray() const { return false; }

        int label() const
        {
            if (_labelNumber == -1) {
                _labelNumber = _labels;
                ++_labels;
            }
            return _labelNumber;
        }
        int addLabel() { ++_labelReferences; }
        int removeLabel() { --_labelReferences; }
    private:
        Atom _atom;
        ConstReference<SymbolTail> _tail;
        Reference<SymbolCache> _cache;
        mutable int _labelNumber;
        int _labelReferences;

        static int _labels;
    };

    const Implementation* implementation() const { return dynamic_cast<const Implementation*>(SymbolEntryTemplate::implementation()); }
    Implementation* implementation() { return dynamic_cast<Implementation*>(SymbolEntryTemplate::implementation()); }

    template<class T> friend class SymbolEntryTemplate;
    template<class T> friend class SymbolTemplate;
    template<class T> friend class SymbolArrayTemplate;
    template<class T> friend class SymbolLabelTemplate;
};

int Symbol::Implementation::_labels = 0;

class SymbolList
{
public:
    SymbolList() : _count(0) { }
    void add(Symbol symbol)
    {
        _first = new Implementation(symbol, _first);
        if (_count == 0)
            _last = _first;
        ++_count;
    }
    void add(SymbolList list)
    {
        _first = list._last;
        _count += list._count;
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
    Reference<Implementation> _last;
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
    SymbolArrayTemplate(Symbol s1) : SymbolEntry(new Implementation(s1)) { }
    SymbolArrayTemplate(Symbol s1, Symbol s2) : SymbolEntry(new Implementation(s1, s2)) { }
    SymbolArrayTemplate(SymbolList list) : SymbolEntry(new Implementation(list)) { }
    int count() const { return dynamic_cast<const Implementation*>(implementation())->count(); }
    Symbol operator[](int i) { return (*dynamic_cast<const Implementation*>(implementation()))[i]; }
private:
    SymbolArrayTemplate(Implementation* implementation) : SymbolEntry(implementation) { }

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
        Symbol& operator[](int i) { return _symbols[i]; }
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

    template<class T> friend class SymbolEntryTemplate;
};

Reference<SymbolArray::Implementation> SymbolArray::_empty =
    new SymbolArray::Implementation();

template<class T> class SymbolLabelTemplate : public SymbolEntry
{
public:
    SymbolLabelTemplate() { }
    SymbolLabelTemplate(Symbol target)
      : _implementation(new Implementation(target._implementation)) { }
    Symbol target() { return Symbol(_implementation->target()); }
    void setTarget(Symbol target)
    {
        _implementation->setTarget(target._implementation);
    }
private:
    class Implementation : public SymbolEntry::Implementation
    {
    public:
        Implementation(Symbol::Implementation* target) : _target(target)
        {
            _target->addLabel();
        }
        ~Implementation() { _target->removeLabel(); }
        Symbol::Implementation* target() { return _target; }
        void setTarget(Symbol::Implementation* target)
        {
            _target->removeLabel();
            _target = target;
            _target->addLabel();
        }
        bool equals(const Symbol::Implementation* other) const
        {
            const Implementation* o =
                dynamic_cast<const Implementation*>(other);
            if (o == 0)
                return false;
            return _target == o->_target;
        }
        int length(int max) const
        {
            return 2 + decimalLength(_target->label());
        }
        String toString(int width, int spacesPerIndex, int indent, int& x,
            bool& more) const
        {
            x += length();
            more = true;
            return lessThan + String::decimal(_target->label()) + greaterThan;
        }
        int hash() const { return reinterpret_cast<int>(_target); }
        bool isSymbol() const { return false; }
        bool isArray() const { return false; }
    private:
        Symbol::Implementation* _target;
    };
};

#endif // INCLUDED_SYMBOL_H
