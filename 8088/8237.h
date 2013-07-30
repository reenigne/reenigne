#include "common.h"

class Intel8237DMA : public ISA8BitComponent
{
public:
    Intel8237DMA()
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
        _active = (address & 0x400003f0) == 0x40000000;
    }
    void read()
    {
        if(_address < 8)
        {
            set(_channels[_address >> 1].read(_address & 1));
        }
    }
    void write(UInt8 data)
    {
        if(_address < 8)
        {
            _channels[_address >> 1].write(_address & 1,data);
        }
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