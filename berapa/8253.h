template<class T> class Intel8253PITTemplate
  : public ISA8BitComponentTemplate<T>
{
public:
    // The PIT input clock is 1/4 the frequency of the CPU clock.
    Rational<int> hDotsPerCycle() const { return 12; }
    Intel8253PITTemplate()
    {
        _timers[0] = &_timer0;
        _timers[1] = &_timer1;
        _timers[2] = &_timer2;
        for (int i = 0; i < 3; ++i)
            _timers[i]->setGate(true);
    }
    //void site()
    //{
    //    _pic = this->_simulator->getPIC();
    //    _timer0.setPIC(_pic);
    //    _timer1.setSite(this->_simulator);
    //    _timer2.setPPI(this->_simulator->getPPI());
    //}
    void simulateCycle()
    {
        for (int i = 0; i < 3; ++i)
            _timers[i]->simulateCycle();
    }
    void setAddress(UInt32 address)
    {
        _address = address & 3;
        this->_active = (address & 0x400003e0) == 0x40000040;
    }
    void read()
    {
        if (_address < 3)
            this->set(_timers[_address]->read());
    }
    void write(UInt8 data)
    {
        if (_address < 3)
            _timers[_address]->write(data);
        else {
            int timer = (data >> 6) & 3;
            if (timer < 3)
                _timers[timer]->control(data & 0x3f);
        }
    }
    void setT2Gate(bool gate) { _timers[2]->setGate(gate); }

    String save() const
    {
        String s = String() + 
            "{ active: " + String::Boolean(this->_active) +
            ", tick: " + String::Decimal(this->_tick) +
            ", address: " + hex(_address, 5) +
            ", timers: { ";
        bool needComma = false;
        for (int i = 0; i < 4; ++i) {
            if (needComma)
                s += ", ";
            needComma = true;
            s += _timers[i]->save();
        }
        return s + " }}\n";
    }
    ::Type persistenceType() const
    {
        List<StructuredType::Member> members;
        members.add(StructuredType::Member("active", false));
        members.add(StructuredType::Member("tick", 0));
        members.add(StructuredType::Member("address", 0));
        members.add(StructuredType::Member("timers",
            TypedValue(SequenceType(_timer0.type()), List<TypedValue>())));
        return StructuredType("PIT", members);
    }
    void load(const TypedValue& value)
    {
        auto members = value.value<Value<HashTable<String, TypedValue>>>();
        this->_active = (*members)["active"].value<bool>();
        this->_tick = (*members)["tick"].value<int>();
        _address = (*members)["address"].value<int>();
        auto timers = (*members)["timers"].value<List<TypedValue>>();

        int j = 0;
        for (auto i = timers.begin(); i != timers.end(); ++i) {
            _timers[j]->load((*i).value<TypedValue>());
            ++j;
            if (j == 3)
                break;
        }
        for (;j < 3; ++j) {
            _timers[j]->load(TypedValue(StructuredType(String(),
                List<StructuredType::Member>()),
                Value<HashTable<String, TypedValue>>()).
                convertTo(_timer0.type()));
        }
    }

    class Type : public Component::Type
    {
    public:
        Type(Simulator* simulator)
          : Component::Type(new Implementation(simulator)) { }
    private:
        class Implementation : public Component::Type::Implementation
        {
        public:
            Implementation(Simulator* simulator)
              : Component::Type::Implementation(simulator) { }
            String toString() const { return "Intel8253PIT"; }
        };
    };
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

        String save() const
        {
            return String("\n    ") + 
                "{ value: " + hex(_value, 4) +
                ", latch: " + hex(_latch, 4) +
                ", count: " + hex(_count, 4) +
                ", bcd: " + String::Boolean(_bcd) +
                ", bytes: " + String::Decimal(_bytes) +
                ", lowCount: " + hex(_lowCount, 2) +
                ", firstByte: " + String::Boolean(_firstByte) +
                ", gate: " + String::Boolean(_gate) +
                ", output: " + String::Boolean(_output) +
                ", latched: " + String::Boolean(_latched) +
                ", state: " + stringForState(_state) +
                " }";
        }
        ::Type type() const
        {
            List<StructuredType::Member> members;
            members.add(StructuredType::Member("value", 0));
            members.add(StructuredType::Member("latch", 0));
            members.add(StructuredType::Member("count", 0));
            members.add(StructuredType::Member("bcd", false));
            members.add(StructuredType::Member("bytes", 0));
            members.add(StructuredType::Member("lowCount", 0));
            members.add(StructuredType::Member("firstByte", false));
            members.add(StructuredType::Member("gate", false));
            members.add(StructuredType::Member("output", false));
            members.add(StructuredType::Member("latched", false));
            members.add(StructuredType::Member("state", 
                TypedValue(_stateType, stateStopped0)));
            return StructuredType("Timer", members);
        }
        void load(const TypedValue& value)
        {
            auto members = value.value<Value<HashTable<String, TypedValue>>>();
            _value = (*members)["value"].value<int>();
            _latch = (*members)["latch"].value<int>();
            _count = (*members)["count"].value<int>();
            _bcd = (*members)["bcd"].value<bool>();
            _bytes = (*members)["bytes"].value<int>();
            _lowCount = (*members)["lowCount"].value<int>();
            _firstByte = (*members)["firstByte"].value<bool>();
            _gate = (*members)["gate"].value<bool>();
            _output = (*members)["output"].value<bool>();
            _latched = (*members)["latched"].value<bool>();
            _state = (*members)["state"].value<State>();
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
        Type _stateType;
    };
    class Timer0 : public Timer
    {
    public:
        void setPIC(Intel8259PIC* pic) { _pic = pic; }
        void outputChanged(bool output)
        {
            if (output)
                _pic->requestInterrupt(0);
        }
    private:
        Intel8259PIC* _pic;
    };
    class Timer1 : public Timer
    {
    public:
        Timer1() : _dmaRequested(false) { }
        void setSite(SimulatorTemplate<T>* simulator)
        {
            _bus = simulator->getBus();
            _dma = simulator->getDMA();
        }
        void outputChanged(bool output)
        {
            if (output) {
                _dma->dmaRequest(0);
                _dmaRequested = true;
            }
        }
        void simulateCycle()
        {
            if (_dmaRequested) {
                if (_dma->dmaAcknowledged(0)) {
                    // Don't do anything with the data we read - the only
                    // purpose of the DMA is to refresh RAM.
                    _dma->dmaComplete(0);
                    _dmaRequested = false;
                }
            }
        }
    private:
        ISA8BitBus* _bus;
        Intel8237DMA* _dma;
        bool _dmaRequested;
    };
    class Timer2 : public Timer
    {
    public:
        void setPPI(Intel8255PPI* ppi) { _ppi = ppi; }
        void outputChanged(bool output)
        {
            if(_ppi->portB() & 1) _ppi->_portc = (_ppi->_portc & ~0x10) | (output ? 0x10 : 0);
            _ppi->_portc = (_ppi->_portc & ~0x40) | (output ? 0x40 : 0);
        }
    private:
        Intel8255PPI* _ppi;
    };
    Timer0 _timer0;
    Timer1 _timer1;
    Timer2 _timer2;
    Timer* _timers[3];
    int _address;
    Intel8259PIC* _pic;
};

