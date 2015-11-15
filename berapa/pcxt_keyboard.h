template<class T> class PCXTKeyboardTemplate : public Component
{
public:
    PCXTKeyboardTemplate() : _connector(this) { }

    class Connector : public ::Connector
    {
    public:
        Connector(PCXTKeyboard* keyboard) : _keyboard(keyboard) { }
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
        PCXTKeyboard* _keyboard;
    protected:
        ::Connector::Type type() const { return Type(); }
        void connect(::Connector* other)
        {
            _keyboard->_port =
                static_cast<PCXTKeyboardPort::Connector*>(other)->_port;
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
            String toString() const { return "PCXTKeyboard"; }
            Reference<Component> createComponent() const
            {
                return Reference<Component>::create<PCXTKeyboard>();
            }
        };
    };
private:
    Connector _connector;
    PCXTKeyboardPort* _port;
};