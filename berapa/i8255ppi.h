template<class T> class Intel8255PPITemplate
  : public ISA8BitComponent<Intel8255PPITemplate<T>>
{
public:
    Intel8255PPITemplate()
    {
        _directionA = true;
        _directionB = true;
        _directionCUpper = true;
        _directionCLower = true;
        _modeA = 0;
        _modeB = 0;
        for (int i = 0; i < 3; ++i) {
            _bytes[i]._ppi = this;
            _bytes[i]._i = i;
        }
        for (int i = 0; i < 24; ++i) {
            _bits[i]._ppi = this;
            _bits[i]._i = i;
        }
    }
    void setAddress(UInt32 address)
    {
        _address = address & 0x3;
        this->_active = (address & 0x400003f0) == 0x40000060;
    }
    void read()
    {
        UInt8 d = 0;
        switch(_address) {
            case 0:
                if (_modeA == 0)
                    if (_directionA)
                        d = _external[0];
                    else
                        d = _output[0];
                break;
            case 1:
                if (_modeB == 0)
                    if (_directionB)
                        d = _external[1];
                    else
                        d = _output[1];
                break;
            case 2:
                if (_modeA == 0)
                    if (_directionCUpper)
                        d = _external[2] & 0xf0;
                    else
                        d = _output[2] & 0xf0;
                if (_modeB == 0)
                    if (_directionCLower)
                        d |= _external[2] & 0x0f;
                    else
                        d |= _output[2] & 0xf0;
                break;
            case 3:
                // Disallowed by datasheet. TODO: figure out what this actually
                // is.
                d = 0xff;
                break;
        }
        this->set(d);
    }
    void write(UInt8 data)
    {
        UInt8 d = data;
        bool needSet = false;
        switch(_address) {
            case 0:
                if (_modeA == 0)
                    if (!_directionA)
                        set(0, data);
                break;
            case 1:
                if (_modeB == 0)
                    if (!_directionB)
                        set(1, data);
                break;
            case 2:
                if (_modeA == 0)
                    if (_directionCUpper)
                        needSet = true;
                    else
                        d = (d & 0x0f) | (_external[2] & 0xf0);
                if (_modeB == 0)
                    if (_directionCLower)
                        needSet = true;
                    else
                        d = (d & 0xf0) | (_external[2] & 0x0f);
                if (needSet)
                    set(2, data);
                break;
            case 3:
                if ((data & 0x80) == 0) {
                    // Bit set/reset
                    if ((data & 1) == 0)
                        _output[2] &= ~(1 << ((data >> 1) & 3));
                    else
                        _output[2] |= 1 << ((data >> 1) & 3);
                }
                else {
                    // Mode set
                    // TODO: reset status flip-flops
                    _directionCLower = ((data & 1) != 0);
                    _directionB = ((data & 2) ! = 0);
                    _modeB = ((data & 4) >> 2);

                    _directionCUppser = ((data & 8) != 0);
                    _directionA = ((data & 0x10) != 0);
                    _modeA = ((data >> 5) & 3);

                    set(0, 0);
                    set(1, 0);
                    set(2, 0);
                }

        }
    }
    static String name() { return "Intel8255PPI"; }

    class Type : public ISA8BitComponent::Type
    {
    public:
        Type(Simulator* simulator)
            : ISA8BitComponent::Type(new Body(simulator)) { }
    private:
        class Body : public ISA8BitComponent::Type::Body
        {
        public:
            Body(Simulator* simulator)
                : ISA8BitComponent::Type::Body(simulator) { }
            ::Type member(Identifier name) const
            {
                String n = name.name();
                if (n.count() == 1 && n[0] >= 'a' && n[0] <= 'c')
                    return BidirectionalConnector<Byte>::Type();
                if (n.count() == 2 && n[0] >= 'a' && n[0] <= 'c' &&
                    n[1] >= '0' && n[1] <= '7')
                    return BidirectionalConnector<bool>::Type();
                return ISA8BitComponent::Type::Body::member(name);
            }
        };
    };

    Value getValue(Identifier i) const
    {
        String n = name.name();
        if (n.count() == 1 && n[0] >= 'a' && n[0] <= 'c')
            return _bytes[n[0]-'a'].getValue();
        if (n.count() == 2 && n[0] >= 'a' && n[0] <= 'c' &&
            n[1] >= '0' && n[1] <= '7')
            return _bits[((n[0]-'a') << 3) | (n[1] - '0')].getValue();
        return ISA8BitComponent::getValue(i);
    }

    void setData(int i, Byte v) { _external[i] = v; }
    void setData(int i, bool v)
    {
        int n = i >> 3;
        int b = 1 << (i & 7);
        _external[n] = (_external[n] & ~b) | (v ? b : 0);
    }
private:
    template<class T> class Connector : public BidirectionalConnector<T>
    {
    public:
        void setData(T v) { _ppi->setData(i, v); }

        BidirectionalConnector<T>* other;
        Intel8255PPI* _ppi;
        int _i;
    };
    void set(int i, UInt8 v)
    {
        if (v != _output[i]) {  // TODO: is this right?
            _bytes[i].other->setData(v);
            for (int b = 0; b < 8; ++b)
                if (((v ^ _output[i]) & (1 << b)) != 0)
                    _bits[(i<<3) | b].other->setData((v & (1 << b)) != 0);
            _output[i] = v;
        }
    }

    Connector<Byte> _bytes[3];
    Connector<bool> _bits[24];
    int _modeA;
    int _modeB;
    bool _directionA; // true for input
    bool _directionB;
    bool _directionCUpper;
    bool _directionCLower;
    Byte _external[3];
    int _address;
    Intel8259PIC* _pic;
};
