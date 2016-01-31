template<class T> class NoRGBISourceT;
typedef NoRGBISourceT<void> NoRGBISource;

template<class T> class NoRGBISourceT : public ComponentBase<NoRGBISource>
{
public:
    static String typeName() { return "NoRGBISource"; }
    NoRGBISourceT(Component::Type type) : Component(type), _connector(this)
    {
        connector("", &_connector);
    }

    class Connector : public ConnectorBase<Connector>
    {
    public:
        Connector(NoRGBISourceT* c) : ConnectorBase<Connector>(c) { }
        static String typeName() { return "NoRGBISource.Connector"; }
        static auto protocolDirection()
        {
            return ProtocolDirection(RGBIProtocol, true);
        }
    };
private:
    Connector _connector;
};

template<class T> class RGBIMonitorT : public ComponentBase<RGBIMonitor>
{
public:
    static String typeName() { return "RGBIMonitor"; }
    RGBIMonitorT(Component::Type type) : ComponentBase(type), _connector(this)
    {
        _palette.allocate(64);
        _palette[0x0] = 0xff000000;
        _palette[0x1] = 0xff0000aa;
        _palette[0x2] = 0xff00aa00;
        _palette[0x3] = 0xff00aaaa;
        _palette[0x4] = 0xffaa0000;
        _palette[0x5] = 0xffaa00aa;
        _palette[0x6] = 0xffaa5500;
        _palette[0x7] = 0xffaaaaaa;
        _palette[0x8] = 0xff555555;
        _palette[0x9] = 0xff5555ff;
        _palette[0xa] = 0xff55ff55;
        _palette[0xb] = 0xff55ffff;
        _palette[0xc] = 0xffff5555;
        _palette[0xd] = 0xffff55ff;
        _palette[0xe] = 0xffffff55;
        _palette[0xf] = 0xffffffff;

        // Create some special colours for visualizing sync pulses.
        for (int i = 0; i < 16; ++i) {
            int r = ((_palette[i] >> 16) & 0xff) >> 4;
            int g = ((_palette[i] >> 8) & 0xff) >> 4;
            int b = (_palette[i] & 0xff) >> 4;
            int rgb = (r << 16) + (g << 8) + b;
            _palette[i + 16] = 0xff002200 + rgb; // hsync
            _palette[i + 32] = 0xff220022 + rgb; // vsync
            _palette[i + 48] = 0xff222222 + rgb; // hsync+vsync
        }
        connector("", &_connector);
    }
    void load(const Value& v)
    {
        Component::load(v);
        // Defer creating the window until load time to avoid creating windows
        // during type building.
        _window = Reference<Window>::template create<Window>();
    }

    class Connector : public ConnectorBase<Connector>
    {
    public:
        Connector(RGBIMonitor* monitor)
          : ConnectorBase(monitor), _monitor(monitor) { }
        void connect(::Connector* other)
        {
            // TODO
        }
        static String typeName() { return "RGBIMonitor.Connector"; }
        static auto protocolDirection()
        {
            return ProtocolDirection(RGBIProtocol(), false);
        }
    private:
        RGBIMonitor* _monitor;
    };

    class BGRISink : public Sink<BGRI>
    {
    public:
        BGRISink(RGBIMonitor* monitor) : _monitor(monitor) { }
        // We ignore the suggested number of samples and just read a whole
        // frame's worth once there is enough for a frame.
        void consume(int nSuggested)
        {
            // Since the pumping is currently done by Simulator::simulate(),
            // don't try to pull more data from the CGA than we have.
            if (remaining() < 912*262 + 1)
                return;

            // We have enough data for a frame - update the screen.
            read(_monitor->consume(Sink::reader(912*262 + 1)));
        }

    private:
        RGBIMonitor* _monitor;
    };

    int consume(Accessor<BGRI> reader)
    {
        SDLTextureLock _lock(&_window->_texture);
        int y = 0;
        int x = 0;
        bool hSync = false;
        bool vSync = false;
        bool oldHSync = false;
        bool oldVSync = false;
        int n = 0;
        UInt8* row = reinterpret_cast<UInt8*>(_lock._pixels);
        int pitch = _lock._pitch;
        UInt32* output = reinterpret_cast<UInt32*>(row);
        do {
            BGRI p = reader.item();
            hSync = ((p & 0x10) != 0);
            vSync = ((p & 0x20) != 0);
            if (x == 912 || (oldHSync && !hSync)) {
                x = 0;
                ++y;
                row += pitch;
                output = reinterpret_cast<UInt32*>(row);
            }
            if (y == 262 || (oldVSync && !vSync))
                break;
            oldHSync = hSync;
            oldVSync = vSync;
            *output = _palette[p];
            ++output;
            ++n;
            ++x;
            reader.advance(1);
        } while (true);
        _window->_renderer.renderTexture(&_window->_texture);
        return n;
    }
private:
    class Window
    {
    public:
        Window() : _renderer(&_window), _texture(&_renderer) { }
        SDLWindow _window;
        SDLRenderer _renderer;
        SDLTexture _texture;
    };
    Reference<Window> _window;
    Array<UInt32> _palette;

    Connector _connector;
};

