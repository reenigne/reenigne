#include "unity/string.h"
#include "unity/file.h"
#include <vector>
#include <stdio.h>

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

class MaleConnector
{
public:
    virtual void maleWrite(double t, bool d) = 0;
    virtual bool maleRead(double t) = 0;
};

class FemaleConnector
{
public:
    virtual void femaleWrite(double t, bool d) = 0;
    virtual bool femaleRead(double t) = 0;
};

class DisconnectedMaleConnector : public MaleConnector
{
public:
    void maleWrite(double t, bool d) { }
    bool maleRead(double t) { return true; }
};

class DisconnectedFemaleConnector : public FemaleConnector
{
public:
    void femaleWrite(double t, bool d) { }
    bool femaleRead(double t) { return true; }
};

class Bar : public ReferenceCounted
{
public:
    Bar(Simulation* simulation, const Program* program, bool indent = false)
      : _simulation(simulation), _program(program), _t(0), _indent(indent), _debug(false)
    {
        for (int i = 0; i < 0x20; ++i)
            _memory[i] = 0;
        _memory[2] = 0xff;
        _memory[3] = 0x18;
        _memory[4] = 0xe0;
        _memory[5] = 0xfe;
        _tris = 0x3f;
        _io = 0x3f;
        _option = 0xff;
        _pch = 0x100;
    }
    void simulateTo(double t)
    {
        if (_debug) {
            if (_indent)
                printf("        ");
            printf("Simulating to %lf\n", t);
        }
        while (_t <= t - 1)
            simulateCycle();
    }
    void adjustTime(double t) { _t -= t; }
    void simulateCycle()
    {
        _t0 = _t;
        int op = _program->op(_pch | _memory[2]);
        if (_skipping) {
            _skipping = false;
            _t = _t0 + 1.0;
            return;
        }
        incrementPC();
        UInt16 r;
        if (_debug) {
            if (_indent)
                printf("        ");
            printf("%lf ", _t);
        }
        if ((op & 0x800) == 0) {
            int f = op & 0x1f;
            if ((op & 0x400) == 0) {
                bool d = ((op & 0x20) != 0);  // true if destination is f, false if destination is W
                char dc = d ? 'f' : 'W';
                switch (op >> 6) {
                    case 0x0:
                        if (!d)
                            switch (f) {
                                case 0:
                                    if (_debug) printf("NOP\n");
                                    break;
                                case 2:
                                    if (_debug) printf("OPTION\n");
                                    _option = _w;
                                    break;
                                case 3:
                                    if (_debug) printf("SLEEP\n");
                                    throw Exception(String("SLEEP not supported"));
                                    break;
                                case 4:
                                    if (_debug) printf("CLRWDT\n");
                                    throw Exception(String("CLRWDT not supported"));
                                    break;
                                case 5:  // Not a real PIC12F508 opcode - used for simulator escape (data)
                                    {
                                        bool bit = ((_w & 1) != 0);
                                        _simulation->streamBit(bit);
                                        printf("%c", bit ? '1' : '0');
                                        if (_debug) printf("\n");
                                    }
                                    break;
                                case 6:
                                    if (_debug) printf("TRIS GPIO\n");
                                    _tris = _w;
                                    updateIO();
                                    break;
                                case 7:  // Not a real PIC12F08 opcode - used for simulator escape (space)
                                    _simulation->streamStart();
                                    if (_debug) printf("-\n"); else
                                        printf("\n");
                                    break;
                                default:
                                    unrecognizedOpcode(op);
                                    break;
                            }
                        else {
                            if (_debug) printf("MOVWF 0x%02x\n", f);
                            writeMemory(f, _w);
                        }
                        break;
                    case 0x1:
                        if (_debug)
                            if (!d)
                                printf("CLRW\n");
                            else
                                printf("CLRF 0x%02x\n", f);
                        storeZ(0, f, d);
                        break;
                    case 0x2:
                        if (_debug) printf("SUBWF 0x%02x, %c\n", f, dc);
                        r = readMemory(f) - _w;
                        if (r & 0x100)
                            _memory[3] |= 1;
                        else
                            _memory[3] &= 0xfe;
                        if ((_memory[f] & 0xf) - (_w & 0xf) != (r & 0xf))
                            _memory[3] |= 2;
                        else
                            _memory[3] &= 0xfd;
                        storeZ(r, f, d);
                        break;
                    case 0x3:
                        if (_debug) printf("DECF 0x%02x, %c\n", f, dc);
                        storeZ(readMemory(f) - 1, f, d);
                        break;
                    case 0x4:
                        if (_debug) printf("IORWF 0x%02x, %c\n", f, dc);
                        storeZ(readMemory(f) | _w, f, d);
                        break;
                    case 0x5:
                        if (_debug) printf("ANDWF 0x%02x, %c\n", f, dc);
                        storeZ(readMemory(f) & _w, f, d);
                        break;
                    case 0x6:
                        if (_debug) printf("XORWF 0x%02x, %c\n", f, dc);
                        storeZ(readMemory(f) ^ _w, f, d);
                        break;
                    case 0x7:
                        if (_debug) printf("ADDWF 0x%02x, %c\n", f, dc);
                        r = readMemory(f) + _w;
                        if (r & 0x100)
                            _memory[3] |= 1;
                        else
                            _memory[3] &= 0xfe;
                        if ((_w & 0xf) + (_memory[f] & 0xf) != (r & 0xf))
                            _memory[3] |= 2;
                        else
                            _memory[3] &= 0xfd;
                        storeZ(r, f, d);
                        break;
                    case 0x8:
                        if (_debug) printf("MOVF 0x%02x, %c\n", f, dc);
                        storeZ(readMemory(f), f, d);
                        break;
                    case 0x9:
                        if (_debug) printf("COMF 0x%02x, %c\n", f, dc);
                        storeZ(~readMemory(f), f, d);
                        break;
                    case 0xa:
                        if (_debug) printf("INCF 0x%02x, %c\n", f, dc);
                        storeZ(readMemory(f) + 1, f, d);
                        break;
                    case 0xb:
                        if (_debug) printf("DECFSZ 0x%02x, %c\n", f, dc);
                        r = readMemory(f) - 1;
                        store(r, f, d);
                        if (r == 0) {
                            incrementPC();
                            _skipping = true;
                        }
                        break;
                    case 0xc:
                        if (_debug) printf("RRF 0x%02x, %c\n", f, dc);
                        r = readMemory(f) | ((_memory[3] & 1) << 8);
                        setCarry((r & 1) != 0);
                        store(r >> 1, f, d);
                        break;
                    case 0xd:
                        if (_debug) printf("RLF 0x%02x, %c\n", f, dc);
                        r = (readMemory(f) << 1) | (_memory[3] & 1);
                        setCarry((r & 0x100) != 0);
                        store(r, f, d);
                        break;
                    case 0xe:
                        if (_debug) printf("SWAPF 0x%02x, %c\n", f, dc);
                        r = readMemory(f);
                        store((r >> 4) | (r << 4), f, d);
                        break;
                    case 0xf:
                        if (_debug) printf("INCFSF 0x%02x, %c\n", f, dc);
                        r = readMemory(f) + 1;
                        store(r, f, d);
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
                        if (_debug) printf("BCF 0x%02x, %i\n", f, b);
                        writeMemory(f, readMemory(f) & ~m);
                        break;
                    case 5:
                        if (_debug) printf("BSF 0x%02x, %i\n", f, b);
                        writeMemory(f, readMemory(f) | m);
                        break;
                    case 6:
                        if (_debug) printf("BTFSC 0x%02x, %i\n", f, b);
                        if ((readMemory(f, m) & m) == 0) {
                            incrementPC();
                            _skipping = true;
                        }
                        break;
                    case 7:
                        if (_debug) printf("BTFSS 0x%02x, %i\n", f, b);
                        if ((readMemory(f, m) & m) != 0) {
                            incrementPC();
                            _skipping = true;
                        }
                        break;
                }
            }
        }
        else {
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
        _t = _t0 + 1.0;
    }
    void connectNorth(MaleConnector* north) { _north = north; }
    void connectEast(FemaleConnector* east) { _east = east; }
    void connectSouth(FemaleConnector* south) { _south = south; }
    void connectWest(MaleConnector* west) { _west = west; }
private:
    UInt8 readMemory(int address, UInt8 care = 0xff)
    {
        _t = _t0 + 0.25;
        if (address == 0)
            address = _memory[4] & 0x1f;
        if (address == 6) {
            UInt8 r = 8;
            if ((care & 1) != 0)
                if ((_tris & 1) == 0)
                    r |= (_memory[6] & 1);
                else
                    r |= (_south->femaleRead(_t) ? 1 : 0);
            if ((care & 2) != 0)
                if ((_tris & 2) == 0)
                    r |= (_memory[6] & 2);
                else
                    r |= (_west->maleRead(_t) ? 2 : 0);
            if ((_tris & 4) == 0)
                r |= (_memory[6] & 4);
            else
                r |= 4;  // Switch not implemented for now
            if ((care & 0x10) != 0)
                if ((_tris & 0x10) == 0)
                    r |= (_memory[6] & 0x10);
                else
                    r |= (_north->maleRead(_t) ? 0x10 : 0);
            if ((care & 0x20) != 0)
                if ((_tris & 0x20) == 0)
                    r |= (_memory[6] & 0x20);
                else
                    r |= (_east->femaleRead(_t) ? 0x20 : 0);
            if (_debug) {
                if (_indent)
                    printf("        ");
                printf("Read 0x%02x\n", r);
            }
            return r;
        }
        return _memory[address];
    }
    void updateIO()
    {
        _t = _t0 + 1.0;
        UInt8 h = _memory[6] | _tris;
        if ((h & 0x10) != (_io & 0x10)) _north->maleWrite(_t, (h & 0x10) != 0);
        if ((h & 0x20) != (_io & 0x20)) _east->femaleWrite(_t, (h & 0x20) != 0);
        if ((h & 1) != (_io & 1)) _south->femaleWrite(_t, (h & 1) != 0);
        if ((h & 2) != (_io & 2)) _west->maleWrite(_t, (h & 2) != 0);
        _io = h;
        if (_debug) {
            if (_indent)
                printf("        ");
            printf("Wrote 0x%02x\n", h);
        }
    }
    void writeMemory(int address, UInt8 data)
    {
        if (address == 0)
            address = _memory[4] & 0x1f;
        _memory[address] = data;
        if (address == 6)
            updateIO();
        if (address == 2)
            _pch = 0;
    }
    void unrecognizedOpcode(int op)
    {
        static String unrecognized("Unrecognized opcode 0x");
        throw Exception(unrecognized + String::hexadecimal(op, 3));
    }
    void store(UInt16 r, int f, bool d)
    {
        if (d)
            writeMemory(f, static_cast<UInt8>(r));
        else
            _w = static_cast<UInt8>(r);
    }
    void storeZ(UInt16 r, int f, bool d)
    {
        store(r, f, d);
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

    Simulation* _simulation;
    const Program* _program;
    UInt8 _memory[0x20];
    UInt8 _tris;
    UInt8 _option;
    int _stack[2];
    int _pch;
    UInt8 _w;
    bool _skipping;
    double _tPerCycle;
    MaleConnector* _north;
    FemaleConnector* _east;
    FemaleConnector* _south;
    MaleConnector* _west;
    double _t;
    double _t0;
    bool _indent;
    bool _debug;
    UInt8 _io;
};

class ConnectedBars : public MaleConnector, public FemaleConnector, public ReferenceCounted
{
public:
    ConnectedBars(Bar* male, Bar* female) : _male(male), _female(female), _maleHigh(true), _femaleHigh(true) { }
    void maleWrite(double t, bool d)
    {
        _female->simulateTo(t);
        _maleHigh = d;
    }
    void femaleWrite(double t, bool d)
    {
        _male->simulateTo(t);
        _femaleHigh = d;
    }
    bool maleRead(double t)
    {
        _female->simulateTo(t);
        return _femaleHigh;
    }
    bool femaleRead(double t)
    {
        _male->simulateTo(t);
        return _maleHigh;
    }
private:
    Bar* _male;
    Bar* _female;
    bool _maleHigh;
    bool _femaleHigh;
};

class Simulation
{
public:
    Simulation() { }
    void simulate()
      : _totalBars(100);
    {
        Program intervalProgram(String("../intervals.HEX"));
        intervalProgram.load();

        Program rootProgram(String("../root.HEX"));
        rootProgram.load();

        Reference<Bar> root = new Bar(&rootProgram);
        add(root);

        for (int i = 0; i < _totalBars; ++i) {
            Reference<Bar> interval = new Bar(&intervalProgram, true);
            add(interval);
        }

        connect(root, interval, 2, 0);

        clock_t c0 = clock();
        for (int j = 0; j < 65536; ++j) {
            for (int i = 0; i < 256; ++i)
                root->simulateCycle();
            for (std::vector<Reference<Bar> >::iterator i = _bars.begin(); i != _bars.end(); ++i)
                (*i)->adjustTime(256.0);
        }
        clock_t c1 = clock();
        printf("%i\n",c1-c0);
    }
    void add(const Reference<Bar>& bar)
    {
        _bars.push_back(bar);
        bar->connectNorth(&_disconnectedMaleConnector);
        bar->connectEast(&_disconnectedFemaleConnector);
        bar->connectSouth(&_disconnectedFemaleConnector);
        bar->connectWest(&_disconnectedMaleConnector);
    }
    void connect(Bar* bar1, Bar* bar2, int direction1, int direction2)
    {
        Bar* male;
        Bar* female;
        if (direction1 == 0 || direction1 == 3) {
            male = bar1;
            female = bar2;
        }
        else {
            female = bar1;
            male = bar2;
        }
        Reference<ConnectedBars> connectedBars = new ConnectedBars(male, female);
        switch (direction1) {
            case 0:
                bar1->connectNorth(connectedBars);
                break;
            case 1:
                bar1->connectEast(connectedBars);
                break;
            case 2:
                bar1->connectSouth(connectedBars);
                break;
            case 3:
                bar1->connectWest(connectedBars);
                break;
        }
        switch (direction2) {
            case 0:
                bar2->connectNorth(connectedBars);
                break;
            case 1:
                bar2->connectEast(connectedBars);
                break;
            case 2:
                bar2->connectSouth(connectedBars);
                break;
            case 3:
                bar2->connectWest(connectedBars);
                break;
        }
        _connectedBars.push_back(connectedBars);
    }
    void streamBit(bool bit)
    {
    }
    void streamStart()
    {
    }
private:
    std::vector<Reference<Bar> > _bars;
    std::vector<Reference<ConnectedBars> > _connectedBars;
    DisconnectedMaleConnector _disconnectedMaleConnector;
    DisconnectedFemaleConnector _disconnectedFemaleConnector;
    int _totalBars;
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
