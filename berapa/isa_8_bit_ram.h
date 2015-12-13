template<class T> class ISA8BitRAMT
  : public ISA8BitComponent<ISA8BitRAMT<T>>
{
public:
    static String typeName() { return "ISA8BitRAM"; }
    ISA8BitRAMT(Component::Type type)
      : ISA8BitComponent<ISA8BitRAMT<T>>(type), _ram(RAM::Type(_simulator))
    {
        this->persist("address", &_address, HexPersistenceType(5));
        this->persist("ram", &_ram, _ram.persistenceType());
        config("ram", &_ram, RAM::Type(_simulator));
    }
    void setAddress(UInt32 address)
    {
        _address = address & 0x400fffff;
        this->_active = (_address < _ram.size());
    }
    void read(Tick tick)
    {
        ISA8BitComponent<ISA8BitRAMT<T>>::set(
            _ram.read(tick, _address));
    }
    void write(Tick tick, UInt8 data) { _ram.write(tick, _address, data); }
    UInt8 debugRead(UInt32 address) { return _ram.debugRead(address); }
private:
    int _address;
    RAM _ram;
};
