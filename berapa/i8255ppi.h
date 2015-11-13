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
        // IBF is reset by rising edge of RD input
        if (_modeA == 1 && _directionA)
            ibfA(false);
        if (_modeB == 1 && _directionB)
            ibfB(false);

        // INTR is reset by falling edge of -RD
        if (_modeA == 1 && _directionA)
            intrA(false);
        if (_modeB == 1 && _directionB)
            intrB(false);


        UInt8 d = 0;
        switch(_address) {
            case 0:
                if (_modeA == 0)
                    if (_directionA)
                        d = _incoming[0];
                    else
                        d = _outgoing[0];
                break;
            case 1:
                if (_modeB == 0)
                    if (_directionB)
                        d = _incoming[1];
                    else
                        d = _outgoing[1];
                break;
            case 2:
                if (_modeA == 0)
                    if (_directionCUpper)
                        d = _incoming[2] & 0xf0;
                    else
                        d = _outgoing[2] & 0xf0;
                if (_modeB == 0)
                    if (_directionCLower)
                        d |= _incoming[2] & 0x0f;
                    else
                        d |= _outgoing[2] & 0x0f;
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
                        outgoing(0, data);
                break;
            case 1:
                if (_modeB == 0)
                    if (!_directionB)
                        outgoing(1, data);
                break;
            case 2:
                if (_modeA == 0)
                    if (_directionCUpper)
                        needSet = true;
                    else
                        d = (d & 0x0f) | (_outgoing[2] & 0xf0);
                if (_modeB == 0)
                    if (_directionCLower)
                        needSet = true;
                    else
                        d = (d & 0xf0) | (_outgoing[2] & 0x0f);
                if (needSet)
                    outgoing(2, data);
                break;
            case 3:
                if ((data & 0x80) == 0) {
                    // Bit set/reset
                    if ((data & 1) == 0)
                        _portC &= ~(1 << ((data >> 1) & 3));
                    else
                        _portC |= 1 << ((data >> 1) & 3);
                }
                else {
                    // Mode set
                    // TODO: reset status flip-flops
                    _directionCLower = ((data & 1) != 0);
                    _directionB = ((data & 2) ! = 0);
                    _modeB = ((data & 4) >> 2);

                    _directionCUpper = ((data & 8) != 0);
                    _directionA = ((data & 0x10) != 0);
                    _modeA = ((data >> 5) & 3);

                    outgoing(0, _directionA ? 0xff : 0);
                    outgoing(1, _directionB ? 0xff : 0);
                    outgoing(2, (_directionCUpper ? 0xf0 : 0) |
                        (_directionCLower ? 0x0f : 0));
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

    void setData(int i, Byte v) { incoming(i, v); }
    void setData(int i, bool v)
    {
        int n = i >> 3;
        int b = 1 << (i & 7);
        incoming(n, (_incoming[n] & ~b) | (v ? b : 0));
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
    void outgoing(int i, UInt8 v)
    {
        if (v != _outgoing[i]) {
            _bytes[i].other->setData(v);
            for (int b = 0; b < 8; ++b)
                if (((v ^ _outgoing[i]) & (1 << b)) != 0)
                    _bits[(i<<3) | b].other->setData((v & (1 << b)) != 0);
            _outgoing[i] = v;
        }
    }
    void checkInterrupts()
    {
        if (_modeA == 1 && _directionA && stbA() && ibfA() && inteAIn())
            intrA(true);
        if (_modeB == 1 && _directionB && stbB() && ibfB() && inteB())
            intrB(true);
    }
    void incoming(int i, UInt8 v)
    {
        if (i == 2) {
            if (_modeA == 1 && _directionA)
                if (stbA() && (v & 0x10) == 0) {
                    // -STB_A falling
                    _input[0] = _incoming[0];
                    ibfA(true);
                }
            if (_modeB == 1 && _directionB)
                if ((_incoming[2] & 4) != 0 && (v & 4) == 0) {
                    // -STB_B falling
                    _input[1] = _incoming[1];
                    ibfB(true);
                }
        }
        _incoming[i] = v;
        if (i == 2)
            checkInterrupts();
    }
    void setOutgoing()
    {
        outgoing(0, _directionA ? 0xff : _port[0]);
        outgoing(1, _directionB ? 0xff : _port[1]);
        Byte d;
        if (_modeB == 1)
            d = 4 | ((_directionB ? _ibfB : _obfB) ? 2 : 0) | (_intrB ? 1 : 0);
        else
            d = ((_directionCLower ? 0xff : _port[2]) & 7);
        if (_modeA != 0)
            d |= (_intrA ? 8 : 0);
        else
            d |= ((_directionCLower ? 0xff : _port[2]) & 8);
        if (_modeA == 2 && (_modeA == 1 && _directionA))
            d |= (_ibfA ? 0x20 : 0) | 0x10;
        else
            d |= ((_directionCUpper ? 0xff : _port[2]) & 0x30);
        if (_modeA == 2 && (_modeA == 1 && !_directionA))
            d |= (_obfA ? 0x80 : 0) | 0x40;
        else
            d |= ((_directionCUpper ? 0xff : _port[2]) & 0xc0);
        outgoing(2, d);
    }

    void obfA(bool x) { outgoing(2, (_outgoing[2] & ~0x80) | (x ? 0x80 : 0)); }
    bool ackA() const { return (_incoming[2] & 0x40) != 0; }
    void ibfA(bool x) { outgoing(2, (_outgoing[2] & ~0x20) | (x ? 0x20 : 0)); }
    bool stbA() const { return (_incoming[2] & 0x10) != 0; }
    bool ibfA() const { return (_outgoing[2] & 0x20) != 0; }
    void intrA(bool x) { outgoing(2, (_outgoing[2] & ~8) | (x ? 8 : 0)); }
    bool ackB() const { return (_incoming[2] & 4) != 0; }
    void obfB(bool x) { outgoing(2, (_outgoing[2] & ~2) | (x ? 2 : 0)); }
    void intrB(bool x) { outgoing(2, (_outgoing[2] & ~1) | (x ? 1 : 0)); }
    bool stbB() const { return (_incoming[2] & 4) != 0; }
    void ibfB(bool x) { outgoing(2, (_outgoing[2] & ~2) | (x ? 2 : 0)); }
    bool inteAIn() const { return (_port[2] & 0x10) != 0; }
    bool inteAOut() const { return (_port[2] & 0x40) != 0; }
    bool inteB() const { return (_port[2] & 4) != 0; }

    Connector<Byte> _bytes[3];
    Connector<bool> _bits[24];
    int _modeA;
    int _modeB;
    bool _directionA; // true for input
    bool _directionB;
    bool _directionCUpper;
    bool _directionCLower;
    bool _obfA;
    bool _ackA;
    bool _ibfA;
    bool _stbA;
    bool _intrA;
    bool _inteAIn;
    bool _inteAOut;
    bool _obfB;
    bool _ackB;
    bool _ibfB;
    bool _stbB;
    bool _intrB;

    Byte _incoming[3];
    Byte _outgoing[3];
    Byte _input[2];
    Byte _port[3];
    int _address;
    Intel8259PIC* _pic;
};
