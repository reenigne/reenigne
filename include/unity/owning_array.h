#ifndef INCLUDED_OWNING_ARRAY_H
#define INCLUDED_OWNING_ARRAY_H

// An object which holds other objects and deletes them all on destruction.
template<class T> class OwningArray
{
public:
    void add(T* object)
    {
        _array.append(object);
    }
    ~OwningArray()
    {
        for (int i = 0; i < _array.count(); ++i)
            delete _array[i];
    }
private:
    AppendableArray<T*> _array;
};

#endif // INCLUDED_OWNING_ARRAY_H
