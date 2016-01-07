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
    }
    virtual ISA8BitComponentBase* setAddressReadMemory(Tick tick,
        UInt32 address)
    {
        return this;
    }
    virtual ISA8BitComponentBase* setAddressWriteMemory(Tick tick,
        UInt32 address)
    {
        return this;
    }
    virtual ISA8BitComponentBase* setAddressReadIO(Tick tick, UInt32 address)
    {
        return this;
    }
    virtual ISA8BitComponentBase* setAddressWriteIO(Tick tick, UInt32 address)
    {
        return this;
    }
    virtual UInt8 readMemory(Tick tick) { }
    virtual void writeMemory(Tick tick, UInt8 data) { }
    virtual UInt8 readIO(Tick tick) { }
    virtual void writeIO(Tick tick, UInt8 data) { }
    virtual bool wait() { return false; }
    void setBus(ISA8BitBus* bus) { _bus = bus; }
    virtual UInt8 debugReadMemory(UInt32 address) { return 0xff; }

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
    ISA8BitBusT<T>* _bus;
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
        for (int i = 0; i < 8; ++i) {
            _chipConnectors[i].init(i);
            connector("chip" + decimal(i), &_chipConnectors[i]);
        }
        connector("parityError", &_parityError);
        persist("activeAddress", &_activeAddress);
        persist("activeAccess", &_activeAccess);
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
    ISA8BitComponentBase* setAddressReadMemory(Tick tick, UInt32 address)
    {
        _activeAddress = address;
        _activeAccess = 0;
        _activeComponent = _readMemory.setAddressReadMemory(tick, address);
        return _activeComponent;
    }
    ISA8BitComponentBase* setAddressWriteMemory(Tick tick, UInt32 address)
    {
        _activeAddress = address;
        _activeAccess = 1;
        _activeComponent = _readMemory.setAddressWriteMemory(tick, address);
        return _activeComponent;
    }
    ISA8BitComponentBase* setAddressReadIO(Tick tick, UInt16 address)
    {
        _activeAddress = address;
        _activeAccess = 2;
        _activeComponent = _readMemory.setAddressReadIO(tick, address);
        return _activeComponent;
    }
    ISA8BitComponentBase* setAddressWriteIO(Tick tick, UInt16 address)
    {
        _activeAddress = address;
        _activeAccess = 3;
        _activeComponent = _readMemory.setAddressWriteIO(tick, address);
        return _activeComponent;
    }
    UInt8 readMemory(Tick tick) const
    {
        return _activeComponent->readMemory(tick);
    }
    void writeMemory(Tick tick, UInt8 data)
    {
        _activeComponent->writeMemory(tick, data);
    }
    UInt8 readIO(Tick tick) const
    {
        return _activeComponent->readIO(tick);
    }
    void writeIO(Tick tick, UInt8 data)
    {
        _activeComponent->writeIO(tick, data);
    }
    UInt8 debugReadMemory(UInt32 address)
    {
        UInt8 data = 0xff;
        for (auto i : _components)
            data &= i->debugReadMemory(address);
        return data;
    }
    void load(Value v)
    {
        Component::load(v);
        switch (_activeAccess) {
            case 0: setAddressReadMemory()
        }
    }
private:
    CPUSocket _cpuSocket;
    Connector _connector;
    ChipConnector _chipConnectors[8];
    List<ISA8BitComponentBase*> _components;
    OutputConnector<bool> _parityError;
    UInt32 _activeAddress;
    int _activeAccess;
    Tick _accessTick;
    ISA8BitComponentBase* _activeComponent;

    class Choice : public ISA8BitComponentBase
    {
    public:
        ISA8BitComponentBase* setAddressReadMemory(Tick tick, UInt32 address)
        {
            if (address < _secondAddress)
                return _first->setAddressReadMemory(tick, address);
            return _second->setAddressReadMemory(tick, address);
        }
        ISA8BitComponentBase* setAddressWriteMemory(Tick tick, UInt32 address)
        {
            if (address < _secondAddress)
                return _first->setAddressWriteMemory(tick, address);
            return _second->setAddressWriteMemory(tick, address);
        }
        ISA8BitComponentBase* setAddressReadIO(Tick tick, UInt32 address)
        {
            if (address < _secondAddress)
                return _first->setAddressReadIO(tick, address);
            return _second->setAddressReadIO(tick, address);
        }
        ISA8BitComponentBase* setAddressWriteIO(Tick tick, UInt32 address)
        {
            if (address < _secondAddress)
                return _first->setAddressWriteIO(tick, address);
            return _second->setAddressWriteIO(tick, address);
        }
        UInt32 _secondAddress;
        ISA8BitComponentBase* _first;
        ISA8BitComponentBase* _second;
    };

    class Combination : public ISABitComponentBase
    {
    public:
        ISA8BitComponentBase* setAddressReadMemory(Tick tick, UInt32 address)
        {
            _first-setAddressReadMemory(tick, address);
            _second-setAddressReadMemory(tick, address);
            return this;
        }
        ISA8BitComponentBase* setAddressWriteMemory(Tick tick, UInt32 address)
        {
            _first-setAddressWriteMemory(tick, address);
            _second-setAddressWriteMemory(tick, address);
            return this;
        }
        ISA8BitComponentBase* setAddressReadIO(Tick tick, UInt32 address)
        {
            _first-setAddressReadIO(tick, address);
            _second-setAddressReadIO(tick, address);
            return this;
        }
        ISA8BitComponentBase* setAddressWriteIO(Tick tick, UInt32 address)
        {
            _first-setAddressWriteIO(tick, address);
            _second-setAddressWriteIO(tick, address);
            return this;
        }
        UInt8 readMemory(Tick tick)
        {
            return _first->readMemory(tick) & _second->readMemory(tick);
        }
        void writeMemory(Tick tick, UInt8 data)
        {
            _first->writeMemory(tick, data);
            _second->writeMemory(tick, data);
        }
        UInt8 readIO(Tick tick)
        {
            return _first->readIO(tick) & _second->readIO(tick);
        }
        void writeIO(Tick tick, UInt8 data)
        {
            _first->writeIO(tick, data);
            _second->writeIO(tick, data);
        }
        ISA8BitComponentBase* _first;
        ISA8BitComponentBase* _second;
    };

    Choice _readMemory;
    Choice _writeMemory;
    Choice _readIO;
    Choice _writeIO;

    friend class ISA8BitComponentBaseT<T>;
};
