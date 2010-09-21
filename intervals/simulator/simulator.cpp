#include "unity/string.h"
#include "unity/file.h"
#include <vector>
#include <stdio.h>
#include <stdlib.h>

#include <time.h>

class Program
{
public:
    Program(String fileName)
    {
		File file(fileName);
		String contents = file.contents();
        _source = contents.start();
    }

    void load()
    {
        for (int i = 0; i < 0x400; ++i)
            _data[i] = 0;
        _done = false;
        while (!_done)
            parseLine();
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
private:
    UInt8 _data[0x400];
    bool _done;
    int _checkSum;
    CharacterSource _source;
};

class Simulation;

template<class Simulation> class BarTemplate : public ReferenceCounted
{
public:
    BarTemplate(Simulation* simulation, const Program* program, bool root = false)
      : _simulation(simulation), _program(program), _t(0), _root(root), _debug(false), _readSubCycle(true)
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
    }
    void simulateTo(double t)
    {
        if (_debug) {
            if (_root)
                printf("        ");
            printf("Simulating to %lf\n", t);
        }
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
        if (_debug) {
            if (_root)
                printf("        ");
            printf("%lf ", _t);
        }
        if (_readSubCycle) {
            _t += 0.25*_tPerCycle;
            simulateToRead();
            _readSubCycle = false;
        }
        else {
            _t += 0.75*_tPerCycle;
            simulateToWrite();
            _readSubCycle = true;
        }
    }
    void simulateToRead()
    {
        int op = _program->op(_pch | _memory[2]);
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
                                    if (_debug) printf("NOP\n");
                                    _f = -1;
                                    break;
                                case 2:
                                    if (_debug) printf("OPTION\n");
                                    _f = -1;
                                    _option = _w;
                                    break;
                                case 3:
                                    if (_debug) printf("SLEEP\n");
                                    _f = -1;
                                    throw Exception(String("SLEEP not supported"));
                                    break;
                                case 4:
                                    if (_debug) printf("CLRWDT\n");
                                    _f = -1;
                                    throw Exception(String("CLRWDT not supported"));
                                    break;
                                case 5:  // Not a real PIC12F508 opcode - used for simulator escape (data)
                                    _f = -1;
                                    _simulation->streamBit((_w & 1) != 0);
                                    break;
                                case 6:
                                    if (_debug) printf("TRIS GPIO\n");
                                    _f = 0x20;
                                    _data = _w;
                                    break;
                                case 7:  // Not a real PIC12F08 opcode - used for simulator escape (space)
                                    _f = -1;
                                    _simulation->streamStart();
                                    break;
                                default:
                                    unrecognizedOpcode(op);
                                    break;
                            }
                        else {
                            if (_debug) printf("MOVWF 0x%02x\n", _f);
                            _data = _w;
                        }
                        break;
                    case 0x1:
                        if (_debug)
                            if (!d)
                                printf("CLRW\n");
                            else
                                printf("CLRF 0x%02x\n", _f);
                        storeZ(0, d);
                        break;
                    case 0x2:
                        if (_debug) printf("SUBWF 0x%02x, %c\n", _f, dc);
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
                        if (_debug) printf("DECF 0x%02x, %c\n", _f, dc);
                        storeZ(readMemory(_f) - 1, d);
                        break;
                    case 0x4:
                        if (_debug) printf("IORWF 0x%02x, %c\n", _f, dc);
                        storeZ(readMemory(_f) | _w, d);
                        break;
                    case 0x5:
                        if (_debug) printf("ANDWF 0x%02x, %c\n", _f, dc);
                        storeZ(readMemory(_f) & _w, d);
                        break;
                    case 0x6:
                        if (_debug) printf("XORWF 0x%02x, %c\n", _f, dc);
                        storeZ(readMemory(_f) ^ _w, d);
                        break;
                    case 0x7:
                        if (_debug) printf("ADDWF 0x%02x, %c\n", _f, dc);
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
                        if (_debug) printf("MOVF 0x%02x, %c\n", _f, dc);
                        storeZ(readMemory(_f), d);
                        break;
                    case 0x9:
                        if (_debug) printf("COMF 0x%02x, %c\n", _f, dc);
                        storeZ(~readMemory(_f), d);
                        break;
                    case 0xa:
                        if (_debug) printf("INCF 0x%02x, %c\n", _f, dc);
                        storeZ(readMemory(_f) + 1, d);
                        break;
                    case 0xb:
                        if (_debug) printf("DECFSZ 0x%02x, %c\n", _f, dc);
                        r = readMemory(_f) - 1;
                        store(r, d);
                        if (r == 0) {
                            incrementPC();
                            _skipping = true;
                        }
                        break;
                    case 0xc:
                        if (_debug) printf("RRF 0x%02x, %c\n", _f, dc);
                        r = readMemory(_f) | ((_memory[3] & 1) << 8);
                        setCarry((r & 1) != 0);
                        store(r >> 1, d);
                        break;
                    case 0xd:
                        if (_debug) printf("RLF 0x%02x, %c\n", _f, dc);
                        r = (readMemory(_f) << 1) | (_memory[3] & 1);
                        setCarry((r & 0x100) != 0);
                        store(r, d);
                        break;
                    case 0xe:
                        if (_debug) printf("SWAPF 0x%02x, %c\n", _f, dc);
                        r = readMemory(_f);
                        store((r >> 4) | (r << 4), d);
                        break;
                    case 0xf:
                        if (_debug) printf("INCFSF 0x%02x, %c\n", _f, dc);
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
                        if (_debug) printf("BCF 0x%02x, %i\n", _f, b);
                        _data = readMemory(_f) & ~m;
                        break;
                    case 5:
                        if (_debug) printf("BSF 0x%02x, %i\n", _f, b);
                        _data = readMemory(_f) | m;
                        break;
                    case 6:
                        if (_debug) printf("BTFSC 0x%02x, %i\n", _f, b);
                        if ((readMemory(_f, m) & m) == 0) {
                            incrementPC();
                            _skipping = true;
                        }
                        break;
                    case 7:
                        if (_debug) printf("BTFSS 0x%02x, %i\n", _f, b);
                        if ((readMemory(_f, m) & m) != 0) {
                            incrementPC();
                            _skipping = true;
                        }
                        break;
                }
            }
        }
        else {
            _f = -1;
            int d = op & 0xff;
            switch (op >> 8) {
                case 0x8:
                    if (_debug) printf("RETLW 0x%02x\n", d);
                    _skipping = true;
                    _memory[2] = _stack[0];
                    _pch = _stack[0] & 0x100;
                    _stack[0] = _stack[1];
                    _w = d;
                    break;
                case 0x9:
                    if (_debug) printf("CALL 0x%02x\n", d);
                    _skipping = true;
                    _stack[1] = _stack[0];
                    _stack[0] = _memory[2] | _pch;
                    _pch = 0;
                    _memory[2] = d;
                    break;
                case 0xa:
                case 0xb:
                    if (_debug) printf("GOTO 0x%03x\n", op & 0x1ff);
                    _skipping = true;
                    _pch = op & 0x100;
                    _memory[2] = d;
                    break;
                case 0xc:
                    if (_debug) printf("MOVLW 0x%02x\n", d);
                    _w = d;
                    break;
                case 0xd:
                    if (_debug) printf("IORLW 0x%02x\n", d);
                    _w |= d;
                    break;
                case 0xe:
                    if (_debug) printf("ANDLW 0x%02\n", d);
                    _w &= d;
                    break;
                case 0xf:
                    if (_debug) printf("XORLW 0x%02x\n", d);
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
            if (_debug) {
                if (_root)
                    printf("        ");
                printf("Wrote 0x%02x\n", h);
            }
        }
        if (_f == 2)
            _pch = 0;
    }
    void connect(int direction, int connectedBar, int connectedDirection)
    {
        _connectedBar[direction] = connectedBar;
        _connectedDirection[direction] = connectedDirection;
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
    void prime(int parent)
    {
        _primed = true;
        int childB = (parent + 1) & 3;
        int childC = (parent + 2) & 3;
        int childD = (parent + 3) & 3;
        _childBpresent = (childB != -1);
        if (_childBpresent) {
            Bar* bar = _simulation->bar(_connectedBar[childB]);
            if (bar->primed())
                _childBpresent = false;
            else
                bar->prime(_connectedDirection[childB]);
        }
        _childCpresent = (childC != -1);
        if (_childCpresent) {
            Bar* bar = _simulation->bar(_connectedBar[childC]);
            if (bar->primed())
                _childCpresent = false;
            else
                bar->prime(_connectedDirection[childC]);
        }
        _childDpresent = (childD != -1);
        if (_childDpresent) {
            Bar* bar = _simulation->bar(_connectedBar[childD]);
            if (bar->primed())
                _childDpresent = false;
            else
                bar->prime(_connectedDirection[childD]);
        }
    }
    int* storeExpectedStream(int parent, int* store)
    {
        int childB = (parent + 1) & 3;
        int childC = (parent + 2) & 3;
        int childD = (parent + 3) & 3;
        if (!_root) {
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
            if (_debug) {
                if (_root)
                    printf("        ");
                printf("Read 0x%02x\n", r);
            }
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
    bool _root;
    bool _debug;
    UInt8 _io;
    int _f;
    UInt8 _data;
    bool _primed;
    bool _childBpresent;
    bool _childCpresent;
    bool _childDpresent;
    bool _readSubCycle;
};

typedef BarTemplate<Simulation> Bar;

class Simulation
{
public:
    Simulation()
      : _totalBars(100), _stream(_totalBars*8), _expectedStream(_totalBars*8), _t(0), _badStreamOk(false)
    { }
    void simulate()
    {
        Program intervalProgram(String("../intervals.HEX"));
        intervalProgram.load();

        Program rootProgram(String("../root.HEX"));
        rootProgram.load();

        Bar* root;
        for (int i = 0; i <= _totalBars; ++i) {
            Reference<Bar> bar;
            if (i == 0) {
                bar = new Bar(this, &rootProgram);
                root = bar;
            }
            else
                bar = new Bar(this, &intervalProgram, true);
            _bars.push_back(bar);
            bar->connect(0, -1, 0);
            bar->connect(1, -1, 0);
            bar->connect(2, -1, 0);
            bar->connect(3, -1, 0);
        }
        _streamPointer = &_stream[0];
        _connectedPairs = 0;
        do {
            root->simulateSubcycle();
            ++_t;
            if (_t == 256) {
                _t = 0;
                for (std::vector<Reference<Bar> >::iterator i = _bars.begin(); i != _bars.end(); ++i)
                    (*i)->adjustTime(256.0);
            }
            if (rand() % 1000 == 0) {
                _badStreamOk = true;
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
                    bar->connect(connectorNumber, connectedBarNumber, connectedDirection);
                    otherBar->connect(connectedDirection, barNumber, connectorNumber);
                }
                else {
                    // This connector is connected - disconnect it.
                    Bar* connectedBar = _bars[connectedBarNumber];
                    bar->connect(connectorNumber, -1, 0);
                    connectedBar->connect(connectedDirection, -1, 0);
                    --_connectedPairs;
                }
            }
        } while (true);
    }
    void streamBit(bool bit)
    {
        *(_streamPointer++) = bit;
    }
    void streamStart()
    {
        if (!_badStreamOk) {
            int* streamPointer = &_stream[0];
            int* expectedStreamPointer = &_expectedStream[0];
            _bars[0]->prime(0);
            int* expectedStreamPointerEnd = _bars[0]->storeExpectedStream(0, expectedStreamPointer);
            bool good = true;
            do {
                if ((*streamPointer) != (*expectedStreamPointer)) {
                    good = false;
                    break;
                }
                ++streamPointer;
                ++expectedStreamPointer;
                if ((streamPointer == _streamPointer) != (expectedStreamPointer == expectedStreamPointerEnd)) {
                    good = false;
                    break;
                }
            } while (true);
            if (!good) {
                printf("Bad stream. Expected ");
                for (expectedStreamPointer = &_expectedStream[0]; expectedStreamPointer != expectedStreamPointerEnd; ++expectedStreamPointer)
                    printf("%i", *expectedStreamPointer);
                printf(", observed ");
                for (streamPointer = &_stream[0]; streamPointer != _streamPointer; ++streamPointer)
                    printf("%i", *streamPointer);
                printf("\n");
            }
        }
        _streamPointer = &_stream[0];
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
    int _t;
    bool _badStreamOk;
    int _connectedPairs;
};

int main()
{
	BEGIN_CHECKED {
        Simulation simulation;
        simulation.simulate();
	}
	END_CHECKED(Exception& e) {
		e.write(Handle::consoleOutput());
	}
}
