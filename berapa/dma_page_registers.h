template<class T> class DMAPageRegistersTemplate
  : public ISA8BitComponent<DMAPageRegisters>
{
public:
    static String typeName() { return "DMAPageRegisters"; }
    DMAPageRegistersTemplate()
    {
        for (int i = 0; i < 4; ++i)
            _dmaPages[i] = 0;
        persist("data", &_dmaPages[0], 0, ArrayType(ByteType(), 4));
        persist("address", &_address, 0);
    }
    void setAddress(UInt32 address) { _address = address & 3; }
    void write(UInt8 data) { _dmaPages[_address] = data & 0x0f; }
    UInt8 pageForChannel(int channel)
    {
        switch (channel) {
            case 2: return _dmaPages[1];
            case 3: return _dmaPages[2];
            default: return _dmaPages[3];
        }
    }
private:
    int _address;
    Byte _dmaPages[4];
};
