#ifndef INCLUDED_CHARACTER_SOURCE
#define INCLUDED_CHARACTER_SOURCE

class CharacterSource;

#include "string.h"

class CharacterSource
{
public:
    CharacterSource(const String& string) : _string(string), _offset(0)
    {
        initSimpleData();
    }
    void copyToUTF16(Array<UInt8>* data)
    {
        CharacterSource s = *this;
        int l = 2;
        while (!s.empty()) {
            int c = s.get();
            l += 2;
            if (c >= 0x10000)
                l += 2;
        }
        s = *this;
        data->allocate(l);
        while (!s.empty()) {
            int c = s.get();
            if (c >= 0x10000) {
                c -= 0x10000;
                data[l++] = (c >> 10) & 0xff;
                data[l++] = 0xd8 + ((c >> 18) & 0x03);
                data[l++] = c & 0xff;
                data[l++] = 0xdc + ((c >> 8) & 0x03);
            }
            else {
                data[l++] = c & 0xff;
                data[l++] = (c >> 8) & 0xff;
            }
        }
        data[l++] = 0;
        data[l++] = 0;
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
        static String expectedFirst("Expected 0x00..0x7F or 0xC0..0xF7, found ");
            static String expectedNext("Expected 0x80..0xBF, found ");
        String expected = first ? expectedFirst : expectedNext;
        static String endOfString("end of string");
        String s = (_length > 0 ? String(_buffer[0], 2) : endOfString;
        throw UTF8Exception(expected + s);
    }
    void throwUTF8Exception(String message)
    {
        static String at(" at ");
        static String in(" in ");
        String s = subString(offset, 1);
        throw UTF8Exception(message + at + String(s._string->offset(), 8) + in + s._string->buffer()->fileName());
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

#endif // INCLUDED_CHARACTER_SOURCE