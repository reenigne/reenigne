template<class T> class Intel8255PPITemplate
  : public ISA8BitComponentTemplate<T>
{
public:
    Intel8255PPITemplate()
    {
        _keyboardclk = true;
        _keyboarddata = true;
        _keyboardtick = 0;
        _portb = 0xCC;
        _portc = 0x03;
        _scancode = 0;
        _wait = 0;
    }
    Rational<int> hDotsPerCycle() const { return 3; }
    void site()
    {
        _pic = this->_simulator->getPIC();
    }
    void simulateCycle()
    {
        _keyboardtick++;
        if(_keyboardtick == 239)
        {
            _keyboardtick = 0;
            simulateKeyboardCycle();
        }
    }
    void simulateKeyboardCycle()
    {
        if(_keyboardclk == true && _keyboarddata == true)
        {
            if(_wait != 0)
            {
                _wait--;
                if(_wait == 0)
                {
                    _pic->requestInterrupt(1);
                    _keyboarddata = false;
                }
            }
        }
    }
    void setAddress(UInt32 address)
    {
        _address = address & 0x3;
        this->_active = (address & 0x400003f0) == 0x40000060;
    }
    void read()
    {
        switch(_address)
        {
        case 0:
            this->set(_scancode);
            break;
        case 2:
            this->set(_portc);
            break;
        }
    }
    void write(UInt8 data)
    {
        switch(_address)
        {
        case 1:
            if(!(data & 0x40))
            {
                _scancode = 0xaa;
                _keyboarddata = true;
                _wait = 1;
            }
            if((data & 0x80))
            {
                _keyboarddata = true;
            }
            _portb = data;
            _keyboardclk = ((data & 0x40) != 0);
            break;
        }
    }
    UInt8 portB()
    {
        // TODO
        return _portb;
    }
    UInt8 _portc;
private:
    UInt32 _address;
    UInt8 _portb;
    UInt8 _scancode;
    int _wait;
    Intel8259PIC* _pic;
    int _keyboardtick; //Counter to tick the keyboard at approximately 20 KHz.
    bool _keyboardclk;
    bool _keyboarddata;
};
