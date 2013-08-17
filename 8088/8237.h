template<class T> class Intel8237DMATemplate
  : public ISA8BitComponentTemplate<T>
{
public:
    Rational<int> hDotsPerCycle() const { return 3; }
    Intel8237DMATemplate()
    {
    }
    void simulateCycle()
    {
        for(int i = 0; i < 4; ++i)
        {
            _channels[i].simulateCycle();
        }
    }
    void setAddress(UInt32 address)
    {
        _address = address & 0xf;
        this->_active = (address & 0x400003f0) == 0x40000000;
    }
    void read()
    {
        if(_address < 8)
        {
            this->set(_channels[_address >> 1].read(_address & 1));
        }
    }
    void write(UInt8 data)
    {
        if(_address < 8)
        {
            _channels[_address >> 1].write(_address & 1,data);
        }
    }
    // Step 1: the device calls dmaRequest()
    // equivalent to raising a DRQ line.
    void dmaRequest(int channel)
    {
        // TODO
    }
    // Step 2: at the end of the IO cycle the CPU calls dmaRequested()
    // equivalent to checking the status of the READY line and raising the HLDA
    // line.
    bool dmaRequested()
    {
        // TODO: call _bus->setAddress() with the appropriate generated address
        return false;
    }
    // Step 3: device checks dmaAcknowledged() to see when to access the bus.
    // equivalent to checking the status of the DACK line.
    bool dmaAcknowledged(int channel)
    {
        // TODO
        return false;
    }
    // Step 4: the device calls dmaComplete()
    // equivalent to lowering a DRQline.
    void dmaComplete(int channel)
    {
        // TODO
    }

    String getText()
    {
        // TODO
        return String();
    }
private:
    class Channel
    {
    public:
        void simulateCycle()
        {
        }
        UInt8 read(UInt32 address)
        {
            switch(address)
            {
            case 0:
                if(_firstbyte) return _startaddress;
                else return (_startaddress >> 8);
                break;
            case 1:
                if(_firstbyte) return _count;
                else return (_count >> 8);
                break;
            }
        }
        void write(UInt32 address, UInt8 data)
        {
            switch(address)
            {
            case 0:
                if(_firstbyte)
                {
                    _startaddress = (_startaddress & 0xFF00) | data;
                    break;
                }
                else
                {
                    _startaddress = (_startaddress & 0xFF) | (data << 8);
                    break;
                }
                break;
            case 1:
                if(_firstbyte)
                {
                    _count = (_count & 0xFF00) | data;
                    break;
                }
                else
                {
                    _count = (_count & 0xFF) | (data << 8);
                    break;
                }
                break;
            }
        }
    private:
        UInt16 _startaddress;
        UInt16 _count;
        bool _firstbyte;
    };
    Channel _channels[4];
    UInt32 _address;
};
