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
    virtual ISA8BitComponentBase* getComponent(UInt32 address) { return this; }
    virtual UInt8 readMemory(Tick tick) { return 0xff; }
    virtual void writeMemory(Tick tick, UInt8 data) { }
    virtual UInt8 readIO(Tick tick) { return 0xff; }
    virtual void writeIO(Tick tick, UInt8 data) { }
    virtual bool wait() { return false; }
    void setBus(ISA8BitBus* bus) { _bus = bus; }
    virtual UInt8 debugReadMemory(UInt32 address) { return 0xff; }
    void readMemoryRange(UInt32 low, UInt32 high)
    {
        _bus->addRange(0, this, low, high);
    }
    void writeMemoryRange(UInt32 low, UInt32 high)
    {
        _bus->addRange(1, this, low, high);
    }
    void readMemoryRange(UInt32 low, UInt32 high)
    {
        _bus->addRange(2, this, low, high);
    }
    void writeMemoryRange(UInt32 low, UInt32 high)
    {
        _bus->addRange(3, this, low, high);
    }

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
        _parityError(this), _noComponent(Component::Type())
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
    protected:
        ISA8BitBus* _bus;

        virtual void busConnect(ISA8BitComponentBase* component)
        {
            _bus->addComponent(component);
        }
        template<class U> friend class ISA8BitComponent<U>::Connector;
    };
    class ChipConnector : public Connector
    {
    public:
        ChipConnector(ISA8BitBusT<T>* bus) : Connector(bus) { }
        void init(int i) { _i = i; }
        void busConnect(ISA8BitComponentBase* component)
        {
            _bus->addRange(2, component, i*0x20, (i + 1)*0x20);
        }
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
    void setAddressReadMemory(Tick tick, UInt32 address)
    {
        _activeAddress = address;
        _activeAccess = 0;
        _activeComponent = _readMemory.setAddressReadMemory(tick, address);
    }
    void setAddressWriteMemory(Tick tick, UInt32 address)
    {
        _activeAddress = address;
        _activeAccess = 1;
        _activeComponent = _writeMemory.setAddressWriteMemory(tick, address);
    }
    void setAddressReadIO(Tick tick, UInt16 address)
    {
        _activeAddress = address;
        _activeAccess = 2;
        _activeComponent = _readIO.setAddressReadIO(tick, address);
    }
    void setAddressWriteIO(Tick tick, UInt16 address)
    {
        _activeAddress = address;
        _activeAccess = 3;
        _activeComponent = _writeIO.setAddressWriteIO(tick, address);
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
        return _readMemory.debugReadMemory(address);
    }
    void load(Value v)
    {
        Component::load(v);
        // _activeAccess, _activeAddress and ISA8BitComponent::getComponent()
        // only exist for the purposes of persisting _activeComponent.
        _activeComponent =
            choiceForAccess(_activeAccess)->getComponent(_activeAddress);
    }
    void addRange(int access, ISA8BitComponentBase* component, UInt32 low,
        UInt32 high)
    {
        Choice* c = choiceForAccess(access);
        UInt32 end = (access < 2 ? 0x100000 : 0x10000);
        if (c->_first == 0 && c->_second == 0) {
            // Create tree that satisfies invariants:
            //   Root is always a choice (in theory we could have a single
            //    component responsible for the entire address range, but we
            //    don't mind being sub-optimal in this case to avoid a pointer
            //    dereference in the common case).
            //   Tree spans entire address range (addresses not corresponding
            //    to a component are assigned &_noComponent).
            //   A depth-first traversal of the tree yields the components in
            //    address order.
            //   Partially overlapping components are split and the overlapping
            //    range replaced with a Combination.
            if (low == 0) {
                // Component at beginning of address space
                c->_first = component;
                c->_secondAddress = high;
                c->_second = &_noComponent;
            }
            else {
                c->_first = &_noComponent;
                c->_secondAddress = low;
                if (high == end) {
                    // Component at end of address space
                    c->_second = component;
                }
                else {
                    // Component in middle of address space
                    auto r = Reference<Component>::create<Choice>();
                    _treeComponents.add(r);
                    Choice* sc = &(*r);
                    c->_second = sc;
                    sc->_first = component;
                    sc->_secondAddress = high;
                    sc->_second = &_noComponent;
                }
            }
        }
        else {
            // TODO: add the range to the tree
        }
        // There is one more invariant that we want the final tree to have - we
        // want to balance the tree such that both subtrees of each nodes cover
        // roughly equal amounts of address space (without splitting
        // components).

        // TODO: adjust the tree to maintain this invariant.
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
    List<Reference<Component>> _treeComponents;

    class Choice : public ISA8BitComponentBase
    {
    public:
        Choice() : _first(0), _second(0) { }
        ISA8BitComponentBase* setAddressReadMemory(Tick tick, UInt32 address)
        {
            return getComponent(address)->setAddressReadMemory(tick, address);
        }
        ISA8BitComponentBase* setAddressWriteMemory(Tick tick, UInt32 address)
        {
            return getComponent(address)->setAddressWriteMemory(tick, address);
        }
        ISA8BitComponentBase* setAddressReadIO(Tick tick, UInt32 address)
        {
            return getComponent(address)->setAddressReadIO(tick, address);
        }
        ISA8BitComponentBase* setAddressWriteIO(Tick tick, UInt32 address)
        {
            return getComponent(address)->setAddressWriteIO(tick, address);
        }
        UInt8 debugReadMemory(UInt32 address)
        {
            return getComponent(address)->debugReadMemory(address);
        }
        ISA8BitComponentBase* getComponent(UInt32 address) final
        {
            if (address < _secondAddress)
                return _first;
            return _second;
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
        UInt8 debugReadMemory(UInt32 address)
        {
            return _first->debugReadMemory(address) &
                _second->debugReadMemory(address);
        }
        ISA8BitComponentBase* _first;
        ISA8BitComponentBase* _second;
    };
    Choice* choiceForAccess(int access)
    {
        switch (access) {
            case 0: return &_readMemory;
            case 1: return &_writeMemory;
            case 2: return &_readIO;
            case 3: return &_writeIO;
        }
        assert(false);
        return 0;
    }

    Choice _readMemory;
    Choice _writeMemory;
    Choice _readIO;
    Choice _writeIO;
    UInt32 _lowAddress[4];
    UInt32 _highAddress[4];
    NoISA8BitComponent _noComponent;

    friend class ISA8BitComponentBaseT<T>;
};
