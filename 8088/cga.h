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
        if(_wait != 0)
        {
            _wait--;
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
			case 4:
				set(_crtcindex);
				break;
			case 8:
				set(_mode);
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
			case 4:
				_crtcindex = data;
				break;
			case 8:
				_mode = data;
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
private:
    int _memoryAddress;
    bool _memoryActive;
    int _portAddress;
    bool _portActive;
    int _wait;
    int _cycle;
	UInt8 _mode;
	UInt8 _crtcindex;
    Array<UInt8> _data;
};