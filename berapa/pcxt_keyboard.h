template<class T> class PCXTKeyboardT : public Component
{
public:
    static String typeName() { return "PCXTKeyboard"; }
    PCXTKeyboardT(Component::Type type)
      : Component(type), _connector(this)
    {
        connector("", &_connector);
    }

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
            _keyboard->_port = static_cast<typename
                PCXTKeyboardPortT<T>::Connector*>(other)->_port;
        }
    };

    typedef Component::TypeHelper<PCXTKeyboard> Type;
private:
    Connector _connector;
    PCXTKeyboardPort* _port;
};
