template<class T> class ISA8BitBusT;
typedef ISA8BitBusT<void> ISA8BitBus;

template<class T> class ISA8BitComponentBaseT;
typedef ISA8BitComponentBaseT<void> ISA8BitComponentBase;

template<class T> class ISA8BitComponentBaseT : public ClockedComponent
{
public:
    ISA8BitComponentBaseT(Component::Type type, bool noConnectorName)
      : ClockedComponent(type), _connector(this)
    {
        if (noConnectorName)
            connector("", &_connector);
        else
            connector("bus", &_connector);
        persist("active", &_active);
    }
    // Address bit 31 = write
    // Address bit 30 = IO
    virtual void setAddress(UInt32 address) { };
    virtual void write(UInt8 data) { };
    virtual bool wait() { return false; }
    void setBus(ISA8BitBus* bus) { _bus = bus; }
    virtual UInt8 debugRead(UInt32 address) { return 0xff; }
    bool active() const { return _active; }
    virtual void read() { }
    class Connector : public ::Connector
    {
    public:
        Connector(ISA8BitComponentBaseT* component)
          : ::Connector(component), _component(component) { }
        void connect(::Connector* other)
        {
            dynamic_cast<typename ISA8BitBusT<T>::Connector*>(other)
                ->busConnect(_component);
        }
        Component::Type defaultComponentType(Simulator* simulator)
        {
            throw Exception(_component->name() + " is not plugged in.");
        }
        ::Connector::Type type() const { return Type(); }

        class Type : public NamedNullary<::Connector::Type, Type>
        {
        public:
            Type() { }
            Type(::Type type) : NamedNullary<::Connector::Type, Type>(type) { }
            class Body : public NamedNullary<::Connector::Type, Type>::Body
            {
            public:
                bool compatible(::Connector::Type other) const
                {
                    return typename ISA8BitBusT<T>::Type(other).valid();
                }
            private:
            };
            static String name() { return "ISA8BitBus.Connector"; }
        };
    private:
        ISA8BitComponentBase* _component;
    };

protected:
    void set(UInt8 data) { _bus->_data = data; }
    ISA8BitBusT<T>* _bus;
    bool _active;
    Connector _connector;
};

template<class C> class ISA8BitComponent : public ISA8BitComponentBase
{
public:
    ISA8BitComponent(Component::Type type, bool noConnectorName = false)
      : ISA8BitComponentBase(type, noConnectorName) { }
    typedef ClockedComponent::Type<C> Type;
};

class NoISA8BitComponent : public ISA8BitComponent<NoISA8BitComponent>
{
public:
    static String typeName() { return "NoISA8BitComponent"; }
    NoISA8BitComponent(Component::Type type)
      : ISA8BitComponent<NoISA8BitComponent>(type, true) { }
};

template<class T> class ISA8BitBusT : public Component
{
public:
    static String typeName() { return "ISA8BitBus"; }

    ISA8BitBusT(Component::Type type)
      : Component(type), _cpuSocket(this), _connector(this),
        _chipConnectors{this, this, this, this, this, this, this, this},
        _parityError(this)
    {
        connector("cpu", &_cpuSocket);
        connector("slot", &_connector);
        persist("data", &_data, 0xff);
        for (int i = 0; i < 8; ++i) {
            _chipConnectors[i].init(i);
            connector("chip" + decimal(i), &_chipConnectors[i]);
        }
        connector("parityError", &_parityError);
    }

    class Connector : public ::Connector
    {
    public:
        Connector(ISA8BitBus* bus) : ::Connector(bus), _bus(bus) { }
        void connect(::Connector* other) { }
        Type type() const { return Type(); }
        Component::Type defaultComponentType(Simulator* simulator)
        {
            return NoISA8BitComponent::Type(simulator);
        }

        class Type : public NamedNullary<::Connector::Type, Type>
        {
        public:
            class Body : public NamedNullary<::Connector::Type, Type>::Body
            {
            public:
                bool compatible(::Connector::Type other) const
                {
                    return
                        ISA8BitComponentBase::Connector::Type(other).valid();
                }
            };
            static String name() { return "ISA8BitBus.Connector"; }
        };
    private:
        void busConnect(ISA8BitComponentBase* component)
        {
            _bus->addComponent(component);
        }
    protected:
        ISA8BitBus* _bus;

        template<class U> friend class ISA8BitComponent<U>::Connector;
    };
    class ChipConnector : public Connector
    {
    public:
        ChipConnector(ISA8BitBusT<T>* bus) : Connector(bus) { }
        void init(int i) { _i = i; }
    private:
        int _i;
    };

    typedef Component::TypeHelper<ISA8BitBus> Type;

    class CPUSocket : public ::Connector
    {
    public:
        CPUSocket(ISA8BitBus* bus) : ::Connector(bus), _bus(bus) { }
        void connect(::Connector* other) { other->connect(this); }
        Type type() const { return Type(); }
        Component::Type defaultComponentType(Simulator* simulator)
        {
            throw Exception(_bus->name() + " needs a CPU");
        }

        class Type : public NamedNullary<::Connector::Type, Type>
        {
        public:
            static String name() { return "ISA8BitBus.CPUSocket"; }
        };
        ISA8BitBus* _bus;
    };

    void addComponent(ISA8BitComponentBase* component)
    {
        _components.add(component);
        component->setBus(this);
    }
    void setAddress(UInt32 address)
    {
        for (auto i : _components)
            i->setAddress(address);
    }
    void write(UInt8 data)
    {
        for (auto i : _components)
            if (i->active())
                i->write(data);
    }
    void write() { write(_data); }
    UInt8 read() const
    {
        for (auto i : _components)
            if (i->active())
                i->read();
        return _data;
    }
    UInt8 debugRead(UInt32 address)
    {
        UInt8 data = 0xff;
        for (auto i : _components)
            data &= i->debugRead(address);
        return data;
    }
    UInt8 data() const { return _data; }
private:
    UInt8 _data;
    CPUSocket _cpuSocket;
    Connector _connector;
    ChipConnector _chipConnectors[8];
    List<ISA8BitComponentBase*> _components;
    OutputConnector<bool> _parityError;

    friend class ISA8BitComponentBaseT<T>;
};
