#include "alfe/main.h"
#include "alfe/vectors.h"
#include "alfe/rational.h"
#include "alfe/hash_table.h"

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
        _modFile = File(_arguments[1]).contents();
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
        if (type == 0x4d2e4b2e)
            nSamples = 31;
        int offset = 0;
        for (int i = 0; i < nSamples; ++i) {
            Sample sample;
            int p = i*30 + 20;
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
        }
        int nPositions = 
            

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
    Sample _samples[32];
    String _modFile;
};
