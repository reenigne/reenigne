class ISA8BitRAM : public ISA8BitComponentBase<ISA8BitRAM>
{
public:
    static String typeName() { return "ISA8BitRAM"; }
    ISA8BitRAM(Component::Type type)
      : ISA8BitComponentBase<ISA8BitRAM>(type),
        _ram(RAM::Type(this->simulator()))
    {
        this->persist("address", &_address, HexPersistenceType(5));
        this->persist("ram", &_ram, _ram.persistenceType());
        this->config("ram", &_ram, RAM::Type(this->simulator(), &_ram));
    }
    void load(const Value& v)
    {
        ISA8BitComponentBase::load(v);
        readMemoryRange(0, _ram.size());
        writeMemoryRange(0, _ram.size());
    }
    ISA8BitComponent* setAddressReadMemory(Tick tick, UInt32 address)
    {
        _address = address & 0xfffff;
        return this;
    }
    ISA8BitComponent* setAddressWriteMemory(Tick tick, UInt32 address)
    {
        _address = address & 0xfffff;
        return this;
    }
    UInt8 readMemory(Tick tick) { return _ram.read(tick, _address); }
    void writeMemory(Tick tick, UInt8 data)
    {
        _ram.write(tick, _address, data);
    }
    UInt8 debugReadMemory(UInt32 address) { return _ram.debugRead(address); }
private:
    int _address;
    RAM _ram;
};
