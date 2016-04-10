class NMISwitch : public ISA8BitComponentBase<NMISwitch>
{
public:
    static String typeName() { return "NMISwitch"; }
    NMISwitch(Component::Type type)
      : ISA8BitComponentBase(type), _connector(this)
    {
        persist("on", &_nmiOn);
        connector("", &_connector);
    }
    void writeIO(Tick tick, UInt8 data) { _nmiOn = ((data & 0x80) != 0); }
    class Connector : public OutputConnector<bool>
    {
    public:
        Connector(NMISwitch* c) : OutputConnector(c) { }
        void connect(::Connector* other)
        {
            static_cast<NMISwitch*>(component())->_other =
                static_cast<BidirectionalConnector<bool>*>(other);
        }
    };

    BidirectionalConnector<bool>* _other;
private:
    bool _nmiOn;
    Connector _connector;
};
