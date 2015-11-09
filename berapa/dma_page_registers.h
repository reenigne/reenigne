template<class T> class DMAPageRegistersTemplate :
    public ISA8BitComponentTemplate<T>
{
public:
    DMAPageRegistersTemplate()
    {
        for (int i = 0; i < 4; ++i)
            _dmaPages[i] = 0;
    }
    void setAddress(UInt32 address)
    {
        _address = address & 3;
        this->_active = (address & 0xc00003e0) == 0xc0000080;
    }
    void write(UInt8 data) { _dmaPages[_address] = data & 0x0f; }
    String save() const
    {
        String s = "{ data: {";
        bool needComma = false;
        for (int i = 0; i < 4; ++i) {
            if (needComma)
                s += ", ";
            needComma = true;
            s += hex(_dmaPages[i], 1);
        }
        return s + "}, active: " + String::Boolean(this->_active) +
            ", address: " + _address +
            " }\n";
    }
    ::Type persistenceType() const
    {
        List<StructuredType::Member> members;
        members.add(StructuredType::Member("data",
            Value(SequenceType(IntegerType()), List<Value>())));
        members.add(StructuredType::Member("active", false));
        members.add(StructuredType::Member("address", 0));
        return StructuredType("DMAPages", members);
    }
    void load(const Value& value)
    {
        auto members = value.value<HashTable<Identifier, Value>>();
        auto dmaPages = members["data"].value<List<Value>>();
        int j = 0;
        for (auto i : dmaPages) {
            _dmaPages[j] = i.value<int>();
            ++j;
            if (j == 4)
                break;
        }
        for (;j < 4; ++j)
            _dmaPages[j] = 0;
        this->_active = members["active"].value<bool>();
        _address = members["address"].value<int>();
    }

    UInt8 pageForChannel(int channel)
    {
        switch (channel) {
            case 2: return _dmaPages[1];
            case 3: return _dmaPages[2];
            default: return _dmaPages[3];
        }
    }

    class Type : public Component::Type
    {
    public:
        Type(Simulator* simulator) : Component::Type(new Body(simulator)) { }
    private:
        class Body : public Component::Type::Body
        {
        public:
            Body(Simulator* simulator) : Component::Type::Body(simulator) { }
            String toString() const { return "DMAPageRegisters"; }
            Reference<Component> createComponent() const
            {
                return Reference<Component>::create<DMAPageRegisters>();
            }
        };
    };
private:
    int _address;
    int _dmaPages[4];
};
