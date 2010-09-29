#include "unity/string.h"
#include "unity/file.h"
#include <vector>
#include <stdio.h>
#include <stdlib.h>

#include <time.h>

class Program
{
public:
    Program(String fileName, String annotationsFileName)
    {
		File file(fileName);
		String contents = file.contents();
        _source = contents.start();
        File annotationsFile(annotationsFileName);
        _annotations = annotationsFile.contents();
    }

    void load()
    {
        for (int i = 0; i < 0x400; ++i)
            _data[i] = 0;
        _done = false;
        while (!_done)
            parseLine();
        int position = 0;
        CharacterSource annotations = _annotations.start();
        for (int i = 0; i < 0x200; ++i) {
            int end;
            do {
                int c = annotations.get();
                if (c == 10) {
                    end = annotations.position() - 1;
                    break;
                }
                if (c == -1) {
                    end = annotations.position();
                    break;
                }
            } while (true);
            _annotation.push_back(_annotations.subString(position, end - position));
            position = annotations.position();
        }
    }
    void parseLine()
    {
        _source.assert(':');
        _checkSum = 0;
        CharacterSource byteCountLocation = _source;
        int byteCount = readByte();
        int address = readByte() << 8;
        address |= readByte();
        CharacterSource recordTypeLocation = _source;
        int recordType = readByte();
        for (int i = 0; i < byteCount; ++i) {
            int b = readByte();
            if (recordType == 0 && address < 0x400)
                _data[address++] = b;
        }
        switch (recordType) {
            case 0:  // data record - handled above
                break;
            case 1:  // end of file record
                if (byteCount != 0) {
                    static String error("End of file marker incorrect. Expected no data, found ");
                    static String bytes(" bytes.");
                    byteCountLocation.throwError(error + String::decimal(byteCount) + bytes);
                }
                _done = true;
                break;
            case 4:  // extended linear address record
                break;
            default:
                {
                    static String error("Don't know what to do with record type ");
                    recordTypeLocation.throwError(error + String::decimal(recordType));
                }
        }
        CharacterSource checkSumLocation = _source;
        int checkSum = readByte();
        if ((_checkSum & 0xff) != 0) {
            static String error("Checksum incorrect. Expected ");
            static String found(", found ");
            checkSumLocation.throwError(error + String::hexadecimal((checkSum - _checkSum) & 0xff, 2) + found + String::hexadecimal(checkSum, 2));
        }
        _source.assert(10);
    }
    int readByte()
    {
        int b = readNybble() << 4;
        b |= readNybble();
        _checkSum += b;
        return b;
    }
    int readNybble()
    {
        CharacterSource start = _source;
        int n = _source.get();
        if (n >= '0' && n <= '9')
            return n - '0';
        if (n >= 'a' && n <= 'f')
            return n + 10 - 'a';
        if (n >= 'A' && n <= 'F')
            return n + 10 - 'A';
        static String expected("0-9 or A-F");
        start.throwUnexpected(expected, String::codePoint(n));
    }
    int op(int address) const
    {
        address <<= 1;
        return _data[address] | (_data[address + 1] << 8);
    }
    String annotation(int line) const { return _annotation[line]; }
private:
    UInt8 _data[0x400];
    bool _done;
    int _checkSum;
    CharacterSource _source;
    String _annotations;
    std::vector<String> _annotation;
};

class Simulation;

template<class Simulation> class BarTemplate : public ReferenceCounted
{
public:
    BarTemplate(Simulation* simulation, const Program* program, int number, bool debug)
      : _simulation(simulation), _program(program), _t(0), _debug(debug), _tPerCycle(1), _skipping(false), _primed(false), _live(number == 0), _number(number), _indent(0), _console(Handle::consoleOutput())
    {
        reset();
        for (int i = 0; i < 4; ++i)
            _connectedBar[i] = -1;
    }
    void simulateTo(double t)
    {
        if (!_live)
            return;
        if (_debug)
            printf("%*sSimulating bar %i to %lf\n", _indent*8, "", _number, t);
        while (_t < t)
            simulateSubcycle();
    }
    void adjustTime(double t) { _t -= t; }
    void simulateSubcycle()
    {
        if (_skipping) {
            _skipping = false;
            _t = _t + 1.0;
            return;
        }
        if (_readSubCycle) {
            _t += 0.25*_tPerCycle;
            _readSubCycle = false;
            simulateToRead();
        }
        else {
            _t += 0.75*_tPerCycle;
            _readSubCycle = true;
            simulateToWrite();
        }
    }
    void simulateToRead()
    {
        if (_debug)
            printf("%*s% 7.2lf ", _indent*8, "", _t);
        int pc = _pch | _memory[2];
        int op = _program->op(pc);
        incrementPC();
        UInt16 r;
        if ((op & 0x800) == 0) {
            _f = op & 0x1f;
            if ((op & 0x400) == 0) {
                bool d = ((op & 0x20) != 0);  // true if destination is f, false if destination is W
                char dc = d ? 'f' : 'W';
                switch (op >> 6) {
                    case 0x0:
                        if (!d)
                            switch (_f) {
                                case 0:
                                    if (_debug) { printf("NOP             "); _program->annotation(pc).write(_console); printf("\n"); }
                                    _f = -1;
                                    break;
                                case 2:
                                    if (_debug) { printf("OPTION          "); _program->annotation(pc).write(_console); printf("\n"); }
                                    _f = -1;
                                    _option = _w;
                                    break;
                                case 3:
                                    if (_debug) { printf("SLEEP           "); _program->annotation(pc).write(_console); printf("\n"); }
                                    _f = -1;
                                    throw Exception(String("SLEEP not supported"));
                                    break;
                                case 4:
                                    if (_debug) { printf("CLRWDT          "); _program->annotation(pc).write(_console); printf("\n"); }
                                    _f = -1;
                                    throw Exception(String("CLRWDT not supported"));
                                    break;
                                case 5:  // Not a real PIC12F508 opcode - used for simulator escape (data)
                                    _f = -1;
                                    if (_debug) { printf("%i               ", _w & 1); _program->annotation(pc).write(_console); printf("\n"); }
                                    _simulation->streamBit((_w & 1) != 0);
                                    break;
                                case 6:
                                    if (_debug) { printf("TRIS GPIO       "); _program->annotation(pc).write(_console); printf("\n"); }
                                    _f = 0x20;
                                    _data = _w;
                                    break;
                                case 7:  // Not a real PIC12F08 opcode - used for simulator escape (space)
                                    _f = -1;
                                    if (_debug) { printf("---             "); _program->annotation(pc).write(_console); printf("\n"); }
                                    _simulation->streamStart();
                                    break;
                                default:
                                    unrecognizedOpcode(op);
                                    break;
                            }
                        else {
                            if (_debug) { printf("MOVWF 0x%02x      ", _f); _program->annotation(pc).write(_console); printf("\n"); }
                            _data = _w;
                        }
                        break;
                    case 0x1:
                        if (_debug) {
                            if (!d)
                                printf("CLRW            ");
                            else
                                printf("CLRF 0x%02x       ", _f);
                             _program->annotation(pc).write(_console);
                             printf("\n"); 
                        }
                        storeZ(0, d);
                        break;
                    case 0x2:
                        if (_debug) { printf("SUBWF 0x%02x, %c   ", _f, dc); _program->annotation(pc).write(_console); printf("\n"); }
                        {
                            UInt8 m = readMemory(_f);
                            r = m - _w;
                            if (r & 0x100)
                                _memory[3] |= 1;
                            else
                                _memory[3] &= 0xfe;
                            if ((m & 0xf) - (_w & 0xf) != (r & 0xf))
                                _memory[3] |= 2;
                            else
                                _memory[3] &= 0xfd;
                            storeZ(r, d);
                        }
                        break;
                    case 0x3:
                        if (_debug) { printf("DECF 0x%02x, %c    ", _f, dc); _program->annotation(pc).write(_console); printf("\n"); }
                        storeZ(readMemory(_f) - 1, d);
                        break;
                    case 0x4:
                        if (_debug) { printf("IORWF 0x%02x, %c   ", _f, dc); _program->annotation(pc).write(_console); printf("\n"); }
                        storeZ(readMemory(_f) | _w, d);
                        break;
                    case 0x5:
                        if (_debug) { printf("ANDWF 0x%02x, %c   ", _f, dc); _program->annotation(pc).write(_console); printf("\n"); }
                        storeZ(readMemory(_f) & _w, d);
                        break;
                    case 0x6:
                        if (_debug) { printf("XORWF 0x%02x, %c   ", _f, dc); _program->annotation(pc).write(_console); printf("\n"); }
                        storeZ(readMemory(_f) ^ _w, d);
                        break;
                    case 0x7:
                        if (_debug) { printf("ADDWF 0x%02x, %c   ", _f, dc); _program->annotation(pc).write(_console); printf("\n"); }
                        {
                            UInt8 m = readMemory(_f);
                            r = m + _w;
                            if (r & 0x100)
                                _memory[3] |= 1;
                            else
                                _memory[3] &= 0xfe;
                            if ((_w & 0xf) + (m & 0xf) != (r & 0xf))
                                _memory[3] |= 2;
                            else
                                _memory[3] &= 0xfd;
                            storeZ(r, d);
                        }
                        break;
                    case 0x8:
                        if (_debug) { printf("MOVF 0x%02x, %c    ", _f, dc); _program->annotation(pc).write(_console); printf("\n"); }
                        storeZ(readMemory(_f), d);
                        break;
                    case 0x9:
                        if (_debug) { printf("COMF 0x%02x, %c    ", _f, dc); _program->annotation(pc).write(_console); printf("\n"); }
                        storeZ(~readMemory(_f), d);
                        break;
                    case 0xa:
                        if (_debug) { printf("INCF 0x%02x, %c    ", _f, dc); _program->annotation(pc).write(_console); printf("\n"); }
                        storeZ(readMemory(_f) + 1, d);
                        break;
                    case 0xb:
                        if (_debug) { printf("DECFSZ 0x%02x, %c  ", _f, dc); _program->annotation(pc).write(_console); printf("\n"); }
                        r = readMemory(_f) - 1;
                        store(r, d);
                        if (r == 0) {
                            incrementPC();
                            _skipping = true;
                        }
                        break;
                    case 0xc:
                        if (_debug) { printf("RRF 0x%02x, %c     ", _f, dc); _program->annotation(pc).write(_console); printf("\n"); }
                        r = readMemory(_f) | ((_memory[3] & 1) << 8);
                        setCarry((r & 1) != 0);
                        store(r >> 1, d);
                        break;
                    case 0xd:
                        if (_debug) { printf("RLF 0x%02x, %c     ", _f, dc); _program->annotation(pc).write(_console); printf("\n"); }
                        r = (readMemory(_f) << 1) | (_memory[3] & 1);
                        setCarry((r & 0x100) != 0);
                        store(r, d);
                        break;
                    case 0xe:
                        if (_debug) { printf("SWAPF 0x%02x, %c   ", _f, dc); _program->annotation(pc).write(_console); printf("\n"); }
                        r = readMemory(_f);
                        store((r >> 4) | (r << 4), d);
                        break;
                    case 0xf:
                        if (_debug) { printf("INCFSF 0x%02x, %c  ", _f, dc); _program->annotation(pc).write(_console); printf("\n"); }
                        r = readMemory(_f) + 1;
                        store(r, d);
                        if (r == 0) {
                            incrementPC();
                            _skipping = true;
                        }
                        break;
                }
            }
            else {
                int b = (op >> 5) & 7;
                int m = 1 << b;
                switch (op >> 8) {
                    case 4:
                        if (_debug) { printf("BCF 0x%02x, %i     ", _f, b); _program->annotation(pc).write(_console); printf("\n"); }
                        _data = readMemory(_f) & ~m;
                        break;
                    case 5:
                        if (_debug) { printf("BSF 0x%02x, %i     ", _f, b); _program->annotation(pc).write(_console); printf("\n"); }
                        _data = readMemory(_f) | m;
                        break;
                    case 6:
                        if (_debug) { printf("BTFSC 0x%02x, %i   ", _f, b); _program->annotation(pc).write(_console); printf("\n"); }
                        if ((readMemory(_f, m) & m) == 0) {
                            incrementPC();
                            _skipping = true;
                        }
                        _f = -1;
                        break;
                    case 7:
                        if (_debug) { printf("BTFSS 0x%02x, %i   ", _f, b); _program->annotation(pc).write(_console); printf("\n"); }
                        if ((readMemory(_f, m) & m) != 0) {
                            incrementPC();
                            _skipping = true;
                        }
                        _f = -1;
                        break;
                }
            }
        }
        else {
            _f = -1;
            int d = op & 0xff;
            switch (op >> 8) {
                case 0x8:
                    if (_debug) { printf("RETLW 0x%02x      ", d); _program->annotation(pc).write(_console); printf("\n"); }
                    _skipping = true;
                    _memory[2] = _stack[0];
                    _pch = _stack[0] & 0x100;
                    _stack[0] = _stack[1];
                    _w = d;
                    break;
                case 0x9:
                    if (_debug) { printf("CALL  0x%02x      ", d); _program->annotation(pc).write(_console); printf("\n"); }
                    _skipping = true;
                    _stack[1] = _stack[0];
                    _stack[0] = _memory[2] | _pch;
                    _pch = 0;
                    _memory[2] = d;
                    break;
                case 0xa:
                case 0xb:
                    if (_debug) { printf("GOTO  0x%03x     ", op & 0x1ff); _program->annotation(pc).write(_console); printf("\n"); }
                    _skipping = true;
                    _pch = op & 0x100;
                    _memory[2] = d;
                    break;
                case 0xc:
                    if (_debug) { printf("MOVLW 0x%02x      ", d); _program->annotation(pc).write(_console); printf("\n"); }
                    _w = d;
                    break;
                case 0xd:
                    if (_debug) { printf("IORLW 0x%02x      ", d); _program->annotation(pc).write(_console); printf("\n"); }
                    _w |= d;
                    break;
                case 0xe:                                
                    if (_debug) { printf("ANDLW 0x%02x      ", d); _program->annotation(pc).write(_console); printf("\n"); }
                    _w &= d;
                    break;
                case 0xf:
                    if (_debug) { printf("XORLW 0x%02x      ", d); _program->annotation(pc).write(_console); printf("\n"); }
                    _w ^= d;
                    break;
            }
        }
    }
    void simulateToWrite()
    {
        if (_f == -1)
            return;
        if (_f == 0)
            _f = _memory[4] & 0x1f;
        _memory[_f] = _data;
        if (_f == 6 || _f == 0x20) {
            UInt8 h = _memory[6] | _memory[0x20];
            if ((h & 0x10) != (_io & 0x10))
                _simulation->write(_t, _connectedBar[0], _connectedDirection[0], (h & 0x10) != 0);
            if ((h & 0x20) != (_io & 0x20))
                _simulation->write(_t, _connectedBar[1], _connectedDirection[1], (h & 0x20) != 0);
            if ((h & 1) != (_io & 1))
                _simulation->write(_t, _connectedBar[2], _connectedDirection[2], (h & 1) != 0);
            if ((h & 2) != (_io & 2))
                _simulation->write(_t, _connectedBar[3], _connectedDirection[3], (h & 2) != 0);
            _io = h;
            if (_debug)
                printf("%*sWrote 0x%02x\n", _indent*8, "", h);
        }
        if (_f == 2)
            _pch = 0;
    }
    void connect(double t, int direction, int connectedBar, int connectedDirection)
    {
        _connectedBar[direction] = connectedBar;
        _connectedDirection[direction] = connectedDirection;
        if (_number != 0)
            updateLive(t);
    }
    bool read(double t, int direction)
    {
        simulateTo(t);
        switch (direction) {
            case 0: return (_io & 0x10) != 0;
            case 1: return (_io & 0x20) != 0;
            case 2: return (_io & 1) != 0;
            case 3: return (_io & 2) != 0;
        }
        return true;
    }
    void prime(int parent, int indent = 0)
    {
        _indent = indent;
        _primed = true;
        int childB = (parent + 1) & 3;
        int childC = (parent + 2) & 3;
        int childD = (parent + 3) & 3;
        int childBbar = _connectedBar[childB];
        _childBpresent = (childBbar != -1);
        if (_childBpresent) {
            Bar* bar = _simulation->bar(childBbar);
            if (bar->primed())
                _childBpresent = false;
            else
                bar->prime(_connectedDirection[childB], indent + 1);
        }
        int childCbar = _connectedBar[childC];
        _childCpresent = (childCbar != -1);
        if (_childCpresent) {
            Bar* bar = _simulation->bar(childCbar);
            if (bar->primed())
                _childCpresent = false;
            else
                bar->prime(_connectedDirection[childC], indent + 1);
        }
        int childDbar = _connectedBar[childD];
        _childDpresent = (childDbar != -1);
        if (_childDpresent) {
            Bar* bar = _simulation->bar(childDbar);
            if (bar->primed())
                _childDpresent = false;
            else
                bar->prime(_connectedDirection[childD], indent + 1);
        }
    }
    int* storeExpectedStream(int parent, int* store)
    {
        int childB = (parent + 1) & 3;
        int childC = (parent + 2) & 3;
        int childD = (parent + 3) & 3;
        if (_number != 0) {
            *(store++) = 0;
            *(store++) = 0;
            *(store++) = 0;
            *(store++) = parent&1;
            *(store++) = 1;
            *(store++) = _childBpresent ? 1 : 0;
            *(store++) = _childCpresent ? 1 : 0;
            *(store++) = _childDpresent ? 1 : 0;
        }
        if (_childBpresent)
            store = _simulation->bar(_connectedBar[childB])->storeExpectedStream(_connectedDirection[childB], store);
        if (_childCpresent)
            store = _simulation->bar(_connectedBar[childC])->storeExpectedStream(_connectedDirection[childC], store);
        if (_childDpresent)
            store = _simulation->bar(_connectedBar[childD])->storeExpectedStream(_connectedDirection[childD], store);
        _primed = false;
        return store;
    }
    int connectedBar(int direction) const { return _connectedBar[direction]; }
    int connectedDirection(int direction) const { return _connectedDirection[direction]; }
    double time() const { return _t; }
    bool live() const { return _live; }
    void dumpConnections() const
    {
        if (!_live)
            return;
        printf("%03i: ", _number);
        for (int i = 0; i < 4; ++i)
            if (_connectedBar[i] == -1)
                printf("- ");
            else
                printf("%03i/%i ", _connectedBar[i], _connectedDirection[i]);
        printf("\n");
    }
private:
    UInt8 readMemory(int address, UInt8 care = 0xff)
    {
        if (address == 0)
            address = _memory[4] & 0x1f;
        if (address == 6) {
            UInt8 r = 8;
            if ((care & 1) != 0)
                if ((_memory[0x20] & 1) == 0)
                    r |= (_memory[6] & 1);
                else
                    r |= (_simulation->read(_t, _connectedBar[2], _connectedDirection[2]) ? 1 : 0);
            if ((care & 2) != 0)
                if ((_memory[0x20] & 2) == 0)
                    r |= (_memory[6] & 2);
                else
                    r |= (_simulation->read(_t, _connectedBar[3], _connectedDirection[3]) ? 2 : 0);
            if ((_memory[0x20] & 4) == 0)
                r |= (_memory[6] & 4);
            else
                r |= 4;  // Switch not implemented for now
            if ((care & 0x10) != 0)
                if ((_memory[0x20] & 0x10) == 0)
                    r |= (_memory[6] & 0x10);
                else
                    r |= (_simulation->read(_t, _connectedBar[0], _connectedDirection[0]) ? 0x10 : 0);
            if ((care & 0x20) != 0)
                if ((_memory[0x20] & 0x20) == 0)
                    r |= (_memory[6] & 0x20);
                else
                    r |= (_simulation->read(_t, _connectedBar[1], _connectedDirection[1]) ? 0x20 : 0);
            if (_debug)
                printf("%*sRead 0x%02x\n", _indent*8, "", r);
            return r;
        }
        return _memory[address];
    }
    void unrecognizedOpcode(int op)
    {
        static String unrecognized("Unrecognized opcode 0x");
        throw Exception(unrecognized + String::hexadecimal(op, 3));
    }
    void store(UInt16 r, bool d)
    {
        if (d)
            _data = static_cast<UInt8>(r);
        else {
            _f = -1;
            _w = static_cast<UInt8>(r);
        }
    }
    void storeZ(UInt16 r, bool d)
    {
        store(r, d);
        if (r == 0)
            _memory[3] |= 4;
        else
            _memory[3] &= 0xfb;
    }
    void setCarry(bool carry) { _memory[3] = (_memory[3] & 0xfe) | (carry ? 1 : 0); }
    void incrementPC()
    {
        ++_memory[2];
        if (_memory[2] == 0)
            _pch ^= 0x100;
    }
    bool primed() const { return _primed; }
    void reset()
    {
        for (int i = 0; i < 0x20; ++i)
            _memory[i] = 0;
        _memory[2] = 0xff;
        _memory[3] = 0x18;
        _memory[4] = 0xe0;
        _memory[5] = 0xfe;
        _memory[0x20] = 0x3f;
        _io = 0x3f;
        _option = 0xff;
        _pch = 0x100;
        _readSubCycle = true;
    }
    void updateLive(double t)
    {
        // Update live flag
        bool newLive = false;
        for (int i = 0; i < 4; ++i)
            if (_connectedBar[i] != -1 && _simulation->bar(_connectedBar[i])->live()) {
                newLive = true;
                break;
            }
        if (!_live && newLive) {
            _t = t;
            reset();
        }
        if (newLive != _live) {
            _live = newLive;
            for (int i = 0; i < 4; ++i) {
                int n = _connectedBar[i];
                if (n != -1)
                    _simulation->bar(n)->updateLive(t);
            }
        }
    }

    Simulation* _simulation;
    const Program* _program;
    UInt8 _memory[0x21];
    UInt8 _option;
    int _stack[2];
    int _pch;
    UInt8 _w;
    bool _skipping;
    double _tPerCycle;
    int _connectedBar[4];
    int _connectedDirection[4];
    double _t;
    int _number;
    int _indent;
    bool _debug;
    UInt8 _io;
    int _f;
    UInt8 _data;
    bool _primed;
    bool _childBpresent;
    bool _childCpresent;
    bool _childDpresent;
    bool _readSubCycle;
    bool _live;
    Handle _console;
};

typedef BarTemplate<Simulation> Bar;

class Simulation
{
public:
    Simulation()
      : _totalBars(100), _stream(_totalBars*8), _expectedStream(_totalBars*8), _badStreamsOk(100)
    { }
    void simulate()
    {
        Program intervalProgram(String("../intervals.HEX"), String("../intervals.annotation"));
        intervalProgram.load();

        Program rootProgram(String("../root.HEX"), String("../root.annotation"));
        rootProgram.load();

        Bar* root;
        for (int i = 0; i <= _totalBars; ++i) {
            Reference<Bar> bar;
            bar = new Bar(this, (i == 0 ? &rootProgram : &intervalProgram), i, true);
            if (i == 0)
                root = bar;
            _bars.push_back(bar);
        }

        //_bars[0]->connect(0, 2, 1, 0);
        //_bars[1]->connect(0, 0, 0, 2);

        _streamPointer = &_stream[0];
        _connectedPairs = 0;
        int tt = 0;
        do {
            root->simulateSubcycle();
            double t = root->time();
            if (t > 256) {
                for (std::vector<Reference<Bar> >::iterator i = _bars.begin(); i != _bars.end(); ++i)
                    (*i)->adjustTime(256.0);
                t -= 256;
                ++tt;
                if (tt == 4096) {
                    printf("%i ", _liveBars);
                    tt = 0;
                }
            }
            if (rand() % 1000 == 0) {
                // Bring all bars up to date
                for (int i = 1; i <= _totalBars; ++i)
                    if (_bars[i]->live())
                        _bars[i]->simulateTo(t);

                //_badStreamsOk = 1;
                _badStreamsOk = 100;
                int n = rand() % (4*_totalBars + 1);
                int barNumber = (n - 1)/4 + 1;
                int connectorNumber = (n - 1)%4;
                if (n == 0) {
                    barNumber = 0;
                    connectorNumber = 2;
                }
                Bar* bar = _bars[barNumber];
                int connectedBarNumber = bar->connectedBar(connectorNumber);
                int connectedDirection = bar->connectedDirection(connectorNumber);
                if (connectedBarNumber == -1 && _connectedPairs < _totalBars*2) {
                    // This connector is not connected - connect it to a random disconnected connector of the opposite gender
                    Bar* otherBar;
                    if (connectorNumber == 0 || connectorNumber == 3) {
                        // This is a male connector.
                        n = rand() % (1 + 2*_totalBars - _connectedPairs);
                        for (connectedBarNumber = 0; connectedBarNumber <= _totalBars; ++connectedBarNumber) {
                            otherBar = _bars[connectedBarNumber];
                            if (connectedBarNumber > 0 && otherBar->connectedBar(1) == -1) {
                                if (n == 0) {
                                    connectedDirection = 1;
                                    break;
                                }
                                --n;
                            }
                            if (otherBar->connectedBar(2) == -1) {
                                if (n == 0) {
                                    connectedDirection = 2;
                                    break;
                                }
                                --n;
                            }
                        }
                    }
                    else {
                        // This is a female connector.
                        n = rand() % (2*_totalBars - _connectedPairs);
                        for (connectedBarNumber = 1; connectedBarNumber <= _totalBars; ++connectedBarNumber) {
                            otherBar = _bars[connectedBarNumber];
                            if (otherBar->connectedBar(0) == -1) {
                                if (n == 0) {
                                    connectedDirection = 0;
                                    break;
                                }
                                --n;
                            }
                            if (otherBar->connectedBar(3) == -1) {
                                if (n == 0) {
                                    connectedDirection = 3;
                                    break;
                                }
                                --n;
                            }
                        }
                    }
                    printf("***Connecting bar %i direction %i to bar %i direction %i\n", barNumber, connectorNumber, connectedBarNumber, connectedDirection);
                    bar->connect(t, connectorNumber, connectedBarNumber, connectedDirection);
                    otherBar->connect(t, connectedDirection, barNumber, connectorNumber);
                    ++_connectedPairs;
                }
                else {
                    // This connector is connected - disconnect it.
                    printf("***Disconnecting bar %i direction %i from bar %i direction %i\n", barNumber, connectorNumber, connectedBarNumber, connectedDirection);
                    Bar* connectedBar = _bars[connectedBarNumber];
                    bar->connect(t, connectorNumber, -1, 0);
                    connectedBar->connect(t, connectedDirection, -1, 0);
                    --_connectedPairs;
                }
                // Prime to update _indent
                _bars[0]->prime(0);
                _bars[0]->storeExpectedStream(0, &_expectedStream[0]);
                for (int i = 0; i <= _totalBars; ++i)
                    _bars[i]->dumpConnections();
            }
        } while (true);
    }
    void streamBit(bool bit)
    {
        *(_streamPointer++) = bit;
    }
    void streamStart()
    {
//        if (!_badStreamOk) {
            int* streamPointer = &_stream[0];
            int* expectedStreamPointer = &_expectedStream[0];
            _bars[0]->prime(0);
            int* expectedStreamPointerEnd = _bars[0]->storeExpectedStream(0, expectedStreamPointer);
            bool good = true;
            _liveBars = 0;
            do {
                if (streamPointer == _streamPointer) {
                    if (expectedStreamPointer == expectedStreamPointerEnd)
                        break;
                    good = false;
                    break;
                }
                if (expectedStreamPointer == expectedStreamPointerEnd) {
                    good = false;
                    break;
                }
                if ((*streamPointer) != (*expectedStreamPointer)) {
                    good = false;
                    break;
                }
                ++streamPointer;
                ++expectedStreamPointer;
                ++_liveBars;
            } while (true);
            int i;
            if (!good) {
                if (_badStreamsOk > 0)
                    printf("Ignored: ");
                printf("Bad stream. Expected ");
                int i;
                for (i = 0, expectedStreamPointer = &_expectedStream[0]; expectedStreamPointer != expectedStreamPointerEnd; ++expectedStreamPointer, ++i) {
                    if ((i % 8) == 0)
                        printf(" ");
                    printf("%i", *expectedStreamPointer);
                }
                printf(", observed ");
                for (i = 0, streamPointer = &_stream[0]; streamPointer != _streamPointer; ++streamPointer, ++i) {
                    if ((i % 8) == 0)
                        printf(" ");
                    printf("%i", *streamPointer);
                }
                printf("\n");
                if (_badStreamsOk == 0)
                    exit(0);
            }
            else {
                _badStreamsOk = 0;
                printf("Good stream: ");
                for (i = 0, streamPointer = &_stream[0]; streamPointer != _streamPointer; ++streamPointer, ++i) {
                    if ((i % 8) == 0)
                        printf(" ");
                    printf("%i", *streamPointer);
                }
                printf("\n");
            }
//        }
        _streamPointer = &_stream[0];
        if (_badStreamsOk > 0)
            --_badStreamsOk;
    }

    bool read(double t, int bar, int direction)
    {
        if (bar == -1)
            return true;
        return _bars[bar]->read(t, direction);
    }
    void write(double t, int bar, int direction, bool value)
    {
        if (bar == -1)
            return;
        _bars[bar]->simulateTo(t);
    }
    Bar* bar(int n) { return _bars[n]; }
private:
    std::vector<Reference<Bar> > _bars;
    int _totalBars;
    std::vector<int> _stream;
    std::vector<int> _expectedStream;
    int* _streamPointer;
    int _totalConnected;
    int _badStreamsOk;
    int _connectedPairs;
    int _liveBars;
};

int main()
{
	BEGIN_CHECKED {
        setbuf(stdout, NULL);
        Simulation simulation;
        simulation.simulate();
	}
	END_CHECKED(Exception& e) {
		e.write(Handle::consoleOutput());
	}
}
