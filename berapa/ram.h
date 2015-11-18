template<class T> class RAMTemplate : public ISA8BitComponent<RAM>
{
public:
    static String typeName() { return "RAM"; }
    RAMTemplate()
    {
        config("rowBits", &_rowBits);
        config("bytes", &_bytes);
        config("decayTime", &_decayTime, ConcretePersistenceType(second));
        persist("address", &_address, HexPersistType(5));
        persist(_dram.name(), &_dram);
    }
    void setAddress(UInt32 address)
    {
        _address = address & 0x400fffff;
        _active = (_address < _bytes);
    }
    void read(Tick tick)
    {
        if (_dram.decayed(tick, _address)) {
            // RAM has decayed! On a real machine this would not always signal
            // an NMI but we'll make the NMI happen every time to make DRAM
            // decay problems easier to find.

            // TODO: In the config file we might want to have an option for
            // "realistic mode" to prevent this from being used to detect the
            // emulator.
            if (_nmiSwitch->nmiOn() && (_ppi->portB() & 0x10) != 0)
                _cpu->nmi();
        }
        ISA8BitComponent::set(_dram.read(_address));
    }
    void write(Tick tick, UInt8 data)
    {
        _dram.write(tick, _address, data);
    }
    UInt8 memory(UInt32 address)
    {
        if (address < static_cast<UInt32>(_bytes))
            return _dram.memory(address);
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
        _dram.initialize(_bytes, _rowBits, _decayTime, 0);
    }
private:
    int _address;
    DRAM _dram;
    NMISwitch* _nmiSwitch;
    Intel8255PPI* _ppi;
    Intel8088CPUTemplate<T>* _cpu;
    int _rowBits;
    int _bytes;
    Rational _decayTime;
};
