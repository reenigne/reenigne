#include "unity/string.h"
#include "unity/array.h"
#include "unity/file.h"
#include "unity/stack.h"
#include "unity/hash_table.h"
#include "unity/character_source.h"
#include "unity/main.h"
#include "unity/space.h"
#include "unity/config_file.h"

#include <stdlib.h>

//class SourceProgram
//{
//};

template<class T> class Intel8088Template;

typedef Intel8088Template<void> Intel8088;

class Component
{
public:
    virtual void simulateCycle() = 0;
    virtual String save() = 0;
    virtual Type type() = 0;
    virtual String name() = 0;
    virtual void load(const TypedValue& value) = 0;
};

class Simulator
{
public:
    void simulateCycle()
    {
        for (List<Component*>::Iterator i = _components.start();
            i != _components.end(); ++i) {
            (*i)->simulateCycle();
        }
    }
    void addComponent(Component* component) { _components.add(component); }
    String save() const
    {
        String s("simulator = {");
        for (List<Component*>::Iterator i = _components.start();
            i != _components.end(); ++i) {
            s = (*i)->save();
        }
        s += "}";
    }
    Type type()
    {
        List<StructuredType::Member> members;
        for (List<Component*>::Iterator i = _components.start();
            i != _components.end(); ++i) {
            Type type = (*i)->type();
            if (type.valid())
                members.add(
                    StructuredType::Member((*i)->name(), (*i)->type()));
        }
        return StructuredType("Simulator", members);
    }
    void load(const TypedValue& value)
    {

    }
private:
    List<Component*> _components;
};

class ISA8BitComponent : public Component
{
public:
    void simulateCycle() { }

    // Address bit 31 = write
    // Address bit 30 = IO
    virtual void setAddress(UInt32 address) = 0;
    virtual UInt8 read() = 0;
    virtual void write(UInt8 data) = 0;
    void setBus(ISA8BitBus* bus) { _bus = bus; }
protected:
    ISA8BitBus* _bus;
    bool _active;
};

class ISA8BitBus : public Component
{
public:
    void simulateCycle() { }
    void addComponent(ISA8BitComponent* component)
    {
        _components.add(component);
        component->setBus(this);
    }
private:
    List<ISA8BitComponent*> _components;
};

class Motorola6845CRTC : public Component
{
public:
    void simulateCycle()
    {
    }
};

class IBMCGA : public ISA8BitComponent
{
public:
    void setAddress(UInt32 address)
    {
        _memoryActive = ((address & 0x400f8000) == 0xb8000);
        _memoryAddress = address & 0x00003fff;
        _portActive = ((address & 0x400003f0) == 0x400003d0);
        _active = (_memoryActive || _portActive);
    }
    UInt8 read()
    { 
        if (_memoryActive)
            return _data[_memoryAddress];
        return 0;
    }
    void write(UInt8 data)
    {
        if (_memoryActive)
            _data[_memoryAddress] = data;
    }
private:
    int _memoryAddress;
    bool _memoryActive;
    int _portAddress;
    bool _portActive;
    Array<UInt8> _data;
};

class RAM640Kb : public ISA8BitComponent
{
public:
    RAM640Kb() : _data(0xa0000) { }
    void setAddress(UInt32 address)
    {
        _address = address & 0x400fffff;
        _active = (_address < 0xa0000);
    }
    UInt8 read() { return _data[_address]; }
    void write(UInt8 data) { _data[_address] = data; }
private:
    int _address;
    Array<UInt8> _data;
};

class Intel8253PIT : public ISA8BitComponent
{
};

class Intel8237DMA : public ISA8BitComponent
{
};

class Intel8255PPI : public ISA8BitComponent
{
};

class Intel8259PIC : public ISA8BitComponent
{
};

class ROM : public ISA8BitComponent
{
public:
    void initialize(int mask, int start, File file, int offset)
    {
        _mask = mask | 0xc0000000;
        _start = start;
        String romData = file.contents();
        int length = ((start | ~mask) & 0xfffff) + 1 - start;
        _data.allocate(length);
        for (int i = 0; i < length; ++i)
            _data[i] = romData[i + offset];
    }
    void setAddress(UInt32 address)
    {
        _address = address;
        _active = ((_address & _mask) == _start);
    }
    UInt8 read() { return _data[_address & ~_mask]; }
    void write(UInt8 data) { }
private:
    int _mask;
    int _start;
    int _address;
    Array<UInt8> _data;
    bool _selected;
};


template<class T> class DisassemblerTemplate
{
public:
    void setIntel8088(Intel8088* intel8088) { _intel8088 = intel8088; }
    String disassemble(UInt16 address)
    {
        _bytes = empty;
        String i = disassembleInstruction(address);
        return _bytes.alignLeft(10) + space + i;
    }
private:
    String disassembleInstruction(UInt16 address)
    {
        _address = address;
        _opcode = getByte();
        _wordSize = (_opcode & 1) != 0;
        _doubleWord = false;
        if ((_opcode & 0xc4) == 0)
            return alu(_opcode >> 3) + regMemPair();
        if ((_opcode & 0xc6) == 4)
            return alu(_opcode >> 3) + accum() + String(", ") + imm();
        if ((_opcode & 0xe7) == 6)
            return String("PUSH ") + segreg(_opcode >> 3);
        if ((_opcode & 0xe7) == 7)
            return String("POP ") + segreg(_opcode >> 3);
        if ((_opcode & 0xe7) == 0x26)
            return segreg((_opcode >> 3) & 3) + ": " +
                disassemble(address + 1);
        if ((_opcode & 0xf8) == 0x40)
            return String("INC ") + wordRegs(_opcode & 7);
        if ((_opcode & 0xf8) == 0x48)
            return String("DEC ") + wordRegs(_opcode & 7);
        if ((_opcode & 0xf8) == 0x50)
            return String("PUSH ") + wordRegs(_opcode & 7);
        if ((_opcode & 0xf8) == 0x58)
            return String("POP ") + wordRegs(_opcode & 7);
        if ((_opcode & 0xf0) == 0x60)
            return String("???");
        if ((_opcode & 0xfc) == 0x80) {
            _modRM = getByte();
            String s = alu(reg()) + ea() + ", ";
            return s + (_opcode == 0x81 ? iw() : sb());
        }
        if ((_opcode & 0xfc) == 0x88)
            return String("MOV ") + regMemPair();
        if ((_opcode & 0xf8) == 0x90)
            if (_opcode == 0x90)
                return String("NOP");
            else
                return String("XCHG AX, ") + wordRegs(_opcode & 7);
        if ((_opcode & 0xf8) == 0xb0)
            return String("MOV ") + byteRegs(_opcode & 7) + ", " + ib();
        if ((_opcode & 0xf8) == 0xb8)
            return String("MOV ") + wordRegs(_opcode & 7) + ", " + iw();
        if ((_opcode & 0xf6) == 0xc0)
            return String("???");
        if ((_opcode & 0xfc) == 0xd0) {
            _modRM = getByte();
            String s;
            switch (reg()) {
                case 0: s = "ROL "; break;
                case 1: s = "ROR "; break;
                case 2: s = "RCL "; break;
                case 3: s = "RCR "; break;
                case 4: s = "SHL "; break;
                case 5: s = "SHR "; break;
                case 6: s = "SHL "; break;
                case 7: s = "SAR "; break;
            }
            return s + ea() + String(", ") +
                ((_opcode & 2) == 0 ? String("1") : byteRegs(1));
        }
        if ((_opcode & 0xf8) == 0xd8) {
            _modRM = getByte();
            _wordSize = true;
            return String("ESC ") + String::decimal(_opcode & 7) +
                String(", ") + r() + String(", ") + ea();
        }
        if ((_opcode & 0xf6) == 0xe4)
            return String("IN ") + accum() + String(", ") +
                ((_opcode & 8) == 0 ? ib() : wordRegs(2));
        if ((_opcode & 0xf6) == 0xe6)
            return String("OUT ") + ((_opcode & 8) == 0 ? ib() : wordRegs(2)) +
                String(", ") + accum();
        switch (_opcode) {
            case 0x27: return String("DAA");
            case 0x2f: return String("DAS");
            case 0x37: return String("AAA");
            case 0x3f: return String("AAS");
            case 0x70: return String("JO ") + cb();
            case 0x71: return String("JNO ") + cb();
            case 0x72: return String("JB ") + cb();
            case 0x73: return String("JAE ") + cb();
            case 0x74: return String("JE ") + cb();
            case 0x75: return String("JNE ") + cb();
            case 0x76: return String("JBE ") + cb();
            case 0x77: return String("JA ") + cb();
            case 0x78: return String("JS ") + cb();
            case 0x79: return String("JNS ") + cb();
            case 0x7a: return String("JP ") + cb();
            case 0x7b: return String("JNP ") + cb();
            case 0x7c: return String("JL ") + cb();
            case 0x7d: return String("JGE ") + cb();
            case 0x7e: return String("JLE ") + cb();
            case 0x7f: return String("JG ") + cb();
            case 0x84:
            case 0x85: return String("TEST ") + regMemPair();
            case 0x86:
            case 0x87: return String("XCHG" ) + regMemPair();
            case 0x8c:
                _modRM = getByte();
                _wordSize = true;
                return String("MOV ") + ea() + ", " + segreg(reg());
            case 0x8d:
                _modRM = getByte();
                _doubleWord = true;
                _wordSize = false;
                return String("LEA ") + rw() + ", " + ea();
            case 0x8e:
                _modRM = getByte();
                _wordSize = true;
                return String("MOV ") + segreg(reg()) + ", " + ea();
            case 0x8f: _modRM = getByte(); return String("POP ") + ea();
            case 0x98: return String("CBW");
            case 0x99: return String("CWD");
            case 0x9a: return String("CALL ") + cp();
            case 0x9b: return String("WAIT");
            case 0x9c: return String("PUSHF");
            case 0x9d: return String("POPF");
            case 0x9e: return String("SAHF");
            case 0x9f: return String("LAHF");
            case 0xa0:
            case 0xa1: return String("MOV ") + accum() + String(", ") +
                           size() + String("[") + iw() + String("]");
            case 0xa2:
            case 0xa3: return String("MOV ") + size() + String("[") + iw() +
                           String("]") + String(", ") + accum();
            case 0xa4:
            case 0xa5: return String("MOVS") + size();
            case 0xa6:
            case 0xa7: return String("CMPS") + size();
            case 0xa8:
            case 0xa9: return String("TEST ") + accum() + String(", ") +
                           (!_wordSize ? ib() : iw());
            case 0xaa:
            case 0xab: return String("STOS") + size();
            case 0xac:
            case 0xad: return String("LODS") + size();
            case 0xae:
            case 0xaf: return String("SCAS") + size();
            case 0xc2: return String("RET ") + iw();
            case 0xc3: return String("RET");
            case 0xc4:
                _modRM = getByte();
                _doubleWord = true;
                return String("LDS ") + rw() + ", " + ea();
            case 0xc5:
                _modRM = getByte();
                _doubleWord = true;
                _wordSize = false;
                return String("LES ") + rw() + ", " + ea();
            case 0xc6:
            case 0xc7:
                _modRM = getByte();
                return String("MOV ") + ea() + String(", ") +
                    (!_wordSize ? ib() : iw());
            case 0xca: return String("RETF ") + iw();
            case 0xcb: return String("RETF");
            case 0xcc: return String("INT 3");
            case 0xcd: return String("INT ") + ib();
            case 0xce: return String("INTO");
            case 0xcf: return String("IRET");
            case 0xd4: return String("AAM ") + ib();
            case 0xd5: return String("AAD ") + ib();
            case 0xd6: return String("SALC");
            case 0xd7: return String("XLATB");
            case 0xe0: return String("LOOPNE ") + cb();
            case 0xe1: return String("LOOPE ") + cb();
            case 0xe2: return String("LOOP ") + cb();
            case 0xe3: return String("JCXZ ") + cb();
            case 0xe8: return String("CALL ") + cw();
            case 0xe9: return String("JMP ") + cw();
            case 0xea: return String("JMP ") + cp();
            case 0xeb: return String("JMP ") + cb();
            case 0xf0: return String("LOCK");
            case 0xf1: return String("???");
            case 0xf2: return String("REPNE ") + disassemble(address + 1);
            case 0xf3: return String("REP ") + disassemble(address + 1);
            case 0xf4: return String("HLT");
            case 0xf5: return String("CMC");
            case 0xf6:
            case 0xf7:
                _modRM = getByte();
                switch (reg()) {
                    case 0:
                    case 1: return String("TEST ") + ea() + String(", ") +
                                (!_wordSize ? ib() : iw());
                    case 2: return String("NOT ") + ea();
                    case 3: return String("NEG ") + ea();
                    case 4: return String("NUL ") + ea();
                    case 5: return String("IMUL ") + ea();
                    case 6: return String("DIV ") + ea();
                    case 7: return String("IDIV ") + ea();
                }
            case 0xf8: return String("CLC");
            case 0xf9: return String("STC");
            case 0xfa: return String("CLI");
            case 0xfb: return String("STI");
            case 0xfc: return String("CLD");
            case 0xfd: return String("STD");
            case 0xfe:
            case 0xff:
                _modRM = getByte();
                switch (reg()) {
                    case 0: return String("INC ") + ea();
                    case 1: return String("DEC ") + ea();
                    case 2: return String("CALL ") + ea();
                    case 3: _doubleWord = true; return String("CALL ") + ea();
                    case 4: return String("JMP ") + ea();
                    case 5: _doubleWord = true; return String("JMP ") + ea();
                    case 6: return String("PUSH ") + ea();
                    case 7: return String("??? ") + ea();
                }
        }
        return String("");
    }
    UInt8 getByte()
    {
        UInt8 v = _intel8088->memory(1, _address);
        ++_address;
        _bytes += String::hexadecimal(v, 2);
        return v;
    }
    UInt16 getWord() { UInt8 low = getByte(); return low + (getByte() << 8); }
    String regMemPair()
    {
        _modRM = getByte();
        if ((_opcode & 2) == 0)
            return ea() + String(", ") + r();
        return r() + String(", ") + ea();
    }
    String r() { return !_wordSize ? rb() : rw(); }
    String rb() { return byteRegs(reg()); }
    String rw() { return wordRegs(reg()); }
    String byteRegs(int r)
    {
        static String b[8] = {"AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH"};
        return b[r];
    }
    String wordRegs(int r)
    {
        static String w[8] = {"AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI"};
        return w[r];
    }
    String ea()
    {
        String s;
        switch (mod()) {
            case 0: s = disp(); break;
            case 1: s = disp() + sb(); break;
            case 2: s = disp() + iw(); break;
            case 3: return !_wordSize ? byteRegs(rm()) : wordRegs(rm());
        }
        return size() + String("[") + s + String("]");
    }
    String size()
    {
        if (!_doubleWord)
            return (!_wordSize ? String("B") : String("W"));
        else
            return (!_wordSize ? String("") : String("D"));
    }
    String disp()
    {
        static String d[8] = {
            "BX+SI", "BX+DI", "BP+SI", "BP+DI", "SI", "DI", "BP", "BX"};
        if (mod() == 0 && rm() == 6)
            return iw();
        return d[rm()];
    }
    String alu(int op)
    {
        static String o[8] = {
            "ADD ", "OR ", "ADC ", "SBB ", "AND ", "SUB ", "XOR ", "CMP "};
        return o[op];
    }
    int mod() { return _modRM >> 6; }
    int reg() { return (_modRM >> 3) & 7; }
    int rm() { return _modRM & 7; }
    String iw() { return String::hexadecimal(getWord(), 4); }
    String ib() { return String::hexadecimal(getByte(), 2); }
    String sb()
    {
        UInt8 byte = getByte();
        if ((byte & 0x80) == 0)
            return String("+") + String::hexadecimal(byte, 2);
        return String("-") + String::hexadecimal((-byte) & 0x7f, 2);
    }
    String imm() { return !_wordSize ? ib() : iw(); }
    String accum() { return !_wordSize ? String("AL") : String("AX"); }
    String segreg(int r)
    {
        static String sr[8] = {"ES", "CS", "SS", "DS", "??", "??", "??", "??"};
        return sr[r];
    }
    String cb()
    {
        SInt8 byte = static_cast<SInt8>(getByte());
        return String::hexadecimal(_address + byte, 4);
    }
    String cw() { return String::hexadecimal(_address + getWord(), 4); }
    String cp()
    {
        UInt16 offset = getWord();
        return String::hexadecimal(getWord(), 4) + String(":") +
            String::hexadecimal(offset, 4);
    }

    Intel8088* _intel8088;
    UInt16 _address;
    UInt8 _opcode;
    UInt8 _modRM;
    bool _wordSize;
    bool _doubleWord;
    String _bytes;
};

typedef DisassemblerTemplate<void> Disassembler;

template<class T> class Intel8088Template
{
public:
    Intel8088Template(Handle* console)
      : _flagsData(0x0002),  // ?
        _state(stateFetch),
        _ip(0xfff0),
        _prefetchOffset(0),
        _prefetched(0),
        _segmentOverride(-1),
        _prefetchAddress(_ip),
        _ioRequested(ioNone),
        _ioInProgress(ioInstructionFetch),
        _busState(t1),
        _abandonFetch(false),
        _useIO(false),
        _halted(false),
        _console(console),
        _wait(0),
        _newInstruction(true),
        _newIP(0xfff0)
    {
        static String b[8] = {"AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH"};
        static String w[8] = {"AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI"};
        static String s[8] = {"ES", "CS", "SS", "DS", "??", "??", "??", "??"};
        for (int i = 0; i < 8; ++i) {
            _registerData[i] = 0;  // ?
            _wordRegisters[i].init(w[i], &_registerData[i]);
            _byteRegisters[i].init(b[i], reinterpret_cast<UInt8*>(
                &_registerData[i & 3]) + (i >= 4 ? 1 : 0));
            _segmentRegisters[i].init(s[i], &_segmentRegisterData[i]);
        }
        _flags.init("F", &_flagsData);
        _segmentRegisterData[0] = 0x0000;  // ?
        _segmentRegisterData[1] = 0xf000;
        _segmentRegisterData[2] = 0x0000;  // ?
        _segmentRegisterData[3] = 0x0000;  // ?
        _segmentRegisterData[7] = 0x0000;  // For IO accesses
        _memory.allocate(0x100000);
    }
    void setBus(ISA8BitBus* bus) { _bus = bus; }
    UInt8& memory(UInt32 physicalAddress)
    {
        return _memory[physicalAddress];
    }
    UInt8& memory(int segment, UInt16 offset)
    {
        return memory(physicalAddress(_segmentRegisterData[segment], offset));
    }
    void simulate()
    {
        Disassembler disassembler;
        disassembler.setIntel8088(this);
        int cycle = 0;

        do {
            String line = String::decimal(cycle).alignRight(5) + space;
            switch (_busState) {
                case t1:
                    {
                        UInt32 a;
                        if (_ioInProgress != ioInstructionFetch) {
                            int segment = _segment;
                            if (_segmentOverride != -1)
                                segment = _segmentOverride;
                            a = physicalAddress(_segmentRegisterData[segment],
                                _address);
                        }
                        else
                            a = physicalAddress(csQuiet(), _prefetchAddress);
                        line += String("T1 ") + String::hexadecimal(a, 5) +
                            space;
                    }
                    break;
                case t2:
                    line += "T2 ";
                    if (_ioInProgress == ioWrite)
                        line += String("M<-") + String::hexadecimal(_data, 2) +
                            space;
                    else
                        line += space*6;
                    break;
                case t3: line += "T3       "; break;
                case tWait: line += "Tw       "; break;
                case t4:
                    {
                        UInt8 d;
                        if (_ioInProgress != ioInstructionFetch)
                            d = _data;
                        else
                            d = memory(1, _prefetchAddress - 1);
                        line += "T4 ";
                        if (_ioInProgress == ioWrite)
                            line += space*6;
                        else
                            if (_abandonFetch)
                                line += "----- ";
                            else
                                line += String("M->") +
                                    String::hexadecimal(d, 2) + space;
                    }
                    break;
                case tIdle: line += "         "; break;
            }
            if (_newInstruction) {
                line += String::hexadecimal(csQuiet(), 4) + colon +
                    String::hexadecimal(_newIP, 4) + space +
                    disassembler.disassemble(_newIP);
            }
            line = line.alignLeft(50);
            for (int i = 0; i < 8; ++i) {
                line += _byteRegisters[i].text();
                line += _wordRegisters[i].text();
                line += _segmentRegisters[i].text();
            }
            line += _flags.text();
            _newInstruction = false;
            _console->write(line + newLine);
            ++cycle;
            simulateCycle();
        } while (!_halted);
    }
    void simulateCycle()
    {
        bool busDone;
        do {
            busDone = true;
            switch (_busState) {
                case t1:
                    _busState = t2;
                    break;
                case t2:
                    _busState = t3;
                    break;
                case t3:
                    _busState = tWait;
                    busDone = false;
                    return;
                case tWait:
                    switch (_ioInProgress) {
                        case ioRead:
                            {
                                _ioRequested = ioNone;
                                switch (_byte) {
                                    case ioSingleByte:
                                        _data = memory();
                                        _state = _afterIO;
                                        break;
                                    case ioWordFirst:
                                        _data = memory();
                                        _ioInProgress = ioRead;
                                        _byte = ioWordSecond;
                                        ++_address;
                                        break;
                                    case ioWordSecond:
                                        _data |= static_cast<UInt16>(
                                            memory()) << 8;
                                        _state = _afterIO;
                                        _byte = ioSingleByte;
                                        break;
                                }
                            }
                            break;
                        case ioWrite:
                            {
                                _ioRequested = ioNone;
                                switch (_byte) {
                                    case ioSingleByte:
                                        memory() = _data;
                                        _state = _afterIO;
                                        break;
                                    case ioWordFirst:
                                        memory() = _data;
                                        _ioInProgress = ioWrite;
                                        _byte = ioWordSecond;
                                        ++_address;
                                        break;
                                    case ioWordSecond:
                                        memory() = _data >> 8;
                                        _state = _afterIO;
                                        _byte = ioSingleByte;
                                        break;
                                }
                            }
                            break;
                        case ioInstructionFetch:
                            if (!_abandonFetch) {
                                _prefetchQueue[
                                    (_prefetchOffset + _prefetched) & 3] =
                                    memory(1, _prefetchAddress);
                                ++_prefetched;
                                ++_prefetchAddress;
                            }
                            completeInstructionFetch();
                            break;
                    }
                    _busState = t4;
                    return;
                case t4:
                    _busState = tIdle;
                    busDone = false;
                    break;
                case tIdle:
                    if (_byte != ioWordSecond)
                        _ioInProgress = ioNone;
                    _abandonFetch = false;
                    if (_ioInProgress == ioNone && _ioRequested != ioNone) {
                        _ioInProgress = _ioRequested;
                        _busState = t1;
                    }
                    if (_ioInProgress == ioNone && _prefetched < 4) {
                        _ioInProgress = ioInstructionFetch;
                        _busState = t1;
                    }
                    busDone = true;
                    break;
            }
        } while (!busDone);
        do {
            if (_wait > 0) {
                --_wait;
                return;
            }
            switch (_state) {
                case stateWaitingForBIU:
                    return;

                case stateFetch: fetch(stateFetch2, false); break;
                case stateFetch2:
                    {
                        _opcode = _data;
                        _wordSize = ((_opcode & 1) != 0);
                        _sourceIsRM = ((_opcode & 2) != 0);
                        static State stateForOpcode[0x100] = {
stateALU,          stateALU,          stateALU,          stateALU,          
stateALUAccumImm,  stateALUAccumImm,  statePushSegReg,   statePopSegReg,
stateALU,          stateALU,          stateALU,          stateALU,          
stateALUAccumImm,  stateALUAccumImm,  statePushSegReg,   statePopSegReg,
stateALU,          stateALU,          stateALU,          stateALU,          
stateALUAccumImm,  stateALUAccumImm,  statePushSegReg,   statePopSegReg,
stateALU,          stateALU,          stateALU,          stateALU,          
stateALUAccumImm,  stateALUAccumImm,  statePushSegReg,   statePopSegReg,
stateALU,          stateALU,          stateALU,          stateALU,          
stateALUAccumImm,  stateALUAccumImm,  stateSegOverride,  stateDAA,
stateALU,          stateALU,          stateALU,          stateALU,          
stateALUAccumImm,  stateALUAccumImm,  stateSegOverride,  stateDAS,
stateALU,          stateALU,          stateALU,          stateALU,          
stateALUAccumImm,  stateALUAccumImm,  stateSegOverride,  stateAAA,
stateALU,          stateALU,          stateALU,          stateALU,          
stateALUAccumImm,  stateALUAccumImm,  stateSegOverride,  stateAAS,
stateIncDecRW,     stateIncDecRW,     stateIncDecRW,     stateIncDecRW,     
stateIncDecRW,     stateIncDecRW,     stateIncDecRW,     stateIncDecRW,
stateIncDecRW,     stateIncDecRW,     stateIncDecRW,     stateIncDecRW,     
stateIncDecRW,     stateIncDecRW,     stateIncDecRW,     stateIncDecRW,
statePushRW,       statePushRW,       statePushRW,       statePushRW,       
statePushRW,       statePushRW,       statePushRW,       statePushRW,
statePopRW,        statePopRW,        statePopRW,        statePopRW,        
statePopRW,        statePopRW,        statePopRW,        statePopRW,
stateInvalid,      stateInvalid,      stateInvalid,      stateInvalid,      
stateInvalid,      stateInvalid,      stateInvalid,      stateInvalid,
stateInvalid,      stateInvalid,      stateInvalid,      stateInvalid,      
stateInvalid,      stateInvalid,      stateInvalid,      stateInvalid,
stateJCond,        stateJCond,        stateJCond,        stateJCond,        
stateJCond,        stateJCond,        stateJCond,        stateJCond,
stateJCond,        stateJCond,        stateJCond,        stateJCond,        
stateJCond,        stateJCond,        stateJCond,        stateJCond,
stateALURMImm,     stateALURMImm,     stateALURMImm,     stateALURMImm,     
stateTestRMReg,    stateTestRMReg,    stateXchgRMReg,    stateXchgRMReg,
stateMovRMReg,     stateMovRMReg,     stateMovRegRM,     stateMovRegRM,     
stateMovRMWSegReg, stateLEA,          stateMovSegRegRMW, statePopMW,
stateXchgAxRW,     stateXchgAxRW,     stateXchgAxRW,     stateXchgAxRW,     
stateXchgAxRW,     stateXchgAxRW,     stateXchgAxRW,     stateXchgAxRW,
stateCBW,          stateCWD,          stateCallCP,       stateWait,         
statePushF,        statePopF,         stateSAHF,         stateLAHF,
stateMovAccumInd,  stateMovAccumInd,  stateMovIndAccum,  stateMovIndAccum,  
stateMovS,         stateMovS,         stateCmpS,         stateCmpS,
stateTestAccumImm, stateTestAccumImm, stateStoS,         stateStoS,         
stateLodS,         stateLodS,         stateScaS,         stateScaS,
stateMovRegImm,    stateMovRegImm,    stateMovRegImm,    stateMovRegImm,    
stateMovRegImm,    stateMovRegImm,    stateMovRegImm,    stateMovRegImm,
stateMovRegImm,    stateMovRegImm,    stateMovRegImm,    stateMovRegImm,    
stateMovRegImm,    stateMovRegImm,    stateMovRegImm,    stateMovRegImm,
stateInvalid,      stateInvalid,      stateRet,          stateRet,          
stateLoadFar,      stateLoadFar,      stateMovRMImm,     stateMovRMImm,
stateInvalid,      stateInvalid,      stateRet,          stateRet,          
stateInt,          stateInt,          stateIntO,         stateIRet,
stateShift,        stateShift,        stateShift,        stateShift,        
stateAAM,          stateAAD,          stateSALC,         stateXlatB,
stateEscape,       stateEscape,       stateEscape,       stateEscape,       
stateEscape,       stateEscape,       stateEscape,       stateEscape,
stateLoop,         stateLoop,         stateLoop,         stateLoop,         
stateInOut,        stateInOut,        stateInOut,        stateInOut,
stateCallCW,       stateJmpCW,        stateJmpCP,        stateJmpCB,        
stateInOut,        stateInOut,        stateInOut,        stateInOut,
stateLock,         stateInvalid,      stateRep,          stateRep,          
stateHlt,          stateCmC,          stateMath,         stateMath,
stateLoadC,        stateLoadC,        stateLoadI,        stateLoadI,        
stateLoadD,        stateLoadD,        stateMisc,         stateMisc};
                        _state = stateForOpcode[_opcode];
                    }
                    break;
                case stateEndInstruction:
                    _segmentOverride = -1;
                    _state = stateFetch;
                    _rep = 0;
                    _useIO = false;
                    _newInstruction = true;
                    _newIP = _ip;
                    break;

                case stateModRM:
                    _afterModRM = _afterIO;
                    fetch(stateModRM2, false);
                    break;
                case stateModRM2:
                    _afterIO = _afterModRM;
                    _modRM = _data;
                    if ((_modRM & 0xc0) == 0xc0) {
                        _useMemory = false;
                        _address = _modRM & 7;
                        _state = _afterEA;
                        break;
                    }
                    _useMemory = true;
                    switch (_modRM & 7) {
                        case 0: _wait = 7; _address = bx() + si(); break;
                        case 1: _wait = 8; _address = bx() + di(); break;
                        case 2: _wait = 8; _address = bp() + si(); break;
                        case 3: _wait = 7; _address = bp() + di(); break;
                        case 4: _wait = 5; _address =        si(); break;
                        case 5: _wait = 5; _address =        di(); break;
                        case 6: _wait = 5; _address = bp();        break;
                        case 7: _wait = 5; _address = bx();        break;
                    }
                    switch (_modRM & 0xc0) {
                        case 0x00:
                            if ((_modRM & 7) == 6)
                                fetch(stateEAOffset, true);
                            else
                                _state = stateEASetSegment;
                            break;
                        case 0x40: fetch(stateEAByte, false); break;
                        case 0x80: fetch(stateEAWord, true); break;
                    }
                    break;
                case stateEAOffset:
                    _address = _data;
                    _segment = 3;
                    _wait = 6;
                    _state = _afterEA;
                    break;
                case stateEAByte:
                    _address += signExtend(_data);
                    _wait += 4;
                    _state = stateEASetSegment;
                    break;
                case stateEAWord:
                    _address += _data;
                    _wait += 4;
                    _state = stateEASetSegment;
                    break;
                case stateEASetSegment:
                    {
                        static int segments[8] = {3, 3, 2, 2, 3, 3, 2, 3};
                        _segment = segments[_modRM & 7];
                    }
                    _state = _afterEA;
                    break;

                case stateIO:
                    if (_useMemory)
                        initIO(_afterIO, _ioType, _wordSize);
                    else {
                        if (!_wordSize)
                            if (_ioType == ioRead)
                                _data = _byteRegisters[_address];
                            else
                                _byteRegisters[_address] = _data;
                        else
                            if (_ioType == ioRead)
                                _data = _wordRegisters[_address];
                            else
                                _wordRegisters[_address] = _data;
                        _state = _afterIO;
                    }
                    break;

                case statePush:
                    sp() -= 2;
                case statePush2:
                    _address = sp();
                    _segment = 2;
                    initIO(_afterIO, ioWrite, true);
                    break;
                case statePop:
                    _address = sp();
                    sp() += 2;
                    _segment = 2;
                    initIO(_afterIO, ioRead, true);
                    break;

                case stateALU: readEA(stateALU2); break;
                case stateALU2:
                    _wait = 5;
                    if (!_sourceIsRM) {
                        _destination = _data;
                        _source = getReg();
                        _wait += 3;
                    }
                    else {
                        _destination = getReg();
                        _source = _data;
                    }
                    if (!_useMemory)
                        _wait = 3;
                    _aluOperation = (_opcode >> 3) & 7;
                    _state = stateALU3;
                    break;
                case stateALU3:
                    doALUOperation();
                    if (_aluOperation != 7) {
                        if (!_sourceIsRM)
                            writeEA(_data, 0);
                        else {
                            setReg(_data);
                            _state = stateEndInstruction;
                        }
                    }
                    else
                        _state = stateEndInstruction;
                    break;

                case stateALUAccumImm:
                    fetch(stateALUAccumImm2, _wordSize);
                    break;
                case stateALUAccumImm2:
                    _destination = getAccum();
                    _source = _data;
                    _aluOperation = (_opcode >> 3) & 7;
                    doALUOperation();
                    if (_aluOperation != 7)
                        setAccum();
                    end(4);
                    break;

                case statePushSegReg:
                    push(_segmentRegisters[_opcode >> 3]);
                    break;

                case statePopSegReg: pop(statePopSegReg2); break;
                case statePopSegReg2:
                    _segmentRegisters[_opcode >> 3] = _data;
                    end(4);
                    break;

                case stateSegOverride:
                    _segmentOverride = (_opcode >> 3) & 3;
                    _state = stateFetch;
                    _wait = 2;
                    break;

                case stateDAA:
                    if (af() || (al() & 0x0f) > 9) {
                        _data = al() + 6;
                        al() = _data;
                        setAF(true);
                        if ((_data & 0x100) != 0)
                            setCF(true);
                    }
                    if (cf() || al() > 0x9f) {
                        al() += 0x60;
                        setCF(true);
                    }
                    _state = stateDA;
                    break;
                case stateDAS:
                    {
                        UInt8 t = al();
                        if (af() || ((al() & 0xf) > 9)) {
                            _data = al() - 6;
                            al() = _data;
                            setAF(true);
                            if ((_data & 0x100) != 0)
                                setCF(true);
                        }
                        if (cf() || t > 0x9f) {
                            al() -= 0x60;
                            setCF(true);
                        }
                    }
                    _state = stateDA;
                    break;
                case stateDA:
                    _wordSize = false;
                    _data = al();
                    setPZS();
                    end(4);
                    break;
                case stateAAA:
                    if (af() || ((al() & 0xf) > 9)) {
                        al() += 6;
                        ++ah();
                        setCA();
                    }
                    else
                        clearCA();
                    _state = stateAA;
                    break;
                case stateAAS:
                    if (af() || ((al() & 0xf) > 9)) {
                        al() -= 6;
                        --ah();
                        setCA();
                    }
                    else
                        clearCA();
                    _state = stateAA;
                    break;
                case stateAA:
                    al() &= 0x0f;
                    end(4);
                    break;

                case stateIncDecRW:
                    _destination = rw();
                    _source = 1;
                    _wordSize = true;
                    if ((_opcode & 8) == 0) {
                        _data = _destination + _source;
                        setOFAdd();
                    }
                    else {
                        _data = _destination - _source;
                        setOFSub();
                    }
                    doAF();
                    setPZS();
                    rw() = _data;
                    end(2);
                    break;
                case statePushRW:
                    sp() -= 2;
                    _data = rw();
                    _afterIO = stateEndInstruction;
                    _state = statePush2;
                    _wait += 7;
                    break;
                case statePopRW: pop(statePopRW2); break;
                case statePopRW2: rw() = _data; end(4); break;

                case stateInvalid: end(0); break;

                case stateJCond: fetch(stateJCond2, false); break;
                case stateJCond2:
                    {
                        bool jump;
                        switch (_opcode & 0x0e) {
                            case 0x00: jump =  of(); break;
                            case 0x02: jump =  cf(); break;
                            case 0x04: jump =  zf(); break;
                            case 0x06: jump =  cf() || zf(); break;
                            case 0x08: jump =  sf(); break;
                            case 0x0a: jump =  pf(); break;
                            case 0x0c: jump = (sf() != of()); break;
                            case 0x0e: jump = (sf() != of()) || zf(); break;
                        }
                        if ((_opcode & 1) != 0)
                            jump = !jump;
                        if (jump)
                            jumpShort();
                        end(jump ? 16 : 4);
                    }
                    break;

                case stateALURMImm: readEA(stateALURMImm2); break;
                case stateALURMImm2:
                    _destination = _data;
                    fetch(stateALURMImm3, _opcode == 0x81);
                    break;
                case stateALURMImm3:
                    if (_opcode != 0x83)
                        _source = _data;
                    else
                        _source = signExtend(_data);
                    _aluOperation = modRMReg();
                    _wait = 9;
                    if (_aluOperation == 7)
                        _wait = 6;
                    if (!_useMemory)
                        _wait = 4;
                    doALUOperation();
                    if (_aluOperation != 7)
                        writeEA(stateEndInstruction, _wait);
                    else
                        _state = stateEndInstruction;
                    break;

                case stateTestRMReg: readEA(stateTestRMReg2); break;
                case stateTestRMReg2:
                    test(_data, getReg());
                    end(_useMemory ? 5 : 3);
                    break;
                case stateXchgRMReg: readEA(stateXchgRMReg2); break;
                case stateXchgRMReg2:
                    _source = getReg();
                    setReg(_data);
                    writeEA(_source, _useMemory ? 9 : 4);
                    break;
                case stateMovRMReg: loadEA(stateMovRMReg2); break;
                case stateMovRMReg2:
                    writeEA(getReg(), _useMemory ? 5 : 2);
                    break;
                case stateMovRegRM: readEA(stateMovRegRM2); break;
                case stateMovRegRM2:
                    setReg(_data);
                    end(_useMemory ? 4 : 2);
                    break;
                case stateMovRMWSegReg:
                    _wordSize = true;
                    loadEA(stateMovRMWSegReg2);
                    break;
                case stateMovRMWSegReg2:
                    writeEA(_segmentRegisters[modRMReg() & 3],
                        _useMemory ? 5 : 2);
                    break;
                case stateLEA: loadEA(stateLEA2); break;
                case stateLEA2: setReg(_address); end(2); break;
                case stateMovSegRegRMW:
                    _wordSize = true;
                    readEA(stateMovSegRegRMW2);
                    break;
                case stateMovSegRegRMW2:
                    _segmentRegisters[modRMReg() & 3] = _data;
                    end(_useMemory ? 4 : 2);
                    break;
                case statePopMW:
                    _afterIO = statePopMW2;
                    loadEA(statePop);
                    break;
                case statePopMW2: writeEA(_data, _useMemory ? 9 : 12); break;
                case stateXchgAxRW:
                    _data = rw();
                    rw() = ax();
                    ax() = _data;
                    end(3);
                    break;
                case stateCBW: ax() = signExtend(al()); end(2); break;
                case stateCWD:
                    dx() = ((ax() & 0x8000) == 0 ? 0x0000 : 0xffff);
                    end(5);
                    break;
                case stateCallCP: fetch(stateCallCP2, true); break;
                case stateCallCP2:
                    _savedIP = _data;
                    fetch(stateCallCP3, true);
                    break;
                case stateCallCP3:
                    _savedCS = _data;
                    _wait = 2;
                    push(cs(), stateCallCP4);
                    break;
                case stateCallCP4: push(_ip, stateJmpCP4); break;
                case stateWait: end(4); break;
                case statePushF: push(_flags & 0x0fd7); break;
                case statePopF: pop(statePopF2); break;
                case statePopF2: _flags = _data | 2; end(4); break;
                case stateSAHF:
                    _flags = (_flags & 0xff02) | ah();
                    end(4);
                    break;
                case stateLAHF: ah() = _flags & 0xd7; end(4); break;

                case stateMovAccumInd: fetch(stateMovAccumInd2, true); break;
                case stateMovAccumInd2:
                    _address = _data;
                    initIO(stateMovAccumInd3, ioRead, _wordSize);
                    break;
                case stateMovAccumInd3: setAccum(); end(6); break;
                case stateMovIndAccum: fetch(stateMovIndAccum2, true); break;
                case stateMovIndAccum2:
                    _address = _data;
                    _data = getAccum();
                    _wait = 6;
                    initIO(stateEndInstruction, ioWrite, _wordSize);
                    break;

                case stateMovS:
                    _wait = (_rep != 0 ? 9 : 10);
                    lodS(stateMovS2);
                    break;
                case stateMovS2:
                    _afterRep = stateMovS;
                    stoS(stateRepAction);
                    break;
                case stateRepAction:
                    _state = stateEndInstruction;
                    if (_rep != 0) {
                        --cx();
                        if (cx() == 0 || (zf() == (_rep == 1)))
                            _state = _afterRep;
                    }
                    break;
                case stateCmpS: _wait = 14; lodS(stateCmpS2); break;
                case stateCmpS2:
                    _destination = _data;
                    lodDIS(stateCmpS3);
                    break;
                case stateCmpS3:
                    _source = _data;
                    sub();
                    _afterRep = stateCmpS;
                    _state = stateRepAction;
                    break;

                case stateTestAccumImm:
                    fetch(stateTestAccumImm2, _wordSize);
                    break;
                case stateTestAccumImm2:
                    test(getAccum(), _data);
                    end(4);
                    break;

                case stateStoS:
                    _wait = (_rep != 0 ? 6 : 7);
                    _data = getAccum();
                    _afterRep = stateStoS;
                    stoS(stateRepAction);
                    break;
                case stateLodS:
                    _wait = (_rep != 0 ? 9 : 8);
                    lodS(stateLodS2);
                    break;
                case stateLodS2:
                    setAccum();
                    _afterRep = stateLodS;
                    _state = stateRepAction;
                    break;
                case stateScaS: _wait = 11; lodDIS(stateScaS2); break;
                case stateScaS2:
                    _destination = getAccum();
                    _source = _data;
                    sub();
                    _afterRep = stateScaS;
                    _state = stateRepAction;
                    break;

                case stateMovRegImm:
                    _wordSize = ((_opcode & 8) != 0);
                    fetch(stateMovRegImm2, _wordSize);
                    break;
                case stateMovRegImm2:
                    if (_wordSize)
                        rw() = _data;
                    else
                        rb() = _data;
                    end(4); 
                    break;

                case stateRet: pop(stateRet2); break;
                case stateRet2:
                    _savedIP = _data;
                    if ((_opcode & 8) == 0) {
                        _savedCS = _segmentRegisters[1];
                        _state = stateRet4;
                        _wait = (!_wordSize ? 16 : 12);
                    }
                    else {
                        pop(stateRet3);
                        _wait = (!_wordSize ? 17 : 18);
                    }
                    break;
                case stateRet3:
                    _savedCS = _data;
                    _state = stateRet4;
                    break;
                case stateRet4:
                    if (!_wordSize)
                        fetch(stateRet5, true);
                    else
                        _state = stateRet6;
                    break;
                case stateRet5:
                    sp() += _data;
                    _state = stateRet6;
                    break;
                case stateRet6:
                    _segmentRegisters[1] = _savedCS;
                    setIP(_savedIP);
                    end(0);
                    break;

                case stateLoadFar: readEA(stateLoadFar2); break;
                case stateLoadFar2:
                    setReg(_data);
                    _afterIO = stateLoadFar3;
                    _state = stateLoadFar3;
                    break;
                case stateLoadFar3:
                    _segmentRegisters[!_wordSize ? 0 : 3] = _data;
                    end(8);
                    break;

                case stateMovRMImm: loadEA(stateMovRMImm2); break;
                case stateMovRMImm2: fetch(stateMovRMImm2, _wordSize); break;
                case stateMovRMImm3: writeEA(_data, _useMemory ? 6 : 4); break;

                case stateInt: interrupt(3); _wait = 1; break;
                case stateInt3: fetch(stateIntAction, false); break;
                case stateIntAction:
                    _source = _data;
                    push(_flags & 0x0fd7, stateIntAction2);
                    break;
                case stateIntAction2:
                    setIF(false);
                    setTF(false);
                    push(cs(), stateIntAction3);
                    break;
                case stateIntAction3: push(_ip, stateIntAction4); break;
                case stateIntAction4:
                    _address = _source << 2;
                    _segment = 1;
                    cs() = 0;
                    initIO(stateIntAction5, ioRead, true);
                    break;
                case stateIntAction5:
                    _savedIP = _data;
                    _address += 2;
                    initIO(stateIntAction6, ioRead, true);
                    break;
                case stateIntAction6:
                    cs() = _data;
                    setIP(_savedIP);
                    _wait = 13;
                    break;
                case stateIntO:
                    if (of()) {
                        interrupt(4);
                        _wait = 2;
                    }
                    else
                        end(4);
                    break;
                case stateIRet: pop(stateIRet2); break;
                case stateIRet2: _savedIP = _data; pop(stateIRet3); break;
                case stateIRet3: _savedCS = _data; pop(stateIRet4); break;
                case stateIRet4:
                    _flags = _data | 2;
                    cs() = _savedCS;
                    setIP(_savedIP);
                    _wait = 20;
                    break;

                case stateShift: readEA(stateShift2); break;
                case stateShift2:
                    if ((_opcode & 2) == 0) {
                        _source = 1;
                        _wait = (_useMemory ? 7 : 2);
                    }
                    else {
                        _source = cl();
                        _wait = (_useMemory ? 12 : 8);
                    }
                    _state = stateShift3;
                    break;
                case stateShift3:
                    if (_source == 0) {
                        writeEA(_data, 0);
                        break;
                    }
                    _destination = _data;
                    switch (modRMReg()) {
                        case 0:  // ROL
                            _data <<= 1;
                            doCF();
                            _data |= (cf() ? 1 : 0);
                            setOFRotate();
                            break;
                        case 1:  // ROR
                            setCF((_data & 1) != 0);
                            _data >>= 1;
                            if (cf())
                                _data |= (!_wordSize ? 0x80 : 0x8000);
                            setOFRotate();
                            break;
                        case 2:  // RCL
                            _data = (_data << 1) | (cf() ? 1 : 0);
                            doCF();
                            setOFRotate();
                            break;
                        case 3:  // RCR
                            _data >>= 1;
                            if (cf())
                                _data |= (!_wordSize ? 0x80 : 0x8000);
                            setCF((_destination & 1) != 0);
                            setOFRotate();
                            break;
                        case 4:  // SHL
                        case 6:
                            _data <<= 1;
                            doCF();
                            setOFRotate();
                            setPZS();
                            break;
                        case 5:  // SHR
                            setCF((_data & 1) != 0);
                            _data >>= 1;
                            setOFRotate();
                            setAF(true);
                            setPZS();
                            break;
                        case 7:  // SAR
                            setCF((_data & 1) != 0);
                            _data >>= 1;
                            if (!_wordSize)
                                _data |= (_destination & 0x80);
                            else
                                _data |= (_destination & 0x8000);
                            setOFRotate();
                            setAF(true);
                            setPZS();
                            break;
                    }
                    if ((_opcode & 2) != 0)
                        _wait = 4;
                    --_source;
                    break;

                case stateAAM: fetch(stateAAM2, false); break;
                case stateAAM2:
                    if (_data == 0)
                        interrupt(0);
                    else {
                        ah() = al() / _data;
                        al() %= _data;
                        _wordSize = true;
                        setPZS();
                        end(83);
                    }
                    break;
                case stateAAD: fetch(stateAAD2, false); break;
                case stateAAD2:
                    al() += ah()*_data;
                    ah() = 0;
                    setPZS();
                    end(60);
                    break;

                case stateSALC: al() = (cf() ? 0xff : 0x00); end(4); break;

                case stateXlatB:
                    _address = bx() + al();
                    initIO(stateXlatB2, ioRead, false);
                    break;
                case stateXlatB2:
                    al() = _data;
                    end(7);
                    break;

                case stateEscape: loadEA(stateEscape2); break;
                case stateEscape2: end(2); break;

                case stateLoop: fetch(stateLoop2, false); break;
                case stateLoop2:
                    {
                        bool jump;
                        if (_opcode != 0xe3) {
                            _wait = 5;
                            --cx();
                            jump = (cx() != 0);
                            switch (_opcode) {
                                case 0xe0:
                                    if (zf())
                                        jump = false;
                                    _wait = 6;
                                    break;
                                case 0xe1: if (!zf()) jump = false; break;
                            }
                        }
                        else {
                            _wait = 6;
                            jump = (cx() == 0);
                        }
                        if (jump) {
                            jumpShort();
                            _wait = _opcode == 0xe0 ? 19 :
                                _opcode == 0xe2 ? 17 : 18;
                        }
                    }
                    end(_wait);
                    break;

                case stateInOut:
                    if ((_opcode & 8) == 0) {
                        fetch(stateInOut2, false);
                        _wait = 2;
                    }
                    else {
                        _data = dx();
                        _state = stateInOut2;
                    }
                    break;
                case stateInOut2:
                    _useIO = true;
                    _ioType = ((_opcode & 2) == 0 ? ioRead : ioWrite);
                    _segment = 7;
                    initIO(stateInOut3, _ioType, _wordSize);
                    break;
                case stateInOut3:
                    if (_ioType == ioRead)
                        setAccum();
                    end(4);
                    break;

                case stateCallCW: fetch(stateCallCW2, true); break;
                case stateCallCW2:
                    _savedIP = _data + _ip;
                    _wait = 3;
                    _state = stateCallCW3;
                    break;
                case stateCallCW3: push(_ip, stateJmpCW3); break;

                case stateJmpCW: fetch(stateJmpCW2, true); break;
                case stateJmpCW2:
                    _savedIP = _data + _ip;
                    _wait = 9;
                    _state = stateJmpCW3;
                    break;
                case stateJmpCW3: setIP(_savedIP); end(6); break;

                case stateJmpCP: fetch(stateJmpCP2, true); break;
                case stateJmpCP2:
                    _savedIP = _data;
                    fetch(stateJmpCP3, true);
                    break;
                case stateJmpCP3:
                    _savedCS = _data;
                    _wait = 9;
                    _state = stateJmpCP4;
                    break;
                case stateJmpCP4: cs() = _savedCS; _state = stateJmpCW3; break;

                case stateJmpCB: fetch(stateJmpCB2, false); break;
                case stateJmpCB2: jumpShort(); end(15); break;

                case stateLock: end(2); break;
                case stateRep:
                    _rep = (_opcode == 0xf2 ? 1 : 2);
                    _wait = 9;
                    _state = stateFetch;
                    break;
                case stateHlt: _halted = true; end(2); break;
                case stateCmC: _flags ^= 1; end(2); break;

                case stateMath: readEA(stateMath2); break;
                case stateMath2:
                    if ((modRMReg() & 6) == 0) {
                        _destination = _data;
                        fetch(stateMath3, _wordSize);
                    }
                    else
                        _state = stateMath3;
                    break;
                case stateMath3:
                    switch (modRMReg()) {
                        case 0:
                        case 1:  // TEST
                            test(_destination, _data);
                            end(_useMemory ? 7 : 5);
                            break;
                        case 2:  // NOT
                            writeEA(~_data, _useMemory ? 8 : 3);
                            break;
                        case 3:  // NEG
                            _source = _data;
                            _destination = 0;
                            sub();
                            writeEA(_data, _useMemory ? 8 : 3);
                            break;
                        case 4:  // MUL
                        case 5:  // IMUL
                            _source = _data;
                            _destination = getAccum();
                            _data = _destination;
                            setSF();
                            setPF();
                            _data *= _source;
                            ax() = _data;
                            if (!_wordSize)
                                if (modRMReg() == 4) {
                                    setCF(ah() != 0);
                                    _wait = 70;
                                }
                                else {
                                    if ((_source & 0x80) != 0)
                                        ah() += _destination;
                                    if ((_destination & 0x80) != 0)
                                        ah() += _source;
                                    setCF(ah() ==
                                        ((al() & 0x80) == 0 ? 0 : 0xff));
                                    _wait = 80;
                                }
                            else
                                if (modRMReg() == 4) {
                                    dx() = _data >> 16;
                                    _data |= dx();
                                    setCF(dx() != 0);
                                    _wait = 118;
                                }
                                else {
                                    dx() = _data >> 16;
                                    if ((_source & 0x8000) != 0)
                                        dx() += _destination;
                                    if ((_destination & 0x8000) != 0)
                                        dx() += _source;
                                    _data |= dx();
                                    setCF(dx() ==
                                        ((ax() & 0x8000) == 0 ? 0 : 0xffff));
                                    _wait = 128;
                                }
                            setZF();
                            setOF(cf());
                            if (_useMemory)
                                _wait += 2;
                            _state = stateEndInstruction;
                            break;
                        case 6:  // DIV
                        case 7:  // IDIV
                            _source = _data;
                            if (_source == 0) {
                                interrupt(0);
                                break;
                            }
                            if (!_wordSize) {
                                _destination = ax();
                                if (modRMReg() == 4) {
                                    div();
                                    if (_data > 0xff) {
                                        interrupt(0);
                                        break;
                                    }
                                    _wait = 80;
                                }
                                else {
                                    _destination = ax();
                                    if ((_destination & 0x8000) != 0)
                                        _destination |= 0xffff0000;
                                    _source = signExtend(_source);
                                    div();
                                    if (_data > 0x7f && _data < 0xffffff80) {
                                        interrupt(0);
                                        break;
                                    }
                                    _wait = 101;
                                }
                                ah() = _remainder;
                                al() = _data;
                            }
                            else {
                                _destination = (dx() << 16) + ax();
                                div();
                                if (modRMReg() == 4) {
                                    if (_data > 0xffff) {
                                        interrupt(0);
                                        break;
                                    }
                                    _wait = 144;
                                }
                                else {
                                if (_data > 0x7fff && _data <  0xffff8000) {
                                    interrupt(0);
                                    break;
                                }
                                _wait = 165;
                                }
                                dx() = _remainder;
                                ax() = _data;
                            }
                            if (_useMemory)
                                _wait += 2;
                            _state = stateEndInstruction;
                            break;
                    }
                    break;

                case stateLoadC: setCF(_wordSize); end(2); break;
                case stateLoadI: setIF(_wordSize); end(2); break;
                case stateLoadD: setDF(_wordSize); end(2); break;

                case stateMisc: readEA(stateMisc2); break;
                case stateMisc2:
                    _savedIP = _data;
                    switch (modRMReg()) {
                        case 0:
                        case 1:
                            _destination = _data;
                            _source = 1;
                            if (modRMReg() == 0) {
                                _data = _destination + _source;
                                setOFAdd();
                            }
                            else {
                                _data = _destination - _source;
                                setOFSub();
                            }
                            doAF();
                            setPZS();
                            writeEA(_data, _useMemory ? 7 : 3);
                            break;
                        case 2:
                            _wait = (_useMemory ? 1 : 0);
                            _state = stateCallCW3;
                            break;
                        case 3:
                            if (_useMemory) {
                                _address += 2;
                                _wait = 1;
                                readEA(stateCallCP3);
                            }
                            else
                                end(0);
                            break;
                        case 4:
                            _wait = (_useMemory ? 8 : 5);
                            _state = stateJmpCW3;
                            break;
                        case 5:
                            if (_useMemory) {
                                _address += 2;
                                _wait = 1;
                                readEA(stateJmpCP3);
                            }
                            else
                                end(0);
                            break;
                        case 6:
                            push(_data, stateEndInstruction);
                            _wait += (_useMemory ? 2 : 1);
                            break;
                        case 7:
                            end(0);
                            break;
                    }
                    break;
            }
        } while (true);
    }
private:
    enum IOType
    {
        ioNone,
        ioRead,
        ioWrite,
        ioInstructionFetch
    };
    enum State
    {
        stateWaitingForBIU,
        stateFetch, stateFetch2,
        stateEndInstruction,
        stateModRM, stateModRM2,
        stateEAOffset,
        stateEARegisters,
        stateEAByte,
        stateEAWord,
        stateEASetSegment,
        stateIO,
        statePush, statePush2,
        statePop,

        stateALU, stateALU2, stateALU3,
        stateALUAccumImm, stateALUAccumImm2,
        statePushSegReg,
        statePopSegReg, statePopSegReg2,
        stateSegOverride,
        stateDAA, stateDAS, stateDA, stateAAA, stateAAS, stateAA,
        stateIncDecRW, statePushRW, statePopRW, statePopRW2,
        stateInvalid,
        stateJCond, stateJCond2,
        stateALURMImm, stateALURMImm2, stateALURMImm3,
        stateTestRMReg, stateTestRMReg2,
        stateXchgRMReg, stateXchgRMReg2,
        stateMovRMReg, stateMovRMReg2,
        stateMovRegRM, stateMovRegRM2,
        stateMovRMWSegReg, stateMovRMWSegReg2,
        stateLEA, stateLEA2,
        stateMovSegRegRMW, stateMovSegRegRMW2,
        statePopMW, statePopMW2,
        stateXchgAxRW,
        stateCBW, stateCWD,
        stateCallCP, stateCallCP2, stateCallCP3, stateCallCP4, stateCallCP5,
        stateWait,
        statePushF, statePopF, statePopF2,
        stateSAHF, stateLAHF,
        stateMovAccumInd, stateMovAccumInd2, stateMovAccumInd3,
        stateMovIndAccum, stateMovIndAccum2,
        stateMovS, stateMovS2, stateRepAction,
        stateCmpS, stateCmpS2, stateCmpS3,
        stateTestAccumImm, stateTestAccumImm2,
        stateStoS,
        stateLodS, stateLodS2,
        stateScaS, stateScaS2,
        stateMovRegImm, stateMovRegImm2,
        stateRet, stateRet2, stateRet3, stateRet4, stateRet5, stateRet6,
        stateLoadFar, stateLoadFar2, stateLoadFar3,
        stateMovRMImm, stateMovRMImm2, stateMovRMImm3,
        stateInt, stateInt3, stateIntAction, stateIntAction2, stateIntAction3,
        stateIntAction4, stateIntAction5, stateIntAction6,
        stateIntO,
        stateIRet, stateIRet2, stateIRet3, stateIRet4,
        stateShift, stateShift2, stateShift3,
        stateAAM, stateAAM2,
        stateAAD, stateAAD2,
        stateSALC,
        stateXlatB, stateXlatB2,
        stateEscape, stateEscape2,
        stateLoop, stateLoop2,
        stateInOut, stateInOut2, stateInOut3,
        stateCallCW, stateCallCW2, stateCallCW3, stateCallCW4,
        stateJmpCW, stateJmpCW2, stateJmpCW3,
        stateJmpCP, stateJmpCP2, stateJmpCP3, stateJmpCP4,
        stateJmpCB, stateJmpCB2,
        stateLock, stateRep, stateHlt, stateCmC,
        stateMath, stateMath2, stateMath3,
        stateLoadC, stateLoadI, stateLoadD,
        stateMisc, stateMisc2
    };
    enum BusState
    {
        t1,
        t2,
        t3,
        tWait,
        t4,
        tIdle
    };
    enum IOByte
    {
        ioSingleByte,
        ioWordFirst,
        ioWordSecond
    };
    void div()
    {
        bool negative = false;
        if (modRMReg() == 7) {
            if ((_destination & 0x80000000) != 0) {
                _destination =
                    static_cast<UInt32>(-static_cast<SInt32>(_destination));
                negative = !negative;
            }
            if ((_source & 0x8000) != 0) {
                _source = (static_cast<UInt32>(-static_cast<SInt32>(_source)))
                    & 0xffff;
                negative = !negative;
            }
        }
        _data = _destination / _source;
        UInt32 product = _data * _source;
        // ISO C++ 2003 does not specify a rounding mode, but the x86 always
        // rounds towards zero.
        if (product > _destination) {
            --_data;
            product -= _source;
        }
        _remainder = _destination - product;
        if (negative) {
            _data = static_cast<UInt32>(-static_cast<SInt32>(_data));
            _remainder = static_cast<UInt32>(-static_cast<SInt32>(_remainder));
        }
    }
    void jumpShort() { setIP(_ip + signExtend(_data)); }
    void interrupt(UInt8 number) { _data = number; _state = stateIntAction; }
    void test(UInt16 destination, UInt16 source)
    {
        _destination = destination;
        _source = source;
        bitwise(_destination & _source);
    }
    int stringIncrement()
    {
        int r = (_wordSize ? 2 : 1);
        return !df() ? r : -r;
    }
    void lodS(State state)
    {
        _address = si();
        si() += stringIncrement();
        _segment = 3;
        initIO(state, ioRead, _wordSize);
    }
    void lodDIS(State state)
    {
        _address = di();
        di() += stringIncrement();
        _segment = 0;
        initIO(state, ioRead, _wordSize);
    }
    void stoS(State state)
    {
        _address = di();
        di() += stringIncrement();
        _segment = 0;
        initIO(state, ioWrite, _wordSize);
    }
    void end(int wait) { _wait = wait; _state = stateEndInstruction; }
    void push(UInt16 data, State state = stateEndInstruction)
    {
        _data = data;
        _afterIO = state;
        _state = statePush;
        _wait += 6;
    }
    void pop(State state) { _afterIO = state; _state = statePop; }
    void loadEA(State state) { _afterEA = state; _state = stateModRM; }
    void readEA(State state)
    {
        _afterIO = state;
        _ioType = ioRead;
        loadEA(stateIO);
    }
    void fetch(State state, bool wordSize)
    {
        initIO(state, ioInstructionFetch, wordSize);
    }
    void writeEA(UInt16 data, int wait)
    {
        _data = data;
        _wait = wait;
        _afterIO = stateEndInstruction;
        _ioType = ioWrite;
        _state = stateIO;
    }
    void setCA() { setCF(true); setAF(true); }
    void clearCA() { setCF(false); setAF(false); }
    void clearCAO() { clearCA(); setOF(false); }
    void setPZS() { setPF(); setZF(); setSF(); }
    void bitwise(UInt16 data) { _data = data; clearCAO(); setPZS(); }
    void doAF() { setAF(((_data ^ _source ^ _destination) & 0x10) != 0); }
    void doCF() { setCF((_data & (!_wordSize ? 0x100 : 0x10000)) != 0); }
    void setCAPZS() { setPZS(); doAF(); doCF(); }
    void setOFAdd()
    {
        UInt16 t = (_data ^ _source) & (_data ^ _destination);
        setOF((t & (!_wordSize ? 0x80 : 0x8000)) != 0);
    }
    void add() { _data = _destination + _source; setCAPZS(); setOFAdd(); }
    void setOFSub()
    {
        UInt16 t = (_destination ^ _source) & (_data ^ _destination);
        setOF((t & (!_wordSize ? 0x80 : 0x8000)) != 0);
    }
    void sub() { _data = _destination - _source; setCAPZS(); setOFSub(); }
    void setOFRotate()
    {
        setOF(((_data ^ _destination) & (!_wordSize ? 0x80 : 0x8000)) != 0);
    }

    void doALUOperation()
    {
        switch (_aluOperation) {
            case 0: add(); break;
            case 1: bitwise(_destination | _source); break;
            case 2: _source += cf() ? 1 : 0; add(); break;
            case 3: _source += cf() ? 1 : 0; sub(); break;
            case 4: test(_destination, _source); break;
            case 5:
            case 7: sub(); break;
            case 6: bitwise(_destination ^ _source); break;
        }
    }
    UInt16 signExtend(UInt8 data) { return data + (data < 0x80 ? 0 : 0xff00); }
    template<class T> class Register
    {
    public:
        void init(String name, T* data)
        {
            _name = name;
            _data = data;
            _read = false;
            _written = false;
        }
        const T& operator=(T value)
        {
            *_data = value;
            _writtenValue = value;
            _written = true;
            return *_data;
        }
        operator T()
        {
            if (!_read && !_written) {
                _read = true;
                _readValue = *_data;
            }
            return *_data;
        }
        const T& operator+=(T value)
        {
            return operator=(operator T() + value);
        }
        const T& operator-=(T value)
        {
            return operator=(operator T() - value);
        }
        const T& operator%=(T value)
        {
            return operator=(operator T() % value);
        }
        const T& operator&=(T value)
        {
            return operator=(operator T() & value);
        }
        const T& operator^=(T value)
        {
            return operator=(operator T() ^ value);
        }
        T operator-() const { return -operator T(); }
        void operator++() { operator=(operator T() + 1); }
        void operator--() { operator=(operator T() - 1); }
        String text()
        {
            String text;
            if (_read) {
                text = String::hexadecimal(_readValue, sizeof(T)==1 ? 2 : 4) +
                    String("<-") + _name + space*2;
                _read = false;
            }
            if (_written) {
                text += _name + String("<-") +
                    String::hexadecimal(_writtenValue, sizeof(T)==1 ? 2 : 4) +
                    space*2;
                _written = false;
            }
            return text;
        }
    protected:
        String _name;
        T* _data;
        bool _read;
        bool _written;
        T _readValue;
        T _writtenValue;
    };
    class FlagsRegister : public Register<UInt16>
    {
    public:
        UInt32 operator=(UInt32 value)
        {
            Register<UInt16>::operator=(value);
            return operator UInt16();
        }
        String text()
        {
            String text;
            if (_read) {
                text = flags(_readValue) + String("<-") + _name + space*2;
                _read = false;
            }
            if (_written) {
                text += _name + String("<-") + flags(_writtenValue) + space*2;
                _written = false;
            }
            return text;
        }
    private:
        String flags(UInt16 value) const
        {
            return String(value & 0x800 ? "O" : "p") +
                String(value & 0x400 ? "D" : "d") +
                String(value & 0x200 ? "I" : "i") +
                String(value & 0x100 ? "T" : "t") +
                String(value & 0x80 ? "S" : "s") +
                String(value & 0x40 ? "Z" : "z") +
                String(value & 0x10 ? "A" : "a") +
                String(value & 4 ? "P" : "p") +
                String(value & 1 ? "C" : "c");
        }
    };

    Register<UInt16>& rw() { return _wordRegisters[_opcode & 7]; }
    Register<UInt16>& ax() { return _wordRegisters[0]; }
    Register<UInt16>& cx() { return _wordRegisters[1]; }
    Register<UInt16>& dx() { return _wordRegisters[2]; }
    Register<UInt16>& bx() { return _wordRegisters[3]; }
    Register<UInt16>& sp() { return _wordRegisters[4]; }
    Register<UInt16>& bp() { return _wordRegisters[5]; }
    Register<UInt16>& si() { return _wordRegisters[6]; }
    Register<UInt16>& di() { return _wordRegisters[7]; }
    Register<UInt8>& rb() { return _byteRegisters[_opcode & 7]; }
    Register<UInt8>& al() { return _byteRegisters[0]; }
    Register<UInt8>& cl() { return _byteRegisters[1]; }
    Register<UInt8>& ah() { return _byteRegisters[4]; }
    Register<UInt16>& cs() { return _segmentRegisters[1]; }
    UInt16& csQuiet() { return _segmentRegisterData[1]; }
    bool cf() { return (_flags & 1) != 0; }
    void setCF(bool cf) { _flags = (_flags & ~1) | (cf ? 1 : 0); }
    bool pf() { return (_flags & 4) != 0; }
    void setPF()
    {
        static UInt8 table[0x100] = {
            4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
            0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
            0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
            4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
            0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
            4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
            4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
            0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
            0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
            4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
            4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
            0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
            4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
            0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
            0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
            4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4};
        _flags = (_flags & ~4) | table[_data & 0xff];
    }
    bool af() { return (_flags & 0x10) != 0; }
    void setAF(bool af) { _flags = (_flags & ~0x10) | (af ? 0x10 : 0); }
    bool zf() { return (_flags & 0x40) != 0; }
    void setZF()
    {
        _flags = (_flags & ~0x40) |
            ((_data & (!_wordSize ? 0xff : 0xffff)) == 0 ? 0x40 : 0);
    }
    bool sf() { return (_flags & 0x80) != 0; }
    void setSF()
    {
        _flags = (_flags & ~0x80) |
            ((_data & (!_wordSize ? 0x80 : 0x8000)) != 0 ? 0x80 : 0);
    }
    bool tf() { return (_flags & 0x100) != 0; }
    void setTF(bool tf) { _flags = (_flags & ~0x100) | (tf ? 0x100 : 0); }
    bool intf() { return (_flags & 0x200) != 0; }
    void setIF(bool intf) { _flags = (_flags & ~0x200) | (intf ? 0x200 : 0); }
    bool df() { return (_flags & 0x400) != 0; }
    void setDF(bool df) { _flags = (_flags & ~0x400) | (df ? 0x400 : 0); }
    bool of() { return (_flags & 0x800) != 0; }
    void setOF(bool of) { _flags = (_flags & ~0x800) | (of ? 0x800 : 0); }
    int modRMReg() { return (_modRM >> 3) & 7; }
    Register<UInt16>& modRMRW() { return _wordRegisters[modRMReg()]; }
    Register<UInt8>& modRMRB() { return _byteRegisters[modRMReg()]; }
    UInt16 getReg()
    {
        if (!_wordSize)
            return static_cast<UInt8>(modRMRB());
        return modRMRW(); 
    }
    UInt16 getAccum()
    {
        if (!_wordSize)
            return static_cast<UInt8>(al());
        return ax(); 
    }
    void setAccum() { if (!_wordSize) al() = _data; else ax() = _data; }
    void setReg(UInt16 value)
    {
        if (!_wordSize)
            modRMRB() = static_cast<UInt8>(value);
        else
            modRMRW() = value;
    }
    void initIO(State nextState, IOType ioType, bool wordSize)
    {
        _state = stateWaitingForBIU;
        _afterIO = nextState;
        _ioRequested = ioType;
        _byte = (!wordSize ? ioSingleByte : ioWordFirst);
        if (ioType == ioInstructionFetch)
            completeInstructionFetch();
    }
    UInt16 getIP() { return _ip; }
    void setIP(UInt16 value)
    {
        _ip = value;
        _abandonFetch = true;
        _prefetched = 0;
        _prefetchAddress = _ip;
    }
    UInt32 physicalAddress(UInt16 segment, UInt16 offset)
    {
        return ((segment << 4) + offset) & 0xfffff;
    }
    UInt8& memory()
    {
        int segment = _segment;
        if (_segmentOverride != -1)
            segment = _segmentOverride;
        return memory(segment, _address);
    }
    UInt8 getInstructionByte()
    {
        UInt8 byte = _prefetchQueue[_prefetchOffset & 3];
        _prefetchOffset = (_prefetchOffset + 1) & 3;
        --_prefetched;
        return byte;
    }
    void completeInstructionFetch()
    {
        if (_ioRequested != ioInstructionFetch)
            return;
        if (_byte == ioSingleByte) {
            if (_prefetched > 0) {
                _ioRequested = ioNone;
                _data = getInstructionByte();
                _state = _afterIO;
                ++_ip;
            }
        }
        else {
            if (_prefetched > 1) {
                _data = getInstructionByte();
                _data |= static_cast<UInt16>(getInstructionByte()) << 8;
                _ioRequested = ioNone;
                _state = _afterIO;
                _ip += 2;
            }
        }
    }

    Register<UInt16> _wordRegisters[8];
    Register<UInt8> _byteRegisters[8];
    Register<UInt16> _segmentRegisters[8];
    UInt16 _registerData[8];      /* AX CX DX BX SP BP SI DI */
    UInt16 _segmentRegisterData[8];
    UInt16 _flagsData;
        //   0: CF = unsigned overflow?
        //   1:  1
        //   2: PF = parity: even number of 1 bits in result?
        //   3:  0
        //   4: AF = unsigned overflow for low nybble
        //   5:  0
        //   6: ZF = zero result?
        //   7: SF = result is negative?
        //   8: TF = interrupt after every instruction?
        //   9: IF = interrupts enabled?
        //  10: DF = SI/DI decrement in string operations
        //  11: OF = signed overflow?
    FlagsRegister _flags;
    UInt16 _ip;
    UInt8 _prefetchQueue[4];
    UInt8 _prefetchOffset;
    UInt8 _prefetched;
    int _segment;
    int _segmentOverride;
    Array<UInt8> _memory;
    UInt16 _prefetchAddress;
    IOType _ioType;
    IOType _ioRequested;
    IOType _ioInProgress;
    BusState _busState;
    IOByte _byte;
    bool _abandonFetch;
    int _wait;
    State _state;
    UInt8 _opcode;
    UInt8 _modRM;
    UInt32 _data;
    UInt32 _source;
    UInt32 _destination;
    UInt32 _remainder;
    UInt16 _address;
    bool _useMemory;
    bool _wordSize;
    int _aluOperation;
    State _afterEA;
    State _afterIO;
    State _afterModRM;
    State _afterRep;
    bool _sourceIsRM;
    UInt16 _savedCS;
    UInt16 _savedIP;
    int _rep;
    bool _useIO;
    bool _halted;
    Handle* _console;
    bool _newInstruction;
    UInt16 _newIP;
    ISA8BitBus* _bus;
};

class ROMData
{
public:
    ROMData(int mask, int address, File file, int offset)
      : _mask(mask), _address(address), _file(file), _offset(offset) { }
    int mask() const { return _mask; }
    int address() const { return _address; }
    File file() const { return _file; }
    int offset() const { return _offset; }
private:
    int _mask;
    int _address;
    File _file;
    int _offset;
};

class ROMConversion : public Conversion
{
public:
    ROMConversion(const Type& type) : _type(type) { }
    TypedValue operator()(const TypedValue& value) const
    {
        List<TypedValue> romMembers = value.value<List<TypedValue>>();
        List<TypedValue>::Iterator m = romMembers.start();
        int address = (*m).value<int>();
        ++m;
        int mask = (*m).value<int>();
        ++m;
        File file = (*m).value<String>();
        ++m;
        int offset = (*m).value<int>();
        return TypedValue(_type, Any(ROMData(mask, address, file, offset)),
            value.span());
    }
private:
    Type _type;
};

class Program : public ProgramBase
{
protected:
    void run()
    {
        if (_arguments.count() < 2) {
            static String syntax1("Syntax: ");
            static String syntax2(" <config file name>\n");
            _console.write(syntax1 + _arguments[0] + syntax2);
            return;
        }

        ConfigFile config;

        AtomicType romDataType("ROM");
        config.addType(romDataType);

        ROMConversion conversion(romDataType);

        List<Type> tupleArguments;
        tupleArguments.add(Type::integer);
        tupleArguments.add(Type::string);
        tupleArguments.add(Type::integer);
        tupleArguments.add(Type::integer);

        config.addConversion(Type::tuple(tupleArguments), romDataType,
            &conversion);

        Type romImageArrayType = Type::array(romDataType);
        config.addType(romImageArrayType);
        config.addOption("roms", romImageArrayType);

        config.load(_arguments[1]);

        ISA8BitBus bus;
        IBMCGA cga;
        Intel8253PIT pit;
        Intel8237DMA dma;
        Intel8255PPI ppi;
        Intel8259PIC pic;
        RAM640Kb ram;
        bus.addComponent(&ram);
        bus.addComponent(&cga);
        bus.addComponent(&pit);
        bus.addComponent(&dma);
        bus.addComponent(&ppi);
        bus.addComponent(&pic);

        List<TypedValue> romDatas = config.get<List<TypedValue> >("roms");
        Array<ROM> roms(romDatas.count());
        int r = 0;
        for (List<TypedValue>::Iterator i = romDatas.start();
            i != romDatas.end(); ++i) {
            ROMData romData = (*i).value<ROMData>();
            ROM* rom = &roms[r];
            rom->initialize(romData);
            bus.addComponent(rom);
        }
            bus.addComponent(&rom);

        Simulator simulator;
        Intel8088 cpu(&_console);
        cpu.setBus(&bus);
        simulator.addComponent(&bus);
        simulator.addComponent(&cpu);

        //File file(config.get<String>("sourceFile"));
        //String contents = file.contents();
        //CharacterSource source(contents, file.path());
        //Space::parse(&source);
        //SourceProgram sourceProgram = parseSourceProgram(&source);
        //sourceProgram.assemble(&simulator);
        simulator.simulate();
    }
};
