#include "alfe/main.h"
#include "alfe/vectors.h"
#include "alfe/rational.h"
#include "alfe/hash_table.h"

static const int samplesPerFrame = 262;
static const int minSample = 1;
static const int maxSample = 76;
static const bool minimizeClicks = true;

static const int NE = 2;
static const int NQ = 4;
static const int D5 = 26;
static const int C5 = 24;
static const int B4 = 23;
static const int A4 = 21;
static const int G4 = 19;
static const int F4 = 17;
static const int E4 = 16;
static const int D4 = 14;
static const int C4 = 12;
static const int AS3 = 10;
static const int A3 = 9;
static const int F3 = 5;
static const int D3 = 2;

Rational tuning[12] = {
    Rational(9, 10), Rational(15, 16), Rational(1, 1), Rational(16, 15),
    Rational(9, 8), Rational(6, 5), Rational(5, 4), Rational(4, 3),
    Rational(45, 32), Rational(3, 2), Rational(8, 5), Rational(5, 3) };

static const int notes[] = {
    D4, NE, C4, NE,
    D4, NE, A3, NE, F3, NE, A3, NE, D3, NQ, D4, NE, C4, NE,
    D4, NE, A3, NE, F3, NE, A3, NE, D3, NQ, D4, NE, E4, NE,
    F4, NE, E4, NE, F4, NE, D4, NE, E4, NE, D4, NE, E4, NE, C4, NE,
    D4, NE, C4, NE, D4, NE, AS3, NE, D4, NQ, D4, NE, C4, NE,

    D4, NE, A3, NE, F3, NE, A3, NE, D3, NQ, D4, NE, C4, NE,
    D4, NE, A3, NE, F3, NE, A3, NE, D3, NQ, D4, NE, E4, NE,
    F4, NE, E4, NE, F4, NE, D4, NE, E4, NE, D4, NE, E4, NE, C4, NE,
    D4, NE, C4, NE, D4, NE, E4, NE, F4, NQ, A4, NE, G4, NE,

    A4, NE, F4, NE, C4, NE, F4, NE, A3, NQ, A4, NE, G4, NE,
    A4, NE, F4, NE, C4, NE, F4, NE, A3, NQ, A4, NE, B4, NE,
    C5, NE, B4, NE, C5, NE, A4, NE, B4, NE, A4, NE, B4, NE, G4, NE,
    A4, NE, G4, NE, A4, NE, F4, NE, A4, NQ, A4, NE, G4, NE,

    A4, NE, F4, NE, C4, NE, F4, NE, A3, NQ, A4, NE, G4, NE,
    A4, NE, F4, NE, C4, NE, F4, NE, A3, NQ, A4, NE, B4, NE,
    C5, NE, B4, NE, C5, NE, A4, NE, B4, NE, A4, NE, B4, NE, G4, NE,
    A4, NE, G4, NE, A4, NE, F4, NE, A4, NQ, D5, NE, C5, NE,

    A4, NE, F4, NE, C4, NE, F4, NE, A3, NQ, A4, NE, G4, NE,
    A4, NE, F4, NE, C4, NE, F4, NE, A3, NQ, A4, NE, B4, NE,
    C5, NE, B4, NE, C5, NE, A4, NE, B4, NE, A4, NE, B4, NE, G4, NE,
    A4, NE, G4, NE, F4, NE, G4, NE, A4, NQ
};

class NoteDescriptor
{
public:
    NoteDescriptor() { }
    NoteDescriptor(Rational cyclesPerFrame, Rational amplitude)
      : _cyclesPerFrame(cyclesPerFrame), _amplitude(amplitude) { }
    UInt32 hash() const
    {
        return Hash(typeid(NoteDescriptor)).mixin(_cyclesPerFrame.hash()).
            mixin( _amplitude.hash());
    }
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
    NoteData() { }
    NoteData(NoteDescriptor noteDescriptor, int offset)
        : _noteDescriptor(noteDescriptor), _offset(offset) { }
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
        _data.allocate(0x10000);
        _offset = 0;

        FileStream output = File("tables.asm").openWrite();
        //FileStream outputWave = File("popcorn.pcm").openWrite();
        //FileStream outputWave157 = File("popcorn157.pcm").openWrite();
        //output.write("align 16\n\n");
        output.write("sineTable:");
        for (int y = 0; y < 256; ++y) {
            //int x = sampleFromPosition((sin(tau*y / 256)+1)/2);
            int x = clamp(-128, static_cast<int>(128*sin(tau*y / 256)),
                127)&0xff;
            if ((y & 7) == 0)
                output.write("\n  db ");
            else
                output.write(", ");
            output.write(String(hex(x, 2)));
        }

        output.write("\n\nsongTable:");
        int y = 0;
        for (int i = 0; i < sizeof(notes)/sizeof(notes[0]); i += 2) {
            Rational phase = 0;
            int note = notes[i];
            int duration = notes[i+1];
            Rational cyclesPerFrame = tuning[note % 12]*(4 << (note / 12));
            Rational amplitude = 1;
            for (int j = 0; j < duration*5; ++j) {
                NoteDescriptor d(cyclesPerFrame, amplitude);
                if (!_notes.hasKey(d))
                    addWave(d);
                NoteData data = _notes[d];
                int offset = data.offset(phase);

                if ((y & 7) == 0)
                    output.write("\n  dw ");
                else
                    output.write(", ");
                output.write(String(hex(offset, 4)));
                ++y;

                phase = d.endPhase(phase);

                if (j % 5 == 4)
                    amplitude *= Rational(7, 10);

                //for (int sample = 0; sample < samplesPerFrame; ++sample) {
                //    for (int io = 0; io < maxSample; ++io)
                //        if (io < _data[sample + offset])
                //            outputWave.write<Byte>(0xff);
                //        else
                //            outputWave.write<Byte>(0x00);
                //    outputWave157.write<Byte>(clamp(0, static_cast<int>((_data[sample + offset] - minSample)*256.0/(maxSample - minSample)), 0xff));
                //}
            }
            console.write(".");
        }

        output.write("\n\nsongLength equ " + decimal(y) + "\n\n");

        output.write("waveInfoTable:");
        for (int i = 0; i < _descriptors.count(); ++i) {
            NoteDescriptor d = _descriptors[i];
            //NoteData data = _notes[d];
            if ((i % 3) == 0)
                output.write("\n  dw ");
            else
                output.write(", ");
            output.write(String(hex(d.frequency(), 4)) + ", " +
                String(hex(d.samples(), 4)) + ", " +
                String(hex(clamp(0,
                    static_cast<int>((maxSample-minSample)*d.amplitude()),
                    maxSample-minSample), 4)));
        }

        output.write("\n\nwaveInfoLength equ " +
            decimal(_descriptors.count()) + "\n\n");

        console.write("\n" + String(decimal(_offset)) +
            " bytes of wave table used.\n");
    }
private:
    void addWave(NoteDescriptor descriptor)
    {
        _notes[descriptor] = NoteData(descriptor, _offset);
        _descriptors.append(descriptor);
        int samples = descriptor.samples();
        for (int i = 0; i < samples; ++i) {
            double a = i*tau/descriptor.samplesPerCycle().value<double>();
            double v = (descriptor.amplitude()*sin(a)+1.0)/2.0;
            v = (maxSample - minSample)*v + minSample;
            if (_offset == 0x10000) {
                console.write("Too many samples\n");
                return;
            }
            _data[_offset] =
                clamp(minSample, static_cast<int>(v + 0.5), maxSample);
            ++_offset;
        }
        // _offset += descriptor.samples();
    }
    //int sampleFromPosition(double position)
    //{
    //    return clamp(minSample,
    //        static_cast<int>(minSample + (maxSample-minSample)*position),
    //        maxSample);
    //}

    int _offset;
    Array<Byte> _data;
    HashTable<NoteDescriptor, NoteData> _notes;
    AppendableArray<NoteDescriptor> _descriptors;
};
