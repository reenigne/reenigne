class PCXTKeyboardPort : public Component
{
public:
    class Type : public ComponentType
    {
    public:
        Type(Simulator* simulator)
          : ComponentType(new Implementation(simulator)) { }
    private:
        class Implementation : public ComponentType::Implementation
        {
        public:
            Implementation(Simulator* simulator)
              : ComponentType::Implementation(simulator) { }
            String toString() const { return "PCXTKeyboardPort"; }
        };
    };

};