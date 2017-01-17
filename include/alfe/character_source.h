#include "alfe/main.h"

#ifndef INCLUDED_CHARACTER_SOURCE_H
#define INCLUDED_CHARACTER_SOURCE_H

class Location
{
public:
    Location() : _line(0), _column(0), _offset(0) { }
    Location(const File& file) : _file(file), _line(1), _column(1), _offset(0)
    { }
    Location(const File& file, int line, int column)
      : _file(file), _line(line), _column(column), _offset(0)
    { }
    String toString() const
    {
        return _file.path() + "(" + _line + "," + _column + ")";
    }
    File file() const { return _file; }
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
    File _file;
    int _line;
    int _column;
    int _offset;
};

class Span
{
public:
    Span() : _startLine(-1) { }
    Span(Location start, Location end)
      : _file(start.file()),
        _startLine(start.line()),
        _startColumn(start.column()),
        _endLine(end.line()),
        _endColumn(end.column())
    { }
    Span(const File& file, int startLine, int startColumn, int endLine,
        int endColumn)
      : _file(file),
        _startLine(startLine),
        _startColumn(startColumn),
        _endLine(endLine),
        _endColumn(endColumn)
    { }
    File file() const { return _file; }
    int startLine() const { return _startLine; }
    int startColumn() const { return _startColumn; }
    int endLine() const { return _endLine; }
    int endColumn() const { return _endColumn; }
    String toString() const
    {
        return _file.path() + "(" + _startLine + "," + _startColumn + ")-(" +
            _endLine + "," + _endColumn + ")";
    }
    void throwError(const String& message) const
    {
        if (_startLine == -1)
            throw Exception(message);
        throw Exception(toString() + ": " + message);
    }
    Location start() const
    {
        return Location(_file, _startLine, _startColumn);
    }
    Location end() const { return Location(_file, _endLine, _endColumn); }
    Span operator+(const Span& other) const
    {
        if (_startLine == -1)
            return other;
        if (other._startLine == -1)
            return *this;
        return Span(start(), other.end());
    }
    const Span& operator+=(const Span& other)
    {
        *this = *this + other;
        return *this;
    }
private:
    File _file;
    int _startLine;
    int _startColumn;
    int _endLine;
    int _endColumn;
};

template<class T> class CharacterSourceT
{
public:
    CharacterSourceT() { }
    CharacterSourceT(const String& string) : _string(string) { }
    CharacterSourceT(const String& string, const File& file)
      : _string(string), _location(file) { }
    int get(Span* span = 0)
    {
        Location start = _location;
        int c = getCharacter();
        if (span != 0)
            *span += Span(start, location());
        return c;
    }
    bool parse(int character, Span* span = 0)
    {
        CharacterSource s = *this;
        Span span2;
        if (s.get(&span2) == character) {
            *this = s;
            if (span != 0)
                *span += span2;
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
    bool parseString(const String& string, Span* span = 0)
    {
        CharacterSource s = *this;
        CharacterSource ss(string);
        Span span2;
        do {
            int character = ss.getCharacter();
            if (character == -1)
                break;
            if (!s.parse(character, &span2))
                return false;
            if (span != 0)
                *span += span2;
        } while (true);
        *this = s;
        return true;
    }
    void assertString(const String& string, Span* span = 0)
    {
        CharacterSource s = *this;
        CharacterSource ss(string);
        Span span2;
        do {
            int character = ss.getCharacter();
            if (character == -1)
                break;
            s.assert(character, &span2);
            if (span != 0)
                *span += span2;
        } while (true);
        *this = s;
    }
    String delimitString(const String& delimiter, bool* eof, Span* span = 0)
    {
        CharacterSource s = *this;
        int startOffset = s.offset();
        CharacterSource ss(delimiter);
        int first = ss.getCharacter();
        *eof = false;
        do {
            int endOffset = s.offset();
            Span span2;
            int character = s.get(&span2);
            if (character == -1) {
                *eof = true;
                return String();
            }
            if (character == first) {
                CharacterSource s2 = s;
                CharacterSource ss2 = ss;
                int delimiterCharacter;
                do {
                    delimiterCharacter = ss2.getCharacter();
                    if (delimiterCharacter == -1) {
                        *this = s2;
                        return subString(startOffset, endOffset);
                    }
                } while (s2.getCharacter() == delimiterCharacter);
            }
            if (span != 0)
                *span += span2;
        } while (true);
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
        return _string.subString(start, end - start);
    }
    int length() const { return _string.length(); }
    int getByte()
    {
        int l = _location.offset();
        if (l == _string.length())
            return -1;
        int b = _string[l];
        ++_location;
        return b;
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

String enquote(String s)
{
    String r = "\"";
    CharacterSource source(s);
    int startOffset = source.offset();
    do {
        int offset = source.offset();
        int c = source.get();
        switch (c) {
        case -1:
            return r + source.subString(startOffset, offset) + "\"";
        case '\"':
            r += source.subString(startOffset, offset) + "\\\"";
            startOffset = source.offset();
            break;
        case '\\':
            r += source.subString(startOffset, offset) + "\\\\";
            startOffset = source.offset();
            break;
        }
    } while (true);
}

#endif // INCLUDED_CHARACTER_SOURCE_H
