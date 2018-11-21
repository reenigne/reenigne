#include "alfe/main.h"
#include "alfe/fft.h"
#include <random>

class Component
{
public:
    int _bin;
    Complex<float> _amplitude;
};

static const int componentsPerFrame = 16;

class FrameData
{
public:
    Component _components[componentsPerFrame];
};

class Program : public ProgramBase
{
public:
    void run()
    {
        if (_arguments.count() < 2) {
            console.write("Syntax: " + _arguments[0] +
                " <input file name>\n");
            return;
        }
        File inputFile = File(_arguments[1], true);
        Array<Byte> data;
        inputFile.readIntoArray(&data);
        _wav = &data[0];
        if (!verify()) {
            console.write("Invalid .wav file\n");
            return;
        }
        int nSamples = dLen() / nBlockAlign();

        //int fftSize = 1024;
        int fftSize = 4096;
        FFTWWisdom<float> wisdom(File("wisdom"));
        FFTWRealArray<float> timeDomain(fftSize);
        FFTWComplexArray<float> frequencyDomain(fftSize/2 + 1);
        FFTWPlanDFTR2C1D<float> plan(fftSize, timeDomain, frequencyDomain,
            FFTW_EXHAUSTIVE);
        double sampleOffset = 0;
        double pitRate = 157500000/(11 * 12);
        double blockRate = pitRate / 19912;
        double blockSamples = nSamplesPerSec() / blockRate;
        double binFrequency = nSamplesPerSec() / fftSize;
        double periodBin = pitRate / binFrequency;

        AppendableArray<Word> periods;
        float maxOutput = 0;
        AppendableArray<Complex<float>> fftOutput;
        float maxCorrected = 0;
        AppendableArray<Complex<float>> corrected;
        float maxPower = 0;
        AppendableArray<float> powers;

        std::mt19937 generator;
        std::uniform_real_distribution<double> dist;

        while (sampleOffset <= nSamples) {
            int sample = static_cast<int>(sampleOffset + fftSize/2);
            int i = 0;
            while (sample < 0 && i < fftSize) {
                timeDomain[i] = 0;
                ++i;
                ++sample;
            }
            while (sample < nSamples && i < fftSize) {
                timeDomain[i] = getSample(sample);
                ++i;
                ++sample;
            }
            while (i < fftSize) {
                timeDomain[i] = 0;
                ++i;
            }
            plan.execute();
            float bestFrequency = 0;
            float bestPower = 0;
            float totalPower = 0;

            int cutOffBin = 5000 / periodBin;  // Don't go over 5kHz for now.

            for (int f = 1; f < fftSize/2; ++f) {
                float rawPower = frequencyDomain[f].modulus2() * f * f;
                if (rawPower > bestPower) {
                    bestPower = rawPower;
                    bestFrequency = f;
                }
                //totalPower += power;

                Complex<float> a0 = frequencyDomain[f];
                Complex<float> a1 = frequencyDomain[f + 1];

                float correctedFrequency = ((static_cast<float>(f+1)*a1 - static_cast<float>(f)*a0)/(a1 - a0)).x;
                Complex<float> a = (unit(-correctedFrequency) - 1)/(a0*(f - correctedFrequency));

                float power = a.modulus2() * correctedFrequency * correctedFrequency;
                //if (power > bestPower && f < cutOffBin) {
                //    bestPower = power;
                //    bestFrequency = correctedFrequency;
                //}

                float a0m = a0.modulus();
                fftOutput.append(a0);
                if (a0m > maxOutput)
                    maxOutput = a0m;

                float am = a.modulus();
                corrected.append(a);
                if (am > maxCorrected)
                    maxCorrected = am;

                rawPower = sqrt(rawPower);
                powers.append(rawPower);
                if (rawPower > maxPower)
                    maxPower = rawPower;
            }
            //double r = dist(generator)*totalPower;
            //for (int f = 1; f <= fftSize/2; ++f) {
            //    float power = frequencyDomain[f].modulus2() * f * f;
            //    r -= power;
            //    if (r < 0) {
            //        bestFrequency = f;
            //        break;
            //    }
            //}

            int period = static_cast<int>(periodBin/bestFrequency);
            periods.append(period);

            sampleOffset += blockSamples;
        }
        File("output.pit").openWrite().write(periods);

        AppendableArray<Byte> outputRGB;
        AppendableArray<Byte> correctedRGB;
        AppendableArray<Byte> powerRGB;

        for (int i = 0; i < fftOutput.count(); ++i) {
            appendRGB(&outputRGB, 100.0f*fftOutput[i]/maxOutput);
            appendRGB(&correctedRGB, 100.0f*corrected[i]/maxCorrected);
            appendMagnitude(&powerRGB, 10.0f*powers[i]/maxPower);
        }

        File("output.raw").openWrite().write(outputRGB);
        File("outputC.raw").openWrite().write(correctedRGB);
        File("outputP.raw").openWrite().write(powerRGB);
    }
private:
    UInt16 getWord(int o) { return _wav[o] + (_wav[o + 1] << 8); }
    UInt32 getDWord(int o) { return getWord(o) + (getWord(o + 2) << 16); }
    bool verifyTag(const char* expected, int o)
    {
        for (int i = 0; i < 4; ++i)
            if (expected[i] != _wav[i + o])
                return false;
        return true;
    }
    int nSamplesPerSec() { return getDWord(24); }
    int nBlockAlign() { return getWord(32); }
    int dLen() { return getDWord(40); }
    bool verify()
    {
        if (!verifyTag("RIFF", 0))
            return false;
        if (!verifyTag("WAVE", 8))
            return false;
        if (!verifyTag("fmt ", 12))
            return false;
        if (getDWord(16) != 16)  // Length of "fmt " subchunk
            return false;
        if (getWord(20) != 1)  // wFormatTag
            return false;
        _nChannels = getWord(22);
        if (_nChannels < 1 || _nChannels > 2)
            return false;
        if (getDWord(28) != nSamplesPerSec() * nBlockAlign()) // nAvgBytesPerSec
            return false;
        if (!verifyTag("data", 36))
            return false;
        if (getDWord(4) != dLen() + 36)  // length of "RIFF" chunk
            return false;
        _wBitsPerSample = getWord(34);
        if (nBlockAlign() * 8 != _wBitsPerSample * _nChannels)
            return false;
        if (_wBitsPerSample != 8 && _wBitsPerSample != 16)
            return false;
        if (dLen() % nBlockAlign() != 0)
            return false;
        return true;
    }
    float getSample(int sample)
    {
        if (_nChannels == 1) {
            if (_wBitsPerSample == 8)
                return static_cast<float>((_wav[sample + 44] - 127.5)/127.5);
            return static_cast<float>(
                (static_cast<SInt16>(getWord(sample*2 + 44)) + 0.5)/32767.5);
        }
        if (_wBitsPerSample == 8) {
            return static_cast<float>(
                (_wav[sample*2 + 44] + _wav[sample*2 + 45] - 255)/255.5);
        }
        return static_cast<float>((static_cast<SInt16>(getWord(sample*4 + 44))
            + static_cast<SInt16>(getWord(sample*4 + 46)) + 1)/65535.0);
    }
    float getAmplitudeOfFrequency(int sample, int frequency, int length)
    {
    }
    void appendRGB(AppendableArray<Byte>* image, Complex<float> v)
    {
        image->append(byteClamp(255*(0.5 + 0.9563*v.x + 0.6210*v.y)));
        image->append(byteClamp(255*(0.5 - 0.2721*v.x - 0.6474*v.y)));
        image->append(byteClamp(255*(0.5 - 1.1069*v.x + 1.7046*v.y)));
    }
    void appendMagnitude(AppendableArray<Byte>* image, float v)
    {
        image->append(byteClamp(255*(v)));
        image->append(byteClamp(255*(v - 1)));
        image->append(byteClamp(255*(v - 2)));
    }

    Byte* _wav;
    int _nChannels;
    int _wBitsPerSample;
    int _nSamples;
};