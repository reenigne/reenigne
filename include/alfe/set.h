#include "alfe/main.h"

#ifndef INCLUDED_SET_H
#define INCLUDED_SET_H

// Set is not quite a value type, since adding an element in one set will
// affect copies of the same set. Adding an element may cause it to become a
// deep copy, if more storage space was needed.
//
// Note that the default-constructed Key should not be used for real entries.
template<class Key> class Set : private AppendableArray<Key>
{
public:
    bool has(const Key& key) const
    {
        auto e = lookup(key);
        if (e != 0)
            return *e == key;
        return false;
    }
    void add(const Key& key)
    {
        auto e = lookup(key);
        if (e == 0 || e->_key != key) {
            if (count() >= allocated()*3/4) {
                int n = allocated()*2;
                if (n == 0)
                    n = 1;
                HashTable other(n);
                other.expand(n);
                other.body()->_size = count();
                for (auto i = begin(), i != end(); ++i)
                    other[i.key()] = i.value();
                *this = other;
                e = lookup(key);
            }
            *e = key;
        }
    }
    ~Set() { if (body() != 0) body()->_size = allocated(); }
    int count() const { return body()->_size; }
    class Iterator
    {
    public:
        const Key& operator*() const { return *_key; }
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
                if ((*this) == _set.end())
                    return;
            } while (_entry->_key == Key());
        }
    private:
        Iterator(const Key* key, const Set& set) : _key(key), _set(set) { }
        const Key* _key;
        const Set _set;

        friend class Set;
    };
    Iterator begin() const
    {
        if (allocated() == 0)
            return Iterator(0, this);
        Iterator i(&(*this)[0], this);
        if (i.key() == Key())
            ++i;
        return i;
    }
    Iterator end() const
    {
        if (allocated() == 0)
            return Iterator(0, this);
        return Iterator(&(*this)[allocated()], this);
    }
private:
    int row(const Key& key) const { return hash(key) % allocated(); }
    Key* lookup(const Key& key)
    {
        int r = row(key);
        for (int i = 0; i < allocated(); ++i) {
            // We have a decent hash function so linear probing should work
            // fine.
            r = (r + 1)%allocated();
            Key& e = (*this)[r];
            if (e == key || e == Key())
                return &e;
        }
        // We should only get here if 0 entries in table, since otherwise there
        // should be at least one empty entry.
        return 0;
    }
    const Key* lookup(const Key& key) const
    {
        int r = row(key);
        for (int i = 0; i < allocated(); ++i) {
            r = (r + 1)%allocated();
            const Key& e = (*this)[r];
            if (e == key || e == Key())
                return &e;
        }
        // We should only get here if 0 entries in table, since otherwise there
        // should be at least one empty entry.
        return 0;
    }
};

#endif // INCLUDED_SET_H
