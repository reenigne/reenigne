template<class T> class Intel8253PITTemplate
  : public ISA8BitComponent<Intel8253PITTemplate<T>>
{
public:
    static String typeName() { return "Intel8253PIT"; }
    Intel8253PITTemplate()
    {
        for (int i = 0; i < 3; ++i)
            _timers[i].setGate(true);
        persist("address", &_address, 0, HexPersistenceType(5));
        persist("timers", &_timers[0], ArrayPersistenceType(Timer::Type(), 3));
    }
    void simulateCycle()
    {
        for (int i = 0; i < 3; ++i)
            _timers[i]->simulateCycle();
    }
    void setAddress(UInt32 address) { _address = address & 3; }
    void read()
    {
        if (_address < 3)
            this->set(_timers[_address].read());
    }
    void write(UInt8 data)
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
    class Timer
    {
    public:
        Timer()
        {
            List<EnumerationType::Value> stateValues;
            for (int i = stateStopped0; i <= stateCounting2; ++i) {
                State s = static_cast<State>(i);
                stateValues.add(EnumerationType::Value(stringForState(s), s));
            }
            _stateType = EnumerationType("PITState", stateValues);
            persist("value", &_value, 0, HexPersistenceType(4));
            persist("latch", &_latch, 0, HexPersistenceType(4));
            persist("count", &_count, 0, HexPersistenceType(4));
            persist("bcd", &_bcd, false);
            persist("bytes", &_bytes, 0);
            persist("lowCount", &_lowCount, 0, HexPersistenceType(2));
            persist("firstByte", &_firstByte, false);
            persist("gate", &_gate, false);
            persist("output", &_output, false);
            persist("latched", &_latched, false);
            persist("state", &_state, stateStopped0);
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

        static String stringForState(State state)
        {
            switch (state) {
                case stateStopped0:  return "stopped0";
                case stateCounting0: return "counting0";
                case stateStopped1:  return "stopped1";
                case stateStart1:    return "start1";
                case stateCounting1: return "counting1";
                case stateStopped2:  return "stopped2";
                case stateGateLow2:  return "gateLow2";
                case stateCounting2: return "counting2";
            }
            return "";
        }

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
        virtual void outputChanged(bool output)=0;

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
        ::Type _stateType;
    };
    Timer _timers[3];
    int _address;
};
