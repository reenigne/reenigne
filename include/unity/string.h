#ifndef INCLUDED_STRING_H
#define INCLUDED_STRING_H

#define CODE_MACRO(x) do { x } while (false)

#define IF_TRUE_THROW(expr,exception) CODE_MACRO( \
    if (expr) \
        throw exception; \
)

#define IF_FALSE_THROW(expr) \
    IF_TRUE_THROW(!(expr), Exception::systemError())
#define IF_MINUS_ONE_THROW(expr) \
    IF_TRUE_THROW((expr) == -1, Exception::systemError())
#define IF_ZERO_THROW(expr) \
    IF_TRUE_THROW((expr) == 0, Exception::systemError())
#define IF_NULL_THROW(expr) \
    IF_TRUE_THROW((expr) == NULL, Exception::systemError())
#define IF_NONZERO_THROW(expr) \
    IF_TRUE_THROW((expr) != 0, Exception::systemError())

#define IF_ZERO_CHECK_THROW_LAST_ERROR(expr) CODE_MACRO( \
    if ((expr) == 0) \
        IF_FALSE_THROW(GetLastError() == 0); \
)

#define BEGIN_CHECKED \
    try { \
        try

#define END_CHECKED \
        catch(std::bad_alloc&) { \
            throw Exception::outOfMemory(); \
        } \
        catch(std::exception&) { \
            throw Exception::unknown(); \
        } \
    } \
    catch

template<class T> class StringTemplate;
typedef StringTemplate<void> String;

class StringImplementation;

template<class T> class SimpleStringImplementationTemplate;
typedef SimpleStringImplementationTemplate<void> SimpleStringImplementation;

template<class T> class PaddingStringImplementationTemplate;
typedef PaddingStringImplementationTemplate<void> PaddingStringImplementation;

template<class T> class ConcatenatedStringImplementationTemplate;
typedef ConcatenatedStringImplementationTemplate<void>
    ConcatenatedStringImplementation;

template<class T> class RepeatedStringImplementationTemplate;
typedef RepeatedStringImplementationTemplate<void>
    RepeatedStringImplementation;

class HexadecimalStringImplementation;
class DecimalStringImplementation;
class CodePointStringImplementation;

template<class T> class HandleTemplate;
typedef HandleTemplate<void> Handle;

template<class T> class ExceptionTemplate;
typedef ExceptionTemplate<void> Exception;

#ifdef _WIN32
template<class T> class LocalStringTemplate;
typedef LocalStringTemplate<void> LocalString;
#endif

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#ifdef _WINDOWS
#define UTF16_MESSAGES
#endif
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
#include "unity/buffer.h"

template<class T> class StringTemplate
{
public:
    StringTemplate() : _implementation(_emptyImplementation) { }
    StringTemplate(const char* data)
        : _implementation(
            new SimpleStringImplementation(
                reinterpret_cast<const UInt8*>(data), 0, strlen(data)))
    { }
    StringTemplate(const Buffer& buffer, int start, int n)
        : _implementation(new SimpleStringImplementation(buffer, start, n)) { }
#ifdef _WIN32
    static int countBytes(const WCHAR* utf16)
    {
        int n = 0;
        while (true) {
            int c = *(utf16++);
            if (c == 0)
                break;
            if (c >= 0xdc00 && c < 0xe000)
                throw Exception(String("Expected 0x0000..0xD800 or "
                    "0xE000..0xFFFF, found 0x") + String::hexadecimal(c, 4));
            if (c >= 0xd800 && c < 0xdc00) {
                int c2 = *(utf16);
                if (c2 < 0xdc00 || c2 >= 0xe000)
                    throw Exception(
                        String("Expected 0xDC00..0xDFFF, found 0x") +
                        String::hexadecimal(c2, 4));
                ++n;
                continue;
            }
            if (c >= 0x800)
                ++n;
            if (c >= 0x80)
                ++n;
            ++n;
        }
        return n;
    }
    static UInt8* addToBuffer(const WCHAR* utf16, UInt8* p)
    {
        int i = 0;
        while (true) {
            int codePoint = *(utf16++);
            if (codePoint == 0)
                break;
            if (codePoint < 0x80)
                *(p++) = codePoint;
            else {
                if (codePoint < 0x800)
                    *(p++) = 0xc0 | (codePoint >> 6);
                else {
                    if (codePoint >= 0xd800 && codePoint < 0xdc00) {
                        codePoint = (((codePoint & 0x3ff)<<10) |
                            ((*(utf16++)) & 0x3ff)) + 0x10000;
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
        return p;
    }

    StringTemplate(const WCHAR* utf16)
    {
        Reference<OwningBufferImplementation> bufferImplementation =
            new OwningBufferImplementation();
        int n = countBytes(utf16);
        bufferImplementation->allocate(n);
        addToBuffer(utf16, bufferImplementation->data());
        _implementation =
            new SimpleStringImplementation(Buffer(bufferImplementation), 0, n);
    }
#endif
    static String hexadecimal(UInt32 value, int length)
    {
        return String(new HexadecimalStringImplementation(value, length));
    }
    static String decimal(SInt32 value)
    {
        return String(new DecimalStringImplementation(value));
    }
    static String codePoint(int codePoint)
    {
        return String(new CodePointStringImplementation(codePoint));
    }
    String subString(int start, int length) const
    {
        if (length == 0)
            return String();
        return String(_implementation->subString(start, length));
    }
    const String& operator+=(const String& other)
    {
        if (length() == 0) {
            *this = other;
            return *this;
        }
        if (other.length() == 0)
            return *this;
        {
            const SimpleStringImplementation* l =
                dynamic_cast<const SimpleStringImplementation*>(
                    static_cast<const StringImplementation*>(_implementation));
            const SimpleStringImplementation* r =
                dynamic_cast<const SimpleStringImplementation*>(
                    static_cast<const StringImplementation*>(
                        other._implementation));
            if (l != 0 && r != 0 && l->buffer() == r->buffer() &&
                l->offset() + l->length() == r->offset()) {
                _implementation = new SimpleStringImplementation(l->buffer(),
                    l->offset(), l->length() + r->length());
                return *this;
            }
        }
        if (*this == other) {
            _implementation = new RepeatedStringImplementation(*this, 2);
            return *this;
        }
        {
            const RepeatedStringImplementation* l =
                dynamic_cast<const RepeatedStringImplementation*>(
                    static_cast<const StringImplementation*>(_implementation));
            const RepeatedStringImplementation* r =
                dynamic_cast<const RepeatedStringImplementation*>(
                    static_cast<const StringImplementation*>(
                        other._implementation));
            if (l != 0 && r != 0 && l->_string == r->_string) {
                _implementation = new RepeatedStringImplementation(l->_string,
                    l->_count + r->_count);
                return *this;
            }
            if (l == 0 && r != 0 && *this == r->_string) {
                _implementation = new RepeatedStringImplementation(r->_string,
                    1 + r->_count);
                return *this;
            }
            if (r == 0 && l != 0 && other == l->_string) {
                _implementation = new RepeatedStringImplementation(l->_string,
                    1 + l->_count);
                return *this;
            }
        }
        _implementation = new ConcatenatedStringImplementation(_implementation,
            other._implementation);
        return *this;
    }
    const String& operator*=(int n)
    {
        if (n == 0 || length() == 0) {
            *this = String(_emptyImplementation);
            return *this;
        }
        if (n == 1)
            return *this;
        const RepeatedStringImplementation* r =
            dynamic_cast<const RepeatedStringImplementation*>(
                static_cast<const StringImplementation*>(_implementation));
        if (r != 0) {
            _implementation =
                new RepeatedStringImplementation(r->_string, n*r->_count);
            return *this;
        }
        _implementation = new RepeatedStringImplementation(*this, n);
        return *this;
    }
    String operator+(const String& other) const
    {
        String t = *this;
        t += other;
        return t;
    }
    String operator*(int n) const
    {
        String t = *this;
        t *= n;
        return t;
    }
    void copyTo(Array<UInt8>* data) const
    {
        int l = length();
        data->allocate(l + 1);
        _implementation->copyTo(&(*data)[0]);
        (*data)[l] = 0;
    }
#ifdef _WIN32
    void copyToUTF16(Array<WCHAR>* data) const
    {
        CharacterSource start(*this, String());
        CharacterSource s = start;
        int l = 1;
        do {
            int c = s.get();
            if (c == -1)
                break;
            ++l;
            if (c >= 0x10000)
                ++l;
        } while (true);
        s = start;
        data->allocate(l);
        WCHAR* d = &(*data)[0];
        l = 0;
        do {
            int c = s.get();
            if (c == -1)
                break;
            if (c >= 0x10000) {
                c -= 0x10000;
                d[l++] = 0xd800 + ((c >> 10) & 0x03ff);
                d[l++] = 0xdc00 + (c & 0x03ff);
            }
            else {
                d[l++] = c;
            }
        } while (true);
        d[l++] = 0;
    }
#endif
    int hash() const { return _implementation->hash(0); }
    bool operator==(const String& other) const
    {
        if (_implementation == other._implementation)
            return true;
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
        int c = _implementation->compare(0, other._implementation, 0,
            min(l, otherLength));
        if (c != 0)
            return c < 0;
        return l < otherLength;
    }
    bool operator>(const String& other) const { return other < *this; }
    bool operator<=(const String& other) const { return !operator>(other); }
    bool operator>=(const String& other) const { return !operator<(other); }
    UInt8 operator[](int offset) const
    {
        return _implementation->byteAt(offset);
    }
    int length() const { return _implementation->length(); }
    bool empty() const { return length() == 0; }
    void write(const Handle& handle) const { _implementation->write(handle); }

    void initSimpleData(int offset, Buffer* buffer, int* start, int* length)
        const
    {
        _implementation->initSimpleData(offset, buffer, start, length);
    }

    StringTemplate(const ConstReference<StringImplementation>& implementation)
      : _implementation(implementation) { }
    ConstReference<StringImplementation> implementation()
    {
        return _implementation; 
    }

    String alignRight(int n)
    {
        int spaces = n - length();
        if (spaces > 0)
            return space*spaces + (*this);
        return *this;
    }
    String alignLeft(int n)
    {
        int spaces = n - length();
        if (spaces > 0)
            return (*this) + space*spaces;
        return *this;
    }

private:

    ConstReference<StringImplementation> _implementation;

    template<class T> friend class CharacterSourceTemplate;
    template<class T> friend class RepeatedStringImplementationTemplate;

    static ConstReference<StringImplementation> _emptyImplementation;
};

class NullTerminatedString
{
public:
    NullTerminatedString(String s)
    {
        s.copyTo(&_buffer);
    }
    operator const char*()
    {
        return reinterpret_cast<const char*>(&_buffer[0]);
    }
private:
    Array<UInt8> _buffer;
};

#ifdef _WIN32
class NullTerminatedWideString
{
public:
    NullTerminatedWideString(String s)
    {
        s.copyToUTF16(&_buffer);
    }
    operator const WCHAR*()
    {
        return &_buffer[0];
    }
private:
    Array<WCHAR> _buffer;
};
#endif

class StringImplementation : public ReferenceCounted
{
public:
    StringImplementation() : _length(0) { }
    int length() const { return _length; }
    virtual const StringImplementation* subString(int start, int length) const
        = 0;
    virtual void copyTo(UInt8* buffer) const = 0;
    virtual int hash(int h) const = 0;

    // works like memcmp(this+start, other+otherStart, l) - returns 1 if "this"
    // is greater than "other".
    virtual int compare(int start, const StringImplementation* other,
        int otherStart, int l) const = 0;

    // works like memcmp(this+start, data, l) - returns 1 if "this" is greater
    // than "other".
    virtual int compare(int start, const UInt8* data, int l) const = 0;  

    virtual UInt8 byteAt(int offset) const = 0;
    virtual Buffer buffer() const = 0;
    virtual int offset() const = 0;
    virtual void initSimpleData(int offset, Buffer* buffer, int* start,
        int* length) const = 0;
    virtual void write(const Handle& handle) const = 0;
protected:
    void setLength(int length) { _length = length; }
private:
    int _length;
};

template<class T> class SimpleStringImplementationTemplate
  : public StringImplementation
{
public:
    StringImplementation* subString(int start, int length) const
    {
        return new SimpleStringImplementation(_buffer, _start + start, length);
    }
    SimpleStringImplementationTemplate(const UInt8* data, int start,
        int length) : _buffer(data), _start(start)
    {
        setLength(length);
    }
    SimpleStringImplementationTemplate(const Buffer& buffer, int start,
        int length) : _buffer(buffer), _start(start)
    {
        setLength(length);
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
    int compare(int start, const StringImplementation* other, int otherStart,
        int l) const
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
        if (length() == 0)
            return;
        handle.write(static_cast<const void*>(_buffer.data() + _start),
            length());
    }
protected:
    Buffer _buffer;
    int _start;
};

ConstReference<StringImplementation> String::_emptyImplementation =
    new SimpleStringImplementation(Buffer(), 0, 0);

template<class T> class RepeatedStringImplementationTemplate
  : public StringImplementation
{
public:
    RepeatedStringImplementationTemplate(String string, int count)
      : _string(string), _count(count)
    {
        setLength(count*string.length());
    }
    const StringImplementation* subString(int start, int l) const
    {
        int sl = _string.length();
        int s = start % sl;
        int ll = sl - s;
        if (l < ll)
            ll = l;
        String r = _string.subString(s, ll);
        start += ll;
        l -= ll;

        r += _string * (l / sl);
        l %= sl;

        return (r + _string.subString(0, l))._implementation;
    }
    void copyTo(UInt8* buffer) const
    {
        for (int i = 0; i < length(); ++i) {
            _string._implementation->copyTo(buffer);
            buffer += _string.length();
        }
    }
    int hash(int h) const
    {
        for (int i = 0; i < length(); ++i)
            h = _string._implementation->hash(h);
        return h;
    }
    int compare(int start, const StringImplementation* other, int otherStart,
        int l) const
    {
        int sl = _string.length();
        int s = start % sl;
        int ll = sl - s;
        if (l < ll)
            ll = l;
        int r = _string._implementation->compare(s, other, otherStart, ll);
        if (r != 0)
            return r;
        start += ll;
        l -= ll;
        otherStart += ll;
        while (l >= sl) {
            r = _string._implementation->compare(0, other, otherStart, sl);
            if (r != 0)
                return r;
            l -= sl;
            otherStart += sl;
        }
        if (l == 0)
            return 0;
        return _string._implementation->compare(0, other, otherStart, l);
    }
    int compare(int start, const UInt8* data, int l) const
    {
        int sl = _string.length();
        int s = start % sl;
        int ll = sl - s;
        if (l < ll)
            ll = l;
        int r = _string._implementation->compare(s, data, ll);
        if (r != 0)
            return r;
        start += ll;
        l -= ll;
        data += ll;
        while (l >= sl) {
            r = _string._implementation->compare(0, data, sl);
            if (r != 0)
                return r;
            l -= sl;
            data += sl;
        }
        if (l == 0)
            return 0;
        return _string._implementation->compare(0, data, l);
    }
    UInt8 byteAt(int offset) const
    {
        return _string._implementation->byteAt(offset % _count);
    }
    Buffer buffer() const { return _string._implementation->buffer(); }
    int offset() const { return 0; }
    void initSimpleData(int offset, Buffer* buf, int* start, int* l) const
    {
        _string.initSimpleData(offset % _string.length(), buf, start, l);
    }
    void write(const Handle& handle) const
    {
        for (int i = 0; i < _count; ++i)
            _string.write(handle);
    }
private:
    String _string;
    int _count;

    friend class StringTemplate<T>;
};

template<class T> class ConcatenatedStringImplementationTemplate
  : public StringImplementation
{
public:
    ConcatenatedStringImplementationTemplate(
        const ConstReference<StringImplementation>& left,
        const ConstReference<StringImplementation>& right)
      : _left(left), _right(right)
    {
        setLength(_left->length() + _right->length());
    }
    const StringImplementation* subString(int start, int length) const
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
    int compare(int start, const StringImplementation* other, int otherStart,
        int l) const
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
    void initSimpleData(int offset, Buffer* buffer, int* start, int* length)
        const
    {
        int leftLength = _left->length();
        if (offset < leftLength)
            return _left->initSimpleData(offset, buffer, start, length);
        return _right->initSimpleData(offset - leftLength, buffer, start,
            length);
    }
    void write(const Handle& handle) const
    {
        _left->write(handle);
        _right->write(handle);
    }
private:
    ConstReference<StringImplementation> _left;
    ConstReference<StringImplementation> _right;
};

template<int N> class FixedStringImplementation
  : public SimpleStringImplementation
{
public:
    FixedStringImplementation(int start, int length)
      : SimpleStringImplementation(Buffer(), start, length),
        _bufferImplementation(_data)
    {
        _bufferImplementation.addReference();
        _buffer = Buffer(&_bufferImplementation);
    }
protected:
    NonOwningBufferImplementation _bufferImplementation;
    UInt8 _data[N];
};

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

String empty("");

template<class T> class HandleTemplate : Uncopyable
{
public:
#ifdef _WIN32
    HandleTemplate() : _handle(INVALID_HANDLE_VALUE), _name(empty) { }
    HandleTemplate(HANDLE handle, const String& name = empty)
      : _handle(handle), _name(name)
    { }
    operator HANDLE() const { return _handle; }
    bool valid() const { return _handle != INVALID_HANDLE_VALUE; }
    void set(HANDLE handle, const String& name = empty)
    {
        _handle = handle;
        _name = name;
    }
#else
    HandleTemplate() : _fileDescriptor(-1), _name(empty) { }
    HandleTemplate(int fileDescriptor, const String& name = empty)
      : _fileDescriptor(fileDescriptor), _name(name)
    { }
    operator int() const { return _fileDescriptor; }
    bool valid() const { return _fileDescriptor != -1; }
    void set(int fileDescriptor, const String& name = empty)
    {
        _fileDescriptor = fileDescriptor;
        _name = name;
    }
#endif
    String name() const { return _name; }
    // Be careful using the template read() and write() functions with types
    // other than single bytes and arrays thereof - they are not endian-safe.
    template<class U> void write(const U& value) const
    {
        write(static_cast<const void*>(&value), sizeof(U));
    }
    void write(const String& s) const { s.write(*this); }
    void write(const Exception& e) const { e.write(*this); }
    void write(const void* buffer, int bytes) const
    {
        if (bytes == 0)
            return;
        static String writingFile("Writing file ");
#ifdef _WIN32
        DWORD bytesWritten;
        if (WriteFile(_handle, buffer, bytes, &bytesWritten, NULL) == 0 ||
            bytesWritten != bytes)
            throw Exception::systemError(writingFile + _name);
#else
        ssize_t writeResult = write(_fileDescriptor, buffer, bytes);
        if (writeResult < length())
            throw Exception::systemError(writingFile + _name);
#endif
    }
    template<class U> U read()
    {
        U value;
        read(static_cast<void*>(&value), sizeof(U));
        return value;
    }
    int tryReadByte()
    {
        Byte b;
        static String readingFile("Reading file ");
#ifdef _WIN32
        DWORD bytesRead;
        if (ReadFile(_handle, static_cast<void*>(&b), 1, &bytesRead, NULL)
            == 0) {
            if (GetLastError() == ERROR_HANDLE_EOF)
                return -1;
            throw Exception::systemError(readingFile + _name);
        }
        if (bytesRead != 1)
            return -1;
#else
        ssize_t readResult = read(_fileDescriptor, static_cast<void*>(&b), 1);
        if (readResult < 1) {
            if (_eof(_fileDescriptor))
                return -1;
            throw Exception::systemError(readingFile + _name);
        }
#endif
        return b;
    }
    void read(void* buffer, int bytes) const
    {
        if (bytes == 0)
            return;
        static String readingFile("Reading file ");
#ifdef _WIN32
        DWORD bytesRead;
        if (ReadFile(_handle, buffer, bytes, &bytesRead, NULL) == 0)
            throw Exception::systemError(readingFile + _name);
        if (bytesRead != bytes) {
            static String endOfFile("End of file reading file ");
            throw Exception(endOfFile + _name);
        }
#else
        ssize_t readResult = read(_fileDescriptor, buffer, bytes);
        if (readResult < bytes)
            throw Exception::systemError(readingFile + _name);
#endif
    }
private:
#ifdef _WIN32
    HANDLE _handle;
#else
    int _fileDescriptor;
#endif
    String _name;
};

template<class T> class ExceptionTemplate
{
public:
#ifdef _WIN32
    ExceptionTemplate()
      : _message(messageFromErrorCode(E_FAIL)),
        _implementation(new OwningImplementation(_message))
    { }
#else
    ExceptionTemplate()
      : _message("Unspecified error"),
        _implementation(new StaticImplementation(_message))
    { }
#endif
    ExceptionTemplate(const String& message)
      : _message(message),
        _implementation(new OwningImplementation(message))
    { }
    void write(const Handle& handle) const
    {
        (_message + newLine).write(handle);
    }
    static Exception systemError(const String& message = "")
    {
        String m;
#ifdef _WIN32
        m = messageFromErrorCode(GetLastError());
#else
        m = String(strerror(errno));
#endif
        if (message.length() == 0)
            return Exception(m);
        static String colon(" : ");
        return Exception(message + colon + m);
    }
    static Exception outOfMemory()
    {
#ifdef _WIN32
        return Exception(messageFromErrorCode(E_OUTOFMEMORY));
#else
        return Exception(strerror(ENOMEM));
#endif
    }
    static Exception unknown() { return Exception(); }
    String message() const { return _message; }

#ifdef _WIN32
    static Exception fromErrorCode(DWORD error)
    {
        return Exception(messageFromErrorCode(error));
    }
#else
    static Exception fromErrorCode(int error)
    {
        return Exception(strerror(error));
    }
#endif
private:
    class Implementation : public ReferenceCounted
    {
    public:
#ifdef UTF16_MESSAGES
        virtual const WCHAR* message() = 0;
#else
        virtual const char* message() = 0;
#endif
    };
    class OwningImplementation : public Implementation
    {
    public:
        OwningImplementation(String string) : _string(string) { }
#ifdef UTF16_MESSAGES
        const WCHAR* message() { return _string; }
#else
        const char* message() { return _string; }
#endif
    private:
#ifdef UTF16_MESSAGES
        NullTerminatedWideString _string;
#else
        NullTerminatedString _string;
#endif
    };
    class StaticImplementation : public Implementation
    {
#ifdef UTF16_MESSAGES
    public:
        StaticImplementation(const WCHAR* string) : _string(string) { }
        const WCHAR* message() { return _string; }
    private:
        const WCHAR* _string;
#else
    public:
        StaticImplementation(const char* string) : _string(string) { }
        const char* message() { return _string; }
    private:
        const char* _string;
#endif
    };
#ifdef _WIN32
    static String messageFromErrorCode(DWORD error)
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
    Reference<Implementation> _implementation;
};

class AutoHandle : public Handle
{
public:
    AutoHandle() { }
#ifdef _WIN32
    AutoHandle(HANDLE handle, const String& name = empty)
      : Handle(handle, name)
    { }
    ~AutoHandle() { if (valid()) CloseHandle(operator HANDLE()); }
#else
    AutoHandle(int fileDescriptor, const String& name = empty)
      : Handle(fileDescriptor, name)
    { }
    ~AutoHandle() { if (valid()) close(operator int()); }
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

String openParenthesis = String::codePoint('(');
String closeParenthesis = String::codePoint(')');
String comma = String::codePoint(',');
String commaSpace(", ");
String asterisk = String::codePoint('*');
String space = String::codePoint(' ');
String newLine = String::codePoint(10);
String backslash = String::codePoint('\\');
String colon = String::codePoint(':');
String openBracket = String::codePoint('[');
String closeBracket = String::codePoint(']');
String colonSpace(": ");
String tab = String::codePoint(9);
String doubleQuote = String::codePoint('"');
String dollar = String::codePoint('$');
String singleQuote = String::codePoint('\'');
String backQuote = String::codePoint('`');
String lessThan = String::codePoint('<');
String greaterThan = String::codePoint('>');
String dot = String::codePoint('.');

#include "unity/character_source.h"

#endif // INCLUDED_STRING_H
