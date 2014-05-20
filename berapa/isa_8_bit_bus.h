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
    TypedValue getValue(String name)
    {
        if (name == "bus")
            return &_connector;
        return ComponentTemplate<T>::getValue(name);
    }

    class BusConnector : public Connector
    {
    public:
        BusConnector(ISA8BitComponentTemplate<T>* component)
          : _component(component) { }
        void connect(Connector* other)
        {
            // TODO
        }
        Type type() const { return Type(); }

        class Type : public Connector::Type
        {
        public:
        private:
            class Implementation : public Connector::Type::Implementation
            {
            public:
                bool compatible(Connector::Type other) const
                {
                    // TODO
                }
            private:
            };
        };
    private:
        ISA8BitComponentTemplate<T>* _component;
    };

    class Type : public Component::Type
    {
    public:
        Type(Simulator* simulator)
          : Component::Type(new Implementation(simulator)) { }
        Type(const Implementation* implementation)
          : Component::Type(implementation) { }
    protected:
        class Implementation : public Component::Type::Implementation
        {
        public:
            Implementation(Simulator* simulator)
                : Component::Type::Implementation(simulator) { }
            bool has(String memberName) const
            { 
                if (memberName == "bus")
                    return true;
                return Component::Type::Implementation::has(memberName);
            }
        };
    };
protected:
    void set(UInt8 data) { _bus->_data = data; }
    ISA8BitBusTemplate<T>* _bus;
    bool _active;
    BusConnector _connector;
};

typedef ISA8BitComponentTemplate<void> ISA8BitComponent;

template<class T> class ISA8BitBusTemplate : public ComponentTemplate<T>
{
public:
    class Type : public Component::Type
    {
    public:
        Type(Simulator* simulator)
          : Component::Type(new Implementation(simulator)) { }
    private:
        class Implementation : public Component::Type::Implementation
        {
        public:
            Implementation(Simulator* simulator)
              : Component::Type::Implementation(simulator) { }
            String toString() const { return "ISA8BitBus"; }
        };
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
private:
    UInt8 _data;

    List<ISA8BitComponent*> _components;

    template<class U> friend class ISA8BitComponentTemplate;
};
