#include "alfe/string.h"
#include "alfe/file.h"
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include "alfe/main.h"

class SimulatedProgram
{
public:
    SimulatedProgram(String fileName)
    {
        File file(fileName);
        String contents = file.contents();
        _source = CharacterSource(contents, fileName);
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
        Location byteCountLocation = _source.location();
        int byteCount = readByte();
        int address = readByte() << 8;
        address |= readByte();
        Location recordTypeLocation = _source.location();
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
                if (byteCount != 0)
                    byteCountLocation.throwError(String("End of file marker "
                        "incorrect. Expected no data, found ") + byteCount +
                        " bytes.");
                _done = true;
                break;
            case 4:  // extended linear address record
                break;
            default:
                recordTypeLocation.throwError(
                    String("Don't know what to do with record type ") +
                    recordType);
        }
        Location checkSumLocation = _source.location();
        int checkSum = readByte();
        if ((_checkSum & 0xff) != 0) {
            static String found(", found ");
            checkSumLocation.throwError("Checksum incorrect. Expected " +
                hex((checkSum - _checkSum) & 0xff, 2) + ", found " +
                hex(checkSum, 2));
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
        start.throwUnexpected("0-9 or A-F");
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
    BarTemplate(Simulation* simulation, const SimulatedProgram* program,
        int number, bool debug)
      : _simulation(simulation), _program(program), _t(0), _debug(debug),
        _skipping(false), _number(number), _indent(0)
    {
        reset();
    }
    void simulateTo(int t)
    {
        if (_debug)
            printf("%*sSimulating bar %i to %lf\n", _indent*8, "", _number,
                t/(400.0*256.0));
        do {
            if (_tNextStop >= t)
                break;
            _t = _tNextStop;
            switch (_state) {
                case 0:
                    // 0.00 to 0.25: read
                    simulateToRead();
                    _state = 1;
                    _tNextStop += 3*_tPerQuarterCycle;
                    ++_cyclesSinceLastSync;
                    break;
                case 1:
                    // 0.25 to 1.00: write
                    simulateToWrite();
                    if (_skipping) {
                        _skipping = false;
                        _state = 2;
                        _tNextStop += 4*_tPerQuarterCycle;
                    }
                    else {
                        _state = 0;
                        _tNextStop += _tPerQuarterCycle;
                    }
                    break;
                case 2:
                    // 1.00 to 2.00: skip
                    _state = 0;
                    _tNextStop += _tPerQuarterCycle;
                    ++_cyclesSinceLastSync;
                    break;
            }
        } while (true);
        if (t > _t)
            _t = t;
    }
    void resetTime() { _tNextStop -= _t; _t = 0; }
    void simulateToRead()
    {
        if (_debug)
            printf("%*s% 7.2lf ", _indent*8, "", _tNextStop/(400.0*256.0));
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
                                    if (_debug) { printf("NOP             "); printf("\n"); }
                                    _f = -1;
                                    break;
                                case 2:
                                    if (_debug) { printf("OPTION          "); printf("\n"); }
                                    _f = -1;
                                    _option = _w;
                                    break;
                                case 3:
                                    if (_debug) { printf("SLEEP           "); printf("\n"); }
                                    _f = -1;
                                    throw Exception(String("SLEEP not supported"));
                                    break;
                                case 4:
                                    if (_debug) { printf("CLRWDT          "); printf("\n"); }
                                    _f = -1;
                                    throw Exception(String("CLRWDT not supported"));
                                    break;
                                case 6:
                                    if (_debug) { printf("TRIS GPIO       "); printf("\n"); }
                                    _f = 0x20;
                                    _data = _w;
                                    break;
                                default:
                                    unrecognizedOpcode(op);
                                    break;
                            }
                        else {
                            if (_debug) { printf("MOVWF 0x%02x      ", _f); printf("\n"); }
                            _data = _w;
                        }
                        break;
                    case 0x1:
                        if (_debug) {
                            if (!d)
                                printf("CLRW            ");
                            else
                                printf("CLRF 0x%02x       ", _f);
                             printf("\n");
                        }
                        storeZ(0, d);
                        break;
                    case 0x2:
                        if (_debug) { printf("SUBWF 0x%02x, %c   ", _f, dc); printf("\n"); }
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
                        if (_debug) { printf("DECF 0x%02x, %c    ", _f, dc); printf("\n"); }
                        storeZ(readMemory(_f) - 1, d);
                        break;
                    case 0x4:
                        if (_debug) { printf("IORWF 0x%02x, %c   ", _f, dc); printf("\n"); }
                        storeZ(readMemory(_f) | _w, d);
                        break;
                    case 0x5:
                        if (_debug) { printf("ANDWF 0x%02x, %c   ", _f, dc); printf("\n"); }
                        storeZ(readMemory(_f) & _w, d);
                        break;
                    case 0x6:
                        if (_debug) { printf("XORWF 0x%02x, %c   ", _f, dc); printf("\n"); }
                        storeZ(readMemory(_f) ^ _w, d);
                        break;
                    case 0x7:
                        if (_debug) { printf("ADDWF 0x%02x, %c   ", _f, dc); printf("\n"); }
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
                        if (_debug) { printf("MOVF 0x%02x, %c    ", _f, dc); printf("\n"); }
                        storeZ(readMemory(_f), d);
                        break;
                    case 0x9:
                        if (_debug) { printf("COMF 0x%02x, %c    ", _f, dc); printf("\n"); }
                        storeZ(~readMemory(_f), d);
                        break;
                    case 0xa:
                        if (_debug) { printf("INCF 0x%02x, %c    ", _f, dc); printf("\n"); }
                        storeZ(readMemory(_f) + 1, d);
                        break;
                    case 0xb:
                        if (_debug) { printf("DECFSZ 0x%02x, %c  ", _f, dc); printf("\n"); }
                        r = readMemory(_f) - 1;
                        store(r, d);
                        if (r == 0) {
                            incrementPC();
                            _skipping = true;
                        }
                        break;
                    case 0xc:
                        if (_debug) { printf("RRF 0x%02x, %c     ", _f, dc); printf("\n"); }
                        r = readMemory(_f) | ((_memory[3] & 1) << 8);
                        setCarry((r & 1) != 0);
                        store(r >> 1, d);
                        break;
                    case 0xd:
                        if (_debug) { printf("RLF 0x%02x, %c     ", _f, dc); printf("\n"); }
                        r = (readMemory(_f) << 1) | (_memory[3] & 1);
                        setCarry((r & 0x100) != 0);
                        store(r, d);
                        break;
                    case 0xe:
                        if (_debug) { printf("SWAPF 0x%02x, %c   ", _f, dc); printf("\n"); }
                        r = readMemory(_f);
                        store((r >> 4) | (r << 4), d);
                        break;
                    case 0xf:
                        if (_debug) { printf("INCFSF 0x%02x, %c  ", _f, dc); printf("\n"); }
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
                        if (_debug) { printf("BCF 0x%02x, %i     ", _f, b); printf("\n"); }
                        _data = readMemory(_f) & ~m;
                        break;
                    case 5:
                        if (_debug) { printf("BSF 0x%02x, %i     ", _f, b); printf("\n"); }
                        _data = readMemory(_f) | m;
                        break;
                    case 6:
                        if (_debug) { printf("BTFSC 0x%02x, %i   ", _f, b); printf("\n"); }
                        if ((readMemory(_f, m) & m) == 0) {
                            incrementPC();
                            _skipping = true;
                        }
                        _f = -1;
                        break;
                    case 7:
                        if (_debug) { printf("BTFSS 0x%02x, %i   ", _f, b); printf("\n"); }
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
                    if (_debug) { printf("RETLW 0x%02x      ", d); printf("\n"); }
                    _skipping = true;
                    _memory[2] = _stack[0];
                    _pch = _stack[0] & 0x100;
                    _stack[0] = _stack[1];
                    _w = d;
                    break;
                case 0x9:
                    if (_debug) { printf("CALL  0x%02x      ", d); printf("\n"); }
                    _skipping = true;
                    _stack[1] = _stack[0];
                    _stack[0] = _memory[2] | _pch;
                    _pch = 0;
                    _memory[2] = d;
                    break;
                case 0xa:
                case 0xb:
                    if (_debug) { printf("GOTO  0x%03x     ", op & 0x1ff); printf("\n"); }
                    _skipping = true;
                    _pch = op & 0x100;
                    _memory[2] = d;
                    break;
                case 0xc:
                    if (_debug) { printf("MOVLW 0x%02x      ", d); printf("\n"); }
                    _w = d;
                    break;
                case 0xd:
                    if (_debug) { printf("IORLW 0x%02x      ", d); printf("\n"); }
                    storeZ(_w | d, false);
                    break;
                case 0xe:
                    if (_debug) { printf("ANDLW 0x%02x      ", d); printf("\n"); }
                    storeZ(_w & d, false);
                    break;
                case 0xf:
                    if (_debug) { printf("XORLW 0x%02x      ", d); printf("\n"); }
                    storeZ(_w ^ d, false);
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
            _io = h;
            if (_debug) {
                printf("%*sWrote 0x%02x (0x%02x | 0x%02x)\n", _indent*8, "", h,
                    _memory[6], _memory[0x20]);
                if (_f == 6 && _data != 0)
                    printf("%*sGPIO=0x%02x\n", _indent*8, "", _data);
            }
        }
        if (_f == 2)
            _pch = 0;
    }
    bool read(int t, int direction, char* readMarker)
    {
        simulateTo(t);
        switch (direction) {
            case 0:
                return (_io & 0x10) != 0;
            case 1:
                return (_io & 0x20) != 0;
            case 2:
                return (_io & 1) != 0;
            case 3:
                return (_io & 2) != 0;
        }
        return true;
    }
    int time() const { return _t; }
    bool live() const { return _live; }
    void clearLive() { _oldLive = _live; _live = false; }
    void resetNewlyConnected() { if (_live && !_oldLive) reset(); }
    void debug() { _debug = true; }
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
                    r |= 1;
            if ((care & 2) != 0)
                if ((_memory[0x20] & 2) == 0)
                    r |= (_memory[6] & 2);
                else
                    r |= 2;
            if ((_memory[0x20] & 4) == 0)
                r |= (_memory[6] & 4);
            else
                r |= 4;  // Switch not implemented for now
            if ((care & 0x10) != 0)
                if ((_memory[0x20] & 0x10) == 0)
                    r |= (_memory[6] & 0x10);
                else
                    r |= 0x10;
            if ((care & 0x20) != 0)
                if ((_memory[0x20] & 0x20) == 0)
                    r |= (_memory[6] & 0x20);
                else
                    r |= 0x20;
            if (_debug)
                printf("%*sRead 0x%02x\n", _indent*8, "", r);
            return r;
        }
        return _memory[address];
    }
    void unrecognizedOpcode(int op)
    {
        throw Exception("Unrecognized opcode " + hex(op, 3));
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
    void setCarry(bool carry)
    {
        _memory[3] = (_memory[3] & 0xfe) | (carry ? 1 : 0);
    }
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
        _t = 0;
        _tPerQuarterCycle = 100*256;
        _state = 0;
        _tNextStop = _tPerQuarterCycle;
    }

    Simulation* _simulation;
    const SimulatedProgram* _program;
    UInt8 _memory[0x21];
    UInt8 _option;
    int _stack[2];
    int _pch;
    UInt8 _w;
    bool _skipping;
    int _tPerQuarterCycle;
    int _t;
    int _tNextStop;
    int _number;
    int _indent;
    bool _debug;
    UInt8 _io;
    int _f;
    UInt8 _data;
    Handle _console;
    int _state;
    int _cyclesSinceLastSync;
};

typedef BarTemplate<Simulation> Bar;

class Simulation
{
public:
    Simulation() { }
    void simulate()
    {
        SimulatedProgram waveformProgram("../programmer/waveform.HEX");
        waveformProgram.load();

        _bar = new Bar(this, &waveformProgram, 0, true);
        for (int i = 0; i < 64; ++i) {
            _bar->simulateTo(256*400*256);
            _bar->resetTime();
        }
    }
private:
    Reference<Bar> _bar;
};

class Program : public ProgramBase
{
public:
    void run()
    {
        setbuf(stdout, NULL);
        Simulation simulation;
        simulation.simulate();
    }
};
