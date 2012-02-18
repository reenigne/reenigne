#include "unity/string.h"

#ifndef INCLUDED_CHARACTER_SOURCE_H
#define INCLUDED_CHARACTER_SOURCE_H

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
        return _string.subString(start, end - start);
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
        int l = _location.offset();
        if (l == _string.length())
            return -1;
        int b = _string[l];
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

#endif // INCLUDED_CHARACTER_SOURCE_H
