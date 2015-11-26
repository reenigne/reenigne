template<class T> class ISA8BitRAMTemplate : public ISA8BitComponent<RAM>
{
public:
    static String typeName() { return "ISA8BitRAM"; }
    ISA8BitRAMTemplate()
    {
        connector("parityError", &_ram._parityError);
        config("rowBits", &_ram._rowBits);
        config("bytes", &_ram._ramSize);
        config("decayTime", &_ram._decayTime, ConcretePersistenceType(second));
        persist("address", &_address, HexPersistType(5));
        persist("ram", &_ram);
    }
    void setAddress(UInt32 address)
    {
        _address = address & 0x400fffff;
        _active = (_address < _bytes);
    }
    void read(Tick tick)
    {
        ISA8BitComponent::set(_ram.read(_address));
    }
    void write(Tick tick, UInt8 data)
    {
        _ram.write(tick, _address, data);
    }
    UInt8 memory(UInt32 address)
    {
        if (address < static_cast<UInt32>(_bytes))
            return _ram.memory(address);
        return 0xff;
    }
private:
    int _address;
    RAM _ram;
};
