#ifndef INCLUDED_CHARACTER_SOURCE_H
#define INCLUDED_CHARACTER_SOURCE_H

class DiagnosticLocation
{
public:
    DiagnosticLocation() { }
    DiagnosticLocation(String fileName)
      : _fileName(fileName), _line(1), _column(1)
    { }
    DiagnosticLocation(String fileName, int line, int column)
      : _fileName(fileName), _line(line), _column(column)
    { }
    String asString() const
    {
        return _fileName + openParenthesis + String::decimal(_line) + comma +
            String::decimal(_column) + closeParenthesis;
    }
    String fileName() const { return _fileName; }
    void advanceColumn() { ++_column; }
    void advanceLine() { _column = 1; ++_line; }
    void throwError(const String& message) const
    {
        throw Exception(asString() + colonSpace + message);
    }
    int line() const { return _line; }
    int column() const { return _column; }
private:
    String _fileName;
    int _line;
    int _column;
};

class DiagnosticSpan
{
public:
    DiagnosticSpan() { }
    DiagnosticSpan(DiagnosticLocation start, DiagnosticLocation end)
      : _fileName(start.fileName()),
        _startLine(start.line()),
        _startColumn(start.column()),
        _endLine(end.line()),
        _endColumn(end.column())
    { }
    DiagnosticSpan(String fileName, int startLine, int startColumn, int endLine, int endColumn)
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
    String asString() const
    {
        static String s(")-(");
        return _fileName + openParenthesis + String::decimal(_startLine) + comma +
            String::decimal(_startColumn) + s + String::decimal(_endLine) + comma + 
            String::decimal(_endColumn) + closeParenthesis;
    }
    void throwError(const String& message) const
    {
        throw Exception(asString() + colonSpace + message);
    }
    DiagnosticLocation start() const
    { 
        return DiagnosticLocation(_fileName, _startLine, _startColumn);
    }
    DiagnosticLocation end() const
    {
        return DiagnosticLocation(_fileName, _endLine, _endColumn);
    }
private:
    String _fileName;
    int _startLine;
    int _startColumn;
    int _endLine;
    int _endColumn;
};

class ByteSource
{
public:
    ByteSource() { }
    ByteSource(const String& string) : _string(string), _offset(0)
    {
        initSimpleData();
    }
    int get()
    {
        if (_length == 0)
            return -1;
        int byte;
        if (_buffer.valid()) {
            byte = _buffer[_start];
            ++_start;
        }
        else {
            UInt8 nybble = _start & 0x0f;
            _start >>= 4;
            byte = (nybble < 10 ? nybble + '0' : nybble + 'A' - 10);
        }
        ++_offset;
        --_length;
        if (_length == 0)
            initSimpleData();
        return byte;
    }
    int offset() const { return _offset; }
    String subString(int start, int end) { return _string.subString(start, end - start); }
private:
    void initSimpleData()
    {
        _string.initSimpleData(_offset, &_buffer, &_start, &_length);
    }
    Buffer _buffer;
    int _start;
    int _length;
    String _string;
    int _offset;
};

class CodePointSource
{
public:
    CodePointSource() { }
    CodePointSource(const String& string) : _byteSource(string) { }
    int get()
    {
        static String overlongEncoding("Overlong encoding");
        static String codePointTooHigh("Code point too high");
        static String unexpectedSurrogate("Unexpected surrogate");

        int offset = _byteSource.offset();
        int b0 = _byteSource.get();
        if (b0 < 0x80)
            return b0;
        if (b0 < 0xc0 || b0 >= 0xf8)
            throwUTF8Exception(true, b0, offset);

        int b1 = getNextByte();
        if (b0 >= 0xc0 && b0 < 0xe0) {
            int codePoint = ((b0 & 0x1f) << 6) | (b1 & 0x3f);
            if (codePoint < 0x80)
                throwUTF8Exception(overlongEncoding, offset);
            return codePoint;
        }

        int b2 = getNextByte();
        if (b0 >= 0xe0 && b0 < 0xf0) {
            int codePoint = ((b0 & 0x0f) << 12) | ((b1 & 0x3f) << 6) | (b2 & 0x3f);
            if (codePoint < 0x800)
                throwUTF8Exception(overlongEncoding, offset);
            if (codePoint >= 0xd800 && codePoint < 0xe000)
                throwUTF8Exception(unexpectedSurrogate, offset);
            return codePoint;
        }

        int b3 = getNextByte();
        int codePoint = ((b0 & 0x07) << 18) | ((b1 & 0x3f) << 12) | ((b2 & 0x3f) << 6) | (b3 & 0x3f);
        if (codePoint < 0x10000)
            throwUTF8Exception(overlongEncoding, offset);
        if (codePoint >= 0x110000)
            throwUTF8Exception(codePointTooHigh, offset);
        return codePoint;
    }
    int offset() const { return _byteSource.offset(); }
    String subString(int start, int end) { return _byteSource.subString(start, end); }
private:
    void throwUTF8Exception(bool first, int b, int offset)
    {
        static String expectedFirst("Expected 0x00..0x7F or 0xC0..0xF7, found 0x");
        static String expectedNext("Expected 0x80..0xBF, found 0x");
        String expected = first ? expectedFirst : expectedNext;
        static String endOfString("end of string");
        String s = (b == -1) ? endOfString : String::hexadecimal(b, 2);
        throwUTF8Exception(expected + s, offset);
    }
    void throwUTF8Exception(String message, int offset)
    {
        static String at(" at ");
        throw Exception(message + at + String::hexadecimal(offset, 8));
    }
    int getNextByte()
    {
        int b = _byteSource.get();
        if (b < 0x80 || b >= 0xc0)
            throwUTF8Exception(false, b, _byteSource.offset());
        return b;
    }

    ByteSource _byteSource;
};

class CharacterSource
{
public:
    CharacterSource() { }
    CharacterSource(const String& string, const String& fileName)
      : _codePointSource(string), _location(fileName)
    { }
    int get()
    {
        int c = -1;
        try {
            c = _codePointSource.get();
            _location.advanceColumn();
            if (c == 10) {
                CodePointSource s = _codePointSource;
                if (s.get() == 13)
                    _codePointSource = s;
                _location.advanceLine();
                return 10;
            }
            if (c == 13) {
                CodePointSource s = _codePointSource;
                if (s.get() == 10)
                    _codePointSource = s;
                _location.advanceLine();
                return 10;
            }
        }
        catch (Exception& e) {
            static String in(" in ");
            throw Exception(e.message() + in + _location.fileName());
        }
        return c;
    }
    bool parse(int character)
    {
        CharacterSource s = *this;
        if (s.get() == character) {
            *this = s;
            return true;
        }
        return false;
    }
    void assert(int character)
    {
        CharacterSource start = *this;
        int found = get();
        if (found == character)
            return;
        start.throwUnexpected(String::codePoint(character), String::codePoint(found));
    }
    DiagnosticLocation location() const { return _location; }
    void throwUnexpected(const String& expected, const String& observed)
    {
        static String expectedMessage("Expected ");
        static String found(", found ");
        _location.throwError(expectedMessage + expected + found + observed);
    }
    int offset() const { return _codePointSource.offset(); }
    String subString(int start, int end) { return _codePointSource.subString(start, end); }
private:          
    CodePointSource _codePointSource;
    DiagnosticLocation _location;
};

#endif // INCLUDED_CHARACTER_SOURCE_H
