#ifndef INCLUDED_BUFFER_H
#define INCLUDED_BUFFER_H

#include "unity/integer_types.h"
#include "unity/reference_counted.h"
#include "unity/array.h"

class Buffer
{
protected:
    class Implementation : public ReferenceCounted
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

    Reference<Implementation> _implementation;
public:
    class NonOwningImplementation : public Implementation
    {
    public:
        NonOwningImplementation(const UInt8* data) { setData(data); }
    };

    Buffer() { }
    Buffer(const UInt8* data)
      : _implementation(new NonOwningImplementation(data)) { }
    Buffer(const Reference<Implementation>& implementation)
      : _implementation(implementation) { }
    bool operator==(const Buffer& other) const
    {
        return _implementation == other._implementation; 
    }
    const UInt8* data() const { return _implementation->data(); }
    bool valid() const { return _implementation.valid(); }
    void copyTo(UInt8* destination, int start, int length) const
    {
        _implementation->copyTo(destination, start, length);
    }
    const UInt8& operator[](int i) const { return data()[i]; }
};

class OwningBuffer : public Buffer
{
public:
    OwningBuffer(int size) : Buffer(new Implementation)
    { 
        implementation()->allocate(size);
    }
    UInt8& operator[](int i) { return data()[i]; }
    UInt8* data() { return implementation()->data(); }
private:
    class Implementation : public Buffer::Implementation
    {
    public:
        void allocate(int bytes) { _data.allocate(bytes); setData(&_data[0]); }
        UInt8* data() { return &_data[0]; }
    private:
        Array<UInt8> _data;
    };

    Implementation* implementation()
    {
        return Reference<Implementation>(_implementation);
    }
};

class GrowingBuffer : public Buffer
{
private:
    class Implementation : public Buffer::Implementation
    {
    public:
        Implementation() : _n(0) { }
        void allocate(int bytes)
        {
            if (bytes > _data.count()) {
                int newBytes = _data.count();
                if (newBytes == 0)
                    newBytes = 1;
                while (newBytes < bytes)
                    newBytes *= 2;
                Array<UInt8> data(newBytes);
                memcpy(&data[0], &_data[0], _n);
                _data.swap(data);
                setData(&_data[0]);
            }
            else
                if (bytes <= _data.count() / 2) {
                    int newBytes = 1;
                    while (newBytes < bytes)
                        newBytes *= 2;
                    Array<UInt8> data(newBytes);
                    // Don't need to preserve data when shrinking.
                    _data.swap(data);
                    setData(&_data[0]);
                }
            _n = bytes;
        }
        UInt8* data() { return &_data[0]; }
        int count() { return _n; }
    private:
        Array<UInt8> _data;
        int _n;
    };
    Implementation* implementation()
    {
        return Reference<Implementation>(_implementation);
    }
public:
    GrowingBuffer() : Buffer(new Implementation) { }
    GrowingBuffer(const char* p) : Buffer(new Implementation)
    {
        int n = strlen(p);
        allocate(n);
        memcpy(data(), p, n);
    }
    const UInt8* data() const { return _implementation->data(); }
    UInt8* data() { return implementation()->data(); }
    void allocate(int bytes) { implementation()->allocate(bytes); }
    int count() { return implementation()->count(); }
};

#endif // INCLUDED_BUFFER_H
