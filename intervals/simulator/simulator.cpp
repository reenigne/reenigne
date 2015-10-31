#include "alfe/string.h"
#include "alfe/file.h"
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "alfe/main.h"

//#define DUMP

class SimulatedProgram
{
public:
    SimulatedProgram(String fileName, String annotationsFileName)
    {
        File file(fileName);
        String contents = file.contents();
        _source = CharacterSource(contents, fileName);
        File annotationsFile(annotationsFileName);
        _annotations = annotationsFile.contents();
        _annotationsFileName = annotationsFileName;
    }

    void load()
    {
        for (int i = 0; i < 0x400; ++i)
            _data[i] = 0;
        _done = false;
        while (!_done)
            parseLine();
        int position = 0;
        CharacterSource annotations = CharacterSource(_annotations,
            _annotationsFileName);
        for (int i = 0; i < 0x200; ++i) {
            int end;
            int col = 0;
            int marker = 0;
            do {
                int c = annotations.get();
                if (c == 10) {
                    end = annotations.offset() - 1;
                    break;
                }
                if (c == -1) {
                    end = annotations.offset();
                    break;
                }
                ++col;
                if (col == 50)
                    marker = annotations.offset();
            } while (true);
            _annotation.push_back(_annotations.subString(position, end - position));
            position = annotations.offset();
            if (marker == 0)
                _markers.push_back("");
            else
                _markers.push_back(_annotations.subString(marker, end - marker));
        }
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
        if ((_checkSum & 0xff) != 0)
            checkSumLocation.throwError("Checksum incorrect. Expected " +
                hex((checkSum - _checkSum) & 0xff, 2) + ", found " +
                hex(checkSum, 2));
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
    String annotation(int line) const { return _annotation[line]; }
    String marker(int line) const { return _markers[line]; }
private:
    UInt8 _data[0x400];
    bool _done;
    int _checkSum;
    CharacterSource _source;
    String _annotations;
    std::vector<String> _annotation;
    std::vector<String> _markers;
    String _annotationsFileName;
};

class Simulation;

template<class Simulation> class BarTemplate : public Handle::Body
{
public:
    BarTemplate(Simulation* simulation, const SimulatedProgram* program,
        int number, bool debug)
      : _simulation(simulation), _program(program), _t(0), _debug(debug),
        _skipping(false), _primed(false), _live(number == 0), _number(number),
        _indent(0)
    {
        reset();
        for (int i = 0; i < 4; ++i)
            _connectedBar[i] = -1;
        _child = 0;
        _parent = 0;
    }
    void simulateTo(int t)
    {
        if (!_live)
            return;
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
#ifdef DUMP
        String markerCode = _program->marker(pc);
#endif
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
                                    if (_debug) { printf("NOP             "); debug->write(_program->annotation(pc)); printf("\n"); }
                                    _f = -1;
                                    break;
                                case 2:
                                    if (_debug) { printf("OPTION          "); debug->write(_program->annotation(pc)); printf("\n"); }
                                    _f = -1;
                                    _option = _w;
                                    break;
                                case 3:
                                    if (_debug) { printf("SLEEP           "); debug->write(_program->annotation(pc)); printf("\n"); }
                                    _f = -1;
                                    throw Exception(String("SLEEP not supported"));
                                    break;
                                case 4:
                                    if (_debug) { printf("CLRWDT          "); debug->write(_program->annotation(pc)); printf("\n"); }
                                    _f = -1;
                                    throw Exception(String("CLRWDT not supported"));
                                    break;
                                case 5:  // Not a real PIC12F508 opcode - used for simulator escape (data)
                                    _f = -1;
                                    //if (_debug) { printf("%i               ", _w & 1); debug->write(_program->annotation(pc)); printf("\n"); }
                                    //_simulation->streamBit((_w & 1) != 0);
                                    //_skipping = true;
                                    {
                                        for (int i = 0; i < 8; ++i) {
                                            _simulation->streamBit((_memory[7+i] & 1) != 0);
                                            if (_debug)
                                                printf("%i", _memory[7+i] & 1);
                                        }
                                    }
                                    if (_debug) { printf("        "); debug->write(_program->annotation(pc)); printf("\n"); }
                                    break;
                                case 6:
                                    if (_debug) { printf("TRIS GPIO       "); debug->write(_program->annotation(pc)); printf("\n"); }
                                    _f = 0x20;
                                    _data = _w;
                                    break;
                                case 7:  // Not a real PIC12F08 opcode - used for simulator escape (space)
                                    _f = -1;
                                    if (_debug) { printf("---             "); debug->write(_program->annotation(pc)); printf("\n"); }
                                    _simulation->streamStart();
                                    break;
                                default:
                                    unrecognizedOpcode(op);
                                    break;
                            }
                        else {
                            if (_debug) { printf("MOVWF 0x%02x      ", _f); debug->write(_program->annotation(pc)); printf("\n"); }
                            _data = _w;
                        }
                        break;
                    case 0x1:
                        if (_debug) {
                            if (!d)
                                printf("CLRW            ");
                            else
                                printf("CLRF 0x%02x       ", _f);
                             debug->write(_program->annotation(pc));
                             printf("\n");
                        }
                        storeZ(0, d);
                        break;
                    case 0x2:
                        if (_debug) { printf("SUBWF 0x%02x, %c   ", _f, dc); debug->write(_program->annotation(pc)); printf("\n"); }
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
                        if (_debug) { printf("DECF 0x%02x, %c    ", _f, dc); debug->write(_program->annotation(pc)); printf("\n"); }
                        storeZ(readMemory(_f) - 1, d);
                        break;
                    case 0x4:
                        if (_debug) { printf("IORWF 0x%02x, %c   ", _f, dc); debug->write(_program->annotation(pc)); printf("\n"); }
                        storeZ(readMemory(_f) | _w, d);
                        break;
                    case 0x5:
                        if (_debug) { printf("ANDWF 0x%02x, %c   ", _f, dc); debug->write(_program->annotation(pc)); printf("\n"); }
                        storeZ(readMemory(_f) & _w, d);
                        break;
                    case 0x6:
                        if (_debug) { printf("XORWF 0x%02x, %c   ", _f, dc); debug->write(_program->annotation(pc)); printf("\n"); }
                        storeZ(readMemory(_f) ^ _w, d);
                        break;
                    case 0x7:
                        if (_debug) { printf("ADDWF 0x%02x, %c   ", _f, dc); debug->write(_program->annotation(pc)); printf("\n"); }
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
                        if (_debug) { printf("MOVF 0x%02x, %c    ", _f, dc); debug->write(_program->annotation(pc)); printf("\n"); }
                        storeZ(readMemory(_f), d);
                        break;
                    case 0x9:
                        if (_debug) { printf("COMF 0x%02x, %c    ", _f, dc); debug->write(_program->annotation(pc)); printf("\n"); }
                        storeZ(~readMemory(_f), d);
                        break;
                    case 0xa:
                        if (_debug) { printf("INCF 0x%02x, %c    ", _f, dc); debug->write(_program->annotation(pc)); printf("\n"); }
                        storeZ(readMemory(_f) + 1, d);
                        break;
                    case 0xb:
                        if (_debug) { printf("DECFSZ 0x%02x, %c  ", _f, dc); debug->write(_program->annotation(pc)); printf("\n"); }
                        r = readMemory(_f) - 1;
                        store(r, d);
                        if (r == 0) {
                            incrementPC();
                            _skipping = true;
                        }
                        break;
                    case 0xc:
                        if (_debug) { printf("RRF 0x%02x, %c     ", _f, dc); debug->write(_program->annotation(pc)); printf("\n"); }
                        r = readMemory(_f) | ((_memory[3] & 1) << 8);
                        setCarry((r & 1) != 0);
                        store(r >> 1, d);
                        break;
                    case 0xd:
                        if (_debug) { printf("RLF 0x%02x, %c     ", _f, dc); debug->write(_program->annotation(pc)); printf("\n"); }
                        r = (readMemory(_f) << 1) | (_memory[3] & 1);
                        setCarry((r & 0x100) != 0);
                        store(r, d);
                        break;
                    case 0xe:
                        if (_debug) { printf("SWAPF 0x%02x, %c   ", _f, dc); debug->write(_program->annotation(pc)); printf("\n"); }
                        r = readMemory(_f);
                        store((r >> 4) | (r << 4), d);
                        break;
                    case 0xf:
                        if (_debug) { printf("INCFSF 0x%02x, %c  ", _f, dc); debug->write(_program->annotation(pc)); printf("\n"); }
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
                        if (_debug) { printf("BCF 0x%02x, %i     ", _f, b); debug->write(_program->annotation(pc)); printf("\n"); }
                        _data = readMemory(_f) & ~m;
                        break;
                    case 5:
                        if (_debug) { printf("BSF 0x%02x, %i     ", _f, b); debug->write(_program->annotation(pc)); printf("\n"); }
                        _data = readMemory(_f) | m;
                        break;
                    case 6:
                        if (_debug) { printf("BTFSC 0x%02x, %i   ", _f, b); debug->write(_program->annotation(pc)); printf("\n"); }
                        if ((readMemory(_f, m) & m) == 0) {
                            incrementPC();
                            _skipping = true;
                        }
                        _f = -1;
                        break;
                    case 7:
                        if (_debug) { printf("BTFSS 0x%02x, %i   ", _f, b); debug->write(_program->annotation(pc)); printf("\n"); }
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
                    if (_debug) { printf("RETLW 0x%02x      ", d); debug->write(_program->annotation(pc)); printf("\n"); }
                    _skipping = true;
                    _memory[2] = _stack[0];
                    _pch = _stack[0] & 0x100;
                    _stack[0] = _stack[1];
                    _w = d;
                    break;
                case 0x9:
                    if (_debug) { printf("CALL  0x%02x      ", d); debug->write(_program->annotation(pc)); printf("\n"); }
                    _skipping = true;
                    _stack[1] = _stack[0];
                    _stack[0] = _memory[2] | _pch;
                    _pch = 0;
                    _memory[2] = d;
                    break;
                case 0xa:
                case 0xb:
                    if (_debug) { printf("GOTO  0x%03x     ", op & 0x1ff); debug->write(_program->annotation(pc)); printf("\n"); }
                    _skipping = true;
                    _pch = op & 0x100;
                    _memory[2] = d;
                    break;
                case 0xc:
                    if (_debug) { printf("MOVLW 0x%02x      ", d); debug->write(_program->annotation(pc)); printf("\n"); }
                    _w = d;
                    break;
                case 0xd:
                    if (_debug) { printf("IORLW 0x%02x      ", d); debug->write(_program->annotation(pc)); printf("\n"); }
                    storeZ(_w | d, false);
                    break;
                case 0xe:
                    if (_debug) { printf("ANDLW 0x%02x      ", d); debug->write(_program->annotation(pc)); printf("\n"); }
                    storeZ(_w & d, false);
                    break;
                case 0xf:
                    if (_debug) { printf("XORLW 0x%02x      ", d); debug->write(_program->annotation(pc)); printf("\n"); }
                    storeZ(_w ^ d, false);
                    break;
            }
        }
#ifdef DUMP
//        if (_debug) {
            CharacterSource c = markerCode.start();
            do {
                int ch = c.get();
                if (ch == -1)
                    break;
                switch (ch) {
                    case 'P':
                        _parent = c.get() - '0';
                        //printf("%*sBar %i parent %i -> (", _indent*2, "", _number, _parent);
                        //if (_connectedBar[_parent] == -1)
                        //    printf("disconnected");
                        //else
                        //    printf("%i/%i", _connectedBar[_parent], _connectedDirection[_parent]);
                        //printf(") child %i (", _child);
                        //if (_connectedBar[_child] == -1)
                        //    printf("disconnected");
                        //else
                        //    printf("%i/%i", _connectedBar[_child], _connectedDirection[_child]);
                        //printf(")\n");
                        //if (_parent != _staticParent)
                        //    printf("Expected %i not %i for parent of %i\n",_staticParent, _parent, _number);
                        break;
                    case 'C':
                        _child = c.get() - '0';
                        //printf("%*sBar %i parent %i (", _indent*2, "", _number, _parent);
                        //if (_connectedBar[_parent] == -1)
                        //    printf("disconnected");
                        //else
                        //    printf("%i/%i", _connectedBar[_parent], _connectedDirection[_parent]);
                        //printf(") child %i -> (", _child);
                        //if (_connectedBar[_child] == -1)
                        //    printf("disconnected");
                        //else
                        //    printf("%i/%i", _connectedBar[_child], _connectedDirection[_child]);
                        //printf(")\n");
                        break;
                    case 'p':
                        {
                            int cLow = c.get();
                            int cHigh = c.get();
                            switch (_parent) {
                                case 0:
                                    _newMarker[0] = (_data & 0x10) ? cHigh : cLow;
                                    break;
                                case 1:
                                    _newMarker[1] = (_data & 0x20) ? cHigh : cLow;
                                    break;
                                case 2:
                                    _newMarker[2] = (_data & 1) ? cHigh : cLow;
                                    break;
                                case 3:
                                    _newMarker[3] = (_data & 2) ? cHigh : cLow;
                                    break;
                            }
                        }
                        break;
                    case 'c':
                        {
                            int cLow = c.get();
                            if (cLow == 'R') {
                                if (_number != _simulation->getNumberForIndent(_indent)) {
                                    _simulation->setNumberForIndent(_indent, _number);
                                    _simulation->setMatrix(_t/(400*256), _indent, -(_number + 1));
                                }
                                else
                                    _simulation->setMatrix(_t/(400*256), _indent, _cyclesSinceLastSync);
                                _cyclesSinceLastSync = 0;
                            }
                            int cHigh = c.get();
                            switch (_child) {
                                case 0:
                                    _newMarker[0] = (_data & 0x10) ? cHigh : cLow;
                                    break;
                                case 1:
                                    _newMarker[1] = (_data & 0x20) ? cHigh : cLow;
                                    break;
                                case 2:
                                    _newMarker[2] = (_data & 1) ? cHigh : cLow;
                                    break;
                                case 3:
                                    _newMarker[3] = (_data & 2) ? cHigh : cLow;
                                    break;
                            }
                        }
                        break;
                    case 'o':
                        {
                            int cLow = c.get();
                            int cHigh = c.get();
                            if (_parent != 0 && _child != 0)
                                _newMarker[0] = (_data & 0x10) ? cHigh : cLow;
                            if (_parent != 1 && _child != 1)
                                _newMarker[1] = (_data & 0x20) ? cHigh : cLow;
                            if (_parent != 2 && _child != 2)
                                _newMarker[2] = (_data & 1) ? cHigh : cLow;
                            if (_parent != 3 && _child != 3)
                                _newMarker[3] = (_data & 2) ? cHigh : cLow;
                        }
                        break;
                    case 'r':
                        do {
                            int r = c.get();
                            if (r == _readMarker)
                                break;
                            if (r == -1) {
                                printf("Bar %i read marker %c from %i, expected ", _number, _readMarker, _readFromBar);
                                debug->write(markerCode);
                                printf("\n");
                                break;
                            }
                        } while (true);
                        break;
                }
            } while (true);
//        }
#endif
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
            if ((h & 0x10) != (_io & 0x10) || _marker[0] != _newMarker[0])
                _simulation->write(_t, _connectedBar[0], _connectedDirection[0], (h & 0x10) != 0);
            if ((h & 0x20) != (_io & 0x20) || _marker[1] != _newMarker[1])
                _simulation->write(_t, _connectedBar[1], _connectedDirection[1], (h & 0x20) != 0);
            if ((h & 1) != (_io & 1) || _marker[2] != _newMarker[2])
                _simulation->write(_t, _connectedBar[2], _connectedDirection[2], (h & 1) != 0);
            if ((h & 2) != (_io & 2) || _marker[3] != _newMarker[3])
                _simulation->write(_t, _connectedBar[3], _connectedDirection[3], (h & 2) != 0);
            _io = h;
            if (_debug) {
                printf("%*sWrote 0x%02x (0x%02x | 0x%02x)\n", _indent*8, "", h, _memory[6], _memory[0x20]);
                if (_f == 6 && _data != 0)
                    printf("%*sGPIO=0x%02x\n", _indent*8, "", _data);
            }
            for (int i = 0; i < 4; ++i)
                _marker[i] = _newMarker[i];
        }
        if (_f == 2)
            _pch = 0;
    }
    void connect(int direction, int connectedBar, int connectedDirection)
    {
        _connectedBar[direction] = connectedBar;
        _connectedDirection[direction] = connectedDirection;
        _marker[direction] = _newMarker[direction] = '.';
    }
    bool read(int t, int direction, char* readMarker)
    {
        simulateTo(t);
        switch (direction) {
            case 0:
                *readMarker = _marker[0];
                return (_io & 0x10) != 0;
            case 1:
                *readMarker = _marker[1];
                return (_io & 0x20) != 0;
            case 2:
                *readMarker = _marker[2];
                return (_io & 1) != 0;
            case 3:
                *readMarker = _marker[3];
                return (_io & 2) != 0;
        }
        return true;
    }
    int prime(int parent, int indent = 0)
    {
        int liveBars = 1;
        _live = true;
        _staticParent = parent;
        _indent = indent;
        _primed = true;
        int childB = (parent + 1) & 3;
        int childC = (parent + 2) & 3;
        int childD = (parent + 3) & 3;
        int childBbar = _connectedBar[childB];
        _childBabsent = (childBbar == -1);
        if (!_childBabsent) {
            Bar* bar = _simulation->bar(childBbar);
            if (bar->primed())
                _childBabsent = true;
            else
                liveBars += bar->prime(_connectedDirection[childB], indent + 1);
        }
        int childCbar = _connectedBar[childC];
        _childCabsent = (childCbar == -1);
        if (!_childCabsent) {
            Bar* bar = _simulation->bar(childCbar);
            if (bar->primed())
                _childCabsent = true;
            else
                liveBars += bar->prime(_connectedDirection[childC], indent + 1);
        }
        int childDbar = _connectedBar[childD];
        _childDabsent = (childDbar == -1);
        if (!_childDabsent) {
            Bar* bar = _simulation->bar(childDbar);
            if (bar->primed())
                _childDabsent = true;
            else
                liveBars += bar->prime(_connectedDirection[childD], indent + 1);
        }
        return liveBars;
    }
    void dumpConnections(int parent)
    {
        int childB = (parent + 1) & 3;
        int childC = (parent + 2) & 3;
        int childD = (parent + 3) & 3;
//#ifdef DUMP
        printf("%*s%03i: ", _indent*2, "", _number);
        for (int i = 0; i < 4; ++i)
            if (_connectedBar[i] == -1)
                printf("---/- ");
            else
                printf("%03i/%i ", _connectedBar[i], _connectedDirection[i]);
        printf("\n");
//#endif
        if (!_childBabsent)
            _simulation->bar(_connectedBar[childB])->dumpConnections(_connectedDirection[childB]);
        if (!_childCabsent)
            _simulation->bar(_connectedBar[childC])->dumpConnections(_connectedDirection[childC]);
        if (!_childDabsent)
            _simulation->bar(_connectedBar[childD])->dumpConnections(_connectedDirection[childD]);
        _primed = false;
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
            *(store++) = _childBabsent ? 1 : 0;
            *(store++) = _childCabsent ? 1 : 0;
            *(store++) = _childDabsent ? 1 : 0;
        }
        if (!_childBabsent)
            store = _simulation->bar(_connectedBar[childB])->storeExpectedStream(_connectedDirection[childB], store);
        if (!_childCabsent)
            store = _simulation->bar(_connectedBar[childC])->storeExpectedStream(_connectedDirection[childC], store);
        if (!_childDabsent)
            store = _simulation->bar(_connectedBar[childD])->storeExpectedStream(_connectedDirection[childD], store);
        _primed = false;
        return store;
    }
    int connectedBar(int direction) const { return _connectedBar[direction]; }
    int connectedDirection(int direction) const { return _connectedDirection[direction]; }
    int time() const { return _t; }
    bool live() const { return _live; }
    void clearLive() { _oldLive = _live; _live = false; }
    void resetNewlyConnected() { if (_live && !_oldLive) reset(); }
    void setDebug() { _debug = true; }
    void dump()
    {
        if (!_live)
            return;
        printf("Bar %3i:", _number);
        printf("  Memory: %02x", _w);
        for (int i = 0; i < 0x21; ++i)
            printf(" %02x", _memory[i]);
        printf(" %02x", _option);
        printf("  Stack: %03x %03x %03x %c %i %07x", _memory[2] | _pch, _stack[0], _stack[1], _skipping ? 'S' : 'N', _state, _tNextStop);
        printf("  Connections:");
        for (int i = 0; i < 4; ++i)
            if (_connectedBar[i] == -1)
                printf(" ---/-");
            else
                printf(" %03i/%i", _connectedBar[i], _connectedDirection[i]);
        printf("  ");
        debug->write(_program->annotation(_pch | _memory[2]).subString(4, 13));
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
                else {
                    _readFromBar = _connectedBar[2];
                    r |= (_simulation->read(_t, _connectedBar[2], _connectedDirection[2], &_readMarker) ? 1 : 0);
                }
            if ((care & 2) != 0)
                if ((_memory[0x20] & 2) == 0)
                    r |= (_memory[6] & 2);
                else {
                    _readFromBar = _connectedBar[3];
                    r |= (_simulation->read(_t, _connectedBar[3], _connectedDirection[3], &_readMarker) ? 2 : 0);
                }
            if ((_memory[0x20] & 4) == 0)
                r |= (_memory[6] & 4);
            else
                r |= 4;  // Switch not implemented for now
            if ((care & 0x10) != 0)
                if ((_memory[0x20] & 0x10) == 0)
                    r |= (_memory[6] & 0x10);
                else {
                    _readFromBar = _connectedBar[0];
                    r |= (_simulation->read(_t, _connectedBar[0], _connectedDirection[0], &_readMarker) ? 0x10 : 0);
                }
            if ((care & 0x20) != 0)
                if ((_memory[0x20] & 0x20) == 0)
                    r |= (_memory[6] & 0x20);
                else {
                    _readFromBar = _connectedBar[1];
                    r |= (_simulation->read(_t, _connectedBar[1], _connectedDirection[1], &_readMarker) ? 0x20 : 0);
                }
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
        _stack[0] = _stack[1] = 0;
        _memory[2] = 0xff;
        _memory[3] = 0x18;
        _memory[4] = 0xe0;
        _memory[5] = 0xfe;
        _memory[0x20] = 0x3f;
        _io = 0x3f;
        _option = 0xff;
        _pch = 0x100;
        for (int i = 0; i < 4; ++i)
            _marker[i] = _newMarker[i] = '.';
        _t = 0;
        if (_number == 0)
            _tPerQuarterCycle = 100*256;
        else
            _tPerQuarterCycle = (rand() % 512) + 99*256;  // 99*256 to 101*256 units of cycle/(100*256)
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
    int _connectedBar[4];
    int _connectedDirection[4];
    int _t;
    int _tNextStop;
    int _number;
    int _indent;
    bool _debug;
    UInt8 _io;
    int _f;
    UInt8 _data;
    bool _primed;
    bool _childBabsent;
    bool _childCabsent;
    bool _childDabsent;
    bool _live;
    char _marker[4];
    char _readMarker;
    int _readFromBar;
    int _parent;
    int _child;
    char _newMarker[4];
    int _staticParent;
    bool _oldLive;
    int _state;
    int _cyclesSinceLastSync;
};

typedef BarTemplate<Simulation> Bar;

class Simulation
{
public:
    Simulation()
      : _totalBars(100),
        _stream(_totalBars*8),
        _expectedStream(_totalBars*8),
        _good(false),
        _cyclesThisStream(0),
        _settlingCycles(0),
        _maxSettlingCycles(0),
        _changes(0),
        _totalSettlingCycles(0),
        _streams(0),
        _wall(0),
        _goodCycles(0),
        _goodWords(0),
        _t(0),
        _streamsSinceLastChange(0),
        _goodsSinceLastChange(0),
        _settled(false),
        _oldGood(false),
        _matrix(101*256),
        _numbers(101, -1),
        _dumpMatrix(false),
        _badStreams(0),
        _maxBadStreams(0),
        _dumping(false)
    {
        CONSOLE_SCREEN_BUFFER_INFO consoleScreenBufferInfo;
        GetConsoleScreenBufferInfo(*debug, &consoleScreenBufferInfo);
        _cursorPosition = consoleScreenBufferInfo.dwCursorPosition;
    }
    void simulate()
    {
        SimulatedProgram intervalProgram(String("../intervals.HEX"), String("../intervals.annotation"));
        intervalProgram.load();

        SimulatedProgram rootProgram(String("../root.HEX"), String("../root.annotation"));
        rootProgram.load();

        Bar* root;
        for (int i = 0; i <= _totalBars; ++i) {
            Handle bar;
            bar = new Bar(this, (i == 0 ? &rootProgram : &intervalProgram), i, false);
            if (i == 0)
                root = bar;
            _bars.push_back(bar);
        }

//        _bars[0]->setDebug();
//        _bars[39]->setDebug();

        _streamPointer = &_stream[0];
        _streamEndPointer = _streamPointer + _totalBars*8;
        _connectedPairs = 0;
        do {
            double cyclesBeforeChange = -log((static_cast<double>(rand()) + 1)/(static_cast<double>(RAND_MAX) + 1))*10000.0;
            bool final = false;
            do {
#ifdef DUMP1
                for (int i = 0; i < 101*256; ++i)
                    _matrix[i] = 0;
#endif
                int t;
                if (cyclesBeforeChange > 256.0) {
                    t = 256*400*256;
                    cyclesBeforeChange -= 256.0;
                }
                else {
                    t = static_cast<int>(cyclesBeforeChange*400*256);
                    final = true;
                }
                for (std::vector<Handle>::iterator i = _bars.begin(); i != _bars.end(); ++i)
                    (*i)->simulateTo(t);
                for (std::vector<Handle>::iterator i = _bars.begin(); i != _bars.end(); ++i)
                    (*i)->resetTime();
#ifdef DUMP1
                if (_dumpMatrix)
                    for (int i = 0; i < t/(400*256); ++i) {
                        for (int j = 0; j < 101; ++j) {
                            int n = _matrix[j + i*101];
                            if (n == 0)
                                printf("    ");
                            else
                                if (n < 0)
                                    printf("-%03i", -(n-1));
                                else
                                    printf("%3i ", n);
                        }
                        printf("\n");
                    }
#endif
                if (!_settled)
                    _settlingCycles += t/(400.0*256.0);
                _cyclesThisStream += t/(400.0*256.0);
                _t += t/(1000000.0*400.0*256.0);
            } while (!final);

            if (!_dumping && (GetKeyState('D')&0x80000000) != 0) {
                for (int i = 0; i <= _totalBars; ++i)
                    _bars[i]->dump();
                _dumping = true;
            }
            if (_dumping && (GetKeyState('D')&0x80000000) == 0)
                _dumping = false;
            double goodStreamProportion = static_cast<double>(_goodsSinceLastChange) / static_cast<double>(_streamsSinceLastChange);

            //if (_changes == 33254) {
            //    _bars[1]->setDebug();
            //    _bars[57]->setDebug();
            //}

            //if (_changes == 33255 && _cyclesThisStream >= 1000000)
            //    exit(0);

            if (_settled) {
                _good = false;
                _oldGood = false;
                _settlingCycles = 0;
                _streamsSinceLastChange = 0;
                _goodsSinceLastChange = 0;
                _settled = false;
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
                ++_changes;
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
#ifdef DUMP
                    printf("Configuration %i, time %lf: Connecting bar %i direction %i to bar %i direction %i. ", _changes, _t, barNumber, connectorNumber, connectedBarNumber, connectedDirection);
#endif
                    bar->connect(connectorNumber, connectedBarNumber, connectedDirection);
                    otherBar->connect(connectedDirection, barNumber, connectorNumber);
                    ++_connectedPairs;
                }
                else {
                    // This connector is connected - disconnect it.
#ifdef DUMP
                    printf("Configuration %i, time %lf: Disconnecting bar %i direction %i from bar %i direction %i. ", _changes, _t, barNumber, connectorNumber, connectedBarNumber, connectedDirection);
#endif
                    Bar* connectedBar = _bars[connectedBarNumber];
                    bar->connect(connectorNumber, -1, 0);
                    connectedBar->connect(connectedDirection, -1, 0);
                    --_connectedPairs;
                }
                // Prime to update _indent
                for (std::vector<Handle>::iterator i = _bars.begin(); i != _bars.end(); ++i)
                    (*i)->clearLive();
                int liveBars = _bars[0]->prime(0);
#ifdef DUMP
                printf("Live %i connections %i\n", liveBars, _connectedPairs);
#endif
#ifdef DUMP
                _bars[0]->dumpConnections(0);
#else
                _bars[0]->storeExpectedStream(0, &_expectedStream[0]);
#endif
                for (std::vector<Handle>::iterator i = _bars.begin(); i != _bars.end(); ++i)
                    (*i)->resetNewlyConnected();
                //if (_changes == 4251) {
                //    //_bars[30]->setDebug();
                //    _bars[98]->setDebug();
                //    _bars[37]->setDebug();
                //}
            }
        } while (true);
    }
    void streamBit(bool bit)
    {
        if (_streamPointer != &*_streamEndPointer)
            *(_streamPointer++) = bit;
    }
    void streamStart()
    {
        int* streamPointer = &_stream[0];
        int* expectedStreamPointer = &_expectedStream[0];
        int liveBars = _bars[0]->prime(0);
        int* expectedStreamPointerEnd = _bars[0]->storeExpectedStream(0, expectedStreamPointer);
        _good = true;
        do {
            if (streamPointer == _streamPointer) {
                if (expectedStreamPointer == expectedStreamPointerEnd)
                    break;
                _good = false;
                break;
            }
            if (expectedStreamPointer == expectedStreamPointerEnd) {
                _good = false;
                break;
            }
            if ((*streamPointer) != (*expectedStreamPointer)) {
                _good = false;
                break;
            }
            ++streamPointer;
            ++expectedStreamPointer;
        } while (true);
        int i;
        if (!_good) {
#ifdef DUMP
            printf("Time %lf, Stream %i is bad. Expected", _t, _streams);
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
#endif
            if (_oldGood) {
                printf("Bad after good\n");
                exit(0);
            }
            ++_badStreams;
            if (_badStreams >= _maxBadStreams)
                _maxBadStreams = _badStreams;
        }
        else {
            _badStreams = 0;
            ++_goodsSinceLastChange;
            _goodCycles += _cyclesThisStream;
            _goodWords += liveBars;
#ifdef DUMP
            printf("Time %lf, Stream %i is good:", _t, _streams);
            for (i = 0, streamPointer = &_stream[0]; streamPointer != _streamPointer; ++streamPointer, ++i) {
                if ((i % 8) == 0)
                    printf(" ");
                printf("%i", *streamPointer);
            }
            printf("\n");
#endif
            if (!_settled) {
                _settled = true;
#ifdef DUMP
                    printf("Time %lf, Configuration %i settled in %lf\n", _t, _changes, _settlingCycles);
#endif
                _totalSettlingCycles += _settlingCycles;
            }
        }
        _oldGood = _good;
        //if (_changes == 4251) {
        //    if (_badStreams > 10)
        //        exit(0);
        //}
        //if (_streams == 8900) {
        //    _bars[98]->setDebug();
        //    _bars[80]->setDebug();
        //}
        ++_streams;
        ++_streamsSinceLastChange;
        if (_settlingCycles > _maxSettlingCycles) {
            _maxSettlingCycles = _settlingCycles;
        }
#ifndef DUMP
        clock_t wall = clock();
        if ((wall - _wall) > CLOCKS_PER_SEC / 10) {
            _wall = wall;
            SetConsoleCursorPosition(*debug, _cursorPosition);
            printf("Configuration: %i\n", _changes);
            printf("Time: %lf\n", _t);
            printf("Bars: %i  \n", liveBars);
            printf("Streams: %i\n", _streams);
            printf("Maximum settling cycles: %lf\n", _maxSettlingCycles);
            printf("Mean settling cycles: %lf  \n", _totalSettlingCycles/_changes);
            printf("Cycles per word: %lf  \n", _goodCycles/_goodWords);
            printf("Bad streams: %i  \n", _badStreams);
            printf("Max bad streams: %i\n", _maxBadStreams);
        }
#endif
        if (_badStreams > 100)
            exit(0);
        _cyclesThisStream = 0;
        _streamPointer = &_stream[0];
    }

    bool read(int t, int bar, int direction, char* readMarker)
    {
        if (bar == -1) {
            *readMarker = '.';
            return true;
        }
        return _bars[bar]->read(t, direction, readMarker);
    }
    void write(int t, int bar, int direction, bool value)
    {
        if (bar == -1)
            return;
        _bars[bar]->simulateTo(t);
    }
    Bar* bar(int n) { return _bars[n]; }

    int getNumberForIndent(int indent) { return _numbers[indent]; }
    void setNumberForIndent(int indent, int number) { _numbers[indent] = number; }
    void setMatrix(int t, int indent, int value) { _matrix[indent + t*101] = value; }

private:
    std::vector<Handle> _bars;
    int _totalBars;
    std::vector<int> _stream;
    std::vector<int> _expectedStream;
    int* _streamPointer;
    int* _streamEndPointer;
    int _connectedPairs;
    bool _good;
    double _cyclesThisStream;
    double _settlingCycles;
    double _maxSettlingCycles;
    COORD _cursorPosition;
    double _t;
    int _changes;
    double _totalSettlingCycles;
    int _streams;
    clock_t _wall;
    double _goodCycles;
    double _goodWords;
    bool _settled;
    int _goodsSinceLastChange;
    int _streamsSinceLastChange;
    bool _oldGood;
    std::vector<int> _matrix;
    std::vector<int> _numbers;
    bool _dumpMatrix;
    int _badStreams;
    int _maxBadStreams;
    bool _dumping;
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
