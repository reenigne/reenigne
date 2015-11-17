template<class T> class ISA8BitBusTemplate;
typedef ISA8BitBusTemplate<void> ISA8BitBus;

template<class T> class ISA8BitComponentBaseTemplate;
typedef ISA8BitComponentBaseTemplate<void> ISA8BitComponentBase;

template<class T> class ISA8BitComponentBaseTemplate : public ClockedComponent
{
public:
    ISA8BitComponentBaseTemplate() : _connector(this)
    {
        config("bus", &_connector);
        persist("active", &_active, false);
    }
    // Address bit 31 = write
    // Address bit 30 = IO
    virtual void setAddress(UInt32 address) { };
    virtual void write(UInt8 data) { };
    virtual bool wait() { return false; }
    void setBus(ISA8BitBus* bus) { _bus = bus; }
    virtual UInt8 memory(UInt32 address) { return 0xff; }
    bool active() const { return _active; }
    virtual void read() { }
    class Connector : public ::Connector
    {
    public:
        Connector(ISA8BitComponentBaseTemplate* component)
          : _component(component) { }
        void connect(::Connector* other)
        {
            dynamic_cast<ISA8BitBusTemplate<T>::Connector*>(other)
                ->busConnect(_component);
        }
        ::Connector::Type type() const { return Type(); }

        class Type : public NamedNullary<::Connector::Type, Type>
        {
        public:
            class Body : public NamedNullary<::Connector::Type, Type>::Body
            {
            public:
                bool compatible(::Connector::Type other) const
                {
                    return dynamic_cast<const ISA8BitBus::Type::Body*>(
                        other.body()) != 0;
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
    ISA8BitBus* _bus;
    bool _active;
    Connector _connector;
};

template<class C> class ISA8BitComponent : public ISA8BitComponentBase
{
public:
    typedef ClockedComponent::Type<C> Type;
    //class Type : public ClockedComponent::Type<C>
    //{
    //public:
    //    Type(Simulator* simulator)
    //      : ClockedComponent::Type<C>(new Body(simulator)) { }
    //protected:
    //    class Body : public ClockedComponent::Type<C>::Body
    //    {
    //    public:
    //        Body(Simulator* simulator)
    //          : ClockedComponent::Type::Body(simulator) { }
    //    };
    //    Type(const Body* body) : ClockedComponent::Type(body) { }
    //};
};

template<class T> class ISA8BitBusTemplate : public Component
{
public:
    static String typeName() { return "ISA8BitBus"; }

    ISA8BitBusTemplate() : _cpuSocket(this), _connector(this)
    {
        config("cpu", &_cpuSocket);
        config("slot", &_connector);
        for (int i = 0; i < 8; ++i) {
            _chipConnectors[i].init(this, i);
            config("chip" + decimal(i), &_chipConnectors[i]);
        }
    }

    class Connector : public ::Connector
    {
    public:
        Connector(ISA8BitBus* bus) : _bus(bus) { }
        void connect(::Connector* other) { }
        Type type() const { return Type(); }

        class Type : public NamedNullary<::Connector::Type, Type>
        {
        public:
            class Body : public NamedNullary<::Connector::Type, Type>::Body
            {
            public:
                bool compatible(::Connector::Type other) const
                {
                    return other == ISA8BitComponent::Connector::Type();
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
        ChipConnector() : Connector(0) { }
        void init(ISA8BitBus* bus, int i) { _bus = bus; _i = i; }
    private:
        int _i;
    };

    typedef Component::TypeHelper<ISA8BitBus> Type;

    //class Type : public Component::TypeHelper<ISA8BitBus>
    //{
    //public:
    //    Type(Simulator* simulator)
    //        : Component::TypeHelper<ISA8BitBus>(new Body(simulator)) { }
    //private:
    //    class Body : public Component::TypeHelper<ISA8BitBus>::Body
    //    {
    //    public:
    //        Body(Simulator* simulator)
    //          : Component::TypeHelper<ISA8BitBus>::Body(simulator) { }
    //    };
    //    //template<class U> friend class
    //    //    ISA8BitComponent<U>::Connector::Type::Body;
    //    //friend class ISA8BitBus::Connector::Type::Body;
    //};

    class CPUSocket : public ::Connector
    {
    public:
        CPUSocket(ISA8BitBus* bus) : _bus(bus) { }
        void connect(::Connector* other) { other->connect(this); }
        Type type() const { return Type(); }

        class Type : public NamedNullary<::Connector::Type, Type>
        {
        public:
            class Body : public NamedNullary<::Connector::Type, Type>::Body
            {
            public:
                bool compatible(::Connector::Type other) const
                {
                    return other == Intel8088CPU::Connector::Type();
                }
            };
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
    UInt8 memory(UInt32 address)
    {
        UInt8 data = 0xff;
        for (auto i : _components)
            data &= i->memory(address);
        return data;
    }
    String save() const { return hex(_data, 2) + "\n"; }
    virtual ::Type persistenceType() const { return IntegerType(); }
    virtual void load(const Value& value) { _data = value.value<int>(); }
    virtual Value initial() const { return 0xff; }
    UInt8 data() const { return _data; }
private:
    UInt8 _data;
    CPUSocket _cpuSocket;
    Connector _connector;
    ChipConnector _chipConnectors[8];
    List<ISA8BitComponentBase*> _components;

    friend class ISA8BitComponentBaseTemplate<T>;
};
