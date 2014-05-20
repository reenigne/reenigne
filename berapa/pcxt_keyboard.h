class PCXTKeyboard : public Component
{
public:
    class Type : public Component::Type
    {
    public:
        Type(Simulator* simulator)
          : Component::Type(new Implementation(simulator)) { }
    private:
        class Implementation : public Component::Type::Implementation
        {
        public:
            Implementation(Simulator* simulator)
              : Component::Type::Implementation(simulator) { }
            String toString() const { return "PCXTKeyboard"; }
        };
    };

};