#include "alfe\pipes.h"
#include "alfe\assert.h"
#include "alfe\audio.h"
#include "alfe\hash_table.h"
#include "alfe\owning_array.h"
#include "alfe\set.h"
#include "alfe\gcd.h"
#define _USE_MATH_DEFINES
#include <math.h>
//#include "alfe\convolution_pipe.h"
#include "alfe\main.h"

typedef signed short Sample;

class WaveBank
{
public:
    WaveBank() { _waveFile = fopen("waves.raw","wb"); }
    ~WaveBank()
    {
        printf("%i\n",_waves.count());
        fclose(_waveFile);
    }
    class WaveDescriptor
    {
    public:
        WaveDescriptor() : _volume(0) { }
        WaveDescriptor(int volume, int pulseWidth, int type, int scale)
          : _volume(volume),
            _pulseWidth(pulseWidth),
            _type(type),
            _scale(scale)
        {
            //_volume = (((_volume * 0xf) / 0xef1) * 0xfff) / 0xf;
            //_pulseWidth &= 0xf0;
            if (type != 2)
                _pulseWidth = 0;
        }
        int hash() const
        {
            if (_volume == 0)
                return 0;
            return _volume + (_pulseWidth << 12) + (_type << 21) + (_scale << 24);
        }
        bool operator==(const WaveDescriptor& other) const
        {
            if (_volume == 0 && other._volume == 0)
                return true;
            if (_volume != other._volume)
                return false;
            if (_pulseWidth != other._pulseWidth)
                return false;
            if (_type != other._type)
                return false;
            if (_scale != other._scale)
                return false;
            return true;
        }
        int volume() const { return _volume; }
        int pulseWidth() const { return _pulseWidth; }
        int type() const { return _type; }
        int scale() const { return _scale; }
        int operator[](int offset)  // offset = 0..0xff, returns -0x800000..0x7fffff
        {
            return (unscaled(offset*_scale) - 0x800)*_volume;
        }
//        void print() { printf("%03x %02x %01x %04x\n",_volume,_pulseWidth,_type,_scale); }
    private:
        int unscaled(int position)  // position = 0..0xffff, returns 0..0xfff
        {
            static int noise[256] = {
                0xda0, 0xe30, 0x900, 0xaf0, 0x050, 0x720, 0x070, 0xc90,
                0x2e0, 0xd70, 0x190, 0x860, 0x7e0, 0x280, 0x950, 0x3c0,
                0x4b0, 0x700, 0xba0, 0xcd0, 0x300, 0xf60, 0x400, 0xa80,
                0xe40, 0x590, 0x810, 0xd20, 0x6b0, 0x810, 0xb70, 0x060,
                0x420, 0x250, 0xcd0, 0x0e0, 0x920, 0x580, 0x0c0, 0xbd0,
                0x300, 0x120, 0x290, 0x690, 0x770, 0x920, 0xc30, 0x650,
                0xee0, 0x870, 0x900, 0x4e0, 0x4c0, 0xb90, 0x940, 0x1a0,
                0x290, 0x700, 0x3e0, 0x800, 0x710, 0x650, 0xca0, 0xa20,
                0xd10, 0x4c0, 0x870, 0xf00, 0x020, 0x8d0, 0x290, 0x570,
                0x130, 0x8a0, 0x670, 0x3c0, 0x870, 0x140, 0x6b0, 0x4c0,
                0xba0, 0x9c0, 0x510, 0x3d0, 0xe20, 0x3a0, 0xa00, 0x5c0,
                0x650, 0xd00, 0xe30, 0x880, 0xea0, 0x150, 0xd00, 0x070,
                0xe10, 0x2b0, 0x860, 0x1b0, 0x450, 0x560, 0xae0, 0x800,
                0x3c0, 0x0d0, 0x580, 0x320, 0xd90, 0x280, 0xb60, 0x700,
                0x000, 0xe40, 0x690, 0xc10, 0x930, 0xcb0, 0x420, 0xb70,
                0x850, 0x060, 0x270, 0x480, 0x0e0, 0x9c0, 0x590, 0x140,
                0xbb0, 0x280, 0x320, 0x3d0, 0x410, 0x770, 0xe30, 0xcb0,
                0xa60, 0xde0, 0x440, 0x950, 0xec0, 0x0a0, 0xb90, 0x190,
                0x5f0, 0x330, 0xf30, 0x260, 0xae0, 0x640, 0x550, 0xcc0,
                0xcb0, 0xf90, 0x9b0, 0x9f0, 0x330, 0x760, 0x270, 0xac0,
                0x670, 0x5d0, 0xca0, 0xdf0, 0xf90, 0x9f0, 0xb70, 0x3b0,
                0x660, 0x3f0, 0xed0, 0x570, 0xff0, 0xca0, 0xff0, 0xbd0,
                0xde0, 0x370, 0xf80, 0x6e0, 0xbc0, 0xf80, 0x550, 0xf80,
                0xea0, 0xf10, 0xb90, 0xc60, 0x720, 0xe00, 0xc40, 0xac0,
                0xc00, 0x500, 0x890, 0xc80, 0x320, 0x910, 0x010, 0x260,
                0x620, 0x000, 0x840, 0x4d0, 0x410, 0x930, 0x8a0, 0x0a0,
                0x350, 0x150, 0x060, 0x220, 0x690, 0x0c0, 0x9e0, 0x510,
                0x510, 0xaf0, 0xaa0, 0x320, 0x150, 0x4c0, 0x670, 0xf10,
                0x8a0, 0x8e0, 0x780, 0x540, 0x950, 0xa80, 0x630, 0x390,
                0x8b0, 0x570, 0x730, 0xc60, 0xa60, 0xac0, 0x440, 0x1d0,
                0xc80, 0x5b0, 0xb80, 0x9b0, 0x340, 0x330, 0x640, 0x220,
                0xed0, 0x410, 0xdf0, 0xc20, 0xdb0, 0xa50, 0x9f0, 0x270};
            switch (_type) {
                case 0:
                    return (((position & 0x800000) != 0 ? ~position : position) >> 11) & 0xfff;
                case 1:
                    return (position >> 12) & 0xfff;
                case 2:
                    return (((position >> 16) & 0xff) >= _pulseWidth) ? 0xfff : 0;
                case 3:
                    return noise[(position >> 16) & 0xff];
            }
        }
        int _volume;     // 0 to 0xfff (really 0xff*0xf = 0xef1)
        int _pulseWidth; // 0 to 0x100
        int _type;       // 0 = triangle, 1 = sawtooth, 2 = square/pulse, 3 = noise
        int _scale;      // Number of cycles (for sync) in units of 1/65536 of a cycle
    };
    class Wave
    {
    public:
        int hash() const
        {
            int h = 0;
            for (int i = 0; i < 0x100; ++i)
                h = h * 67 + _data[i] - 113;
            return h;
        }
        bool operator==(const Wave& other)
        {
            for (int i = 0; i < 0x100; ++i)
                if (_data[i] != other._data[i])
                    return false;
            return true;
        }
        int& operator[](int offset) { return _data[offset]; }
        const int& operator[](int offset) const { return _data[offset]; }
        UInt64 distance(const Wave* other)
        {
            UInt64 total = 0;
            for (int sample = 0; sample < 0x100; ++sample) {
                int d = _data[sample] - other->_data[sample];
                total += d*d;
            }
            return total;
        }
    private:
        int _data[0x100];
    };
    Wave* getWave(WaveDescriptor descriptor)
    {
//        descriptor.print();
        Wave* wave;
        if (!_bank.hasKey(descriptor)) {
            Wave t;
            for (int i = 0; i < 0x100; ++i) {
                //t[i] = (((descriptor[i] + 0x800000) * 72) & 0xff000000) / 72 - 0x800000;
                t[i] = descriptor[i];
            }
            SInt64 s = 0;
            SInt64 c = 0;
            static int sineTable[0x100] = {
                 0x0000, 0x0324, 0x0647, 0x096a, 0x0c8b, 0x0fab, 0x12c8, 0x15e2,
                 0x18f8, 0x1c0b, 0x1f19, 0x2223, 0x2528, 0x2826, 0x2b1f, 0x2e11,
                 0x30fb, 0x33de, 0x36ba, 0x398c, 0x3c56, 0x3f17, 0x41ce, 0x447a,
                 0x471c, 0x49b4, 0x4c3f, 0x4ebf, 0x5133, 0x539b, 0x55f5, 0x5842,
                 0x5a82, 0x5cb4, 0x5ed7, 0x60ec, 0x62f2, 0x64e8, 0x66cf, 0x68a6,
                 0x6a6d, 0x6c24, 0x6dca, 0x6f5f, 0x70e2, 0x7255, 0x73b5, 0x7504,
                 0x7641, 0x776c, 0x7884, 0x798a, 0x7a7d, 0x7b5d, 0x7c29, 0x7ce3,
                 0x7d8a, 0x7e1d, 0x7e9d, 0x7f09, 0x7f62, 0x7fa7, 0x7fd8, 0x7ff6,
                 0x8000, 0x7ff6, 0x7fd8, 0x7fa7, 0x7f62, 0x7f09, 0x7e9d, 0x7e1d,
                 0x7d8a, 0x7ce3, 0x7c29, 0x7b5d, 0x7a7d, 0x798a, 0x7884, 0x776c,
                 0x7641, 0x7504, 0x73b5, 0x7255, 0x70e2, 0x6f5f, 0x6dca, 0x6c24,
                 0x6a6d, 0x68a6, 0x66cf, 0x64e8, 0x62f2, 0x60ec, 0x5ed7, 0x5cb4,
                 0x5a82, 0x5842, 0x55f5, 0x539b, 0x5133, 0x4ebf, 0x4c3f, 0x49b4,
                 0x471c, 0x447a, 0x41ce, 0x3f17, 0x3c56, 0x398c, 0x36ba, 0x33de,
                 0x30fb, 0x2e11, 0x2b1f, 0x2826, 0x2528, 0x2223, 0x1f19, 0x1c0b,
                 0x18f8, 0x15e2, 0x12c8, 0x0fab, 0x0c8b, 0x096a, 0x0647, 0x0324,
                 0x0000,-0x0324,-0x0647,-0x096a,-0x0c8b,-0x0fab,-0x12c8,-0x15e2,
                -0x18f8,-0x1c0b,-0x1f19,-0x2223,-0x2528,-0x2826,-0x2b1f,-0x2e11,
                -0x30fb,-0x33de,-0x36ba,-0x398c,-0x3c56,-0x3f17,-0x41ce,-0x447a,
                -0x471c,-0x49b4,-0x4c3f,-0x4ebf,-0x5133,-0x539b,-0x55f5,-0x5842,
                -0x5a82,-0x5cb4,-0x5ed7,-0x60ec,-0x62f2,-0x64e8,-0x66cf,-0x68a6,
                -0x6a6d,-0x6c24,-0x6dca,-0x6f5f,-0x70e2,-0x7255,-0x73b5,-0x7504,
                -0x7641,-0x776c,-0x7884,-0x798a,-0x7a7d,-0x7b5d,-0x7c29,-0x7ce3,
                -0x7d8a,-0x7e1d,-0x7e9d,-0x7f09,-0x7f62,-0x7fa7,-0x7fd8,-0x7ff6,
                -0x8000,-0x7ff6,-0x7fd8,-0x7fa7,-0x7f62,-0x7f09,-0x7e9d,-0x7e1d,
                -0x7d8a,-0x7ce3,-0x7c29,-0x7b5d,-0x7a7d,-0x798a,-0x7884,-0x776c,
                -0x7641,-0x7504,-0x73b5,-0x7255,-0x70e2,-0x6f5f,-0x6dca,-0x6c24,
                -0x6a6d,-0x68a6,-0x66cf,-0x64e8,-0x62f2,-0x60ec,-0x5ed7,-0x5cb4,
                -0x5a82,-0x5842,-0x55f5,-0x539b,-0x5133,-0x4ebf,-0x4c3f,-0x49b4,
                -0x471c,-0x447a,-0x41ce,-0x3f17,-0x3c56,-0x398c,-0x36ba,-0x33de,
                -0x30fb,-0x2e11,-0x2b1f,-0x2826,-0x2528,-0x2223,-0x1f19,-0x1c0b,
                -0x18f8,-0x15e2,-0x12c8,-0x0fab,-0x0c8b,-0x096a,-0x0647,-0x0324};
            for (int i = 0; i < 0x100; ++i) {
                s += t[i]*sineTable[i];
                c += t[i]*sineTable[(i+0x40)&0xff];
            }
            int phase = static_cast<int>(0x100*atan2(static_cast<double>(c),static_cast<double>(s))/M_PI);
            Wave shifted;
            for (int i = 0; i < 0x100; ++i)
                shifted[i] = t[(i-phase)&0xff];

            SInt64 best = 0x7fffffffffffffffLL;
            int bestIndex = -1;
            for (int i = 0; i < _waves.count(); ++i) {
                SInt64 t = 0;
                for (int j = 0; j < 0x100; ++j) {
                    int d = (shifted[j] - (*(_waves[i]))[j]) >> 8;
                    t += d*d;
                }
                if (t < best) {
                    wave = _waves[i];
                    best = t;
                }
            }
            if (best >= 100000000LL*0x100) {  // TODO: tune
                wave = new Wave(shifted);
                _waves.add(wave);
                for (int i = 0; i < 0x100; ++i) {
                    fputc(shifted[i] >> 8, _waveFile);
                    fputc(shifted[i] >> 16, _waveFile);
                }
            }
            _bank.add(descriptor, wave);
        }
        else
            wave = _bank[descriptor];
        return wave;
    }
    class RingModulationParameters
    {
    public:
        RingModulationParameters()
            : _tFrequency(0), _sFrequency(0), _volume(0)
        { }
        RingModulationParameters(UInt16 tFrequency, UInt16 sFrequency,
            int volume)
          : _tFrequency(tFrequency), _sFrequency(sFrequency), _volume(volume)
        { }
        int hash() const
        {
            if (_volume == 0)
                return 0;
            return ((_tFrequency << 16) + _sFrequency)^_volume;
        }
        bool operator==(const RingModulationParameters& other)
        {
            if (_volume == 0 && other._volume)
                return true;
            return _tFrequency == other._tFrequency &&
                _sFrequency == other._sFrequency && _volume == other._volume;
        }
    private:
        UInt16 _tFrequency;
        UInt16 _sFrequency;
        int _volume;
    };
    void setRingModulation(UInt16 tFrequency, UInt16 sFrequency, int volume)
    {
        RingModulationParameters parameters(tFrequency, sFrequency, volume);
        if (!_ringModulations.has(parameters)) {
            _ringModulations.add(parameters);
            printf("0x%04x 0x%04x 0x%08x %lf %i\n", tFrequency, sFrequency, lcm(static_cast<int>(tFrequency), static_cast<int>(sFrequency)), static_cast<double>(tFrequency)/static_cast<double>(sFrequency), volume);
        }
    }
private:
    HashTable<WaveDescriptor, Wave*> _bank;
    Set<RingModulationParameters> _ringModulations;
    OwningArray<Wave> _waves;
    FILE* _waveFile;
};

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
        _modeVol(0),
        _frame(0)
    {
        for (int i = 0; i < 3; ++i) {
            _oscillators[i].setPrevious(&_oscillators[(i + 2)%3]);
            _oscillators[i].setSID(this);
        }
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
                        //if ((value & 0x80) != (_modeVol & 0x80))
                        //    printf("3off changed to %i\n",value >> 7);
                        //if ((value & 0x1) != (_modeVol & 0x1))
                        //    printf("filt1 changed to %i\n",(value >> 0) & 1);
                        //if ((value & 0x2) != (_modeVol & 0x2))
                        //    printf("filt2 changed to %i\n",(value >> 1) & 1);
                        //if ((value & 0x4) != (_modeVol & 0x4))
                        //    printf("filt3 changed to %i\n",(value >> 2) & 1);
                        _modeVol = value;
                        break;
                }
        } while (true);
        int volume = _modeVol & 0x0f;
        for (int i = 0; i < 3; ++i)
            _oscillators[i].setWave(volume);
        _sink.read(read);
        for (int i = 0; i < 19656; ++i) {
            for (int j = 0; j < 3; ++j)
                _oscillators[j].update();
            int s1 = _oscillators[0].sample(volume);
            int s2 = _oscillators[1].sample(volume);
            int s3 = _oscillators[2].sample(volume);
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
                //if ((_modeVol & 0x80) == 0)   // 3OFF not used in Sanxion
                    unfiltered += s3;
            // TODO: filter "filtered" - Sanxion sounds fine without it though for the most part - the filters are only used in the end fade-out.
            int s = unfiltered + filtered;
            //s *= (_modeVol & 0x0f);
            writer.item() = static_cast<Sample>(s >> 10);
        }
        _source.written(19656);
        if (_sink.finite() && _sink.remaining() <= 0)
            _source.remaining(0);
        ++_frame;
        if (_frame == 50) {
            printf(".");
            _frame = 0;
        }
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
        void setSID(MOSSID* sid) { _sid = sid; }
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
                    _pulseWidth = (_pulseWidth & 0xff) | ((value << 8) & 0xf00);
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
            int frequency = _frequency;
            if ((_control & 0xf0) == 0x80)
                frequency >>= 4;       // TODO: Fix up ring modulation
            if ((_control & 2) != 0 && _previous->_frequency != 0)
                frequency = _previous->_frequency;
            _accumulator = (_accumulator + frequency) & 0xffffff;
            bool bit23 = ((_accumulator & 0x800000) != 0);
            _bit23Rising = (bit23 && !_bit23);
            _bit23 = bit23;
            bool bit19 = ((_accumulator & 0x80000) != 0);
            if (bit19 && !_bit19)
                _lfsr = ((_lfsr << 1) & 0x7fffff) |
                    (((_lfsr>>22) ^ (_lfsr>>17)) & 1);
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
            if (_holdZero)
                return;
            switch (_envelopeState) {
                case 0:
                    _envelope = (_envelope + 1) & 0xff;
                    if (_envelope == 0xff)
                        _envelopeState = 1;
                    break;
                case 1:
                    if (_envelope !=
                        ((_sustainRelease & 0xf0) | (_sustainRelease >> 4)))
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
        int sample(int volume)  // -0x800000 to 0x7fffff
        {
            //return unenvelopedSample()*_envelope*volume;
            return sample1(_envelope*volume);
        }
        int unenvelopedSample()  // -0x800 to 0x7ff
        {
            if (synchronize() && !_previous->synchronize())
                _accumulator = 0;
            int s = 0;
            int type = _control & 0xf0;
            //if (_next->ringModulation() && (_next->_control & 0xf0) == 0x10 /*&& _envelope == 0*/)
            //    type = 1;
            switch (type) {
                case 0:
                    break;
                //case 1:
                //    s = ((_accumulator & 0x800000) != 0 ? 0xfff : 0);
                //    break;
                case 0x10:
                    {
                        UInt32 a = _accumulator;
                        if (ringModulation())
                            a ^= _previous->_accumulator;
                        s = (((a & 0x800000) != 0 ? ~_accumulator : _accumulator) >> 11) & 0xfff;
                        //if (ringModulation())
                        //    s = ((((s - 0x800)*_previous->unenvelopedSample()) >> 11) + 0x800) & 0xfff;
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
            //if (!(_next->ringModulation() && (_next->_control & 0xf0) == 0x10 && _next->_envelope != 0))
            //    return 0;
            return s - 0x800;
        }
        int sample1(int volume) { return (*_wave)[_accumulator >> 16]; }
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
        void setWave(int volume)
        {
            volume *= _envelope;
            int type;
            switch (_control & 0xf0) {
                case 0:
                    volume = 0;
                    type = 0;
                    break;
                case 0x10:
                    type = 0;
                    break;
                case 0x20:
                    type = 1;
                    break;
                case 0x40:
                    type = 2;
                    break;
                case 0x80:
                    type = 3;
                    break;
            }
            int scale = 0x10000;
            if ((_control & 2) != 0 && _previous->_frequency != 0)
                scale = 0x10000*_frequency/_previous->_frequency;
            _wave = _sid->_bank.getWave(WaveBank::WaveDescriptor(volume, _pulseWidth >> 4, type, scale));
            //if (ringModulation())
            //    _sid->_bank.setRingModulation(_frequency, _previous->_frequency, volume);
        }
    private:
        MOSSID* _sid;
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
        WaveBank::Wave* _wave;
    };
    Byte _filterControlLow;
    Byte _filterControlHigh;
    Byte _resonanceFilter;
    Byte _modeVol;
    Oscillator _oscillators[3];
    int _frame;
public:
    WaveBank _bank;
};

class Program : public ProgramBase
{
public:
    void run()
    {
        //for (int i = 0; i < 0x100; ++i) {
        //    int s = static_cast<int>(0x8000*sin(i*2*M_PI/0x100));
        //    printf("%c0x%04x,",s<0 ? '-' : ' ',abs(s));
        //    if (i % 8 == 7)
        //        printf("\n");
        //}

        //UInt32 lfsr = 0x7ffff8;
        //for (int i = 0; i < 16384; ++i)
        //    lfsr = ((lfsr << 1) & 0x7fffff) |
        //        (((lfsr>>22) ^ (lfsr>>17)) & 1);
        //for (int i = 0; i < 256; ++i) {
        //    lfsr = ((lfsr << 1) & 0x7fffff) |
        //        (((lfsr>>22) ^ (lfsr>>17)) & 1);
        //    int s =
        //        ((lfsr >> 11) & 0x800) |
        //        ((lfsr >> 10) & 0x400) |
        //        ((lfsr >>  7) & 0x200) |
        //        ((lfsr >>  5) & 0x100) |
        //        ((lfsr >>  4) & 0x080) |
        //        ((lfsr >>  1) & 0x040) |
        //        ((lfsr <<  1) & 0x020) |
        //        ((lfsr <<  2) & 0x010);
        //    printf("0x%03x, ", s);
        //}
        COMInitializer com;
        File file(String("sid.dat"));
        FileSource<Byte> source(file);
        MOSSID sid;
        //NearestNeighborInterpolator<Sample> interpolator(985248, 44100);
        LinearInterpolator<Sample> interpolator(985248, 44100);
        //XAudio2Sink<Sample> sink;
        //DirectSoundSink<Sample> sink(NULL, 44100, 1024, 1);
        //WaveOutSink<Sample> sink;
        WaveFileSink<Sample> sink(File(String("sanxion.wav")));
        source.connect(sid.sink());
        sid.source()->connect(interpolator.sink());
        interpolator.source()->connect(&sink);
        sink.play();
        //sink.wait();
    }
};
