class PCXTNoKeyboard : public ComponentBase<PCXTNoKeyboard>
{
public:
    static String typeName() { return "PCXTNoKeyboard"; }
    PCXTNoKeyboard(Component::Type type)
      : ComponentBase(type), _connector(this)
    {
        connector("", &_connector);
    }

    class Connector : public ConnectorBase<Connector>
    {
    public:
        Connector(PCXTNoKeyboard* c) : ConnectorBase(c) { }
        static String typeName() { return "PCXTNoKeyboard.Connector"; }
        static auto protocolDirection()
        {
            return ProtocolDirection(PCXTKeyboardProtocol(), true);
        }
    };
private:
    Connector _connector;
};

template<class T> class PCXTKeyboardPortT
  : public ComponentBase<PCXTKeyboardPort>
{
public:
    static String typeName() { return "PCXTKeyboardPort"; }
    PCXTKeyboardPortT(Component::Type type)
      : ComponentBase<PCXTKeyboardPort>(type), _clearConnector(this),
        _clockConnector(this), _connector(this), _irqConnector(this),
        _dataConnector(this)
    {
        connector("data", &_dataConnector);
        connector("clear", &_clearConnector);
        connector("clock", &_clockConnector);
        connector("irq", &_irqConnector);
        connector("", &_connector);
    }
    class Connector : public ConnectorBase<Connector>
    {
    public:
        Connector(PCXTKeyboardPort* p) : ConnectorBase<Connector>(p) { }
        static String typeName() { return "PCXTKeyboardPort.Connector"; }
        static auto protocolDirection()
        {
            return ProtocolDirection(PCXTKeyboardProtocol(), false);
        }
    protected:
        void connect(::Connector* other)
        {
            static_cast<PCXTKeyboardPort*>(component())->_keyboard =
                static_cast<PCXTKeyboard::Connector*>(other)->_keyboard;
        }
    };
    class ClearConnector : public InputConnector<bool>
    {
    public:
        ClearConnector(PCXTKeyboardPort* p) : InputConnector<bool>(p) { }
        void setData(Tick t, bool v)
        {
            static_cast<PCXTKeyboardPort*>(component())->setClear(t, v);
        }
    };
    class ClockConnector : public InputConnector<bool>
    {
    public:
        ClockConnector(PCXTKeyboardPort* p) : InputConnector<bool>(p) { }
        void setData(Tick t, bool v)
        {
            static_cast<PCXTKeyboardPort*>(component())->setClock(t, v);
        }
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
