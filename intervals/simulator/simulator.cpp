#include "unity/string.h"
#include "unity/file.h"

#include <vector>

#include <stdio.h>

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

class Component
{
public:
    Component(Simulation* simulation)
      : _simulation(simulation), _t(0)
    { }
    void adjustTime(double t) { _t -= t; }
    virtual void simulateTo(double t) = 0;
protected:
    Simulation* _simulation;
    double _t;
};

class Connection
{
public:
    Connection(Simulation* simulation)
      : _simulation(simulation), _t(0), _maleHigh(true), _femaleHigh(true)
    { }
    void connectMale(Component* male) { _male = male; }
    void connectFemale(Component* female) { _female = female; }
    void maleLow(double t)
    {
        _female->simulateTo(t);
        _maleHigh = false;
    }
    void femaleLow(double t)
    {
        _male->simulateTo(t);
        _femaleHigh = false;
    }
    void maleHigh(double t)
    {
        _female->simulateTo(t);
        _maleHigh = true;
    }
    void femaleHigh(double t)
    {
        _male->simulateTo(t);
        _femaleHigh = true;
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
    void adjustTime(double t) { _t -= t; }
private:
    Component* _male;
    Component* _female;
    Simulation* _simulation;
    double _t;
    bool _maleHigh;
    bool _femaleHigh;
};

class Simulation
{
public:
    void addComponent(Component* component) { _components.push_back(component); }
    void addConnection(Connection* connection) { _connections.push_back(connection); }
    void adjustTime(double t)
    {
        for (std::vector<Component*>::iterator i = _components.begin(); i != _components.end(); ++i)
            (*i)->adjustTime(t);
        for (std::vector<Connection*>::iterator i = _connections.begin(); i != _connections.end(); ++i)
            (*i)->adjustTime(t);
    }
private:
    std::vector<Component*> _components;
    std::vector<Connection*> _connections;
};

class Interval : public Component
{
public:
    Interval(Simulation* simulation, const Program* program)
      : Component(simulation),
        _program(program)
    {
        for (int i = 0; i < 0x20; ++i)
            _memory[i] = 0;
        _memory[2] = 0xff;
        _memory[3] = 0x18;
        _memory[4] = 0xe0;
        _memory[5] = 0xfe;
        _tris = 0x3f;
        _option = 0xff;
        _pch = 0x100;
    }
    void simulateTo(double t)
    {
        // TODO
    }
    void simulateCycle()
    {
        int op = _program->op(_pch | _memory[2]);
        if (_skipping) {
            _skipping = false;
            return;
        }
        incrementPC();
        UInt16 r;
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
                                    printf("NOP\n");
                                    break;
                                case 2:
                                    printf("OPTION\n");
                                    _option = _w;
                                    break;
                                case 3:
                                    printf("SLEEP\n");
                                    throw Exception(String("SLEEP not supported"));
                                    break;
                                case 4:
                                    printf("CLRWDT\n");
                                    throw Exception(String("CLRWDT not supported"));
                                    break;
                                case 5:  // Not a real PIC12F508 opcode - used for simulator escape (data)
                                    printf("%c", ((_w & 0x10) != 0) ? '1' : '0');
                                    break;
                                case 6:
                                    printf("TRIS GPIO\n");
                                    _tris = _w;
                                    updateIO();
                                    break;
                                case 7:  // Not a real PIC12F08 opcode - used for simulator escape (space)
                                    printf("\n");
                                    break;
                                default:
                                    unrecognizedOpcode(op);
                                    break;
                            }
                        else {
                            printf("MOVWF 0x%02x\n", f);
                            writeMemory(f, _w);
                        }
                        break;
                    case 0x1:
                        if (!d)
                            printf("CLRW\n");
                        else
                            printf("CLRF 0x%02x\n", f);
                        storeZ(0, f, d);
                        break;
                    case 0x2:
                        printf("SUBWF 0x%02x, %c\n", f, dc);
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
                        printf("DECF 0x%02x, %c\n", f, dc);
                        storeZ(readMemory(f) - 1, f, d);
                        break;
                    case 0x4:
                        printf("IORWF 0x%02x, %c\n", f, dc);
                        storeZ(readMemory(f) | _w, f, d);
                        break;
                    case 0x5:
                        printf("ANDWF 0x%02x, %c\n", f, dc);
                        storeZ(readMemory(f) & _w, f, d);
                        break;
                    case 0x6:
                        printf("XORWF 0x%02x, %c\n", f, dc);
                        storeZ(readMemory(f) ^ _w, f, d);
                        break;
                    case 0x7:
                        printf("ADDWF 0x%02x, %c\n", f, dc);
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
                        printf("MOVF 0x%02x, %c\n", f, dc);
                        storeZ(readMemory(f), f, d);
                        break;
                    case 0x9:
                        printf("COMF 0x%02x, %c\n", f, dc);
                        storeZ(~readMemory(f), f, d);
                        break;
                    case 0xa:
                        printf("INCF 0x%02x, %c\n", f, dc);
                        storeZ(readMemory(f) + 1, f, d);
                        break;
                    case 0xb:
                        printf("DECFSZ 0x%02x, %c\n", f, dc);
                        r = readMemory(f) - 1;
                        store(r, f, d);
                        if (r == 0) {
                            incrementPC();
                            _skipping = true;
                        }
                        break;
                    case 0xc:
                        printf("RRF 0x%02x, %c\n", f, dc);
                        r = readMemory(f) | ((_memory[3] & 1) << 8);
                        setCarry((r & 1) != 0);
                        store(r >> 1, f, d);
                        break;
                    case 0xd:
                        printf("RLF 0x%02x, %c\n", f, dc);
                        r = (readMemory(f) << 1) | (_memory[3] & 1);
                        setCarry((r & 0x100) != 0);
                        store(r, f, d);
                        break;
                    case 0xe:
                        printf("SWAPF 0x%02x, %c\n", f, dc);
                        r = readMemory(f);
                        store((r >> 4) | (r << 4), f, d);
                        break;
                    case 0xf:
                        printf("INCFSF 0x%02x, %c\n", f, dc);
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
                        printf("BCF 0x%02x, %i\n", f, b);
                        writeMemory(f, readMemory(f) & ~m);
                        break;
                    case 5:
                        printf("BSF 0x%02x, %i\n", f, b);
                        writeMemory(f, readMemory(f) | m);
                        break;
                    case 6:
                        printf("BTFSC 0x%02x, %i\n", f, b);
                        if ((readMemory(f) & m) == 0) {
                            incrementPC();
                            _skipping = true;
                        }
                        break;
                    case 7:
                        printf("BTFSS 0x%02x, %i\n", f, b);
                        if ((readMemory(f) & m) != 0) {
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
                    printf("RETLW 0x%02x\n", d);
                    _skipping = true;
                    _memory[2] = _stack[0];
                    _pch = _stack[0] & 0x100;
                    _stack[0] = _stack[1];
                    _w = d;
                    break;
                case 0x9:
                    printf("CALL 0x%02x\n", d);
                    _skipping = true;
                    _stack[1] = _stack[0];
                    _stack[0] = _memory[2] | _pch;
                    _pch = 0;
                    _memory[2] = d;
                    break;
                case 0xa:
                case 0xb:
                    printf("GOTO 0x%03x\n", op & 0x1ff);
                    _skipping = true;
                    _pch = op & 0x100;
                    _memory[2] = d;
                    break;
                case 0xc:
                    printf("MOVLW 0x%02x\n", d);
                    _w = d;
                    break;
                case 0xd:
                    printf("IORLW 0x%02x\n", d);
                    _w |= d;
                    break;
                case 0xe:
                    printf("ANDLW 0x%02\n", d);
                    _w &= d;
                    break;
                case 0xf:
                    printf("XORLW 0x%02x\n", d);
                    _w ^= d;
                    break;
            }
        }
    }
    void connect(int direction, Connection* connection)
    {
        _connections[direction] = connection;
    }
private:
    UInt8 readMemory(int address)
    {
        if (address == 0)
            address = _memory[4] & 0x1f;
        if (address == 6) {
            UInt8 r = 8;
            if ((_tris & 1) == 0)
                r |= (_memory[6] & 1);
            else
                r |= (_connections[2]->femaleRead(_t) ? 1 : 0);
            if ((_tris & 2) == 0)
                r |= (_memory[6] & 2);
            else
                r |= (_connections[3]->maleRead(_t) ? 2 : 0);
            if ((_tris & 4) == 0)
                r |= (_memory[6] & 4);
            else
                r |= 1;  // Switch not implemented for now
            if ((_tris & 0x10) == 0)
                r |= (_memory[6] & 0x10);
            else
                r |= (_connections[0]->maleRead(_t) ? 0x10 : 0);
            if ((_tris & 0x20) == 0)
                r |= (_memory[6] & 0x20);
            else
                r |= (_connections[1]->femaleRead(_t) ? 0x20 : 0);
            return r;
        }
        return _memory[address];
    }
    void updateIO()
    {
        UInt8 h = _memory[6] | _tris;
        if ((h & 0x10) == 0) _connections[0]->maleLow(_t); else _connections[0]->maleHigh(_t);
        if ((h & 0x20) == 0) _connections[1]->femaleLow(_t); else _connections[1]->femaleHigh(_t);
        if ((h & 1) == 0) _connections[2]->femaleLow(_t); else _connections[2]->femaleHigh(_t);
        if ((h & 2) == 0) _connections[3]->maleLow(_t); else _connections[3]->maleHigh(_t);
    }
    void writeMemory(int address, UInt8 data)
    {
        if (address == 0)
            address = _memory[4] & 0x1f;
        _memory[address] = data;
        if (address == 6)
            updateIO();
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

    const Program* _program;
    UInt8 _memory[0x20];
    UInt8 _tris;
    UInt8 _option;
    int _stack[2];
    int _pch;
    UInt8 _w;
    bool _skipping;
    double _tPerCycle;
    Connection* _connections[4];
};

class Disconnection : public Component
{
public:
    Disconnection(Simulation* simulation) : Component(simulation) { }
    void simulateTo(double t) { }
};

int main()
{
	BEGIN_CHECKED {
        Program intervalProgram(String("../intervals.HEX"));
        intervalProgram.load();

        Simulation simulation;

        Interval interval(&simulation, &intervalProgram);

        Program rootProgram(String("../root.HEX"));
        rootProgram.load();
        Interval root(&simulation, &rootProgram);

        Disconnection d(&simulation);
       
        Connection c(&simulation);

        c.connectMale(&root);
        c.connectFemale(&d);

        while (true)
            root.simulateCycle();
		//contents.write(Handle::consoleOutput());
	}
	END_CHECKED(Exception& e) {
		e.write(Handle::consoleOutput());
	}
}
