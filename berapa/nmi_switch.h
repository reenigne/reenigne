class NMISwitch : public ISA8BitComponent<NMISwitch>
{
public:
    void setAddress(UInt32 address)
    {
        _active = (address & 0xc00003e0) == 0xc00000a0;
    }
    void write(UInt8 data) { _nmiOn = ((data & 0x80) != 0); }
    String save() const
    {
        return String("{ on: ") + String::Boolean(_nmiOn) + ", active: " +
            String::Boolean(_active) + " }\n";
    }
    ::Type persistenceType() const
    {
        List<StructuredType::Member> members;
        members.add(StructuredType::Member("on", false));
        members.add(StructuredType::Member("active", false));
        return StructuredType("NMISwitch", members);
    }
    void load(const Value& value)
    {
        auto members = value.value<HashTable<Identifier, Value>>();
        _nmiOn = members["on"].value<bool>();
        _active = members["active"].value<bool>();
    }
    bool nmiOn() const { return _nmiOn; }

    class ValueConnector : public ::Connector
    {
    public:
        ValueConnector(NMISwitch* component) : _component(component) { }
        void connect(::Connector* other)
        {
            // TODO
        }
        ::Connector::Type type() const { return BitOutputConnector::Type(); }
    private:
        NMISwitch* _component;
    };
    static String name() { return "NMISwitch"; }

private:
    bool _nmiOn;
};
