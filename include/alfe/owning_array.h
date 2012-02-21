#include "alfe/main.h"

#ifndef INCLUDED_OWNING_ARRAY_H
#define INCLUDED_OWNING_ARRAY_H

// An object which holds other objects and deletes them all on destruction.
template<class T> class OwningArray : public AppendableArray<T*>
{
public:
    void add(T* object)
    {
        append(object);
    }
    ~OwningArray()
    {
        for (int i = 0; i < count(); ++i)
            delete (*this)[i];
    }
};

#endif // INCLUDED_OWNING_ARRAY_H
