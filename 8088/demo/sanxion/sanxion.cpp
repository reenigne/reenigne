typedef signed int Sample;

class SID : public Pipe<Byte, Sample>
{
public:
    void produce(int n)
    {
        // We process a frame at once - (1/50)s or 19656 SID cycles (SID clock rate is 985248Hz)
        Accessor<Byte> reader(_sink.reader(1000));
        Accessor<Sample> writer(_source.writer(19656));
        int read = 0;
        do {
            Byte address = reader.item();
            Byte value = reader.item();
            read += 2;
            if (address == 0xff)
                break;
            if (address < 0x15)
                _oscillators[address / 7].write(address % 7, value);
            else
                switch (address) {
                    case 0x15:
                        _filterControlLow = value;
                        break;
                    case 0x16:
                        _filterControlHigh = value;
                        break;
                    case 0x17:
                        _resonanceFilter = value;
                        break;
                    case 0x18:
                        _modeVol = value;
                        break;
                }
        } while (true);
        _sink.read(read);
        for (int i = 0; i < 19656; ++i) {
            int s1 = (_oscillators[0].sample() - 2048) << 18;
            int s2 = (_oscillators[1].sample() - 2048) << 18;
            int s3 = (_oscillators[2].sample() - 2048) << 18;
            int unfiltered = 0;
            int filtered = 0;
            if ((_resonanceFilter & 1) == 0)
                filtered += s1;
            else
                unfiltered += s1;
            if ((_resonanceFilter & 2) == 0)
                filtered += s2;
            else
                unfiltered += s2;
            if ((_resonanceFilter & 4) == 0)
                filtered += s3;
            else
                if ((_modeVol & 0x80) == 0)
                    unfiltered += s3;
            // TODO: filter "filtered"
            int s = (unfiltered + filtered) >> 4;
            s *= (_modeVol & 0x0f);
            writer.item() = static_cast<Sample>(s >> 16);
        }
        _source.written(19656);
    }
private:
    class Oscillator
    {
    public:
        void write(int address, Byte value)
        {
            switch (address) {
                case 0:
                    _frequency = (_freqency & 0xff00) | value;
                    break;
                case 1:
                    _frequency = (_frequency & 0xff) | (value << 8);
                    break;
                case 2:
                    _pulseWidth = (_pulseWidth & 0xf00) | value;
                    break;
                case 3:
                    _pulseWidth = (_pulseWidth & 0xff) | ((value << 8) & 0x0f);
                    break;
                case 4:
                    _control = value;
                    break;
                case 5:
                    _attackDecay = value;
                    break;
                case 6:
                    _sustainRelease = value;
                    break;
            }
        }
        int sample()  // 0 to 4095
        {
            _accumulator = (_accumulator + _frequency) & 0xffffff;
            switch (_control & 0xf0) {
                case 0:
                    return 0;
                case 0x10:
                    {
                        UInt32 a = _accumulator;
                        if ((_control & 4) != 0)
                            a ^= _syncSource->_accumulator;
                        return (((a & 0x800000) != 0 ? ~_accumulator : _accumulator) >> 11) & 0xfff;
                    }
                case 0x20:
                    return (_accumulator >> 12);
                case 0x40:
                    return ((_accmuluator >> 12) >= _pulseWidth) ? 0xfff : 0;
                case 0x80:
                    // TODO: noise
                default:
                    // Others not used in Sanxion
                    assert(false);
            }
        }
    private:
        UInt16 _frequency;
        UInt16 _pulseWidth;
        UInt32 _accumulator;
        Byte _control;
        Byte _attackDecay;
        Byte _sustainRelease;
        Oscillator* _syncSource;
    };
    Byte _filterControlLow;
    Byte _filterControlHigh;
    Byte _resonanceFilter;
    Byte _modeVol;
    Oscillator _oscillators[3];
};

// 0000 -> DC
// 0001 -> 0.059 Hz
// FFFF -> 3.848 KHz
