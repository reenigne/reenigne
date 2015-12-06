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
            if (count() >= this->allocated()*3/4) {
                int n = this->allocated()*2;
                if (n == 0)
                    n = 1;
                Set<Key> other(n);
                other.expand(n);
                other.body()->_size = count();
                for (auto i : *this)
                    other.add(i);
                *this = other;
                e = lookup(key);
            }
            *e = key;
        }
    }
    ~Set() { if (this->body() != 0) this->body()->_size = this->allocated(); }
    int count() const { return this->body()->_size; }
    class Iterator
    {
    public:
        const Key& operator*() const { return *_key; }
        bool operator==(const Iterator& other) const
        {
            return _key == other._key;
        }
        bool operator!=(const Iterator& other) const
        {
            return !operator==(other);
        }
        void operator++()
        {
            do {
                ++_key;
                if ((*this) == _set.end())
                    return;
            } while (*_key == Key());
        }
    private:
        Iterator(const Key* key, const Set& set) : _key(key), _set(set) { }
        const Key* _key;
        const Set _set;

        friend class Set;
    };
    Iterator begin() const
    {
        if (this->allocated() == 0)
            return Iterator(0, this);
        Iterator i(&(*this)[0], this);
        if (i.key() == Key())
            ++i;
        return i;
    }
    Iterator end() const
    {
        if (this->allocated() == 0)
            return Iterator(0, this);
        return Iterator(&(*this)[this->allocated()], this);
    }
private:
    int row(const Key& key) const { return hash(key) % this->allocated(); }
    Key* lookup(const Key& key)
    {
        int r = row(key);
        for (int i = 0; i < this->allocated(); ++i) {
            // We have a decent hash function so linear probing should work
            // fine.
            r = (r + 1)%this->allocated();
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
        for (int i = 0; i < this->allocated(); ++i) {
            r = (r + 1)%this->allocated();
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
