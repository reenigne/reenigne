template<class T> class Intel8255PPITemplate
  : public ISA8BitComponent<Intel8255PPITemplate<T>>
{
public:
    static String typeName() { return "Intel8255PPI"; }
    Intel8255PPITemplate()
    {
        _mode = 0x1b;
        for (int i = 0; i < 3; ++i) {
            _bytes[i]._ppi = this;
            _bytes[i]._i = i;
            _incoming[i] = 0;
            _outgoing[i] = 0xff;
            _input[i] = 0;
            _output[i] = 0;
        }
        for (int i = 0; i < 24; ++i) {
            _bits[i]._ppi = this;
            _bits[i]._i = i;
        }
    }
    void setAddress(UInt32 address) { _address = address & 3; }
    void read()
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
        this->set(d);
    }
    void write(UInt8 data)
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
                _input[2] = 0;
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
                if (n.length() == 1 && n[0] >= 'a' && n[0] <= 'c')
                    return BidirectionalConnector<Byte>::Type();
                if (n.length() == 2 && n[0] >= 'a' && n[0] <= 'c' &&
                    n[1] >= '0' && n[1] <= '7')
                    return BidirectionalConnector<bool>::Type();
                return ISA8BitComponent::Type::Body::member(name);
            }
        };
    };

    Value getValue(Identifier i) const
    {
        String n = i.name();
        if (n.length() == 1 && n[0] >= 'a' && n[0] <= 'c')
            return _bytes[n[0] - 'a'].getValue();
        if (n.length() == 2 && n[0] >= 'a' && n[0] <= 'c' &&
            n[1] >= '0' && n[1] <= '7')
            return _bits[((n[0] - 'a') << 3) | (n[1] - '0')].getValue();
        return ISA8BitComponent::getValue(i);
    }

    void setData(int i, Byte v) { incoming(i, v); }
    void setData(int i, bool v)
    {
        int n = i >> 3;
        int b = 1 << (i & 7);
        incoming(n, (_incoming[n] & ~b) | (v ? b : 0));
    }
    String save() const
    {
        String s("{\n");
        s += "  active: " + String::Boolean(this->_active) + ",\n";
        s += "  address: " + hex(_address, 5) + ",\n";
        s += "  incoming: {" + hex(_incoming[0], 2) + ", " +
            hex(_incoming[1], 2) + ", " + hex(_incoming[2], 2) + "},\n";
        s += "  outgoing: {" + hex(_outgoing[0], 2) + ", " +
            hex(_outgoing[1], 2) + ", " + hex(_outgoing[2], 2) + "},\n";
        s += "  input: {" + hex(_input[0], 2) + ", " + hex(_input[1], 2) +
            ", " + hex(_input[2], 2) + "},\n";
        s += "  output: {" + hex(_output[0], 2) + ", " + hex(_output[1], 2) +
            ", " + hex(_output[2], 2) + "}}\n";
        return s;
    }
    ::Type persistenceType() const
    {
        typedef StructuredType::Member M;
        List<M> members;
        members.add(M("active", false));
        members.add(M("address", 0));
        members.add(M("mode", 0x1b));
        members.add(M("incoming",
            Value(SequenceType(IntegerType()), List<Value>())));
        members.add(M("outgoing",
            Value(SequenceType(IntegerType()), List<Value>())));
        members.add(M("input",
            Value(SequenceType(IntegerType()), List<Value>())));
        members.add(M("output",
            Value(SequenceType(IntegerType()), List<Value>())));
        return StructuredType("PPI", members);
    }
    void load(const Value& value)
    {
        auto members = value.value<HashTable<Identifier, Value>>();
        this->_active = members["active"].value<bool>();
        _address = members["address"].value<int>();

        auto incoming = members["incoming"].value<List<Value>>();
        int j = 0;
        for (auto i : incoming) {
            _incoming[j % 3] = i.value<int>();
            ++j;
        }
        auto outgoing = members["outgoing"].value<List<Value>>();
        j = 0;
        for (auto i : outgoing) {
            _outgoing[j % 3] = i.value<int>();
            ++j;
        }
        auto input = members["input"].value<List<Value>>();
        j = 0;
        for (auto i : input) {
            _input[j % 3] = i.value<int>();
            ++j;
        }
        auto output = members["output"].value<List<Value>>();
        j = 0;
        for (auto i : output) {
            _output[j % 3] = i.value<int>();
            ++j;
        }
    }

private:
    template<class T> class Connector : public BidirectionalConnector<T>
    {
    public:
        void setData(T v) { _ppi->setData(_i, v); }
        Intel8255PPI* _ppi;
        int _i;
    };
    void outgoing(int i, UInt8 v)
    {
        if (v != _outgoing[i]) {
            _bytes[i]._other->setData(_tick, v);
            for (int b = 0; b < 8; ++b)
                if (((v ^ _outgoing[i]) & (1 << b)) != 0)
                    _bits[(i<<3) | b]._other->
                        setData(_tick, (v & (1 << b)) != 0);
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
    Byte _input[3];
    Byte _output[3];
    int _address;
};
