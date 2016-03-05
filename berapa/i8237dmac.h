template<class T> class Intel8237DMACT
  : public ISA8BitComponentBase<Intel8237DMACT<T>>
{
    using Base = ISA8BitComponentBase<Intel8237DMACT<T>>;
    enum State
    {
        stateIdle,
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
    static String typeName() { return "Intel8237DMAC"; }
    Intel8237DMACT(Component::Type type)
      : Base(type), _channels{this, this, this, this}
    {
        this->persist("address", &_address, HexPersistenceType(1));
        this->persist("command", &_command);
        this->persist("channels", &_channels[0],
            ArrayType(_channels[0].persistenceType(), 4));
        this->persist("lastByte", &_lastByte);
        this->persist("channel", &_channel);
        this->persist("highAddress", &_highAddress, 0xffff,
            HexPersistenceType(4));

        typename EnumerationType<State>::Helper h;
        h.add(stateIdle,  "idle");
        h.add(stateS0,    "s0");
        h.add(stateS1,    "s1");
        h.add(stateS2,    "s2");
        h.add(stateS3,    "s3");
        h.add(stateS4,    "s4");
        h.add(stateYield, "yield");
        this->persist("state", &_state,
            EnumerationType<State>("State", h, typeName() + "."));

        for (int i = 0; i < 4; ++i) {
            connector("dreq" + decimal(i), &_channels[i]._dReq);
            connector("dack" + decimal(i), &_channels[i]._dAck);
        }
        config("clock", &_clock, _clock.type());
    }
    //void runTo(Tick tick)
    //{
    //    while (_tick < tick) {
    //        _tick += _ticksPerCycle;
    //        simulateCycle();
    //    }
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
                    if (type == transferTypeWrite) {
                        this->_bus->setDMAAddressWrite(
                            _channels[_channel].currentAddress());
                        if (_channel == 1 && memoryToMemory())
                            this->_bus->write(_temporary);
                        else
                            this->_bus->write();
                    }
                    else {
                        this->_bus->setDMAAddressRead(
                            _channels[_channel].currentAddress());
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
                    UInt8 d = this->_bus->read();
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
    ISA8BitComponent* setAddressReadIO(Tick tick, UInt32 address)
    {
        _address = address & 0xf;
        return this;
    }
    ISA8BitComponent* setAddressWriteIO(Tick tick, UInt32 address)
    {
        _address = address & 0xf;
        return this;
    }
    UInt8 readIO(Tick tick)
    {
        if (_address < 8) {
            UInt8 r = _channels[_address >> 1].read(_address & 1, _lastByte);
            _lastByte = !_lastByte;
            return r;
        }
        switch (_address) {
            case 8:
                {
                    Byte b = 0;
                    for (int i = 0; i < 4; ++i)
                        b |= _channels[i].status() << i;
                    return b;
                }
            case 13:
                return _temporary;
        }
        return 0xff;
    }
    void writeIO(Tick tick, UInt8 data)
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

    //String getText()
    //{
    //    String line;
    //    switch (_state) {
    //        case stateS1:
    //            line += "D1 " + hex(getAddress(), 5, false) + " ";
    //            break;
    //        case stateS2:
    //            line += "D2 ";
    //            if (_channels[_channel].transferType() == transferTypeWrite)
    //                line += "M<-" + hex(_bus->data(), 2, false) + " ";
    //            else
    //                line += "      ";
    //            break;
    //        case stateS3: line += "D3       "; break;
    //        case stateS4:
    //            line += "D4 ";
    //            if (_channels[_channel].transferType() == transferTypeWrite)
    //                line += "      ";
    //            else
    //                line += "M->" + hex(_bus->data(), 2, false) + " ";
    //            break;
    //        case stateIdle:
    //            line = "";
    //            break;
    //    }
    //    return line;
    //}
    void setBus(ISA8BitBus* bus)
    {
        Base::setBus(bus);
        this->_bus->setDMAC(this);
    }
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

    class Channel : public SubComponent<Channel>
    {
    public:
        static String typeName() { return "Channel"; }
        Channel(Intel8237DMAC* dmac) : Channel(Type(dmac->simulator())) { }
        Channel(Component::Type type)
          : SubComponent<Channel>(type), _dAck(this), _dReq(this)
        {
            this->persist("mode", &_mode);
            this->persist("baseAddress", &_baseAddress);
            this->persist("baseCount", &_baseCount);
            this->persist("currentAddress", &_currentAddress);
            this->persist("currentCount", &_currentCount);
            this->persist("hardRequest", &_hardRequest);
            this->persist("softRequest", &_softRequest);
            this->persist("mask", &_mask);
            this->persist("terminalCount", &_terminalCount);
            this->persist("internalRequest", &_internalRequest);
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
            UInt16 m = 0xff00 >> shift;
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
        void setDReq(Tick tick, bool v)
        {
            // TODO
        }
        class Connector : public InputConnector<bool>
        {
        public:
            Connector(Channel* c) : InputConnector<bool>(c) { }
            void setData(Tick tick, bool v)
            {
                static_cast<Channel*>(component())->setDReq(tick, v);
            }
        };

        UInt16 _baseAddress;
        UInt16 _baseCount;
        UInt16 _currentAddress;
        UInt16 _currentCount;
        Byte _mode;
        //bool _hardRequest;
        //bool _softRequest;
        //bool _internalRequest;
        bool _mask;
        bool _request;
        //bool _terminalCount;
        //bool _blockContinue;
        OutputConnector<bool> _dAck;
        Connector _dReq;
    };

    bool memoryToMemory() const { return (_command & 1) != 0; }
    bool channel0AddressHold() const { return (_command & 2) != 0; }
    bool disabled() const { return (_command & 4) != 0; }
    bool compressedTiming() const { return (_command & 8) != 0; }
    bool rotatingPriority() const { return (_command & 0x10) != 0; }
    bool extendedWriteSelection() const { return (_command & 0x20) != 0; }
    bool dreqSenseActiveLow() const { return (_command & 0x40) != 0; }
    bool dackSenseActiveHigh() const { return (_command & 0x80) != 0; }

    Channel _channels[4];
    int _address;
    UInt16 _temporaryAddress;
    UInt16 _temporaryCount;
    Byte _status;
    Byte _command;
    //int _channel;
    bool _lastByte;
    Byte _temporary;
    //bool _dAck;
    //int _highAddress;
    State _state;
    Clock _clock;
};
