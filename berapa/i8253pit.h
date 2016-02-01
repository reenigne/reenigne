class Intel8253PIT : public ISA8BitComponentBase<Intel8253PIT>
{
public:
    static String typeName() { return "Intel8253PIT"; }
    Intel8253PIT(Component::Type type)
      : ISA8BitComponentBase<Intel8253PIT>(type),
        _timers{Timer::Type(this->simulator()), Timer::Type(this->simulator()),
            Timer::Type(this->simulator())}
    {
        this->persist("address", &_address);
        this->persist("timers", &_timers[0],
            ArrayType(_timers[0].persistenceType(), 3));
    }
    void simulateCycle()
    {
        for (int i = 0; i < 3; ++i)
            _timers[i].simulateCycle();
    }
    ISA8BitComponent* setAddressReadIO(Tick tick, UInt32 address)
    {
        _address = address & 3;
        return this;
    }
    ISA8BitComponent* setAddressWriteIO(Tick tick, UInt32 address)
    {
        _address = address & 3;
        return this;
    }
    UInt8 readIO(Tick tick)
    {
        if (_address < 3)
            return _timers[_address].read();
        // TODO
        return 0xff;
    }
    void writeIO(Tick tick, UInt8 data)
    {
        if (_address < 3)
            _timers[_address].write(data);
        else {
            int timer = (data >> 6) & 3;
            if (timer < 3)
                _timers[timer].control(data & 0x3f);
        }
    }
private:
    class Timer : public SubComponent<Timer>
    {
    public:
        static String typeName() { return "Timer"; }
        Timer(Component::Type type) : SubComponent(type)
        {
            persist("value", &_value);
            persist("latch", &_latch);
            persist("count", &_count);
            persist("bcd", &_bcd);
            persist("bytes", &_bytes);
            persist("lowCount", &_lowCount);
            persist("firstByte", &_firstByte);
            persist("gate", &_gate);
            persist("output", &_output);
            persist("latched", &_latched);

            typename EnumerationType<State>::Helper h;
            h.add(stateStopped0,  "stopped0");
            h.add(stateCounting0, "counting0");
            h.add(stateStopped1,  "stopped1");
            h.add(stateStart1,    "start1");
            h.add(stateCounting1, "counting1");
            h.add(stateStopped2,  "stopped2");
            h.add(stateGateLow2,  "gateLow2");
            h.add(stateCounting2, "counting2");
            persist("state", &_state,
                EnumerationType<State>("State", h,
                    Intel8253PIT::typeName() + "." + typeName() + "."));

            setGate(true);
        }
        void simulateCycle()
        {
            switch (_state) {
                case stateStopped0:
                    break;
                case stateCounting0:
                    if (!_gate)
                        break;
                    countDown();
                    if (_value == 0)
                        setOutput(true);
                    break;
                case stateStopped1:
                    break;
                case stateStart1:
                    _value = _count;
                    setOutput(false);
                    _state = stateCounting1;
                    break;
                case stateCounting1:
                    if (_gate)
                        _value = _count;
                    countDown();
                    if (_value == 0)
                        setOutput(true);
                    break;
                case stateStopped2:
                    break;
                case stateGateLow2:
                    if(_gate)
                    {
                        _state = stateCounting2;
                        _value = _count;
                    }
                    break;
                case stateCounting2:
                    if (!_gate)
                    {
                        setOutput(true);
                        _state = stateGateLow2;
                        break;
                    }
                    if(_value == 1)
                    {
                        setOutput(true);
                        _value = _count;
                        break;
                    }
                    countDown();
                    if (_value == 1)
                        setOutput(false);
                    break;
            }
        }
        UInt8 read()
        {
            switch (_bytes) {
                case 0:
                    return _latch & 0xff;
                    break;
                case 1:
                    if (_latched)
                        return _latch & 0xff;
                    return _value & 0xff;
                case 2:
                    if (_latched)
                        return _latch >> 8;
                    return _value >> 8;
                case 3:
                    if (_latched) {
                        if (_firstByte)
                            return _latch & 0xff;
                        return _latch >> 8;
                    }
                    if (_firstByte)
                        return _value & 0xff;
                    return _value >> 8;
            }
            return 0;
        }
        void write(UInt8 data)
        {
            switch (_bytes) {
                case 0:
                    break;
                case 1:
                    loadCount(data);
                    break;
                case 2:
                    loadCount(data << 8);
                    break;
                case 3:
                    if (_firstByte) {
                        _lowCount = data;
                        _firstByte = false;
                        switch (_state) {
                            case stateCounting0:
                                _state = stateStopped0;
                                break;
                            case stateCounting1:
                                _state = stateStopped1;
                                break;
                            case stateCounting2:
                                _state = stateStopped2;
                                break;
                        }
                    }
                    else {
                        loadCount(_lowCount | (data << 8));
                        _firstByte = true;
                    }
                    break;
            }
        }
        void control(UInt8 data)
        {
            int command = (data >> 4) & 3;
            if (command == 0) {
                _latch = _value;
                _latched = true;
                return;
            }
            _bcd = ((data & 1) != 0);
            _bytes = command;
            switch ((data >> 1) & 7) {
                case 0:
                    _state = stateStopped0;
                    setOutput(false);
                    break;
                case 1:
                    _state = stateStopped1;
                    setOutput(true);
                    break;
                case 2:
                    _state = stateStopped2;
                    setOutput(true);
                    break;
            }
        }
        void setGate(bool gate)
        {
            switch (_state) {
                case stateStopped0:
                case stateCounting0:
                    break;
                case stateStopped1:
                case stateStart1:
                case stateCounting1:
                    if (_gate && !gate)
                        _state = stateStart1;
                    break;
            }
            _gate = gate;
        }
    private:
        enum State
        {
            stateStopped0,
            stateCounting0,
            stateStopped1,
            stateStart1,
            stateCounting1,
            stateStopped2,
            stateGateLow2,
            stateCounting2,
        };

        void loadCount(UInt16 value)
        {
            _count = value;
            switch (_state) {
                case stateStopped0:
                case stateCounting0:
                    _value = _count;
                    _state = stateCounting0;
                    break;
                case stateStopped1:
                case stateStart1:
                case stateCounting1:
                    break;
                case stateStopped2:
                case stateCounting2:
                    _state = stateCounting2;
                    break;
            }
        }
        void countDown()
        {
            if (!_bcd) {
                --_value;
                return;
            }
            if ((_value & 0xf) != 0) {
                --_value;
                return;
            }
            if ((_value & 0xf0) != 0) {
                _value -= (0x10 - 9);
                return;
            }
            if ((_value & 0xf00) != 0) {
                _value -= (0x100 - 0x99);
                return;
            }
            _value -= (0x1000 - 0x999);
        }
        void setOutput(bool output)
        {
            if (output != _output) {
                _output = output;
                outputChanged(output);
            }
        }
        void outputChanged(bool output) { }

        UInt16 _value;
        UInt16 _latch;
        UInt16 _count;
        bool _bcd;
        int _bytes;
        UInt8 _lowCount;
        bool _firstByte;
        bool _gate;
        bool _output;
        bool _latched;
        State _state;
    };
    Timer _timers[3];
    int _address;
};
