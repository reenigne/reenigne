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

template<class T> class Intel8255PPITemplate;
typedef Intel8255PPITemplate<void> Intel8255PPI;

template<class T> class RAMTemplate;
typedef RAMTemplate<void> RAM;

template<class T> class ROMTemplate;
typedef ROMTemplate<void> ROM;

template<class T> class IBMCGATemplate;
typedef IBMCGATemplate<void> IBMCGA;

template<class T> class DMAPageRegistersTemplate;
typedef DMAPageRegistersTemplate<void> DMAPageRegisters;

template<class T> class ISA8BitBusTemplate;
typedef ISA8BitBusTemplate<void> ISA8BitBus;

class TimeType : public Nullary<Type, TimeType>
{
public:
    static String name() { return "Time"; }
};

template<> Nullary<Type, TimeType> Nullary<Type, TimeType>::_instance;

class Tick
{
    typedef unsigned int Base;
public:
    Tick(const Base& t) { _t = t; }
    bool operator==(const Tick& other) const { return _t == other._t; }
//    bool operator!=(const Tick& other) const { return _t != other._t; }
    bool operator<=(const Tick& other) const
    {
        return other._t - _t < (static_cast<Base>(-1) >> 1);
    }
    bool operator<(const Tick& other) const
    {
        return (*this) <= other && (*this) != other;
    }
    bool operator>=(const Tick& other) const { return other <= *this; }
    bool operator>(const Tick& other) const { return other < *this; }
    const Tick& operator+=(const Tick& other) { _t += other._t; return *this; }
    const Tick& operator-=(const Tick& other) { _t -= other._t; return *this; }
    Tick operator+(const Tick& other) { return Tick(_t + other._t); }
    Tick operator-(const Tick& other) { return Tick(_t - other._t); }
    operator Base() const { return _t; }
private:
    Base _t;
};

template<class T> class ComponentTemplate : public Structure
{
public:
    ComponentTemplate() : _simulator(0), _tick(0), _ticksPerCycle(0)
    {
    }
    void setSimulator(Simulator* simulator) { _simulator = simulator; site(); }
    virtual void site() { }
    virtual void simulateCycle() { }
    virtual String save() const { return String(); }
    virtual ::Type persistenceType() const
    {
        return ::Type();
    }
    String name() const { return _name; }
    virtual void load(const TypedValue& value) { }
    virtual TypedValue initial() const
    {
        return StructuredType::empty().convertTo(type());
    }
    virtual Rational cyclesPerSecond() const
    {
        Rational h = hDotsPerCycle();
        if (h == 0)
            return 0;
        return 157500000/(11*h);
    }
    virtual Rational hDotsPerCycle() const { return 0; }
    void setTicksPerCycle(Tick ticksPerCycle)
    {
        _ticksPerCycle = ticksPerCycle;
        _tick = 0;
    }
    void simulateTicks(Tick ticks)
    {
        _tick += ticks;
        if (_ticksPerCycle != 0 && _tick >= _ticksPerCycle) {
            simulateCycle();
            _tick -= _ticksPerCycle;
        }
    }
    void set(String name, TypedValue value)
    {
        if (name == "*")
            _name = value.value<String>();
    }
    class Type : public ::Type
    {
    protected:
        Type(const Implementation* implementation) : ::Type(implementation) { }
        class Implementation : public ::Type::Implementation
        {
        public:
            Implementation(Simulator* simulator) : _simulator(simulator) { }
            bool has(String memberName) const { return memberName == "*"; }
        protected:
            Simulator* _simulator;
        };
    };

    virtual Type type() const = 0;
protected:
    Tick _tick;

    SimulatorTemplate<T>* _simulator;
private:
    Tick _ticksPerCycle;
    String _name;
};

class Connector
{
public:
    class Type : public ::Type
    {
    public:
        bool compatible(Type other) const
        {
            return implementation()->compatible(other);
        }
        bool canConnectMultiple() const
        {
            return implementation()->canConnectMultiple();
        }
    private:
        class Implementation : public ::Type::Implementation
        {
        public:
            virtual bool compatible(Type other) const = 0;
            virtual bool canConnectMultiple() const = 0;
        };
        const Implementation* implementation() const
        {
            return _implementation.referent<Implementation>();
        }
    };

    virtual Type type() const = 0;
    virtual void connect(Connector* other) = 0;
};

template<class T> class SimulatorTemplate
{
public:
    SimulatorTemplate() : _halted(false)
    {
        Rational l = 0;
        for (auto i = _components.begin(); i != _components.end(); ++i) {
            Rational cyclesPerSecond = (*i)->cyclesPerSecond();
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
            Rational cyclesPerSecond = (*i)->cyclesPerSecond();
            if (cyclesPerSecond != 0) {
                Rational t = l / cyclesPerSecond;
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
        String s("{");
        bool needComma = false;
        for (auto i = _components.begin(); i != _components.end(); ++i) {
            if ((*i)->name().empty())
                continue;
            if (needComma)
                s += ", ";
            needComma = true;
            s += (*i)->name() + ": " + (*i)->save();
        }
        s += "};";
        return s;
    }

    void halt() { _halted = true; }
    void addComponent(Component* component)
    {
        _components.add(component);
    }
    void load(String initialStateFile)
    {
        TypedValue value;
        if (!initialStateFile.empty()) {
            ConfigFile initialState;
            initialState.addDefaultOption(name(), persistenceType(),
                initial());
            initialState.load(initialStateFile);
            value = initialState.getValue(name());
        }
        else
            value = initial();

        Value<HashTable<Identifier, TypedValue> > object =
            value.value<Value<HashTable<Identifier, TypedValue> > >();
        for (auto i = _components.begin(); i != _components.end(); ++i)
            (*i)->load((*object)[(*i)->name()]);
    }
    String name() const { return "simulator"; }
private:
    TypedValue initial() const
    {
        return StructuredType::empty().convertTo(persistenceType());
    }
    ::Type persistenceType() const
    {
        List<StructuredType::Member> members;
        for (auto i = _components.begin(); i != _components.end(); ++i) {
            Type type = (*i)->type();
            if (!type.valid())
                continue;
            members.add(StructuredType::Member((*i)->name(), (*i)->initial()));
        }
        return StructuredType("Simulator", members);
    }

    List<Component*> _components;
    bool _halted;
    int _minTicksPerCycle;
};

#include "isa_8_bit_bus.h"
#include "nmi_switch.h"
#include "8259.h"
#include "8237.h"
#include "pcxt_keyboard.h"
#include "pcxt_keyboard_port.h"
#include "8255.h"
#include "8253.h"
#include "mc6845.h"
#include "dram.h"
#include "ram.h"
#include "dma_page_registers.h"
#include "rom.h"
#include "8088.h"
#include "cga.h"
#include "rgbi_monitor.h"

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

        Simulator simulator;
        Simulator* p = &simulator;

        List<Component::Type> componentTypes;
        componentTypes.add(Intel8088::Type(p));
        componentTypes.add(ISA8BitBus::Type(p));
        componentTypes.add(RAM::Type(p));
        componentTypes.add(NMISwitch::Type(p));
        componentTypes.add(DMAPageRegisters::Type(p));
        componentTypes.add(Intel8259PIC::Type(p));
        componentTypes.add(Intel8237DMA::Type(p));
        componentTypes.add(Intel8255PPI::Type(p));
        componentTypes.add(Intel8253PIT::Type(p));
        componentTypes.add(PCXTKeyboardPort::Type(p));
        componentTypes.add(PCXTKeyboard::Type(p));
        componentTypes.add(IBMCGA::Type(p));
        componentTypes.add(RGBIMonitor::Type(p));

        ConfigFile configFile;
        configFile.addDefaultOption("stopSaveState", StringType(), String(""));
        configFile.addDefaultOption("initialState", StringType(), String(""));
        configFile.addType(TimeType());

        for (auto i = componentTypes.begin(); i != componentTypes.end(); ++i)
            configFile.addType(*i);

        configFile.addDefaultOption("second", TimeType(), Rational(1));

        configFile.load(File(_arguments[1], CurrentDirectory(), true));

        String stopSaveState = configFile.get<String>("stopSaveState");

        String initialStateFile = configFile.get<String>("initialState");
        simulator.load(initialStateFile);

        class Saver
        {
        public:
            Saver(Simulator* simulator, String stopSaveState)
              : _simulator(simulator), _stopSaveState(stopSaveState) { }
            ~Saver()
            {
                try {
                    String save = _simulator->name() + " = " +
                        _simulator->save();
                    if (!_stopSaveState.empty())
                        File(_stopSaveState).save(save);
                }
                catch (...) {
                }
            }
        private:
            Simulator* _simulator;
            String _stopSaveState;
        };
        Saver saver(&simulator, stopSaveState);
        simulator.simulate();
    }
};
