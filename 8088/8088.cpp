#include "alfe/string.h"
#include "alfe/array.h"
#include "alfe/file.h"
#include "alfe/stack.h"
#include "alfe/hash_table.h"
#include "alfe/main.h"
#include "alfe/space.h"
#include "alfe/config_file.h"
#include "alfe/type.h"
#include "alfe/rational.h"
#include "alfe/pipes.h"
#include "alfe/sdl2.h"

#include <stdlib.h>
#include <limits.h>

typedef UInt8 BGRI;

//class SourceProgram
//{
//};

template<class T> class SimulatorTemplate;
typedef SimulatorTemplate<void> Simulator;

template<class T> class Intel8088Template;
typedef Intel8088Template<void> Intel8088;

template<class T> class ComponentTemplate;
typedef ComponentTemplate<void> Component;

template<class T> class Intel8237DMATemplate;
typedef Intel8237DMATemplate<void> Intel8237DMA;

template<class T> class Intel8253PITTemplate;
typedef Intel8253PITTemplate<void> Intel8253PIT;

template<class T> class RAM640KbTemplate;
typedef RAM640KbTemplate<void> RAM640Kb;

template<class T> class ROMTemplate;
typedef ROMTemplate<void> ROM;

template<class T> class IBMCGATemplate;
typedef IBMCGATemplate<void> IBMCGA;

template<class T> class ComponentTemplate
{
public:
    ComponentTemplate() : _simulator(0) { }
    void setSimulator(Simulator* simulator) { _simulator = simulator; site(); }
    virtual void site() { }
    virtual void simulateCycle() { }
    virtual String save() const { return String(); }
    virtual Type type() const { return Type(); }
    virtual String name() const { return String(); }
    virtual void load(const TypedValue& value) { }
    virtual TypedValue initial() const
    {
        // Default initial value is the result of converting the empty
        // structured type to the component type.
        return TypedValue(StructuredType(String(),
            List<StructuredType::Member>()),
            Value<HashTable<String, TypedValue>>()).convertTo(type());
    }
    virtual Rational<int> cyclesPerSecond() const
    {
        Rational<int> h = hDotsPerCycle();
        if (h == 0)
            return 0;
        return 157500000/(11*h);
    }
    virtual Rational<int> hDotsPerCycle() const { return 0; }
    void setTicksPerCycle(int ticksPerCycle)
    {
        _ticksPerCycle = ticksPerCycle;
        _tick = 0;
    }
    void simulateTicks(int ticks)
    {
        _tick += ticks;
        if (_ticksPerCycle != 0 && _tick >= _ticksPerCycle) {
            simulateCycle();
            _tick -= _ticksPerCycle;
        }
    }
protected:
    int _tick;

    SimulatorTemplate<T>* _simulator;
private:
    int _ticksPerCycle;
};

template<class T> class ISA8BitBusTemplate;
typedef ISA8BitBusTemplate<void> ISA8BitBus;

template<class T> class ISA8BitComponentTemplate : public ComponentTemplate<T>
{
public:
    // Address bit 31 = write
    // Address bit 30 = IO
    virtual void setAddress(UInt32 address) = 0;
    virtual void write(UInt8 data) { };
    virtual bool wait() { return false; }
    void setBus(ISA8BitBus* bus) { _bus = bus; }
    virtual UInt8 memory(UInt32 address) { return 0xff; }
    bool active() const { return _active; }
    virtual void read() { }
    void requestInterrupt(UInt8 data)
    {
        _bus->_interruptnum = data & 7;
        _bus->_interrupt = true;
    }
protected:
    void set(UInt8 data) { _bus->_data = data; }
    ISA8BitBusTemplate<T>* _bus;
    bool _active;
};

typedef ISA8BitComponentTemplate<void> ISA8BitComponent;

template<class T> class ISA8BitBusTemplate : public ComponentTemplate<T>
{
public:
    void addComponent(ISA8BitComponent* component)
    {
        _components.add(component);
        this->_simulator->addComponent(component);
        component->setBus(this);
    }
    void setAddress(UInt32 address)
    {
        for (auto i = _components.begin(); i != _components.end(); ++i)
            (*i)->setAddress(address);
    }
    void write(UInt8 data)
    {
        for (auto i = _components.begin(); i != _components.end(); ++i)
            if ((*i)->active())
                (*i)->write(data);
    }
    UInt8 read() const
    {
        for (auto i = _components.begin(); i != _components.end(); ++i)
            if ((*i)->active())
                (*i)->read();
        return _data;
    }
    UInt8 memory(UInt32 address)
    {
        UInt8 data = 0xff;
        for (auto i = _components.begin(); i != _components.end(); ++i)
            data &= (*i)->memory(address);
        return data;
    }
    String save() const { return String("bus: ") + hex(_data, 2) + "\n"; }
    virtual Type type() const { return Type::integer; }
    virtual String name() const { return String("bus"); }
    virtual void load(const TypedValue& value) { _data = value.value<int>(); }
    virtual TypedValue initial() const { return 0xff; }

private:
    UInt8 _data;

    List<ISA8BitComponent*> _components;

    template<class U> friend class ISA8BitComponentTemplate;
};

class NMISwitch : public ISA8BitComponent
{
public:
    NMISwitch() : _nmiOn(false) { }
    void setAddress(UInt32 address)
    {
        _active = (address & 0xc00003e0) == 0xc00000a0;
    }
    void write(UInt8 data) { _nmiOn = ((data & 0x80) != 0); }
    String save() const
    {
        return String("nmiSwitch: { on: ") + String::Boolean(_nmiOn) +
            ", active: " + String::Boolean(_active) + " }\n";
    }
    Type type() const
    {
        List<StructuredType::Member> members;
        members.add(StructuredType::Member("on", false));
        members.add(StructuredType::Member("active", false));
        return StructuredType("NMISwitch", members);
    }
    void load(const TypedValue& value)
    {
        auto members = value.value<Value<HashTable<String, TypedValue>>>();
        _nmiOn = (*members)["on"].value<bool>();
        _active = (*members)["active"].value<bool>();
    }
    String name() const { return "nmiSwitch"; }
    bool nmiOn() const { return _nmiOn; }
private:
    bool _nmiOn;
};

#include "8259.h"

#include "8237.h"

#include "8255.h"

#include "8253.h"

#include "mc6845.h"

#include "dram.h"

template<class T> class RAM640KbTemplate : public ISA8BitComponent
{
public:
    void site()
    {
        _nmiSwitch = this->_simulator->getNMISwitch();
        _ppi = this->_simulator->getPPI();
        _cpu = this->_simulator->getCPU();

        ConfigFile* config = this->_simulator->config();

        // _rowBits is 7 for 4116 RAM chips
        //             8 for 4164
        //             9 for 41256
        // We use 9 here because programs written for _rowBits == N will work
        // for _rowBits < N but not necessarily _rowBits > N.
        config->addDefaultOption("ramRowBits", Type::integer, 9);

        // 640KB should be enough for anyone.
        config->addDefaultOption("ramBytes", Type::integer, 0xa0000);

        config->addDefaultOption("decayTime", Type::integer, 0);
    }
    void initialize()
    {
        ConfigFile* config = this->_simulator->config();
        int rowBits = config->get<int>("ramRowBits");
        int bytes = config->get<int>("ramBytes");
        int decayTime = config->get<int>("decayTime");
        if (decayTime == 0) {
            // DRAM decay time in cycles.
            // This is the fastest that DRAM could decay and real hardware
            // would still work.
            decayTime = (18*4) << rowBits;
            // 2ms for 4116 and 2118
            // 4ms for 4164
            // 8ms for 41256
            //   decayTime =  (13125 << rowBits) / 176;
        }
        _dram.initialize(bytes, rowBits, decayTime, 0);
    }
    Rational<int> hDotsPerCycle() const { return 3; }
    void simulateCycle() { _dram.simulateCycle(); }
    void setAddress(UInt32 address)
    {
        _address = address & 0x400fffff;
        _active = (_address < 0xa0000);
    }
    void read()
    {
        if (_dram.decayed(_address)) {
            // RAM has decayed! On a real machine this would not always signal
            // an NMI but we'll make the NMI happen every time to make DRAM
            // decay problems easier to find.

            // TODO: In the config file we might want to have an option for
            // "realistic mode" to prevent this from being used to detect the
            // emulator.
            if (_nmiSwitch->nmiOn() && (_ppi->portB() & 0x10) != 0)
                _cpu->nmi();
        }
        set(_dram.read(_address));
    }
    void write(UInt8 data) { _dram.write(_address, data); }
    UInt8 memory(UInt32 address)
    {
        if (address < 0xa0000)
            return _dram.memory(address);
        return 0xff;
    }
    Type type() const
    {
        List<StructuredType::Member> members;
        members.add(StructuredType::Member("dram", _dram.initial()));
        members.add(StructuredType::Member("active", false));
        members.add(StructuredType::Member("tick", 0));
        members.add(StructuredType::Member("address", 0));
        return StructuredType("RAM", members);
    }
    void load(const TypedValue& value)
    {
        auto members = value.value<Value<HashTable<String, TypedValue>>>();
        _dram.load((*members)["dram"]);
        _active = (*members)["active"].value<bool>();
        _tick = (*members)["tick"].value<int>();
        _address = (*members)["address"].value<int>();
    }
    String save() const
    {
        return String("ram: { ") + _dram.save() + ",\n  active: " +
            String::Boolean(_active) + ", tick: " + _tick + ", address: " +
            hex(_address, 5) + "}\n";
    }
private:
    int _address;
    DRAM _dram;
    NMISwitch* _nmiSwitch;
    Intel8255PPI* _ppi;
    Intel8088Template<T>* _cpu;
};

class DMAPageRegisters : public ISA8BitComponent
{
public:
    DMAPageRegisters() { for (int i = 0; i < 4; ++i) _dmaPages[i] = 0; }
    void setAddress(UInt32 address)
    {
        _address = address & 3;
        _active = (address & 0xc00003e0) == 0xc0000080;
    }
    void write(UInt8 data) { _dmaPages[_address] = data & 0x0f; }
    String save() const
    {
        String s = "dmaPages: { data: {";
        bool needComma = false;
        for (int i = 0; i < 4; ++i) {
            if (needComma)
                s += ", ";
            needComma = true;
            s += hex(_dmaPages[i], 1);
        }
        return s + "}, active: " + String::Boolean(_active) + ", address: " +
            _address + " }\n";
    }
    Type type() const
    {
        List<StructuredType::Member> members;
        members.add(StructuredType::Member("data",
            TypedValue(Type::array(Type::integer), List<TypedValue>())));
        members.add(StructuredType::Member("active", false));
        members.add(StructuredType::Member("address", 0));
        return StructuredType("RAM", members);
    }
    void load(const TypedValue& value)
    {
        auto members = value.value<Value<HashTable<String, TypedValue>>>();
        auto dmaPages = (*members)["data"].value<List<TypedValue>>();
        int j = 0;
        for (auto i = dmaPages.begin(); i != dmaPages.end(); ++i) {
            _dmaPages[j] = (*i).value<int>();
            ++j;
            if (j == 4)
                break;
        }
        for (;j < 4; ++j)
            _dmaPages[j] = 0;
        _active = (*members)["active"].value<bool>();
        _address = (*members)["address"].value<int>();
    }
    String name() const { return "dmaPages"; }
private:
    int _address;
    int _dmaPages[4];
};

class ROMData
{
public:
    ROMData(int mask, int start, String file, int offset)
      : _mask(mask), _start(start), _file(file), _offset(offset) { }
    int mask() const { return _mask; }
    int start() const { return _start; }
    String file() const { return _file; }
    int offset() const { return _offset; }
private:
    int _mask;
    int _start;
    String _file;
    int _offset;
};

template<class T> class ROMTemplate : public ISA8BitComponentTemplate<T>
{
public:
    void initialize(const ROMData& romData)
    {
        _mask = romData.mask() | 0xc0000000;
        _start = romData.start();
        String data = File(romData.file(),
            this->_simulator->config()->file().parent(), true).contents();
        int length = ((_start | ~_mask) & 0xfffff) + 1 - _start;
        _data.allocate(length);
        int offset = romData.offset();
        for (int i = 0; i < length; ++i)
            _data[i] = data[i + offset];
    }
    void setAddress(UInt32 address)
    {
        _address = address & 0xfffff & ~_mask;
        this->_active = ((address & _mask) == _start);
    }
    void read() { this->set(_data[_address & ~_mask]); }
    UInt8 memory(UInt32 address)
    {
        if ((address & _mask) == _start)
            return _data[address & ~_mask];
        return 0xff;
    }
    String save() const
    {
        return String("rom: { active: ") + String::Boolean(this->_active) +
            ", address: " + hex(_address, 5) + "}\n";
    }
    Type type() const
    {
        List<StructuredType::Member> members;
        members.add(StructuredType::Member("active", false));
        members.add(StructuredType::Member("address", 0));
        return StructuredType("ROM", members);
    }
    void load(const TypedValue& value)
    {
        auto members = value.value<Value<HashTable<String, TypedValue>>>();
        this->_active = (*members)["active"].value<bool>();
        _address = (*members)["address"].value<int>();
    }
private:
    int _mask;
    int _start;
    int _address;
    Array<UInt8> _data;
};

template<class T> class DisassemblerTemplate
{
public:
    void setBus(ISA8BitBus* bus) { _bus = bus; }
    void setCPU(Intel8088* cpu) { _cpu = cpu; }
    String disassemble(UInt16 address)
    {
        _bytes = "";
        String i = disassembleInstruction(address);
        return _bytes.alignLeft(10) + " " + i;
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
            return s + ea() + ", " +
                ((_opcode & 2) == 0 ? String("1") : byteRegs(1));
        }
        if ((_opcode & 0xf8) == 0xd8) {
            _modRM = getByte();
            _wordSize = true;
            return String("ESC ") + (_opcode & 7) + ", " + r() + ", " + ea();
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
        UInt8 v = _bus->memory(_cpu->codeAddress(_address));
        ++_address;
        _bytes += hex(v, 2, false);
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
    String iw() { return hex(getWord(), 4, false); }
    String ib() { return hex(getByte(), 2, false); }
    String sb()
    {
        UInt8 byte = getByte();
        if ((byte & 0x80) == 0)
            return String("+") + hex(byte, 2, false);
        return String("-") + hex((-byte) & 0x7f, 2, false);
    }
    String imm() { return !_wordSize ? ib() : iw(); }
    String accum() { return !_wordSize ? "AL" : "AX"; }
    String segreg(int r)
    {
        static String sr[8] = {"ES", "CS", "SS", "DS", "??", "??", "??", "??"};
        return sr[r];
    }
    String cb()
    {
        SInt8 byte = static_cast<SInt8>(getByte());
        return hex(_address + byte, 4, false);
    }
    String cw() { return hex(_address + getWord(), 4, false); }
    String cp()
    {
        UInt16 offset = getWord();
        return hex(getWord(), 4, false) + ":" + hex(offset, 4, false);
    }

    ISA8BitBus* _bus;
    Intel8088Template<T>* _cpu;
    UInt16 _address;
    UInt8 _opcode;
    UInt8 _modRM;
    bool _wordSize;
    bool _doubleWord;
    String _bytes;
};

typedef DisassemblerTemplate<void> Disassembler;

template<class T> class Intel8088Template : public ComponentTemplate<T>
{
public:
    Rational<int> hDotsPerCycle() const { return 3; }
    Intel8088Template()
    {
        static String b[8] = {"AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH"};
        static String w[8] = {"AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI"};
        static String s[8] = {"ES", "CS", "SS", "DS", "??", "??", "??", "??"};
        for (int i = 0; i < 8; ++i) {
            _wordRegisters[i].init(w[i], &_registerData[i]);
            _byteRegisters[i].init(b[i], reinterpret_cast<UInt8*>(
                &_registerData[i & 3]) + (i >= 4 ? 1 : 0));
            _segmentRegisters[i].init(s[i], &_segmentRegisterData[i]);
        }
        _flags.init("F", &_flagsData);

        _segmentRegisterData[7] = 0x0000;  // For IO accesses

        List<EnumerationType::Value> stateValues;
        for (int i = stateWaitingForBIU; i <= stateMisc2; ++i) {
            State s = static_cast<State>(i);
            stateValues.add(EnumerationType::Value(stringForState(s), s));
        }
        _stateType = EnumerationType("State", stateValues);

        List<EnumerationType::Value> ioTypeValues;
        for (int i = ioNone; i <= ioInstructionFetch; ++i) {
            IOType t = static_cast<IOType>(i);
            ioTypeValues.add(EnumerationType::Value(stringForIOType(t), t));
        }
        _ioTypeType = EnumerationType("IOType", ioTypeValues);

        List<EnumerationType::Value> ioByteValues;
        for (int i = ioSingleByte; i <= ioWordSecond; ++i) {
            IOByte b = static_cast<IOByte>(i);
            ioByteValues.add(EnumerationType::Value(stringForIOByte(b), b));
        }
        _ioByteType = EnumerationType("IOByte", ioByteValues);

        List<EnumerationType::Value> busStateValues;
        for (int i = t1; i <= tIdle; ++i) {
            BusState s = static_cast<BusState>(i);
            busStateValues.add(EnumerationType::Value(stringForBusState(s), s));
        }
        _busStateType = EnumerationType("BusState", busStateValues);

        _disassembler.setCPU(this);
    }
    void setStopAtCycle(int stopAtCycle) { _stopAtCycle = stopAtCycle; }
    void site()
    {
        _bus = this->_simulator->getBus();
        _disassembler.setBus(_bus);
        _pic = this->_simulator->getPIC();
        _dma = this->_simulator->getDMA();
    }
    void nmi() { _nmiRequested = true; }
    UInt32 codeAddress(UInt16 offset) { return physicalAddress(1, offset); }
    void simulateCycle()
    {
        simulateCycleAction();
        if (_cycle >= 32000000) {
            String line = String(decimal(_cycle)).alignRight(5) + " ";
            switch (_busState) {
                case t1:
                    line += "T1 " + hex(_busAddress, 5, false) + " ";
                    break;
                case t2:
                    line += "T2 ";
                    if (_ioInProgress == ioWrite)
                        line += "M<-" + hex(_busData, 2, false) + " ";
                    else
                        line += "      ";
                    break;
                case t3: line += "T3       "; break;
                case tWait: line += "Tw       "; break;
                case t4:
                    line += "T4 ";
                    if (_ioInProgress == ioWrite)
                        line += "      ";
                    else
                        if (_abandonFetch)
                            line += "----- ";
                        else
                            line += "M->" + hex(_busData, 2, false) + " ";
                    break;
                case tIdle: line += "         "; break;
                case tDMA:
                    line += "D" + _dma->getText();
            }
            if (_newInstruction) {
                line += hex(csQuiet(), 4, false) + ":" + hex(_newIP, 4, false) +
                    " " + _disassembler.disassemble(_newIP);
            }
            line = line.alignLeft(50);
            for (int i = 0; i < 8; ++i) {
                line += _byteRegisters[i].text();
                line += _wordRegisters[i].text();
                line += _segmentRegisters[i].text();
            }
            line += _flags.text();
            //if(_newInstruction)
                console.write(line + "\n");
            _newInstruction = false;
        }
        ++_cycle;
        if (_halted /*|| _cycle == _stopAtCycle*/) {
            console.write("Stopped at cycle " + String(decimal(_cycle)) + "\n");
            this->_simulator->halt();
        }
    }
    void simulateCycleAction()
    {
        bool busDone;
        do {
            busDone = true;
            switch (_busState) {
                case t1:
                    if (_dma->dmaRequested()) {
                        _busState = tDMA;
                        break;
                    }
                    if (_ioInProgress == ioInstructionFetch)
                        _busAddress = physicalAddress(1, _prefetchAddress);
                    else {
                        int segment = _segment;
                        if (_segmentOverride != -1)
                            segment = _segmentOverride;
                        _busAddress = physicalAddress(segment, _address);
                        if (_usePortSpace)
                            _busAddress |= 0x40000000;
                        if (_ioInProgress == ioWrite)
                            _busAddress |= 0x80000000;
                    }
                    _bus->setAddress(_busAddress);
                    _busState = t2;
                    break;
                case t2:
                    if (_ioInProgress == ioWrite) {
                        _ioRequested = ioNone;
                        switch (_byte) {
                            case ioSingleByte:
                                _busData = _data;
                                _state = _afterIO;
                                break;
                            case ioWordFirst:
                                _busData = _data;
                                _ioInProgress = ioWrite;
                                _byte = ioWordSecond;
                                ++_address;
                                break;
                            case ioWordSecond:
                                _busData = _data >> 8;
                                _state = _afterIO;
                                _byte = ioSingleByte;
                                break;
                        }
                        _bus->write(_busData);
                    }
                    _busState = t3;
                    break;
                case t3:
                    _busState = tWait;
                    busDone = false;
                    break;
                case tWait:
                    _busState = t4;
                    if (_ioInProgress == ioWrite)
                        break;
                    _busData = _bus->read();
                    if (_ioInProgress == ioRead) {
                        _ioRequested = ioNone;
                        switch (_byte) {
                            case ioSingleByte:
                                _data = _busData;
                                _state = _afterIO;
                                break;
                            case ioWordFirst:
                                _data = _busData;
                                _ioInProgress = ioRead;
                                _byte = ioWordSecond;
                                ++_address;
                                break;
                            case ioWordSecond:
                                _data |= static_cast<UInt16>(_busData) << 8;
                                _state = _afterIO;
                                _byte = ioSingleByte;
                                break;
                        }
                        break;
                    }
                    if (_abandonFetch)
                        break;
                    _prefetchQueue[(_prefetchOffset + _prefetched) & 3] =
                        _busData;
                    ++_prefetched;
                    ++_prefetchAddress;
                    completeInstructionFetch();
                    break;
                case t4:
                    _busState = tIdle;
                    busDone = false;
                    break;
                case tIdle:
                    if (_byte == ioWordSecond)
                        _busState = t1;
                    else
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
                case tDMA:
                    if (!_dma->dmaRequested())
                        _busState = t1;
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

                case stateBegin: fetch(stateDecodeOpcode, false); break;
                case stateDecodeOpcode:
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
stateJCond,        stateJCond,        stateJCond,        stateJCond,
stateJCond,        stateJCond,        stateJCond,        stateJCond,
stateJCond,        stateJCond,        stateJCond,        stateJCond,
stateJCond,        stateJCond,        stateJCond,        stateJCond,
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
stateInt3,         stateInt,          stateIntO,         stateIRet,
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
                    _rep = 0;
                    _usePortSpace = false;
                    _newInstruction = true;
                    _newIP = _ip;
                    _afterInt = stateBegin;
                    _state = stateCheckInt;
                    break;

                case stateBeginModRM: fetch(stateDecodeModRM, false); break;
                case stateDecodeModRM:
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

                case stateEAIO:
                    if (_useMemory)
                        initIO(_afterEAIO, _ioType, _wordSize);
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
                        _state = _afterEAIO;
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
                    _state = stateBegin;
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
                        writeEA(_data, _wait);
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
                case statePushF: push((_flags & 0x0fd7) | 0xf000); break;
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
                        _afterInt = _afterRep;
                        if (cx() == 0 || (zf() == (_rep == 1)))
                            _afterInt = stateEndInstruction;
                        _state = stateCheckInt;
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
                case stateMovRMImm2: fetch(stateMovRMImm3, _wordSize); break;
                case stateMovRMImm3: writeEA(_data, _useMemory ? 6 : 4); break;

                case stateCheckInt:
                    _state = _afterInt;
                    if (_nmiRequested) {
                        _data = 2;
                        _state = stateIntAction;
                        _nmiRequested = false;
                    }
                    if (intf() && _pic->interruptRequest()) {
                        _state = stateHardwareInt;
                        _pic->interruptAcknowledge();
                        _wait = 1;
                    }
                    break;
                case stateHardwareInt:
                    _pic->interruptAcknowledge();
                    _wait = 1;
                    _state = stateHardwareInt2;
                    break;
                case stateHardwareInt2:
                    _data = _pic->_interruptnum;
                    _state = stateIntAction;
                    break;
                case stateInt3:
                    interrupt(3);
                    _wait = 1;
                    break;
                case stateInt:
                    fetch(stateIntAction, false);
                    _afterInt = stateEndInstruction;
                    break;
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
                    _address++;
                    initIO(stateIntAction6, ioRead, true);
                    break;
                case stateIntAction6:
                    cs() = _data;
                    setIP(_savedIP);
                    _wait = 13;
                    _state = _afterInt;
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
                    end(20);
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
                    _usePortSpace = true;
                    _ioType = ((_opcode & 2) == 0 ? ioRead : ioWrite);
                    _segment = 7;
                    _address = _data;
                    if (_ioType == ioWrite)
                        _data = getAccum();
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
                    _state = stateBegin;
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

    String save() const
    {
        String s("cpu: {\n");
        s += String("  ip: ") + hex(_ip, 4) + ",\n";
        s += String("  ax: ") + hex(_registerData[0], 4) + ",\n";
        s += String("  cx: ") + hex(_registerData[1], 4) + ",\n";
        s += String("  dx: ") + hex(_registerData[2], 4) + ",\n";
        s += String("  bx: ") + hex(_registerData[3], 4) + ",\n";
        s += String("  sp: ") + hex(_registerData[4], 4) + ",\n";
        s += String("  bp: ") + hex(_registerData[5], 4) + ",\n";
        s += String("  si: ") + hex(_registerData[6], 4) + ",\n";
        s += String("  di: ") + hex(_registerData[7], 4) + ",\n";
        s += String("  es: ") + hex(_segmentRegisterData[0], 4) + ",\n";
        s += String("  cs: ") + hex(_segmentRegisterData[1], 4) + ",\n";
        s += String("  ss: ") + hex(_segmentRegisterData[2], 4) + ",\n";
        s += String("  ds: ") + hex(_segmentRegisterData[3], 4) + ",\n";
        s += String("  flags: 0x") + hex(_flagsData, 4) + ",\n";
        s += String("  prefetch: {");
        bool needComma = false;
        for (int i = 0; i < _prefetched; ++i) {
            if (needComma)
                s += ", ";
            needComma = true;
            s += hex(_prefetchQueue[(i + _prefetchOffset) & 3], 2);
        }
        s += "},\n";
        s += String("  segment: ") + _segment + ",\n";
        s += String("  segmentOverride: ") + _segmentOverride + ",\n";
        s += String("  prefetchAddress: ") + hex(_prefetchAddress, 4) + ",\n";
        s += String("  ioType: ") + stringForIOType(_ioType) + ",\n";
        s += String("  ioRequested: ") + stringForIOType(_ioRequested) +
            ",\n";
        s += String("  ioInProgress: ") + stringForIOType(_ioInProgress) +
            ",\n";
        s += String("  busState: ") + stringForBusState(_busState) + ",\n";
        s += String("  byte: ") + stringForIOByte(_byte) + ",\n";
        s += String("  abandonFetch: ") + String::Boolean(_abandonFetch) +
            ",\n";
        s += String("  wait: ") + _wait + ",\n";
        s += String("  state: ") + stringForState(_state) + ",\n";
        s += String("  opcode: ") + hex(_opcode, 2) + ",\n";
        s += String("  modRM: ") + hex(_modRM, 2) + ",\n";
        s += String("  data: ") + hex(_data, 8) + ",\n";
        s += String("  source: ") + hex(_source, 8) + ",\n";
        s += String("  destination: ") + hex(_destination, 8) + ",\n";
        s += String("  remainder: ") + hex(_remainder, 8) + ",\n";
        s += String("  address: 0x") + hex(_address, 4) + ",\n";
        s += String("  useMemory: ") + String::Boolean(_useMemory) + ",\n";
        s += String("  wordSize: ") + String::Boolean(_wordSize) + ",\n";
        s += String("  aluOperation: ") + _aluOperation + ",\n";
        s += String("  afterEA: ") + stringForState(_afterEA) + ",\n";
        s += String("  afterIO: ") + stringForState(_afterIO) + ",\n";
        s += String("  afterEAIO: ") + stringForState(_afterEAIO) + ",\n";
        s += String("  afterRep: ") + stringForState(_afterRep) + ",\n";
        s += String("  afterInt: ") + stringForState(_afterInt) + ",\n";
        s += String("  sourceIsRM: ") + String::Boolean(_sourceIsRM) + ",\n";
        s += String("  savedCS: ") + hex(_savedCS, 4) + ",\n";
        s += String("  savedIP: ") + hex(_savedIP, 4) + ",\n";
        s += String("  rep: ") + _rep + ",\n";
        s += String("  usePortSpace: ") + String::Boolean(_usePortSpace) +
            ",\n";
        s += String("  halted: ") + String::Boolean(_halted) + ",\n";
        s += String("  newInstruction: ") + String::Boolean(_newInstruction) +
            ",\n";
        s += String("  newIP: ") + hex(_newIP, 4) + ",\n";
        s += String("  nmiRequested: ") + String::Boolean(_nmiRequested) +
            ",\n";
        s += String("  cycle: ") + _cycle + ",\n";
        s += String("  tick: ") + this->_tick;
        return s + "}\n";
    }
    Type type() const
    {
        typedef StructuredType::Member M;
        List<M> members;
        members.add(M("ip", 0));
        members.add(M("ax", 0));  // ?
        members.add(M("cx", 0));  // ?
        members.add(M("dx", 0));  // ?
        members.add(M("bx", 0));  // ?
        members.add(M("sp", 0));  // ?
        members.add(M("bp", 0));  // ?
        members.add(M("si", 0));  // ?
        members.add(M("di", 0));  // ?
        members.add(M("es", 0));  // ?
        members.add(M("cs", 0xffff));
        members.add(M("ss", 0));  // ?
        members.add(M("ds", 0));  // ?
        members.add(M("flags", 2));  // ?
        members.add(M("prefetch",
            TypedValue(Type::array(Type::integer), List<TypedValue>())));
        members.add(M("segment", 0));
        members.add(M("segmentOverride", -1));
        members.add(M("prefetchAddress", 0));
        members.add(M("ioType", TypedValue(_ioTypeType, ioNone)));
        members.add(M("ioRequested", TypedValue(_ioTypeType, ioNone)));
        members.add(M("ioInProgress",
            TypedValue(_ioTypeType, ioInstructionFetch)));
        members.add(M("busState", TypedValue(_busStateType, t1)));
        members.add(M("byte", TypedValue(_ioByteType, ioSingleByte)));
        members.add(M("abandonFetch", false));
        members.add(M("wait", 0));
        members.add(M("state", TypedValue(_stateType, stateBegin)));
        members.add(M("opcode", 0));
        members.add(M("modRM", 0));
        members.add(M("data", 0));
        members.add(M("source", 0));
        members.add(M("destination", 0));
        members.add(M("remainder", 0));
        members.add(M("address", 0));
        members.add(M("useMemory", false));
        members.add(M("wordSize", false));
        members.add(M("aluOperation", 0));
        members.add(M("afterEA", TypedValue(_stateType, stateWaitingForBIU)));
        members.add(M("afterIO", TypedValue(_stateType, stateWaitingForBIU)));
        members.add(M("afterEAIO",
            TypedValue(_stateType, stateWaitingForBIU)));
        members.add(M("afterRep", TypedValue(_stateType, stateWaitingForBIU)));
        members.add(M("afterInt", TypedValue(_stateType, stateWaitingForBIU)));
        members.add(M("sourceIsRM", false));
        members.add(M("savedCS", 0));
        members.add(M("savedIP", 0));
        members.add(M("rep", 0));
        members.add(M("usePortSpace", false));
        members.add(M("halted", false));
        members.add(M("newInstruction", true));
        members.add(M("newIP", 0));
        members.add(M("nmiRequested", false));
        members.add(M("cycle", 0));
        members.add(M("tick", 0));
        return StructuredType("CPU", members);
    }
    void load(const TypedValue& value)
    {
        auto members = value.value<Value<HashTable<String, TypedValue>>>();
        _ip = (*members)["ip"].value<int>();
        _registerData[0] = (*members)["ax"].value<int>();
        _registerData[1] = (*members)["cx"].value<int>();
        _registerData[2] = (*members)["dx"].value<int>();
        _registerData[3] = (*members)["bx"].value<int>();
        _registerData[4] = (*members)["sp"].value<int>();
        _registerData[5] = (*members)["bp"].value<int>();
        _registerData[6] = (*members)["si"].value<int>();
        _registerData[7] = (*members)["di"].value<int>();
        _segmentRegisterData[0] = (*members)["es"].value<int>();
        _segmentRegisterData[1] = (*members)["cs"].value<int>();
        _segmentRegisterData[2] = (*members)["ss"].value<int>();
        _segmentRegisterData[3] = (*members)["ds"].value<int>();
        _flagsData = (*members)["flags"].value<int>();
        auto prefetch = (*members)["prefetch"].value<List<TypedValue>>();
        _prefetched = 0;
        _prefetchOffset = 0;
        for (auto i = prefetch.begin(); i != prefetch.end(); ++i) {
            _prefetchQueue[_prefetched] = (*i).value<int>();
            ++_prefetched;
        }
        _segment = (*members)["segment"].value<int>();
        _segmentOverride = (*members)["segmentOverride"].value<int>();
        _prefetchAddress = (*members)["prefetchAddress"].value<int>();
        _ioType = (*members)["ioType"].value<IOType>();
        _ioRequested = (*members)["ioRequested"].value<IOType>();
        _ioInProgress = (*members)["ioInProgress"].value<IOType>();
        _busState = (*members)["busState"].value<BusState>();
        _byte = (*members)["byte"].value<IOByte>();
        _abandonFetch = (*members)["abandonFetch"].value<bool>();
        _wait = (*members)["wait"].value<int>();
        _state = (*members)["state"].value<State>();
        _opcode = (*members)["opcode"].value<int>();
        _modRM = (*members)["modRM"].value<int>();
        _data = (*members)["data"].value<int>();
        _source = (*members)["source"].value<int>();
        _destination = (*members)["destination"].value<int>();
        _remainder = (*members)["remainder"].value<int>();
        _address = (*members)["address"].value<int>();
        _useMemory = (*members)["useMemory"].value<bool>();
        _wordSize = (*members)["wordSize"].value<bool>();
        _aluOperation = (*members)["aluOperation"].value<int>();
        _afterEA = (*members)["afterEA"].value<State>();
        _afterIO = (*members)["afterIO"].value<State>();
        _afterEAIO = (*members)["afterEAIO"].value<State>();
        _afterRep = (*members)["afterRep"].value<State>();
        _afterInt = (*members)["afterInt"].value<State>();
        _sourceIsRM = (*members)["sourceIsRM"].value<bool>();
        _savedCS = (*members)["savedCS"].value<int>();
        _savedIP = (*members)["savedIP"].value<int>();
        _rep = (*members)["rep"].value<int>();
        _usePortSpace = (*members)["usePortSpace"].value<bool>();
        _halted = (*members)["halted"].value<bool>();
        _newInstruction = (*members)["newInstruction"].value<bool>();
        _newIP = (*members)["newIP"].value<int>();
        _nmiRequested = (*members)["nmiRequested"].value<bool>();
        _cycle = (*members)["cycle"].value<int>();
        this->_tick = (*members)["tick"].value<int>();
    }
    String name() const { return "cpu"; }

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
        stateBegin, stateDecodeOpcode,
        stateEndInstruction,
        stateBeginModRM, stateDecodeModRM,
        stateEAOffset,
        stateEARegisters,
        stateEAByte,
        stateEAWord,
        stateEASetSegment,
        stateEAIO,
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
        stateCheckInt, stateHardwareInt, stateHardwareInt2,
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
        tIdle,
        tDMA
    };
    enum IOByte
    {
        ioSingleByte,
        ioWordFirst,
        ioWordSecond
    };
    static String stringForState(State state)
    {
        switch (state) {
            case stateWaitingForBIU:  return "waitingForBIU";
            case stateBegin:          return "stateBegin";
            case stateDecodeOpcode:   return "stateDecodeOpcode";
            case stateEndInstruction: return "stateEndInstruction";
            case stateBeginModRM:     return "stateBeginModRM";
            case stateDecodeModRM:    return "stateDecodeModRM";
            case stateEAOffset:       return "stateEAOffset";
            case stateEARegisters:    return "stateEARegisters";
            case stateEAByte:         return "stateEAByte";
            case stateEAWord:         return "stateEAWord";
            case stateEASetSegment:   return "stateEASetSegment";
            case stateEAIO:           return "stateEAIO";
            case statePush:           return "statePush";
            case statePush2:          return "statePush2";
            case statePop:            return "statePop";
            case stateALU:            return "stateALU";
            case stateALU2:           return "stateALU2";
            case stateALU3:           return "stateALU3";
            case stateALUAccumImm:    return "stateALUAccumImm";
            case stateALUAccumImm2:   return "stateALUAccumImm2";
            case statePushSegReg:     return "statePushSegReg";
            case statePopSegReg:      return "statePopSegReg";
            case statePopSegReg2:     return "statePopSegReg2";
            case stateSegOverride:    return "stateSegOverride";
            case stateDAA:            return "stateDAA";
            case stateDAS:            return "stateDAS";
            case stateDA:             return "stateDA";
            case stateAAA:            return "stateAAA";
            case stateAAS:            return "stateAAS";
            case stateAA:             return "stateAA";
            case stateIncDecRW:       return "stateIncDecRW";
            case statePushRW:         return "statePushRW";
            case statePopRW:          return "statePopRW";
            case statePopRW2:         return "statePopRW2";
            case stateInvalid:        return "stateInvalid";
            case stateJCond:          return "stateJCond";
            case stateJCond2:         return "stateJCond2";
            case stateALURMImm:       return "stateALURMImm";
            case stateALURMImm2:      return "stateALURMImm2";
            case stateALURMImm3:      return "stateALURMImm3";
            case stateTestRMReg:      return "stateTestRMReg";
            case stateTestRMReg2:     return "stateTestRMReg2";
            case stateXchgRMReg:      return "stateXchgRMReg";
            case stateXchgRMReg2:     return "stateXchgRMReg2";
            case stateMovRMReg:       return "stateMovRMReg";
            case stateMovRMReg2:      return "stateMovRMReg2";
            case stateMovRegRM:       return "stateMovRegRM";
            case stateMovRegRM2:      return "stateMovRegRM2";
            case stateMovRMWSegReg:   return "stateMovRMWSegReg";
            case stateMovRMWSegReg2:  return "stateMovRMWSegReg2";
            case stateLEA:            return "stateLEA";
            case stateLEA2:           return "stateLEA2";
            case stateMovSegRegRMW:   return "stateMovSegRegRMW";
            case stateMovSegRegRMW2:  return "stateMovSegRegRMW2";
            case statePopMW:          return "statePopMW";
            case statePopMW2:         return "statePopMW2";
            case stateXchgAxRW:       return "stateXchgAxRW";
            case stateCBW:            return "stateCBW";
            case stateCWD:            return "stateCWD";
            case stateCallCP:         return "stateCallCP";
            case stateCallCP2:        return "stateCallCP2";
            case stateCallCP3:        return "stateCallCP3";
            case stateCallCP4:        return "stateCallCP4";
            case stateCallCP5:        return "stateCallCP5";
            case stateWait:           return "stateWait";
            case statePushF:          return "statePushF";
            case statePopF:           return "statePopF";
            case statePopF2:          return "statePopF2";
            case stateSAHF:           return "stateSAHF";
            case stateLAHF:           return "stateLAHF";
            case stateMovAccumInd:    return "stateMovAccumInd";
            case stateMovAccumInd2:   return "stateMovAccumInd2";
            case stateMovAccumInd3:   return "stateMovAccumInd3";
            case stateMovIndAccum:    return "stateMovIndAccum";
            case stateMovIndAccum2:   return "stateMovIndAccum2";
            case stateMovS:           return "stateMovS";
            case stateMovS2:          return "stateMovS2";
            case stateRepAction:      return "stateRepAction";
            case stateCmpS:           return "stateCmpS";
            case stateCmpS2:          return "stateCmpS2";
            case stateCmpS3:          return "stateCmpS3";
            case stateTestAccumImm:   return "stateTestAccumImm";
            case stateTestAccumImm2:  return "stateTestAccumImm2";
            case stateStoS:           return "stateStoS";
            case stateLodS:           return "stateLodS";
            case stateLodS2:          return "stateLodS2";
            case stateScaS:           return "stateScaS";
            case stateScaS2:          return "stateScaS2";
            case stateMovRegImm:      return "stateMovRegImm";
            case stateMovRegImm2:     return "stateMovRegImm2";
            case stateRet:            return "stateRet";
            case stateRet2:           return "stateRet2";
            case stateRet3:           return "stateRet3";
            case stateRet4:           return "stateRet4";
            case stateRet5:           return "stateRet5";
            case stateRet6:           return "stateRet6";
            case stateLoadFar:        return "stateLoadFar";
            case stateLoadFar2:       return "stateLoadFar2";
            case stateLoadFar3:       return "stateLoadFar3";
            case stateMovRMImm:       return "stateMovRMImm";
            case stateMovRMImm2:      return "stateMovRMImm2";
            case stateMovRMImm3:      return "stateMovRMImm3";
            case stateCheckInt:       return "stateCheckInt";
            case stateHardwareInt:    return "stateHardwareInt";
            case stateHardwareInt2:   return "stateHardwareInt2";
            case stateInt:            return "stateInt";
            case stateInt3:           return "stateInt3";
            case stateIntAction:      return "stateIntAction";
            case stateIntAction2:     return "stateIntAction2";
            case stateIntAction3:     return "stateIntAction3";
            case stateIntAction4:     return "stateIntAction4";
            case stateIntAction5:     return "stateIntAction5";
            case stateIntAction6:     return "stateIntAction6";
            case stateIntO:           return "stateIntO";
            case stateIRet:           return "stateIRet";
            case stateIRet2:          return "stateIRet2";
            case stateIRet3:          return "stateIRet3";
            case stateIRet4:          return "stateIRet4";
            case stateShift:          return "stateShift";
            case stateShift2:         return "stateShift2";
            case stateShift3:         return "stateShift3";
            case stateAAM:            return "stateAAM";
            case stateAAM2:           return "stateAAM2";
            case stateAAD:            return "stateAAD";
            case stateAAD2:           return "stateAAD2";
            case stateSALC:           return "stateSALC";
            case stateXlatB:          return "stateXlatB";
            case stateXlatB2:         return "stateXlatB2";
            case stateEscape:         return "stateEscape";
            case stateEscape2:        return "stateEscape2";
            case stateLoop:           return "stateLoop";
            case stateLoop2:          return "stateLoop2";
            case stateInOut:          return "stateInOut";
            case stateInOut2:         return "stateInOut2";
            case stateInOut3:         return "stateInOut3";
            case stateCallCW:         return "stateCallCW";
            case stateCallCW2:        return "stateCallCW2";
            case stateCallCW3:        return "stateCallCW3";
            case stateCallCW4:        return "stateCallCW4";
            case stateJmpCW:          return "stateJmpCW";
            case stateJmpCW2:         return "stateJmpCW2";
            case stateJmpCW3:         return "stateJmpCW3";
            case stateJmpCP:          return "stateJmpCP";
            case stateJmpCP2:         return "stateJmpCP2";
            case stateJmpCP3:         return "stateJmpCP3";
            case stateJmpCP4:         return "stateJmpCP4";
            case stateJmpCB:          return "stateJmpCB";
            case stateJmpCB2:         return "stateJmpCB2";
            case stateLock:           return "stateLock";
            case stateRep:            return "stateRep";
            case stateHlt:            return "stateHlt";
            case stateCmC:            return "stateCmC";
            case stateMath:           return "stateMath";
            case stateMath2:          return "stateMath2";
            case stateMath3:          return "stateMath3";
            case stateLoadC:          return "stateLoadC";
            case stateLoadI:          return "stateLoadI";
            case stateLoadD:          return "stateLoadD";
            case stateMisc:           return "stateMisc";
            case stateMisc2:          return "stateMisc2";
        }
        return "";
    }
    static String stringForIOType(IOType type)
    {
        switch (type) {
            case ioNone:             return "ioNone";
            case ioRead:             return "ioRead";
            case ioWrite:            return "ioWrite";
            case ioInstructionFetch: return "ioInstructionFetch";
        }
        return "";
    }
    static String stringForBusState(BusState state)
    {
        switch (state) {
            case t1:    return "t1";
            case t2:    return "t2";
            case t3:    return "t3";
            case tWait: return "tWait";
            case t4:    return "t4";
            case tIdle: return "tIdle";
            case tDMA:  return "tDMA";
        }
        return "";
    }
    static String stringForIOByte(IOByte byte)
    {
        switch (byte) {
            case ioSingleByte: return "ioSingleByte";
            case ioWordFirst:  return "ioWordFirst";
            case ioWordSecond: return "ioWordSecond";
        }
        return "";
    }
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
    void interrupt(UInt8 number)
    {
        _data = number;
        _state = stateIntAction;
        _afterInt = stateEndInstruction;
    }
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
    void loadEA(State state) { _afterEA = state; _state = stateBeginModRM; }
    void readEA(State state)
    {
        _afterEAIO = state;
        _ioType = ioRead;
        loadEA(stateEAIO);
    }
    void fetch(State state, bool wordSize)
    {
        initIO(state, ioInstructionFetch, wordSize);
    }
    void writeEA(UInt16 data, int wait)
    {
        _data = data;
        _wait = wait;
        _afterEAIO = stateEndInstruction;
        _ioType = ioWrite;
        _state = stateEAIO;
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
    template<class U> class Register
    {
    public:
        void init(String name, U* data)
        {
            _name = name;
            _data = data;
            _read = false;
            _written = false;
        }
        const U& operator=(U value)
        {
            *_data = value;
            _writtenValue = value;
            _written = true;
            return *_data;
        }
        operator U()
        {
            if (!_read && !_written) {
                _read = true;
                _readValue = *_data;
            }
            return *_data;
        }
        const U& operator+=(U value)
        {
            return operator=(operator U() + value);
        }
        const U& operator-=(U value)
        {
            return operator=(operator U() - value);
        }
        const U& operator%=(U value)
        {
            return operator=(operator U() % value);
        }
        const U& operator&=(U value)
        {
            return operator=(operator U() & value);
        }
        const U& operator^=(U value)
        {
            return operator=(operator U() ^ value);
        }
        U operator-() const { return -operator U(); }
        void operator++() { operator=(operator U() + 1); }
        void operator--() { operator=(operator U() - 1); }
        String text()
        {
            String text;
            if (_read) {
                text = hex(_readValue, sizeof(U)==1 ? 2 : 4, false) + "<-" +
                    _name + "  ";
                _read = false;
            }
            if (_written) {
                text += _name + "<-" +
                    hex(_writtenValue, sizeof(U)==1 ? 2 : 4, false) + "  ";
                _written = false;
            }
            return text;
        }
    protected:
        String _name;
        U* _data;
        bool _read;
        bool _written;
        U _readValue;
        U _writtenValue;
    };
    class FlagsRegister : public Register<UInt16>
    {
    public:
        UInt32 operator=(UInt32 value)
        {
            Register<UInt16>::operator=(value);
            return Register<UInt16>::operator UInt16();
        }
        String text()
        {
            String text;
            if (this->_read) {
                text = flags(this->_readValue) + String("<-") + this->_name +
                    "  ";
                this->_read = false;
            }
            if (this->_written) {
                text += this->_name + String("<-") +
                    flags(this->_writtenValue) + "  ";
                this->_written = false;
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
        return ((_segmentRegisterData[segment] << 4) + offset) & 0xfffff;
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
    State _afterEAIO;
    State _afterRep;
    State _afterInt;
    bool _sourceIsRM;
    UInt16 _savedCS;
    UInt16 _savedIP;
    int _rep;
    bool _usePortSpace;
    bool _halted;
    bool _newInstruction;
    UInt16 _newIP;
    ISA8BitBus* _bus;
    Intel8259PIC* _pic;
    Intel8237DMA* _dma;
    int _cycle;
    int _stopAtCycle;
    UInt32 _busAddress;
    UInt8 _busData;
    bool _nmiRequested;

    Type _stateType;
    Type _ioTypeType;
    Type _ioByteType;
    Type _busStateType;

    Disassembler _disassembler;
};

class ROMDataType : public AtomicType
{
public:
    ROMDataType() : AtomicType(implementation()) { }
private:
    class Implementation : public AtomicType::Implementation
    {
    public:
        Implementation() : AtomicType::Implementation("ROM")
        {
            List<StructuredType::Member> members;
            members.add(StructuredType::Member("mask", Type::integer));
            members.add(StructuredType::Member("address", Type::integer));
            members.add(StructuredType::Member("fileName", Type::string));
            members.add(StructuredType::Member("fileOffset",
                TypedValue(Type::integer, 0)));
            _structuredType = StructuredType(toString(), members);
        }
        TypedValue tryConvert(const TypedValue& value, String* why) const
        {
            TypedValue stv = value.type().tryConvertTo(_structuredType, value,
                why);
            if (!stv.valid())
                return stv;
            auto romMembers =
                stv.value<Value<HashTable<String, TypedValue>>>();
            int mask = (*romMembers)["mask"].value<int>();
            int address = (*romMembers)["address"].value<int>();
            String file = (*romMembers)["fileName"].value<String>();
            int offset = (*romMembers)["fileOffset"].value<int>();
            return TypedValue(ROMDataType(),
                Any(ROMData(mask, address, file, offset)), value.span());
        }
    private:
        static StructuredType _structuredType;
    };
    static Reference<Implementation> _implementation;
    static Reference<Implementation> implementation()
    {
        if (!_implementation.valid())
            _implementation = new Implementation();
        return _implementation;
    }
};

Reference<ROMDataType::Implementation> ROMDataType::_implementation;
StructuredType ROMDataType::Implementation::_structuredType;

#include "cga.h"

template<class T> class SimulatorTemplate : public Component
{
public:
    SimulatorTemplate(File configFile) : _halted(false)
    {
        ROMDataType romDataType;
        Type romImageArrayType = Type::array(romDataType);
        _configFile.addOption("roms", romImageArrayType);
        _configFile.addDefaultOption("stopAtCycle", Type::integer, -1);
        _configFile.addDefaultOption("stopSaveState", Type::string,
            String(""));
        _configFile.addDefaultOption("initialState", Type::string, String(""));

        addComponent(&_bus);
        _bus.addComponent(&_ram);
        _bus.addComponent(&_nmiSwitch);
        _bus.addComponent(&_dmaPageRegisters);
        _bus.addComponent(&_cga);
        _bus.addComponent(&_pit);
        _bus.addComponent(&_dma);
        _bus.addComponent(&_ppi);
        _bus.addComponent(&_pic);
        addComponent(&_cpu);

        _configFile.load(configFile);

        _cga.initialize();
        _ram.initialize();

        List<TypedValue> romDatas = _configFile.get<List<TypedValue> >("roms");
        _roms.allocate(romDatas.count());
        int r = 0;
        for (auto i = romDatas.begin(); i != romDatas.end(); ++i) {
            ROMData romData = (*i).value<ROMData>();
            ROM* rom = &_roms[r];
            _bus.addComponent(rom);
            rom->initialize(romData);
            ++r;
        }

        String stopSaveState = _configFile.get<String>("stopSaveState");

        String initialStateFile = _configFile.get<String>("initialState");
        TypedValue stateValue;
        if (!initialStateFile.empty()) {
            ConfigFile initialState;
            initialState.addDefaultOption("simulator", type(), initial());
            initialState.load(initialStateFile);
            stateValue = initialState.get("simulator");
        }
        else
            stateValue = initial();
        load(stateValue);

        this->_cpu.setStopAtCycle(_configFile.get<int>("stopAtCycle"));
        _stopSaveState = _configFile.get<String>("stopSaveState");

        Rational<int> l = 0;
        for (auto i = _components.begin(); i != _components.end(); ++i) {
            Rational<int> cyclesPerSecond = (*i)->cyclesPerSecond();
            if (cyclesPerSecond != 0)
                if (l == 0)
                    l = cyclesPerSecond;
                else
                    l = lcm(l, cyclesPerSecond);
        }
        if (l == 0)
            throw Exception("None of the components is clocked!");
        _minTicksPerCycle = INT_MAX;
        for (auto i = _components.begin(); i != _components.end(); ++i) {
            Rational<int> cyclesPerSecond = (*i)->cyclesPerSecond();
            if (cyclesPerSecond != 0) {
                Rational<int> t = l / cyclesPerSecond;
                if (t.denominator != 1)
                    throw Exception("Scheduler LCM calculation incorrect");
                int ticksPerCycle = t.numerator;
                (*i)->setTicksPerCycle(ticksPerCycle);
                if (ticksPerCycle < _minTicksPerCycle)
                    _minTicksPerCycle = ticksPerCycle;
            }
            else
                (*i)->setTicksPerCycle(0);
        }
    }
    void simulate()
    {
        do {
            for (auto i = _components.begin(); i != _components.end(); ++i)
                (*i)->simulateTicks(_minTicksPerCycle);
        } while (!_halted);
    }
    String save() const
    {
        String s("simulator = {");
        bool needComma = false;
        for (auto i = _components.begin(); i != _components.end(); ++i) {
            if ((*i)->name().empty())
                continue;
            if (needComma)
                s += ", ";
            needComma = true;
            s += (*i)->save();
        }
        s += "};";
        return s;
    }

    void halt() { _halted = true; }
    ConfigFile* config() { return &_configFile; }
    ISA8BitBus* getBus() { return &_bus; }
    Intel8259PIC* getPIC() { return &_pic; }
    Intel8237DMA* getDMA() { return &_dma; }
    NMISwitch* getNMISwitch() { return &_nmiSwitch; }
    Intel8255PPI* getPPI() { return &_ppi; }
    Intel8088* getCPU() { return &_cpu; }
    IBMCGA* getCGA() { return &_cga; }
    String getStopSaveState() { return _stopSaveState; }
    void addComponent(Component* component)
    {
        _components.add(component);
        component->setSimulator(this);
    }
    Type type() const
    {
        List<StructuredType::Member> members;
        for (auto i = _components.begin(); i != _components.end(); ++i) {
            Type type = (*i)->type();
            if (!type.valid())
                continue;
            members.add(StructuredType::Member((*i)->name(),(*i)->initial()));
        }
        return StructuredType("Simulator", members);
    }
    void load(const TypedValue& value)
    {
        Value<HashTable<String, TypedValue> > object =
            value.value<Value<HashTable<String, TypedValue> > >();
        for (auto i = _components.begin(); i != _components.end(); ++i)
            (*i)->load((*object)[(*i)->name()]);
    }
private:
    List<Component*> _components;
    bool _halted;
    int _minTicksPerCycle;

    ConfigFile _configFile; 
    ISA8BitBus _bus;
    RAM640Kb _ram;
    NMISwitch _nmiSwitch;
    DMAPageRegisters _dmaPageRegisters;
    Intel8253PIT _pit;
    Intel8237DMA _dma;
    Intel8255PPI _ppi;
    Intel8259PIC _pic;
    Intel8088 _cpu;
    IBMCGA _cga;
    Array<ROM> _roms;

    String _stopSaveState;
};

class RGBIMonitor : public Sink<BGRI>
{
public:
    RGBIMonitor() : _renderer(&_window), _texture(&_renderer)
    {
        _palette.allocate(64);
        _palette[0x0] = 0xff000000;
        _palette[0x1] = 0xff0000aa;
        _palette[0x2] = 0xff00aa00;
        _palette[0x3] = 0xff00aaaa;
        _palette[0x4] = 0xffaa0000;
        _palette[0x5] = 0xffaa00aa;
        _palette[0x6] = 0xffaa5500;
        _palette[0x7] = 0xffaaaaaa;
        _palette[0x8] = 0xff555555;
        _palette[0x9] = 0xff5555ff;
        _palette[0xa] = 0xff55ff55;
        _palette[0xb] = 0xff55ffff;
        _palette[0xc] = 0xffff5555;
        _palette[0xd] = 0xffff55ff;
        _palette[0xe] = 0xffffff55;
        _palette[0xf] = 0xffffffff;

        // Create some special colours for visualizing sync pulses.
        for (int i = 0; i < 16; ++i) {
            int r = ((_palette[i] >> 16) & 0xff) >> 4;
            int g = ((_palette[i] >> 8) & 0xff) >> 4;
            int b = (_palette[i] & 0xff) >> 4;
            int rgb = (r << 16) + (g << 8) + b;
            _palette[i + 16] = 0xff002200 + rgb; // hsync
            _palette[i + 32] = 0xff220022 + rgb; // vsync
            _palette[i + 48] = 0xff222222 + rgb; // hsync+vsync
        }
    }

    // We ignore the suggested number of samples and just read a whole frame's
    // worth once there is enough for a frame.
    void consume(int nSuggested)
    {
        // Since the pumping is currently done by Simulator::simulate(), don't
        // try to pull more data from the CGA than we have.
        if (remaining() < 912*262 + 1)
            return;

        // We have enough data for a frame - update the screen.
        Accessor<BGRI> reader = Sink::reader(912*262 + 1);
        SDLTextureLock _lock(&_texture);
        int y = 0;
        int x = 0;
        bool hSync = false;
        bool vSync = false;
        bool oldHSync = false;
        bool oldVSync = false;
        int n = 0;
        UInt8* row = reinterpret_cast<UInt8*>(_lock._pixels);
        int pitch = _lock._pitch;
        UInt32* output = reinterpret_cast<UInt32*>(row);
        do {
            BGRI p = reader.item();
            hSync = ((p & 0x10) != 0);
            vSync = ((p & 0x20) != 0);
            if (x == 912 || (oldHSync && !hSync)) {
                x = 0;
                ++y;
                row += pitch;
                output = reinterpret_cast<UInt32*>(row);
            }
            if (y == 262 || (oldVSync && !vSync))
                break;
            oldHSync = hSync;
            oldVSync = vSync;
            *output = _palette[p];
            ++output;
            ++n;
            ++x;
            reader.advance(1);
        } while (true);
        read(n);
        _renderer.renderTexture(&_texture);
    }

private:
    SDLWindow _window;
    SDLRenderer _renderer;
    SDLTexture _texture;
    Array<UInt32> _palette;
};

class Program : public ProgramBase
{
protected:
    void run()
    {
        // We should remove SDL_INIT_NOPARACHUTE when building for Linux if we
        // go fullscreen, otherwise the desktop resolution would not be
        // restored on a crash. Otherwise it's a bad idea since if the program
        // crashes all invariants are destroyed and any further execution could
        // cause data loss.
        SDL sdl(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE);

        if (_arguments.count() < 2) {
            console.write("Syntax: " + _arguments[0] +
                " <config file name>\n");
            return;
        }

        RGBIMonitor monitor;
        Simulator simulator(File(_arguments[1], CurrentDirectory(), true));
        monitor.connect(simulator.getCGA()->bgriSource());

        //File file(config.get<String>("sourceFile"));
        //String contents = file.contents();
        //CharacterSource source(contents, file.path());
        //Space::parse(&source);
        //SourceProgram sourceProgram = parseSourceProgram(&source);
        //sourceProgram.assemble(&simulator);
        class Saver
        {
        public:
            Saver(Simulator* simulator) : _simulator(simulator) { }
            ~Saver()
            {
                try {
                    String save = _simulator->save();
                    String stopSaveState = _simulator->getStopSaveState();
                    if (!stopSaveState.empty())
                        File(stopSaveState).save(save);
                }
                catch (...) {
                }
            }
        private:
            Simulator* _simulator;
        };
        Saver saver(&simulator);
        //DWORD tc0 = GetTickCount();
        simulator.simulate();
        //DWORD tc1 = GetTickCount();
        //console.write("Elapsed time: " + String(decimal(tc1 - tc0)) + "ms\n");
    }
};
