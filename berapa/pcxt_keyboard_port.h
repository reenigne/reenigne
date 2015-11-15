template<class T> class PCXTKeyboardPortTemplate : public Component
{
public:
    PCXTKeyboardPortTemplate()
      : _clearConnector(this), _clockConnector(this), _connector(this) { }
    Value getValue(Identifier i) const
    {
        if (i.name() == "data")
            return _dataConnector.getValue();
        if (i.name() == "clear")
            return _clearConnector.getValue();
        if (i.name() == "clock")
            return _clockConnector.getValue();
        if (i.name() == "irq")
            return _irqConnector.getValue();
        if (i.name() == "plug")
            return _connector.getValue();
        return Component::getValue(i);
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

    class Type : public Component::Type
    {
    public:
        Type(Simulator* simulator) : Component::Type(new Body(simulator)) { }
    private:
        class Body : public Component::Type::Body
        {
        public:
            Body(Simulator* simulator) : Component::Type::Body(simulator) { }
            String toString() const { return "PCXTKeyboardPort"; }
            ::Type member(Identifier i) const
            {
                String n = i.name();
                if (n == "data")
                    return OutputConnector<Byte>::Type();
                if (n == "clear" || n == "clock")
                    return InputConnector<bool>::Type();
                if (n == "irq")
                    return OutputConnector<bool>::Type();
                if (n == "plug")
                    return Connector::Type();
                return Component::Type::Body::member(i);
            }
            Reference<Component> createComponent() const
            {
                return Reference<Component>::create<PCXTKeyboardPort>();
            }
        };
    };

    class ClearConnector : public InputConnector<bool>
    {
    public:
        ClearConnector(PCXTKeyboardPort* port) : _port(port) { }
        void setData(bool t) { _port->setClear(t); }
    private:
        PCXTKeyboardPort* _port;
    };
    class ClockConnector : public InputConnector<bool>
    {
    public:
        ClockConnector(PCXTKeyboardPort* port) : _port(port) { }
        void setData(bool t) { _port->setClock(t); }
    private:
        PCXTKeyboardPort* _port;
    };

    void setClear(bool clear)
    {
    }
    void setClock(bool clock)
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