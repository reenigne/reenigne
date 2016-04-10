class PCXTKeyboardProtocol : public ProtocolBase<PCXTKeyboardProtocol> { };

template<class T> class PCXTKeyboardT : public ComponentBase<PCXTKeyboard>
{
public:
    static String typeName() { return "PCXTKeyboard"; }
    PCXTKeyboardT(Component::Type type)
      : ComponentBase<PCXTKeyboard>(type), _connector(this)
    {
        connector("", &_connector);
    }

    class Connector : public ConnectorBase<Connector>
    {
    public:
        Connector(PCXTKeyboard* k) : ConnectorBase<Connector>(k) { }
        static String typeName() { return "PCXTKeyboard.Connector"; }
        static auto protocolDirection()
        {
            return ProtocolDirection(PCXTKeyboardProtocol(), true);
        }
    protected:
        void connect(::Connector* other)
        {
            static_cast<PCXTKeyboard*>(component())->_port =
                static_cast<PCXTKeyboardPort*>(other->component());
        }
    };
private:
    Connector _connector;
    PCXTKeyboardPort* _port;
};
