template<class T> class PCXTKeyboardPortTemplate : public Component
{
public:
    static String typeName() { return "PCXTKeyboardPort"; }
    PCXTKeyboardPortTemplate(Component::Type type)
      : Component(type), _clearConnector(this), _clockConnector(this),
        _connector(this)
    {
        connector("data", &_dataConnector);
        connector("clear", &_clearConnector);
        connector("clock", &_clockConnector);
        connector("irq", &_irqConnector);
        connector("", &_connector);
    }
    class Connector : public ::Connector
    {
    public:
        Connector(PCXTKeyboardPort* port) : _port(port) { }
        class Type : public NamedNullary<::Connector::Type, Type>
        {
        public:
            static String name() { return "PCXTKeyboardPort.Connector"; }
            class Body : public NamedNullary<::Connector::Type, Type>::Body
            {
            public:
                bool compatible(::Connector::Type other) const
                {
                    return other == PCXTKeyboard::Connector::Type();
                }
            };
        };
        PCXTKeyboardPort* _port;
    protected:
        ::Connector::Type type() const { return Type(); }
        void connect(::Connector* other)
        {
            _port->_keyboard =
                static_cast<PCXTKeyboard::Connector*>(other)->_keyboard;
        }
    };

    typedef Component::TypeHelper<PCXTKeyboardPort> Type;

    class ClearConnector : public InputConnector<bool>
    {
    public:
        ClearConnector(PCXTKeyboardPort* port) : _port(port) { }
        void setData(Tick t, bool v) { _port->setClear(t, v); }
    private:
        PCXTKeyboardPort* _port;
    };
    class ClockConnector : public InputConnector<bool>
    {
    public:
        ClockConnector(PCXTKeyboardPort* port) : _port(port) { }
        void setData(Tick t, bool v) { _port->setClock(t, v); }
    private:
        PCXTKeyboardPort* _port;
    };

    void setClear(Tick t, bool clear)
    {
    }
    void setClock(Tick t, bool clock)
    {
    }
private:
    OutputConnector<Byte> _dataConnector;
    ClearConnector _clearConnector;
    ClockConnector _clockConnector;
    OutputConnector<bool> _irqConnector;
    Connector _connector;
    PCXTKeyboard* _keyboard;
};