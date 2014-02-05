template<class T> class Intel8237DMATemplate
  : public ISA8BitComponentTemplate<T>
{
public:
    Rational<int> hDotsPerCycle() const { return 3; }
    enum State
    {
        stateIdle = 0,
        stateS0,
        stateS1,
        stateS2,
        stateS3,
        stateS4
    } _state;
    Intel8237DMATemplate()
    {
        List<EnumerationType::Value> stateValues;
        for (int i = stateIdle; i <= stateS4; ++i) {
            State s = static_cast<State>(i);
            stateValues.add(EnumerationType::Value(stringForState(s), s));
        }
        _stateType = EnumerationType("DMAState", stateValues);
    }
    void site()
    {
        _pageRegisters = this->_simulator->getDMAPageRegisters();
        _bus = this->_simulator->getBus();
    }
    void simulateCycle()
    {
        switch(_state) {
            case stateS1:
                _state = stateS2;
                break;
            case stateS2:
                _dAck = true;
                _bus->setAddress(getAddress());
                _state = stateS3;
                break;
            case stateS3:
                switch (_channels[_channel].transferType()) {
                    case Channel::transferTypeVerify:
                        // TODO
                        break;
                    case Channel::transferTypeWrite:
                        // TODO
                        break;
                    case Channel::transferTypeRead:
                        _bus->read();
                        break;
                    case Channel::transferTypeIllegal:
                        // TODO
                        break;
                }
                _state = stateS4;
                break;
            case stateS4:
                if (_channels[_channel].update()) {
                    _dAck = false;
                    _state = stateIdle;
                }
                else {
                    switch (_channels[_channel].transferMode()) {
                        case Channel::transferModeDemand:
                            // TODO
                            break;
                        case Channel::transferModeSingle:
                            _dAck = false;
                            _state = stateIdle;
                            break;
                        case Channel::transferModeBlock:
                            // TODO
                            break;
                        case Channel::transferModeCascade:
                            // TODO
                            break;
                    }
                }
                break;
        }
    }
    void setAddress(UInt32 address)
    {
        _address = address & 0xf;
        this->_active = ((address & 0x400003f0) == 0x40000000);
    }
    void read()
    {
        if (_address < 8) {
            this->set(_channels[_address >> 1].read(_address & 1, _lastByte));
            _lastByte = !_lastByte;
            return;
        }
        switch (_address) {
            case 8:
                this->set(_channels[0].status() | (_channels[1].status() << 1)
                    | (_channels[2].status() << 2) |
                    (_channels[3].status() << 3));
                return;
            case 13:
                this->set(_temporary);
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
            case 9: _channels[data & 3].setSoftRequest((data & 4) != 0); break;
            case 10: _channels[data & 3].setMask((data & 4) != 0); break;
            case 11: _channels[data & 3].setMode(data & 0xfc); break;
            case 12: _lastByte = false; break;
            case 13:
                {
                    // Master clear
                    _command = 0;
                    _status = 0;
                    _request = 0;
                    _temporary = 0;
                    _lastByte = false;
                    for (int i = 0; i < 4; ++i)
                        _channels[i].clear();
                    _state = stateIdle;
                }
                break;
            case 14:
                {
                    for (int i = 0; i < 4; ++i)
                        _channels[i].setMask(false);
                }
                break;
            case 15:
                {
                    for (int i = 0; i < 4; ++i)
                    {
                        Byte tmp = data & (1 << i); //Workaround for dumb compilers.
                        _channels[i].setMask(tmp != 0);
                    }
                }
                break;
        }
    }
    // Step 1: the device calls dmaRequest()
    // equivalent to raising a DRQ line.
    void dmaRequest(int channel)
    {
        bool oldRequest = _channels[channel].request();
        _channels[channel].setHardRequest(true);
        if (!oldRequest && !disabled() && _state == stateIdle) {
            _state = stateS0;
            _channel = channel;
        }
    }
    // Step 2: at the end of the IO cycle the CPU calls dmaRequested()
    // equivalent to checking the status of the READY line and raising the HLDA
    // line.
    bool dmaRequested()
    {
        //if (disabled())
        //    return;
        if (_state == stateS0)
            _state = stateS1;
        return _state != stateIdle;
    }
    // Step 3: device checks dmaAcknowledged() to see when to access the bus.
    // equivalent to checking the status of the DACK line.
    bool dmaAcknowledged(int channel)
    {
        return channel == _channel && _dAck;
    }
    // Step 4: the device calls dmaComplete()
    // equivalent to lowering a DRQline.
    void dmaComplete(int channel) { _channels[channel].setHardRequest(false); }

    String getText()
    {
        String line;
        //// TODO
        //return String(hex(_channels[_activechannel]._transferaddress,4,false));
        switch (_state) {
            case stateS1:
                line += "D1 " + hex(getAddress(), 5, false) + " ";
                break;
            case stateS2:
                line += "D2 ";
                //if (_channels[_channel].transferType == Channel::transferTypeRead)
                    //TODO: line += "M<-" + hex(_busData, 2, false) + " ";
                //else
                    line += "      ";
                break;
            case stateS3: line += "D3       "; break;
            case stateS4:
                line += "D4 ";
                if (_channels[_channel].transferType == Channel::transferTypeWrite)
                    line += "      ";
                /*else
                    if (_abandonFetch)
                        line += "----- ";
                    else
                        line += "M->" + hex(_busData, 2, false) + " ";*/
                break;
            case stateIdle:
                line = "";
                break;
        }
        return line;
    }

    String save() const
    {
        String s = String() + 
            "{ active: " + String::Boolean(this->_active) +
            ", tick: " + String::Decimal(this->_tick) +
            ", address: " + hex(_address, 5) +
            ", command: " + hex(_command, 2) +
            ", channels: { ";
        bool needComma = false;
        for (int i = 0; i < 4; ++i) {
            if (needComma)
                s += ", ";
            needComma = true;
            s += _channels[i].save();
        }
        return s + " }, lastByte: " + String::Boolean(_lastByte) +
            ", temporary: " + hex(_temporary, 2) +
            ", channel: " + decimal(_channel) +
            ", state: " + stringForState(_state) +
            " }\n";
    }
    Type type() const
    {
        List<StructuredType::Member> members;
        members.add(StructuredType::Member("active", false));
        members.add(StructuredType::Member("tick", 0));
        members.add(StructuredType::Member("address", 0));
        members.add(StructuredType::Member("command", 0));
        members.add(StructuredType::Member("channels",
            TypedValue(Type::array(_channels[0].type()), List<TypedValue>())));
        members.add(StructuredType::Member("lastByte", false));
        members.add(StructuredType::Member("temporary", 0));
        members.add(StructuredType::Member("channel", 0));
        members.add(StructuredType::Member("state",
            TypedValue(_stateType, stateIdle)));
        return StructuredType("DMA", members);
    }
    void load(const TypedValue& value)
    {
        auto members = value.value<Value<HashTable<String, TypedValue>>>();
        this->_active = (*members)["active"].value<bool>();
        this->_tick = (*members)["tick"].value<int>();
        _address = (*members)["address"].value<int>();
        _command = (*members)["command"].value<int>();
        auto channels = (*members)["channels"].value<List<TypedValue>>();
        for (auto i = channels.begin(); i != channels.end(); ++i)
            _channels[i].load((*i).value<TypedValue>());
        _lastByte = (*members)["lastByte"].value<bool>();
        _temporary = (*members)["temporary"].value<int>();
        _channel = (*members)["channel"].value<int>();
        _state = (*members)["state"].value<State>();
    }
    String name() const { return "dma"; }
private:
    class Channel
    {
    public:
        enum TransferType
        {
            transferTypeVerify,
            transferTypeWrite,
            transferTypeRead,
            transferTypeIllegal
        };
        enum TransferMode
        {
            transferModeDemand,
            transferModeSingle,
            transferModeBlock,
            transferModeCascade
        };
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
        void setHardRequest(bool hardRequest) { _hardRequest = hardRequest; }
        void setSoftRequest(bool softRequest) { _softRequest = softRequest; }
        void setMask(bool mask) { _mask = mask; }
        void clear() { _mask = true; _terminalCount = false; }
        UInt16 currentAddress() const { return _currentAddress; }
        bool update()
        {
            if (addressDecrement())
                --_currentAddress;
            else
                ++_currentAddress;
            --_currentCount;
            return _currentCount == 0xffff;
        }

        String save() const
        {
            return String("\n    ") + 
                "{ mode: " + hex(_mode, 2) +
                ", baseAddress: " + hex(_baseAddress, 4) +
                ", baseCount: " + hex(_baseCount, 4) +
                ", currentAddress: " + hex(_currentAddress, 4) +
                ", currentCount: " + hex(_currentCount, 4) +
                ", hardRequest: " + String::Boolean(_hardRequest) +
                ", softRequest: " + String::Boolean(_softRequest) +
                ", mask: " + String::Boolean(_mask) +
                ", terminalCount: " + String::Boolean(_terminalCount) +
                " }";
        }
        Type type() const
        {
            List<StructuredType::Member> members;
            members.add(StructuredType::Member("mode", 0));
            members.add(StructuredType::Member("baseAddress", 0));
            members.add(StructuredType::Member("baseCount", 0));
            members.add(StructuredType::Member("currentAddress", 0));
            members.add(StructuredType::Member("currentCount", 0));
            members.add(StructuredType::Member("hardRequest", false));
            members.add(StructuredType::Member("softRequest", false));
            members.add(StructuredType::Member("mask", true));
            members.add(StructuredType::Member("terminalCount", false));
            return StructuredType("DMAChannel", members);
        }
        void load(const TypedValue& value)
        {
            auto members = value.value<Value<HashTable<String, TypedValue>>>();
            _mode = (*members)["mode"].value<int>();
            _baseAddress = (*members)["baseAddress"].value<int>();
            _baseCount = (*members)["baseCount"].value<int>();
            _currentAddress = (*members)["currentAddress"].value<int>();
            _currentCount = (*members)["currentCount"].value<int>();
            _hardRequest = (*members)["hardRequest"].value<bool>();
            _softRequest = (*members)["softRequest"].value<bool>();
            _mask = (*members)["mask"].value<bool>();
            _terminalCount = (*members)["terminalCount"].value<bool>();
        }
        Byte status() const
        {
            Byte s = (_terminalCount ? 1 : 0) | (request() ? 0x10 : 0);
            _terminalCount = false;
            return s;
        }

        bool request() const { return _hardRequest | _softRequest; }

        TransferType transferType() const { return (_mode >> 2) & 3; }
        bool autoInitialization() const { return (_mode & 0x10) != 0; }
        bool addressDecrement() const { return (_mode & 0x20) != 0; }
        TransferMode transferMode() const { return (_mode >> 6) & 3; }

    private:
        Byte _mode;
        UInt16 _baseAddress;
        UInt16 _baseCount;
        UInt16 _currentAddress;
        UInt16 _currentCount;
        bool _hardRequest;
        bool _softRequest;
        bool _mask;
        bool _terminalCount;
    };

    bool memoryToMemory() const { return (_command & 1) != 0; }
    bool channel0AddressHold() const { return (_command & 2) != 0; }
    bool disabled() const { return (_command & 4) != 0; }
    bool compressedTiming() const { return (_command & 8) != 0; }
    bool rotatingPriority() const { return (_command & 0x10) != 0; }
    bool extendedWriteSelection() const { return (_command & 0x20) != 0; }
    bool dreqSenseActiveLow() const { return (_command & 0x40) != 0; }
    bool dackSenseActiveHigh() const { return (_command & 0x80) != 0; }

    UInt32 getAddress() const
    {
        return _channels[_channel].currentAddress() |
            (_pageRegisters->pageForChannel(_channel) << 16);
    }

    static String stringForState(State state)
    {
        switch (_state) {
            case stateIdle: return "idle";
            case stateS0:   return "s0";
            case stateS1:   return "s1";
            case stateS2:   return "s2";
            case stateS3:   return "s3";
            case stateS4:   return "s4";
        }
        return "";
    }

    DMAPageRegisters* _pageRegisters;
    ISA8BitBus* _bus;
    Channel _channels[4];
    int _address;
    Byte _command;
    Byte _status;
    Byte _request;
    int _channel;
    bool _lastByte;
    Byte _temporary;
    bool _dAck;

    Type _stateType;
};
