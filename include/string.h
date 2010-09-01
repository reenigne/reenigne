#ifndef INCLUDED_STRING_H
#define INCLUDED_STRING_H

#include <string.h>
#include "minimum_maximum.h"
#include "reference_counted.h"

class Buffer : public ReferenceCounted
{
public:
    Buffer() { }
    Buffer(const UInt8* data) : _implementation(new NonOwningBuffer(data)) { }
    Buffer(Reference<BufferImplementation> implementation) : _implementation(implementation) { }
    bool operator==(const Buffer& other) const { return _implementation == other._implementation; }
    String fileName() const { return _implementation->fileName(); }
    const UInt8* data() const { return _implementation->data(); }
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
    OwningBuffer(const String& fileName) : _fileName(fileName) { }
    void allocate(int bytes) { _data->allocate(bytes); setData(&_data[0]); }
    UInt8* data() { return &_data[0]; }
    String fileName() const { return _fileName; }
private:
    Array<UInt8> _data;
    String _fileName;
};


class CharacterSource
{
public:
    CharacterSource(const String& string) : _string(string), _offset(0)
    {
        initSimpleData();
    }
    int get()
    {
        static String overlongEncoding("Overlong encoding");
        static String codePointTooHigh("Code point too high");
        static String unexpectedSurrogate("Unexpected surrogate");

        if (_length == 0)
            return -1;
        int b0 = _buffer[_start];
        if (b0 >= 0 && b0 < 0x80)
            return b;
        if (b0 < 0xc0 || b0 >= 0xf8)
            throwUTF8Exception(true);
        String start = *this;
        next();

        int b1 = getNextByte();
        if (b0 >= 0xc0 && b0 < 0xe0) {
            int r = ((b0 & 0x1f) << 6) | (b1 & 0x3f);
            if (r < 0x80)
                start.throwUTF8Exception(overlongEncoding);
            return r;
        }

        int b2 = getNextByte();
        if (b0 >= 0xe0 && b0 < 0xf0) {
            int r = ((b0 & 0x0f) << 12) | ((b1 & 0x3f) << 6) | (b2 & 0x3f);
            if (r < 0x800)
                start.throwUTF8Exception(overlongEncoding);
            if (r >= 0xd800 && r < 0xe000)
                start.throwUTF8Exception(unexpectedSurrogate);
            return r;
        }

        int b3 = getNextByte();
        int r = ((b0 & 0x07) << 18) | ((b1 & 0x3f) << 12) | ((b2 & 0x3f) << 6) | (b3 & 0x3f);
        if (r < 0x10000)
            start.throwUTF8Exception(overlongEncoding);
        if (r >= 0x110000)
            start.throwUTF8Exception(codePointTooHigh);
        return r;
    }
    bool empty() const
    {
        return _length == 0;
    }
private:
    void throwUTF8Exception(bool first)
    {
        static String expectedFirst("Expected 0x00..0x7F or 0xC0..0xF7, found 0x");
            static String expectedNext("Expected 0x80..0xBF, found 0x");
        String expected = first ? expectedFirst : expectedNext;
        static String endOfString("end of string");
        String s = (_length > 0 ? String::hexadecimal(_buffer[0], 2) : endOfString;
        throwUTF8Exception(expected + s);
    }
    void throwUTF8Exception(String message)
    {
        static String at(" at ");
        static String in(" in ");
        String s = subString(offset, 1);
        throw UTF8Exception(message + at + String::hexadecimal(s._implmentation->offset(), 8) + in + s._implementation->buffer()->fileName());
    }
    int getNextByte()
    {
        if (empty())
            throwUTF8Exception(false);
        int b = _buffer[_start];
        if (b < 0x80 || b >= 0xc0)
            throwUTF8Exception(false);
        next();
        return b;
    }
    void initSimpleData()
    {
        _string.initSimpleData(_offset, &_buffer, &_start, &_length);
    }
    void next()
    {
        ++_offset;
        ++_start;
        --_length;
        if (_length != 0)
            return;
        initSimpleData();
    }

    Buffer _buffer;
    int _start;
    int _length;
    String _string;
    int _offset;
};


class String
{
public:
    String(const char* data) : _implementation(new SimpleStringImplementation(reinterpret_cast<const UInt8*>(data), 0, strlen(data))) { }
    String(const Buffer& buffer, int start, int n) : _implementation(new SimpleStringImplementation(buffer, start, n)) { }
#ifdef _WIN32
    String(const WCHAR* utf16)
    {
        int n = 0;
        int i = 0;
        while (true) {
            int c = utf16[i++];
            if (c == 0)
                break;
            if (c >= 0xdc00 && c < 0xe000) {
                static String expected("Expected 0x0000..0xD800 or 0xE000..0xFFFF, found 0x");
                throw Exception(expected + String::hexadecimal(c, 4));
            }
            if (c >= 0xd800 && c < 0xdc00) {
                int c2 = utf16[i++];
                if (c2 < 0xdc00 || c2 >= 0xe000) {
                    static String expected("Expected 0xDC00..0xDFFF, found 0x");
                    throw Exception(expected + String::hexadecimal(c2, 4));
                }
                ++n;
                continue;
            }
            if (c >= 0x800)
                ++n;
            if (c >= 0x80)
                ++n;
            ++n;
        }
        static String system("System");
        Reference<OwningBufferImplementation> bufferImplementation = new OwningBufferImplementation(system);
        bufferImplementation->allocate(n);
        i = 0;
        UInt8* p = bufferImplementation->data();
        while (true) {
            int codePoint = utf16[i++];
            if (codePoint == 0)
                break;
            if (codePoint < 0x80)
                *(p++) = codePoint;
            else {
                if (codePoint < 0x800)
                    *(p++) = 0xc0 | (codePoint >> 6);
                else {
                    if (codePoint >= 0xd800 && codePoint < 0xdc00) {
                        codePoint = (((codePoint & 0x3ff)<<10) | (utf16[i++] & 0x3ff)) + 0x10000;
                        *(p++) = 0xf0 | (codePoint >> 18);
                        *(p++) = 0x80 | ((codePoint >> 12) & 0x3f);
                    }
                    else
                        *(p++) = 0xe0 | (codePoint >> 12);
                    *(p++) = 0x80 | ((codePoint >> 6) & 0x3f);
                }
                *(p++) = 0x80 | (codePoint & 0x3f);
            }
        }
        _implementation = new SimpleStringImplementation(Buffer(bufferImplementation), 0, n);
    }
#endif
    static String hexadecimal(UInt32 value, int length)
    {
        String s;
        s._implementation = new HexadecimalStringImplementation(value, length);
        return s;
    }
    String subString(int start, int length)
    {
        return String(_implementation->subString(start, length));
    }
    const String& operator+=(const String& other)
    {
        _implementation = _implementation->withAppended(other._implementation);
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
        _implementation->copyTo(&data[0]);
        data[l] = 0;
    }
#ifdef _WIN32
    void copyToUTF16(Array<WCHAR>* data)
    {
        CharacterSource s = start();
        int l = 2;
        while (!s.empty()) {
            int c = s.get();
            l += 2;
            if (c >= 0x10000)
                l += 2;
        }
        s = start();
        data->allocate(l);
        while (!s.empty()) {
            int c = s.get();
            if (c >= 0x10000) {
                c -= 0x10000;
                data[l++] = 0xd800 + ((c >> 10) & 0x03ff);
                data[l++] = 0xdc00 + (c & 0x03ff);
            }
            else {
                data[l++] = c;
            }
        }
        data[l++] = 0;
    }
#endif
    int hash() const { return _implementation->hash(0); }
    bool operator==(const String& other) const
    {
        int l = length();
        if (l != other.length())
            return false;
        return _implementation->compare(0, other._implementation, 0, l) == 0;
    }
    bool operator!=(const String& other) const { return !operator==(other); }
    bool operator<(const String& other) const
    {
        int l = length();
        int otherLength = other.length())
        int c = _implementation->compare(0, other._implementation, 0, min(l, otherLength));
        if (c != 0)
            return c < 0;
        return l < otherLength;
    }
    bool operator>(const String& other) const { return other < *this; }
    bool operator<=(const String& other) const { return !operator>(other); }
    bool operator>=(const String& other) const { return !operator<(other); }
    UInt8 operator[](int offset) const { return _implementation->byteAt(offset); }
    CharacterSource start() { return CharacterSource(*this); }
    int length() const { return _implementation->length(); }
    bool empty() const { return length() == 0; }
    void write(const Handle& handle) const { _implementation->write(handle); }

    void initSimpleData(int offset, Buffer* buffer, int* start, int* length)
    {
        _implementation->initSimpleData(offset, buffer, start, length);
    }
private:
    Reference<StringImplementation> _implementation;
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
    virtual void write(const Handle& handle) const = 0;
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
            h = h * 67 + _buffer.data()[_start + i] - 113;
        return h;
    }
    int compare(int start, const StringImplementation* other, int otherStart, int l) const
    {
        return -other->compare(otherStart, _buffer,data() + _start + start, l);
    }
    int compare(int start, const UInt8* data, int l) const
    {
        return memcmp(_buffer()->data() + _start + start, data, l);
    }
    UInt8 byteAt(int offset) const { return _buffer.data()[_start + offset]; }
    Buffer buffer() const { return _buffer; }
    int offset() const { return _start; }
    void initSimpleData(int offset, Buffer* buffer, int* start, int* length)
    {
        *buffer = _buffer;
        *start = _start + offset;
        *length = length() - offset;
    }
    void write(const Handle& handle)
    {
#ifdef _WIN32
        DWORD bytesWritten;
        if (WriteFile(handle, reinterpret_cast<LPCVOID>(_buffer.data() + _start), length(), &bytesWritten, NULL) == 0 || bytesWritten != length()) {
            static String writingFile("Writing file ");
            Exception::throwSystemError(writingFile + handle.name());
        }
#else
        ssize_t writeResult = write(fileDescriptor, static_cast<void*>(_buffer.data() + _start), length());
        static String readingFile("Writing file ");
        if (writeResult < length()) {
            static String writingFile("Writing file ");
            Exception::throwSystemError(writingFile + handle.name());
        }
#endif
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
    void write(const Handle& handle)
    {
        _left->write(handle);
        _right->write(handle);
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
    void write(const Handle& handle)
    {
        UInt8 buffer[8];
        copyTo(buffer);
#ifdef _WIN32
        DWORD bytesWritten;
        if (WriteFile(handle, reinterpret_cast<LPCVOID>(buffer), length(), &bytesWritten, NULL) == 0 || bytesWritten != length()) {
            static String writingFile("Writing file ");
            Exception::throwSystemError(writingFile + handle.name());
        }
#else
        ssize_t writeResult = write(fileDescriptor, static_cast<void*>(buffer), length());
        static String readingFile("Writing file ");
        if (writeResult < length()) {
            static String writingFile("Writing file ");
            Exception::throwSystemError(writingFile + handle.name());
        }
#endif
    }
private:
    UInt32 _value;
};


#endif // INCLUDED_STRING_H
