#include "alfe/main.h"

#ifndef INCLUDED_STRING_H
#define INCLUDED_STRING_H

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
        int bytes() const { return _digits + (_ox ? 2 : 0); }
        void write(::Byte* destination) const
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
        void write(::Byte* destination) const
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

    class Byte
    {
    public:
        Byte(int b) : _b(b) { }
        String operator+(const char* a)
        {
            String s(1 + strlen(a));
            s += *this;
            s += a;
            return s;
        }
    private:
        void write(::Byte* destination) const
        {
            *destination = static_cast< ::Byte>(_b);
        }
        int _b;
        friend class StringTemplate;
    };

    class Decimal
    {
    public:
        Decimal(int n, int digits = 0) : _n(n), _digits(digits) { }
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
            if (l < _digits)
                return _digits;
            return l;
        }
        void write(::Byte* destination) const
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
            do {
                --l;
                destination[l] = d + '0';
                n /= 10;
                d = n % 10;
            } while (l > 0);
        }

        int _n;
        int _digits;
        friend class StringTemplate;
    };

    StringTemplate() : _start(0), _length(0) { }
    StringTemplate(const char* data)
      : _buffer(data), _start(0), _length(strlen(data)) { }
    StringTemplate(const char* data, int length)
      : _buffer(data), _start(0), _length(length) { }
    explicit StringTemplate(int length)
      : _buffer(length), _start(0), _length(0) { }
    StringTemplate(const Array<Byte>& array)
      : _buffer(&array[0]), _start(0), _length(array.count()) { }
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
    StringTemplate(const CodePoint& c) : _start(0)
    {
        int l = c.bytes();
        _buffer = Buffer(l);
        c.write(data());
        _length = l;
    }
    StringTemplate(const Decimal d) : _start(0)
    {
        int l = d.bytes();
        _buffer = Buffer(l);
        d.write(data());
        _length = l;
    }
    StringTemplate(const Byte b): _start(0)
    {
        _buffer = Buffer(1);
        b.write(data());
        _length = 1;
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
    const String& operator+=(const Byte& b)
    {
        expand(1);
        _buffer.expand(1);
        b.write(data() + _length);
        ++_length;
        return *this;
    }
    String operator+(const Byte& b) const
    {
        String s(*this);
        s += b;
        return s;
    }
    const String& operator*=(int n)
    {
        int l = _length*(n - 1);
        expand(l);
        _buffer.expand(l);
        ::Byte* source = data();
        ::Byte* destination = source + _length;
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
    const ::Byte* data() const { return _buffer.data() + _start; }
    String subString(int start, int length) const
    {
        if (length == 0)
            return String();
        return String(_buffer, _start + start, length);
    }
    void operator++() { ++_start; --_length; }
    ::Byte operator*() const { return *data(); }
    ::Byte operator[](int offset) const { return *(data() + offset); }
    int hash() const
    {
        int h = 0;
        const ::Byte* p = data();
        for (int i = 0; i < _length; ++i) {
            h = h * 67 + *p - 113;
            ++p;
        }
        return h;
    }
    bool equalsIgnoreCase(const String& other) const
    {
        if (_length != other._length)
            return false;
        if (_length != 0) {
            const ::Byte* a = data();
            const ::Byte* b = other.data();
            for (int i = 0; i < _length; ++i) {
                if (tolower(*a) != tolower(*b))
                    return false;
                ++a;
                ++b;
            }
        }
        return true;
    }
    bool equalsIgnoreCase(const char* b) const
    {
        if (_length != 0) {
            const ::Byte* a = data();
            for (int i = 0; i < _length; ++i) {
                if (*b == 0)
                    return false;
                if (tolower(*a) != tolower(*b))
                    return false;
                ++a;
                ++b;
            }
        }
        return *b == 0;    
    }
    bool operator==(const String& other) const
    {
        if (_length != other._length)
            return false;
        if (_length != 0) {
            const ::Byte* a = data();
            const ::Byte* b = other.data();
            for (int i = 0; i < _length; ++i) {
                if (*a != *b)
                    return false;
                ++a;
                ++b;
            }
        }
        return true;
    }
    bool operator==(const char* b) const
    {
        if (_length != 0) {
            const ::Byte* a = data();
            for (int i = 0; i < _length; ++i) {
                if (*b == 0)
                    return false;
                if (*a != *b)
                    return false;
                ++a;
                ++b;
            }
        }
        return *b == 0;
    }
    bool operator!=(const String& other) const { return !operator==(other); }
    bool operator<(const String& other) const
    {
        const ::Byte* a = data();
        const ::Byte* b = other.data();
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
        void expand(const ::Byte* data, int length)
        {
            _implementation->expand(data, length);
        }
        void expand(int length) { _implementation->expand(length); }
        const ::Byte* data() const
        {
            if (!_implementation.valid())
                return 0;
            return _implementation->constData();
        }
        ::Byte* data()
        {
            if (!_implementation.valid())
                return 0;
            return _implementation->data();
        }
    private:
        class Implementation : public ReferenceCounted
        {
        public:
            virtual int end() const = 0;
            virtual void expand(const ::Byte* data, int length) = 0;
            virtual void expand(int length) = 0;
            const ::Byte* constData() const { return _data; }
            ::Byte* data() { return const_cast< ::Byte*>(_data); }
        protected:
            const ::Byte* _data;
        };
        class OwningImplementation : public Implementation
        {
        public:
            OwningImplementation(int n)
            {
                _allocated = n;
                _used = 0;
                Implementation::_data = static_cast< ::Byte*>(operator new(n));
            }
            ~OwningImplementation()
            {
                operator delete(data());
            }
            int end() const { return _used; }
            void expand(const ::Byte* source, int length)
            {
                expand(length);
                memcpy(data() + _used - length, source, length);
            }
            void expand(int length)
            {
                int allocate = _allocated;
                while (allocate < _used + length)
                    allocate *= 2;
                if (_allocated < allocate) {
                    const ::Byte* newData =
                        static_cast<const ::Byte*>(operator new(allocate));
                    swap(Implementation::_data, newData);
                    memcpy(data(), newData, _used);
                    operator delete(const_cast< ::Byte*>(newData));
                    _allocated = allocate;
                }
                _used += length;
            }
        private:
            int _allocated;
            int _used;
        };

        class LiteralImplementation : public Implementation
        {
        public:
            LiteralImplementation(const char* data)
            {
                Implementation::_data = reinterpret_cast<const ::Byte*>(data);
            }
            int end() const { return -1; }
            void expand(const ::Byte* data, int length) { throw Exception(); }
            void expand(int length) { throw Exception(); }
        };
        Reference<Implementation> _implementation;
    };

    StringTemplate(const Buffer& buffer, int start, int length)
      : _buffer(buffer), _start(start), _length(length) { }
    bool expandable() const { return _buffer.end() == _length; }
    ::Byte* data() { return _buffer.data() + _start; }
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
                throw Exception("String offset " + hex(offset) +
                    ": expected 0x0000..0xD800 or 0xE000..0xFFFF, found " +
                    hex(c, 4));
            ++offset;
            if (c >= 0xd800 && c < 0xdc00) {
                int c2 = *utf16;
                ++utf16;
                if (c2 < 0xdc00 || c2 >= 0xe000)
                    throw Exception("String offset " + hex(offset) +
                        ": expected 0xDC00..0xDFFF, found " + hex(c2, 4));
                ++offset;
                c = (((c & 0x3ff) << 10) | (c2 & 0x3ff)) + 0x10000;
            }
            n += CodePoint(c).bytes();
        }
        return n;
    }
    static void write(::Byte* destination, const WCHAR* utf16)
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
            Buffer newBuffer = Buffer(_length + l);
            newBuffer.expand(data(), _length);
            _buffer = newBuffer;
            _start = 0;
        }
    }

    ::Byte* writableData() { return _buffer.data() + _start; }

    Buffer _buffer;
    int _start;
    int _length;

    friend class NullTerminatedString;
    friend class ProgramBase;
    template<class U> friend class FileTemplate;
    template<class U> friend class HandleTemplate;
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

template<class T> class HandleTemplate;
typedef HandleTemplate<void> Handle;

#ifdef _WIN32
template<class T> class NullTerminatedWideStringTemplate
{
public:
    NullTerminatedWideStringTemplate(String s)
    {
        CharacterSource start(s);
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
    operator WCHAR*()
    {
        return &_buffer[0];
    }
private:
    Array<WCHAR> _buffer;
};

typedef NullTerminatedWideStringTemplate<void> NullTerminatedWideString;
#endif

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

#endif // INCLUDED_STRING_H
