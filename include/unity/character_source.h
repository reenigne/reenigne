#ifndef INCLUDED_CHARACTER_SOURCE_H
#define INCLUDED_CHARACTER_SOURCE_H

template<class T> class CharacterSourceTemplate;
typedef CharacterSourceTemplate<void> CharacterSource;

#include "unity/string.h"
#include "unity/exception.h"
#include "unity/buffer.h"

template<class T> class CharacterSourceTemplate
{
public:
    CharacterSourceTemplate(const String& string) : _string(string), _offset(0)
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
        int b0 = getByte();
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
        String s = _length > 0 ? String::hexadecimal(_buffer[0], 2) : endOfString;
        throwUTF8Exception(expected + s);
    }
    void throwUTF8Exception(String message)
    {
        static String at(" at ");
        static String in(" in ");
        String s = subString(offset, 1);
        throw Exception(message + at + String::hexadecimal(s._implmentation->offset(), 8) + in + s._implementation->buffer()->fileName());
    }
    int getByte()
    {
        if (_buffer.valid())
            return _buffer[_start];
        UInt8 nybble = _start & 0x0f;
        _start >>= 4;
        return (nybble < 10 ? nybble + '0' : nybble + 'A' - 10);
    }
    int getNextByte()
    {
        if (empty())
            throwUTF8Exception(false);
        int b = getByte();
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
    StringTemplate<T> _string;
    int _offset;
};

#endif // INCLUDED_CHARACTER_SOURCE_H
