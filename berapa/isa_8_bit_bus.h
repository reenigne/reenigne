template<class T> class ISA8BitBusTemplate;
typedef ISA8BitBusTemplate<void> ISA8BitBus;

template<class T> class ISA8BitComponentBaseTemplate;
typedef ISA8BitComponentBaseTemplate<void> ISA8BitComponentBase;

template<class T> class ISA8BitComponentBaseTemplate : public ClockedComponent
{
public:
    ISA8BitComponentBaseTemplate() : _connector(this) { }
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
    Value getValue(Identifier i) const
    {
        if (i.name() == "bus")
            return _connector.getValue();
        return Component::getValue(i);
    }

    class Connector : public ::Connector
    {
    public:
        Connector(ISA8BitComponentBase* component) : _component(component) { }
        void connect(::Connector* other)
        {
            dynamic_cast<ISA8BitBus::Connector*>(other)
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
            static String name() { return "ISA8BitBusConnector"; }
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

template<class T> class ISA8BitComponent : public ISA8BitComponentBase
{
public:
    class Type : public ClockedComponent::Type
    {
    public:
        Type(Simulator* simulator)
          : ClockedComponent::Type(new Body(simulator)) { }
    protected:
        class Body : public ClockedComponent::Type::Body
        {
        public:
            Body(Simulator* simulator)
              : ClockedComponent::Type::Body(simulator) { }
            ::Type member(Identifier i) const
            {
                if (i.name() == "bus")
                    return Connector::Type();
                return ClockedComponent::Type::Body::member(i);
            }
            String toString() const { return T::name(); }
            Reference<Component> createComponent() const
            {
                return Reference<Component>::create<T>();
            }
        };
        Type(const Body* body) : ClockedComponent::Type(body) { }
    };
};

template<class T> class ISA8BitBusTemplate : public ComponentTemplate<T>
{
public:
    ISA8BitBusTemplate() : _cpuSocket(this), _connector(this) { }
    Value getValue(Identifier i) const
    {
        if (i.name() == "cpu")
            return _cpuSocket.getValue();
        if (i.name() == "slot")
            return _connector.getValue();
        return Component::getValue(i);
    }

    class Type : public Component::Type
    {
    public:
        Type(Simulator* simulator)
          : Component::Type(new Body(simulator)) { }
    private:
        class Body : public Component::Type::Body
        {
        public:
            Body(Simulator* simulator) : Component::Type::Body(simulator) { }
            String toString() const { return "ISA8BitBus"; }
            ::Type member(Identifier i) const
            {
                if (i.name() == "cpu")
                    return CPUSocket::Type();
                if (i.name() == "slot")
                    return Connector::Type();
                return Component::Type::Body::member(i);
            }
            Reference<Component> createComponent() const
            {
                return Reference<Component>::create<ISA8BitBus>();
            }
        };
        template<class U> friend class
            ISA8BitComponent<U>::Connector::Type::Body;
        friend class ISA8BitBusTemplate::Connector::Type::Body;
    };

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
                    return other == ISA8BitComponentBase::Connector::Type();
                }
            };
            static String name() { return "ISA8BitBus.Connector"; }
        };
    private:
        void busConnect(ISA8BitComponentBase* component)
        {
            _bus->addComponent(component);
        }

        ISA8BitBus* _bus;

        template<class U> friend class ISA8BitComponent<U>::Connector;
    };

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
                    return other == Intel8088::Connector::Type();
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
    List<ISA8BitComponentBase*> _components;

    template<class U> friend class ISA8BitComponentBaseTemplate;
};
