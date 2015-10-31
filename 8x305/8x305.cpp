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
#include "alfe/sdl.h"

#include <stdlib.h>
#include <limits.h>

//class SourceProgram
//{
//};

template<class T> class SimulatorTemplate;
typedef SimulatorTemplate<void> Simulator;

template<class T> class Signetics8x305Template;
typedef Signetics8x305Template<void> Signetics8x305;

template<class T> class ComponentTemplate;
typedef ComponentTemplate<void> Component;

template<class T> class RAM32KbTemplate;
typedef RAM32KBTemplate<void> RAM32KB;

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
        return TypedValue(
            StructuredType(String(), List<StructuredType::Member>()),
            HashTable<Identifier, TypedValue>()).convertTo(type());
    }
    virtual Rational<int> cyclesPerSecond() const
    {
        Rational<int> h = hDotsPerCycle();
        if (h == 0)
            return 0;
        return 20000000/h;
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
    SimulatorTemplate<T>* _simulator;
private:
    int _ticksPerCycle;
    int _tick;
};

template<class T> class RAM32KBTemplate : public Component
{
public:
    RAM32KBTemplate() : _data(0x8000)
    {
        // _rowBits is 7 for 4116 RAM chips
        //             8 for 4164
        //             9 for 41256
        // We use 9 here because programs written for _rowBits == N will work
        // for _rowBits < N but not necessarily _rowBits > N.
        // TODO: make this settable in the config file.
        int rowBits = 9;

        // DRAM decay time in cycles.
        // This is the fastest that DRAM could decay and real hardware would
        // still work.
        // TODO: make this settable in the config file.
        _decayTime = (18*4) << rowBits;
        // 2ms for 4116
        // 4ms for 4164
        // 8ms for 41256
        //   _decayTime =  (13125 << rowBits) / 176;

        // Initially, all memory is decayed so we'll get an NMI if we try to
        // read from it.
        _cycle = _decayTime;

        _rows = 1 << rowBits;
        _refreshTimes.allocate(_rows);
        for (int r = 0; r < _rows; ++r)
            _refreshTimes[r] = 0;
        _rowMask = _rows - 1;
        for (int a = 0; a < 0xa0000; ++a)
            _data[a] = 0;
    }
    void site()
    {
        _nmiSwitch = this->_simulator->getNMISwitch();
        _ppi = this->_simulator->getPPI();
        _cpu = this->_simulator->getCPU();
    }
    Rational<int> hDotsPerCycle() const { return 3; }
    void simulateCycle()
    {
        ++_cycle;
        if (_cycle == 0x40000000) {
            int adjust = _decayTime - _cycle;
            for (int r = 0; r < _rows; ++r)
                if (_refreshTimes[r] + adjust > 0)
                    _refreshTimes[r] += adjust;
                else
                    _refreshTimes[r] = 0;
            _cycle += adjust;
        }
    }
    void setAddress(UInt32 address)
    {
        _address = address & 0x400fffff;
        _active = (_address < 0xa0000);
    }
    void read()
    {
        int row = _address & _rowMask;
        if (_cycle >= _refreshTimes[row] + _decayTime) {
            // RAM has decayed! On a real machine this would not always signal
            // an NMI but we'll make the NMI happen every time to make DRAM
            // decay problems easier to find.

            // TODO: In the config file we might want to have an option for
            // "realistic mode" to prevent this from being used to detect the
            // emulator.
            if (_nmiSwitch->nmiOn() && (_ppi->portB() & 0x10) != 0)
                _cpu->nmi();
        }
        _refreshTimes[row] = _cycle;
        set(_data[_address]);
    }
    void write(UInt8 data)
    {
        _refreshTimes[_address & _rowMask] = _cycle;
        _data[_address] = data;
    }
    UInt8 memory(UInt32 address)
    {
        if (address < 0xa0000)
            return _data[address];
        return 0xff;
    }
    String save() const
    {
        String s("ram: ###\n");
        for (int y = 0; y < 0xa0000; y += 0x20) {
            String line;
            bool gotData = false;
            for (int x = 0; x < 0x20; x += 4) {
                int p = y + x;
                UInt32 v = (_data[p]<<24) + (_data[p+1]<<16) +
                    (_data[p+2]<<8) + _data[p+3];
                if (v != 0)
                    gotData = true;
                line += " " + hex(v, 8, false);
            }
            if (gotData)
                s += hex(y, 5, false) + ":" + line + "\n";
        }
        s += "###\n";
        return s;
    }
    Type type() const { return Type::string; }
    void load(const TypedValue& value)
    {
        String s = value.value<String>();
        CharacterSource source(s);
        Space::parse(&source);
        do {
            Span span;
            int t = parseHexadecimalCharacter(&source, &span);
            if (t == -1)
                break;
            int a = t;
            for (int i = 0; i < 4; ++i) {
                t = parseHexadecimalCharacter(&source, &span);
                if (t < 0)
                    span.throwError("Expected hexadecimal character");
                a = (a << 4) + t;
            }
            Space::assertCharacter(&source, ':', &span);
            for (int i = 0; i < 16; ++i) {
                t = parseHexadecimalCharacter(&source, &span);
                if (t < 0)
                    span.throwError("Expected hexadecimal character");
                int t2 = parseHexadecimalCharacter(&source, &span);
                if (t2 < 0)
                    span.throwError("Expected hexadecimal character");
                _data[a++] = (t << 4) + t2;
                Space::parse(&source);
            }
        } while (true);
        CharacterSource s2(source);
        if (s2.get() != -1)
            source.location().throwError("Expected hexadecimal character");
    }
    String name() const { return "ram"; }
    TypedValue initial() const { return String(); }
private:
    int _address;
    Array<UInt8> _data;
    Array<int> _refreshTimes;
    int _rows;
    int _rowMask;
    int _cycle;
    int _decayTime;
    NMISwitch* _nmiSwitch;
    Intel8255PPI* _ppi;
    Intel8088Template<T>* _cpu;
};

class ROM : public Component
{
public:
    void initialize(const ROMData& romData, const File& configFile)
    {
        _mask = romData.mask() | 0xc0000000;
        _start = romData.start();
        String data = File(romData.file(), configFile.parent(), true).
            contents();
        int length = ((_start | ~_mask) & 0xfffff) + 1 - _start;
        _data.allocate(length);
        int offset = romData.offset();
        for (int i = 0; i < length; ++i)
            _data[i] = data[i + offset];
    }
    void setAddress(UInt32 address)
    {
        _address = address & 0xfffff & ~_mask;
        _active = ((address & _mask) == _start);
    }
    void read() { set(_data[_address & ~_mask]); }
    UInt8 memory(UInt32 address)
    {
        if ((address & _mask) == _start)
            return _data[address & ~_mask];
        return 0xff;
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
};

typedef DisassemblerTemplate<void> Disassembler;

template<class T> class Signetics8x305Template : public ComponentTemplate<T>
{
public:
    enum IVBank { ivLeft, ivRight };
    enum IVType { ivSelect, ivWrite };
    void simulateCycle()
    {
        UInt16 instruction = programBus(_next);

        int s = (instruction >> 8) & 0x1f;
        int l = (instruction >> 5) & 7;
        int d = instruction & 0x1f;
        IVBank s1 = ((s & 8) != 0 ? ivRight : ivLeft);
        IVBank d1 = ((d & 8) != 0 ? ivRight : ivLeft);
        int s0 = s & 7;

        if ((instruction & 0x8000) == 0x0000) {
            // The ALU instructions are very regular.
            int r = 0;
            UInt8 v0;
            UInt8 v;
            if ((instruction & 0x1000) != 0) {
                v0 = ivBusRead(s1);
                v = v0;
                r = s0;
            }
            else {
                if ((instruction & 0x0010) != 0)
                    v0 = ivBusRead(d1);
                else
                    r = l;
                v = _registers[s];
            }
            simulateOtherCycles(3);
            v = rotateRight(v, r);
            if ((instruction & 0x1000) != 0)
                v &= mask(l, 0);

            switch (instruction & 0xe000) {
                case 0x0000:
                    // MOVE
                    break;
                case 0x2000:
                    // ADD
                    {
                        int r = v + aux();
                        // TODO: Check what the other bits of OVF are.
                        _registers[8] = (r >> 8) & 1;
                        v = r;
                    }
                    break;
                case 0x4000:
                    // AND
                    v &= aux();
                    break;
                case 0x6000:
                    // XOR
                    v ^= aux();
                    break;
            }

            if ((instruction & 0x0010) != 0)
                mergeWrite(d1, v0, v, l, d & 7);
            else
                setRegister(d, v);
            ++_pc;
            _next = _pc;
            simulateOtherCycles(1);
            return;
        }
        // The other instructions are less regular but use some similar
        // patterns.
        switch (instruction & 0xf000) {
            case 0x8000:
                // XEC register
                _next = ((_registers[s] + instruction) & 0xff) | (_pc & 0x1f00);
                simulateOtherCycles(4);
                break;
            case 0x9000:
                // XEC IV bus
                _next = (((ivBusRead(s1) & mask(l, 0)) + instruction) & 0x1f) | (_pc & 0x1fe0);
                simulateOtherCycles(4);
                break;
            case 0xa000:
                // NZT register
                if (_registers[s] != 0)
                    _pc = (instruction & 0xff) | (_pc & 0x1f00);
                else
                    ++_pc;
                _next = _pc;
                simulateOtherCycles(4);
                break;
            case 0xb000:
                // NZT IV bus
                if ((rotateRight(ivBusRead(s1), s0) & mask(l, 0)) != 0)
                    _pc = (instruction & 0x1f) | (_pc & 0x1fe0);
                else
                    ++_pc;
                _next = _pc;
                simulateOtherCycles(4);
                break;
            case 0xc000:
                // XMIT register
                // XMIT register, IV bus
                // XMIT register, IV bus address
                simulateOtherCycles(3);
                if ((s & 14) == 10)
                    ivBusWrite((s & 1) != 0 ? ivRight : ivLeft, ivWrite, instruction);
                else
                    setRegister(s, instruction);
                ++_pc;
                _next = _pc;
                simulateOtherCycles(1);
                break;
            case 0xd000:
                // XMIT IV bus
                v0 = ivBusRead(s1);
                simulateOtherCycles(3);
                mergeWrite(s1, v0, instruction & 0x1f, l, s0);
                ++_pc;
                _next = _pc;
                simulateOtherCycles(1);
                break;
            case 0xe000:
            case 0xf000:
                // JMP
                _pc = instruction & 0x1fff;
                _next = _pc;
                simulateOtherCycles(4);
                break;
        }
    }
    void mergeWrite(IVBank bank, UInt8 v0, UInt8 v, int l, int sh)
    {
        // TODO: Figure out what happens if l + sh > 8
        UInt8 m = mask(l, sh);
        ivBusWrite(bank, ivWrite, (v0 & ~m) | ((v << sh) & m));
    }
    UInt8 rotateRight(UInt8 v, int n) { return (v >> n) | (v << (8 - n)); }
    UInt8 mask(int n, int sh) { return ((1 << n) - 2) | 1) << sh; }
    UInt8 aux() { return _registers[0]; }
    void setRegister(int d, UInt8 v)
    {
        if (d == 8) {
            // TODO: Figure out what real hardware does here
            throw Exception("OVF is read only");
        }
        _registers[d] = v;
        if ((d & 7) == 7)
            ivBusWrite((d & 8) != 0 ? ivRight : ivLeft, ivSelect, v);
    }
    UInt16 programBus(UInt16 pc)
    {
        // TODO
    }
    void ivBusWrite(IVBank bank, IVType type, UInt8 data)
    {
        // TODO
    }
    UInt8 ivBusRead(IVBank bank)
    {
        // TODO
    }
    void simulateOtherCycles(int n)
    {
        // TODO
    }

private:
    UInt16 _pc;
    UInt16 _next;
    UInt8 _registers[0x10];
};

class ROMDataType : public NamedNullary<Type, ROMDataType>
{
public:
    ROMDataType() : AtomicType(body()) { }
private:
    class Body : public NamedNullary<Type, ROMDataType>::Body
    {
    public:
        static String name() { return "ROM"; }
        Body()
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
            auto romMembers = stv.value<HashTable<Identifier, TypedValue>>();
            int mask = romMembers["mask"].value<int>();
            int address = romMembers["address"].value<int>();
            String file = romMembers["fileName"].value<String>();
            int offset = romMembers["fileOffset"].value<int>();
            return TypedValue(ROMDataType(),
                Any(ROMData(mask, address, file, offset)), value.span());
        }
    private:
        static StructuredType _structuredType;
    };
};

template<> Nullary<Type, ROMDataType> Nullary<Type, ROMDataType>::_instance;
StructuredType ROMDataType::Body::_structuredType;

template<class T> class SimulatorTemplate : public Component
{
protected:
    SimulatorTemplate(File configFile) : _halted(false)
    {
        ConfigFile config;

        config.addDefaultOption("rom", Type::string, String(""));
        config.addDefaultOption("stopAtCycle", Type::integer, -1);
        config.addDefaultOption("stopSaveState", Type::string, String(""));
        config.addDefaultOption("initialState", Type::string, String(""));

        config.load(configFile);

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

        List<TypedValue> romDatas = config.get<List<TypedValue> >("roms");
        _roms.allocate(romDatas.count());
        int r = 0;
        for (auto i = romDatas.begin(); i != romDatas.end(); ++i) {
            ROMData romData = (*i).value<ROMData>();
            ROM* rom = &_roms[r];
            rom->initialize(romData, configFile);
            _bus.addComponent(rom);
            ++r;
        }

        _cga.initialize(config.get<String>("cgarom"), configFile);

        String stopSaveState = config.get<String>("stopSaveState");

        String initialStateFile = config.get<String>("initialState");
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

        _cpu.setStopAtCycle(config.get<int>("stopAtCycle"));
        _stopSaveState = config.get<String>("stopSaveState");

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
public:
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
    ISA8BitBus* getBus() { return &_bus; }
    Intel8259PIC* getPIC() { return &_pic; }
    Intel8237DMA* getDMA() { return &_dma; }
    NMISwitch* getNMISwitch() { return &_nmiSwitch; }
    Intel8255PPI* getPPI() { return &_ppi; }
    Intel8088* getCPU() { return &_cpu; }
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
        HashTable<Identifier, TypedValue> object =
            value.value<HashTable<Identifier, TypedValue>>();
        for (auto i = _components.begin(); i != _components.end(); ++i)
            (*i)->load(object[(*i)->name()]);
    }
    IBMCGA _cga;
private:
    List<Component*> _components;
    bool _halted;
    int _minTicksPerCycle;

    Signetics8x305 _cpu;
    ROM _rom;
    Synth _synth;
    RAM32KB _ram;
    IVBus _bus;
    Video _video;
    Motorola6845CRTC _crtc;

    String _stopSaveState;
};

class VGAMonitor : public Sink<UInt32>
{
public:
    VGAMonitor() : _renderer(&_window), _texture(&_renderer) { }

    // We ignore the suggested number of samples and just read a whole frame's
    // worth once there is enough for a frame.
    void consume(int nSuggested)
    {
        // Since the pumping is currently done by Simulator::simulate(), don't
        // try to pull more data from the VGA than we have.
        if (remaining() < 640*525 + 1)
            return;

        // We have enough data for a frame - update the screen.
        Accessor<UInt32> reader = Sink::reader(640*525 + 1);
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
            UInt32 p = reader.item();
            hSync = ((p & 0x1000000) != 0);
            vSync = ((p & 0x2000000) != 0);
            if (x == 640 || (oldHSync && !hSync)) {
                x = 0;
                ++y;
                row += pitch;
                output = reinterpret_cast<UInt32*>(row);
            }
            if (y == 525 || (oldVSync && !vSync))
                break;
            oldHSync = hSync;
            oldVSync = vSync;
            *output = 0xff000000 | (p & 0xffffff);
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

        VGAMonitor monitor;
        Simulator simulator(File(_arguments[1], CurrentDirectory(), true));
        monitor.connect(&simulator._vga);

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
