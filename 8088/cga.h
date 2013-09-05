class IBMCGA : public ISA8BitComponent, public Source<BGRI>
{
public:
    IBMCGA()
    {
        _data.allocate(0x4000);
    }
    void simulateCycle()
    {
        ++_cycle;
        if(_cycle == 16)
        {
            _cycle = 0;
        }
        if(_cycle == 0 || (_cycle == 8 && (_mode & 1) != 0))
        {
            _crtc.simulateCycle();
            _crtc.ma &= 0x1fff;
            _crtc.ra &= 0x7;
            _chr = _data[_crtc.ma << 1];
            _attr = _data[(_crtc.ma << 1) + 1];
            _chrdata = _romdata[(0x1800 | _crtc.ra) + (_chr << 3)];
            _status = !_crtc._displayenable | ((_crtc._ycounter > _crtc._crtcdata[6]) ? 8 : 0);
        }
        if(_wait != 0)
        {
            _wait--;
        }
        if(_mode & 2)
        {
        }
        else
        {
            UInt8 tmp = _chrdata & (1 << (_cycle & 7));
            if(tmp)
            {
                _r = 1;
                _g = 1;
                _b = 1;
                _i = 1;
            }
            else
            {
                _r = 0;
                _g = 0;
                _b = 0;
                _i = 0;
            }
            produce(1);
        }
    }
    void produce(int n)
    {
        Accessor<BGRI> acc = writer(n);
        acc.item() = _b | (_g << 1) | (_r << 2) | (_i << 3) | ((_crtc._xcounter > _crtc._crtcdata[1]) ? 0x10 : 0) | ((_crtc._ycounter > _crtc._crtcdata[6]) ? 0x20 : 0);
        written(1);
    }
    void setAddress(UInt32 address)
    {
        _memoryActive = ((address & 0x400f8000) == 0xb8000);
        _memoryAddress = address & 0x00003fff;
        _portActive = ((address & 0x400003f0) == 0x400003d0);
        _portAddress = address & 0x0000000f;
        _active = (_memoryActive || _portActive);
    }
    void read()
    {
        if (_memoryActive && _wait == 0)
        {
            _wait = 8 + (16 - _cycle);
            set(_data[_memoryAddress]);
        }
        if(_portActive)
        {
            switch(_portAddress)
            {
            case 0:
            case 2:
            case 4:
            case 6:
                set(_crtcindex);
                break;
            case 1:
            case 3:
            case 5:
            case 7:
                set(_crtc._crtcdata[_crtcindex]);
                break; 
            case 8:
                set(_mode);
                break;
            case 9:
                set(_colsel);
                break;
            case 0xa:
                set(_status);
                break;
            }
        }
    }
    void write(UInt8 data)
    {
        if (_memoryActive && _wait == 0)
        {
            _wait = 8 + (16 - _cycle);
            _data[_memoryAddress] = data;
        }
        if(_portActive)
        {
            switch(_portAddress)
            {
            case 0:
            case 2:
            case 4:
            case 6:
                _crtcindex = data;
                break;
            case 1:
            case 3:
            case 5:
            case 7:
                _crtc._crtcdata[_crtcindex] = data;
                break;   
            case 8:
                _mode = data;
                break;
            case 9:
                _colsel = data;
                break; 
            }
        }
    }
    UInt8 memory(UInt32 address)
    {
        if ((address & 0xf8000) == 0xb8000)
        {
            return _data[address & 0x3fff];
        }
        else return 0xff;
    }
    Rational<int> hDotsPerCycle() { return 1; }
    void initialize(String fileName, const File& configFile)
    {
        String data = File(fileName, configFile.parent(), true).contents();
        int length = 0x2000;
        _romdata.allocate(length);
        for (int i = 0; i < length; ++i)
            _romdata[i] = data[i];
    }
    Array<UInt8> _romdata;
private:
    UInt8 _chr;
    UInt8 _attr;
    UInt8 _chrdata;
    int _memoryAddress;
    bool _memoryActive;
    int _portAddress;
    bool _portActive;
    int _wait;
    int _cycle;
    UInt8 _mode;
    UInt8 _crtcindex;
    UInt8 _colsel;
    UInt8 _status;
    UInt8 _r;
    UInt8 _g;
    UInt8 _b;
    UInt8 _i;
    Array<UInt8> _data;    
    Motorola6845CRTC _crtc;
};