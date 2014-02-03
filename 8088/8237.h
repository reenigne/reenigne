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
        if (_address < 8) {
            this->set(_channels[_address >> 1].read(_address & 1, _lastByte));
            _lastByte = !_lastByte;
            return;
        }
        switch (address) {
            case 8:
                // TODO: read status register
                // this->set(data);
                return;
            case 13:
                // TODO: read temporary register
                // this->set(data);
                return;
        }
    }
    void write(UInt8 data)
    {
        if (_address < 8) {
            _channels[_address >> 1].write(_address & 1, data, _lastByte);
            _lastByte = !_lastByte;
            return;
        }
        switch(_address) {
            case 8: _command = data; break;
            case 9: _channels[data & 3].setRequest((data & 4) != 0); break;
            case 10: _channels[data & 3].setMask((data & 4) != 0); break;
            case 11: _channels[data & 3].setMode(data & 0xfc); break;
            case 12: _lastByte = false; break;
            case 13:
                // Master clear
                _command = 0;
                _status = 0;
                _request = 0;
                _temporary = 0;
                _lastByte = false;
                _mask = 15;
                _state = stateIdle;
                break;
            case 14: _mask = 0; break;
            case 15:
                _channels[0].setMask((data & 1) ! = 0);
                _channels[1].setMask((data & 2) ! = 0);
                _channels[2].setMask((data & 4) ! = 0);
                _channels[3].setMask((data & 8) ! = 0);
                break;
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
        String s = String("{ active: " + String::Boolean(_active) +
            ", tick: " + _tick + ", address: " + hex(_address, 5) +
            ", command: " + hex(_command, 2) + ", channels: { ";
        bool needComma = false;
        for (int i = 0; i < 4; ++i) {
            if (needComma)
                s += ", ";
            needComma = true;
            s += _channels[i].save();
        }
        return s + " }, lastByte: " + String::Boolean(_lastByte + " }\n";
    }
    Type type() const
    {
        List<StructuredType::Member> members;
        members.add(StructuredType::Member("active", false));
        members.add(StructuredType::Member("tick", 0));
        members.add(StructuredType::Member("address", 0));
        members.add(StructuredType::Member("command", 0));
        members.add(StructuredType::Member("channels",
            TypedValue(Type::array(_channels[0].type()), List<TypedValue>()));
        members.add(StructuredType::Member("lastByte", false));
        return StructuredType("DMA", members);
    }
    void load(const TypedValue& value)
    {
        auto members = value.value<Value<HashTable<String, TypedValue>>>();
        _active = (*members)["active"].value<bool>();
        _tick = (*members["tick"].value<int>();
        _address = (*members)["address"].value<int>();
        _command = (*members)["command"].value<int>();
        auto channels = (*members)["channels"].value<List<TypedValue>>();
        for (int i = 0; i < 4; ++i)
            _channels[i].load((*i).value<TypedValue>());
        _lastByte = (*members)["lastByte"].value<bool>();
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
        UInt8 read(UInt32 address, bool lastByte)
        {
            int shift = (lastByte ? 8 : 0);
            if (address == 0)
                return (_currentAddress >> shift) & 0xff;
            return (_currentCount >> shift) & 0xff;
        }
        void write(UInt32 address, UInt8 data, bool lastByte)
        {
            int shift = (lastByte ? 8 : 0);
            UInt16 d = data << shift;
            UInt16 m = 0xff << shift;
            if (address == 0) {
                _baseAddress = (_baseAddress & m) | d;
                _currentAddress = (_currentAddress & m) | d;
            }
            else {
                _baseCount = (_baseCount & m) | d;
                _currentCount = (_currentCount & m) | d;
            }
        }
        void setMode(UInt8 mode) { _mode = mode; }

        String save() const
        {

        }
        Type type() const
        {
        }
        void load(const TypedValue& value)
        {
        }

    private:
        UInt16 _baseAddress;
        UInt16 _baseCount;
        UInt16 _currentAddress;
        UInt16 _currentCount;
        UInt8 _mode;

    //    UInt16 _transferaddress;     
    //    UInt16 _startaddress;        
    //    ISA8BitBus* _bus;
    //    bool _dack;
    //private:
    //    UInt16 _count;
    //    UInt16 _currentcount;
    //    bool _firstbyte;
    };

    bool disabled() const { return (_command & 4) != 0; }

    Channel _channels[4];
    int _address;
    Byte _command;
    int _activechannel;
    bool _lastByte;

    enum State
    {
        stateIdle = 0,
        stateS0,
        stateS1,
        stateS2,
        stateS3,
        stateS4
    } _state;

};
