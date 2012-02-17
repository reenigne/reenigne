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
#include <limits>
#include <string.h>
#include "unity/integer_types.h"
#include "unity/reference_counted.h"
#include "unity/array.h"
#include "unity/minimum_maximum.h"

template<class T> class ExceptionTemplate;
typedef ExceptionTemplate<void> Exception;

template<class T> class StringTemplate;
typedef StringTemplate<void> String;

template<class T> class StringTemplate
{
public:
    class Hex
    {
    public:
        Hex(int n, int digits, bool ox) : _n(n), _digits(digits), _ox(ox) { }
        String operator+(const char* a)
        {
            String s(bytes() + strlen(a));
            s += *this;
            s += a;
            return s;
        }
    private:
        int bytes() const { return _digits + _ox ? 2 : 0; }
        void write(Byte* destination) const
        {
            if (_ox) {
                *destination = '0';
                ++destination;
                *destination = 'x';
                ++destination;
            }
            for (int i = 0; i < _digits; ++i) {
                int d = (_n >> ((_digits - i - 1) << 2)) & 0xf;
                *destination = d < 10 ? d + '0' : d + 'A' - 10;
                ++destination;
            }
        }

        int _n;
        int _digits;
        bool _ox;
        friend class StringTemplate;
    };

    class CodePoint
    {
    public:
        CodePoint(int c) : _c(c) { }
        String operator+(const char* a)
        {
            String s(bytes() + strlen(a));
            s += *this;
            s += a;
            return s;
        }
    private:
        int bytes() const
        {
            if (_c < 0x80)
                return 1;
            if (_c < 0x800)
                return 2;
            if (_c < 0x10000)
                return 3;
            return 4;
        }
        void write(Byte* destination) const
        {
            if (_c < 0x80) {
                *destination = _c;
                return;
            }
            if (_c < 0x800) {
                *destination = 0xc0 | (_c >> 6);
                ++destination;
            }
            else {
                if (_c < 0x10000) {
                    *destination = 0xe0 | (_c >> 12);
                    ++destination;
                }
                else {
                    *destination = 0xf0 | (_c >> 18);
                    ++destination;
                    *destination = 0x80 | ((_c >> 12) & 0x3f);
                    ++destination;
                }
                *destination = 0x80 | ((_c >> 6) & 0x3f);
                ++destination;
            }
            *destination = 0x80 | (_c & 0x3f);
        }
        int _c;
        friend class StringTemplate;
    };

    class Decimal
    {
    public:
        Decimal(int n) : _n(n) { }
    private:
        int bytes() const
        {
            int l = 1;
            int n = _n;
            if (n < 0) {
                if (n == std::numeric_limits<int>::min())
                    n = 1 - n;
                else
                    n = -n;
                ++l;
            }
            while (n >= 10) {
                ++l;
                n /= 10;
            }
            return l;
        }
        void write(Byte* destination) const
        {
            int n = _n;
            int l = bytes();
            bool intMin = false;
            if (n < 0) {
                if (n == std::numeric_limits<int>::min()) {
                    n = 1 - n;
                    intMin = true;
                }
                else
                    n = -n;
                *destination = '-';
                ++destination;
                --l;
            }
            int d = n % 10;
            if (intMin)
                ++d;
            --l;
            destination[l] = d + '0';
            n /= 10;
            while (n > 0) {
                --l;
                destination[l] = (n % 10) + '0';
                n /= 10;
            }
        }

        int _n;
        friend class StringTemplate;
    };

    StringTemplate() : _start(0), _length(0) { }
    StringTemplate(const char* data)
      : _buffer(data), _start(0), _length(strlen(data)) { }
    explicit StringTemplate(int length)
      : _buffer(length), _start(0), _length(0) { }
#ifdef _WIN32
    StringTemplate(const WCHAR* utf16) : _start(0)
    {
        int l = bytes(utf16);
        _buffer = Buffer(l);
        write(data(), utf16);
        _length = l;
    }
#endif
    StringTemplate(const Hex& hex) : _start(0)
    {
        int l = hex.bytes();
        _buffer = Buffer(l);
        hex.write(data());
        _length = l;
    }
    StringTemplate(const CodePoint& c)
    {
        int l = c.bytes();
        _buffer = Buffer(l);
        c.write(data());
        _length = l;
    }
    StringTemplate(const Decimal d)
    {
        int l = d.bytes();
        _buffer = Buffer(l);
        d.write(data());
        _length = l;
    }
    const String& operator+=(const String& other)
    {
        if (other.empty())
            return *this;
        int l = other._length;
        expand(l);
        _buffer.expand(other.data(), l);
        _length += l;
        return *this;
    }
    String operator+(const String& other) const
    {
        String s(*this);
        s += other;
        return s;
    }
    const String& operator+=(int d)
    {
        Decimal dd(d);
        int l = dd.bytes();
        expand(l);
        _buffer.expand(l);
        dd.write(data() + _length);
        _length += l;
        return *this;
    }
    String operator+(int d) const { String s(*this); s += d; return s; }
#ifdef _WIN32
    const String& operator+=(const WCHAR* utf16)
    {
        int l = bytes(utf16);
        expand(l);
        _buffer.expand(l);
        write(data() + _length, utf16);
        _length += l;
        return *this;
    }
    String operator+(const WCHAR* utf16) const
    {
        String s(*this);
        s += utf16;
        return s;
    }
#endif
    const String& operator+=(const Hex& hex)
    {
        int l = hex.bytes();
        expand(l);
        _buffer.expand(l);
        hex.write(data() + _length);
        _length += l;
        return *this;
    }
    String operator+(const Hex& hex) const
    {
        String s(*this);
        s += hex;
        return s;
    }
    const String& operator+=(const CodePoint& c)
    {
        int l = c.bytes();
        expand(l);
        _buffer.expand(l);
        c.write(data() + _length);
        _length += l;
        return *this;
    }
    String operator+(const CodePoint& c) const
    {
        String s(*this);
        s += c;
        return s;
    }
    const String& operator*=(int n)
    {
        int l = _length*n;
        expand(l);
        _buffer.expand(l);
        Byte* source = data();
        Byte* destination = source + _length;
        for (int i = 1; i < n; ++i) {
            memcpy(destination, source, _length);
            destination += _length;
        }
        _length += l;
        return *this;
    }
    String operator*(int n) { String s(*this); s *= n; return s; }

    bool empty() const { return _length == 0; }
    int length() const { return _length; }
    const Byte* data() const { return _buffer.data() + _start; }
    String subString(int start, int length) const
    {
        if (length == 0)
            return String();
        return String(_buffer, _start + start, length);
    }
    void operator++() { ++_start; --_length; }
    Byte operator*() const { return *data(); }
    Byte operator[](int offset) const { return *(data() + offset); }
    int hash() const
    {
        int h = 0;
        const Byte* p = data();
        for (int i = 0; i < _length; ++i) {
            h = h * 67 + *p - 113;
            ++p;
        }
        return h;
    }
    bool operator==(const String& other) const
    {
        if (_length != other._length)
            return false;
        const Byte* a = data();
        const Byte* b = other.data();
        for (int i = 0; i < _length; ++i) {
            if (*a != *b)
                return false;
            ++a;
            ++b;
        }
        return true;
    }
    bool operator==(const char* b) const
    {
        const Byte* a = data();
        for (int i = 0; i < _length; ++i) {
            if (*b == 0)
                return false;
            if (*a != *b)
                return false;
            ++a;
            ++b;
        }
        return *b == 0;
    }
    bool operator!=(const String& other) const { return !operator==(other); }
    bool operator<(const String& other) const
    {
        const Byte* a = data();
        const Byte* b = other.data();
        for (int i = 0; i < min(_length, other._length); ++i) {
            if (*a < *b)
                return true;
            if (*a > *b)
                return false;
            ++a;
            ++b;
        }
        return _length < other._length;
    }
    bool operator>(const String& other) const { return other < *this; }
    bool operator<=(const String& other) const { return !operator>(other); }
    bool operator>=(const String& other) const { return !operator<(other); }

    String alignRight(int n)
    {
        int spaces = n - _length;
        if (spaces > 0)
            return String(" ")*spaces + (*this);
        return *this;
    }
    String alignLeft(int n)
    {
        int spaces = n - _length;
        if (spaces > 0)
            return (*this) + String(" ")*spaces;
        return *this;
    }
private:
    class Buffer
    {
    public:
        Buffer() { }
        Buffer(const char* data)
          : _implementation(new LiteralImplementation(data)) { }
        Buffer(int length) : _implementation(new OwningImplementation(length))
        { }
        int end() const
        {
            if (!_implementation.valid())
                return -1;
            return _implementation->end();
        }
        void expand(const Byte* data, int length)
        {
            _implementation->expand(data, length);
        }
        void expand(int length) { _implementation->expand(length); }
        const Byte* data() const { return _implementation->data(); }
        Byte* data() { return _implementation->data(); }
    private:
        class Implementation : public ReferenceCounted
        {
        public:
            virtual int end() const = 0;
            virtual void expand(const Byte* data, int length) = 0;
            virtual void expand(int length) = 0;
            virtual const Byte* data() const = 0;
            virtual Byte* data() = 0;
        };

        class OwningImplementation : public Implementation
        {
        public:
            OwningImplementation(int n) { _data.reserve(n); }
            int end() const { return _data.count(); }
            void expand(const Byte* data, int length)
            {
                _data.append(data, length);
            }
            void expand(int length) { _data.expand(length); }
            const Byte* data() const { return &_data[0]; }
            Byte* data() { return &_data[0]; }
        private:
            AppendableArray<Byte> _data;
        };

        class LiteralImplementation : public Implementation
        {
        public:
            LiteralImplementation(const char* data)
              : _data(reinterpret_cast<const Byte*>(data)) { }
            int end() const { return -1; }
            void expand(const Byte* data, int length) { throw Exception(); }
            void expand(int length) { throw Exception(); }
            const Byte* data() const { return _data; }
            Byte* data() { throw Exception(); }
        private:
            const Byte* _data;
        };

        Reference<Implementation> _implementation;
    };

    StringTemplate(const Buffer& buffer, int start, int length)
      : _buffer(buffer), _start(start), _length(length) { }
    bool expandable() const { return _buffer.end() == _length; }
    Byte* data() { return _buffer.data() + _start; }
#ifdef _WIN32
    static int bytes(const WCHAR* utf16)
    {
        int n = 0;
        int offset = 0;
        while (true) {
            int c = *utf16;
            ++utf16;
            if (c == 0)
                break;
            if (c >= 0xdc00 && c < 0xe000)
                throw Exception(String("String offset ") + hex(offset) +
                    ": expected 0x0000..0xD800 or 0xE000..0xFFFF, found " +
                    hex(c, 4));
            ++offset;
            if (c >= 0xd800 && c < 0xdc00) {
                int c2 = *utf16;
                ++utf16;
                if (c2 < 0xdc00 || c2 >= 0xe000)
                    throw Exception(String("String offset ") + hex(offset) +
                        ": expected 0xDC00..0xDFFF, found " + hex(c2, 4));
                ++offset;
                c = (((c & 0x3ff) << 10) | (c2 & 0x3ff)) + 0x10000;
            }
            n += CodePoint(c).bytes();
        }
        return n;
    }
    static void write(Byte* destination, const WCHAR* utf16)
    {
        int i = 0;
        while (true) {
            int c = *utf16;
            ++utf16;
            if (c == 0)
                break;
            if (c >= 0xd800 && c < 0xdc00) {
                c = (((c & 0x3ff) << 10) | ((*utf16) & 0x3ff)) + 0x10000;
                ++utf16;
            }
            CodePoint cp(c);
            cp.write(destination);
            destination += cp.bytes();
        }
    }
#endif
    void expand(int l)
    {
        if (!expandable()) {
            _buffer = Buffer(_length + l);
            _start = 0;
        }
    }

    Buffer _buffer;
    int _start;
    int _length;

    friend class NullTerminatedString;
    friend class ProgramBase;
    template<class T> friend class FileTemplate;
};

String operator+(const char* a, const String& b)
{
    String r(strlen(a) + b.length()); r += a; r += b; return r;
}

String::Hex hex(int n, int digits = 8, bool ox = true)
{
    return String::Hex(n, digits, ox);
}

String::Decimal decimal(int n) { return String::Decimal(n); }
String::CodePoint codePoint(int n) { return String::CodePoint(n); }

class Handle;

#ifdef _WIN32
template<class T> class NullTerminatedWideStringTemplate
{
public:
    NullTerminatedWideStringTemplate(String s)
    {
        CharacterSource start(s, "");
        CharacterSource cs = start;
        int l = 1;
        do {
            int c = cs.get();
            if (c == -1)
                break;
            ++l;
            if (c >= 0x10000)
                ++l;
        } while (true);
        cs = start;
        _buffer.allocate(l);
        WCHAR* d = &_buffer[0];
        do {
            int c = cs.get();
            if (c == -1)
                break;
            if (c >= 0x10000) {
                c -= 0x10000;
                *d = 0xd800 + ((c >> 10) & 0x03ff);
                ++d;
                *d = 0xdc00 + (c & 0x03ff);
                ++d;
            }
            else {
                *d = c;
                ++d;
            }
        } while (true);
        *d = 0;
    }
    operator const WCHAR*()
    {
        return &_buffer[0];
    }
private:
    Array<WCHAR> _buffer;
};

typedef NullTerminatedWideStringTemplate<void> NullTerminatedWideString;
#endif

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
        handle.write(_message + codePoint(10));
    }
    static Exception systemError(const String& message = String())
    {
        String m;
#ifdef _WIN32
        m = messageFromErrorCode(GetLastError());
#else
        m = String(strerror(errno));
#endif
        if (message.empty())
            return Exception(m);
        return Exception(message + " : " + m);
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
        if (formatted == 0)
            return String("FormatMessage failed: 0x") +
                hex(GetLastError(), 8);
        return strMessage.string();
    }
#endif
    String _message;
    Reference<Implementation> _implementation;
};

class NullTerminatedString
{
public:
    NullTerminatedString(String s) : _s(s + codePoint(0)) { }
    operator const char*()
    {
        return reinterpret_cast<const char*>(_s.data());
    }
private:
    String _s;
    Array<UInt8> _buffer;
};

class Handle : Uncopyable
{
public:
#ifdef _WIN32
    Handle() : _handle(INVALID_HANDLE_VALUE), _name("") { }
    Handle(HANDLE handle, const String& name = "")
      : _handle(handle), _name(name)
    { }
    operator HANDLE() const { return _handle; }
    bool valid() const { return _handle != INVALID_HANDLE_VALUE; }
    void set(HANDLE handle, const String& name = "")
    {
        _handle = handle;
        _name = name;
    }
#else
    Handle() : _fileDescriptor(-1) { }
    Handle(int fileDescriptor, const String& name = "")
      : _fileDescriptor(fileDescriptor), _name(name)
    { }
    operator int() const { return _fileDescriptor; }
    bool valid() const { return _fileDescriptor != -1; }
    void set(int fileDescriptor, const String& name = "")
    {
        _fileDescriptor = fileDescriptor;
        _name = name;
    }
#endif
    String name() const { return _name; }
    // Be careful using the template read() and write() functions with types
    // other than single bytes and arrays thereof - they are not endian-safe.
    void write(const char* string) const
    {
        write(static_cast<const void*>(string), strlen(string));
    }
    template<class U> void write(const U& value) const
    {
        write(static_cast<const void*>(&value), sizeof(U));
    }
    template<class U> void write(const Array<U>& value) const
    {
        write(static_cast<const void*>(&value[0]), value.count()*sizeof(U));
    }
    void write(const String& s) const { write(s.data(), s.length()); }
    void write(const Exception& e) const { e.write(*this); }
    void write(const void* buffer, int bytes) const
    {
        if (bytes == 0)
            return;
#ifdef _WIN32
        DWORD bytesWritten;
        if (WriteFile(_handle, buffer, bytes, &bytesWritten, NULL) == 0 ||
            bytesWritten != bytes)
            throw Exception::systemError(String("Writing file ") + _name);
#else
        ssize_t writeResult = write(_fileDescriptor, buffer, bytes);
        if (writeResult < length())
            throw Exception::systemError(String("Writing file ") + _name);
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
#ifdef _WIN32
        DWORD bytesRead;
        if (ReadFile(_handle, static_cast<void*>(&b), 1, &bytesRead, NULL)
            == 0) {
            if (GetLastError() == ERROR_HANDLE_EOF)
                return -1;
            throw Exception::systemError(String("Reading file ") + _name);
        }
        if (bytesRead != 1)
            return -1;
#else
        ssize_t readResult = read(_fileDescriptor, static_cast<void*>(&b), 1);
        if (readResult < 1) {
            if (_eof(_fileDescriptor))
                return -1;
            throw Exception::systemError(String("Reading file ") + _name);
        }
#endif
        return b;
    }
    void read(void* buffer, int bytes) const
    {
        if (bytes == 0)
            return;
#ifdef _WIN32
        DWORD bytesRead;
        if (ReadFile(_handle, buffer, bytes, &bytesRead, NULL) == 0)
            throw Exception::systemError(String("Reading file ") + _name);
        if (bytesRead != bytes)
            throw Exception(String("End of file reading file ") + _name);
#else
        ssize_t readResult = read(_fileDescriptor, buffer, bytes);
        if (readResult < bytes)
            throw Exception::systemError(String("Reading file ") + _name);
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

#ifdef _WIN32
class LocalString : Uncopyable
{
public:
    LocalString() : _str(NULL) { }
    ~LocalString() { LocalFree(_str); }
    LPWSTR* operator&() { return &_str; }
    String string() { return String(_str); }
    operator LPWSTR() { return _str; }
private:
    LPWSTR _str;
};
#endif

class AutoHandle : public Handle
{
public:
    AutoHandle() { }
#ifdef _WIN32
    AutoHandle(HANDLE handle, const String& name = "")
      : Handle(handle, name)
    { }
    ~AutoHandle() { if (valid()) CloseHandle(operator HANDLE()); }
#else
    AutoHandle(int fileDescriptor, const String& name = "")
      : Handle(fileDescriptor, name)
    { }
    ~AutoHandle() { if (valid()) close(operator int()); }
#endif
};

class Location
{
public:
    Location() : _line(0), _column(0), _offset(0) { }
    Location(String fileName)
      : _fileName(fileName), _line(1), _column(1), _offset(0)
    { }
    Location(String fileName, int line, int column)
      : _fileName(fileName), _line(line), _column(column), _offset(0)
    { }
    String toString() const
    {
        return _fileName + "(" + _line + "," + _column + ")";
    }
    String fileName() const { return _fileName; }
    void operator++() { ++_offset; }
    void advanceColumn() { ++_column; }
    void advanceLine() { _column = 1; ++_line; }
    void throwError(const String& message) const
    {
        throw Exception(toString() + ": " + message);
    }
    void throwErrorWithOffset(const String& message) const
    {
        throw Exception(toString() + ": " + message + "(offset " +
            hex(_offset) + ")");
    }
    int offset() const { return _offset; }
    int line() const { return _line; }
    int column() const { return _column; }
private:
    String _fileName;
    int _line;
    int _column;
    int _offset;
};

class Span
{
public:
    Span() : _startLine(-1) { }
    Span(Location start, Location end)
      : _fileName(start.fileName()),
        _startLine(start.line()),
        _startColumn(start.column()),
        _endLine(end.line()),
        _endColumn(end.column())
    { }
    Span(String fileName, int startLine, int startColumn, int endLine,
        int endColumn)
      : _fileName(fileName),
        _startLine(startLine),
        _startColumn(startColumn),
        _endLine(endLine),
        _endColumn(endColumn)
    { }
    String fileName() const { return _fileName; }
    int startLine() const { return _startLine; }
    int startColumn() const { return _startColumn; }
    int endLine() const { return _endLine; }
    int endColumn() const { return _endColumn; }
    String toString() const
    {
        return _fileName + "(" + _startLine + "," + _startColumn + ")-(" +
            _endLine + "," + _endColumn + ")";
    }
    void throwError(const String& message) const
    {
        throw Exception(toString() + ": " + message);
    }
    Location start() const
    {
        return Location(_fileName, _startLine, _startColumn);
    }
    Location end() const
    {
        return Location(_fileName, _endLine, _endColumn);
    }
    Span operator+(const Span& other) const
    {
        if (_startLine == -1)
            return other;
        return Span(start(), other.end());
    }
    const Span& operator+=(const Span& other)
    {
        *this = *this + other;
        return *this;
    }
private:
    String _fileName;
    int _startLine;
    int _startColumn;
    int _endLine;
    int _endColumn;
};

class CharacterSource
{
public:
    CharacterSource() { }
    CharacterSource(const String& string) : _string(string) { }
    CharacterSource(const String& string, const String& fileName)
      : _string(string), _location(fileName) { }
    int get(Span* span = 0)
    {
        Location start = _location;
        int c = getCharacter();
        if (span != 0)
            *span = Span(start, location());
        return c;
    }
    bool parse(int character, Span* span = 0)
    {
        CharacterSource s = *this;
        if (s.get(span) == character) {
            *this = s;
            return true;
        }
        return false;
    }
    void assert(int character, Span* span = 0)
    {
        CharacterSource s = *this;
        int found = s.get(span);
        if (found == character) {
            *this = s;
            return;
        }
        throwUnexpected(printable(character), printable(found));
    }
    Location location() const { return _location; }
    void throwUnexpected(const String& expected, String observed = "")
    {
        if (observed.empty()) {
            CharacterSource s = *this;
            observed = printable(s.get());
        }
        _location.throwError("Expected " + expected + ", found " + observed +
            ".");
    }
    int offset() const { return _location.offset(); }
    String subString(int start, int end) const
    {
        return _string.subString(start, end);
    }
private:
    static String printable(int character)
    {
        if (character == -1)
            return "end of file";
        if (character == 10)
            return "end of line";
        if (character == '\'')
            return "single quote";
        if (character >= ' ' && character <= '~')
            return String("'") + codePoint(character) + "'";
        return String("code point U+") + hex(character,
            character < 0x10000 ? 4 : character < 0x100000 ? 5 : 6);
    }
    int getCharacter()
    {
        int c = getCodePoint();
        if (c == 10) {
            CharacterSource s = *this;
            if (s.getCodePoint() == 13)
                *this = s;
            _location.advanceLine();
            return 10;
        }
        if (c == 13) {
            CharacterSource s = *this;
            if (s.getCodePoint() == 10)
                *this = s;
            _location.advanceLine();
            return 10;
        }
        return c;
    }
    int getCodePoint()
    {
        int b0 = getByte();
        if (b0 < 0x80) {
            if (b0 >= 0)
                _location.advanceColumn();
            return b0;
        }
        if (b0 < 0xc0 || b0 >= 0xf8)
            _location.throwErrorWithOffset(
                String("Expected 0x00..0x7F or 0xC0..0xF7, found ") +
                printableByte(b0));

        int b1 = getContinuationByte();
        if (b0 >= 0xc0 && b0 < 0xe0) {
            int codePoint = ((b0 & 0x1f) << 6) | (b1 & 0x3f);
            if (codePoint < 0x80)
                _location.throwErrorWithOffset("Overlong encoding");
            _location.advanceColumn();
            return codePoint;
        }

        int b2 = getContinuationByte();
        if (b0 >= 0xe0 && b0 < 0xf0) {
            int codePoint = ((b0 & 0x0f) << 12) | ((b1 & 0x3f) << 6) |
                (b2 & 0x3f);
            if (codePoint < 0x800)
                _location.throwErrorWithOffset("Overlong encoding");
            if (codePoint >= 0xd800 && codePoint < 0xe000)
                _location.throwErrorWithOffset("Unexpected surrogate");
            _location.advanceColumn();
            return codePoint;
        }

        int b3 = getContinuationByte();
        int codePoint = ((b0 & 0x07) << 18) | ((b1 & 0x3f) << 12) |
            ((b2 & 0x3f) << 6) | (b3 & 0x3f);
        if (codePoint < 0x10000)
            _location.throwErrorWithOffset("Overlong encoding");
        if (codePoint >= 0x110000)
            _location.throwErrorWithOffset("Code point too high");
        _location.advanceColumn();
        return codePoint;
    }
    int getByte()
    {
        int b = *_string;
        ++_string;
        ++_location;
        return b;
    }
    String printableByte(int b)
    {
        return b == -1 ? String("end of string") : String(hex(b, 2));
    }
    int getContinuationByte()
    {
        int b = getByte();
        if (b < 0x80 || b >= 0xc0)
            _location.throwErrorWithOffset(
                String("Expected 0x80..0xBF, found ") + printableByte(b));
        return b;
    }

    String _string;
    Location _location;
};

#endif // INCLUDED_STRING_H
