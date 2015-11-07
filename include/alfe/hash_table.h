#include "alfe/main.h"

#ifndef INCLUDED_HASH_TABLE_H
#define INCLUDED_HASH_TABLE_H

#include "alfe/tuple.h"

// HashTable is not quite a value type, since changing an element in one table
// will affect copies of the same table. Adding an element may cause it to
// become a deep copy, if more storage space was needed.
//
// Note that the default-constructed Key should not be used for real entries.

template<class Key, class Value> class HashTable
  : private AppendableArray<Tuple<Key, Value>>
{
    typedef Tuple<Key, Value> Entry;
public:
    bool hasKey(const Key& key) const
    {
        auto e = lookup(key);
        if (e != 0)
            return e->first() == key;
        return false;
    }
    Value& operator[](const Key& key)
    {
        auto e = lookup(key);
        if (e != 0 && e->first() == key)
            return e->second();
        if (count() >= allocated()*3/4) {
            int n = allocated()*2;
            if (n == 0)
                n = 1;
            HashTable other;
            other.allocate(n);
            n = other.allocated();
            other.expand(n);
            other.body()->_size = count();
            for (auto i = begin(); i != end(); ++i)
                other[i.key()] = i.value();
            *this = other;
            e = lookup(key);
        }
        e->first() = key;
        return e->second();
    }
    Value operator[](const Key& key) const
    {
        auto e = lookup(key);
        if (e != 0)
            return e->second();
        return Value();
    }
    void add(const Key& key, const Value& value) { (*this)[key] = value; }
    ~HashTable() { if (body() != 0) body()->_size = allocated(); }
    int count() const { return body() == 0 ? 0 : body()->_size; }
    class Iterator
    {
    public:
        const Key& key() { return _entry->first(); }
        const Value& value() { return _entry->second(); }
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
            do {
                ++_entry;
                if ((*this) == _table.end())
                    return;
            } while (_entry->first() == Key());
        }
    private:
        Iterator(const Entry* entry, const HashTable& table)
          : _entry(entry), _table(table) { }
        const Entry* _entry;
        const HashTable _table;

        friend class HashTable;
    };
    Iterator begin() const
    {
        if (allocated() == 0)
            return Iterator(0, *this);
        Iterator i(data(0), *this);
        if (i.key() == Key())
            ++i;
        return i;
    }
    Iterator end() const
    {
        if (allocated() == 0)
            return Iterator(0, *this);
        return Iterator(data(allocated()), *this);
    }
private:
    int row(const Key& key) const { return ::hash(key) % allocated(); }
    Entry* lookup(const Key& key)
    {
        if (allocated() == 0)
            return 0;
        int r = row(key);
        for (int i = 0; i < allocated(); ++i) {
            // We have a decent hash function so linear probing should work
            // fine.
            r = (r + 1)%allocated();
            Entry* e = data(r);
            if (e->first() == key || e->first() == Key())
                return e;
        }
        return 0;
    }
    const Entry* lookup(const Key& key) const
    {
        if (allocated() == 0)
            return 0;
        int r = row(key);
        for (int i = 0; i < allocated(); ++i) {
            r = (r + 1)%allocated();
            const Entry* e = data(r);
            if (e->first() == key || e->first() == Key())
                return e;
        }
        return 0;
    }
    Entry* data(int row) { return &static_cast<AppendableArray&>(*this)[row]; }
    const Entry* data(int row) const
    {
        return &static_cast<const AppendableArray&>(*this)[row];
    }
};

#endif // INCLUDED_HASH_TABLE_H
