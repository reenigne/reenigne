#include "alfe/main.h"

#ifndef INCLUDED_SET_H
#define INCLUDED_SET_H

template<class Key> class SetBody : public Array<Key>::AppendableBaseBody
{
public:
    virtual void justSetSize(int size) const = 0;
    void preDestroy() const { justSetSize(this->_allocated); }
};

// Set is not quite a value type, since adding an element in one set will
// affect copies of the same set. Adding an element may cause it to become a
// deep copy, if more storage space was needed.
//
// Note that the default-constructed Key should not be used for real entries.
template<class Key> class Set : private AppendableArray<Key, SetBody<Key>>
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
        if (e != 0 && *e == key)
            return;
        if (count() >= this->allocated()*3/4) {
            int n = this->allocated()*2;
            if (n == 0)
                n = 1;
            Set other;
            other.allocate(n);
            n = other.allocated();
            other.expand(n);
            other.body()->_size = 0;
            for (auto i : *this)
                other.add(i);
            *this = other;
            e = lookup(key);
        }
        ++this->body()->_size;
        *e = key;
    }
    int count() const { return this->body() == 0 ? 0 : this->body()->_size; }
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
            return Iterator(0, *this);
        Iterator i(&(*this)[0], *this);
        if (*i == Key())
            ++i;
        return i;
    }
    Iterator end() const
    {
        if (this->allocated() == 0)
            return Iterator(0, *this);
        return Iterator(&(*this)[this->allocated()], *this);
    }
private:
    int row(const Key& key) const { return ::hash(key) % this->allocated(); }
    Key* lookup(const Key& key)
    {
        if (this->allocated() == 0)
            return 0;
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
        if (this->allocated() == 0)
            return 0;
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
