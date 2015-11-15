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
#include "alfe/reference.h"

#include <stdlib.h>
#include <limits.h>

typedef UInt8 BGRI;

//class SourceProgram
//{
//};

template<class T> class SimulatorTemplate;
typedef SimulatorTemplate<void> Simulator;

template<class T> class Intel8088CPUTemplate;
typedef Intel8088CPUTemplate<void> Intel8088CPU;

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

Concrete second;

class Tick
{
    typedef int Base;
public:
    Tick() : _t(0) { }
    Tick(const Base& t) : _t(t) { }
    bool operator<=(const Tick& other) const { return _t <= other._t; }
    bool operator<(const Tick& other) const { return _t < other._t; }
    bool operator>=(const Tick& other) const { return _t >= other._t; }
    bool operator>(const Tick& other) const { return _t > other._t; }
    const Tick& operator+=(const Tick& other) { _t += other._t; return *this; }
    const Tick& operator-=(const Tick& other) { _t -= other._t; return *this; }
    Tick operator+(const Tick& other) { return Tick(_t + other._t); }
    Tick operator-(const Tick& other) { return Tick(_t - other._t); }
    operator String() const { return decimal(_t); }
    Tick operator-() const { return Tick(-_t); }
    Tick operator*(int other) { return Tick(_t*other); }
private:
    Base _t;
};

template<class T> class ComponentTemplate : public Structure
{
public:
    ComponentTemplate() : _simulator(0), _tick(0), _ticksPerCycle(0) { }
    void setSimulator(Simulator* simulator) { _simulator = simulator; }
    virtual void runTo(Tick tick) { _tick = tick; }
    virtual void maintain(Tick ticks) { _tick -= ticks; }
    virtual String save() const { return String(); }
    virtual ::Type persistenceType() const { return ::Type(); }
    String name() const { return _name; }
    virtual void load(const Value& value) { }
    virtual Value initial() const
    {
        return StructuredType::empty().convertTo(persistenceType());
    }
    virtual Rational cyclesPerSecond() const { return 0; }
    void setTicksPerCycle(Tick ticksPerCycle)
    {
        _ticksPerCycle = ticksPerCycle;
        _tick = 0;
    }
    void set(Identifier name, Value value)
    {
        if (name.name() == "*") {
            _name = value.value<String>();
            return;
        }
        if (Connector::Type(value.type()).valid()) {
            auto l = getValue(name).value<Connector*>();
            auto r = value.value<Connector*>();
            l->connect(r);
            r->connect(l);
            return;
        }
        Structure::set(name, value);
    }
    class Type : public ::Type
    {
    protected:
        Type(const Body* body) : ::Type(body) { }
        class Body : public ::Type::Body
        {
        public:
            Body(Simulator* simulator) : _simulator(simulator) { }
            ::Type member(Identifier i) const
            {
                if (i.name() == "*")
                    return StringType();
                return ::Type();
            }
            virtual Reference<Component> createComponent() const = 0;
            Value tryConvert(const Value& value, String* why) const
            {
                Value stv = value.type().tryConvertTo(
                    StructuredType::empty().type(), value, why);
                if (!stv.valid())
                    return stv;
                auto v = createComponent();
                _simulator->addComponent(v);
                return Value(type(), static_cast<Structure*>(&(*v)),
                    value.span());
            }
        protected:
            Simulator* _simulator;
        };
    };
protected:
    Tick _tick;

    SimulatorTemplate<T>* _simulator;
private:
    Tick _ticksPerCycle;
    String _name;
};

class ClockedComponent : public Component
{
public:
    void set(Identifier name, Value value)
    {
        if (name.name() == "frequency")
            _cyclesPerSecond = (second*value.value<Concrete>()).value();
        Component::set(name, value);
    }
    class Type : public Component::Type
    {
    protected:
        Type(const Body* body) : Component::Type(body) { }
        class Body : public Component::Type::Body
        {
        public:
            Body(Simulator* simulator) : Component::Type::Body(simulator) { }
            ::Type member(Identifier i) const
            {
                if (i.name() == "frequency")
                    return -second.type();
                return Component::Type::Body::member(i);
            }
        };
    };
private:
    Rational _cyclesPerSecond;
};

class Connector
{
public:
    class Type : public NamedNullary<::Type, Type>
    {
    public:
        Type() { }
        Type(const ::Type& t) : NamedNullary(t) { }
        bool compatible(Type other) const
        {
            return body()->compatible(other);
        }
        bool canConnectMultiple() const
        {
            return body()->canConnectMultiple();
        }
        Type(const Body* body) : NamedNullary(body) { }
        bool valid() const { return body() != 0; }
        static String name() { return "Connector"; }
        class Body : public NamedNullary::Body
        {
        public:
            virtual bool compatible(Type other) const { return false; }
            virtual bool canConnectMultiple() const { return false; }
            Value tryConvert(const Value& value, String* reason) const
            {
                Connector::Type t(value.type());
                if (!t.valid()) {
                    *reason = value.type().toString() + " is not a connector.";
                    return Value();
                }
                if (!compatible(t)) {
                    *reason = t.toString() + " and " + toString()
                        + " are not compatible connectors.";
                    return Value();
                }
                return Value(type(), value.value(), value.span());
            }
        };
    public:
        const Body* body() const { return as<Body>(); }
    };
    Value getValue() const
    {
        // The const_cast here is a result of the syntax
        // "componentA.connector1 = componentB.connector2;" in the config file.
        // In pure ALFE syntax, obtaining the "value" of connector2 does not
        // modify componentB, so getValue() is const. However, in Berapa,
        // componentB's state may be modified by this connection.
        return Value(type(), const_cast<Connector*>(this));
    }

    virtual Type type() const = 0;
    virtual void connect(Connector* other) = 0;
};

template<class T> class OutputConnector;
template<class T> class InputConnector;
template<class T> class BidirectionalConnector;

template<class T> class OutputConnector : public Connector
{
public:
    class Type : public NamedNullary<Connector::Type, Type>
    {
    public:
        class Body : public NamedNullary<Connector::Type, Type>::Body
        {
        public:
            bool compatible(Connector::Type other) const
            {
                return other == InputConnector<T>::Type() ||
                    other == BidirectionalConnector<T>::Type();
            }
        };
        static String name()
        {
            return "OutputConnector<" +
                typeFromCompileTimeType<T>().toString() + ">";
        }
    };
};

template<class T> class InputConnector : public Connector
{
public:
    virtual void setData(T v) = 0;
    class Type : public NamedNullary<Connector::Type, Type>
    {
    public:
        class Body : public NamedNullary<Connector::Type, Type>::Body
        {
        public:
            bool compatible(Connector::Type other) const
            {
                return other == OutputConnector<T>::Type() ||
                    other == BidirectionalConnector<T>::Type();
            }
        };
        static String name()
        {
            return "InputConnector<" +
                typeFromCompileTimeType<T>().toString() + ">";
        }
    };
};

template<class T> class BidirectionalConnector : public Connector
{
public:
    virtual void setData(T v) = 0;
    class Type : public NamedNullary<Connector::Type, Type>
    {
    public:
        class Body : public NamedNullary<Connector::Type, Type>::Body
        {
        public:
            bool compatible(Connector::Type other) const
            {
                return other == OutputConnector<T>::Type() ||
                    other == InputConnector<T>::Type() ||
                    other == BidirectionalConnector<T>::Type();
            }
        };
        static String name()
        {
            return "BidirectionalConnector<" +
                typeFromCompileTimeType<T>().toString() + ">";
        }
    };
};

template<class T> class AndComponent : public Component
{
public:
    class Type : public Component::Type
    {
    public:
        Type(Simulator* simulator) : Component::Type(new Body(simulator)) { }
    private:
        class Body : public Component::Type::Body
        {
        public:
            Body(Simulator* simulator) : Component::Type::Body(simulator) { }
            ::Type member(Identifier i) const
            {
                if (i.name() == "input1" || i.name() == "input2")
                    return InputConnector<T>::Type();
                if (i.name() == "output")
                    return OutputConnector<T>::Type();
                return Component::Type::Body::member(i);
            }
            Reference<Component> createComponent() const
            {
                return Reference<Component>::create<Intel8088CPU>();
            }
            String toString() const
            {
                return "And<" + typeFromCompileTimeType<T>().toString() + ">";
            }
        };
    };
private:
    InputConnector<T> _input1;
    InputConnector<T> _input2;
    OutputConnector<T> _output;
};

template<class T> class OrComponent : public Component
{
public:
    class Type : public Component::Type
    {
    public:
        Type(Simulator* simulator) : Component::Type(new Body(simulator)) { }
    private:
        class Body : public Component::Type::Body
        {
        public:
            Body(Simulator* simulator) : Component::Type::Body(simulator) { }
            ::Type member(Identifier i) const
            {
                if (i.name() == "input1" || i.name() == "input2")
                    return InputConnector<T>::Type();
                if (i.name() == "output")
                    return OutputConnector<T>::Type();
                return Component::Type::Body::member(i);
            }
            Reference<Component> createComponent() const
            {
                return Reference<Component>::create<Intel8088CPU>();
            }
            String toString() const
            {
                return "Or<" + typeFromCompileTimeType<T>().toString() + ">";
            }
        };
    };
private:                   
    InputConnector<T> _input1;
    InputConnector<T> _input2;
    OutputConnector<T> _output;
};

template<class T> class BucketComponent : public Component
{
public:
    InputConnector<T> _connector;
};

template<class T> class ConstantComponent : public Component
{
private:
    OutputConnector<T> _connector;
};

class AndConnectorFunco : public Nullary<Funco, AndConnectorFunco>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            //auto i = arguments.begin();
            //Concrete l = i->value<Concrete>();
            //++i;
            //return Value(l + i->value<Concrete>());
        }
        Identifier identifier() const { return OperatorAmpersand(); }
        bool argumentsMatch(List<Type> argumentTypes) const
        {
            if (argumentTypes.count() != 2)
                return false;
            auto i = argumentTypes.begin();
            //if ()
            //ConcreteType l(*i);
            //if (!l.valid())
            //    return false;
            //++i;
            //return ConcreteType(*i).valid();
        }
        FunctionTyco tyco() const
        {
            return FunctionTyco(Connector::Type(), Connector::Type(),
                Connector::Type());
        }
    };
};

class OrConnectorFunco : public Nullary<Funco, OrConnectorFunco>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
        }
        Identifier identifier() const { return OperatorBitwiseOr(); }
        //bool argumentsMatch(List<Type> argumentTypes) const
        //{
        //}
        FunctionTyco tyco() const
        {
            return FunctionTyco(Connector::Type(), Connector::Type(),
                Connector::Type());
        }
    };
};


template<class T> class SimulatorTemplate
{
public:
    SimulatorTemplate() : _halted(false), _ticksPerSecond(0) { }
    void simulate()
    {
        // Don't let any component get more than 20ms behind.
        Tick delta = (_ticksPerSecond / 50).value<int>();
        do {
            for (auto i : _components)
                i->runTo(delta);
            for (auto i : _components)
                i->maintain(delta);
        } while (!_halted);
    }
    String save() const
    {
        String s("{");
        bool needComma = false;
        for (auto i : _components) {
            if (i->name().empty())
                continue;
            if (needComma)
                s += ", ";
            needComma = true;
            s += i->name() + ": " + i->save();
        }
        s += "};";
        return s;
    }

    void halt() { _halted = true; }
    void addComponent(Reference<Component> c) { _components.add(c); }
    void load(String initialStateFile)
    {
        for (auto i : _components) {
            Rational cyclesPerSecond = i->cyclesPerSecond();
            if (cyclesPerSecond != 0)
                if (_ticksPerSecond == 0)
                    _ticksPerSecond = cyclesPerSecond;
                else
                    _ticksPerSecond = lcm(_ticksPerSecond, cyclesPerSecond);
        }
        if (_ticksPerSecond == 0)
            throw Exception("None of the components is clocked!");
        for (auto i : _components) {
            Rational cyclesPerSecond = i->cyclesPerSecond();
            if (cyclesPerSecond != 0) {
                Rational t = _ticksPerSecond / cyclesPerSecond;
                if (t.denominator != 1)
                    throw Exception("Scheduler LCM calculation incorrect");
                int ticksPerCycle = t.numerator;
                i->setTicksPerCycle(ticksPerCycle);
            }
            else
                i->setTicksPerCycle(0);
        }

        Value value;
        if (!initialStateFile.empty()) {
            ConfigFile initialState;
            initialState.addDefaultOption(name(), persistenceType(),
                initial());
            initialState.load(initialStateFile);
            value = initialState.getValue(name());
        }
        else
            value = initial();

        auto object = value.value<HashTable<Identifier, Value>>();
        for (auto i : _components)
            i->load(object[i->name()]);
    }
    String name() const { return "simulator"; }
    Rational ticksPerSecond() const { return _ticksPerSecond; }
private:
    Value initial() const
    {
        return StructuredType::empty().convertTo(persistenceType());
    }
    ::Type persistenceType() const
    {
        List<StructuredType::Member> members;
        for (auto i : _components) {
            Type type = i->persistenceType();
            if (!type.valid())
                continue;
            members.add(StructuredType::Member(i->name(), i->initial()));
        }
        return StructuredType("Simulator", members);
    }

    List<Reference<Component>> _components;
    bool _halted;
    Rational _ticksPerSecond;
};

#include "isa_8_bit_bus.h"
#include "nmi_switch.h"
#include "i8259pic.h"
#include "i8237dmac.h"
#include "pcxt_keyboard.h"
#include "pcxt_keyboard_port.h"
#include "i8255ppi.h"
#include "i8253pit.h"
#include "mc6845crtc.h"
#include "dram.h"
#include "ram.h"
#include "dma_page_registers.h"
#include "rom.h"
#include "i8088cpu.h"
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
        componentTypes.add(Intel8088CPU::Type(p));
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
        configFile.addType(String("Time"), second.type());

        for (auto i : componentTypes)
            configFile.addType(i.toString(), i);

        configFile.addDefaultOption("second", second);

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
