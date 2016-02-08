class Intel8255PPI : public ISA8BitComponentBase<Intel8255PPI>
{
public:
    static String typeName() { return "Intel8255PPI"; }
    Intel8255PPI(Component::Type type)
      : ISA8BitComponentBase<Intel8255PPI>(type), _bytes{this, this, this},
        _bits{this, this, this, this, this, this, this, this, this, this, this,
        this, this, this, this, this, this, this, this, this, this, this, this,
        this}
    {
        _mode = 0x1b;
        for (int i = 0; i < 3; ++i) {
            _bytes[i]._i = i;
            this->connector(codePoint('a' + i), &_bytes[i]);
        }
        for (int i = 0; i < 24; ++i) {
            _bits[i]._i = i;
            this->connector(String(codePoint('a' + (i >> 3))) + decimal(i & 7),
                &_bits[i]);
        }
        this->persist("address", &_address);
        ArrayType t(ByteType(), 3);
        this->persist("incoming", &_incoming[0], t);
        this->persist("outgoing", &_outgoing[0], 0xff, t);
        this->persist("input", &_input[0], ArrayType(ByteType(), 2));
        this->persist("output", &_output[0], t);
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
        // TODO: IBF is reset by rising edge of RD input
        if (inputA())
            ibfA(false);
        if (inputB())
            ibfB(false);

        // TODO: INTR is reset by falling edge of -RD
        if (inputA())
            intrA(false);
        if (inputB())
            intrB(false);

        UInt8 d = 0;
        switch (_address) {
            case 0:
                if (inputA())
                    d = _input[0];
                else
                    d = (directionA() ? _incoming[0] : _outgoing[0]);
                break;
            case 1:
                if (inputB())
                    d = _input[1];
                else
                    d = (directionB() ? _incoming[1] : _outgoing[1]);
                break;
            case 2:
                if (directionCUpper())
                    d = _incoming[2] & 0xf0;
                else
                    d = _outgoing[2] & 0xf0;
                if (directionCLower())
                    d |= _incoming[2] & 0x0f;
                else
                    d |= _outgoing[2] & 0x0f;
                if (modeB() == 1)
                    d = (d & 0xf8) | bitsB();
                if (inputA())
                    d = (d & 0xc7) | bitsAIn();
                if (outputA())
                    d = (d & 0x37) | bitsAOut();
                break;
            case 3:
                // Disallowed by datasheet. TODO: figure out what this actually
                // is.
                d = _mode;
                break;
        }
        return d;
    }
    void writeIO(Tick tick, UInt8 data)
    {
        // TODO: INTR is reset by falling edge of -WR
        if (outputA())
            intrA(false);
        if (outputB())
            intrB(false);

        UInt8 d = data;
        bool needSet = false;
        if (_address == 3) {
            if ((data & 0x80) == 0) {
                // Bit set/reset
                int n = (data >> 1) & 7;
                if ((data & 1) != 0)
                    _output[2] |= 1 << n;
                else
                    _output[2] &= ~(1 << n);
                checkInterrupts();
            }
            else {
                // Mode set
                _output[0] = 0;
                _output[1] = 0;
                _output[2] = 0;
                _input[0] = 0;
                _input[1] = 0;
                _mode = data;
            }
        }
        else {
            // TODO: Datasheet implies that, if programmed as outputs, bits
            // C4-C7 cannot be set by writing to port C (when not all bits of
            // C are used for control/status).
            _output[_address] = data;
        }

        // TODO: OBF is set by the rising edge of the -WR input.
        if (_address == 0 && ((modeA() == 1 && !directionA()) || mode2()))
            obfA(false);
        if (_address == 1 && modeB() == 1 && !directionB())
            obfB(false);

        setOutgoing();
    }

    void setData(int i, Byte v) { incoming(i, v); }
    void setData(int i, bool v)
    {
        int n = i >> 3;
        int b = 1 << (i & 7);
        incoming(n, (_incoming[n] & ~b) | (v ? b : 0));
    }
private:
    template<class U> class Connector : public BidirectionalConnector<U>
    {
    public:
        Connector(Intel8255PPI* c) : BidirectionalConnector<U>(c) { }
        void setData(Tick tick, U v)
        {
            static_cast<Intel8255PPI*>(component())->setData(_i, v);
        }
        int _i;
    };
    void outgoing(int i, UInt8 v)
    {
        if (v != _outgoing[i]) {
            _bytes[i]._other->setData(this->_tick, v);
            for (int b = 0; b < 8; ++b)
                if (((v ^ _outgoing[i]) & (1 << b)) != 0) {
                    _bits[(i<<3) | b]._other->
                        setData(this->_tick, (v & (1 << b)) != 0);
                }
            _outgoing[i] = v;
        }
    }
    void checkInterrupts()
    {
        if (modeA() == 1 && ((directionA() && stbA() && ibfA() && inteAIn()) ||
            (!directionA() && ackA() && obfA() && inteAOut())))
            intrA(true);
        if (modeB() == 1 && inteB() && ((directionB() && stbB() && ibfB()) ||
            (!directionB() && ackB() && obfB())))
            intrB(true);
    }
    void incoming(int i, UInt8 v)
    {
        _incoming[i] = v;
        if (inputA() && !stbA()) {
            // -STB_A low loads data into input latch
            _input[0] = _incoming[0];
            ibfA(true);
        }
        if (inputB() && !stbB()) {
            // -STB_B low loads data into input latch
            _input[1] = _incoming[1];
            ibfB(true);
        }
        if (i == 2) {
            // -ACK low resets -OBF.
            if (!ackA() && outputA())
                obfA(true);
            if (!ackB() && outputB())
                obfB(true);
            checkInterrupts();
            setOutgoing();
        }
    }
    void setOutgoing()
    {
        if (mode2())
            outgoing(0, ackA() ? 0xff : _output[0]);
        else
            outgoing(0, directionA() ? 0xff : _output[0]);
        outgoing(1, directionB() ? 0xff : _output[1]);
        Byte d;
        if (modeB() == 1)
            d = bitsB() | 4;
        else
            d = ((directionCLower() ? 0xff : _output[2]) & 7);
        if (modeA() != 0)
            d |= (intrA() ? 8 : 0);
        else
            d |= ((directionCLower() ? 0xff : _output[2]) & 8);
        if (inputA())
            d |= (ibfA() ? 0x20 : 0) | 0x10;
        else
            d |= ((directionCUpper() ? 0xff : _output[2]) & 0x30);
        if (outputA())
            d |= (obfA() ? 0x80 : 0) | 0x40;
        else
            d |= ((directionCUpper() ? 0xff : _output[2]) & 0xc0);
        outgoing(2, d);
    }

    void obfA(bool x) { outgoing(2, (_outgoing[2] & ~0x80) | (x ? 0x80 : 0)); }
    bool obfA() const { return (_outgoing[2] & 0x80) != 0; }
    bool ackA() const { return (_incoming[2] & 0x40) != 0; }
    void ibfA(bool x) { outgoing(2, (_outgoing[2] & ~0x20) | (x ? 0x20 : 0)); }
    bool stbA() const { return (_incoming[2] & 0x10) != 0; }
    bool ibfA() const { return (_outgoing[2] & 0x20) != 0; }
    void intrA(bool x) { outgoing(2, (_outgoing[2] & ~8) | (x ? 8 : 0)); }
    bool intrA() const { return (_outgoing[2] & 8) != 0; }
    bool ackB() const { return (_incoming[2] & 4) != 0; }
    void obfB(bool x) { outgoing(2, (_outgoing[2] & ~2) | (x ? 2 : 0)); }
    bool obfB() const { return (_outgoing[2] & 2) != 0; }
    void intrB(bool x) { outgoing(2, (_outgoing[2] & ~1) | (x ? 1 : 0)); }
    bool intrB() const { return (_outgoing[2] & 1) != 0; }
    bool stbB() const { return (_incoming[2] & 4) != 0; }
    void ibfB(bool x) { outgoing(2, (_outgoing[2] & ~2) | (x ? 2 : 0)); }
    bool ibfB() const { return (_outgoing[2] & 2) != 0; }
    bool inteAIn() const { return (_output[2] & 0x10) != 0; }
    bool inteAOut() const { return (_output[2] & 0x40) != 0; }
    bool inteB() const { return (_output[2] & 4) != 0; }
    bool directionA() const { return (_mode & 0x10) != 0; }
    bool directionB() const { return (_mode & 2) != 0; }
    int modeA() const { return (_mode >> 5) & 3; }
    int modeB() const { return (_mode >> 2) & 1; }
    bool directionCLower() const { return (_mode & 1) != 0; }
    bool directionCUpper() const { return (_mode & 8) != 0; }
    bool mode2() const { return (_mode & 0x40) != 0; }
    bool inputA() const { return mode2() || (modeA() == 1 && directionA()); }
    bool outputA() const { return mode2() || (modeA() == 1 && !directionA()); }
    bool inputB() const { return modeB() == 1 && directionB(); }
    bool outputB() const { return modeB() == 1 && !directionB(); }
    int bitsB() const
    {
        return ((directionB() ? stbB() : ackB()) ? 4 : 0) |
            ((directionB() ? ibfB() : obfB()) ? 2 : 0) | (intrB() ? 1 : 0);
    }
    int bitsAIn() const
    {
        return (ibfA() ? 0x20 : 0) | (stbA() ? 0x10 : 0) | (intrA() ? 8 : 0);
    }
    int bitsAOut() const
    {
        return (obfA() ? 0x80 : 0) | (ackA() ? 0x40 : 0) | (intrA() ? 8 : 0);
    }

    Connector<Byte> _bytes[3];
    Connector<bool> _bits[24];
    Intel8259PIC* _pic;

    Byte _mode;
    // _incoming is the bits that would be on the pins if we were not
    // outputting anything (i.e. the value we get from the external device).
    // _outgoing is the bits that would be on the pins if the external device
    // is not outputting anything (i.e. the value we're sending to the external
    // device). The actual value on the pins is _incoming & _outgoing (assuming
    // open-collector, pull-high logic).
    Byte _incoming[3];
    Byte _outgoing[3];
    Byte _input[2];
    Byte _output[3];
    int _address;
};
