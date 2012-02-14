#ifndef INCLUDED_SET_H
#define INCLUDED_SET_H

#include "unity/array.h"

template<class Key, class Base> class SetBase : public Base
{
public:
    SetBase() : _n(0)
    {
        _table.allocate(1);
    }
    bool has(const Key& key)
    {
        return _table[row(key)].has(key);
    }
    void add(const Key& key)
    {
        if (_n == _table.count()) {
            Array<TableEntry> table;
            table.allocate(_table.count() * 2);
            table.swap(_table);
            _n = 0;
            for (int i = 0; i < table.count(); ++i)
                table[i].addAllTo(this);
        }
        _table[row(key)].add(key);
        ++_n;
    }
    int count() const { return _n; }
private:
    int row(const Key& key) const { return hash(key) & (_table.count() - 1); }

    class TableEntry
    {
    public:
        TableEntry() : _next(0) { }
        ~TableEntry()
        {
            while (_next != 0 && _next != this) {
                TableEntry* t = _next->_next;
                _next->_next = 0;
                delete _next;
                _next = t;
            }
        }
        bool has(const Key& key)
        {
            if (_next == 0)
                return false;
            TableEntry* t = this;
            do {
                if (t->_key == key)
                    return true;
                t = t->_next;
            } while (t != this);
            return false;
        }
        void add(const Key& key)
        {
            if (_next == 0) {
                _key = key;
                _next = this;
                return;
            }
            TableEntry* t = this;
            while (t->_next != this)
                t = t->_next;
            t->_next = new TableEntry();
            t->_next->_key = key;
            t->_next->_next = this;
        }
        void addAllTo(SetBase* table)
        {
            if (_next == 0)
                return;
            TableEntry* t = this;
            do {
                table->add(t->_key);
                t = t->_next;
            } while (t != this);
        }
    private:
        Key _key;
        TableEntry* _next;
    };
    Array<TableEntry> _table;
    int _n;
};

template<class Key> class SetRow : Uncopyable
{
protected:
    int hash(const Key& key) const { return key.hash(); }
};

template<> class SetRow<int> : Uncopyable
{
protected:
    int hash(int key) const { return key; }
};

template<class Key> class Set
    : public SetBase<Key, SetRow<Key> >
{
};

#endif // INCLUDED_SET_H
