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

template<class T> class PCXTKeyboardTemplate;
typedef PCXTKeyboardTemplate<void> PCXTKeyboard;

template<class T> class PCXTKeyboardPortTemplate;
typedef PCXTKeyboardPortTemplate<void> PCXTKeyboardPort;

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
    class TypeBase : public ::Type
    {
    public:
        Reference<Component> createComponent()
        {
            return body()->createComponent();
        }
    protected:
        TypeBase(const Body* body) : ::Type(body) { }
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
        const Body* body() { return as<Body>(); }
    };
    template<class C> class Type : public TypeBase
    {
    protected:
        Type(const Body* body) : TypeBase(body) { }
        class Body : public TypeBase::Body
        {
        public:
            Body(Simulator* simulator) : TypeBase::Body(simulator) { }
            Reference<Component> createComponent() const
            {
                return Reference<Component>::create<C>();
            }
        };
    };
    String save()
    {
        String s("{\n");
        bool needComma = false;
        for (auto i = _persist.begin(); i != _persist.end(); ++i) {
            if (needComma)
                s += ",\n";
            needComma = true;
            ::Type type = i.value().type();
            s += "  " + i.key() + ": " +
                type.toString(i.value().value<void*>());
        }
        return s + "}\n";
    }

protected:
    template<class C> void config(String name, C* p,
        ::Type type = typeFromCompileTimeType<C>())
    {
        _members.add(name, Value(type, static_cast<void*>(p)));
    }                                           
    template<class C> void persist(String name, C* p, C initial,
        ::Type type = typeFromCompileTimeType<C>())
    {
        *p = initial;
        _persist.add(name, Value(type, static_cast<void*>(p)));
    }
    HashTable<String, Value> _config;
    HashTable<String, Value> _persist;

    Tick _tick;

    SimulatorTemplate<T>* _simulator;
private:
    Tick _ticksPerCycle;
    String _name;
};

template<class C> class ClockedComponent : public Component
{
public:
    void set(Identifier name, Value value)
    {
        if (name.name() == "frequency")
            _cyclesPerSecond = (second*value.value<Concrete>()).value();
        Component::set(name, value);
    }
    class Type : public Component::Type<C>
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

class BidirectionalConnectorBase : public Connector
{
public:
    class Type : public Connector::Type
    {
    public:
        Type(const ::Type& t) : Connector::Type(t) { }
        bool valid() const { return body() != 0; }
        ::Type transportType() const { return body()->transportType(); }
    protected:
        class Body
        {
        public:
            virtual ::Type transportType() const = 0;
        };
        const Body* body() const { return as<Body>(); }
    };
};

template<class T> class BidirectionalConnector
    : public BidirectionalConnectorBase
{
public:
    Connector::Type type() const { return Type(); };
    virtual void setData(Tick t, T v) = 0;
    void connect(::Connector* other)
    {
        _other = dynamic_cast<BidirectionalConnector<T>*>(other);
    }
    class Type : public NamedNullary<Connector::Type, Type>
    {
    public:
        Type() { }
        Type(const ::Type& t) : NamedNullary(t) { }
        class Body : public NamedNullary<Connector::Type, Type>::Body
        {
        public:
            bool compatible(Connector::Type other) const
            {
                return other == OutputConnector<T>::Type() ||
                    other == InputConnector<T>::Type() ||
                    other == BidirectionalConnector<T>::Type();
            }
            ::Type transportType() const
            {
                return typeFromCompileTimeType<T>();
            }
        };
        static String name()
        {
            return "BidirectionalConnector<" +
                typeFromCompileTimeType<T>().toString() + ">";
        }
    };
    BidirectionalConnector<T>* _other;
};

template<class T> class OutputConnector : public BidirectionalConnector<T>
{
public:
    Connector::Type type() const { return Type(); };
    void setData(Tick t, T v) { }
    class Type : public NamedNullary<BidirectionalConnector<T>::Type, Type>
    {
    public:
        class Body
          : public NamedNullary<BidirectionalConnector<T>::Type, Type>::Body
        {
        public:
            bool compatible(Connector::Type other) const
            {
                return other == InputConnector<T>::Type() ||
                    other == BidirectionalConnector<T>::Type();
            }
            ::Type transportType() const
            {
                return typeFromCompileTimeType<T>();
            }
        };
        static String name()
        {
            return "OutputConnector<" +
                typeFromCompileTimeType<T>().toString() + ">";
        }
    };
};

template<class T> class InputConnector : public BidirectionalConnector<T>
{
public:
    Connector::Type type() const { return Type(); };
    virtual void setData(Tick t, T v) = 0;
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

template<class T, class C> class ParametricComponentType
  : public Component<C>::Type
{
protected:
    class Body : public Component::Type::Body
    {
    public:
        String parameter() const
        {
            return "<" + typeFromCompileTimeType<T>().toString() + ">";
        }
    };
};

template<class T, class C> class BooleanComponent : public Component
{
public:
    BooleanComponent() : _input1(this), _input2(this), _output(this) { }
    void config()
    {
        config("input1", &_input1);
        config("input2", &_input2);
        config("output", &_output);
    }

    Value getValue(Identifier i) const
    {
        String n = i.name();
        if (n == "input1")
            return _input1.getValue();
        if (n == "input2")
            return _input2.getValue();
        if (n == "output")
            return _output.getValue();
        return Component::getValue(i);
    }
    class Type : public ParametricComponentType<T, C>
    {
    public:
        Type(Body* body) : Component::Type(body) { }
    protected:
        class Body : public Component::Type::Body
        {
        public:
            Body(Simulator* simulator) : Component::Type::Body(simulator) { }
            ::Type member(Identifier i) const
            {
                if (i.name() == "input1" || i.name() == "input2")
                    return ::InputConnector<T>::Type();
                if (i.name() == "output")
                    return ::OutputConnector<T>::Type();
                return Component::Type::Body::member(i);
            }
        };
    };
    virtual void update() = 0;
private:
    class InputConnector : public ::InputConnector<T>
    {
    public:
        InputConnector(BooleanComponent *component) : _component(component) { }
        void connect(::Connector* other) { _other = other; }
        void setData(Tick tick, T t) { _t = t; _component->update(); }
        ::Connector* _other;
        T _t;
        BooleanComponent* _component;
    };
    class OutputConnector : public ::OutputConnector<T>
    {
    public:
        OutputConnector(BooleanComponent *component)
          : _component(component) { }
        void connect(::Connector* other)
        {
            _other = dynamic_cast<BidirectionalConnector<T>*>(other);
        }
        void set(Tick tick, T v)
        {
            if (v != _t) {
                _t = v;
                _other->setData(tick, v);
            }
        }
        BidirectionalConnector<T>* _other;
        T _t;
        BooleanComponent* _component;
    };
protected:
    InputConnector _input1;
    InputConnector _input2;
    OutputConnector _output;
};

template<class T> class AndComponent
  : public BooleanComponent<T, AndComponent<T>>
{
public:
    void update() { _output.set(_input1._t & _input2._t); }
    class Type : public BooleanComponent::Type
    {
    public:
        Type(Simulator* simulator)
          : BooleanComponent::Type(new Body(simulator)) { }
    private:
        class Body : public BooleanComponent::Type::Body
        {
        public:
            Body(Simulator* simulator)
              : BooleanComponent::Type::Body(simulator) { }
            String toString() const { return "And" + parameters(); }
        };
    };
};

template<class T> class OrComponent
  : public BooleanComponent<T, OrComponent<T>>
{
public:
    void update() { _output.set(_input1._t | _input2._t); }
    class Type : public BooleanComponent::Type
    {
    public:
        Type(Simulator* simulator)
          : BooleanComponent::Type(new Body(simulator)) { }
    private:
        class Body : public BooleanComponent::Type::Body
        {
        public:
            Body(Simulator* simulator)
              : BooleanComponent::Type::Body(simulator) { }
            String toString() const { return "Or" + parameters(); }
        };
    };
};

template<class T> class BucketComponent : public Component
{
private:
    class Type : public Component::Type
    {
    public:
        Type(Simulator* simulator) : Component::Type(simulator) { }
    private:
        class Body : public Component::Type::Body
        {
        public:
            String toString() const { return "Sink" + }
        };
    };
    class Connector : public InputConnector<T>
    {
    public:
        void setData(Tick tick, T t) { }
    };
    Connector _connector;
};

template<class T> class ConstantComponent : public Component
{
private:
    ConstantComponent(T v) : _v(v) { }
    T _v;
    OutputConnector<T> _connector;
};

template<template<class> class Component> class ComponentFunco : public Funco
{                      
public:
    ComponentFunco(Body* body) : Funco(body) { }
    class Body : public Funco::Body
    {
    public:
        Body(Simulator* simulator) : _simulator(simulator) { }
        Identifier identifier() const { return OperatorAmpersand(); }
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            Type t =
                BidirectionalConnectorBase::Type(i->type()). transportType();
            auto l = *i;
            ++i;
            auto r = *i;
            Reference<::Component> c;
            if (t == ByteType())
                c = Component<Byte>::Type(_simulator).createComponent();
            else
                if (t == BooleanType())
                    c = Component<bool>::Type(_simulator).createComponent();
            c->set("input1", l);
            c->set("input2", r);
            return c->getValue("output");
        }
        bool argumentsMatch(List<Type> argumentTypes) const
        {
            if (argumentTypes.count() != 2)
                return false;
            auto i = argumentTypes.begin();
            BidirectionalConnectorBase::Type l(*i);
            if (!l.valid())
                return false;
            Type lTransport = l.transportType();
            ++i;
            BidirectionalConnectorBase::Type r(*i);
            if (!r.valid())
                return false;
            return lTransport == r.transportType();
        }
        FunctionTyco tyco() const
        {
            return FunctionTyco(Connector::Type(), Connector::Type(),
                Connector::Type());
        }
    private:
        Simulator* _simulator;
    };
};

class AndComponentFunco : public ComponentFunco<AndComponent>
{
public:
    AndComponentFunco(Simulator* simulator)
      : ComponentFunco(new Body(simulator)) { }
    class Body : public ComponentFunco::Body
    {
    public:
        Body(Simulator* simulator) : ComponentFunco::Body(simulator) { }
        Identifier identifier() const { return OperatorAmpersand(); }
    };
};

class OrComponentFunco : public ComponentFunco<OrComponent>
{
public:
    OrComponentFunco(Simulator* simulator)
      : ComponentFunco(new Body(simulator)) { }
    class Body : public ComponentFunco::Body
    {
    public:
        Body(Simulator* simulator) : ComponentFunco::Body(simulator) { }
        Identifier identifier() const { return OperatorBitwiseOr(); }
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
        configFile.addFunco(AndComponentFunco(p));
        configFile.addFunco(OrComponentFunco(p));

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
