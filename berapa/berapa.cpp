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

template<class T> class SimulatorT;
typedef SimulatorT<void> Simulator;

template<class T> class Intel8088CPUT;
typedef Intel8088CPUT<void> Intel8088CPU;

template<class T> class ComponentT;
typedef ComponentT<void> Component;

template<class T> class Intel8237DMACT;
typedef Intel8237DMACT<void> Intel8237DMAC;

template<class T> class Intel8259PICT;
typedef Intel8259PICT<void> Intel8259PIC;

template<class T> class DMAPageRegistersT;
typedef DMAPageRegistersT<void> DMAPageRegisters;

template<class T> class PCXTKeyboardT;
typedef PCXTKeyboardT<void> PCXTKeyboard;

template<class T> class PCXTKeyboardPortT;
typedef PCXTKeyboardPortT<void> PCXTKeyboardPort;

template<class T> class ConnectorT;
typedef ConnectorT<void> Connector;

Concrete second;

class Tick
{
    typedef int Base;
public:
    Tick() : _t(0) { }
    Tick(const Base& t) : _t(t) { }
    static Tick infinity() { return std::numeric_limits<Base>::max(); }
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

class Protocol : public ConstHandle
{
public:
    Protocol() { }
    Protocol(const ConstHandle& r) : ConstHandle(r) { }
};
template<class T> class ProtocolBase : public Nullary<Protocol, T> { };

// The meaning of _output is up to the particular Protocol. In some cases (e.g.
// ISA card edge/slot) neither is really an output so we just have to pick a
// direction arbitrarily. Non-gendered connectors should specify two
// ProtocolDirections, one for each direction.
class ProtocolDirection
{
public:
    ProtocolDirection() : _output(false) { }
    ProtocolDirection(Protocol p, bool o) : _protocol(p), _output(o) { }
    bool matches(ProtocolDirection other) const
    {
        return _protocol == other._protocol && _output != other._output;
    }
    bool operator==(ProtocolDirection r) const
    {
        return _protocol == r._protocol && _output == r._output;
    }
    UInt32 hash() const
    {
        return Hash(typeid(ProtocolDirection)).mixin(_protocol.hash()).
            mixin(_output ? 1 : 0);
    }
    auto operator-() const { return ProtocolDirection(_protocol, !_output); }
private:
    Protocol _protocol;
    bool _output;
};

Value connectorFromValue(Value v);
Type connectorTypeFromType(Type t);

template<class T> class ConnectorT
{
public:
    ConnectorT(Component* component)
      : _component(component), _connected(false) { }
    class Type : public ::Type
    {
    public:
        Type() { }
        Type(::Type type) : ::Type(to<Body>(type)) { }
        bool compatible(Type other) const
        {
            return body()->compatible(other) ||
                other.body()->compatible(*this);
        }
        bool canConnectMultiple() const
        {
            return body()->canConnectMultiple();
        }
        List<ProtocolDirection> protocolDirections() const
        {
            return body()->protocolDirections();
        }
        static String name() { return "Connector"; }
        class Body : public ::Type::Body
        {
        public:
            Body(Simulator* simulator) : _simulator(simulator) { }
            virtual List<ProtocolDirection> protocolDirections() const = 0;
            virtual bool canConnectMultiple() const { return false; }
            virtual bool canConvertFrom(const ::Type& other, String* reason)
                const
            {
                ConnectorT<T>::Type t = connectorTypeFromType(other);
                if (!t.valid()) {
                    *reason = other.toString() + " is not a connector.";
                    return false;
                }
                if (!_simulator->canConnect(Type(type()), t)) {
                    *reason = t.toString() + " and " + this->toString() +
                        " are not compatible connectors.";
                    return false;
                }
                return true;
            }
            virtual ValueT<T> convert(const ValueT<T>& value) const
            {
                return Value(type(), connectorFromValue(value).value(),
                    value.span());
            }
        private:
            SimulatorT<T>* _simulator;
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
    void doConnect(ConnectorT<T>* other, ProtocolDirection pd, Span span)
    {
        if (_connected && !type().canConnectMultiple())
            span.throwError("Connector is already connected");
        connect(other, pd, span);
        _connected = true;
    }
    virtual bool canBeDisconnected() const
    {
        return type().canConnectMultiple();
    }
    void checkConnected()
    {
        if (!_connected && !canBeDisconnected())
            _component->simulator()->connectStub(this);
    }
    Component* component() const { return _component; }
    void setName(String name) { _name = name; }
    String name() const { return _component->name() + "." + _name; }

    virtual Type type() const = 0;
    virtual void connect(Connector* other, ProtocolDirection pd, Span span)
    {
        connect(other, pd);
    }
    virtual void connect(Connector* other, ProtocolDirection pd)
    {
        connect(other);
    }
    virtual void connect(Connector* other) { }
    Simulator* simulator() const { return _component->simulator(); }

private:
    bool _connected;
    ComponentT<T>* _component;
    String _name;
};

template<class T, class B = Connector> class ConnectorBase : public B
{
public:
    ConnectorBase(Component* c) : B(c) { }
    static auto protocolDirections()
    {
        List<ProtocolDirection> r;
        r.add(T::protocolDirection());
        return r;
    }
    static auto canConnectMultiple() { return false; }
    Connector::Type type() const
    {
        return typename T::Type(this->simulator());
    }
    class Type : public B::Type
    {
    public:
        Type(Simulator* simulator)
          : B::Type(Type::template create<Body>(simulator)) { }
        Type(const ConstHandle& r) : B::Type(r) { }

        class Body : public B::Type::Body
        {
        public:
            Body(Simulator* simulator) : B::Type::Body(simulator) { }
            List<ProtocolDirection> protocolDirections() const
            {
                return T::protocolDirections();
            }
            bool canConnectMultiple() const { return T::canConnectMultiple(); }
            String toString() const { return T::typeName(); }
        };
    };
};

class ClockedComponent;

template<class T> class ComponentT : public Structure
{
public:
    class Type : public ::Type
    {
    public:
        Type() { }
        Type(::Type type) : ::Type(to<Body>(type)) { }
        Component* createComponent()
        {
            auto c = body()->createComponent();
            simulator()->addComponent(c);
            return &*c;
        }
        Simulator* simulator() const { return body()->simulator(); }
        typename ConnectorT<T>::Type defaultConnectorType() const
        {
            return body()->defaultConnectorType();
        }
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
            bool canConvertFrom(const ::Type& other, String* why) const
            {
                return other.canConvertTo(StructuredType::empty().type(), why);
            }
            Value convert(const Value& value) const
            {
                return Value(type(), Type(type()).createComponent(),
                    value.span());
            }
            Simulator* simulator() const { return _simulator; }
            typename ConnectorT<T>::Type defaultConnectorType() const
            {
                return _defaultConnectorType;
            }
        protected:
            Simulator* _simulator;
            typename ConnectorT<T>::Type _defaultConnectorType;
        };
        const Body* body() const { return as<Body>(); }
    };
    ComponentT(Type type)
      : _type(type), _simulator(type.simulator()),
        _defaultConnector(0), _loopCheck(false)
    {
        persist("tick", &_tick);
    }
    virtual void runTo(Tick tick) { _tick = tick; }
    virtual void maintain(Tick ticks) { _tick -= ticks; }
    String name() const { return _name; }
    void set(Identifier name, Value value, Span span)
    {
        String n = name.name();
        if (n == "*") {
            _name = value.value<String>();
            return;
        }
        if (typename ConnectorT<T>::Type(value.type()).valid()) {
            auto l = getValue(name).template value<Connector*>();
            auto r = value.value<Connector*>();
            _simulator->connect(l, r, span);
            return;
        }
        if (_config.hasKey(n)) {
            Member m = _config[n];
            m.type().deserialize(value, m._p);
        }
        Structure::set(name, value, span);
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
                for (auto i : component._config)
                    _members[i.key()] = i.value().type();
                if (component._defaultConnector != 0) {
                    this->_defaultConnectorType =
                        component._defaultConnector->type();
                }
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
            int size() const { return sizeof(C); }
            Value value(void* p) const { return static_cast<C*>(p)->value(); }
        private:
            HashTable<Identifier, ::Type> _members;
        };
    };
    virtual String save(int width, int used, int indent, int delta) const
    {
        // First try putting everything on one line
        String s("{ ");
        bool needComma = false;
        bool separate = false;
        used += 5;
        for (auto i : _persist) {
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
        for (auto i : _persist) {
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
        for (auto i : _persist)
            members.add(StructuredType::Member(i.key(), i.value()._initial));
        return PersistenceType(_type.toString(), members, _type.size());
    }
    virtual void load(const Value& value)
    {
        auto members = value.value<HashTable<Identifier, Value>>();
        for (auto i : _persist) {
            Member m = i.value();
            m.type().deserialize(members[i.key()], m._p);
        }
    }
    Value value() const
    {
        HashTable<Identifier, Value> h;
        for (auto i : _persist) {
            Member m = i.value();
            h.add(i.key(), m.type().value(m._p));
        }
        return Value(persistenceType(), h);
    }
    Type type() const { return _type; }
    Connector* defaultConnector() const { return _defaultConnector; }
    void checkConnections(int* n)
    {
        for (auto i : _config) {
            Member m = i.value();
            if (typename ConnectorT<T>::Type(m.type()).valid())
                static_cast<ConnectorT<T>*>(m._p)->checkConnected();
        }
        if (_name == "") {
            _name = decimal(*n);
            ++*n;
        }
    }
    SimulatorT<T>* simulator() const { return _simulator; }
    bool loopCheck()
    {
        if (_loopCheck)
            return true;
        _loopCheck = true;
        if (subLoopCheck())
            return true;
        _loopCheck = false;
        return false;
    }
    bool isInLoop() const { return _loopCheck; }
    virtual bool subLoopCheck() { return false; }

    Tick _tick;
    virtual List<ClockedComponent*> enumerateClocks()
    {
        return List<ClockedComponent*>();
    }

protected:
    void connector(String name, ConnectorT<T>* p)
    {
        if (name == "") {
            _defaultConnector = p;
            Identifier i = Identifier(OperatorAssignment());
            OverloadedFunctionSet o(i);
            o.add(AssignmentFunco(this));
            Value v(o);
            _config.add(i, Member(0, v));
            Structure::set(i, v, Span());
        }
        else {
            // Connectors don't have a default value. If they did, a connector
            // supporting multiple connections would always be connected to the
            // default. We can't convert directly from the type to the value,
            // or the conversion would call convert() which would attempt to
            // connect.
            _config.add(name,
                Member(static_cast<void*>(p), Value(p->type(), 0)));
            Structure::set(name, p->getValue(), Span());
        }
        p->setName(name);
    }
    template<class C> void config(String name, C* p,
        ::Type type = typeFromCompileTimeType<C>())
    {
        _config.add(name, Member(static_cast<void*>(p), type));
        Structure::set(name, type, Span());
        set(name, type, Span());
    }
    template<class C, class I, std::enable_if_t<!std::is_base_of<::Type, I>
        ::value>* = nullptr>
        void persist(String name, C* p, I initial,
            ::Type type = typeFromCompileTimeType<C>())
    {
        Value v(type, initial);
        ArrayType arrayType(type);
        if (arrayType.valid()) {
            Value m(arrayType.contained(), initial);
            List<Value> initialArray;
            LessThanType l(arrayType.indexer());
            if (!l.valid()) {
                throw Exception(
                    "Don't know how many elements to put in default.");
            }
            int n = l.n();
            for (int i = 0; i < n; ++i)
                initialArray.add(m);
            v = Value(type, initialArray);
        }
        _persist.add(name, Member(static_cast<void*>(p), v));

    }
    template<class C> void persist(String name, C* p,
        Value initial = Value(typeFromCompileTimeType<C>()))
    {
        _persist.add(name, Member(static_cast<void*>(p), initial));
    }
private:
    class PersistenceType : public StructuredType
    {
    public:
        PersistenceType(String name, List<StructuredType::Member> members,
            int size)
          : StructuredType(create<Body>(name, members, size)) { }
    private:
        class Body : public StructuredType::Body
        {
        public:
            Body(String name, List<Member> members, int size)
              : StructuredType::Body(name, members), _size(size) { }
            Value value(void* p) const
            {
                return static_cast<Component*>(p)->value();
            }
            int size() const { return _size; }
            String serialize(void* p, int width, int used, int indent,
                int delta) const
            {
                return static_cast<Component*>(p)->
                    save(width, used, indent, delta);
            }
            void deserialize(const Value& value, void* p) const
            {
                static_cast<Component*>(p)->load(value);
            }

        private:
            int _size;
        };
    };
    class AssignmentFunco : public Funco
    {
    public:
        AssignmentFunco(Component* component)
          : Funco(create<Body>(component)) { }
        ::Type type() const { return Funco::type(); }
        class Body : public Funco::Body
        {
        public:
            Body(Component* component) : _component(component) { }
            Value evaluate(List<Value> arguments, Span span) const
            {
                auto i = arguments.begin();
                auto ls = i->rValue().value<Structure*>();
                auto l = dynamic_cast<ComponentT<T>*>(ls)->_defaultConnector;
                auto lt = l->getValue().type();
                ++i;
                String reason;
                if (Type(i->type()).valid()) {
                    auto rs = i->value<Structure*>();
                    auto r = dynamic_cast<ComponentT<T>*>(rs)->
                        _defaultConnector;
                    if (!lt.canConvertFrom(r->getValue().type(), &reason))
                        span.throwError(reason);
                    _component->simulator()->connect(l, r, span);
                    return Value();
                }
                auto r = dynamic_cast<ConnectorT<T>*>(i->value<Structure*>());
                if (!lt.canConvertFrom(r->getValue().type(), &reason))
                    span.throwError(reason);
                _component->simulator()->connect(l, r, span);
                return Value();
            }
            Identifier identifier() const { return OperatorAssignment(); }
            bool argumentsMatch(List<::Type> argumentTypes) const
            {
                if (argumentTypes.count() != 2)
                    return false;
                auto i = argumentTypes.begin();
                if (!Type(i->rValue()).valid())
                    return false;
                ++i;
                if (Type(*i).valid())
                    return true;
                return typename ConnectorT<T>::Type(*i).valid();
            }
            // Won't actually be used (since there's only one function matching
            // the argument types) but necessary to avoid AssignmentFunco being
            // an abstract class.
            List<Tyco> parameterTycos() const
            {
                List<Tyco> r;
                r.add(typename ConnectorT<T>::Type());
                r.add(LValueType(_component->type()));
                return r;
            }
        private:
            ComponentT<T>* _component;
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
    HashTable<Identifier, Member> _config;
    HashTable<String, Member> _persist;

    String _name;
    Type _type;
    Connector* _defaultConnector;
    SimulatorT<T>* _simulator;
    bool _loopCheck;

    friend class ConnectorT<T>::Type::Body;
    friend class AssignmentFunco::Body;
};

template<class C> class ComponentBase : public Component
{
public:
    ComponentBase(Component::Type type) : Component(type) { }
    typedef Component::TypeHelper<C> Type;
};

// For sub-components (like RAM) the type doesn't create the component, it just
// returns the pointer to the member of the parent component.
template<class C, template<class> class B = ComponentBase> class SubComponent
  : public B<C>
{
public:
    SubComponent(Component::Type type) : B<C>(type) { }
    class Type : public B<C>::Type
    {
    public:
        Type(Simulator* simulator, C* c = 0)
          : Component::TypeHelper<C>(
                Component::TypeHelper<C>::template create<Body>(simulator, c))
        { }
    private:
        class Body : public ComponentBase<C>::Type::Body
        {
        public:
            Body(Simulator* simulator, C* c)
              : B<C>::Type::Body(simulator), _c(c) { }
            Value convert(const Value& value) const
            {
                return Value(this->type(), static_cast<Structure*>(_c),
                    value.span());
            }
        private:
            C* _c;
        };
    };
};

Value connectorFromValue(Value v)
{
    Component::Type ct(v.type());
    if (ct.valid()) {
        Structure* s = v.value<Structure*>();
        auto component = static_cast<Component*>(s);
        Connector* connector = component->defaultConnector();
        return connector->getValue();
    }
    return v;
}

Type connectorTypeFromType(Type t)
{
    Component::Type ct(t);
    if (ct.valid())
        return ct.defaultConnectorType();
    return t;
}

class ClockedComponent : public Component
{
public:
    ClockedComponent(Type type) : Component(type), _ticksPerCycle(0)
    {
        config("frequency", &_cyclesPerSecond, (1/second).type());
        config("offset", &_offset, RationalType());
    }
    Rational cyclesPerSecond() const { return _cyclesPerSecond; }
    void setTicksPerCycle(Tick ticksPerCycle)
    {
        _ticksPerCycle = ticksPerCycle;
        _tick = 0;
    }
    List<ClockedComponent*> enumerateClocks()
    {
        List<ClockedComponent*> r;
        r.add(this);
        return r;
    }
protected:
    Tick _ticksPerCycle;
private:
    Rational _cyclesPerSecond;
    Rational _offset;
};

template<class C> class ClockedComponentBase : public ClockedComponent
{
public:
    ClockedComponentBase(Component::Type t) : ClockedComponent(t) { }
    typedef Component::TypeHelper<C> Type;
};

template<class C> using ClockedSubComponent =
    SubComponent<C, ClockedComponentBase>;

template<class T> class SimpleProtocol
  : public ProtocolBase<SimpleProtocol<T>> { };

template<class T> class FanoutComponent;

class TransportConnector : public Connector
{
public:
    TransportConnector(Component* c) : Connector(c) { }
    class Type : public Connector::Type
    {
    public:
        Type(const ConstHandle& other) : Connector::Type(other) { }
        ::Type transportType() const { return body()->transportType(); }
    protected:
        class Body : public Connector::Type::Body
        {
        public:
            Body(Simulator* s) : Connector::Type::Body(s) { }
            virtual ::Type transportType() const = 0;
        };
        const Body* body() const { return as<Body>(); }
    };
};

template<class C, class T, class B = TransportConnector>
    class TransportConnectorBase : public ConnectorBase<C, B>
{
    using Base = ConnectorBase<C, B>;
public:
    TransportConnectorBase(Component* c) : Base(c) { }
    static auto protocolDirections()
    {
        List<ProtocolDirection> r;
        r.add(ProtocolDirection(SimpleProtocol<T>(), true));
        r.add(ProtocolDirection(SimpleProtocol<T>(), false));
        return r;
    }
    static auto canConnectMultiple() { return true; }
    static String typeName()
    {
        return C::tycoName() + "<" + typeFromCompileTimeType<T>().toString() +
            ">";
    }
    class Type : public Base::Type
    {
    public:
        Type(Simulator* s) : Base::Type(Base::Type::template create<Body>(s))
        { }
        Type(const ConstHandle& r) : Base::Type(r) { }
        class Body : public Base::Type::Body
        {
        public:
            Body(Simulator* s) : Base::Type::Body(s) { }
            ::Type transportType() const
            {
                return typeFromCompileTimeType<T>();
            }
        };
    };
};

template<class T> class BidirectionalConnector
  : public TransportConnectorBase<BidirectionalConnector<T>, T>
{
public:
    static String tycoName() { return "BidirectionalConnector"; }
    BidirectionalConnector(Component* c)
      : TransportConnectorBase<BidirectionalConnector<T>, T>(c), _other(0) { }
    virtual void setData(Tick t, T v) = 0;
    void connect(::Connector* other, ProtocolDirection pd, Span span)
    {
        auto o = static_cast<BidirectionalConnector<T>*>(other);
        if (_other == 0) {
            _other = o;
            _otherComponent = _other->component();
            return;
        }
        auto fanout = dynamic_cast<FanoutComponent<T>*>(_otherComponent);
        if (fanout == 0) {
            fanout = static_cast<FanoutComponent<T>*>(typename
                FanoutComponent<T>::Type(this->simulator()).createComponent());
            _other->_other = 0;
            fanout->connect(_other, span);
            _other = 0;
            fanout->connect(this, span);
        }
        o->_other = 0;
        fanout->connect(o, span);
    }
    bool canBeDisconnected() const { return false; }
    BidirectionalConnector<T>* _other;
    Component* _otherComponent;
};

template<class T> class OutputConnector
  : public TransportConnectorBase<OutputConnector<T>, T,
        BidirectionalConnector<T>>
{
public:
    static String tycoName() { return "OutputConnector"; }
    static auto protocolDirection()
    {
        return ProtocolDirection(SimpleProtocol<T>(), true);
    }
    OutputConnector(Component* c) : TransportConnectorBase<OutputConnector<T>,
        T, BidirectionalConnector<T>>(c) { }
    void setData(Tick t, T v) { }
    bool loopCheck() { return this->_otherComponent->loopCheck(); }
};

template<class T> class OptimizedOutputConnector : public OutputConnector<T>
{
public:
    OptimizedOutputConnector(Component* c) : OutputConnector<T>(c) { }
    void set(Tick t, T v)
    {
        if (v != _v) {
            _v = v;
            this->_other->setData(t, v);
        }
    }
    void init(T v) { _v = v; }
private:
    T _v;
};

template<class T> class InputConnector
  : public TransportConnectorBase<InputConnector<T>, T,
        BidirectionalConnector<T>>
{
    using Base = TransportConnectorBase<InputConnector<T>, T,
        BidirectionalConnector<T>>;
public:
    static String tycoName() { return "InputConnector"; }
    // It may seem strange that we don't override protocolDirections() here,
    // meaning that we speak ProtocolDirection(SimpleProtocol<T>(), true). This
    // is because it's allowed for two InputConnectors to be connected to each
    // other. The result will be that both are connected to outputs on the same
    // fanout, with the input stubbbed (and hence connected to
    // ConstantComponent). The result is ultimately the same as if both
    // InputConnectors are left disconnected (and hence stubbed to separate
    // ConstantComponents).
    InputConnector(Component* c) : Base(c) { }
    virtual void setData(Tick t, T v) = 0;
    void connect(::Connector* other, ProtocolDirection pd, Span span)
    {
        if (this->_other == 0 &&
            dynamic_cast<InputConnector<T>*>(other) != 0) {
            auto fanout = static_cast<FanoutComponent<T>*>(typename
                FanoutComponent<T>::Type(this->simulator()).createComponent());
            fanout->connect(this, span);
            fanout->connect(static_cast<BidirectionalConnector<T>*>(other),
                span);
        }
        else
            Base::connect(other, pd, span);
    }
};

template<class C, class T> class TransportComponentBase
  : public ComponentBase<C>
{
public:
    TransportComponentBase(Component::Type t) : ComponentBase<C>(t) { }
    static String typeName()
    {
        return C::tycoName() + "<" + typeFromCompileTimeType<T>().toString() +
            ">";
    }
};

template<class C, class T> class BooleanComponent
  : public TransportComponentBase<C, T>
{
public:
    BooleanComponent(Component::Type type)
      : TransportComponentBase<C, T>(type), _input0(this), _input1(this),
        _output(this)
    {
        this->connector("input0", &_input0);
        this->connector("input1", &_input1);
        this->connector("output", &_output);
    }
    bool subLoopCheck() { return _output.loopCheck(); }
    virtual void update0(Tick t, T v) = 0;
    virtual void update1(Tick t, T v) = 0;
private:
    class InputConnector : public ::InputConnector<T>
    {
    public:
        InputConnector(BooleanComponent *c)
          : ::InputConnector<T>(c), _component(c) { }
        T _v;
        BooleanComponent* _component;
    };
    class InputConnector0 : public InputConnector
    {
    public:
        InputConnector0(BooleanComponent *c) : InputConnector(c) { }
        void setData(Tick t, T v)
        {
            this->_component->update0(t, v);
            this->_v = v;
        }
    };
    class InputConnector1 : public InputConnector
    {
    public:
        InputConnector1(BooleanComponent *c) : InputConnector(c) { }
        void setData(Tick t, T v)
        {
            this->_component->update1(t, v);
            this->_v = v;
        }
    };
protected:
    InputConnector0 _input0;
    InputConnector1 _input1;
    OptimizedOutputConnector<T> _output;
};

template<class T> class BinaryTraits
{
public:
    static T zero() { return 0; }
    static T invert(const T& other) { return ~other; }
    static T one() { return invert(zero()); }
};

template<> class BinaryTraits<bool>
{
public:
    static bool zero() { return false; }
    static bool invert(const bool& other) { return !other; }
    static bool one() { return true; }
};

template<class T> class AndComponent
  : public BooleanComponent<AndComponent<T>, T>
{
    using Base = BooleanComponent<AndComponent<T>, T>;
public:
    static String tycoName() { return "And"; }
    AndComponent(Component::Type type) : Base(type) { }
    void load(const Value& v)
    {
        Base::load(v);
        this->_output.init(this->_input0._v & this->_input1._v);
    }
    void runTo(Tick tick)
    {
        this->_input0._otherComponent->runTo(tick);
        if (this->_input0._v != BinaryTraits<T>::zero())
            this->_input1._otherComponent->runTo(tick);
        Component::runTo(tick);
    }
    void update0(Tick t, T v)
    {
        if (t > this->_input1._otherComponent->_tick)
            this->_input1._otherComponent->runTo(t);
        else {
            this->_tick = t;
            if (this->_input1._v != BinaryTraits<T>::zero())
                this->_output.set(t, v & this->_input1._v);
        }
    }
    void update1(Tick t, T v)
    {
        if (t > this->_input0._otherComponent->_tick)
            this->_input0._otherComponent->runTo(t);
        else {
            this->_tick = t;
            if (this->_input0._v != BinaryTraits<T>::zero())
                this->_output.set(t, v & this->_input0._v);
        }
    }
};

template<class T> class OrComponent
  : public BooleanComponent<OrComponent<T>, T>
{
    using Base = BooleanComponent<OrComponent<T>, T>;
public:
    static String tycoName() { return "Or"; }
    OrComponent(Component::Type type) : Base(type) { }
    void load(const Value& v)
    {
        Base::load(v);
        this->_output.init(this->_input0._v | this->_input1._v);
    }
    void runTo(Tick tick)
    {
        this->_input0._otherComponent->runTo(tick);
        if (this->_input0._v != BinaryTraits<T>::one())
            this->_input1._otherComponent->runTo(tick);
        Component::runTo(tick);
    }
    void update0(Tick t, T v)
    {
        if (t > this->_input1._otherComponent->_tick)
            this->_input1._otherComponent->runTo(t);
        else {
            this->_tick = t;
            if (this->_input1._v != BinaryTraits<T>::one())
                this->_output.set(t, v | this->_input1._v);
        }
    }
    void update1(Tick t, T v)
    {
        if (t > this->_input0._otherComponent->_tick)
            this->_input0._otherComponent->runTo(t);
        else {
            this->_tick = t;
            if (this->_input0._v != BinaryTraits<T>::one())
                this->_output.set(t, v | this->_input0._v);
        }
    }
};

template<class T> class NotComponent
  : public TransportComponentBase<NotComponent<T>, T>
{
public:
    static String tycoName() { return "Not"; }
    NotComponent(Component::Type type)
      : TransportComponentBase<NotComponent<T>, T>(type), _input(this),
        _output(this)
    {
        this->connector("input", &_input);
        this->connector("output", &_output);
    }
    bool subLoopCheck() { return _output.loopCheck(); }
    void runTo(Tick tick)
    {
        // We're up to date if our input is up to date.
        _input._otherComponent->runTo(tick);
        Component::runTo(tick);
    }
    void update(Tick t, T v)
    {
        _output._other->setData(t, BinaryTraits<T>::invert(v));
    }
private:
    class InputConnector : public ::InputConnector<T>
    {
    public:
        InputConnector(NotComponent *c)
          : ::InputConnector<T>(c),
            _component(static_cast<NotComponent<T>*>(c))
        { }
        void setData(Tick t, T v) { _component->update(t, v); }
        NotComponent<T>* _component;
    };
protected:
    InputConnector _input;
    OutputConnector<T> _output;
};

template<class T> class BucketComponent
  : public TransportComponentBase<BucketComponent<T>, T>
{
public:
    static String tycoName() { return "Bucket"; }
    BucketComponent(Component::Type t)
      : TransportComponentBase<BucketComponent<T>, T>(t), _connector(this)
    {
        this->connector("", &_connector);
    }
    class Connector : public InputConnector<T>
    {
    public:
        Connector(BucketComponent* c) : InputConnector<T>(c) { }
        void setData(Tick tick, T t) { }
    };
    Connector _connector;
};

template<class T> class ConstantComponent
  : public TransportComponentBase<ConstantComponent<T>, T>
{
public:
    static String tycoName() { return "Constant"; }
    ConstantComponent(Component::Type t, T v = BinaryTraits<T>::one())
      : TransportComponentBase<ConstantComponent<T>, T>(t), _v(v),
        _connector(this)
    {
        this->connector("", &_connector);
    }
private:
    void load(const Value& v) { _connector.setData(0, _v); }
    T _v;
    OutputConnector<T> _connector;
};

template<class T> class FanoutComponent
  : public TransportComponentBase<FanoutComponent<T>, T>
{
public:
    static String tycoName() { return "Fanout"; }
    FanoutComponent(Component::Type t)
      : TransportComponentBase<FanoutComponent<T>, T>(t), _input(this)
    {
        this->connector("input", &_input);
    }
    void connect(BidirectionalConnector<T>* c, Span span)
    {
        if (dynamic_cast<::InputConnector<T>*>(c) != 0)
            this->simulator()->connect(_output.add(this), c, span);
        else
            if (dynamic_cast<::OutputConnector<T>*>(c) == 0)
                this->simulator()->connect(_bidirectional.add(this), c, span);
            else {
                if (_input._other != 0) {
                    span.throwError("Cannot connect " +
                        _input._other->name() + " and " + c->name() +
                        " together as they are both outputs.");
                }
                this->simulator()->connect(&_input, c, span);
            }
    }
    bool subLoopCheck()
    {
        for (auto i : _output)
            if (i.loopCheck())
                return true;
        for (auto i : _bidirectional)
            if (i.loopCheck())
                return true;
        return true;
    }
private:
    class InputConnector : public ::InputConnector<T>
    {
    public:
        InputConnector(FanoutComponent* c)
          : ::InputConnector<T>(c), _fanout(c) { }
        void setData(Tick t, T v) { _fanout->update(t, v, 0); }
        bool canBeDisconnected() const
        {
            return _fanout->_bidirectional.count() != 0;
        }
        FanoutComponent* _fanout;
    };
    class BidirectionalConnector : public ::BidirectionalConnector<T>
    {
    public:
        BidirectionalConnector(FanoutComponent* c)
          : ::BidirectionalConnector<T>(c), _fanout(c),
            _v(BinaryTraits<T>::one())
        { }
        void setData(Tick t, T v) { _v = v; _fanout->update(t, v, this); }
        FanoutComponent* _fanout;
        T _v;
    };
    void update(Tick t, T v, BidirectionalConnector* c)
    {
        for (auto i : _bidirectional) {
            if (&i == c)
                continue;
            i._otherComponent->runTo(t);
            v &= i._v;
        }
        for (auto i : _output)
            i._other->setData(t, v);
        for (auto i : _bidirectional) {
            if (&i == c)
                continue;
            i._other->setData(t, v);
        }
    }
    InputConnector _input;
    List<OutputConnector<T>> _output;
    List<BidirectionalConnector> _bidirectional;
    friend class InputConnector;
    friend class BidirectionalConnector;
};

// SRLatch works like a NAND latch with inverters on the inputs.
class SRLatch : public ComponentBase<SRLatch>
{
public:
    static String typeName() { return "SRLatch"; }
    SRLatch(Component::Type type)
      : ComponentBase(type), _set(this), _reset(this), _isSet(false),
        _lastSet(this), _lastReset(this)
    {
        connector("set", &_set);
        connector("reset", &_reset);
        connector("lastSet", &_lastSet);
        connector("lastReset", &_lastReset);
    }
    void load(const Value& v)
    {
        Component::load(v);
        if (!_set._v && !_reset._v) {
            _lastSet.init(_isSet);
            _lastReset.init(!_isSet);
        }
        else {
            _lastSet.init(_set._v);
            _lastReset.init(_reset._v);
        }
    }
    void runTo(Tick tick)
    {
        _set._otherComponent->runTo(tick);
        _reset._otherComponent->runTo(tick);
        Component::runTo(tick);
    }
    bool subLoopCheck()
    {
        return _lastSet.loopCheck() || _lastReset.loopCheck();
    }
    // Truth table:
    //   set reset _isSet  lastSet lastReset
    //    0    0      0       0        1
    //    0    0      1       1        0
    //    1    0      X       1        0       _isSet <- 1
    //    0    1      X       0        1       _isSet <- 0
    //    1    1      X       1        1
    void doSet(Tick t, bool v)
    {
        if (t > _reset._otherComponent->_tick)
            _reset._otherComponent->runTo(t);
        else {
            this->_tick = t;
            update(t, v, _reset._v);
        }
    }
    void doReset(Tick t, bool v)
    {
        if (t > _set._otherComponent->_tick)
            _set._otherComponent->runTo(t);
        else {
            this->_tick = t;
            update(t, _set._v, v);
        }
    }
    void update(Tick t, bool s, bool r)
    {
        if (!s) {
            if (!r) {
                _lastSet.setData(t, _isSet);
                _lastReset.setData(t, !_isSet);
            }
            else {
                _isSet = false;
                _lastSet.setData(t, false);
                _lastReset.setData(t, true);
            }
        }
        else {
            if (!r) {
                _isSet = true;
                _lastSet.setData(t, true);
                _lastReset.setData(t, false);
            }
            else {
                _lastSet.setData(t, true);
                _lastReset.setData(t, true);
            }
        }
    }

private:
    class SetConnector : public InputConnector<bool>
    {
    public:
        SetConnector(SRLatch* latch) : InputConnector(latch), _latch(latch) { }
        void setData(Tick t, bool v) { _latch->doSet(t, v); _v = v; }
        bool _v;
    private:
        SRLatch* _latch;
    };
    class ResetConnector : public InputConnector<bool>
    {
    public:
        ResetConnector(SRLatch* latch)
          : InputConnector(latch), _latch(latch) { }
        void setData(Tick t, bool v) { _latch->doReset(t, v); _v = v; }
        bool _v;
    private:
        SRLatch* _latch;
    };

    SetConnector _set;
    ResetConnector _reset;
    OptimizedOutputConnector<bool> _lastSet;
    OptimizedOutputConnector<bool> _lastReset;
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
            Value l = connectorFromValue(*i);
            Type t = TransportConnector::Type(l.type()).transportType();
            ++i;
            Value r = connectorFromValue(*i);
            ::Component* c;
            if (t == ByteType()) {
                c = typename Component<Byte>::Type(_simulator).
                    createComponent();
            }
            else {
                if (t == BooleanType()) {
                    c = typename Component<bool>::Type(_simulator).
                        createComponent();
                }
            }
            c->set("input0", l, l.span());
            c->set("input1", r, r.span());
            return c->getValue("output");
        }
        bool argumentsMatch(List<Type> argumentTypes) const
        {
            if (argumentTypes.count() != 2)
                return false;
            auto i = argumentTypes.begin();
            TransportConnector::Type l(connectorTypeFromType(*i));
            if (!l.valid())
                return false;
            ++i;
            TransportConnector::Type r(connectorTypeFromType(*i));
            if (!r.valid())
                return false;
            return l.transportType() == r.transportType();
        }
        // Won't actually be used (since there's only one function matching the
        // argument types) but necessary to avoid ComponentFunco being
        // an abstract class.
        List<Tyco> parameterTycos() const
        {
            List<Tyco> r;
            r.add(Connector::Type());
            r.add(Connector::Type());
            return r;
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

class NotComponentFunco : public Funco
{
public:
    NotComponentFunco(Simulator* simulator)
      : Funco(create<Body>(simulator)) { }
    class Body : public Funco::Body
    {
    public:
        Body(Simulator* simulator) : _simulator(simulator) { }
        Identifier identifier() const { return OperatorTwiddle(); }
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            Value l = connectorFromValue(*i);
            Type t = TransportConnector::Type(l.type()).transportType();
            ::Component* c;
            if (t == ByteType())
                c = NotComponent<Byte>::Type(_simulator).createComponent();
            else {
                if (t == BooleanType())
                    c = NotComponent<bool>::Type(_simulator).createComponent();
            }
            c->set("input", l, span);
            return c->getValue("output");
        }
        bool argumentsMatch(List<Type> argumentTypes) const
        {
            if (argumentTypes.count() != 1)
                return false;
            auto i = argumentTypes.begin();
            return TransportConnector::Type(connectorTypeFromType(*i)).valid();
        }
        // Won't actually be used (since there's only one function matching the
        // argument types) but necessary to avoid ComponentFunco being
        // an abstract class.
        List<Tyco> parameterTycos() const
        {
            List<Tyco> r;
            r.add(Connector::Type());
            return r;
        }
    private:
        Simulator* _simulator;
    };
};

template<class T> class SimulatorT
{
public:
    SimulatorT(Directory directory)
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
        String s("{\n");
        bool needComma = false;
        for (auto i : _components) {
            if (i->value() == i->persistenceType().defaultValue())
                continue;
            String l = "    " + i->name() + ": ";
            String v = i->save(79, l.length(), 8, 4);
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
        int n = 0;
        for (auto i : _components)
            i->checkConnections(&n);
        for (auto i : _components) {
            bool r = i->loopCheck();
            if (r) {
                String s = "Dependency loop involving ";
                bool needComma = false;
                for (auto j : _components) {
                    if (!j->isInLoop())
                        continue;
                    if (needComma)
                        s += ", ";
                    s += j->name();
                }
                throw Exception(s);
            }
        }
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
    bool canConnect(Connector::Type l, Connector::Type r)
    {
        return bestPath(l, r).chosen();
    }
    void connect(Connector* l, Connector* r, Span span)
    {
        do {
            auto lt = l->type();
            auto rt = r->type();
            auto path = bestPath(l->type(), r->type());
            if (path._score == 0) {
                l->doConnect(r, path._pd, span);
                r->doConnect(l, -path._pd, span);
                return;
            }
            else {
                auto c = path._componentType.createComponent();
                if (path._output) {
                    connect(l, c->template get<Connector*>("output"), span);
                    l = c->template get<Connector*>("input");
                }
                else {
                    connect(l, c->template get<Connector*>("input"), span);
                    l = c->template get<Connector*>("output");
                }
            }
        } while (true);
    }
    // Conversion components are instantiated when a connection is made between
    // two connectors that don't share a protocol.
    void registerConversionComponent(Component::Type type)
    {
        auto i = Connector::Type(type.member("input")).protocolDirections();
        auto o = Connector::Type(type.member("output")).protocolDirections();
        for (auto ipd : i) {
            for (auto opd : o) {
                registerConversionPath(Pair(ipd, opd), Path(1, type, false));
                registerConversionPath(Pair(opd, ipd), Path(1, type, true));
            }
        }
    }
    // Stub components are connected to connectors that don't have anything
    // connected to them.
    void registerStubComponent(Component::Type type)
    {
        auto l = type.defaultConnectorType().protocolDirections();
        auto path = Path(0, type, false);
        for (auto pd : l) {
            // We only need to register the conversion one way because nothing
            // ever interfaces with NoProtocol.
            registerConversionPath(
                Pair(ProtocolDirection(NoProtocol(), false), pd), path);
        }
    }
    void connectStub(Connector* connector)
    {
        auto pds = connector->type().protocolDirections();
        Path best;
        for (auto pd : pds) {
            best.choose(_conversionPaths[
                Pair(ProtocolDirection(NoProtocol(), false), -pd)]);
        }
        if (!best.chosen())
            throw Exception(connector->name() + " needs to be connected");
        Component* component = best._componentType.createComponent();
        connect(connector, component->defaultConnector(), Span());
    }
private:
    // The NoProtocol is used so we can treat stub components internally as
    // conversion components from their singleton connector to a fake
    // "NoProtocol" connector which doesn't actually get connected to anything.
    class NoProtocol : public ProtocolBase<NoProtocol> { };
    struct Path
    {
        Path() : _score(std::numeric_limits<int>::max()) { }
        Path(ProtocolDirection pd) : _score(0), _pd(pd) { }
        Path(int score, Component::Type t, bool output)
          : _score(score), _componentType(t), _output(output) { }
        void choose(Path other)
        {
            if (other._score < _score)
                *this = other;
        }
        bool chosen() const { return _score < Path()._score; }
        ProtocolDirection _pd;
        int _score;
        Component::Type _componentType;
        bool _output;
    };
    struct Pair
    {
        Pair() { }
        Pair(ProtocolDirection i, ProtocolDirection o) : _input(i), _output(o)
        { }
        bool operator==(Pair r) const
        {
            return _input == r._input && _output == r._output;
        }
        UInt32 hash() const
        {
            return Hash(typeid(Pair)).mixin(_input.hash()).
                mixin(_output.hash());
        }
        ProtocolDirection _input;
        ProtocolDirection _output;
    };
    Path bestPath(Connector::Type l, Connector::Type r)
    {
        auto ll = l.protocolDirections();
        auto rl = r.protocolDirections();
        Path best;
        for (auto lpd : ll)
            for (auto rpd : rl) {
                if (lpd.matches(rpd))
                    return Path(lpd);
                best.choose(_conversionPaths[Pair(lpd, rpd)]);
            }
        return best;
    }
    bool registerConversionPath(Pair pair, Path path)
    {
        if (_conversionPaths.hasKey(pair)) {
            auto existing = _conversionPaths[pair];
            if (path._score >= existing._score)
                return false;
        }
        _conversionPaths[pair] = path;
        bool added;
        do {
            added = false;
            for (auto p : _conversionPaths) {
                auto tPair = p.key();
                auto tPath = p.value();
                if (pair._output.matches(tPair._input)) {
                    if (registerConversionPath(
                        Pair(pair._input, tPair._output),
                        Path(path._score + tPath._score, path._componentType,
                            path._output)))
                        added = true;
                }
            }
        } while (added);
        return true;
    }
    Value initial() const { return persistenceType(); }
    ::Type persistenceType() const
    {
        List<StructuredType::Member> members;
        for (auto i : _components) {
            Type type = i->persistenceType();
            if (!type.valid())
                continue;
            members.add(StructuredType::Member(i->name(), Value(type)));
        }
        return StructuredType("Simulator", members);
    }

    Directory _directory;
    List<Reference<Component>> _components;
    bool _halted;
    Rational _ticksPerSecond;
    HashTable<Pair, Path> _conversionPaths;
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

        simulator.registerStubComponent(ConstantComponent<bool>::Type(p));
        simulator.registerStubComponent(ConstantComponent<Byte>::Type(p));
        simulator.registerStubComponent(BucketComponent<bool>::Type(p));
        simulator.registerStubComponent(BucketComponent<Byte>::Type(p));
        simulator.registerStubComponent(NoRGBIMonitor::Type(p));
        simulator.registerStubComponent(NoRGBISource::Type(p));
        simulator.registerStubComponent(NoISA8BitComponent::Type(p));
        simulator.registerStubComponent(PCXTNoKeyboard::Type(p));

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
        componentTypes.add(ROM::Type(p));

        ConfigFile configFile;
        configFile.addDefaultOption("stopSaveState", StringType(), String(""));
        configFile.addDefaultOption("initialState", StringType(), String(""));
        configFile.addType(second.type(), TycoIdentifier("Time"));
        configFile.addFunco(AndComponentFunco(p));
        configFile.addFunco(OrComponentFunco(p));
        configFile.addFunco(NotComponentFunco(p));

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
