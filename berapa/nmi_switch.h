class NMISwitch : public ISA8BitComponent<NMISwitch>
{
public:
    static String typeName() { return "NMISwitch"; }
    NMISwitch() : _connector(this)
    {
        persist("on", &_nmiOn, false, BooleanType());
        connector("", &_connector);
    }
    void write(UInt8 data) { _nmiOn = ((data & 0x80) != 0); }
    class Connector : public OutputConnector<bool>
    {
    public:
        Connector(NMISwitch* component) : _component(component) { }
        //void connect(::Connector* other)
        //{
        //    // TODO
        //}
    private:
        NMISwitch* _component;
    };

private:
    bool _nmiOn;
    Connector _connector;
};
