#include "alfe/main.h"

#ifndef INCLUDED_SYMBOL_H
#define INCLUDED_SYMBOL_H

#include <stdint.h>
#include "alfe/linked_list.h"

String quote(String string)
{
    CharacterSource s(string);
    String r("\"");
    int start = 0;
    int end;
    do {
        int c = s.get();
        if (c == -1) {
            end = s.offset();
            r += s.subString(start, end);
            return r + "\"";
        }
        if (c == '"' || c == '\\') {
            end = s.offset();
            r += s.subString(start, end) + "\\";
            start = end;
        }
    } while (true);
}

int quotedLength(String string)
{
    CharacterSource s(string);
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

template<class T> class SymbolEntryT;
typedef SymbolEntryT<void> SymbolEntry;

template<class T> class SymbolT;
typedef SymbolT<void> Symbol;

template<class T> class SymbolArrayT;
typedef SymbolArrayT<void> SymbolArray;

template<class T> class SymbolLabelT;
typedef SymbolLabelT<void> SymbolLabel;

template<class T> class SymbolEntryT : public Handle
{
public:
    SymbolEntryT() { }
    SymbolEntryT(int value) : Handle(new IntegerBody(value)) { }
    SymbolEntryT(String value) : Handle(new StringBody(value)) { }
    int integer() const
    {
        return as<IntegerBody>()->value();
    }
    String string() const
    {
        return as<StringBody>()->value();
    }
    SymbolArrayT<T> array()
    {
        return SymbolArrayT<T>(body());
    }
    SymbolT<T> symbol()
    {
        return SymbolT<T>(dynamic_cast<Symbol::Body*>(body()));
    }
    SymbolLabelT<T> label()
    {
        return SymbolLabelT<T>(dynamic_cast<Symbol::Body*>(body()));
    }
    Atom atom() { return symbol().atom(); }
    bool valid() const { return _body.valid(); }
    int length(int max) const { return body()->length(max); }
    String toString(int width, int spacesPerIndent, int indent, int& x,
        bool& more) const
    {
        return body()->toString(width, spacesPerIndent, indent, x, more);
    }
    String toString() const
    {
        int x;
        bool more;
        return toString(80, 2, 0, x, more);
    }
    bool isSymbol() const { return body()->isSymbol(); }
    bool isArray() const { return body()->isArray(); }
protected:
    class Body : public Handle::Body
    {
    public:
        virtual int length(int max) const = 0;
        virtual String toString(int width, int spacesPerIndent, int indent,
            int& x, bool& more) const = 0;
        virtual bool isSymbol() const = 0;
        virtual bool isArray() const = 0;
    };
    class IntegerBody : public Body
    {
    public:
        IntegerBody(int value) : _value(value) { }
        bool equals(const ConstHandle::Body* other) const
        {
            auto o = other->as<IntegerBody>();
            return o != 0 && _value == o->_value;
        }
        String toString(int width, int spacesPerIndent, int indent, int& x,
            bool& more) const
        {
            x += decimalLength(_value);
            more = true;
            return decimal(_value);
        }
        int length(int max) const { return decimalLength(_value); }
        int value() const { return _value; }
        Hash hash() const { return Body::hash().mixin(_value); }
        bool isSymbol() const { return false; }
        bool isArray() const { return false; }
    private:
        int _value;
    };
    class StringBody : public Body
    {
    public:
        StringBody(String value) : _value(value) { }
        bool equals(const ConstHandle::Body* other) const
        {
            auto o = other->as<StringBody>();
            return o != 0 && _value == o->_value;
        }
        String toString(int width, int spacesPerIndent, int indent, int& x,
            bool& more) const
        {
            x += quotedLength(_value);
            more = true;
            return quote(_value);
        }
        int length(int max) const { return quotedLength(_value); }
        String value() const { return _value; }
        Hash hash() const { return Body::hash().mixin(_value.hash()); }
        bool isSymbol() const { return false; }
        bool isArray() const { return false; }
    private:
        String _value;
    };
    SymbolEntryT(Body* body) : Handle(body) { }
    const Body* body() const { return Handle::Body<Body>(); }
    Body* body() { return Handle::Body<Body>(); }
private:
    template<class T> friend class SymbolT;
    template<class T> friend class SymbolArrayT;
};

class SymbolTail : public Handle::Body
{
public:
    SymbolTail(SymbolEntry head) : _head(head) { }
    SymbolTail(SymbolEntry head, SymbolTail* tail) : _head(head), _tail(tail)
    { }
    SymbolEntry head() const { return _head; }
    SymbolEntry& head() { return _head; }
    const SymbolTail* tail() const { return _tail; }
    SymbolTail* tail() { return _tail; }
    bool equals(const ConstHandle::Body* other) const
    {
        auto o = other->as<SymbolTail>();
        return o != 0 && _head == o->_head && _tail == o->_tail;
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

template<class T> class SymbolT : public SymbolEntryT<T>
{
public:
    SymbolT() { }
    SymbolT(Atom atom, SymbolCache* cache = 0)
      : SymbolEntry(new Body(atom, cache, 0)) { }
    SymbolT(Atom atom, SymbolEntry symbol1, SymbolCache* cache = 0)
      : SymbolEntry(
        new Body(atom, cache, new SymbolTail(symbol1, 0))) { }
    SymbolT(Atom atom, SymbolEntry symbol1, SymbolEntry symbol2,
        SymbolCache* cache = 0)
      : SymbolEntry(new Body(atom, cache, new SymbolTail(symbol1,
          new SymbolTail(symbol2, 0)))) { }
    SymbolT(Atom atom, SymbolEntry symbol1, SymbolEntry symbol2,
        SymbolEntry symbol3, SymbolCache* cache = 0)
      : SymbolEntry(new Body(atom, cache, new SymbolTail(symbol1,
          new SymbolTail(symbol2, new SymbolTail(symbol3, 0))))) { }
    SymbolT(Atom atom, SymbolEntry symbol1, SymbolEntry symbol2,
        SymbolEntry symbol3, SymbolEntry symbol4, SymbolCache* cache = 0)
      : SymbolEntry(new Body(atom, cache, new SymbolTail(symbol1,
          new SymbolTail(symbol2, new SymbolTail(symbol3,
          new SymbolTail(symbol4, 0)))))) { }
    SymbolT(Atom atom, SymbolEntry symbol1, SymbolEntry symbol2,
        SymbolEntry symbol3, SymbolEntry symbol4, SymbolEntry symbol5,
        SymbolCache* cache = 0)
      : SymbolEntry(new Body(atom, cache, new SymbolTail(symbol1,
          new SymbolTail(symbol2, new SymbolTail(symbol3,
          new SymbolTail(symbol4, new SymbolTail(symbol5, 0))))))) { }

    SymbolT(Atom atom, const SymbolTail* tail, SymbolCache* cache)
      : SymbolEntry(new Body(atom, cache, tail)) { }

    Atom atom() const { return body()->atom(); }
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
                throw Exception(String("Out of bounds access in to Symbol"));
            --n;
            t = t->tail();
        }
        return t->head();
    }

    const SymbolTail* tail() const { return body()->tail(); }
    SymbolTail* tail() { return body()->tail(); }

    template<class U> U* cache()
    {
        return body()->cache()->cast<U>();
    }
private:
    SymbolT(Body* body)
      : SymbolEntry(body) { }

    class Body : public SymbolEntry::Body
    {
    public:
        Body(Atom atom, SymbolCache* cache, SymbolTail* tail)
          : _atom(atom), _cache(cache), _tail(tail), _labelReferences(0),
          _labelNumber(-1)
        { }
        bool equals(const ConstHandle::Body* other) const
        {
            auto o = other->as<Body>();
            return o != 0 && _atom == o->_atom && _tail == o->_tail;
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

        String toString(int width, int spacesPerIndent, int indent, int& x,
            bool& more) const
        {
            ++x;
            more = true;
            String s("(");
            if (_labelReferences > 0) {
                s = decimal(label()) + ":" + s;
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
                if (canInlineNext && x + 1 + entry.length(width - x) <=
                    width - 1) {
                    // Fits on the line - inline it
                    s += " ";
                    ++x;
                }
                else {
                    // Doesn't fit on the line - put it on its own line.
                    s += "\n" + String(" ")*(indent + 2);
                    x = indent + 2;
                    more = false;
                }
                s += entry.toString(width, spacesPerIndent, indent + 2, x,
                    canInlineNext);
                tail = tail->tail();
            }
            ++x;
            return s + ")";
        }

        Atom atom() const { return _atom; }

        SymbolCache* cache() { return _cache; }
        const SymbolTail* tail() const { return _tail; }
        SymbolTail* tail() { return _tail; }

        void setCache(Reference<ReferenceCounted> cache) { _cache = cache; }

        Hash hash() const
        {
            Hash h = SymbolEntry::Body::hash().mixin(atom());
            const SymbolTail* t = _tail;
            while (t != 0) {
                h.mixin(t->head().hash());
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
        Reference<SymbolTail> _tail;
        Reference<SymbolCache> _cache;
        mutable int _labelNumber;
        int _labelReferences;

        static int _labels;
    };

    const Body* body() const
    {
        return dynamic_cast<const Body*>(
            SymbolEntryT::body());
    }
    Body* body()
    {
        return dynamic_cast<Body*>(
            SymbolEntryT::body());
    }

    template<class T> friend class SymbolEntryT;
    template<class T> friend class SymbolT;
    template<class T> friend class SymbolArrayT;
    template<class T> friend class SymbolLabelT;
};

int Symbol::Body::_labels = 0;

class SymbolList
{
public:
    SymbolList() : _count(0) { }
    void add(Symbol symbol)
    {
        _first = new Body(symbol, _first);
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
    class Body : public ReferenceCounted
    {
    public:
        Body(Symbol symbol, Reference<Body> next)
          : _symbol(symbol), _next(next) { }
        Symbol symbol() const { return _symbol; }
        Reference<Body> next() const { return _next; }
    private:
        Symbol _symbol;
        Reference<Body> _next;
    };
    Reference<Body> _first;
    Reference<Body> _last;
    int _count;

    void copyTo(Array<Symbol>* symbols)
    {
        symbols->allocate(_count);
        Reference<Body> body = _first;
        for (int i = _count - 1; i >= 0; --i) {
            (*symbols)[i] = body->symbol();
            body = body->next();
        }
    }

    template<class T> friend class SymbolArrayT;
};

template<class T> class SymbolArrayT : public SymbolEntry
{
public:
    SymbolArrayT() : SymbolEntry(_empty) { }
    SymbolArrayT(Symbol s1) : SymbolEntry(new Body(s1)) { }
    SymbolArrayT(Symbol s1, Symbol s2)
      : SymbolEntry(new Body(s1, s2)) { }
    SymbolArrayT(SymbolList list)
      : SymbolEntry(new Body(list)) { }
    int count() const
    {
        return dynamic_cast<const Body*>(body())->count();
    }
    Symbol operator[](int i)
    {
        return (*dynamic_cast<const Body*>(body()))[i];
    }
private:
    SymbolArrayT(Body* body)
      : SymbolEntry(body) { }

    class Body : public SymbolEntry::Body
    {
    public:
        Body(SymbolList list) { list.copyTo(&_symbols); }
        Body()
        {
            _symbols.allocate(0);
        }
        Body(Symbol s0)
        {
            _symbols.allocate(1);
            _symbols[0] = s0;
        }
        Body(Symbol s0, Symbol s1)
        {
            _symbols.allocate(2);
            _symbols[0] = s0;
            _symbols[1] = s1;
        }
        bool equals(const ConstHandle::Body* other) const
        {
            auto o = other->as<Body>();
            return o != 0 && _symbols == o->_symbols;
        }
        Hash hash() const
        {
            Hash h = SymbolEntry::Body::hash();
            for (int i = 0; i < _symbols.count(); ++i)
                h.mixin(_symbols[i].hash());
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
        String toString(int width, int spacesPerIndent, int indent, int& x,
            bool& more) const
        {
            ++x;
            int n = _symbols.count();
            more = true;
            if (n == 0) {
                ++x;
                return "[]";
            }

            bool canInlineNext;
            String s = "[" + _symbols[0].toString(width,
                spacesPerIndent, indent + 2, x, canInlineNext);

            for (int i = 1; i < n; ++i) {
                Symbol symbol = _symbols[i];
                if (canInlineNext && x + 1 + symbol.length(width - x) <=
                    width - 1) {
                    // Fits on the line - inline it
                    s += " ";
                    ++x;
                }
                else {
                    // Doesn't fit on the line - put it on its own line.
                    s += "\n" + String(" ")*(indent + 2);
                    x = indent + 2;
                    more = false;
                }
                s += symbol.toString(width, spacesPerIndent, indent + 2, x,
                    canInlineNext);
            }
            ++x;
            return s + "]";
        }
        bool isSymbol() const { return false; }
        bool isArray() const { return true; }
    private:
        Array<Symbol> _symbols;
    };
    static Reference<Body> _empty;

    template<class T> friend class SymbolEntryT;
};

Reference<SymbolArray::Body> SymbolArray::_empty =
    new SymbolArray::Body();

template<class T> class SymbolLabelT : public SymbolEntry
{
public:
    SymbolLabelT() { }
    SymbolLabelT(Symbol target) : _body(new Body(target._body)) { }
    Symbol target() { return Symbol(_body->target()); }
    void setTarget(Symbol target)
    {
        _body->setTarget(target._body);
    }
private:
    class Body : public SymbolEntry::Body
    {
    public:
        Body(Symbol::Body* target) : _target(target)
        {
            _target->addLabel();
        }
        ~Body() { _target->removeLabel(); }
        Symbol::Body* target() { return _target; }
        void setTarget(Symbol::Body* target)
        {
            _target->removeLabel();
            _target = target;
            _target->addLabel();
        }
        bool equals(const ConstHandle::Body* other) const
        {
            auto o = other->as<Body>();
            return o != 0 && _target == o->_target;
        }
        int length(int max) const
        {
            return 2 + decimalLength(_target->label());
        }
        String toString(int width, int spacesPerIndent, int indent, int& x,
            bool& more) const
        {
            x += length();
            more = true;
            return "<" + decimal(_target->label()) + ">";
        }
        Hash hash() const
        {
            return SymbolEntry::Body::hash().
                mixin(reinterpret_cast<int>(_target));
        }
        bool isSymbol() const { return false; }
        bool isArray() const { return false; }
    private:
        Symbol::Body* _target;
    };
};

#endif // INCLUDED_SYMBOL_H
