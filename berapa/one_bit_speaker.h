class OneBitSpeaker : public ComponentBase<OneBitSpeaker>
{
public:
    static String typeName() { return "OneBitSpeaker"; }
    OneBitSpeaker(Component::Type type) : ComponentBase(type), _connector(this)
    {
        connector("", &_connector);
    }
    void setData(Tick tick, bool v)
    {
        // TODO
    }
    class Connector : public InputConnector<bool>
    {
    public:
        Connector(OneBitSpeaker* c) : InputConnector(c) { }
        static String typeName() { return "OneBitSpeaker.Connector"; }
        void setData(Tick tick, bool v)
        {
            static_cast<OneBitSpeaker*>(component())->setData(tick, v);
        }
    };
private:
    Connector _connector;
};

