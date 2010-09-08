#ifndef INCLUDED_STRING_H
#define INCLUDED_STRING_H

template<class T> class StringTemplate;
typedef StringTemplate<void> String;

class StringImplementation;

template<class T> class SimpleStringImplementationTemplate;
typedef SimpleStringImplementationTemplate<void> SimpleStringImplementation;

template<class T> class ConcatenatedStringImplementationTemplate;
typedef ConcatenatedStringImplementationTemplate<void> ConcatenatedStringImplementation;

//template<class T> class HexadecimalStringImplementationTemplate;
//typedef HexadecimalStringImplementationTemplate<void> HexadecimalStringImplementation;

class HexadecimalStringImplementation;
class DecimalStringImplementation;
class CodePointStringImplementation;

template<class T> class CharacterSourceTemplate;
typedef CharacterSourceTemplate<void> CharacterSource;

template<class T> class HandleTemplate;
typedef HandleTemplate<void> Handle;

template<class T> class ExceptionTemplate;
typedef ExceptionTemplate<void> Exception;

#ifdef _WIN32
template<class T> class LocalStringTemplate;
typedef LocalStringTemplate<void> LocalString;
#endif

template<class T> class NonOwningBufferImplementationTemplate;
typedef NonOwningBufferImplementationTemplate<void> NonOwningBufferImplementation;

template<class T> class OwningBufferImplementationTemplate;
typedef OwningBufferImplementationTemplate<void> OwningBufferImplementation;

template<class T> class BufferTemplate;
typedef BufferTemplate<void> Buffer;

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include <exception>
#include <string.h>
#include "unity/uncopyable.h"
#include "unity/integer_types.h"
#include "unity/minimum_maximum.h"
#include "unity/reference_counted.h"
#include "unity/array.h"

class BufferImplementation : public ReferenceCounted
{
public:
    void copyTo(UInt8* destination, int start, int length)
    {
        memcpy(destination, _data + start, length);
    }
    virtual String fileName() const = 0;
    const UInt8* data() const { return &_data[0]; }
protected:
    void setData(const UInt8* data) { _data = data; }
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
    void allocate(int bytes) { _data.allocate(bytes); setData(&_data[0]); }
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
    bool valid() const { return _implementation.valid(); }
    void copyTo(UInt8* destination, int start, int length) const { _implementation->copyTo(destination, start, length); }
    const UInt8& operator[](int i) const { return data()[i]; }
private:
    Reference<BufferImplementation> _implementation;
};

template<class T> class StringTemplate
{
public:
    StringTemplate() : _implementation(new SimpleStringImplementation(Buffer(), 0, 0)) { }
    StringTemplate(const char* data) : _implementation(new SimpleStringImplementation(reinterpret_cast<const UInt8*>(data), 0, strlen(data))) { }
    StringTemplate(const Buffer& buffer, int start, int n) : _implementation(new SimpleStringImplementation(buffer, start, n)) { }
#ifdef _WIN32
    StringTemplate(const WCHAR* utf16)
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
    static String decimal(SInt32 value)
    {
        String s;
        s._implementation = new DecimalStringImplementation(value);
        return s;
    }
    static String codePoint(int codePoint)
    {
        String s;
        s._implementation = new CodePointStringImplementation(codePoint);
        return s;
    }
    String subString(int start, int length) const
    {
        return String(_implementation->subString(start, length));
    }
    const String& operator+=(const String& other)
    {
        _implementation = _implementation->withAppended(other._implementation);
        return *this;
    }
    String operator+(const String& other) const
    {
        String t = *this;
        t += other;
        return t;
    }
    void copyTo(Array<UInt8>* data) const
    {
        int l = length();
        data->allocate(l + 1);
        _implementation->copyTo(&data[0]);
        data[l] = 0;
    }
#ifdef _WIN32
    void copyToUTF16(Array<WCHAR>* data) const
    {
        CharacterSource s = start();
        int l = 1;
        while (!s.empty()) {
            int c = s.get();
            ++l;
            if (c >= 0x10000)
                ++l;
        }
        s = start();
        data->allocate(l);
        WCHAR* d = &(*data)[0];
        l = 0;
        while (!s.empty()) {
            int c = s.get();
            if (c >= 0x10000) {
                c -= 0x10000;
                d[l++] = 0xd800 + ((c >> 10) & 0x03ff);
                d[l++] = 0xdc00 + (c & 0x03ff);
            }
            else {
                d[l++] = c;
            }
        }
        d[l++] = 0;
    }
#endif
    int hash() const { return _implementation->hash(0); }
    bool operator==(const String& other) const
    {
        int l = length();
        if (l != other.length())
            return false;
        if (l == 0)
            return true;
        return _implementation->compare(0, other._implementation, 0, l) == 0;
    }
    bool operator!=(const String& other) const { return !operator==(other); }
    bool operator<(const String& other) const
    {
        int l = length();
        int otherLength = other.length();
        int c = _implementation->compare(0, other._implementation, 0, min(l, otherLength));
        if (c != 0)
            return c < 0;
        return l < otherLength;
    }
    bool operator>(const String& other) const { return other < *this; }
    bool operator<=(const String& other) const { return !operator>(other); }
    bool operator>=(const String& other) const { return !operator<(other); }
    UInt8 operator[](int offset) const { return _implementation->byteAt(offset); }
    CharacterSource start() const;
    int length() const { return _implementation->length(); }
    bool empty() const { return length() == 0; }
    void write(const Handle& handle) const { _implementation->write(handle); }

    void initSimpleData(int offset, Buffer* buffer, int* start, int* length)
    {
        _implementation->initSimpleData(offset, buffer, start, length);
    }
private:
    StringTemplate(const Reference<StringImplementation>& implementation) : _implementation(implementation) { }

    Reference<StringImplementation> _implementation;

    template<class T> friend class CharacterSourceTemplate;
};

class StringImplementation : public ReferenceCounted
{
public:
	StringImplementation() : _length(0) { }
    int length() const { return _length; }
    virtual Reference<StringImplementation> subString(int start, int length) const = 0;
    virtual Reference<StringImplementation> withAppended(const Reference<StringImplementation>& other) = 0;
    virtual void copyTo(UInt8* buffer) const = 0;
    virtual int hash(int h) const = 0;
    virtual int compare(int start, const StringImplementation* other, int otherStart, int l) const = 0;  // works like memcmp(this+start, other+otherStart, l) - returns 1 if this is greater.
    virtual int compare(int start, const UInt8* data, int l) const = 0;  // works like memcmp(this+start, data, l) - returns 1 if this is greater.
    virtual UInt8 byteAt(int offset) const = 0;
    virtual Buffer buffer() const = 0;
    virtual int offset() const = 0;
    virtual void initSimpleData(int offset, Buffer* buffer, int* start, int* length) const = 0;
    virtual void write(const Handle& handle) const = 0;
protected:
    void setLength(int length) { _length = length; }
private:
    int _length;
};

template<class T> class SimpleStringImplementationTemplate : public StringImplementation
{
public:
    Reference<StringImplementation> subString(int start, int length) const
    {
        return new SimpleStringImplementation(_buffer, _start + start, length);
    }
    SimpleStringImplementationTemplate(const UInt8* data, int start, int length)
      : _buffer(data),
        _start(start)
    {
        setLength(length);
    }
    SimpleStringImplementationTemplate(const Buffer& buffer, int start, int length)
      : _buffer(buffer),
        _start(start)
    {
        setLength(length);
    }
    Reference<StringImplementation> withAppended(const Reference<StringImplementation>& other)
    {
        Reference<SimpleStringImplementation> simpleOther(other);
        if (simpleOther.valid() && _buffer == simpleOther->_buffer && _start + length() == simpleOther->_start)
            return new SimpleStringImplementation(_buffer, _start, length() + simpleOther->length());
        return new ConcatenatedStringImplementation(this, other);
    }
    void copyTo(UInt8* buffer) const
    {
        _buffer.copyTo(buffer, _start, length());
    }
    int hash(int h) const
    {
        for (int i = 0; i < length(); ++i)
            h = h * 67 + _buffer.data()[_start + i] - 113;
        return h;
    }
    int compare(int start, const StringImplementation* other, int otherStart, int l) const
    {
        return -other->compare(otherStart, _buffer.data() + _start + start, l);
    }
    int compare(int start, const UInt8* data, int l) const
    {
        return memcmp(_buffer.data() + _start + start, data, l);
    }
    UInt8 byteAt(int offset) const { return _buffer.data()[_start + offset]; }
    Buffer buffer() const { return _buffer; }
    int offset() const { return _start; }
    void initSimpleData(int offset, Buffer* buffer, int* start, int* l) const
    {
        *buffer = _buffer;
        *start = _start + offset;
        *l = length() - offset;
    }
    void write(const Handle& handle) const
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
protected:
    Buffer _buffer;
    int _start;
};

template<class T> class ConcatenatedStringImplementationTemplate : public StringImplementation
{
public:
    ConcatenatedStringImplementationTemplate(const Reference<StringImplementation>& left, const Reference<StringImplementation>& right)
      : _left(left), _right(right)
    {
        setLength(_left->length() + _right->length());
    }
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
    Reference<StringImplementation> withAppended(const Reference<StringImplementation>& other)
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
    int compare(int start, const StringImplementation* other, int otherStart, int l) const
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
    int compare(int start, const UInt8* data, int l) const
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
    void initSimpleData(int offset, Buffer* buffer, int* start, int* length) const
    {
        int leftLength = _left->length();
        if (offset < leftLength)
            return _left->initSimpleData(offset, buffer, start, length);
        return _right->initSimpleData(offset - leftLength, buffer, start, length);
    }
    void write(const Handle& handle) const
    {
        _left->write(handle);
        _right->write(handle);
    }
private:
    Reference<StringImplementation> _left;
    Reference<StringImplementation> _right;
};

template<int N> class FixedStringImplementation : public SimpleStringImplementation
{
public:
    FixedStringImplementation(int start, int length)
      : SimpleStringImplementation(Buffer(), start, length),
        _bufferImplementation(_data)
    {
        _buffer = Buffer(&_bufferImplementation);
        _bufferImplementation.addReference();
    }
protected:
    NonOwningBufferImplementation _bufferImplementation;
    UInt8 _data[N];
};

//template<class T> class HexadecimalStringImplementationTemplate : public StringImplementation
//{
//public:
//    HexadecimalStringImplementationTemplate(UInt32 value, int length)
//      : _value(value)
//    {
//        setLength(length);
//    }
//    Reference<StringImplementation> subString(int start, int l) const
//    {
//        return new HexadecimalStringImplementation(_value >> ((length() - (start + l)) << 2), l);
//    }
//    Reference<StringImplementation> withAppended(const Reference<StringImplementation>& other)
//    {
//        return new ConcatenatedStringImplementation(this, other);
//    }
//    void copyTo(UInt8* destination) const
//    {
//        for (int i = 0; i < length(); ++i)
//            *(destination++) = byteAt(i);
//    }
//    int hash(int h) const
//    {
//        for (int i = 0; i < length(); ++i)
//            h = h * 67 + byteAt(i) - 113;
//        return h;
//    }
//    UInt8 byteAt(int offset) const
//    {
//        UInt8 nybble = nybbleAt(offset);
//        return (nybble < 10 ? nybble + '0' : nybble + 'A' - 10);
//    }
//    int compare(int start, const StringImplementation* other, int otherStart, int l) const
//    {
//        for (int i = 0; i < l; ++i) {
//            UInt8 a = byteAt(i);
//            UInt8 b = other->byteAt(i + otherStart);
//            if (a > b)
//                return 1;
//            if (a < b)
//                return -1;
//        }
//        return 0;
//    }
//    int compare(int start, const UInt8* data, int l) const
//    {
//        for (int i = 0; i < l; ++i) {
//            UInt8 a = byteAt(i);
//            UInt8 b = data[i];
//            if (a > b)
//                return 1;
//            if (a < b)
//                return -1;
//        }
//        return 0;
//    }
//    Buffer buffer() const { return Buffer(); }
//    int offset() const { return 0; }
//    void write(const Handle& handle) const
//    {
//        UInt8 buffer[8];
//        copyTo(buffer);
//#ifdef _WIN32
//        DWORD bytesWritten;
//        if (WriteFile(handle, reinterpret_cast<LPCVOID>(buffer), length(), &bytesWritten, NULL) == 0 || bytesWritten != length()) {
//            static String writingFile("Writing file ");
//            Exception::throwSystemError(writingFile + handle.name());
//        }
//#else
//        ssize_t writeResult = write(fileDescriptor, static_cast<void*>(buffer), length());
//        static String readingFile("Writing file ");
//        if (writeResult < length()) {
//            static String writingFile("Writing file ");
//            Exception::throwSystemError(writingFile + handle.name());
//        }
//#endif
//    }
//    void initSimpleData(int offset, Buffer* buffer, int* start, int* l) const
//    {
//        *buffer = Buffer();
//        int n = 0;
//        for (int i = offset; i < length(); ++i)
//            n = (n << 4) | nybbleAt(i);
//        *start = n;
//        *l = length() - offset;
//    }
//private:
//    int nybbleAt(int offset) const
//    {
//        return (_value >> ((length() - (offset + 1)) << 2)) & 0x0f;
//    }
//
//    UInt32 _value;
//};

class HexadecimalStringImplementation : public FixedStringImplementation<8>
{
public:
    HexadecimalStringImplementation(UInt32 value, int length)
      : FixedStringImplementation(8 - length, length)
    {
        for (int i = 7; i >= 8 - length; --i) {
            int n = value & 0xf;
            _data[i] = (n < 10 ? (n + '0') : (n + 'A' - 10));
            value >>= 4;
        }
    }
};

class DecimalStringImplementation : public FixedStringImplementation<11>
{
public:
    DecimalStringImplementation(SInt32 value)
      : FixedStringImplementation(0, 1)
    {
        SInt32 a = value;
        if (value < 0)
            a = -value;
        int i;
        for (i = 10; i >= 0; --i) {
            _data[i] = (a % 10) + '0';
            a /= 10;
            if (a == 0)
                break;
        }
        if (value < 0)
            _data[--i] = '-';
        _start = i;
        setLength(11 - i);
    }
};

class CodePointStringImplementation : public FixedStringImplementation<4>
{
public:
    CodePointStringImplementation(int codePoint)
      : FixedStringImplementation(0, 1)
    {
        if (codePoint < 0x80) {
            _data[0] = codePoint;
            return;
        }
        if (codePoint < 0x800) {
            _data[0] = (codePoint >> 6) | 0xc0;
            _data[1] = (codePoint & 0x3f) | 0x80;
            setLength(2);
            return;
        }
        if (codePoint < 0x10000) {
            _data[0] = (codePoint >> 12) | 0xe0;
            _data[1] = ((codePoint >> 6) & 0x3f) | 0x80;
            _data[2] = (codePoint & 0x3f) | 0x80;
            setLength(3);
            return;
        }
        _data[0] = (codePoint >> 18) | 0xf0;
        _data[1] = ((codePoint >> 12) & 0x3f) | 0x80;
        _data[2] = ((codePoint >> 6) & 0x3f) | 0x80;
        _data[3] = (codePoint & 0x3f) | 0x80;
        setLength(4);
    }
};


template<class T> class CharacterSourceTemplate
{
public:
    CharacterSourceTemplate(const String& string) : _string(string), _offset(0), _line(1), _column(1)
    {
        initSimpleData();
    }
    int getByte()
    {
        if (empty())
            return -1;
        if (_buffer.valid()) {
            int r = _buffer[_start];
            next();
            return r;
        }
        UInt8 nybble = _start & 0x0f;
        _start >>= 4;
        return (nybble < 10 ? nybble + '0' : nybble + 'A' - 10);
    }
    int getCodePoint()
    {
        static String overlongEncoding("Overlong encoding");
        static String codePointTooHigh("Code point too high");
        static String unexpectedSurrogate("Unexpected surrogate");

        int b0 = getByte();
        if (b0 < 0x80)
            return b0;
        if (b0 < 0xc0 || b0 >= 0xf8)
            throwUTF8Exception(true);
        CharacterSource start = *this;
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
    int get()
    {
        int c = getCodePoint();
        ++_column;
        if (c == 10) {
            CharacterSource s = *this;
            if (s.getCodePoint() == 13)
                *this = s;
            _column = 0;
            ++_line;
            return 10;
        }
        if (c == 13) {
            CharacterSource s = *this;
            if (s.getCodePoint() == 10)
                *this = s;
            _column = 0;
            ++_line;
            return 10;
        }
        return c;
    }
    bool empty() const { return _length == 0; }
    void assert(int codePoint)
    {
        CharacterSource start = *this;
        int found = get();
        if (found == codePoint)
            return;
        start.throwUnexpected(String::codePoint(codePoint), String::codePoint(found));
    }
    int line() const { return _line; }
    int column() const { return _column; }
    void throwUnexpected(const String& expected, const String& observed)
    {
        static String expectedMessage("Expected ");
        static String found(", found ");
        throwError(expectedMessage + expected + found + observed);
    }
    void throwError(const String& message)
    {
        static String openBracket("(");
        static String comma(",");
        static String closeBracket("): ");
        String s = _string.subString(_offset, 1);
        throw Exception(s._implementation->buffer().fileName() + openBracket + String::decimal(_line) + comma + String::decimal(_column) + closeBracket + message);
    }
private:
    void throwUTF8Exception(bool first)
    {
        static String expectedFirst("Expected 0x00..0x7F or 0xC0..0xF7, found 0x");
        static String expectedNext("Expected 0x80..0xBF, found 0x");
        String expected = first ? expectedFirst : expectedNext;
        static String endOfString("end of string");
        String s = _length > 0 ? String::hexadecimal(_buffer[_start], 2) : endOfString;
        throwUTF8Exception(expected + s);
    }
    void throwUTF8Exception(String message)
    {
        static String at(" at ");
        static String in(" in ");
        String s = _string.subString(_offset, 1);
        throw Exception(message + at + String::hexadecimal(s._implementation->offset(), 8) + in + s._implementation->buffer().fileName());
    }
    int getNextByte()
    {
        if (empty())
            throwUTF8Exception(false);
        int b = getByte();
        if (b < 0x80 || b >= 0xc0)
            throwUTF8Exception(false);
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
    int _line;
    int _column;
};

template<class T> inline CharacterSource StringTemplate<T>::start() const { return CharacterSource(*this); }

template<class T> class HandleTemplate : Uncopyable
{
public:
#ifdef _WIN32
    HandleTemplate() : _handle(INVALID_HANDLE_VALUE) { }
    HandleTemplate(HANDLE handle, const String& name) : _handle(handle), _name(name) { }
    operator HANDLE() const { return _handle; }
    bool valid() const { return _handle != INVALID_HANDLE_VALUE; }
    static Handle consoleOutput()
    {
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        if (h == INVALID_HANDLE_VALUE || h == NULL) {
            static String openingConsole("Getting console handle ");
            Exception::throwSystemError(openingConsole);
        }
        static String console("console");
        return Handle(h, console);
    }
#else
    HandleTemplate() : _fileDescriptor(-1) { }
    HandleTemplate(int fileDescriptor) : _fileDescriptor(fileDescriptor) { }
    operator int() const { return _fileDescriptor; }
    bool valid() const { return _fileDescriptor != -1; }
    static Handle consoleOutput()
    {
        static String console("console");
        return Handle(STDOUT_FILENO, console);
    }
#endif
    String name() const { return _name; }
private:
#ifdef _WIN32
    HANDLE _handle;
#else
    int _fileDescriptor;
#endif
    String _name;
};

class AutoHandle : public Handle
{
public:
    AutoHandle() { }
#ifdef _WIN32
    AutoHandle(HANDLE handle, const String& name) : Handle(handle, name) { }
    ~AutoHandle() { if (valid()) CloseHandle(*this); }
#else
    AutoHandle(int fileDescriptor) : Handle(fileDescriptor) { }
    ~AutoHandle() { if (valid()) close(*this); }
#endif
};

#ifdef _WIN32
template<class T> class LocalStringTemplate : Uncopyable
{
public:
    LocalStringTemplate() : _str(NULL) { }
    ~LocalStringTemplate() { LocalFree(_str); }
    LPWSTR* operator&() { return &_str; }
    String string() { return String(_str); }
    operator LPWSTR() { return _str; }
private:
    LPWSTR _str;
};
#endif

template<class T> class ExceptionTemplate
{
public:
    ExceptionTemplate(const String& message) : _message(message) { }
    void write(const Handle& handle) const { _message.write(handle); }

    static void throwSystemError(const String& message)
    {
        String m;
#ifdef _WIN32
        m = messageForSystemCode(GetLastError());
#else
        m = String(strerror(errno));
#endif
        static String colon(" : ");
        throw Exception(message + colon + m);
    }
    static void throwOutOfMemory()
    {
#ifdef _WIN32
        throw Exception(messageForSystemCode(E_OUTOFMEMORY));
#else
        throw Exception(strerror(ENOMEM));
#endif
    }
    static void throwUnknown()
    {
#ifdef _WIN32
        throw Exception(messageForSystemCode(E_FAIL));
#else
        static String unspecifiedError("Unspecified error");
        throw Exception(unspecifiedError);
#endif
    }
private:
#ifdef _WIN32
    static String messageForSystemCode(DWORD error)
    {
        if (error == 0) {
            // If there was really no error we wouldn't be here. Avoid emitting
            // messages like "Error: Success"
            error = E_FAIL;
        }
        LocalString strMessage;
        DWORD formatted = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            error,
            0,
            reinterpret_cast<LPWSTR>(&strMessage),
            0,
            NULL);
        if (formatted == 0) {
            error = GetLastError();
            static String formatMessageFailed("FormatMessage failed: 0x");
            return formatMessageFailed + String::hexadecimal(error, 8);
        }
        return strMessage.string();
    }
#endif

    String _message;
};

#define BEGIN_CHECKED \
    try { \
        try

#define END_CHECKED \
        catch(std::bad_alloc&) { \
            Exception::throwOutOfMemory(); \
        } \
        catch(std::exception&) { \
            Exception::throwUnknown(); \
        } \
    } \
    catch

#endif // INCLUDED_STRING_H
