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
    void site()
    {
        _pic = this->_simulator->getPIC();
        _timer0.setPIC(_pic);
        _timer1.setSite(this->_simulator);
    }
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
    String save()
    {
        return "";
    }
    Type type() {return Type(); }
    void load(const TypedValue& value) { }
    String name() { return "timer"; }
    TypedValue initial() { return TypedValue(type());}
private:
    class Timer
    {
    public:
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
                    return _latch;
                    break;
                case 1:
                    if (_latched)
                        return _latch;
                    return _value;
                case 2:
                    if (_latched)
                        return _latch >> 8;
                    return _value >> 8;
                case 3:
                    if (_latched) {
                        if (_firstByte)
                            return _latch;
                        return _latch >> 8;
                    }
                    if (_firstByte)
                        return _value;
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
                    _bus->read();
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
        void outputChanged(bool output)
        {
        }
    };
    Timer0 _timer0;
    Timer1 _timer1;
    Timer2 _timer2;
    Timer* _timers[3];
    int _address;
    Intel8259PIC* _pic;
};

