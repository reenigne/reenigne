class Intel8259PIC : public ISA8BitComponent<Intel8259PIC>
{
public:
    Intel8259PIC()
      : _interruptrdy(false), _secondAck(false), _state(stateReady), _imr(0xff)
    {
        for (int i = 0; i < 8; ++i)
            _irqConnector[i].init(this, i);
    }
    Value getValue(Identifier i) const
    {
        String n = i.name();
        if (n.length() == 4 && n.subString(0, 3) == "irq" && n[3] >= '0' &&
            n[3] < '8')
            return _irqConnector[n[3] - '0'].getValue();
        return Component::getValue(i);
    }
    class Connector : public InputConnector<bool>
    {
    public:
        void setData(bool v) { _pic->setIRQ(_i, v); }

        Intel8259PIC* _pic;
        int _i;
    };
    void setIRQ(int i, bool v)
    {
        // TODO
    }

    void setAddress(UInt32 address) { _address = address & 0x1; }
    void read()
    {
        if (_address == 1) {
            if (_secondAck) {
                set(_interruptnum);
                _secondAck = false;
            }
            else
                set(_imr);
        }
    }
    void write(UInt8 data)
    {
        if (_address == 0) {
            if(data & 0x10) {
                _icw1 = data & 0x0f;
                _state = stateICW2;
            }
        }
        else {
            switch (_state) {
                case stateICW2:
                    _offset = data;
                    _state = stateICW3;
                    break;
                case stateICW3:
                    if ((_icw1 & 1) != 0)
                        _state = stateICW4;
                    else 
                        _state = stateReady;
                    break;
                case stateICW4:
                    _icw4 = data;
                    _state = stateReady;
                    break;
                case stateReady:
                    _imr = data;
                    break;
            }
        }
    }
    bool interruptRequest() { return _interruptrdy; }
    void interruptAcknowledge()
    {
        if (!_secondAck)
            _secondAck = true;
        else
            _active = true;
        _interruptrdy = false;
    }

    void requestInterrupt(int line)
    {
        if (_state == stateReady) {
            _interruptnum = line + _offset;
            _interruptrdy = (((~_imr) & (1 << line)) != 0);
            _interrupt = false;
            _secondAck = false;
        }
        else
            _interrupt = false;
    }
    static String name() { return "Intel8259PIC"; }

private:
    UInt8 _interruptnum;
    enum State
    {
        stateReady,
        stateICW2,
        stateICW3,
        stateICW4,
    } _state;

    bool _interrupt;
    bool _interruptrdy;
    bool _secondAck;

    int _address;
    UInt8 _offset;
    UInt8 _irr;
    UInt8 _imr;
    UInt8 _isr;

    UInt8 _icw1;
    UInt8 _icw4;

    IRQConnector _irqConnector[8];
};