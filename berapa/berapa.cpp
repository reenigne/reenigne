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

template<class T> class Intel8237DMACTemplate;
typedef Intel8237DMACTemplate<void> Intel8237DMAC;

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
         
template<class T> class ConnectorTemplate;
typedef ConnectorTemplate<void> Connector;

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
    typedef IntegerType Type;
private:
    Base _t;
};

class ConcretePersistenceType : public Type
{
public:
    ConcretePersistenceType(Concrete unit) : Type(new Body(unit)) { }
private:
    class Body : public Type::Body
    {
    public:
        Body(Concrete unit) : _unit(unit) { }
        Concrete _unit;
        Value tryConvert(const Value& value, String* reason) const
        {
            return _unit.type().tryConvert(value, reason);
        }
        Value tryConvertTo(const Type& to, const Value& value, String* reason)
            const
        {
            return _unit.type().tryConvertTo(to, value, reason);
        }
        void deserialize(const Value& value, void* p) const
        {
            *static_cast<Rational*>(p) =
                (value.value<Concrete>()/_unit).value();
        }
        String toString() const { return _unit.type().toString(); }
    };
};

class HexPersistenceType : public IntegerType
{
public:
    HexPersistenceType(int digits) : IntegerType(new Body(digits)) { }
private:
    class Body : public IntegerType::Body
    {
    public:
        Body(int digits) : _digits(digits) { }

    private:
        int _digits;
    };
};

template<class T> class ConnectorTemplate
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
                // If assigning component=connector, connect to the default
                // connector on the component instead.
                Component::Type ct(value.type());
                if (ct.valid()) {
                    Structure* s = value.value<Structure*>();
                    Component* component = static_cast<Component*>(s);
                    Connector* connector = component->_defaultConnector;
                    if (connector != 0)
                        return tryConvert(connector->getValue(), reason);
                }

                Connector::Type t(value.type());
                if (!t.valid()) {
                    *reason = value.type().toString() + " is not a connector.";
                    return Value();
                }
                if (!compatible(t)) {
                    *reason = t.toString() + " and " + toString() +
                        " are not compatible connectors.";
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

template<class T> class ComponentTemplate : public Structure
{
public:
    ComponentTemplate()
      : _simulator(0), _tick(0), _ticksPerCycle(0), _defaultConnector(0)
    {
        persist("tick", &_tick, Tick(0), Tick::Type());
    }
    void setSimulator(Simulator* simulator) { _simulator = simulator; }
    virtual void runTo(Tick tick) { _tick = tick; }
    virtual void maintain(Tick ticks) { _tick -= ticks; }
    String name() const { return _name; }
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
        if (_config.hasKey(name)) {
            Member m = _config[name];
            m._type.deserialize(value, m._p);
            return;
        }
        Structure::set(name, value);
    }
    class Type : public ::Type
    {
    public:
        Type() { }
        Type(::Type type) : ::Type(type) { }
        Reference<Component> createComponent()
        {
            auto c = body()->createComponent();
            c->setType(*this);
            return c;
        }
        bool valid() const { return body() != 0; }
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
        const Body* body() const { return as<Body>(); }
    };
    template<class C> class TypeHelper : public Type
    {
    public:
        TypeHelper(::Type type) : Type(type) { }
        TypeHelper(Simulator* simulator) : Type(new Body(simulator)) { }
    protected:
        class Body : public Type::Body
        {
        public:
            Body(Simulator* simulator) : Type::Body(simulator)
            {
                C component;
                for (auto i = component._config.begin();
                    i != component._config.end(); ++i) {
                    _members[i.key()] = i.value()._type;
                }
            }
            Reference<Component> createComponent() const
            {
                return Reference<Component>::create<C>();
            }
            String toString() const { return C::typeName(); }
            ::Type member(Identifier i) const
            {
                if (_members.hasKey(i))
                    return _members[i];
                return Type::Body::member(i);
            }
        private:
            HashTable<Identifier, ::Type> _members;
        };
        TypeHelper(const Body* body) : Type(body) { }
    };
    virtual String save() const
    {
        String s("{\n");
        bool needComma = false;
        for (auto i = _persist.begin(); i != _persist.end(); ++i) {
            if (needComma)
                s += ",\n";
            needComma = true;
            ::Type type = i.value()._type;
            s += "  " + i.key() + ": " +
                type.serialize(i.value()._p);
        }
        return s + "}\n";
    }
    virtual ::Type persistenceType() const
    {
        List<StructuredType::Member> members;
        for (auto i = _persist.begin(); i != _persist.end(); ++i) {
            members.add(StructuredType::Member(
                i.key(), Value(i.value()._type, i.value()._initial)));
        }
        return StructuredType(_type.toString(), members);
    }
    virtual void load(const Value& value)
    {
        auto members = value.value<HashTable<Identifier, Value>>();
        for (auto i = _persist.begin(); i != _persist.end(); ++i)
            i.value()._type.deserialize(members[i.key()], i.value()._p);
    }

    void setType(Type type) { _type = type; }

protected:
    void connector(String name, Connector* p)
    {
        if (name == "") {
            _defaultConnector = p;
            Structure::set(Identifier(OperatorAssignment()),
                AssignmentFunco(this));
        }
        else {
            config(name, p, p->type());
            Structure::set(name, p->getValue());
        }
    }
    template<class C> void config(String name, C* p,
        ::Type type = typeFromCompileTimeType<C>())
    {
        _config.add(name, Member(type, static_cast<void*>(p)));
    }                                           
    template<class C> void persist(String name, C* p, C initial,
        ::Type type = typeFromCompileTimeType<C>())
    {
        _persist.add(name, Member(type, static_cast<void*>(p), initial));
    }
    Tick _tick;

private:
    class AssignmentFunco : public Funco
    {
    public:
        AssignmentFunction(Component* component)
          : Funco(new Body(component)) { }
        class Body : public Funco::Body
        {
        public:
            Body(Component* component) : _component(component) { }
            Value evaluate(List<Value> arguments, Span span) const
            {
                auto i = arguments.begin();
                auto l = i->value<Component*>()->_defaultConnector;
                auto lt = l->getValue().type();
                ++i;
                String reason;
                if (Type(i->type()).valid()) {
                    auto r = i->value<Component*>()->_defaultConnector;
                    if (!lt.tryConvert(r->getValue(), &reason).valid())
                        span.throwError(reason);
                    l->connect(r);
                    r->connect(l);
                    return;
                }
                auto r = i->value<Connector*>();
                if (!lt.tryConvert(r->getValue(), &reason).valid())
                    span.throwError(reason);
                l->connect(r);
                r->connect(l);
                return Value();
            }
            Identifier identifier() const { return OperatorAssignment(); }
            bool argumentsMatch(List<Type> argumentTypes) const
            {
                if (argumentTypes.count() != 2)
                    return false;
                auto i = argumentTypes.begin();
                if (!Type(*i).valid())
                    return false;
                ++i;
                if (Type(*i).valid())
                    return true;
                return Connector::Type(*i).valid();
            }
            FunctionTyco tyco() const
            {
                return FunctionTyco(VoidType(),
                    PointerType(Type()), PointerType(Connector::Type()));
            }
        private:
            Component* _component;
        };
    };
    class Member
    {
    public:
        Member() { }
        Member(::Type type, void* p, Any initial = Any())
            : _type(type), _p(p), _initial(initial) { }
        ::Type _type;
        void* _p;
        Any _initial;
    };
    HashTable<String, Member> _config;
    HashTable<String, Member> _persist;

    SimulatorTemplate<T>* _simulator;
    Tick _ticksPerCycle;
    String _name;
    Type _type;
    Connector* _defaultConnector;

    friend class Connector::Type::Body;
    friend class AssignmentFunco::Body
};

class ClockedComponent : public Component
{
public:
    ClockedComponent()
    {
        config("frequency", &_cyclesPerSecond, 
            ConcretePersistenceType(1/second));
    }
    Rational cyclesPerSecond() const { return _cyclesPerSecond; }
    template<class C> using Type = Component::TypeHelper<C>;
private:
    Rational _cyclesPerSecond;
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
        static String parameter()
        {
            return "<" + typeFromCompileTimeType<T>().toString() + ">";
        }
        static String name() { return "BidirectionalConnector" + parameter(); }
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
        };
        static String name()
        {
            return "OutputConnector" +
                BidirectionalConnector::Type::parameter();
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
            return "InputConnector" +
                BidirectionalConnector::Type::parameter();
        }
    };
};

template<class T, class C> class ParametricComponentType
  : public Component::TypeHelper<C>
{
protected:
    ParametricComponentType(Body* body) : Component::TypeHelper<C>(body) { }
    class Body : public Component::TypeHelper<C>::Body
    {
    public:
        Body(Simulator* simulator)
          : Component::TypeHelper<C>::Body(simulator) { }
        static String parameter()
        {
            return "<" + typeFromCompileTimeType<T>().toString() + ">";
        }
    };
};

template<class T, class C> class BooleanComponent : public Component
{
public:
    BooleanComponent() : _input1(this), _input2(this), _output(this)
    { 
        connector("input1", &_input1);
        connector("input2", &_input2);
        connector("output", &_output);
    }
    class Type : public ParametricComponentType<T, C>
    {
    public:
        Type(Body* body) : ParametricComponentType(body) { }
    protected:
        class Body : public ParametricComponentType::Body
        {
        public:
            Body(Simulator* simulator)
              : ParametricComponentType::Body(simulator) { }
        };
    };
    virtual void update(Tick t) = 0;
private:
    class InputConnector : public ::InputConnector<T>
    {
    public:
        InputConnector(BooleanComponent *component) : _component(component) { }
        void connect(::Connector* other) { _other = other; }
        void setData(Tick t, T v) { _v = v; _component->update(t); }
        ::Connector* _other;
        T _v;
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
        void set(Tick t, T v)
        {
            if (v != _v) {
                _v = v;
                _other->setData(t, v);
            }
        }
        BidirectionalConnector<T>* _other;
        T _v;
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
    static String typeName() { return "And"; }
    void update(Tick t) { _output.set(t, _input1._v & _input2._v); }
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
            String toString() const { return "And" + parameter(); }
        };
    };
};

template<class T> class OrComponent
  : public BooleanComponent<T, OrComponent<T>>
{
public:
    static String typeName() { return "Or"; }
    void update(Tick t) { _output.set(t, _input1._v | _input2._v); }
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
            String toString() const { return "Or" + parameter(); }
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
        componentTypes.add(Intel8237DMAC::Type(p));
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
