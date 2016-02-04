template<class T> class ISA8BitBusT;
typedef ISA8BitBusT<void> ISA8BitBus;

template<class T> class ISA8BitComponentT;
typedef ISA8BitComponentT<void> ISA8BitComponent;

class ISA8BitProtocol : public ProtocolBase<ISA8BitProtocol> { };
class DMAPageRegistersProtocol : public ProtocolBase<DMAPageRegistersProtocol>
{ };
class CPU8088Protocol : public ProtocolBase<CPU8088Protocol> { };

template<class T> class ISA8BitComponentT : public ClockedComponent
{
public:
    ISA8BitComponentT(Component::Type type, bool noConnectorName)
      : ClockedComponent(type), _connector(this)
    {
        if (noConnectorName)
            connector("", &_connector);
        else
            connector("bus", &_connector);
    }
    virtual ISA8BitComponent* setAddressReadMemory(Tick tick,
        UInt32 address)
    {
        return this;
    }
    virtual ISA8BitComponent* setAddressWriteMemory(Tick tick,
        UInt32 address)
    {
        return this;
    }
    virtual ISA8BitComponent* setAddressReadIO(Tick tick, UInt32 address)
    {
        return this;
    }
    virtual ISA8BitComponent* setAddressWriteIO(Tick tick, UInt32 address)
    {
        return this;
    }
    virtual ISA8BitComponent* getComponent(UInt32 address) { return this; }
    virtual UInt8 readMemory(Tick tick) { return 0xff; }
    virtual void writeMemory(Tick tick, UInt8 data) { }
    virtual UInt8 readIO(Tick tick) { return 0xff; }
    virtual void writeIO(Tick tick, UInt8 data) { }
    virtual bool wait() { return false; }
    virtual void setBus(ISA8BitBus* bus) { _bus = bus; }
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

    class Connector : public ConnectorBase<Connector>
    {
    public:
        Connector(ISA8BitComponentT* component)
          : ConnectorBase<Connector>(component), _component(component) { }
        void connect(::Connector* other)
        {
            dynamic_cast<typename ISA8BitBusT<T>::Connector*>(other)
                ->busConnect(_component);
        }
        static String typeName() { return "ISA8BitConnector"; }
        static auto protocolDirection()
        {
            return ProtocolDirection(ISA8BitProtocol(), false);
        }
    private:
        ISA8BitComponent* _component;
    };

protected:
    ISA8BitBusT<T>* _bus;
    Connector _connector;
};

template<class C> class ISA8BitComponentBase : public ISA8BitComponent
{
public:
    ISA8BitComponentBase(Component::Type type, bool noConnectorName = false)
      : ISA8BitComponent(type, noConnectorName) { }
    typedef typename ClockedComponentBase<C>::Type Type;
};

class NoISA8BitComponent : public ISA8BitComponentBase<NoISA8BitComponent>
{
public:
    static String typeName() { return "NoISA8BitComponent"; }
    NoISA8BitComponent(Component::Type type)
      : ISA8BitComponentBase<NoISA8BitComponent>(type, true) { }
};

template<class T> class ISA8BitBusT : public ComponentBase<ISA8BitBusT<T>>
{
public:
    static String typeName() { return "ISA8BitBus"; }

    ISA8BitBusT(Component::Type type)
      : ComponentBase<ISA8BitBusT<T>>(type), _cpuSocket(this),
        _connector(this),
        _chipConnectors{this, this, this, this, this, this, this, this},
        _parityError(this), _noComponent(type), _readMemory(this),
        _writeMemory(this), _readIO(this), _writeIO(this),
        _dmaPageRegistersSocket(this)
    {
        this->connector("cpu", &_cpuSocket);
        this->connector("slot", &_connector);
        for (int i = 0; i < 8; ++i) {
            _chipConnectors[i].init(i);
            this->connector("chip" + decimal(i), &_chipConnectors[i]);
        }
        this->connector("parityError", &_parityError);
        this->connector("dmaPageRegisters", &_dmaPageRegistersSocket);
        this->persist("activeAddress", &_activeAddress);
        this->persist("activeAccess", &_activeAccess);
    }

    class Connector : public ConnectorBase<Connector>
    {
    public:
        Connector(ISA8BitBus* bus)
          : ConnectorBase<Connector>(bus), _bus(bus) { }
        static String typeName() { return "ISA8BitBus.Connector"; }
        static auto protocolDirection()
        {
            return ProtocolDirection(ISA8BitProtocol(), true);
        }
        static auto canConnectMultiple() { return true; }
    protected:
        ISA8BitBus* _bus;

        virtual void busConnect(ISA8BitComponent* component)
        {
            _bus->addComponent(component);
        }
        template<class U> friend class ISA8BitComponentT<U>::Connector;
    };
    class ChipConnector : public Connector
    {
    public:
        ChipConnector(ISA8BitBusT<T>* bus) : Connector(bus) { }
        void init(int i) { _i = i; }
        void busConnect(ISA8BitComponent* component)
        {
            if (dynamic_cast<NoISA8BitComponent*>(component) == 0) {
                this->_bus->addRange(2, component, _i*0x20, (_i + 1)*0x20);
                this->_bus->addRange(3, component, _i*0x20, (_i + 1)*0x20);
            }
        }
    private:
        int _i;
    };

    class CPUSocket : public ConnectorBase<CPUSocket>
    {
    public:
        CPUSocket(ISA8BitBus* bus)
          : ConnectorBase<CPUSocket>(bus), _bus(bus) { }
        static String typeName() { return "ISA8BitBus.CPUSocket"; }
        static auto protocolDirection()
        {
            return ProtocolDirection(CPU8088Protocol(), false);
        }
        ISA8BitBus* _bus;
    };

    class DMAPageRegistersSocket : public ConnectorBase<DMAPageRegistersSocket>
    {
    public:
        DMAPageRegistersSocket(ISA8BitBus* bus)
          : ConnectorBase<DMAPageRegistersSocket>(bus), _bus(bus) { }
        static String typeName()
        {
            return "ISA8BitBus.DMAPageRegistersSocket";
        }
        static auto protocolDirection()
        {
            return ProtocolDirection(DMAPageRegistersProtocol(), false);
        }
        ISA8BitBus* _bus;
    };

    void addComponent(ISA8BitComponent* component)
    {
        _components.add(component);
        component->setBus(this);
    }
    void setDMAAddressRead(Tick tick, UInt16 address, int channel)
    {
        setAddressReadMemory(tick, address | highAddress(tick, channel));
    }
    void setDMAAddressWrite(Tick tick, UInt16 address, int channel)
    {
        setAddressWriteMemory(tick, address | highAddress(tick, channel));
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
    void load(const Value& v)
    {
        Component::load(v);
        // _activeAccess, _activeAddress and ISA8BitComponent::getComponent()
        // only exist for the purposes of persisting _activeComponent.
        _activeComponent =
            choiceForAccess(_activeAccess)->getComponent(_activeAddress);
    }
    void addRange(int access, ISA8BitComponent* component, UInt32 low,
        UInt32 high)
    {
        Choice* c = choiceForAccess(access);
        UInt32 end = (access < 2 ? 0x100000 : 0x10000);
        c->addRange(component, low, high, 0, end, this);

        // Balance the tree so that both subtrees cover roughly the same amount
        // of address space (without splitting components).
        c->balance(low, high, 0, end);
    }
    void setDMAPageRegisters(DMAPageRegisters* c) { _dmaPageRegisters = c; }
    void setDMAC(Intel8237DMAC* dmac) { _dmac = dmac; }
    void setDMATick(Tick tick) { _dmaTick = tick; }

    // The bus has special knowledge of the PIC because the PIC puts values on
    // the bus via a special "interrupt acknowledge" mechanism that is neither
    // memory nor port access. This is implemented by just allowing the CPU to
    // get a pointer to the PIC, at least for now.
    void setPIC(Intel8259PIC* pic) { _pic = pic; }
    Intel8259PIC* getPIC() { return _pic; }
    void maintain(Tick ticks) { _dmaTick -= ticks; }
private:
    UInt32 highAddress(Tick tick, int channel)
    {
        this->_pageRegisters->runTo(tick);
        return this->_pageRegisters->pageForChannel(channel) << 16;
    }

    CPUSocket _cpuSocket;
    DMAPageRegistersSocket _dmaPageRegistersSocket;
    Connector _connector;
    ChipConnector _chipConnectors[8];
    List<ISA8BitComponent*> _components;
    OutputConnector<bool> _parityError;
    UInt32 _activeAddress;
    int _activeAccess;
    Tick _accessTick;
    ISA8BitComponent* _activeComponent;
    List<Reference<Component>> _treeComponents;
    Intel8237DMAC* _dmac;
    DMAPageRegisters* _dmaPageRegisters;
    Intel8259PIC* _pic;
    Tick _dmaTick;

    class Choice : public ISA8BitComponent
    {
    public:
        Choice(ISA8BitBus* bus)
          : ISA8BitComponent(bus->type(), true), _first(&bus->_noComponent),
            _second(&bus->_noComponent)
        { }
        ISA8BitComponent* setAddressReadMemory(Tick tick, UInt32 address)
        {
            return getComponent(address)->setAddressReadMemory(tick, address);
        }
        ISA8BitComponent* setAddressWriteMemory(Tick tick, UInt32 address)
        {
            return getComponent(address)->setAddressWriteMemory(tick, address);
        }
        ISA8BitComponent* setAddressReadIO(Tick tick, UInt32 address)
        {
            return getComponent(address)->setAddressReadIO(tick, address);
        }
        ISA8BitComponent* setAddressWriteIO(Tick tick, UInt32 address)
        {
            return getComponent(address)->setAddressWriteIO(tick, address);
        }
        UInt8 debugReadMemory(UInt32 address)
        {
            return getComponent(address)->debugReadMemory(address);
        }
        ISA8BitComponent* getComponent(UInt32 address) final
        {
            if (address < _secondAddress)
                return _first;
            return _second;
        }
        void addRange(ISA8BitComponent* component, UInt32 low, UInt32 high,
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
        void balance(UInt32 low, UInt32 high, UInt32 start, UInt32 end)
        {
            UInt32 mid = start + (end - start)/2;
            bool moved = false;
            if (_secondAddress > mid) {
                do {
                    UInt32 lowSplit = highSplit(_first, start);
                    if (lowSplit == start || _secondAddress < mid ||
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
                    if (highSplit == end || _secondAddress > mid ||
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
                if (low > start)
                    low = start;
                if (high < end)
                    high = end;
            }
            if (low < _secondAddress && high > start) {
                auto d = dynamic_cast<Choice*>(_first);
                if (d != 0)
                    d->balance(low, high, start, _secondAddress);
            }
            if (high > _secondAddress && low < end) {
                auto d = dynamic_cast<Choice*>(_second);
                if (d != 0)
                    d->balance(low, high, _secondAddress, end);
            }
        }
        UInt32 _secondAddress;
        ISA8BitComponent* _first;
        ISA8BitComponent* _second;
    private:
        template<class C> C* create(ISA8BitBus* bus) const
        {
            auto r = Reference<Component>::create<C>(bus);
            bus->_treeComponents.add(r);
            return static_cast<C*>(&(*r));
        }
        void addToBranch(ISA8BitComponent** branch,
            ISA8BitComponent* component, UInt32 low, UInt32 high, UInt32 start,
            UInt32 end, ISA8BitBus* bus)
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
        static UInt32 highSplit(ISA8BitComponent* component, UInt32 start)
        {
            auto c = dynamic_cast<Choice*>(component);
            if (c == 0)
                return start;
            return c->highSplit();
        }
        static UInt32 lowSplit(ISA8BitComponent* component, UInt32 end)
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

    class Combination : public ISA8BitComponent
    {
    public:
        Combination(ISA8BitBus* bus) : ISA8BitComponent(bus->type(), true)
        { }
        ISA8BitComponent* setAddressReadMemory(Tick tick, UInt32 address)
        {
            _first->setAddressReadMemory(tick, address);
            _second->setAddressReadMemory(tick, address);
            return this;
        }
        ISA8BitComponent* setAddressWriteMemory(Tick tick, UInt32 address)
        {
            _first->setAddressWriteMemory(tick, address);
            _second->setAddressWriteMemory(tick, address);
            return this;
        }
        ISA8BitComponent* setAddressReadIO(Tick tick, UInt32 address)
        {
            _first->setAddressReadIO(tick, address);
            _second->setAddressReadIO(tick, address);
            return this;
        }
        ISA8BitComponent* setAddressWriteIO(Tick tick, UInt32 address)
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
        ISA8BitComponent* _first;
        ISA8BitComponent* _second;
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

    friend class ISA8BitComponentT<T>;
    friend class Choice;
};
