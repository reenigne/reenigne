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
    void readIORange(UInt32 low, UInt32 high)
    {
        _bus->addRange(2, this, low, high);
    }
    void writeIORange(UInt32 low, UInt32 high)
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
        _parityError(this), _noComponent(type), _readMemory(this),
        _writeMemory(this), _readIO(this), _writeIO(this)
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
            if (dynamic_cast<NoISA8BitComponent*>(component) == 0)
                _bus->addRange(2, component, _i*0x20, (_i + 1)*0x20);
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
        c->addRange(component, low, high, 0, end, this);

        // Balance the tree so that both subtrees cover roughly the same amount
        // of address space (without splitting components).
        c->balance(0, end);
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
        Choice(ISA8BitBus* bus)
          : ISA8BitComponentBase(bus->type(), true),
            _first(&bus->_noComponent), _second(&bus->_noComponent)
        { }
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
        void addRange(ISA8BitComponentBase* component, UInt32 low, UInt32 high,
            UInt32 start, UInt32 end, ISA8BitBus* bus)
        {
            auto none = &bus->_noComponent;
            if (_first == none && _second == none) {
                if (start == low)
                    _secondAddress = high;
                else
                    _secondAddress = low;
            }
            if (high <= _secondAddress) {
                addToBranch(&_first, component, low, high, start,
                    _secondAddress, bus);
            }
            else {
                if (low >= _secondAddress) {
                    addToBranch(&_second, component, low, high, _secondAddress,
                        end, bus);
                }
                else {
                    // Component overlaps both branches - split it.
                    addToBranch(&_first, component, low, _secondAddress, start,
                        _secondAddress, bus);
                    addToBranch(&_second, component, _secondAddress, high,
                        _secondAddress, end, bus);
                }
            }
        }
        void balance(UInt32 start, UInt32 end)
        {
            UInt32 mid = start + (end - start)/2;
            bool moved = false;
            if (_secondAddress > mid) {
                do {
                    UInt32 lowSplit = highSplit(_first, start);
                    if (lowSplit == start ||
                        abs((Int64)lowSplit - mid) >= _secondAddress - mid)
                        break;
                    moved = true;
                    // lowSplit is neither start nor _secondAddress so
                    // _first must be a Choice*.
                    static_cast<Choice*>(_first)->rotateHighestToSecond();
                    rotateRight();
                } while (true);
            }
            else {
                do {
                    UInt32 highSplit = lowSplit(_second, end);
                    if (highSplit == end ||
                        abs((Int64)highSplit - mid) >= mid - _secondAddress)
                        break;
                    moved = true;
                    // highSplit is neither end nor _secondAddress so
                    // _second must be a Choice*.
                    static_cast<Choice*>(_second)->rotateLowestToFirst();
                    rotateLeft();
                } while (true);
            }
            if (moved) {
                auto d = dynamic_cast<Choice*>(_first);
                if (d != 0)
                    d->balance(start, _secondAddress);
                d = dynamic_cast<Choice*>(_second);
                if (d != 0)
                    d->balance(_secondAddress, end);
            }
        }
        UInt32 _secondAddress;
        ISA8BitComponentBase* _first;
        ISA8BitComponentBase* _second;
    private:
        template<class C> C* create(ISA8BitBus* bus) const
        {
            auto r = Reference<Component>::create<C>(bus);
            bus->_treeComponents.add(r);
            return static_cast<C*>(&(*r));
        }
        void addToBranch(ISA8BitComponentBase** branch,
            ISA8BitComponentBase* component, UInt32 low, UInt32 high,
            UInt32 start, UInt32 end, ISA8BitBus* bus)
        {
            auto none = &bus->_noComponent;
            if (*branch == none) {
                if (start == low && end == high) {
                    *branch = component;
                    return;
                }
                *branch = create<Choice>(bus);
            }
            else {
                if (dynamic_cast<Choice*>(*branch) == 0) {
                    // Overlapping with existing component.
                    if (start == low && end == high) {
                        auto c = create<Combination>(bus);
                        // It doesn't matter if this is imbalanced - we need to
                        // walk the entire Combination tree to do any IO on the
                        // range anyway.
                        c->_first = *branch;
                        c->_second = component;
                        *branch = c;
                        return;
                    }
                    // Partially overlapping - split the range.
                    auto c = create<Choice>(bus);
                    c->_first = *branch;
                    c->_second = *branch;
                    if (start == low)
                        c->_secondAddress = high;
                    else
                        c->_secondAddress = low;
                    *branch = c;
                }
            }
            static_cast<Choice*>(*branch)->addRange(component, low, high,
                start, end, bus);
        }
        static UInt32 highSplit(ISA8BitComponentBase* component, UInt32 start)
        {
            auto c = dynamic_cast<Choice*>(component);
            if (c == 0)
                return start;
            return c->highSplit();
        }
        static UInt32 lowSplit(ISA8BitComponentBase* component, UInt32 end)
        {
            auto c = dynamic_cast<Choice*>(component);
            if (c == 0)
                return end;
            return c->lowSplit();
        }
        UInt32 highSplit() const { return highSplit(_second, _secondAddress); }
        UInt32 lowSplit() const { return lowSplit(_first, _secondAddress); }
        void rotateLeft()
        {
            auto o = static_cast<Choice*>(_second);
            _second = o->_second;
            o->_second = o->_first;
            o->_first = _first;
            _first = o;
            swap(_secondAddress, o->_secondAddress);
        }
        void rotateRight()
        {
            auto o = static_cast<Choice*>(_first);
            _first = o->_first;
            o->_first = o->_second;
            o->_second = _second;
            _second = o;
            swap(_secondAddress, o->_secondAddress);
        }
        void rotateHighestToSecond()
        {
            auto c = dynamic_cast<Choice*>(_second);
            if (c != 0) {
                c->rotateHighestToSecond();
                rotateLeft();
            }
        }
        void rotateLowestToFirst()
        {
            auto c = dynamic_cast<Choice*>(_first);
            if (c != 0) {
                c->rotateHighestToSecond();
                rotateLeft();
            }
        }
    };

    class Combination : public ISA8BitComponentBase
    {
    public:
        Combination(ISA8BitBus* bus) : ISA8BitComponentBase(bus->type(), true)
        { }
        ISA8BitComponentBase* setAddressReadMemory(Tick tick, UInt32 address)
        {
            _first->setAddressReadMemory(tick, address);
            _second->setAddressReadMemory(tick, address);
            return this;
        }
        ISA8BitComponentBase* setAddressWriteMemory(Tick tick, UInt32 address)
        {
            _first->setAddressWriteMemory(tick, address);
            _second->setAddressWriteMemory(tick, address);
            return this;
        }
        ISA8BitComponentBase* setAddressReadIO(Tick tick, UInt32 address)
        {
            _first->setAddressReadIO(tick, address);
            _second->setAddressReadIO(tick, address);
            return this;
        }
        ISA8BitComponentBase* setAddressWriteIO(Tick tick, UInt32 address)
        {
            _first->setAddressWriteIO(tick, address);
            _second->setAddressWriteIO(tick, address);
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

    // The roots of our trees are always Choice objects. In theory we could
    // have a single component responsible for the entire address range, but
    // we don't mind being sub-optimal in that case in order to avoid a virtual
    // function call in the common case.
    Choice _readMemory;
    Choice _writeMemory;
    Choice _readIO;
    Choice _writeIO;
    UInt32 _lowAddress[4];
    UInt32 _highAddress[4];
    NoISA8BitComponent _noComponent;

    friend class ISA8BitComponentBaseT<T>;
    friend class Choice;
};
