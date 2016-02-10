template<class T> class DMAPageRegistersT
  : public ISA8BitComponentBase<DMAPageRegisters>
{
public:
    static String typeName() { return "DMAPageRegisters"; }
    DMAPageRegistersT(Component::Type type)
      : ISA8BitComponentBase(type), _connector(this)
    {
        connector("", &_connector);
        for (int i = 0; i < 4; ++i)
            _dmaPages[i] = 0;
        persist("data", &_dmaPages[0], ArrayType(ByteType(), 4));
        persist("address", &_address);
    }
    ISA8BitComponent* setAddressWriteIO(UInt32 address)
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

    class Connector : public ConnectorBase<Connector>
    {
    public:
        Connector(DMAPageRegisters* c) : ConnectorBase<Connector>(c) { }
        static String typeName() { return "DMAPageRegisters.Connector"; }
        static auto protocolDirection()
        {
            return ProtocolDirection(DMAPageRegistersProtocol(), true);
        }
    protected:
        void connect(::Connector* other)
        {
            static_cast<ISA8BitBus::DMAPageRegistersSocket*>(other)->_bus->
                setDMAPageRegisters(static_cast<DMAPageRegisters*>(
                    component());
        }
    };
private:
    int _address;
    Byte _dmaPages[4];
    Connector _connector;
};
