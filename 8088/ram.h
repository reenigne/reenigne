template<class T> class RAMTemplate : public ISA8BitComponent
{
public:
    RAMTemplate() : _rowBits(9), _bytes(0xa0000), _decayTime(0) { }
    //void site()
    //{
    //    ConfigFile* config = this->_simulator->config();

    //    // _rowBits is 7 for 4116 RAM chips
    //    //             8 for 4164
    //    //             9 for 41256
    //    // We use 9 here because programs written for _rowBits == N will work
    //    // for _rowBits < N but not necessarily _rowBits > N.
    //    config->addDefaultOption("ramRowBits", IntegerType(), 9);

    //    // 640KB should be enough for anyone.
    //    config->addDefaultOption("ramBytes", IntegerType(), 0xa0000);

    //    config->addDefaultOption("decayTime", IntegerType(), 0);
    //}
    void initialize()
    {
        ConfigFile* config = this->_simulator->config();
        //int rowBits = config->template get<int>("ramRowBits");
        //int bytes = config->template get<int>("ramBytes");
        //int decayTime = config->template get<int>("decayTime");
        if (_decayTime == 0) {
            // DRAM decay time in cycles.
            // This is the fastest that DRAM could decay and real hardware
            // would still work.
            //decayTime = (18*4) << rowBits;
            // 2ms for 4116 and 2118
            // 4ms for 4164
            // 8ms for 41256
            _decayTime =  (13125 << rowBits) / 176;
        }
        _dram.initialize(_bytes, _rowBits, _decayTime, 0);
    }
    Rational<int> hDotsPerCycle() const { return 3; }
    void simulateCycle() { _dram.simulateCycle(); }
    void setAddress(UInt32 address)
    {
        _address = address & 0x400fffff;
        _active = (_address < 0xa0000);
    }
    void read()
    {
        if (_dram.decayed(_address)) {
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
    void write(UInt8 data) { _dram.write(_address, data); }
    UInt8 memory(UInt32 address)
    {
        if (address < 0xa0000)
            return _dram.memory(address);
        return 0xff;
    }
    ::Type persistenceType() const
    {
        List<StructuredType::Member> members;
        members.add(StructuredType::Member("dram", _dram.initial()));
        members.add(StructuredType::Member("active", false));
        members.add(StructuredType::Member("tick", 0));
        members.add(StructuredType::Member("address", 0));
        return StructuredType("RAM", members);
    }
    void load(const TypedValue& value)
    {
        auto members = value.value<Value<HashTable<String, TypedValue>>>();
        _dram.load((*members)["dram"]);
        _active = (*members)["active"].value<bool>();
        _tick = (*members)["tick"].value<int>();
        _address = (*members)["address"].value<int>();
    }
    String save() const
    {
        return String("{ ") + _dram.name() +
            ": " + _dram.save() +
            ",\n  active: " + String::Boolean(this->_active) +
            ", tick: " + String::Decimal(this->_tick) +
            ", address: " + hex(_address, 5) + " }\n";
    }
    void set(String name, TypedValue value)
    {
        if (name == "rowBits") {
            _rowBits = value.value<int>();
            return;
        }
        if (name == "bytes") {
            _bytes = value.value<int>();
            return;
        }
        if (name == "decayTime") {
            _decayTime = value.value<Rational<int>>();
            return;
        }
        ISA8BitComponent::set(name, value);
    }

    class Type : public ISA8BitComponent::Type
    {
    public:
        Type(Simulator* simulator)
          : ISA8BitComponent::Type(new Implementation(simulator)) { }
    private:
        class Implementation : public ISA8BitComponent::Type::Implementation
        {
        public:
            Implementation(Simulator* simulator)
              : ISA8BitComponent::Type::Implementation(simulator) { }
            String toString() const { return "RAM"; }
            bool has(String name) const
            {
                if (name == "rowBits" || name == "bytes" ||
                    name == "decayTime")
                    return true;
                return ISA8BitComponent::Type::Implementation::has(name);
            }
        };
    };
private:
    int _address;
    DRAM _dram;
    NMISwitch* _nmiSwitch;
    Intel8255PPI* _ppi;
    Intel8088Template<T>* _cpu;
    int _rowBits;
    int _bytes;
    Rational<int> _decayTime;
};
