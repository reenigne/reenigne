class IBMCGA : public ISA8BitComponent
{
public:
    void simulateCycle()
    {
        ++_cycle;
        if(_cycle == 16)
        {
            _cycle = 0;
        }
        if(_cycle == 0 || (_cycle == 8 && !(_mode & 1)))
        {
            _crtc.simulateCycle();
            _crtc.ma &= 0x1fff;
            _crtc.ra &= 0x7;
            _chr = _data[_crtc.ma << 1];
            _attr = data[(_crtc.ma << 1) + 1];
            _chrdata = _romdata[(0x1800 | _crtc.ra) + (chr << 3)];
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
            
        }
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
    Rational<int> hDotsPerCycle() { return 1; 
    void initialize(const ROMData& romData, const File& configFile)
    {
        _start = romData.start();
        String data = File(romData.file(), configFile.parent(), true). contents();
        int length = 0x2000;
        _data.allocate(length);
        int offset = romData.offset();
        for (int i = 0; i < length; ++i)
        _data[i] = data[i + offset];
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