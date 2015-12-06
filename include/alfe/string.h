#include "alfe/main.h"

#ifndef INCLUDED_STRING_H
#define INCLUDED_STRING_H

#include <cstdarg>

template<class T> class ExceptionTemplate;
typedef ExceptionTemplate<void> Exception;

template<class T> class StringTemplate;
typedef StringTemplate<Byte> String;

template<class T> class StringTemplate
{
public:
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
                    ": expected 0x0000..0xDBFF or 0xE000..0xFFFF, found " +
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
#endif
    class Hex
    {
    public:
        Hex(int n, int digits, bool ox) : _n(n), _digits(digits), _ox(ox) { }
        String operator+(const char* a)
        {
            int l = bytes();
            int al = strlen(a);
            String s(l + al);
            write(s.data());
            memcpy(s.data() + l, a, al);
            return s;
        }
    private:
        int bytes() const { return _digits + (_ox ? 2 : 0); }
        void write(T* destination) const
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
            int l = bytes();
            int al = strlen(a);
            String s(l + al);
            write(s.data());
            memcpy(s.data() + l, a, al);
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
        void write(T* destination) const
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
        Byte(const T& b) : _b(b) { }
        String operator+(const char* a)
        {
            int al = strlen(a);
            String s(1 + al);
            write(s.data());
            memcpy(s.data() + 1, a, al);
            return s;
        }
    private:
        void write(T* destination) const { *destination = _b; }
        T _b;
        friend class StringTemplate;
    };

    class Decimal
    {
    public:
        Decimal(int n, int digits = 0) : _n(n), _digits(digits) { }
        String operator+(const char* a)
        {
            int l = bytes();
            int al = strlen(a);
            String s(l + al);
            write(s.data());
            memcpy(s.data() + l, a, al);
            return s;
        }
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
        void write(T* destination) const
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

    class Boolean
    {
    public:
        explicit Boolean(bool b) : _b(b) { }
        int bytes() const { return _b ? 4 : 5; }
        String operator+(const char* a)
        {
            int l = bytes();
            int al = strlen(a);
            String s(l + al);
            write(s.data());
            memcpy(s.data() + l, a, al);
            return s;
        }
    private:
        void write(T* destination) const
        {
            auto s = reinterpret_cast<const T*>(_b ? "true" : "false");
            memcpy(destination, s, bytes());
        }
        bool _b;
        friend class StringTemplate;
    };

    StringTemplate() : StringTemplate(0, 0, 0) { }
    StringTemplate(const char* data, int length)
      : StringTemplate(reinterpret_cast<const T*>(data), length, length, false)
    { }
    StringTemplate(const char* data) : StringTemplate(data, strlen(data)) { }
    StringTemplate(const Array<T>& array)
      : StringTemplate(reinterpret_cast<const char*>(&array[0]), array.count())
    { }
    StringTemplate(const String& other) : StringTemplate(0, 0, 0)
    {
        *this = other;
    }
    StringTemplate(const char* a, const String& b) : StringTemplate(0, 0, 0)
    {
        int al = strlen(a);
        if (al == 0)
            *this = b;
        else {
            int bl = b.length();
            int l = al + bl;
            *this = StringTemplate(reinterpret_cast<const T*>(a), l, al);
            memcpy(data() + al, b.data(), bl);
        }
    }

    ~StringTemplate() { reset(); }
    const String& operator=(const String& other)
    {
        reset();
        setLength(other.length());
        unreset();
        if (small())
            memcpy(data(), other.data(), length());
        else {
            _array = other._array;
            _start = other._start;
        }
        return *this;
    }

#ifdef _WIN32
    StringTemplate(const WCHAR* utf16) : StringTemplate(bytes(utf16))
    {
        write(data(), utf16);
    }
#endif
    StringTemplate(const Hex& hex) : StringTemplate(hex.bytes())
    {
        hex.write(data());
    }
    StringTemplate(const CodePoint& c) : StringTemplate(c.bytes())
    {
        c.write(data());
    }
    StringTemplate(const Decimal d) : StringTemplate(d.bytes())
    {
        d.write(data());
    }
    StringTemplate(const Byte b): StringTemplate(1) { b.write(data()); }
    StringTemplate(const Boolean b): StringTemplate(b.bytes())
    {
        b.write(data());
    }
    const String& operator+=(const String& other)
    {
        extend(other.data(), other.length(), other.length());
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
        extend(0, l, 0);
        dd.write(data() + length() - l);
        return *this;
    }
    String operator+(int d) const { String s(*this); s += d; return s; }
#ifdef _WIN32
    const String& operator+=(const WCHAR* utf16)
    {
        int l = bytes(utf16);
        extend(0, l, 0);
        write(data() + length() - l, utf16);
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
        extend(0, l, 0);
        hex.write(data() + length() - l);
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
        extend(0, l, 0);
        c.write(data() + length() - l);
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
        extend(&b._b, 1, 1);
        return *this;
    }
    String operator+(const Byte& b) const
    {
        String s(*this);
        s += b;
        return s;
    }
    const String& operator+=(const Boolean& b)
    {
        int l = b.bytes();
        extend(0, l, 0);
        b.write(data() + length() - l);
        return *this;
    }
    String operator+(const Boolean& b) const
    {
        String s(*this);
        s += b;
        return s;
    }

    const String& operator*=(int n)
    {
        if (n == 0)
            *this = String();
        else {
            int l = length();
            extend(0, l*(n - 1), 0);
            T* source = data();
            T* destination = source + l;
            for (int i = 1; i < n; ++i) {
                memcpy(destination, source, l);
                destination += l;
            }
        }
        return *this;
    }
    String operator*(int n) { String s(*this); s *= n; return s; }

    String subString(int start, int length) const
    {
        return String(*this, start, length);
    }
    void operator++()
    {
        if (length() > 0)
            *this = subString(_start + 1, length() - 1);
    }
    T operator*() const { return *data(); }
    T operator[](int offset) const { return *(data() + offset); }
    UInt32 hash() const
    {
        Hash h(typeid(String));
        const T* p = data();
        for (int i = 0; i < length(); ++i) {
            h.mixin(*p);
            ++p;
        }
        return h;
    }
    bool operator==(const String& other) const
    {
        if (length() != other.length())
            return false;
        return memcmp(data(), other.data(), length()) == 0;
    }
    bool operator==(const char* b) const
    {
        const T* a = data();
        for (int i = 0; i < length(); ++i) {
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
        const T* a = data();
        const T* b = other.data();
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

    String alignRight(int n) { return String(" ")*(n - length()) + (*this); }
    String alignLeft(int n) { return (*this) + String(" ")*(n - length()); }

    bool empty() const { return length() == 0; }
    const T* data() const
    {
        return small() ? reinterpret_cast<const T*>(this) : _start;
    }
    int length() const
    {
        return *(reinterpret_cast<const int*>(reinterpret_cast<const char*>(
            this+1)) - 1);
    }
private:
    explicit StringTemplate(int length) : StringTemplate(0, length, 0) { }
    void setLength(int l)
    {
        *(reinterpret_cast<int*>(reinterpret_cast<char*>(this+1)) - 1) = l;
    }
    void extend(const T* otherData, int extendLength, int copyLength)
    {
        int newLength = length() + extendLength;
        if (small()) {
            if (newLength > smallStringThreshold()) {
                String a(data(), newLength, length());
                memcpy(a.data() + length(), otherData, copyLength);
                *this = a;
            }
            else {
                memcpy(data() + length(), otherData, copyLength);
                setLength(newLength);
            }
        }
        else {
            if (_array.count() == 0) {
                // We always need to own concatenations, because we don't know
                // where the end of the external buffer is.
                String a(data(), newLength, length());
                memcpy(a.data() + length(), otherData, copyLength);
                *this = a;
            }
            else {
                int t = min(static_cast<std::ptrdiff_t>(copyLength),
                    bufferStart() + bufferCount() - (data() + length()));
                if (memcmp(data() + length(), otherData, t) == 0) {
                    setLength(length() + t);
                    copyLength -= t;
                    otherData += t;
                }
                if (extendLength <= _array.allocated() - bufferCount() &&
                    bufferStart() + bufferCount() == data() + length()) {
                    _array.append(otherData, copyLength);
                    _array.expand(extendLength - copyLength);
                    setLength(newLength);
                }
                else {
                    String a(data(), newLength, length());
                    memcpy(a.data() + length(), otherData, copyLength);
                    *this = a;
                }
            }
        }
    }

    const T* bufferStart() const { return &_array[0]; }
    int bufferCount() const { return _array.count(); }
    // int bufferAllocated() const  // For debugger visualizer
    // {
    //     return
    // }
    T* data()
    {
        return small() ? reinterpret_cast<T*>(this) : const_cast<T*>(_start);
    }
    // With the small string optimization, we use all the space in String
    // (_array, _start and any alignment slop space) as the data buffer. Since
    // _array and _start are pointers (8-byte aligned on 64-bit machines -
    // checked with VC++ and GCC) we should get 8 bytes for small strings on
    // 32-bit and 20 bytes on 64-bit.
    static int smallStringThreshold()
    {
        // This is basically offsetof(String, length()) but that would call
        // length() on a null pointer and invoke undefined behavior.
        return reinterpret_cast<uintptr_t>(reinterpret_cast<int*>(
            reinterpret_cast<char*>(reinterpret_cast<String*>(0) + 1)) - 1);
    }
    bool small() const { return length() <= smallStringThreshold(); }
    // All constructors ultimately use this one, as String construction is
    // quite involved (due to the small string optimization).
    StringTemplate(const T* start, int size, int copy, bool owning = true)
    {
        if (size > smallStringThreshold()) {
            if (owning) {
                _array = AppendableArray<T>(size);
                _array.expand(size);
                _start = bufferStart();
            }
            else
                _start = start;
        }
        else
            owning = true;
        setLength(size);
        unreset();
        if (owning)
            memcpy(data(), start, copy);
    }
    StringTemplate(const String& other, int start, int size)
    {
        if (size > smallStringThreshold()) {
            *this = other;
            _start += start;
            setLength(size);
        }
        else {
            setLength(size);
            unreset();
            memcpy(data(), other.data() + start, size);
        }
    }
    void reset()
    {
        if (small()) {
            // Default construct Handle so destructor works correctly. This
            // should just zero out the _body member.
            new(&_array) AppendableArray<T>();
        }
    }
    void unreset()
    {
        if (small()) {
            // Should be a no-op. We'll undo this in reset().
            (&_array)->~AppendableArray<T>();
        }
    }

#ifdef _WIN32
    static T* write(T* destination, const WCHAR* utf16)
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
        return destination;
    }
#endif
    AppendableArray<T> _array;
    const T* _start;
    // Don't use _length, it's only there for size and alignment purposes. Use
    // length() instead, which places the field at the end of the end of the
    // class's memory in order to maximize small string space.
    int _length;

    // These things create an uninitialized String of a known length and then
    // write into it, something we only let our friends do (String is immutable
    // to non-friends).
    friend class ProgramBase;
    template<class U> friend class FileTemplate;
    template<class U> friend class StreamTemplate;
    template<class U> friend class CurrentDirectoryTemplate;
    friend String format(const char* format, ...);
};

String operator*(int n, String a) { a *= n; return a; }

String operator+(const char* a, const String& b) { return String(a, b); }
String operator+(const char* a, const String::Hex& b) { return String(a) + b; }
String operator+(const char* a, const String::Decimal& b)
{
    return String(a) + b;
}
String operator+(const char* a, const String::Boolean& b)
{
    return String(a) + b;
}
String operator+(const char* a, const String::CodePoint& b)
{
    return String(a) + b;
}
String operator+(const char* a, const String::Byte& b)
{
    return String(a) + b;
}

String::Hex hex(int n, int digits = 8, bool ox = true)
{
    return String::Hex(n, digits, ox);
}

String::Decimal decimal(int n) { return String::Decimal(n); }
String::CodePoint codePoint(int n) { return String::CodePoint(n); }

String format(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int c = vsnprintf(0, 0, format, args);
    String s(c);
    vsnprintf(reinterpret_cast<char*>(s.data()), c, format, args);
    return s.subString(0, s.length() - 1);  // Discard trailing null byte
}

template<class T> class StreamTemplate;
typedef StreamTemplate<void> Stream;

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
    const String _s;
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
