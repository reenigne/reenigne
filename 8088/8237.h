template<class T> class Intel8237DMATemplate
  : public ISA8BitComponentTemplate<T>
{
public:
    Rational<int> hDotsPerCycle() const { return 3; }
    Intel8237DMATemplate()
    {
        _activechannel = 0;
    }
    void site()
    {
        for(int i = 0; i < 4; ++i)
        {
            _channels[i]._bus = this->_simulator->getBus();
        }
    }
    void simulateCycle()
    {
        for(int i = 0; i < 4; ++i)
        {
            _channels[i].simulateCycle();
        }
    }
    void setAddress(UInt32 address)
    {
        _address = address & 0xf;
        this->_active = (address & 0x400003f0) == 0x40000000;
    }
    void read()
    {
        if(_address < 8)
        {
            this->set(_channels[_address >> 1].read(_address & 1));
        }
    }
    void write(UInt8 data)
    {
        if(_address < 8)
        {
            _channels[_address >> 1].write(_address & 1,data);
        }
        switch(_address)
        {
            case 0x08: _command = data; break;
            case 0x0B:
            {
                _channels[data & 3]._mode = data & 0xFC;
                break;
            }
        }
    }
    // Step 1: the device calls dmaRequest()
    // equivalent to raising a DRQ line.
    void dmaRequest(int channel)
    {
        // TODO
        if (disabled())
            return;
        if(_channels[channel]._state != Channel::State::stateIdle) return;
        _channels[channel]._state = Channel::State::stateS0;
        _activechannel = channel;
    }
    // Step 2: at the end of the IO cycle the CPU calls dmaRequested()
    // equivalent to checking the status of the READY line and raising the HLDA
    // line.
    bool dmaRequested()
    {
        // TODO: call _bus->setAddress() with the appropriate generated address
        if (disabled())
            return;
        if(_channels[_activechannel]._state == Channel::State::stateS0)
             _channels[_activechannel]._state = Channel::State::stateS1;
        if(_channels[_activechannel]._state == Channel::State::stateIdle) return false;
        return true;
    }
    // Step 3: device checks dmaAcknowledged() to see when to access the bus.
    // equivalent to checking the status of the DACK line.
    bool dmaAcknowledged(int channel)
    {
        // TODO
        return _channels[channel]._dack;
    }
    // Step 4: the device calls dmaComplete()
    // equivalent to lowering a DRQline.
    void dmaComplete(int channel)
    {
        // TODO
        _channels[channel]._state = Channel::State::stateIdle;
    }

    String getText()
    {
        // TODO
        return String(hex(_channels[_activechannel]._transferaddress,4,false));
    }

    String save() const
    {
        String s = String("{ ") + _dram.save() + ",\n  active: " +
            String::Boolean(_active) + ", tick: " + _tick + ", address: " +
            hex(_address, 5) + ", command: " + hex(_command, 2) +
            ", channels: { ";
        bool needComma = false;
        for (int i = 0; i < 4; ++i) {
            if (needComma)
                s += ", ";
            needComma = true;
            s += _channels[i].save();
        }
        return s + " }\n";
    }
    Type type() const
    {
        List<StructuredType::Member> members;
        members.add(StructuredType::Member("active", false));
        members.add(StructuredType::Member("tick", 0));
        members.add(StructuredType::Member("address", 0));
        members.add(StructuredType::Member("command", 4));
        members.add(StructuredType::Member("channels",
            TypedValue(Type::array(_channels[0].type()), List<TypedValue>()));
        return StructuredType("DMA", members);
    }
    void load(const TypedValue& value)
    {
        auto members = value.value<Value<HashTable<String, TypedValue>>>();
        _active = (*members)["active"].value<bool>();
        _tick = (*members["tick"].value<int>();
        _address = (*members["address"].value<int>();
        _command = (*members["command"].value<int>();
        auto channels = (*members)["channels"].value<List<TypedValue>>();
        for (int i = 0; i < 4; ++i) {
            _channels[i].load((*i).value<TypedValue>());
    }
    String name() const { return "dma"; }
private:
    class Channel
    {
    public:
        Channel()
        {
            _state = stateIdle;
            _dack = false;
            _mode = 0;
        }
        void simulateCycle()
        {
            switch(_state)
            {
                case stateS1:
                {
                    _state = stateS2;
                    break;
                }
                case stateS2:
                {
                    _dack = true;
                    _bus->setAddress(_transferaddress);
                    _state = stateS3;
                    break;
                }
                case stateS3:
                {
                    switch(_mode & 0x0C)
                    {
                        case 0x08:
                        {
                            _bus->read();
                            break;
                        }
                    }
                    _state = stateS4;
                    break;
                }
                case stateS4:
                {
                    _transferaddress++;
                    _currentcount--;
                    if(_currentcount == 0xFFFF)
                    {
                        _dack = false;
                        _state = stateIdle;
                    }
                    else
                    {
                        switch(_mode & 0xC0)
                        {
                            case 0x40:
                            {
                                _dack = false;
                                _state = stateIdle;
                            }
                        }
                    }
                    break;
                }
            }
        }
        UInt8 read(UInt32 address)
        {
            switch(address) {
                case 0:
                    if (_firstbyte)
                        return _startaddress & 0xff;
                    return _startaddress >> 8;
                case 1:
                    if (_firstbyte)
                        return _count & 0xff;
                    return _count >> 8;
            }
            return 0;
        }
        void write(UInt32 address, UInt8 data)
        {
            switch(address) {
                case 0:
                    if (_firstbyte)
                        _startaddress = (_startaddress & 0xFF00) | data;
                    else
                        _startaddress = (_startaddress & 0xFF) | (data << 8);
                    _transferaddress = _startaddress;
                    break;
                case 1:
                    if (_firstbyte)
                    {
                        _count = (_count & 0xFF00) | data;
                        _currentcount = _count;
                    }
                    else
                    {
                        _count = (_count & 0xFF) | (data << 8);
                        _currentcount = _count;
                    }
                    break;
            }
        }
        UInt8 _mode;
        enum State
        {
            stateIdle = 0,
            stateS0,
            stateS1,
            stateS2,
            stateS3,
            stateS4
        } _state;
        UInt16 _transferaddress;
        UInt16 _startaddress;
        ISA8BitBus* _bus;
        bool _dack;
    private:
        UInt16 _count;
        UInt16 _currentcount;
        bool _firstbyte;
    };

    bool disabled() const { return (_command & 4) != 0; }

    Channel _channels[4];
    int _address;
    Byte _command;
    int _activechannel;
};
