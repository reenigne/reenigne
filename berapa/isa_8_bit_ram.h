template<class T> class ISA8BitRAMTemplate : public ISA8BitComponent<RAM>
{
public:
    static String typeName() { return "ISA8BitRAM"; }
    ISA8BitRAMTemplate()
    {
        config("rowBits", &_rowBits);
        config("bytes", &_bytes);
        config("decayTime", &_decayTime, ConcretePersistenceType(second));
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
    void load(const Value& value)
    {
        ISA8BitComponent::load(value);
        if (_decayTime == 0) {
            // DRAM decay time in cycles.
            // This is the fastest that DRAM could decay and real hardware
            // would still work.
            //decayTime = (18*4) << rowBits;
            // 2ms for 4116 and 2118
            // 4ms for 4164
            // 8ms for 41256
            _decayTime =  (13125 << _rowBits) / 176;
        }
        _ram.initialize(_bytes, _rowBits, _decayTime, 0);
    }
private:
    int _address;
    RAM _ram;
    int _rowBits;
    int _bytes;
    Rational _decayTime;
};
