#include "alfe/main.h"
#include "alfe/vectors.h"
#include "alfe/rational.h"
#include "alfe/hash_table.h"
#include "alfe/space.h"

static const int samplesPerFrame = 262;
static const int minSample = 1;
static const int maxSample = 76;
static const bool minimizeClicks = true;

class NoteDescriptor
{
public:
    NoteDescriptor() {}
    NoteDescriptor(Rational cyclesPerFrame, Rational amplitude)
        : _cyclesPerFrame(cyclesPerFrame), _amplitude(amplitude)
    {
    }
    int hash() const { return _cyclesPerFrame.hash()*67 + _amplitude.hash(); }
    bool operator==(const NoteDescriptor& other) const
    {
        return _cyclesPerFrame == other._cyclesPerFrame &&
            _amplitude == other._amplitude;
    }
    int samples() const
    {
        if (minimizeClicks) {
            // We need enough samples in this waveform to be able to start the
            // wave at any phase, to minimize clicking when the frequency
            // changes.
            return samplesPerFrame + samplesPerCycle().value<int>();
        }
        int d = _cyclesPerFrame.denominator;
        return samplesPerFrame +
            ((d-1)*samplesPerFrame/(d*_cyclesPerFrame)).ceiling();
    }
    Rational samplesPerCycle() const
    {
        return samplesPerFrame/_cyclesPerFrame;
    }
    double amplitude() const { return _amplitude.value<double>(); }
    Rational endPhase(Rational startPhase) const
    {
        return (startPhase + _cyclesPerFrame).frac();
    }
    int frequency() const
    {
        return (_cyclesPerFrame*65536/262 + Rational(1, 2)).value<int>();
    }
private:
    Rational _cyclesPerFrame;
    Rational _amplitude;
};

class NoteData
{
public:
    NoteData() {}
    NoteData(NoteDescriptor noteDescriptor, int offset)
        : _noteDescriptor(noteDescriptor), _offset(offset)
    {
    }
    int offset(Rational phase) const
    {
        return _offset + (_noteDescriptor.samplesPerCycle()*phase +
            Rational(1, 2)).value<int>();
    }
private:
    NoteDescriptor _noteDescriptor;
    int _offset;
};


class Program : public ProgramBase
{
public:
    void run()
    {
        int mode;
        bool parsed = false;
        if (_arguments.count() >= 3) {
            CharacterSource source(_arguments[2]);
            Space::parse(&source);
            Span span;
            parsed = Space::parseInteger(&source, &mode, &span);
            CharacterSource s = source;
            if (s.get() != -1)
                source.location().throwError("Expected end of string");
        }
        if (!parsed) {
            console.write("Syntax: " + _arguments[0] +
                " <MOD file name> <mode>\n"
                "Modes are: \n"
                "  0 for VIC (1 channel, sample point per scanline, update "
                "per frame,\n"
                "    262 sample points per frame, 76 levels, 59.92Hz tick "
                "rate).\n"
                "  1 for SID (4 channels, 256-point samples, 65536 "
                "frequencies).\n"
                "  2 for Paula (4 channels, 256 frequencies).\n");
            return;
        }
        _modFile = File(_arguments[1], true).contents();
        int type = getIntAt(1080);
        int nSamples;
        switch (type) {
            case 0x4d2e4b2e:  // M.K.
                nSamples = 31;
                break;
            case 0x464c5434:  // FLT4
            case 0x464c5438:  // FLT8
            case 0x3643484e:  // 6CHN
            case 0x3843484e:  // 8CHN
            case 0x4d214b21:  // M!K!
                throw Exception("Don't understand this format yet");
            default:
                nSamples = 15;
        }
        _samples.allocate(nSamples + 1);
        if (type == 0x4d2e4b2e)
            nSamples = 31;
        int offset = 0;
        int p = 20;
        for (int i = 0; i < nSamples; ++i) {
            Sample sample;
            sample._length = getWordAt(p + 22);
            sample._fineTune = getByteAt(p + 24);
            if (sample._fineTune >= 8)
                sample._fineTune -= 16;
            sample._volume = getByteAt(p + 25);
            sample._repeatOffset = getWordAt(p + 26);
            sample._repeatLength = getWordAt(p + 28);
            sample._offset = o;
            o += sample._length;
            _samples[i] = sample;
            p += 30;
        }
        int nPositions = getByteAt(p);
        ++p;
        int nPatterns = 0;
        for (int i = 0; i < 128; ++i) {
            int pattern = getByteAt(p);
            _positions[i] = pattern;
            nPatterns = max(pattern, nPatterns);
            ++p;
        }
        ++nPatterns;
        _patterns.allocate(nPatterns);
        for (int pattern = 0; pattern < nPatterns; ++pattern) {
            for (int division = 0; division < 64; ++division) {
                for (int channel = 0; channel < 4; ++channel) {
                    int period = getWordAt(p);
                    p += 2;
                    int command = getWordAt(p);
                    p += 2;
                    int sample = ((period & 0xf000) >> 8) |
                        ((command & 0xf000) >> 12);
                    period &= 0xfff;
                    command &= 0xfff;
                    if (sample > nSamples)
                        sample = 0;
                    int operand1 = (command & 0xf0) >> 4;
                    int operand2 = command & 0x0f;
                    command = (command & 0xf00) >> 8;
                    Channel* c = &(_patterns[pattern]._divisions[division].
                        _channels[channel]);
                    c->_period = period;
                    c->_sample = sample;
                    c->_command = command;
                    c->_operand1 = operand1;
                    c->_operand2 = operand2;
                }
            }
        }

        int cyclesPerTick = 1773447/125;
        int ticksPerDivision = 6;
        int position = 0;
        int pattern = _positions[position];
        int division = 0;
        int tick = 0;
        do {
            // TODO: determine:
            //   For each channel:
            for (int channel = 0; channel < 4; ++channel) {
                int sample_current_position;
                int sample_end_position;
                int sample_restart_position;
                int pitch;
                int volume = 64;
                output(channel, sample_current_position, sample_end_position,
                    sample_restart_position, pitch, volume, cyclesPerTick,
                    mode);

            }

            ++tick;
            if (tick == ticksPerDivision) {
                tick = 0;

            }

        } while (true);
        for (int position = 0; position < nPatterns; ++position) {
            int pattern = _positions[position];

        }
            

        FileHandle output = File("tables.asm").openWrite();
        output.write("align 16\n\n");
        //output.write("sineTable:");
        //for (int y = 0; y < 838 + 116 - 1; ++y) {
        //    int x = static_cast<int>(78.5 + 78.5 * sin(7 * tau*y / 838));
        //    if (x >= 157)
        //        x = 156;
        //    if (x < 0)
        //        x = 0;
        //    if ((y & 7) == 0)
        //        output.write("\n  dw ");
        //    else
        //        output.write(", ");
        //    output.write(String(hex(x*2, 4)));
        //}
        //output.write("\npixelTable:");
        //for (int x = 0; x < 320; ++x) {
        //    int xx = x % 160;
        //    
        //}
        output.write("\n\nunrolledCode:\n");
    }
private:
    class Sample
    {
    public:
        int _length;        // in words
        int _fineTune;      // -8..7
        int _volume;        // 0..64
        int _repeatOffset;  // in words
        int _repeatLength;  // in words (<2 for no repeat)
        int _offset;
    };
    class Channel
    {
    public:
        int _period;
        int _sample;
        int _command;
        int _operand1;
        int _operand2;
    };
    class Division
    {
    public:
        Channel _channels[4];
    };
    class Pattern
    {
    public:
        Division _divisions[64];
    };
    int getByteAt(int p)
    {
        return _modFile[p];
    }
    int getWordAt(int p)
    {
        return (getByteAt(p) << 8) | getByteAt(p + 1);
    }
    int getIntAt(int p)
    {
        return (getWordAt(p) << 16) | getWordAt(p + 2);
    }
    Array<Sample> _samples;
    Array<Pattern> _patterns;
    int _positions[128];
    String _modFile;
};
