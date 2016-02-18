class Intel8253PIT : public ISA8BitComponentBase<Intel8253PIT>
{
    using Base = ISA8BitComponentBase<Intel8253PIT>;
public:
    static String typeName() { return "Intel8253PIT"; }
    Intel8253PIT(Component::Type type) : Base(type), _timers{this, this, this}
    {
        this->persist("address", &_address);
        this->persist("timers", &_timers[0],
            ArrayType(_timers[0].persistenceType(), 3));
        config("timer0", &_timers[0], _timers[0].type());
        config("timer1", &_timers[1], _timers[1].type());
        config("timer2", &_timers[2], _timers[2].type());
    }
    void runTo(Tick tick)
    {
        _timers[0].runTo(tick);
        _timers[1].runTo(tick);
        _timers[2].runTo(tick);
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
        return 0xff;  // Tristate according to datasheet
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
    class Timer : public ClockedSubComponent<Timer>
    {
    public:
        static String typeName() { return "Timer"; }
        Timer(Intel8253PIT* pit) : Timer(Type(pit->simulator(), this)) { }
        Timer(Component::Type type)
          : ClockedSubComponent<Timer>(type), _gateConnector(this),
            _output(this)
        {
            persist("value", &_value);
            persist("latch", &_latch);
            persist("count", &_count);
            persist("bcd", &_bcd);
            persist("bytes", &_bytes);
            persist("lowCount", &_lowCount);
            persist("firstByte", &_firstByte);
            persist("gate", &_gate);
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
            h.add(stateStopped3,  "stopped3");
            h.add(stateGateLow3,  "gateLow3");
            h.add(stateCounting3High, "counting3High");
            h.add(stateCounting3Low, "counting3Low");
            h.add(stateStopped4,  "stopped4");
            h.add(stateCounting4, "counting4");
            h.add(stateStopped5,  "stopped5");
            persist("state", &_state,
                EnumerationType<State>("State", h,
                    Intel8253PIT::typeName() + "." + typeName() + "."));

            connector("gate", &_gateConnector);
            connector("output", &_output);
        }
        void runTo(Tick tick)
        {
            while (_tick < tick) {
                _tick += _ticksPerCycle;
                simulateCycle();
            }
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
                        _output.set(_tick, true);
                    break;
                case stateStopped1:
                    break;
                case stateStart1:
                    _value = _count;
                    _output.set(_tick, false);
                    _state = stateCounting1;
                    break;
                case stateCounting1:
                    if (_gate)
                        _value = _count;
                    countDown();
                    if (_value == 0)
                        _output.set(_tick, true);
                    break;
                case stateStopped2:
                    break;
                case stateGateLow2:
                    if (_gate) {
                        _state = stateCounting2;
                        _value = _count;
                    }
                    break;
                case stateCounting2:
                    if (!_gate) {
                        _output.set(_tick, true);
                        _state = stateGateLow2;
                        break;
                    }
                    countDown();
                    if (_value == 0) {
                        _output.set(_tick, true);
                        _value = _count;
                        break;
                    }
                    if (_value == 1)
                        _output.set(_tick, false);
                    break;
                case stateGateLow3:
                    if (_gate) {
                        _state = stateCounting3High;
                        _value = _count;
                    }
                    break;
                case stateCounting3High:
                    if (!_gate) {
                        _state = stateGateLow3;
                        _value = _count;
                    }
                    countDown();
                    if ((_value & 1) == 0)
                        countDown();
                    if (_value == 0) {
                        _output.set(_tick, false);
                        _value = _count;
                        _state = stateCounting3Low;
                    }
                    break;
                case stateCounting3Low:
                    if (!_gate) {
                        _state = stateGateLow3;
                        _value = _count;
                    }
                    if ((_value & 1) != 0)
                        countDown();
                    countDown();
                    countDown();
                    if (_value == 0) {
                        _output.set(_tick, true);
                        _value = _count;
                        _state = stateCounting3High;
                    }
                    break;
                case stateCounting4:
                    if (!_gate)
                        break;
                    countDown();
                    if (_value == 0)
                        _output.set(_tick, true);
                    if (_value == 1)
                        _output.set(_tick, false);
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
                            case stateCounting3High:
                            case stateCounting3Low:
                                _state = stateStopped3;
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
            switch (data & 0xe) {
                case 0x0:
                    _state = stateStopped0;
                    _output.set(_tick, false);
                    break;
                case 0x2:
                    _state = stateStopped1;
                    _output.set(_tick, true);
                    break;
                case 0x4:
                case 0xc:
                    _state = stateStopped2;
                    _output.set(_tick, true);
                    break;
                case 0x6:
                case 0xf:
                    _state = stateStopped3;
                    _output.set(_tick, false);  // ?
                    break;
                case 0x8:
                    _state = stateStopped4;
                    _output.set(_tick, true);
                    break;
                case 0xa:
                    _state = stateStopped5;
                    _output.set(_tick, false);  // ?
                    break;
            }
        }
        void setGate(Tick tick, bool gate)
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
            stateStopped3,
            stateGateLow3,
            stateCounting3High,
            stateCounting3Low,
            stateStopped4,
            stateCounting4,
            stateStopped5,
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
                case stateStopped3:
                case stateCounting3High:
                case stateCounting3Low:
                    _state = stateCounting3High;
                    break;
                case stateStopped4:
                    _state = stateCounting4;
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

        class Connector : public InputConnector<bool>
        {
        public:
            Connector(Timer* c) : InputConnector<bool>(c) { }
            void setData(Tick tick, bool v)
            {
                static_cast<Timer*>(component())->setGate(tick, v);
            }
        };

        UInt16 _value;
        UInt16 _latch;
        UInt16 _count;
        bool _bcd;
        int _bytes;
        UInt8 _lowCount;
        bool _firstByte;
        bool _gate;
        bool _latched;
        State _state;
        bool _outputHigh;

        Connector _gateConnector;
        OptimizedOutputConnector<bool> _output;
    };
    Timer _timers[3];
    int _address;
};
