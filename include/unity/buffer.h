#ifndef INCLUDED_BUFFER_H
#define INCLUDED_BUFFER_H

#include "unity/integer_types.h"
#include "unity/reference_counted.h"
#include "unity/array.h"

class BufferImplementation : public ReferenceCounted
{
public:
    void copyTo(UInt8* destination, int start, int length)
    {
        memcpy(destination, _data + start, length);
    }
    const UInt8* data() const { return &_data[0]; }
protected:
    void setData(const UInt8* data) { _data = data; }
private:
    const UInt8* _data;
};

class NonOwningBufferImplementation : public BufferImplementation
{
public:
    NonOwningBufferImplementation(const UInt8* data) { setData(data); }
};

class OwningBufferImplementation : public BufferImplementation
{
public:
    void allocate(int bytes) { _data.allocate(bytes); setData(&_data[0]); }
    UInt8* data() { return &_data[0]; }
private:
    Array<UInt8> _data;
};

class Buffer
{
public:
    Buffer() { }
    Buffer(const UInt8* data) : _implementation(new NonOwningBufferImplementation(data)) { }
    Buffer(Reference<BufferImplementation> implementation) : _implementation(implementation) { }
    bool operator==(const Buffer& other) const { return _implementation == other._implementation; }
    const UInt8* data() const { return _implementation->data(); }
    bool valid() const { return _implementation.valid(); }
    void copyTo(UInt8* destination, int start, int length) const { _implementation->copyTo(destination, start, length); }
    const UInt8& operator[](int i) const { return data()[i]; }
protected:
    Reference<BufferImplementation> _implementation;
};

class OwningBuffer : public Buffer
{
public:
    OwningBuffer(int size) : Buffer(new OwningBufferImplementation)
    { 
        implementation()->allocate(size);
    }
    UInt8& operator[](int i) { return implementation()->data()[i]; }
private:
    OwningBufferImplementation* implementation()
    {
        return Reference<OwningBufferImplementation>(_implementation);
    }
};

#endif // INCLUDED_BUFFER_H
