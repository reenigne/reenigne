class NMISwitch : public ISA8BitComponent<NMISwitch>
{
public:
    static String typeName() { return "NMISwitch"; }
    NMISwitch(Component::Type type) : ISA8BitComponent(type), _connector(this)
    {
        persist("on", &_nmiOn);
        connector("", &_connector);
    }
    void write(UInt8 data) { _nmiOn = ((data & 0x80) != 0); }
    class Connector : public OutputConnector<bool>
    {
    public:
        Connector(NMISwitch* c) : OutputConnector(c), _component(c) { }
        void connect(::Connector* other)
        {
            _component->_other = 
                static_cast<BidirectionalConnector<bool>*>(other);
        }
    private:
        NMISwitch* _component;
    };

    BidirectionalConnector<bool>* _other;
private:
    bool _nmiOn;
    Connector _connector;
};
