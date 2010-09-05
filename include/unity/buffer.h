#ifndef INCLUDED_BUFFER_H
#define INCLUDED_BUFFER_H

template<class T> class NonOwningBufferImplementationTemplate;
template<class T> class OwningBufferImplementationTemplate;
template<class T> class BufferTemplate;

typedef NonOwningBufferImplementationTemplate<void> NonOwningBufferImplementation;
typedef OwningBufferImplementationTemplate<void> OwningBufferImplementation;
typedef BufferTemplate<void> Buffer;

class Buffer;

#include "unity/string.h"
#include "unity/array.h"
#include "unity/integer_types.h"
#include "unity/reference_counted.h"

class BufferImplementation : public ReferenceCounted
{
public:
    void copyTo(UInt8* destination, int start, int length)
    {
        memcpy(destination, _data + start, length);
    }
    virtual String fileName() const = 0;
protected:
    void setData(const UInt8* data) { _data = data; }
    const UInt8* data() const { return &_data[0]; }
private:
    const UInt8* _data;
};

template<class T> class NonOwningBufferImplementationTemplate : public BufferImplementation
{
public:
    NonOwningBufferImplementationTemplate(const UInt8* data) { setData(data); }
    StringTemplate<T> fileName() const
    {
        static String commandLine("Command line");
        return commandLine;
    }
};

template<class T> class OwningBufferImplementationTemplate : public BufferImplementation
{
public:
    OwningBufferImplementationTemplate(const String& fileName) : _fileName(fileName) { }
    void allocate(int bytes) { _data->allocate(bytes); setData(&_data[0]); }
    UInt8* data() { return &_data[0]; }
    StringTemplate<T> fileName() const { return _fileName; }
private:
    Array<UInt8> _data;
    String _fileName;
};

template<class T> class BufferTemplate
{
public:
    BufferTemplate() { }
    BufferTemplate(const UInt8* data) : _implementation(new NonOwningBufferImplementation(data)) { }
    BufferTemplate(Reference<BufferImplementation> implementation) : _implementation(implementation) { }
    bool operator==(const Buffer& other) const { return _implementation == other._implementation; }
    StringTemplate<T> fileName() const { return _implementation->fileName(); }
    const UInt8* data() const { return _implementation->data(); }
private:
    Reference<BufferImplementation> _implementation;
};

#endif // INCLUDED_BUFFER_H
