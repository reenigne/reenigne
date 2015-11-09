template<class T> class ISA8BitComponentTemplate;
typedef ISA8BitComponentTemplate<void> ISA8BitComponent;

template<class T> class ISA8BitBusTemplate;
typedef ISA8BitBusTemplate<void> ISA8BitBus;

template<class T> class ISA8BitComponentTemplate : public ComponentTemplate<T>
{
public:
    ISA8BitComponentTemplate() : _connector(this) { }
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
    void set(String name, Value value)
    {
        if (name == "bus") {
            Connector* other = value.value<Connector*>();
            _connector.connect(other);
            other->connect(&_connector);
        }
        return ComponentTemplate<T>::set(name, value);
    }
    Value getValue(Identifier i)
    {
        if (i.name() == "bus")
            return _connector.getValue();
        return Component::getValue(i);
    }

    class Connector : public ::Connector
    {
    public:
        Connector(ISA8BitComponent* component) : _component(component) { }
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
        ISA8BitComponent* _component;
    };

    class Type : public Component::Type
    {
    public:
        Type(Simulator* simulator) : Component::Type(new Body(simulator)) { }
        Type(const Body* body) : Component::Type(body) { }
    protected:
        class Body : public Component::Type::Body
        {
        public:
            Body(Simulator* simulator) : Component::Type::Body(simulator) { }
            ::Type member(Identifier i) const
            {
                if (i.name() == "bus")
                    return Connector::Type();
                return Component::Type::Body::member(i);
            }
        };
    };
protected:
    void set(UInt8 data) { _bus->_data = data; }
    ISA8BitBusTemplate<T>* _bus;
    bool _active;
    Connector _connector;
};

template<> Nullary<Connector::Type, ISA8BitComponent::Connector::Type>
    Nullary<Connector::Type, ISA8BitComponent::Connector::Type>::_instance;

template<class T> class ISA8BitBusTemplate : public ComponentTemplate<T>
{
public:
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
                return Component::Type::Body::member(i);
            }
            Reference<Component> createComponent() const
            {
                return Reference<Component>::create<ISA8BitBus>();
            }
        };
        template<class U> friend class
            ISA8BitComponentTemplate<U>::Connector::Type::Body;
        friend class ISA8BitBusTemplate::Connector::Type::Body;
    };

    class Connector : public ::Connector
    {
    public:
        Connector(ISA8BitBus* bus) : _bus(bus) { }
        void connect(Connector* other) { other->connect(this); }
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
        void busConnect(ISA8BitComponent* component)
        {
            _bus->addComponent(component);
        }

        ISA8BitBus* _bus;

        template<class U> friend class ISA8BitComponentTemplate<U>::Connector;
    };

    class CPUSocket : public ::Connector
    {
    public:
        CPUSocket(ISA8BitBus* bus) : _bus(bus) { }
        void connect(Connector* other) { other->connect(this); }
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
    //private:
        ISA8BitBus* _bus;
    };

    void addComponent(ISA8BitComponent* component)
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
    //Component::Type type() const { return _type; }
private:
    //ISA8BitBusTemplate(Type type) : _type(type) { }

    UInt8 _data;

    List<ISA8BitComponent*> _components;

    //Type _type;

    template<class U> friend class ISA8BitComponentTemplate;
};

template<> Nullary<Connector::Type, ISA8BitBus::CPUSocket::Type>
    Nullary<Connector::Type, ISA8BitBus::CPUSocket::Type>::_instance;
