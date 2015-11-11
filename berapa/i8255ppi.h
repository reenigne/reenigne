template<class T> class Intel8255PPITemplate
  : public ISA8BitComponent<Intel8255PPITemplate<T>>
{
public:
    Intel8255PPITemplate()
    {
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
            case 0: d = _data[0]; break;
            case 1: d = _data[1]; break;
            case 2: d = _data[2]; break;
            case 3: d = 0x88; break;
        }
        this->set(d);
    }
    void write(UInt8 data)
    {
        switch(_address) {
            case 0:
                switch (_data[3] & 0x60) {
                    case 0x00:  // Mode 0
                        if ((_data[3] & 0x10) == 0) {
                            // Output
                        }
                        else {
                            // Input
                        }
                        break;
                    case 0x20:  // Mode 1
                        // TODO
                        break;
                    default:  // Mode 2
                        // TODO
                        break;

                }
            case 1:
            case 2:
            case 3:
                // Whenever the mode is changed, all output registers and status flip-flops are reset.
                if ((data & 0x80) == 0) {
                    // Bit set/reset
                    if ((data & 1) == 0)
                        _data[2] &= ~(1 << ((data >> 1) & 3));
                    else
                        _data[2] |= 1 << ((data >> 1) & 3);
                }
                else {
                    // Mode set
                    _data[3] = data;
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

    void setData(int i, Byte t) { _data[i] = t; }
    void setData(int i, bool t)
    {
        int n = i >> 3;
        int b = 1 << (i & 7);
        _data[n] = (_data[n] & ~b) | (t ? b : 0);
    }
    Byte getData(int i, Byte) { return _data[i]; }
    bool getData(int i, bool) { return (_data[i >> 3] & (1 << (i & 7))) != 0; }
private:
    template<class T> class Connector : public BidirectionalConnector<T>
    {
    public:
        void setData(T t) { _ppi->setData(i, t); }
        T getData() { return _ppi->getData(i, T()); }

        Intel8255PPI* _ppi;
        int _i;
    };

    Connector<Byte> _bytes[3];
    Connector<bool> _bits[24];
    Byte _data[4];
    int _address;
    Intel8259PIC* _pic;
};
