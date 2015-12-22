template<class T> class PCXTKeyboardT : public Component
{
public:
    static String typeName() { return "PCXTKeyboard"; }
    PCXTKeyboardT(Component::Type type) : Component(type), _connector(this)
    {
        connector("", &_connector);
    }

    class Connector : public ::Connector
    {
    public:
        Connector(PCXTKeyboard* k) : ::Connector(k), _keyboard(k) { }
        class Type : public NamedNullary<::Connector::Type, Type>
        {
        public:
            static String name() { return "PCXTKeyboard.Connector"; }
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
            _keyboard->_port = static_cast<typename
                PCXTKeyboardPortT<T>::Connector*>(other)->_port;
        }
        Component::Type defaultComponentType(Simulator* simulator)
        {
            throw Exception(_keyboard->name() + " needs to be connected");
        }
    };

    typedef Component::TypeHelper<PCXTKeyboard> Type;
private:
    Connector _connector;
    PCXTKeyboardPort* _port;
};
