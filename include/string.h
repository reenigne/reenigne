#ifndef INCLUDED_STRING_H
#define INCLUDED_STRING_H

#include <string.h>
#include "minimum_maximum.h"
#include "reference_counted.h"

/*
TODO:
  Will probably need DecimalStringImplementation - more complicated than HexadecimalStringImplementation
    Powers of 10
  Rename _string member of String to _implementation
*/

class Buffer : public ReferenceCounted
{
public:
    Buffer() { }
    Buffer(const UInt8* data) : _implementation(new NonOwningBuffer(data)) { }
    bool operator==(const Buffer& other) const { return _implementation == other._implementation; }
    String fileName() const { return _implementation->fileName(); }
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
    const UInt8* data() const { return &_data[0]; }
private:
    const UInt8* _data;
};

class NonOwningBuffer : public Buffer
{
public:
    NonOwningBuffer(const UInt8* data) { setData(data); }
    String fileName() const
    {
        static String commandLine("Command line");
        return commandLine;
    }
};

class OwningBuffer : public Buffer
{
public:
    OwningBuffer(String fileName) : _fileName(fileName) { }
    void allocate(int bytes) { _data->allocate(bytes); setData(&_data[0]); }
    UInt8* data() { return &_data[0]; }
    String fileName() const { return _fileName; }
private:
    Array<UInt8> _data;
    String _fileName;
};

class String
{
public:
    String(const char* data) : _string(new SimpleStringImplementation(reinterpret_cast<const UInt8*>(data), 0, strlen(data))) { }
    String(UInt32 value, int length) : _string(new HexadecimalStringImplementation(value, length)) { }
    String subString(int start, int length)
    {
        return String(_string->subString(start, length));
    }
    const String& operator+=(const String& other)
    {
        _string = _string->withAppended(other._string);
        return *this;
    }
    String operator+(const String& other)
    {
        String t = *this;
        t += other;
        return t;
    }
    void copyTo(Array<UInt8>* data)
    {
        int l = length();
        data->allocate(l + 1);
        _string->copyTo(&data[0]);
        data[l] = 0;
    }
    int hash() const { return _string->hash(0); }
    bool operator==(const String& other) const
    {
        int l = length();
        if (l != other.length())
            return false;
        return _string->compare(0, other._string, 0, l) == 0;
    }
    bool operator<(const String& other) const
    {
        int l = length();
        int otherLength = other.length())
        int c = _string->compare(0, other._string, 0, min(l, otherLength));
        if (c != 0)
            return c < 0;
        return l < otherLength;
    }
    bool operator>(const String& other) const { return other < *this; }
    bool operator<=(const String& other) const { return !operator>(other); }
    bool operator>=(const String& other) const { return !operator<(other); }
    UInt8 operator[](int offset) const { return _string->byteAt(offset); }
    CharacterSource start() { return CharacterSource(*this); }
    int length() const { return _string->length(); }
    bool empty() const { return length() == 0; }

    void initSimpleData(int offset, Buffer* buffer, int* start, int* length)
    {
        _string->initSimpleData(offset, buffer, start, length);
    }
private:
    Reference<StringImplementation> _string;
};

class StringImplementation : public ReferenceCounted
{
public:
    int length() const { return _length; }
    virtual Reference<StringImplementation> subString(int start, int length) = 0;
    virtual Reference<StringImplementation> withAppended(StringImplementation* other) = 0;
    virtual void copyTo(UInt8* buffer) = 0;
    virtual int hash(int h) = 0;
    virtual int compare(int start, const StringImplementation* other, int otherStart, int l) = 0;  // works like memcmp(this+start, other+otherStart, l) - returns 1 if this is greater.
    virtual int compare(int start, const UInt8* data, int l) = 0;  // works like memcmp(this+start, data, l) - returns 1 if this is greater.
    virtual UInt8 byteAt(int offset) = 0;
    virtual Buffer buffer() = 0;
    virtual int offset() = 0;
    virtual void initSampleData(int offset, Buffer* buffer, int* start, int* length) = 0;
protected:
    void setLength(int length) { _length = length; }
private:
    int _length = 0;
};

class SimpleStringImplementation : public StringImplementation
{
public:
    Reference<StringImplementation> subString(int start, int length)
    {
        return new SimpleStringImplementation(_buffer, _start + start, length);
    }
    SimpleStringImplementation(const UInt8* data, int start, int length)
      : _buffer(data),
        _start(start)
    {
        setLength(length);
    }
    SimpleStringImplementation(const Buffer& buffer, int start, int length)
      : _buffer(buffer),
        _start(start)
    {
        setLength(length);
    }
    Reference<StringImplementation> withAppended(StringImplementation* other) const
    {
        Reference<SimpleStringImplementation> simpleOther(other);
        if (simpleOther.valid() && _buffer == simpleOther->_buffer && _start + length() == simpleOther->_start)
            return new SimpleStringImplementation(_buffer, _start, length() + simpleOther->length())
        return new ConcatenatedStringImplementation(this, other);
    }
    void copyTo(UInt8* destination) const
    {
        _buffer.copyTo(destination, _start, length());
    }
    int hash(int h) const
    {
        for (int i = 0; i < length(); ++i)
            h = h * 67 + _buffer->data()[_start + i] - 113;
        return h;
    }
    int compare(int start, const StringImplementation* other, int otherStart, int l) const
    {
        return -other->compare(otherStart, _buffer->data() + _start + start, l);
    }
    int compare(int start, const UInt8* data, int l) const
    {
        return memcmp(_buffer()->data() + _start + start, data, l);
    }
    UInt8 byteAt(int offset) const { return _buffer->data()[_start + offset]; }
    Buffer buffer() const { return _buffer; }
    int offset() const { return _start; }
    void initSimpleData(int offset, Buffer* buffer, int* start, int* length)
    {
        *buffer = _buffer;
        *start = _start + offset;
        *length = length() - offset;
    }
private:
    Buffer _buffer;
    int _start;
};

class ConcatenatedStringImplementation : public StringImplemenation
{
public:
    ConcatenatedStringImplementation(StringImplementation* left, StringImplementation* right)
      : _left(left), _right(right) { }
    Reference<StringImplementation> subString(int start, int length) const
    {
        int leftLength = _left->length();
        if (start >= leftLength)
            return _right->subString(start - _left->length(), length);
        if (start + length <= leftLength)
            return _left->subString(start, length);
        return new ConcatenatedStringImplementation(
            _left->subString(start, leftLength - start),
            _right->subString(0, length - leftLength));
    }
    Reference<StringImplementation> withAppended(StringImplementation* other) const
    {
        return new ConcatenatedStringImplementation(this, other);
    }
    void copyTo(UInt8* destination) const
    {
        _left->copyTo(destination);
        _right->copyTo(destination + _left->length());
    }
    int hash(int h) const
    {
        h = _left->hash(h);
        return _right->hash(h);
    }
    int compare(int start, const StringImplementation* other, int otherStart, int l)
    {
        int leftLength = _left->length();
        if (start < leftLength) {
            if (start + l <= leftLength)
                return _left->compare(start, other, otherStart, l);
            int left = leftLength - start;
            int c = _left->compare(start, other, otherStart, left);
            if (c != 0)
                return c;
            return _right->compare(0, other, otherStart + left, l - left);
        }
        return _right->compare(start - leftLength, other, otherStart, l);
    }
    int compare(int start, const UInt8* data, int l)
    {
        int leftLength = _left->length();
        if (start < leftLength) {
            if (start + l <= leftLength)
                return _left->compare(start, data, l);
            int left = leftLength - start;
            int c = _left->compare(start, data, left);
            if (c != 0)
                return c;
            return _right->compare(0, data + left, l - left);
        }
        return _right->compare(start - leftLength, data, l);
    }
    UInt8 byteAt(int offset) const
    {
        int leftLength = _left->length();
        if (offset < leftLength)
            return _left->byteAt(offset);
        return _right->byteAt(offset - leftLength);
    }
    Buffer buffer() const { return _left->buffer(); }
    int offset() const { return _left->offset(); }
    void initSimpleData(int offset, Buffer* buffer, int* start, int* length)
    {
        int leftLength = _left->length();
        if (offset < leftLength)
            return _left->initSimpleData(offset, buffer, start, length);
        return _right->initSimpleData(offset - leftLength, buffer, start, length);
    }
private:
    Reference<StringImplementation> _left;
    Reference<StringImplementation> _right;
};

class HexadecimalStringImplementation : public StringImplementation
{
public:
    HexadecimalStringImplementation(UInt32 value, int length)
      : _value(value)
    {
        setLength(length);
    }
    Reference<StringImplementation> subString(int start, int l) const
    {
        return new HexadecimalStringImplementation(value >> ((length() - (start + l)) << 2), length);
    }
    Reference<StringImplementation> withAppended(StringImplementation* other) const
    {
        return new ConcatenatedStringImplementation(this, other);
    }
    void copyTo(UInt8* destination) const
    {
        for (int i = 0; i < length(); ++i)
            *(destination++) = byteAt(i);
    }
    int hash(int h) const
    {
        for (int i = 0; i < length(); ++i)
            h = h * 67 + byteAt(i) - 113;
        return h;
    }
    UInt8 byteAt(int offset) const
    {
        UInt8 nybble = (_value >> ((length() - (offset + 1)) << 2)) & 0x0f;
        return (nybble < 10 ? nybble + '0' : nybble + 'A' - 10);
    }
    int compare(int start, const StringImplementation* other, int otherStart, int l)
    {
        for (int i = 0; i < l; ++i) {
            UInt8 a = byteAt(i);
            UInt8 b = other->byteAt(i + otherStart);
            if (a > b)
                return 1;
            if (a < b)
                return -1;
        }
        return 0;
    }
    int compare(int start, const UInt8* data, int l)
    {
        for (int i = 0; i < l; ++i) {
            UInt8 a = byteAt(i);
            UInt8 b = data[i];
            if (a > b)
                return 1;
            if (a < b)
                return -1;
        }
        return 0;
    }
    Buffer buffer() const { return Buffer(); }
    int offset() const { return 0; }
private:
    UInt32 _value;
};

#endif // INCLUDED_STRING_H
