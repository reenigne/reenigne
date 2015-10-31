template<class T> class Intel8237DMATemplate
  : public ISA8BitComponentTemplate<T>
{
    enum State
    {
        stateIdle = 0,
        stateS0,
        stateS1,
        stateS2,
        stateS3,
        stateS4,
        stateYield
    };
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
public:
    Rational hDotsPerCycle() const { return 3; }
    Intel8237DMATemplate()
    {
        List<EnumerationType::Value> stateValues;
        for (int i = stateIdle; i <= stateS4; ++i) {
            State s = static_cast<State>(i);
            stateValues.add(EnumerationType::Value(stringForState(s), s));
        }
        _stateType = EnumerationType("DMAState", stateValues);
    }
    //void site()
    //{
    //    _pageRegisters = this->_simulator->getDMAPageRegisters();
    //    _bus = this->_simulator->getBus();
    //}
    void simulateCycle()
    {
        TransferMode mode = _channels[_channel].transferMode();
        TransferType type = _channels[_channel].transferType();
        switch (_state) {
            case stateS1:
                if (!compressedTiming() || _highAddress !=
                    (_channels[_channel].currentAddress() & 0xff00)) {
                    _state = stateS2;
                    break;
                }
                // Fall through when timing is compressed.
            case stateS2:
                _dAck = true;
                if (mode != transferModeCascade) {
                    _highAddress =
                        _channels[_channel].currentAddress() & 0xff00;
                    _bus->setAddress(getAddress());
                    if (type == transferTypeWrite) {
                        if (_channel == 1 && memoryToMemory())
                            _bus->write(_temporary);
                        else
                            _bus->write();
                    }
                }
                _state = stateS3;
                break;
            case stateS3:
                if (!compressedTiming()) {
                    _state = stateS4;
                    break;
                }
                // Fall through when timing is compressed.
            case stateS4:
                if (type == transferTypeRead && mode != transferModeCascade) {
                    UInt8 d = _bus->read();
                    if (_channel == 0 && memoryToMemory()) {
                        _temporary = d;
                        _channels[1].setInternalRequest(true);
                    }
                }
                _channels[_channel].setInternalRequest(false);
                _dAck = false;
                _state = stateIdle;
                if (!_channels[_channel].update(channel0AddressHold() &&
                    _channel == 0)) {
                    switch (mode) {
                        case transferModeSingle:
                            _state = stateYield;
                            break;
                        case transferModeBlock:
                            if (memoryToMemory() && _channel < 2) {
                                _channels[1 - _channel].
                                    setInternalRequest(true);
                            }
                            else
                                _channels[_channel].setInternalRequest(true);
                            break;
                    }
                }
                checkForDMA();
                break;
            case stateYield:
                _state = stateIdle;
                checkForDMA();
                break;
        }
        if (_state == stateIdle || _state == stateS0 || _state == stateYield)
            _highAddress = 0xffff;
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
                {
                    Byte b = 0;
                    for (int i = 0; i < 4; ++i)
                        b |= _channels[i].status() << i;
                    this->set(b);
                }
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
        switch (_address) {
            case 8: _command = data; checkForDMA(); break;
            case 9:
                _channels[data & 3].setSoftRequest((data & 4) != 0);
                checkForDMA();
                break;
            case 10:
                _channels[data & 3].setMask((data & 4) != 0);
                checkForDMA();
                break;
            case 11: _channels[data & 3].setMode(data & 0xfc); break;
            case 12: _lastByte = false; break;
            case 13:
                {
                    // Master clear
                    _command = 0;
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
                    checkForDMA();
                }
                break;
            case 15:
                {
                    for (int i = 0; i < 4; ++i)
                        _channels[i].setMask((data & (1 << i)) != 0);
                    checkForDMA();
                }
                break;
        }
    }
    // Step 1: the device calls dmaRequest()
    // equivalent to raising a DRQ line.
    void dmaRequest(int channel)
    {
        _channels[channel].setHardRequest(!dreqSenseActiveLow());
        checkForDMA();
    }
    // Step 2: at the end of the IO cycle the CPU calls dmaRequested()
    // equivalent to checking the status of the READY line and raising the HLDA
    // line.
    bool dmaRequested()
    {
        if (_state == stateS0)
            _state = stateS1;
        return _state != stateIdle;
    }
    // Step 3: device checks dmaAcknowledged() to see when to access the bus.
    // equivalent to checking the status of the DACK line.
    bool dmaAcknowledged(int channel)
    {
        return channel == _channel && _dAck == dackSenseActiveHigh();
    }
    // Step 4: the device calls dmaComplete()
    // equivalent to lowering a DRQline.
    void dmaComplete(int channel)
    {
        _channels[channel].setHardRequest(dreqSenseActiveLow());
        checkForDMA();
    }

    String getText()
    {
        String line;
        switch (_state) {
            case stateS1:
                line += "D1 " + hex(getAddress(), 5, false) + " ";
                break;
            case stateS2:
                line += "D2 ";
                if (_channels[_channel].transferType() == transferTypeWrite)
                    line += "M<-" + hex(_bus->data(), 2, false) + " ";
                else
                    line += "      ";
                break;
            case stateS3: line += "D3       "; break;
            case stateS4:
                line += "D4 ";
                if (_channels[_channel].transferType() == transferTypeWrite)
                    line += "      ";
                else
                    line += "M->" + hex(_bus->data(), 2, false) + " ";
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
            ", highAddress: " + hex(_highAddress, 4) +
            ", state: " + stringForState(_state) +
            " }\n";
    }
    ::Type persistenceType() const
    {
        List<StructuredType::Member> members;
        members.add(StructuredType::Member("active", false));
        members.add(StructuredType::Member("tick", 0));
        members.add(StructuredType::Member("address", 0));
        members.add(StructuredType::Member("command", 0));
        members.add(StructuredType::Member("channels",
            TypedValue(SequenceType(_channels[0].type()),
                List<TypedValue>())));
        members.add(StructuredType::Member("lastByte", false));
        members.add(StructuredType::Member("temporary", 0));
        members.add(StructuredType::Member("channel", 0));
        members.add(StructuredType::Member("highAddress", 0xffff));
        members.add(StructuredType::Member("state",
            TypedValue(_stateType, stateIdle)));
        return StructuredType("DMA", members);
    }
    void load(const TypedValue& value)
    {
        auto members = value.value<HashTable<Identifier, TypedValue>>();
        this->_active = members["active"].value<bool>();
        this->_tick = members["tick"].value<int>();
        _address = members["address"].value<int>();
        _command = members["command"].value<int>();
        auto channels = members["channels"].value<List<TypedValue>>();

        int j = 0;
        for (auto i = channels.begin(); i != channels.end(); ++i) {
            _channels[j].load((*i).value<TypedValue>());
            ++j;
            if (j == 4)
                break;
        }
        for (;j < 4; ++j) {
            _channels[j].load(TypedValue(StructuredType(String(),
                List<StructuredType::Member>()),
                HashTable<Identifier, TypedValue>()).
                convertTo(_channels[j].type()));
        }

        _lastByte = members["lastByte"].value<bool>();
        _temporary = members["temporary"].value<int>();
        _channel = members["channel"].value<int>();
        _highAddress = members["highAddress"].value<int>();
        _state = members["state"].value<State>();
    }

    class Type : public Component::Type
    {
    public:
        Type(Simulator* simulator) : Component::Type(new Body(simulator)) { }
    private:
        class Body : public Component::Type::Body
        {
        public:
            Body(Simulator* simulator) : Component::Type::Body(simulator) { }
            String toString() const { return "Intel8237DMA"; }
        };
    };
private:
    void checkForDMA()
    {
        if (disabled())
            return;
        if (_state != stateIdle)
            return;

        if (_channel == 0 && memoryToMemory() && _channels[0].request() &&
            !_channels[1].terminalCount()) {
            _channel = 1;
            _state = stateS0;
            return;
        }

        int i;
        int channel;
        for (i = 0; i < 4; ++i) {
            channel = i;
            if (rotatingPriority())
                channel = (channel + _channel) & 3;
            if (channel == 0 && memoryToMemory()) {
                if (_channels[0].request() && !_channels[1].terminalCount())
                    break;
            }
            else {
                if (_channels[channel].request() &&
                    !_channels[channel].terminalCount())
                    break;
            }
        }
        if (i == 4)
            return;

        _channel = channel;
        _state = stateS0;
    }

    class Channel
    {
    public:
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
        void setInternalRequest(bool internalRequest)
        {
            _internalRequest = internalRequest;
        }
        void setMask(bool mask) { _mask = mask; }
        void clear()
        {
            _mask = true;
            _terminalCount = false;
            _softRequest = false;
        }
        UInt16 currentAddress() const { return _currentAddress; }
        bool update(bool holdAddress)
        {
            if (!holdAddress)
                if (addressDecrement())
                    --_currentAddress;
                else
                    ++_currentAddress;
            --_currentCount;
            if (_currentCount == 0xffff) {
                if (autoInitialization()) {
                    _currentAddress = _baseAddress;
                    _currentCount = _baseCount;
                }
                else
                    _terminalCount = true;
            }
            return _terminalCount;
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
                ", internalRequest: " + String::Boolean(_internalRequest) +
                " }";
        }
        ::Type type() const
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
            members.add(StructuredType::Member("internalRequest", false));
            return StructuredType("DMAChannel", members);
        }
        void load(const TypedValue& value)
        {
            auto members = value.value<HashTable<Identifier, TypedValue>>();
            _mode = members["mode"].value<int>();
            _baseAddress = members["baseAddress"].value<int>();
            _baseCount = members["baseCount"].value<int>();
            _currentAddress = members["currentAddress"].value<int>();
            _currentCount = members["currentCount"].value<int>();
            _hardRequest = members["hardRequest"].value<bool>();
            _softRequest = members["softRequest"].value<bool>();
            _mask = members["mask"].value<bool>();
            _terminalCount = members["terminalCount"].value<bool>();
            _internalRequest = members["internalRequest"].value<bool>();
        }
        Byte status()
        {
            Byte s = (_terminalCount ? 1 : 0) | (request() ? 0x10 : 0);
            _terminalCount = false;
            return s;
        }

        bool request() const
        {
            return (_hardRequest && !_mask) || _softRequest ||
                _internalRequest;
        }

        TransferType transferType() const
        {
            return static_cast<TransferType>((_mode >> 2) & 3);
        }
        bool autoInitialization() const { return (_mode & 0x10) != 0; }
        bool addressDecrement() const { return (_mode & 0x20) != 0; }
        TransferMode transferMode() const
        {
            return static_cast<TransferMode>((_mode >> 6) & 3);
        }
        bool terminalCount() const { return _terminalCount; }

    private:
        Byte _mode;
        UInt16 _baseAddress;
        UInt16 _baseCount;
        UInt16 _currentAddress;
        UInt16 _currentCount;
        bool _hardRequest;
        bool _softRequest;
        bool _internalRequest;
        bool _mask;
        bool _terminalCount;
        bool _blockContinue;
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
            (this->_pageRegisters->pageForChannel(_channel) << 16);
    }

    static String stringForState(State state)
    {
        switch (state) {
            case stateIdle:  return "idle";
            case stateS0:    return "s0";
            case stateS1:    return "s1";
            case stateS2:    return "s2";
            case stateS3:    return "s3";
            case stateS4:    return "s4";
            case stateYield: return "yield";
        }
        return "";
    }

    DMAPageRegisters* _pageRegisters;
    ISA8BitBus* _bus;
    Channel _channels[4];
    int _address;
    Byte _command;
    int _channel;
    bool _lastByte;
    Byte _temporary;
    bool _dAck;
    int _highAddress;
    State _state;

    Type _stateType;
};
