#include "unity\audio.h"
#include "unity\pipes.h"
#include "unity\assert.h"

typedef signed int Sample;

class MOSSID : public Pipe<Byte, Sample, MOSSID>
{
public:
#pragma warning( push )
#pragma warning( disable : 4355 )
    MOSSID()
      : Pipe(this),
         _filterControlLow(0),
        _filterControlHigh(0),
        _resonanceFilter(0),
        _modeVol(0)
    {
        for (int i = 0; i < 3; ++i)
            _oscillators[i].setPrevious(&_oscillators[(i + 2)%3]);
    }
#pragma warning ( pop )
    void consume(int n) { produce(n); }
    void produce(int n)
    {
        // We process a frame at once - 
        // (1/50)s or 19656 SID cycles (SID clock rate is 985248Hz)
        Accessor<Byte> reader = _sink.reader(1000);
        Accessor<Sample> writer = _source.writer(19656);
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
            for (int j = 0; j < 3; ++j)
                _oscillators[j].update();
            int s1 = (_oscillators[0].sample() - 2048) << 10;
            int s2 = (_oscillators[1].sample() - 2048) << 10;
            int s3 = (_oscillators[2].sample() - 2048) << 10;
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
        Oscillator()
          : _frequency(0),
            _pulseWidth(0),
            _accumulator(0),
            _control(0),
            _attackDecay(0),
            _sustainRelease(0),
            _bit23(false),
            _bit19(false),
            _bit23Rising(false),
            _lfsr(0x7ffff8),
            _envelopeClock(0),
            _envelopeState(2),
            _exponentialCounter(0),
            _exponentialCounterPeriod(1),
            _envelope(0),
            _gate(false),
            _holdZero(true)
        { }
        void write(int address, Byte value)
        {
            switch (address) {
                case 0:
                    _frequency = (_frequency & 0xff00) | value;
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
        void update()
        {
            // Update waveform generator
            _accumulator = (_accumulator + _frequency) & 0xffffff;
            bool bit23 = ((_accumulator & 0x800000) != 0);
            _bit23Rising = (bit23 && !_bit23);
            _bit23 = bit23;
            bool bit19 = ((_accumulator & 0x80000) != 0);
            if (bit19 && !_bit19)
                _lfsr = ((_lfsr << 1) & 0x7fffff) | (((_lfsr>>22) ^ (_lfsr>>17)) & 1);
            _bit19 = bit19;

            // Update envelope
            bool gate = ((_control & 1) != 0);
            if (gate && !_gate) {
                _envelopeState = 0;
                _holdZero = false;
            }
            else
                if (!gate && _gate)
                    _envelopeState = 2;
            _gate = gate;
            ++_envelopeClock;
            if ((_envelopeClock & 0x8000) != 0)
                _envelopeClock = (_envelopeClock + 1) & 0x7fff;
            static UInt16 envelopeCounts[16] = {
                    9,    32,    63,    95,   149,   220,   267,   313,
                  392,   977,  1965,  3126,  3907, 11720, 19532, 31251};
            UInt16 count;
            switch (_envelopeState) {
                case 0: count = _attackDecay >> 4; break;
                case 1: count = _attackDecay; break;
                case 2: count = _sustainRelease; break;
            }
            count = envelopeCounts[count & 0xf];
            if (_envelopeClock != count)
                return;
            _envelopeClock = 0;

            bool update = (_envelopeState == 0);
            if (!update) {
                ++_exponentialCounter;
                if (_exponentialCounter == _exponentialCounterPeriod)
                    update = true;
            }
            if (!update)
                return;
            _exponentialCounter = 0;
            switch (_envelopeState) {
                case 0:
                    _envelope = (_envelope + 1) & 0xff;
                    if (_envelope == 0xff)
                        _envelopeState = 1;
                    break;
                case 1:
                    if (_envelope != ((_sustainRelease & 0xf0) | (_sustainRelease >> 4)))
                        --_envelope;
                    break;
                case 2:
                    _envelope = (_envelope - 1) & 0xff;
                    break;
            }
            switch (_envelope) {
                case 0xff: _exponentialCounterPeriod = 1; break;
                case 0x5d: _exponentialCounterPeriod = 2; break;
                case 0x36: _exponentialCounterPeriod = 4; break;
                case 0x1a: _exponentialCounterPeriod = 8; break;
                case 0x0e: _exponentialCounterPeriod = 16; break;
                case 0x06: _exponentialCounterPeriod = 30; break;
                case 0x00:
                    _exponentialCounterPeriod = 1;
                    _holdZero = true;
                    break;
            }
        }
        int sample()  // 0 to 0xfff*0xff
        {
            if (synchronize() && !_previous->synchronize())
                _accumulator = 0;
            int s = 0;
            switch (_control & 0xf0) {
                case 0:
                    break;
                case 0x10:
                    {
                        UInt32 a = _accumulator;
                        if (ringModulation())
                            a ^= _previous->_accumulator;
                        s = (((a & 0x800000) != 0 ? ~_accumulator : _accumulator) >> 11) & 0xfff;
                    }
                    break;
                case 0x20:
                    s = (_accumulator >> 12);
                    break;
                case 0x40:
                    s = ((_accumulator >> 12) >= _pulseWidth) ? 0xfff : 0;
                    break;
                case 0x80:
                    s = ((_lfsr >> 11) & 0x800) |
                        ((_lfsr >> 10) & 0x400) |
                        ((_lfsr >>  7) & 0x200) |
                        ((_lfsr >>  5) & 0x100) |
                        ((_lfsr >>  4) & 0x080) |
                        ((_lfsr >>  1) & 0x040) |
                        ((_lfsr <<  1) & 0x020) |
                        ((_lfsr <<  2) & 0x010);
                    break;
                default:
                    // Others not used in Sanxion
                    assert(false);
            }
            return s*_envelope;
        }
        void setPrevious(Oscillator* previous)
        {
            _previous = previous;
            previous->_next = this;
        }
        bool ringModulation() const { return (_control & 4) != 0; }
        bool synchronize() const
        {
            return ((_control & 2) != 0) && _previous->_bit23Rising;
        }
    private:
        UInt16 _frequency;
        UInt16 _pulseWidth;
        UInt32 _accumulator;
        Byte _control;
        Byte _attackDecay;
        Byte _sustainRelease;
        Oscillator* _previous;
        Oscillator* _next;
        bool _bit23;
        bool _bit19;
        bool _bit23Rising;
        UInt32 _lfsr;
        UInt16 _envelopeClock;
        int _envelopeState;
        UInt8 _exponentialCounter;
        UInt8 _exponentialCounterPeriod;
        UInt8 _envelope;
        bool _gate;
        bool _holdZero;
    };
    Byte _filterControlLow;
    Byte _filterControlHigh;
    Byte _resonanceFilter;
    Byte _modeVol;
    Oscillator _oscillators[3];
};

#ifdef _WIN32
int main()
#else
int main(int argc, char* argv[])
#endif
{
    BEGIN_CHECKED {
        MOSSID sid;
        XAudio2Sink<Sample> sink;
        // TODO: 
    }
    END_CHECKED(Exception& e) {
        e.write(Handle::consoleOutput());
    }
}