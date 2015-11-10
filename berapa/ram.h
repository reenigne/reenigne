template<class T> class RAMTemplate : public ISA8BitComponent
{
public:
    void initialize()
    {
        _rowBits = getValue("rowBits").value<int>();
        _bytes = getValue("bytes").value<int>();
        _decayTime = (getValue("decayTime").value<Concrete>()/second).value();
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
    void setAddress(UInt32 address)
    {
        _address = address & 0x400fffff;
        _active = (_address < 0xa0000);
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
    void load(const Value& value)
    {
        auto members = value.value<HashTable<Identifier, Value>>();
        _dram.load(members["dram"]);
        _active = members["active"].value<bool>();
        _tick = members["tick"].value<int>();
        _address = members["address"].value<int>();
    }
    String save() const
    {
        return String("{ ") + _dram.name() +
            ": " + _dram.save(_tick) +
            ",\n  active: " + String::Boolean(this->_active) +
            ", tick: " + _tick + ", address: " + hex(_address, 5) + " }\n";
    }

    class Type : public ISA8BitComponent::Type
    {
    public:
        Type(Simulator* simulator)
          : ISA8BitComponent::Type(new Body(simulator)) { }
    private:
        class Body : public ISA8BitComponent::Type::Body
        {
        public:
            Body(Simulator* simulator)
              : ISA8BitComponent::Type::Body(simulator) { }
            String toString() const { return "RAM"; }
            ::Type member(Identifier name) const
            {
                if (name.name() == "rowBits" || name.name() == "bytes")
                    return IntegerType();
                if (name.name() == "decayTime")
                    return second.type();
                return ISA8BitComponent::Type::Body::member(name);
            }
            Reference<Component> createComponent() const
            {
                return Reference<Component>::create<RAM>();
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
    Rational _decayTime;
};
