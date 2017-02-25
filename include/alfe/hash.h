#include "alfe/main.h"

#ifndef INCLUDED_HASH_H
#define INCLUDED_HASH_H

#include <typeinfo>

class Hash
{
public:
    // FNV-1a hash
    Hash(const std::type_info& t) : _h(0x811c9dc5)
    {
        mixin(static_cast<UInt32>(t.hash_code()));
    }
    Hash& mixin(UInt32 v) { _h = (_h ^ v) * 0x01000193; return *this; }
    operator UInt32() const { return _h; }
private:
    UInt32 _h;
};

template<class T> UInt32 hash(const T& t) { return t.hash(); }
UInt32 hash(int t) { return Hash(typeid(int)).mixin(t); }
UInt32 hash(DWord t) { return Hash(typeid(int)).mixin(t); }
UInt32 hash(UInt64 t)
{
    return Hash(typeid(UInt64)).mixin(static_cast<UInt32>(t)).
        mixin(static_cast<UInt32>(t >> 32));
}

#endif // INCLUDED_HASH_H
