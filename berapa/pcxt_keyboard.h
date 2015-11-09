class PCXTKeyboard : public Component
{
public:
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

};