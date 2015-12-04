template<class T> class ISA8BitRAMTemplate
  : public ISA8BitComponent<ISA8BitRAMTemplate<T>>
{
public:
    static String typeName() { return "ISA8BitRAM"; }
    ISA8BitRAMTemplate(Component::Type type)
      : ISA8BitComponent(type), _ram(type)
    {
        connector("parityError", &_ram._parityError);
        config("rowBits", &_ram._rowBits);
        config("bytes", &_ram._ramSize);
        config("decayTime", &_ram._decayTime, ConcretePersistenceType(second));
        config("decayValue", &_ram._decayValue);
        persist("address", &_address, HexPersistenceType(5));
        persist("ram", &_ram, _ram.persistenceType());
    }
    void setAddress(UInt32 address)
    {
        _address = address & 0x400fffff;
        _active = (_address < _ram._ramSize);
    }
    void read(Tick tick) { ISA8BitComponent::set(_ram.read(tick, _address)); }
    void write(Tick tick, UInt8 data) { _ram.write(tick, _address, data); }
    UInt8 debugRead(UInt32 address) { return _ram.debugRead(address); }
private:
    int _address;
    RAM _ram;
};
