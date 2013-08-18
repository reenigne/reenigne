class Intel8255PPI : public ISA8BitComponent
{
public:
    Intel8255PPI()
    {
    }
    void simulateCycle()
    {
    }
    void setAddress(UInt32 address)
    {
        _address = address & 0x3;
        _active = (address & 0x400003f0) == 0x40000060;
    }
    void read()
    {
        switch(_address)
        {
        case 0:
            set(0xAA); //Hardcoded for now.
            break;
        case 2:
            if(_portb & 0x08) set(0x00); //Hardcoded for now.
            else set(0x03);
            break;
        }
    }
    void write(UInt8 data)
    {
        switch(_address)
        {
        case 1:
            _portb = data;
            break;
        }
    }
    UInt8 portB()
    {
        // TODO
        return 0;
    }
private:
    UInt32 _address;
    UInt8 _portb;
};