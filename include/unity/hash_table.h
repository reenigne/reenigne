#ifndef INCLUDED_HASH_TABLE_H
#define INCLUDED_HASH_TABLE_H

template<class Key, class Value, class Base> class HashTableBase : public Base
{
public:
    HashTableBase() : _n(0)
    {
        _table.allocate(1);
        _table.constructElements();
    }
    bool hasKey(const Key& key)
    {
        return _table[row(key)].hasKey(key);
    }
    Value& operator[](const Key& key)
    {
        return _table[row(key)].value(key);
    }
    const Value& operator[](const Key& key) const
    {
        return _table[row(key)].value(key);
    }
    void add(const Key& key, const Value& value)
    {
        if (_n == _table.count()) {
            Array<TableEntry> table;
            table.allocate(_table.count() * 2);
            table.constructElements();
            table.swap(_table);
            _n = 0;
            for (int i = 0; i < table.count(); ++i)
                table[i].addAllTo(this);
        }
        _table[row(key)].add(key, value);
        ++_n;
    }
    int count() const { return _n; }
    class Iterator
    {
    public:
        const Key& key() { return _entry->_key; }
        const Value& value() { return _entry->_value; }
        bool operator==(const Iterator& other) const
        {
            return _entry == other._entry;
        }
        bool operator!=(const Iterator& other) const
        {
            return !operator==(other);
        }
        void operator++()
        {
            _entry = _entry->_next;
            while (_entry == 0) {
                ++_row;
                if (_row == _table->_n)
                    break;
                _entry = _table->_table[_row];
            }
        }
    private:
        Iterator(int row, const TableEntry* entry, const HashTable* table)
          : _row(row), _entry(entry), _table(table) { }
        int _row;
        const TableEntry* _entry;
        const HashTable* _table;

        friend class HashTableBase;
    };
    Iterator begin() const
    {
        int row = 0;
        const TableEntry* entry = &_table[0];
        while (entry == 0) {
            ++row;
            if (row == _n)
                break;
            _entry = _table[row];
        }
        Iterator t(row, entry, this);
    }
    Iterator end() const { return Iterator(_n, 0); }
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
        bool hasKey(const Key& key) const { return findEntry(key) != 0; }
        Value& value(const Key& key)
        { 
            const TableEntry* t = findEntry(key);
            if (t == 0)
                return doAdd(key)->_value;
            return t->_value;
        }
        const Value& value(const Key& key) const
        {
            const TableEntry* t = findEntry(key);
            if (t == 0)
                return Value();
            return t->_value;
        }
        void add(const Key& key, const Value& value)
        {
            doAdd(key)->_value = value;
        }
        void addAllTo(HashTableBase* table)
        {
            if (_next == 0)
                return;
            TableEntry* t = this;
            do {
                table->add(t->_key, t->_value);
                t = t->_next;
            } while (t != this);
        }
    private:
        const TableEntry* findEntry(const Key& key) const
        {
            if (_next == 0)
                return 0;
            const TableEntry* t = this;
            do {
                if (t->_key == key)
                    return t;
                t = t->_next;
            } while (t != this);
            return 0;
        }
        const TableEntry* doAdd(const Key& key)
        {
            if (_next == 0) {
                _key = key;
                _next = this;
                return this;
            }
            TableEntry* t = this;
            while (t->_next != this)
                t = t->_next;
            t->_next = new TableEntry();
            t = t->_next;
            t->_key = key;
            t->_next = this;
            return t;
        }
        Key _key;
        Value _value;
        TableEntry* _next;
    };
    Array<TableEntry> _table;
    int _n;
};

template<class Key, class Value> class HashTableRow : Uncopyable
{
protected:
    int hash(const Key& key) const { return key.hash(); }
};

template<class Value> class HashTableRow<int, Value> : Uncopyable
{
protected:
    int hash(int key) const { return key; }
};

template<class Key, class Value> class HashTable
    : public HashTableBase<Key, Value, HashTableRow<Key, Value> >
{
};

#endif // INCLUDED_HASH_TABLE_H
