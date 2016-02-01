class RGBIProtocol : public ProtocolBase<RGBIProtocol> { };

class NoRGBIMonitor : public ComponentBase<NoRGBIMonitor>
{
public:
    static String typeName() { return "NoRGBIMonitor"; }
    NoRGBIMonitor(Component::Type type) : ComponentBase(type), _connector(this)
    {
        connector("", &_connector);
    }

    class Connector : public ConnectorBase<Connector>
    {
    public:
        Connector(NoRGBIMonitor* c) : ConnectorBase(c) { }
        static String typeName() { return "NoRGBIMonitor.Connector"; }
        static auto protocolDirection()
        {
            return ProtocolDirection(RGBIProtocol(), false);
        }
    };
private:
    Connector _connector;
};

class IBMCGA : public ISA8BitComponentBase<IBMCGA>
{
public:
    static String typeName() { return "IBMCGA"; }
    IBMCGA(Component::Type type)
      : ISA8BitComponentBase<IBMCGA>(type), _attr(0), _chrdata(0),
        _wait(0), _cycle(0), _bgri(0), _lightPenStrobe(false),
        _lightPenSwitch(true), _bgriSource(this),
        _ram(RAM::Type(this->simulator())), _rgbiConnector(this)
    {
        this->config("rom", &_rom);
        this->persist("memoryAddress", &_memoryAddress, HexPersistenceType(4));
        this->persist("ioAddress", &_ioAddress, HexPersistenceType(4));
        this->persist("mode", &_mode);
        this->persist("palette", &_palette);
        this->persist("ram", &_ram, _ram.persistenceType());
        this->config("ram", &_ram, RAM::Type(this->simulator(), &_ram));
        this->connector("rgbiOutput", &_rgbiConnector);
    }
    void load(const Value& v)
    {
        ISA8BitComponentBase<IBMCGA>::load(v);
        String data = File(_rom, simulator()->directory()).contents();
        int length = 0x2000;
        _romdata.allocate(length);
        if (data.length() < length) {
            throw Exception(_rom + " is too short: " +
                decimal(data.length()) + " bytes found, " + decimal(length) +
                " bytes required");
        }
        for (int i = 0; i < length; ++i)
            _romdata[i] = data[i];
        _data = _ram.data();
        readMemoryRange(0xb8000, 0xc0000);
        writeMemoryRange(0xb8000, 0xc0000);
        readIORange(0x3d0, 0x3e0);
        writeIORange(0x3d0, 0x3e0);
    }
    void simulateCycle()
    {
        _cycle = (_cycle + 1) & 15;
        if (_cycle == 0) {
            _crtc.simulateCycle();

            int ma = (_crtc.memoryAddress() & 0x1fff) << 1;
            UInt8 ch = _data[ma];
            _attr = _data[ma + 1];
            _chrdata = _romdata[0x1800 + (ch << 3) + (_crtc.rowAddress() & 7)];
        }
        if ((_mode & 2) != 0) {
        }
        else {
            UInt8 tmp = _chrdata & (0x80 >> (_cycle >> 1));
            if(tmp) {
                _bgri = 0x0F;
            }
            else {
                _bgri = 0;
            }
            this->_bgriSource.produce(1);
        }
    }
    ISA8BitComponent* setAddressReadMemory(Tick tick, UInt32 address)
    {
        _memoryAddress = address & 0x00003fff;
        return this;
    }
    ISA8BitComponent* setAddressWriteMemory(Tick tick, UInt32 address)
    {
        _memoryAddress = address & 0x00003fff;
        return this;
    }
    ISA8BitComponent* setAddressReadIO(Tick tick, UInt32 address)
    {
        _ioAddress = address & 0x0000000f;
        return this;
    }
    ISA8BitComponent* setAddressWriteIO(Tick tick, UInt32 address)
    {
        _ioAddress = address & 0x0000000f;
        return this;
    }
    UInt8 readMemory(Tick tick)
    {
        _wait = 8 + (16 - _cycle);
        return _data[_memoryAddress];
    }
    void writeMemory(Tick tick, UInt8 data)
    {
        _wait = 8 + (16 - _cycle);
        _data[_memoryAddress] = data;
    }
    UInt8 readIO(Tick tick)
    {
        if ((_ioAddress & 8) == 0)
            return _crtc.read((_ioAddress & 1) != 0);
        switch (_ioAddress & 7) {
            case 2:
                return (_crtc.displayEnable() ? 0 : 1) |
                    (_lightPenStrobe ? 2 : 0) |
                    (_lightPenSwitch ? 4 : 0) |
                    (_crtc.verticalSync() ? 8 : 0) | 0xf0;
            case 3:
                _lightPenStrobe = false;
                break;
            case 4:
                activateLightPen();
                break;
        }
        return 0xff;
    }
    void writeIO(Tick tick, UInt8 data)
    {
        if ((_ioAddress & 8) == 0) {
            _crtc.write((_ioAddress & 1) != 0, data);
            return;
        }
        switch (_ioAddress & 7) {
            case 0:
                _mode = data;
                break;
            case 1:
                _palette = data;
                break;
            case 3:
                _lightPenStrobe = false;
                break;
            case 4:
                activateLightPen();
                break;
        }
    }
    UInt8 debugReadMemory(UInt32 address) { return _data[address & 0x3fff]; }
    class BGRISource : public Source<BGRI>
    {
    public:
        BGRISource(IBMCGA* cga) : _cga(cga)
        {
        }
        void produce(int n)
        {
            Accessor<BGRI> acc = writer(n);
            acc.item() = this->_cga->_bgri |
                (this->_cga->_crtc.horizontalSync() ? 0x10 : 0) |
                (this->_cga->_crtc.verticalSync() ? 0x20 : 0);
            written(1);
        }
    private:
        IBMCGA* _cga;
    };
    class CompositeSource : public Source<UInt8> { void produce(int n) { } };
    BGRISource* bgriSource() { return &_bgriSource; }
    CompositeSource* compositeSource() { return &_compositeSource; }

    class RGBIConnector : public ConnectorBase<RGBIConnector>
    {
    public:
        RGBIConnector(IBMCGA* cga) : ConnectorBase(cga), _cga(cga) { }
        void connect(::Connector* other)
        {
            // TODO
        }
        static String typeName() { return "IBMCGA.RGBIConnector"; }
        static auto protocolDirection()
        {
            return ProtocolDirection(RGBIProtocol(), true);
        }
    private:
        IBMCGA* _cga;
    };

private:
    void activateLightPen()
    {
        if (!_lightPenStrobe)
            _crtc.activateLightPen();
        _lightPenStrobe = true;
    }

    UInt8* _data;
    RGBIConnector _rgbiConnector;
    String _rom;
    Array<UInt8> _romdata;
    UInt8 _attr;
    UInt8 _chrdata;
    int _memoryAddress;
    int _ioAddress;
    int _wait;
    int _cycle;
    UInt8 _mode;
    UInt8 _palette;
    UInt8 _bgri;
    Motorola6845CRTC _crtc;
    bool _lightPenStrobe;
    bool _lightPenSwitch;
    BGRISource _bgriSource;
    CompositeSource _compositeSource;
    RAM _ram;
};
