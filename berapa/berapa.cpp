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

template<class T> class ISA8BitRAMTemplate;
typedef ISA8BitRAMTemplate<void> ISA8BitRAM;

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

template<class T> class RGBIMonitorTemplate;
typedef RGBIMonitorTemplate<void> RGBIMonitor;

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
template<> Type typeFromCompileTimeType<Tick>() { return Tick::Type(); }

class HexPersistenceType : public IntegerType
{
public:
    HexPersistenceType(int digits) : IntegerType(create<Body>(digits)) { }
private:
    class Body : public IntegerType::Body
    {
    public:
        Body(int digits) : _digits(digits) { }
        String serialize(void* p, int width, int used, int indent, int delta)
            const
        {
            return hex(*static_cast<int*>(p), _digits);
        }
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
        Type(const ConstHandle& other) : NamedNullary<::Type, Type>(other) { }
        bool compatible(Type other) const
        {
            return body()->compatible(other);
        }
        bool canConnectMultiple() const
        {
            return body()->canConnectMultiple();
        }
        bool valid() const { return body() != 0; }
        static String name() { return "Connector"; }
        class Body : public NamedNullary<::Type, Type>::Body
        {
        public:
            virtual bool compatible(Type other) const { return false; }
            virtual bool canConnectMultiple() const { return false; }
            virtual bool canConvertFrom(const ::Type& other, String* reason)
                const
            {
                // If assigning component=connector, connect to the default
                // connector on the component instead.
                typename ComponentTemplate<T>::Type ct(other);
                if (ct.valid()) {
                    auto dct = ct.defaultConnectorType();
                    if (dct.valid())
                        return canConvertFrom(dct, reason);
                }
                Connector::Type t(other);
                if (!t.valid()) {
                    *reason = other.toString() + " is not a connector.";
                    return false;
                }
                if (!compatible(t)) {
                    *reason = t.toString() + " and " + this->toString() +
                        " are not compatible connectors.";
                    return false;
                }
                return true;
            }
            virtual ValueTemplate<T> convert(const ValueTemplate<T>& value)
                const
            {
                // If assigning component=connector, connect to the default
                // connector on the component instead.
                typename ComponentTemplate<T>::Type ct(value.type());
                if (ct.valid()) {
                    Structure* s = value.value<Structure*>();
                    auto component = static_cast<ComponentTemplate<T>*>(s);
                    Connector* connector = component->_defaultConnector;
                    return convert(connector->getValue());
                }
                return Value(type(), value.value(), value.span());
            }
        };
    public:
        const Body* body() const { return this->template as<Body>(); }
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
    class Type : public ::Type
    {
    public:
        Type() { }
        Type(::Type type) : ::Type(type) { }
        Reference<Component> createComponent()
        {
            auto c = body()->createComponent();
            body()->simulator()->addComponent(c);
            return c;
        }
        Simulator* simulator() const { return body()->simulator(); }
        bool valid() const { return body() != 0; }
    protected:
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
            bool canConvert(const ::Type& other, String* why) const
            {
                return other.canConvertTo(StructuredType::empty().type(), why);
            }
            Value convert(const Value& value) const
            {
                auto v = Type(type()).createComponent();
                return Value(type(), static_cast<Structure*>(&(*v)),
                    value.span());
            }
            Simulator* simulator() const { return _simulator; }
        protected:
            Simulator* _simulator;
        };
        const Body* body() const { return as<Body>(); }
    };
    ComponentTemplate(Type type)
      : _type(type), _simulator(type.simulator()), _ticksPerCycle(0),
        _defaultConnector(0)
    {
        persist("tick", &_tick);
    }
    virtual void initialize() { }
    virtual void runTo(Tick tick) { _tick = tick; }
    virtual void maintain(Tick ticks) { _tick -= ticks; }
    String name() const { return _name; }
    virtual Rational cyclesPerSecond() const { return 0; }
    void setTicksPerCycle(Tick ticksPerCycle)
    {
        _ticksPerCycle = ticksPerCycle;
        _tick = 0;
    }
    void set(Identifier name, Value value)
    {
        String n = name.name();
        if (n == "*") {
            _name = value.value<String>();
            return;
        }
        if (Connector::Type(value.type()).valid()) {
            auto l = getValue(name).template value<Connector*>();
            auto r = value.value<Connector*>();
            l->connect(r);
            r->connect(l);
            return;
        }
        if (_config.hasKey(n)) {
            Member m = _config[n];
            m.type().deserialize(value, m._p);
            return;
        }
        Structure::set(name, value);
    }
    template<class C> class TypeHelper : public Type
    {
    public:
        TypeHelper(Simulator* simulator)
          : Type(TypeHelper::template create<Body>(simulator)) { }
        TypeHelper(const ConstHandle& other) : Type(other) { }
    protected:
        class Body : public Type::Body
        {
        public:
            Body(Simulator* simulator) : Type::Body(simulator)
            {
                C component(type());
                for (auto i = component._config.begin();
                    i != component._config.end(); ++i) {
                    _members[i.key()] = i.value().type();
                }
                _default = component.persistenceType();
            }
            Reference<Component> createComponent() const
            {
                return Reference<Component>::create<C>(Type(type()));
            }
            String toString() const { return C::typeName(); }
            ::Type member(Identifier i) const
            {
                if (_members.hasKey(i))
                    return _members[i];
                return Type::Body::member(i);
            }
            String serialize(void* p) const
            {
                return static_cast<C*>(p)->save();
            }
            void deserialize(const Value& value, void* p) const
            {
                static_cast<C*>(p)->load(value);
            }
            int size() const { return sizeof(C); }
            Value defaultValue() const { return _default; }
            Value value(void* p) const { return static_cast<C*>(p)->value(); }
        private:
            HashTable<Identifier, ::Type> _members;
            Value _default;
        };
    };
    virtual String save(int width, int used, int indent, int delta) const
    {
        // First try putting everything on one line
        String s("{ ");
        bool needComma = false;
        bool separate = false;
        used += 5;
        for (auto i = _persist.begin(); i != _persist.end(); ++i) {
            if (used > width) {
                separate = true;
                break;
            }
            Member m = i.value();
            int u = used + i.key().length() + 2 + (needComma ? 2 : 0);
            String v = "{ }";
            ::Type t = m.type();
            if (t.value(m._p) != Value(t)) {
                v = t.serialize(m._p, width, u, 0, 0);
                if (v == "*") {
                    separate = true;
                    s = "";
                    break;
                }
            }
            if (v != "{ }") {
                if (needComma)
                    s += ", ";
                needComma = true;
                s += i.key() + ": " + v;
                used = u + v.length();
            }
        }
        if (s == "{ ")
            return "{ }";
        if (!separate && used <= width)
            return s + " }";
        else
            s = "";
        if (s == "" && indent == 0)
            return "*";
        // It doesn't all fit on one line, put each member on a separate line.
        s = "{\n";
        needComma = false;
        for (auto i = _persist.begin(); i != _persist.end(); ++i) {
            Member m = i.value();
            int u = indent + i.key().length();
            String v = "{ }";
            ::Type t = m.type();
            if (t.value(m._p) != Value(t))
                v = t.serialize(m._p, width, u, indent + delta, delta);
            if (v != "{ }") {
                if (needComma)
                    s += ",\n";
                needComma = true;
                s += String(" ")*indent + i.key() + ": " + v;
            }
        }
        return s + " }";
    }
    virtual ::Type persistenceType() const
    {
        List<StructuredType::Member> members;
        for (auto i = _persist.begin(); i != _persist.end(); ++i)
            members.add(StructuredType::Member(i.key(), i.value()._initial));
        return StructuredType(_type.toString(), members);
    }
    virtual void load(const Value& value)
    {
        auto members = value.value<HashTable<Identifier, Value>>();
        for (auto i = _persist.begin(); i != _persist.end(); ++i) {
            Member m = i.value();
            m.type().deserialize(members[i.key()], m._p);
        }
    }
    Value value() const
    {
        HashTable<Identifier, Value> h;
        for (auto i = _persist.begin(); i != _persist.end(); ++i) {
            Member m = i.value();
            h.add(i.key(), m.type().value(m._p));
        }
        return Value(persistenceType(), h);
    }
    Type type() const { return _type; }

protected:
    void connector(String name, Connector* p)
    {
        if (name == "") {
            _defaultConnector = p;
            Structure::set(Identifier(OperatorAssignment()),
                AssignmentFunco(this));
        }
        else {
            // Connectors don't have a default value. If they did, a connector
            // supporting multiple connections would always be connected to the
            // default. We can't convert directly from the type to the value,
            // or the conversion would call convert() which would attempt to
            // connect.
            _config.add(name,
                Member(static_cast<void*>(p), Value(p->type(), 0)));
            Structure::set(name, p->getValue());
        }
    }
    template<class C> void config(String name, C* p,
        ::Type type = typeFromCompileTimeType<C>())
    {
        _config.add(name, Member(static_cast<void*>(p), type));
        Structure::set(name, type);
        set(name, type);
    }
    template<class C> void persist(String name, C* p, C initial,
        ::Type type = typeFromCompileTimeType<C>())
    {
        Value v(type, initial);
        ArrayType arrayType(type);
        if (arrayType.valid()) {
            Value v(arrayType.contained(), initial);
            List<Value> initial;
            LessThanType l(arrayType.indexer());
            if (!l.valid()) {
                throw Exception(
                    "Don't know how many elements to put in default.");
            }
            int n = l.n();
            for (int i = 0; i < n; ++i)
                initial.add(v);
            v = Value(type, initial);
        }
        _persist.add(name, Member(static_cast<void*>(p), v));

    }
    template<class C> void persist(String name, C* p,
        Value initial = Value(typeFromCompileTimeType<C>()))
    {
        _persist.add(name, Member(static_cast<void*>(p), initial));
    }
    Tick _tick;

protected:
    SimulatorTemplate<T>* _simulator;
private:
    class AssignmentFunco : public Funco
    {
    public:
        AssignmentFunco(Component* component)
          : Funco(create<Body>(component)) { }
        ::Type type() const { return tyco(); }
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
                    if (!lt.canConvert(r->getValue().type(), &reason))
                        span.throwError(reason);
                    l->connect(r);
                    r->connect(l);
                    return Value();
                }
                auto r = i->value<Connector*>();
                if (!lt.canConvert(r->getValue().type(), &reason))
                    span.throwError(reason);
                l->connect(r);
                r->connect(l);
                return Value();
            }
            Identifier identifier() const { return OperatorAssignment(); }
            bool argumentsMatch(List<::Type> argumentTypes) const
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
                    PointerType(_component->type()),
                    PointerType(Connector::Type()));
            }
        private:
            Component* _component;
        };
    };
    class Member
    {
    public:
        Member() { }
        Member(void* p, Value initial) : _p(p), _initial(initial) { }
        ::Type type() const { return _initial.type(); }
        void* _p;
        Value _initial;
    };
    HashTable<String, Member> _config;
    HashTable<String, Member> _persist;

    Tick _ticksPerCycle;
    String _name;
    Type _type;
    Connector* _defaultConnector;

    friend class Connector::Type::Body;
    friend class AssignmentFunco::Body;
};

class ClockedComponent : public Component
{
public:
    ClockedComponent(Type type) : Component(type)
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
        Type(const ConstHandle& other) : Connector::Type(other) { }
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
        Type(const ConstHandle& other)
          : NamedNullary<Connector::Type, Type> (other) { }
        class Body : public NamedNullary<Connector::Type, Type>::Body
        {
        public:
            bool compatible(Connector::Type other) const
            {
                return other == typename OutputConnector<T>::Type() ||
                    other == typename InputConnector<T>::Type() ||
                    other == typename BidirectionalConnector<T>::Type();
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
    class Type
      : public NamedNullary<typename BidirectionalConnector<T>::Type, Type>
    {
    public:
        class Body : public
            NamedNullary<typename BidirectionalConnector<T>::Type, Type>::Body
        {
        public:
            bool compatible(Connector::Type other) const
            {
                return other == typename InputConnector<T>::Type() ||
                    other == typename BidirectionalConnector<T>::Type();
            }
        };
        static String name()
        {
            return "OutputConnector" +
                BidirectionalConnector<T>::Type::parameter();
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
                return other == typename OutputConnector<T>::Type() ||
                    other == typename BidirectionalConnector<T>::Type();
            }
        };
        static String name()
        {
            return "InputConnector" +
                BidirectionalConnector<T>::Type::parameter();
        }
    };
};

template<class T, class C> class ParametricComponentType
  : public Component::TypeHelper<C>
{
public:
    ParametricComponentType(const ConstHandle& other)
      : Component::TypeHelper<C>(other) { }
protected:
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
    BooleanComponent(Component::Type type)
      : Component(type), _input1(this), _input2(this), _output(this)
    {
        connector("input1", &_input1);
        connector("input2", &_input2);
        connector("output", &_output);
    }
    class Type : public ParametricComponentType<T, C>
    {
    public:
        Type(const ConstHandle& other)
          : ParametricComponentType<T, C>(other) { }
    protected:
        class Body : public ParametricComponentType<T, C>::Body
        {
        public:
            Body(Simulator* simulator)
              : ParametricComponentType<T, C>::Body(simulator) { }
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
    AndComponent(Component::Type type)
      : BooleanComponent<T, AndComponent<T>>(type) { }
    void update(Tick t)
    {
        this->_output.set(t, this->_input1._v & this->_input2._v);
    }
    class Type : public BooleanComponent<T, AndComponent<T>>::Type
    {
    public:
        Type(Simulator* simulator)
          : BooleanComponent<T, AndComponent<T>>::Type(
                Type::template create<Body>(simulator)) { }
    private:
        class Body : public BooleanComponent<T, AndComponent<T>>::Type::Body
        {
        public:
            Body(Simulator* simulator)
              : BooleanComponent<T, AndComponent<T>>::Type::Body(simulator) { }
            String toString() const { return "And" + this->parameter(); }
        };
    };
};

template<class T> class OrComponent
  : public BooleanComponent<T, OrComponent<T>>
{
public:
    static String typeName() { return "Or"; }
    OrComponent(Component::Type type)
      : BooleanComponent<T, OrComponent<T>>(type) { }
    void update(Tick t)
    {
        this->_output.set(t, this->_input1._v | this->_input2._v);
    }
    class Type : public BooleanComponent<T, OrComponent<T>>::Type
    {
    public:
        Type(Simulator* simulator)
          : BooleanComponent<T, OrComponent<T>>::Type(
                Type::template create<Body>(simulator)) { }
    private:
        class Body : public BooleanComponent<T, OrComponent<T>>::Type::Body
        {
        public:
            Body(Simulator* simulator)
              : BooleanComponent<T, OrComponent<T>>::Type::Body(simulator) { }
            String toString() const { return "Or" + this->parameter(); }
        };
    };
};

template<class T> class BucketComponent : public Component
{
private:
    class Type : public ParametricComponentType<T, BucketComponent<T>>
    {
    public:
        Type(Simulator* simulator) : Component::Type(simulator) { }
    private:
        class Body : public Component::Type::Body
        {
        public:
            String toString() const { return "Sink" + this->parameter(); }
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
    void load() { _connector->setData(0, _v); }
    T _v;
    OutputConnector<T> _connector;
};

// SRLatch works like a NAND latch with inverters on the inputs.
class SRLatch : public Component
{
public:
    static String typeName() { return "SRLatch"; }
    SRLatch(Component::Type type)
      : Component(type), _set(this), _reset(this), _isSet(false)
    {
        connector("set", &_set);
        connector("reset", &_reset);
        connector("lastSet", &_lastSet);
        connector("lastReset", &_lastReset);
    }
    class Type : public Component::TypeHelper<SRLatch>
    {
    public:
        Type(Simulator* simulator)
          : Component::TypeHelper<SRLatch>(simulator) { }
    private:
        class Body : public Component::Type::Body
        {
        public:
            Body(Simulator* simulator) : Component::Type::Body(simulator) { }
        };
    };
    void doSet(Tick t, bool v) { }
    void doReset(Tick t, bool v) { }
private:
    class SetConnector : public InputConnector<bool>
    {
    public:
        SetConnector(SRLatch* latch) : _latch(latch) { }
        void setData(Tick t, bool v) { _latch->doSet(t, v); }
    private:
        SRLatch* _latch;
    };
    class ResetConnector : public InputConnector<bool>
    {
    public:
        ResetConnector(SRLatch* latch) : _latch(latch) { }
        void setData(Tick t, bool v) { _latch->doReset(t, v); }
    private:
        SRLatch* _latch;
    };

    SetConnector _set;
    ResetConnector _reset;
    OutputConnector<bool> _lastSet;
    OutputConnector<bool> _lastReset;
    bool _isSet;
};

template<template<class> class Component> class ComponentFunco : public Funco
{
public:
    ComponentFunco(const Handle& other) : Funco(other) { }
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
            if (t == ByteType()) {
                c = typename Component<Byte>::Type(_simulator).
                    createComponent();
        }
            else
                if (t == BooleanType()) {
                    c = typename Component<bool>::Type(_simulator).
                        createComponent();
        }
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
      : ComponentFunco(create<Body>(simulator)) { }
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
      : ComponentFunco(create<Body>(simulator)) { }
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
    SimulatorTemplate(Directory directory)
      : _halted(false), _ticksPerSecond(0), _directory(directory) { }
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
            String l = "    " + i->name() + ": ";
            String v = i->save(79, l.length(), 4, 4);
            if (v != "") {
                if (needComma)
                    s += ",\n";
                needComma = true;
                s += l + v;
            }
        }
        return s + "\n};\n";
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
            for (auto i : _components)
                initialState.addType(i->persistenceType());
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
    Directory directory() const { return _directory; }
private:
    Value initial() const { return persistenceType(); }
    ::Type persistenceType() const
    {
        List<StructuredType::Member> members;
        for (auto i : _components) {
            Type type = i->persistenceType();
            if (!type.valid())
                continue;
            members.add(StructuredType::Member(i->name(), Value(i->type())));
        }
        return StructuredType("Simulator", members);
    }

    Directory _directory;
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
#include "ram.h"
#include "isa_8_bit_ram.h"
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

        File configPath(_arguments[1], CurrentDirectory(), true);

        Simulator simulator(configPath.parent());
        Simulator* p = &simulator;

        List<Component::Type> componentTypes;
        componentTypes.add(Intel8088CPU::Type(p));
        componentTypes.add(ISA8BitBus::Type(p));
        componentTypes.add(ISA8BitRAM::Type(p));
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
        componentTypes.add(SRLatch::Type(p));

        ConfigFile configFile;
        configFile.addDefaultOption("stopSaveState", StringType(), String(""));
        configFile.addDefaultOption("initialState", StringType(), String(""));
        configFile.addType(second.type(), TycoIdentifier("Time"));
        configFile.addFunco(AndComponentFunco(p));
        configFile.addFunco(OrComponentFunco(p));

        for (auto i : componentTypes)
            configFile.addType(i);

        configFile.addDefaultOption("second", second);

        configFile.load(configPath);

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
                if (_stopSaveState.empty())
                    return;
                try {
                    String save = _simulator->name() + " = " +
                        _simulator->save();
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
