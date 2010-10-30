#ifndef INCLUDED_HASHTABLE_H
#define INCLUDED_HASHTABLE_H

#include "unity/array.h"

template<class Key, class Value> class HashTable
{
public:
    HashTable() : _n(0)
    {
        _table->allocate(1);
    }
    void add(const Key& key, const Value& value)
    {
        if (_n == _table.count()) {
            Array<TableEntry> table;
            table->allocate(_table.count() * 2);
            table.swap(_table);
            for (int i = 0; i < table.count(); ++i)
                table[i].addAllTo(this);
        }
        _table[row(key)].add(key, value);
        ++_n;
    }
    Value lookUp(const Key& key)
    {
        return _table[row(key)].value(key);
    }
private:
    int row(const Key& key)( { return key.hash() & (_table.count() - 1); }
    class TableEntry
    {
    public:
        TableEntry() : _next(0) { }
        ~TableEntry()
        {
            while (_next != 0) {
                TableEntry* t = _next->_next;
                delete _next;
                _next = t;
            }
        }
        Value value(const Key& key)
        {
            TableEntry* t = this;
            do {
                if (t->_key == key)
                    return t->_value;
                t = t->_next;
            } while (t != 0);
            return Value();
        }
        void add(const Key& key, const Value& value)
        {
            if (_value == Value()) {
                _key = key;
                _value = value;
                return;
            }
            TableEntry* t = this;
            while (t->_next != 0)
                t = t->_next;
            t->_next = new TableEntry();
            t->_next->_key = key;
            t->_next->_value = value;
        }
        void addAllTo(HashTable* table)
        {
            if (_value == Value())
                return;
            table->add(_key, _value);
            TableEntry* t = _next;
            while (t != 0) {
                table->add(t->_key, t->_value);
                t = t->_next;
            }
        }
    private:
        Key _key;
        Value _value;
        TableEntry* _next;
    };
    Array<TableEntry> _table;
    int _n;
};

#endif // INCLUDED_HASHTABLE_H
