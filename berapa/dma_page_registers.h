template<class T> class DMAPageRegistersT
  : public ISA8BitComponent<DMAPageRegisters>
{
public:
    static String typeName() { return "DMAPageRegisters"; }
    DMAPageRegistersT(Component::Type type)
      : ISA8BitComponent(type), _connector(this)
    {
        connector("", &_connector);
        for (int i = 0; i < 4; ++i)
            _dmaPages[i] = 0;
        persist("data", &_dmaPages[0], ArrayType(ByteType(), 4));
        persist("address", &_address);
    }
    ISA8BitComponentBase* setAddressWriteIO(UInt32 address)
    {
        _address = address & 3;
        return this;
    }
    void writeIO(Tick tick, UInt8 data) { _dmaPages[_address] = data & 0x0f; }
    UInt8 pageForChannel(int channel)
    {
        switch (channel) {
            case 2: return _dmaPages[1];
            case 3: return _dmaPages[2];
            default: return _dmaPages[3];
        }
    }

    class Connector : public ::Connector
    {
    public:
        Connector(DMAPageRegisters* c) : ::Connector(c), _c(c) { }
        Component::Type defaultComponentType(Simulator* simulator)
        {
            throw Exception(_c->name() + " needs to be connected");
        }
        class Type : public NamedNullary<::Connector::Type, Type>
        {
        public:
            static String name() { return "DMAPageRegisters.Connector"; }
            class Body : public NamedNullary<::Connector::Type, Type>::Body
            {
            public:
                bool compatible(::Connector::Type other) const
                {
                    return other == ISA8BitBus::DMAPageRegistersSocket::Type();
                }
            };
        };
    protected:
        ::Connector::Type type() const { return Type(); }
        void connect(::Connector* other)
        {
            static_cast<ISA8BitBus::DMAPageRegistersSocket*>(other)->_bus->
                setDMAPageRegisters(_c);
        }
    private:
        DMAPageRegisters* _c;
    };
private:
    int _address;
    Byte _dmaPages[4];
    Connector _connector;
};
