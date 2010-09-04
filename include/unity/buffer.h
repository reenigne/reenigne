#ifndef INCLUDED_BUFFER_H
#define INCLUDED_BUFFER_H

class Buffer;

#include "unity/string.h"
#include "unity/array.h"
#include "unity/integer_types.h"
#include "unity/reference_counted.h"

class Buffer : public ReferenceCounted
{
public:
    Buffer() { }
    Buffer(const UInt8* data) : _implementation(new NonOwningBuffer(data)) { }
    Buffer(Reference<BufferImplementation> implementation) : _implementation(implmentation) { }
    bool operator==(const Buffer& other) const { return _implementation == other._implementation; }
    String fileName() const { return _implementation->fileName(); }
    bool valid() const { return _implementation.valid(); }
    UInt8 operator[](int offset) const { return _implementation->data()[offset]; }
private:
    Reference<BufferImplementation> _implementation;
};

class BufferImplementation : public ReferenceCounted
{
public:
    void copyTo(UInt8* destination, int start, int length)
    {
        memcpy(destination, _data + start, length);
    }
    String fileName() const = 0;
protected:
    void setData(const UInt8* data) { _data = data; }
    const UInt8* data() const { _data; }
private:
    const UInt8* _data;
};

class NonOwningBufferImplementation : public BufferImplementation
{
public:
    NonOwningBuffer(const UInt8* data) { setData(data); }
    String fileName() const
    {
        static String commandLine("Command line");
        return commandLine;
    }
};

class OwningBufferImplementation : public BufferImplementation
{
public:
    OwningBuffer(String fileName) : _fileName(fileName) { }
    void allocate(int bytes) { _data->allocate(bytes); setData(&_data[0]); }
    UInt8* writableData() { return &_data[0]; }
    String fileName() const { return _fileName; }
private:
    Array<UInt8> _data;
    String _fileName;
};

#endif // INCLUDED_BUFFER_H
