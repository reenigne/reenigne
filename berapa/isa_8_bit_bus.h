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
    void set(String name, TypedValue value)
    {
        if (name == "bus") {
            Connector* other = value.value<Connector*>();
            _connector.connect(other);
            other->connect(&_connector);
        }
        return ComponentTemplate<T>::set(name, value);
    }
    TypedValue getValue(Identifier i)
    {
        if (i.name() == "bus")
            return _connector.getValue();
        return TypedValue();
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
            bool has(String memberName) const
            { 
                if (memberName == "bus")
                    return true;
                return Component::Type::Body::has(memberName);
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
            Body(Simulator* simulator)
              : Component::Type::Body(simulator) { }
            String toString() const { return "ISA8BitBus"; }
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
                    return
                        dynamic_cast<const ISA8BitBus::Type::Body*>(
                            other.body()) != 0;
                }
            private:
            };
            static String name() { return "ISA8BitBusConnector"; }

        };
    private:
        void busConnect(ISA8BitComponent* component)
        {
            _bus->addComponent(component);
        }

        ISA8BitBus* _bus;

        template<class U> friend class ISA8BitComponentTemplate<U>::Connector;
    };

    void addComponent(ISA8BitComponent* component)
    {
        _components.add(component);
        this->_simulator->addComponent(component);
        component->setBus(this);
    }
    void setAddress(UInt32 address)
    {
        for (auto i = _components.begin(); i != _components.end(); ++i)
            (*i)->setAddress(address);
    }
    void write(UInt8 data)
    {
        for (auto i = _components.begin(); i != _components.end(); ++i)
            if ((*i)->active())
                (*i)->write(data);
    }
    void write() { write(_data); }
    UInt8 read() const
    {
        for (auto i = _components.begin(); i != _components.end(); ++i)
            if ((*i)->active())
                (*i)->read();
        return _data;
    }
    UInt8 memory(UInt32 address)
    {
        UInt8 data = 0xff;
        for (auto i = _components.begin(); i != _components.end(); ++i)
            data &= (*i)->memory(address);
        return data;
    }
    String save() const { return hex(_data, 2) + "\n"; }
    virtual ::Type persistenceType() const { return IntegerType(); }
    virtual void load(const TypedValue& value) { _data = value.value<int>(); }
    virtual TypedValue initial() const { return 0xff; }
    UInt8 data() const { return _data; }
    Component::Type type() const { return _type; }
private:
    ISA8BitBusTemplate(Type type) : _type(type) { }

    UInt8 _data;

    List<ISA8BitComponent*> _components;

    Type _type;

    template<class U> friend class ISA8BitComponentTemplate;
};
