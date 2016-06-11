#include "alfe/main.h"
#include "alfe/bitmap.h"
#include "alfe/complex.h"
#include "alfe/fft.h"

#ifndef INCLUDED_NTSC_DECODE_H
#define INCLUDED_NTSC_DECODE_H

#include "alfe/image_filter.h"

float sinc(float z)
{
    if (z == 0.0f)
        return 1.0f;
    z *= static_cast<float>(M_PI);
    return sin(z)/z;
}

static const int lobes = 3;

float lanczos(float z)
{
    return sinc(z)*sinc(z/lobes);
}

template<class T> Byte checkClamp(T x)
{
    return byteClamp(x);
//    return x;
}

Complex<float> rotor(float phase)
{
    float angle = static_cast<float>(phase*tau);
    return Complex<float>(cos(angle), sin(angle));
}

template<class T> class NTSCCaptureDecoder
{
public:
    NTSCCaptureDecoder()
    {
        _contrast = 2.3;
        _brightness = -33.0;
        _saturation = 0.34;
        _hue = 0;
        _outputPixelsPerLine = 760;
        _yScale = 1;
        _doDecode = true;
        _chromaSamples = 8;

        for (int i = 8; i < 40; ++i)
            _burstWeights[i] = 1;
        for (int i = 0; i < 8; ++i) {
            _burstWeights[i] = 1 - (cos(tau*i/16) + 1)/2;
            _burstWeights[i + 40] = (cos(tau*i/16) + 1)/2;
        }
    }
    void setOutputPixelsPerLine(int outputPixelsPerLine)
    {
        // outputPixelsPerLine  active width  with 20% overscan  per color carrier cycle  active scanlines  with 20% overscan
        //  380                  266+2/3       320                1+2/3                    200               240
        //  456                  320           384                2                        240               288
        //  570                  400           480                2.5                      300               360
        //  760                  533+1/3       640                3+1/3                    400               480
        //  912                  640           768                4                        480               576
        // 1140                  800           960                5                        600               720
        _outputPixelsPerLine = outputPixelsPerLine;
    }
    void setInputBuffer(Byte* input) { _input = input; }
    void setOutputBuffer(Bitmap<T> output)  { _output = output; }
    void setContrast(float contrast) { _contrast = contrast; }
    void setBrightness(float brightness) { _brightness = brightness; }
    void setSaturation(float saturation) { _saturation = saturation; }
    void setHue(float hue) { _hue = hue; }
    void setYScale(int yscale) { _yScale = yscale; }
    void setDoDecode(bool doDecode) { _doDecode = doDecode; }
    void setChromaSamples(float samples) { _chromaSamples = samples; }

    void decode()
    {
        if (!_doDecode) {
            outputRaw();
            return;
        }
        // Settings

        static const int firstScanline = 0;  // 9
        static const int lines = 240 + firstScanline;
        static const int nominalSamplesPerLine = 1820;
        static const int firstSyncSample = -40;  // Assumed position of previous hsync before our samples started (was -130)
        static const float kernelSize = lobes;  // Lanczos parameter
        static const int nominalSamplesPerCycle = 8;
        static const int driftSamples = 40;
        static const int burstSamples = 48;  // Central 6 of 8-10 cycles
        static const int firstBurstSample = 32 + driftSamples;      // == 72
        static const int burstCenter = firstBurstSample + burstSamples/2;

        Byte* b = _input;


        // Pass 1 - find sync and burst pulses, compute wobble amplitude and phase

        float deltaSamplesPerCycle = 0;

        int syncPositions[lines + 1];
        int fracSyncPositions[lines + 1];
        int oldP = firstSyncSample - driftSamples;                  // == -80
        int p = oldP + nominalSamplesPerLine;
        float samplesPerLine = nominalSamplesPerLine;
        Complex<float> bursts[lines + 1];
        float burstDCs[lines + 1];
        Complex<float> hueRotor = rotor((33 + _hue)/360);
        float burstDCAverage = 0;
        float x = 0;
        for (int line = 0; line < lines + 1; ++line) {
            Complex<float> burst = 0;
            float burstDC = 0;
            float t = 0;
            for (int i = firstBurstSample; i < firstBurstSample + burstSamples; ++i) {
                int j = oldP + i;                                   // == -8
                int sample = b[j];
                float phase = (j&7)/8.0f;
                float w = 1; //_burstWeights[ i - firstBurstSample];
                burst += rotor(phase)*sample*w;
                t += w;
                burstDC += sample;
            }

            float burstAmplitude = burst.modulus()/burstSamples;
            bursts[line] = burst*hueRotor/burstSamples;
            burstDC /= t;
            burstDCs[line] = burstDC;

            syncPositions[line] = p;
            fracSyncPositions[line] = x;
            oldP = p;
            int i;
            for (i = 0; i < driftSamples*2; ++i) {
                if (b[p] < 9)
                    break;
                ++p;
            }
            for (; i < driftSamples*2; ++i) {
                if (b[p] >= 12)
                    break;
                ++p;
            }
            // b[p] >= 12
            // b[p - 1] < 12
            // b[p - 1]*x + b[p]*(1 - x) == 12
            // x == (b[p] - 12)/(b[p] - b[p-1])
            //   12 position is b[p - x]
            int d = b[p] - b[p - 1];
            if (d == 0)
                x = 0;
            else
                x = (b[p] - 12)/d;

            p += nominalSamplesPerLine - driftSamples;

            if (line < 200) {
                samplesPerLine = (2*samplesPerLine + p - oldP)/3;
                burstDCAverage = (2*burstDCAverage + burstDC)/3;
            }
        }

        float deltaSamplesPerLine = samplesPerLine - nominalSamplesPerLine;


        // Pass 2 - render

        Byte* outputRow = _output.data();

        float q = syncPositions[1] - samplesPerLine;
        syncPositions[0] = q;
        Complex<float> burst = bursts[0];
        float rotorTable[8];
        for (int i = 0; i < 8; ++i)
            rotorTable[i] = rotor(i/8.0).x;
        Complex<float> expectedBurst = burst;
        int oldActualSamplesPerLine = nominalSamplesPerLine;
        float contrast1 = _contrast;
        float saturation1 = _saturation*100;
        for (int line = 0; line < lines; ++line) {
            // Determine the phase, amplitude and DC offset of the color signal
            // from the color burst, which starts shortly after the horizontal
            // sync pulse ends. The color burst is 9 cycles long, and we look
            // at the middle 5 cycles.

            Complex<float> actualBurst = bursts[line];
            burst = (expectedBurst*2 + actualBurst)/3;
            //burst = actualBurst;

            float phaseDifference = (actualBurst*(expectedBurst.conjugate())).argument()/tau;
            float adjust = -phaseDifference/_outputPixelsPerLine;

            float bm2 = burst.modulus2();
            Complex<float> chromaAdjust;
            // TODO: Implement proper colour-killer logic (100 scanlines hysterisis?)
            if (bm2 < 50)
                chromaAdjust = 0;
            else
                chromaAdjust = burst.conjugate()*contrast1*saturation1 / bm2;
            burstDCAverage = (2*burstDCAverage + burstDCs[line])/3;
            float brightness1 = _brightness + 65 - burstDCAverage;

            // Resample the image data

            T* output = reinterpret_cast<T*>(outputRow);
            int xStart = 65*_outputPixelsPerLine/760;
            for (int x = xStart; x < xStart + _output.size().x; ++x) {
                float y = 0;
                Complex<float> c = 0;
                float t = 0;

                float kFrac0 = x*samplesPerLine/_outputPixelsPerLine;
                float kFrac = q + kFrac0;
                int k = static_cast<int>(kFrac);
                kFrac -= k;
                float samplesPerCycle = nominalSamplesPerCycle + deltaSamplesPerCycle;
                float z0 = -kFrac/_chromaSamples;
                int firstInput = -kernelSize*_chromaSamples + kFrac;
                int lastInput = kernelSize*_chromaSamples + kFrac;

                for (int j = firstInput; j <= lastInput; ++j) {
                    // The input sample corresponding to the output pixel is k+kFrac
                    // The sample we're looking at in this iteration is j+k
                    // The difference is j-kFrac
                    // So the value we pass to lanczos() is (j-kFrac)/_chromaSamples
                    // So z0 = -kFrac/_chromaSamples;

                    float s = lanczos(j/_chromaSamples + z0);
                    int i = j + k;
                    float z = s*b[i];
                    c.x += rotorTable[i & 7]*z;
                    c.y += rotorTable[(i + 6) & 7]*z;
                    t += s;
                }
                c /= t;

                //float lumaSamples = samplesPerLine/_outputPixelsPerLine;
                float lumaSamples = samplesPerCycle/2;  // i.e. 7.16MHz
                firstInput = -kernelSize*lumaSamples + kFrac;
                lastInput = kernelSize*lumaSamples + kFrac;

                Complex<float> cc = c*2;

                t = 0;
                z0 = -kFrac/lumaSamples;
                for (int j = firstInput; j <= lastInput; ++j) {
                    // The input sample corresponding to the output pixel is k+kFrac
                    // The sample we're looking at in this iteration is j+k
                    // The difference is j-kFrac
                    // So the value we pass to lanczos() is (j-kFrac)/lumaSamples
                    // So z0 = -kFrac/lumaSamples;

                    float s = lanczos(j/lumaSamples + z0);
                    int i = j + k;
                    float z = s*(b[i] - (cc.x*rotorTable[i & 7] + cc.y*rotorTable[(i + 6)&7]));
                    y += z;
                    t += s;
                }

                y = y*contrast1/t + brightness1;
                c = c*chromaAdjust*rotor((x - burstCenter*_outputPixelsPerLine/samplesPerLine)*adjust);

                setOutput(output, SRGB(
                    checkClamp(y + 0.9563*c.x + 0.6210*c.y),
                    checkClamp(y - 0.2721*c.x - 0.6474*c.y),
                    checkClamp(y - 1.1069*c.x + 1.7046*c.y)));
                ++output;
            }

            int p = syncPositions[line + 1];
            int actualSamplesPerLine = p - syncPositions[line];
            samplesPerLine = (2*samplesPerLine + actualSamplesPerLine + fracSyncPositions[line] - fracSyncPositions[line + 1])/3;
            q += samplesPerLine;
            q = (10*q + p)/11;

            expectedBurst = actualBurst;

            if (line >= firstScanline) {
                Byte* outputRow2 = outputRow + _output.stride();
                for (int yy = 1; yy < _yScale; ++yy) {
                    T* output = reinterpret_cast<T*>(outputRow2);
                    T* input = reinterpret_cast<T*>(outputRow);
                    for (int x = 0; x < _output.size().x; ++x) {
                        *output = *input;
                        ++output;
                        ++input;
                    }
                    outputRow2 += _output.stride();
                }
                outputRow = outputRow2;
            }
        }
    }
private:
    void outputRaw()
    {
        Byte* outputRow = _output.data();
        Byte* b = _input;
        for (int y = 0; y < 252; ++y) {
            T* output = reinterpret_cast<T*>(outputRow);
            for (int x = 0; x < 1824; ++x) {
                setOutput(output, SRGB(*b, *b, *b));
                ++output;
                ++b;
            }
            outputRow += _output.stride();
        }
        T* output = reinterpret_cast<T*>(outputRow);
        for (int x = 0; x < 450*1024-252*1824; ++x) {
            setOutput(output, SRGB(*b, *b, *b));
            ++output;
            ++b;
        }
    }


    void setOutput(SRGB* output, SRGB x) { *output = x; }
    void setOutput(UInt32* output, SRGB x)
    {
        *output = (x.x << 16) | (x.y << 8) | x.z;
    }
    void setOutput(DWORD* output, SRGB x)
    {
        *output = (x.x << 16) | (x.y << 8) | x.z;
    }

    int _outputPixelsPerLine;
    float _contrast;
    float _brightness;
    float _saturation;
    float _hue;
    Byte* _input;
    Bitmap<T> _output;
    int _yScale;
    bool _doDecode;
    float _chromaSamples;
    float _burstWeights[48];
};

// This is a simpler decoder for use with sources that generate exactly 4
// samples per colour carrier cycle, i.e. emulators such as CGAComposite.
class NTSCDecoder
{
public:
    void calculateBurst(Byte* burst)
    {
        Complex<double> iq;
        iq.x = burst[0] - burst[2];
        iq.y = burst[1] - burst[3];
        _iqAdjust =
            -iq.conjugate()*unit((33 + 90 + _hue)/360.0)*_saturation*_contrast/
            (iq.modulus()*16);
        _contrast2 = _contrast/32;
        _brightness2 = _brightness*256.0;
    }
    Colour decode(int* s)
    {
        int dc = (s[0] + s[1] + s[2] + s[3])*8;
        Complex<int> iq;
        iq.x = (s[0] - s[2])*8;
        iq.y = (s[1] - s[3])*8;
        return decode(dc, iq);
    }
    Colour decode(const Byte* n, int phase)
    {
        // Filter kernel must be divisible by (1,1,1,1) so that all phases
        // contribute equally.
        int y = n[0] +n[1]*4 +n[2]*7 +n[3]*8 +n[4]*7 +n[5]*4 +n[6];
        Complex<int> iq;
        switch (phase) {
        case 0:
            iq.x =  n[0]   -n[2]*7 +n[4]*7 -n[6];
            iq.y =  n[1]*4 -n[3]*8 +n[5]*4;
            break;
        case 1:
            iq.x = -n[1]*4 +n[3]*8 -n[5]*4;
            iq.y =  n[0]   -n[2]*7 +n[4]*7 -n[6];
            break;
        case 2:
            iq.x = -n[0]   +n[2]*7 -n[4]*7 +n[6];
            iq.y = -n[1]*4 +n[3]*8 -n[5]*4;
            break;
        case 3:
            iq.x = +n[1]*4 -n[3]*8 +n[5]*4;
            iq.y = -n[0]   +n[2]*7 -n[4]*7 +n[6];
            break;
        }
        return decode(y, iq);
    }
    void decodeLine(const Byte* ntsc, SRGB* srgb, int length, int phase)
    {
        for (int x = 0; x < length; ++x) {
            phase = (phase + 1) & 3;
            Colour s = decode(ntsc, phase);
            ++ntsc;
            *srgb = SRGB(byteClamp(s.x), byteClamp(s.y), byteClamp(s.z));
            ++srgb;
        }
    }
    void encodeLine(Byte* ntsc, const SRGB* srgb, int length, int phase)
    {
        phase = (phase + 3) & 3;
        for (int x = 0; x < length; ++x) {
            Vector3<int> mix = Vector3Cast<int>(srgb[0]) +
                4*Vector3Cast<int>(srgb[1]) + 7*Vector3Cast<int>(srgb[2]) +
                8*Vector3Cast<int>(srgb[3]) + 7*Vector3Cast<int>(srgb[4]) +
                4*Vector3Cast<int>(srgb[5]) + Vector3Cast<int>(srgb[6]);
            ++srgb;
            Colour c;
            if (_ntscPrimaries) {
                c.x = (0.6689*mix.x + 0.2679*mix.y + 0.0323*mix.z);
                c.y = (0.0185*mix.x + 1.0743*mix.y - 0.0603*mix.z);
                c.z = (0.0162*mix.x + 0.0431*mix.y + 0.8551*mix.z);
            }
            else
                c = Colour(mix.x, mix.y, mix.z);
            Complex<double> iq;
            double y = 0.299*c.x + 0.587*c.y + 0.114*c.z;
            iq.x = 0.596*c.x - 0.275*c.y - 0.321*c.z;
            iq.y = 0.212*c.x - 0.528*c.y + 0.311*c.z;
            iq /= (_iqAdjust*512);
            y = (y/32 - _brightness2)/(_contrast2*32);
            switch (phase) {
                case 0:
                    *ntsc = byteClamp(y + iq.x);
                    break;
                case 1:
                    *ntsc = byteClamp(y + iq.y);
                    break;
                case 2:
                    *ntsc = byteClamp(y - iq.x);
                    break;
                case 3:
                    *ntsc = byteClamp(y - iq.y);
                    break;
            }
            ++ntsc;
            phase = (phase + 1) & 3;
        }
    }

    bool getNTSCPrimaries() { return _ntscPrimaries; }
    void setNTSCPrimaries(bool ntscPrimaries)
    {
        _ntscPrimaries = ntscPrimaries;
    }
    double getHue() { return _hue; }
    void setHue(double hue) { _hue = hue; }
    double getSaturation() { return _saturation; }
    void setSaturation(double saturation) { _saturation = saturation; }
    double getContrast() { return _contrast; }
    void setContrast(double contrast)
    {
        _contrast = contrast;
    }
    double getBrightness() { return _brightness; }
    void setBrightness(double brightness) { _brightness = brightness; }
    double getSharpness() { return _sharpness; }
    void setSharpness(double sharpness) { _sharpness = sharpness; }

private:
    Colour decode(int y, Complex<int> iq)
    {
        double y2 = y*_contrast2 + _brightness2;
        Complex<double> iq2 = Complex<double>(iq)*_iqAdjust;
        double r = y2 + 0.9563*iq2.x + 0.6210*iq2.y;
        double g = y2 - 0.2721*iq2.x - 0.6474*iq2.y;
        double b = y2 - 1.1069*iq2.x + 1.7046*iq2.y;
        if (_ntscPrimaries) {
            return Colour(
                1.5073*r -0.3725*g -0.0832*b,
                -0.0275*r +0.9350*g +0.0670*b,
                -0.0272*r -0.0401*g +1.1677*b);
        }
        return Colour(r, g, b);
    }

    bool _ntscPrimaries;
    double _hue;
    double _saturation;
    double _contrast;
    double _brightness;
    double _sharpness;

    Complex<double> _iqAdjust;
    double _contrast2;
    double _brightness2;
};

// Similar to NTSCDecoder except that the luma and chroma bandwidths can be
// changed.
class ResamplingNTSCDecoder
{
public:
    ResamplingNTSCDecoder() : _rigor(FFTW_MEASURE) { } //EXHAUSTIVE) { }
    void calculateBurst(Byte* burst)
    {
        Complex<float> iq;
        iq.x = static_cast<float>(burst[0] - burst[2]);
        iq.y = static_cast<float>(burst[1] - burst[3]);
        _iqAdjust =
            -iq.conjugate()*unit((33 + 90 + _hue)/360.0f)*_saturation*
            _contrast*2/iq.modulus();
        _contrast2 = _contrast;
        _brightness2 = _brightness*256.0f;
    }

    void init()
    {
        static const float inputChannelPositions[8] =
            {0, 0, 0.25f, 0.25f, 0.5f, 0.5f, 0.75f, 0.75f};
        static const float outputChannelPositions[3] = {0, 0, 0};
        int inputLeft;
        int inputRight;

        Vector outputSize;  // TODO
        float kernelRadius = 16;
        float offset;  // TODO

        _filter.generate(outputSize, 8, inputChannelPositions, 3,
            outputChannelPositions, kernelRadius,
            [&](float distance, int inputChannel, int outputChannel)
            {
                float y = 0;
                Complex<float> iq = 0;
                float chromaLow = (4 - _chromaBandwidth) / 16;
                float chromaHigh = (4 + _chromaBandwidth) / 16;
                if ((inputChannel & 1) == 0) {
                    // Luma
                    float lumaHigh = _lumaBandwidth / 4;
                    if (lumaHigh < chromaHigh)
                        y = sinc(distance*chromaLow);
                    else {
                        y = sinc(distance*lumaHigh) - sinc(distance*chromaHigh)
                            + sinc(distance*chromaLow);
                    }
                }
                else {
                    // Chroma
                    switch (inputChannel & 6) {
                        case 0: iq.y = 1; break;
                        case 2: iq.x = -1; break;
                        case 4: iq.y = -1; break;
                        case 6: iq.x = 1; break;
                    }
                    iq *= sinc(distance*chromaHigh) - sinc(distance*chromaLow);
                }
                y = y*_contrast2 + _brightness2;
                iq *= _iqAdjust;
                switch (outputChannel) {
                    case 0: return y + 0.9563f*iq.x + 0.6210f*iq.y;
                    case 1: return y - 0.2721f*iq.x - 0.6474f*iq.y;
                    case 2: return y - 1.1069f*iq.x + 1.7046f*iq.y;
                }
                assert(false);
                return 0.0f;
            },
            &inputLeft, &inputRight, 1, offset);
        // TODO: deal with inputLeft and inputRight
    }
    void decode()
    {
        _filter.execute();
    }

    void decodeLine(const Byte* ntsc, DWORD* srgb, int inputLength,
        int outputLength)
    {
        static const int padding = 32;
        int inputTimeLength = inputLength + padding*2;
        int inputFrequencyLength = inputTimeLength/2 + 1;
        _frequency.ensure(inputFrequencyLength);
        int outputTimeLength = static_cast<int>(
            static_cast<float>(inputTimeLength)*outputLength/inputLength);
        int outputFrequencyLength = outputTimeLength/2 + 1;
        _frequency.ensure(outputFrequencyLength);
        if (inputTimeLength != _inputTimeLength) {
            _inputTime.ensure(inputTimeLength);
            _forward = FFTWPlanDFTR2C1D<float>(inputTimeLength, _inputTime,
                _frequency, _rigor);
            _inputTimeLength = inputTimeLength;
        }
        if (outputTimeLength != _outputTimeLength) {
            _iTime.ensure(outputTimeLength);
            _qTime.ensure(outputTimeLength);
            _yTime.ensure(outputTimeLength);
            _backward = FFTWPlanDFTC2R1D<float>(outputTimeLength, _frequency,
                _yTime, _rigor);
            _outputTimeLength = outputTimeLength;
        }

        // Pad and transform Y
        for (int t = 0; t < inputLength; ++t)
            _inputTime[t + padding] = ntsc[t];
        padInput(padding, inputLength);
        _forward.execute(_inputTime, _frequency);

        // Filter Y
        int chromaLow = clamp(0,
            static_cast<int>((inputTimeLength*(4 - _chromaBandwidth))/16),
            outputFrequencyLength);
        int chromaHigh = clamp(0,
            static_cast<int>((inputTimeLength*(4 + _chromaBandwidth))/16),
            outputFrequencyLength);
        int lumaHigh = clamp(0,
            static_cast<int>(inputTimeLength*_lumaBandwidth/4),
            outputFrequencyLength);
        if (lumaHigh < chromaLow) {
            for (int f = lumaHigh; f < outputFrequencyLength; ++f)
                _frequency[f] = 0;
        }
        else {
            if (lumaHigh < chromaHigh) {
                for (int f = chromaLow; f < outputFrequencyLength; ++f)
                    _frequency[f] = 0;
            }
            else {
                for (int f = chromaLow; f < chromaHigh; ++f)
                    _frequency[f] = 0;
                for (int f = lumaHigh; f < outputFrequencyLength; ++f)
                    _frequency[f] = 0;
            }
        }
        _backward.execute(_frequency, _yTime);

        // Pad and transform I
        for (int t = 0; t < inputLength; t += 4) {
            _inputTime[t + padding] = 0;
            _inputTime[t + padding + 1] = static_cast<float>(-ntsc[t + 1]);
            _inputTime[t + padding + 2] = 0;
            _inputTime[t + padding + 3] = static_cast<float>(ntsc[t + 3]);
        }
        padInput(padding, inputLength);
        _forward.execute(_inputTime, _frequency);

        // Filter I
        int chromaCutoff = clamp(0,
            static_cast<int>(inputTimeLength*_chromaBandwidth/16),
            outputFrequencyLength);
        for (int f = chromaCutoff; f < outputFrequencyLength; ++f)
            _frequency[f] = 0;
        _backward.execute(_frequency, _iTime);

        // Pad and transform Q

        for (int t = 0; t < inputLength; t += 4) {
            _inputTime[t + padding] = static_cast<float>(ntsc[t]);
            _inputTime[t + padding + 1] = 0;
            _inputTime[t + padding + 2] = static_cast<float>(-ntsc[t + 2]);
            _inputTime[t + padding + 3] = 0;
        }
        padInput(padding, inputLength);
        _forward.execute(_inputTime, _frequency);

        // Filter Q
        for (int f = chromaCutoff; f < outputFrequencyLength; ++f)
            _frequency[f] = 0;
        _backward.execute(_frequency, _qTime);

        int offset = padding*outputTimeLength/inputTimeLength;
        for (int t = 0; t < outputLength; ++t) {
            float y = _yTime[t + offset]/inputTimeLength;
            Complex<float> iq(_iTime[t + offset], _qTime[t + offset]);

            float y2 = y*_contrast2 + _brightness2;
            Complex<float> iq2 = iq*_iqAdjust/
                static_cast<float>(inputTimeLength);
            float r = y2 + 0.9563f*iq2.x + 0.6210f*iq2.y;
            float g = y2 - 0.2721f*iq2.x - 0.6474f*iq2.y;
            float b = y2 - 1.1069f*iq2.x + 1.7046f*iq2.y;
            Vector3<float> c(r, g, b);
            if (_ntscPrimaries) {
                c = Vector3<float>(
                    1.5073f*r -0.3725f*g -0.0832f*b,
                    -0.0275f*r +0.9350f*g +0.0670f*b,
                    -0.0272f*r -0.0401f*g +1.1677f*b);
            }
            if (_showClipping) {
                *srgb = (invertClip(byteClamp(c.x)) << 16) |
                    (invertClip(byteClamp(c.y)) << 8) |
                    invertClip(byteClamp(c.z));
            }
            else {
                *srgb = (byteClamp(c.x) << 16) | (byteClamp(c.y) << 8) |
                    byteClamp(c.z);
            }
            ++srgb;
        }
    }

    bool getNTSCPrimaries() { return _ntscPrimaries; }
    void setNTSCPrimaries(bool ntscPrimaries)
    {
        _ntscPrimaries = ntscPrimaries;
    }
    double getHue() { return _hue; }
    void setHue(double hue) { _hue = static_cast<float>(hue); }
    double getSaturation() { return _saturation; }
    void setSaturation(double saturation)
    {
        _saturation = static_cast<float>(saturation);
    }
    double getContrast() { return _contrast; }
    void setContrast(double contrast)
    {
        _contrast = static_cast<float>(contrast);
    }
    double getBrightness() { return _brightness; }
    void setBrightness(double brightness)
    {
        _brightness = static_cast<float>(brightness);
    }
    bool getShowClipping() { return _showClipping; }
    void setShowClipping(bool showClipping) { _showClipping = showClipping; }
    double getChromaBandwidth() { return _chromaBandwidth; }
    void setChromaBandwidth(double chromaBandwidth)
    {
        _chromaBandwidth = static_cast<float>(chromaBandwidth);
    }
    double getLumaBandwidth() { return _lumaBandwidth; }
    void setLumaBandwidth(double lumaBandwidth)
    {
        _lumaBandwidth = static_cast<float>(lumaBandwidth);
    }

private:
    void padInput(int padding, int inputLength)
    {
        for (int t = 0; t < padding; ++t) {
            _inputTime[t] = _inputTime[padding + ((t - padding) & 3)];
            _inputTime[t + padding + inputLength] =
                _inputTime[padding + (inputLength - 4) + (t & 3)];
        }
    }
    static Byte invertClip(Byte v)
    {
        if (v == 0)
            return 0xff;
        if (v == 0xff)
            return 0;
        return v;
    }

    bool _ntscPrimaries;
    float _hue;
    float _saturation;
    float _contrast;
    float _brightness;
    bool _showClipping;
    float _chromaBandwidth;
    float _lumaBandwidth;

    Complex<float> _iqAdjust;
    float _contrast2;
    float _brightness2;

    int _inputTimeLength;
    FFTWRealArray<float> _inputTime;
    FFTWPlanDFTR2C1D<float> _forward;
    FFTWComplexArray<float> _frequency;
    FFTWPlanDFTC2R1D<float> _backward;
    FFTWRealArray<float> _yTime;
    FFTWRealArray<float> _iTime;
    FFTWRealArray<float> _qTime;
    int _outputTimeLength;

    int _rigor;

    ImageFilter16 _filter;
};

#endif // INCLUDED_NTSC_DECODE_H
