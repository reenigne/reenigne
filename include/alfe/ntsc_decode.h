#ifndef INCLUDED_NTSC_DECODE_H
#define INCLUDED_NTSC_DECODE_H

#include "alfe/main.h"
#include "alfe/bitmap.h"
#include "alfe/complex.h"
#include "alfe/fft.h"
#include "alfe/image_filter.h"
#include "alfe/colour_space.h"

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

// A non-resampling decoder optimized to decode a large chunk of samples at
// once, using FFTs.
class NTSCDecoder
{
public:
    NTSCDecoder(int length = 512, int outputLength = 448)
      : _rigor(FFTW_EXHAUSTIVE)
    {
        setLength(length, outputLength);
    }

    void setLength(int length, int outputLength)
    {
        _length = length;
        _outputLength = outputLength;
        _iTime.ensure(length);
        _qTime.ensure(length);
        _yTime.ensure(length);
        int fLength = length/2 + 1;
        _frequency.ensure(fLength);
        _lumaForward =
            FFTWPlanDFTR2C1D<float>(length, _yTime, _frequency, _rigor);
        _chromaForward =
            FFTWPlanDFTR2C1D<float>(length / 2, _yTime, _frequency, _rigor);
        _backward =
            FFTWPlanDFTC2R1D<float>(length, _frequency, _yTime, _rigor);
        _yResponse.ensure(fLength);
        _iResponse.ensure(fLength);
        _qResponse.ensure(fLength);
    }
    void calculateBurst(Byte* burst)
    {
        Complex<float> iq;
        iq.x = static_cast<float>(burst[0] - burst[2]);
        iq.y = static_cast<float>(burst[1] - burst[3]);
        _chromaScale = 2/static_cast<float>(_length);
        Complex<float> iqAdjust =
            -iq.conjugate()*unit((33 + 90 + _hue)/360.0f)*_saturation*
            _contrast*_chromaScale/(iq.modulus()*2);
        float contrast = _contrast/_length;
        _brightness2 = _brightness*256.0f;
        int fLength = _length/2 + 1;
        float lumaHigh = _lumaBandwidth / 2;
        float chromaLow = (4 - _chromaBandwidth) / 8;
        float chromaHigh = (4 + _chromaBandwidth) / 8;
        float rollOff = _rollOff / 4;
        float chromaCutoff = _chromaBandwidth / 8;

        float pi = static_cast<float>(tau/2);
        float width = _lobes*4;
        float lumaScale = (pi*lumaHigh)/(2*sinint(pi*lumaHigh*width));
        if (lumaHigh == 0)
            lumaScale = 0;
        float chromaScale = (pi*chromaCutoff)/
            (2*sinint(pi*chromaCutoff*width));
        if (chromaCutoff == 0)
            chromaScale = 0;

        float lumaTotal = 0;
        for (int t = 0; t < fLength; ++t) {
            float d = static_cast<float>(t);
            float r = sinc(d*rollOff)*lumaScale*sinc(d*lumaHigh);
            if (t > width)
                r = 0;
            _yTime[t] = r;
            if (t > 0)
                _yTime[_length - t] = r;
            lumaTotal += r*(t == 0 ? 1 : 2);
        }
        float scale = 1/lumaTotal;
        _lumaForward.execute(_yTime, _frequency);
        for (int f = 0; f < fLength; ++f)
            _yResponse[f] = _frequency[f].x*contrast*scale;

        float chromaTotal = 0;
        for (int t = 0; t < fLength; ++t) {
            float d = static_cast<float>(t);
            float r = sinc(d*rollOff)*chromaScale*sinc(d*chromaCutoff);
            if (t > width)
                r = 0;
            _yTime[t] = r;
            if (t > 0)
                _yTime[_length - t] = r;
            chromaTotal += r*(t == 0 ? 1 : 2);
        }
        scale = 1/chromaTotal;
        _lumaForward.execute(_yTime, _frequency);
        for (int f = 0; f < fLength; ++f) {
            float s = scale * _frequency[f].x;
            _iResponse[f] = s * unit(-f/512.0f);
            _qResponse[f] = s;
        }

        _ri =  0.9563f*iqAdjust.x +0.6210f*iqAdjust.y;
        _rq =  0.6210f*iqAdjust.x -0.9563f*iqAdjust.y;
        _gi = -0.2721f*iqAdjust.x -0.6474f*iqAdjust.y;
        _gq = -0.6474f*iqAdjust.x +0.2721f*iqAdjust.y;
        _bi = -1.1069f*iqAdjust.x +1.7046f*iqAdjust.y;
        _bq =  1.7046f*iqAdjust.x +1.1069f*iqAdjust.y;
    }

    void decodeBlock(SRGB* srgb)
    {
        int fLength = _length/2 + 1;

        // Filter I
        _chromaForward.execute(_iTime, _frequency);
        for (int f = 0; f < fLength/2; ++f) {
            _frequency[f + fLength/2 + 1] =
                _frequency[fLength/2 - 1 - f].conjugate();
        }
        for (int f = 0; f < fLength; ++f)
            _frequency[f] *= _iResponse[f];
        _backward.execute(_frequency, _iTime);

        // Filter Q
        _chromaForward.execute(_qTime, _frequency);
        for (int f = 0; f < fLength/2; ++f) {
            _frequency[f + fLength/2 + 1] =
                _frequency[fLength/2 - 1 - f].conjugate();
        }
        for (int f = 0; f < fLength; ++f)
            _frequency[f] *= _qResponse[f];
        _backward.execute(_frequency, _qTime);

        // Remove remodulated IQ from Y
        for (int t = 0; t < _length; t += 4) {
            _yTime[t] -= _qTime[t]*_chromaScale;
            _yTime[t + 1] += _iTime[t + 1]*_chromaScale;
            _yTime[t + 2] += _qTime[t + 2]*_chromaScale;
            _yTime[t + 3] -= _iTime[t + 3]*_chromaScale;
        }

        // Filter Y
        _lumaForward.execute(_yTime, _frequency);
        for (int f = 0; f < fLength; ++f)
            _frequency[f] *= _yResponse[f];
        _backward.execute(_frequency, _yTime);

        for (int t = _padding; t < _padding + _outputLength; ++t) {
            float y = _yTime[t] + _brightness2;
            Complex<float> iq(_iTime[t], _qTime[t]);
            *srgb = SRGB(byteClamp(y + _ri*iq.x + _rq*iq.y),
                byteClamp(y + _gi*iq.x + _gq*iq.y),
                byteClamp(y + _bi*iq.x + _bq*iq.y));
            ++srgb;
        }
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
    double getRollOff() { return _rollOff; }
    void setRollOff(double rollOff) { _rollOff = static_cast<float>(rollOff); }
    double getLobes() { return _lobes; }
    void setLobes(double lobes) { _lobes = static_cast<float>(lobes); }
    void setInputScaling(int scaling) { }
    void setPadding(int padding) { _padding = padding; }
    float* yData() { return &_yTime[0]; }
    float* iData() { return &_iTime[0]; }
    float* qData() { return &_qTime[0]; }

    void decodeNTSC(Byte* ntsc, SRGB* srgb)
    {
        float* yData = &_yTime[0];
        float* iData = &_iTime[0];
        float* qData = &_qTime[0];
        for (int x = 0; x < _length; x += 4) {
            yData[x] = ntsc[0];
            yData[x + 1] = ntsc[1];
            yData[x + 2] = ntsc[2];
            yData[x + 3] = ntsc[3];
            iData[0] = -static_cast<float>(ntsc[1]);
            iData[1] = ntsc[3];
            qData[0] = ntsc[0];
            qData[1] = -static_cast<float>(ntsc[2]);
            ntsc += 4;
            iData += 2;
            qData += 2;
        }
        decodeBlock(srgb);
    }
private:
    float _hue;
    float _saturation;
    float _contrast;
    float _brightness;
    float _chromaBandwidth;
    float _lumaBandwidth;
    float _rollOff;
    float _lobes;
    float _chromaScale;

    float _ri;
    float _gi;
    float _bi;
    float _rq;
    float _gq;
    float _bq;
    float _brightness2;
    int _padding;

    FFTWComplexArray<float> _frequency;
    Array<float> _yResponse;
    Array<Complex<float>> _iResponse;
    Array<float> _qResponse;
    FFTWRealArray<float> _yTime;
    FFTWRealArray<float> _iTime;
    FFTWRealArray<float> _qTime;
    FFTWPlanDFTR2C1D<float> _lumaForward;
    FFTWPlanDFTR2C1D<float> _chromaForward;
    FFTWPlanDFTC2R1D<float> _backward;

    int _rigor;
    int _length;
    int _outputLength;
};

// A non-resampling decoder optimized to decode a small number of samples at
// once, using FIR filters.
class MatchingNTSCDecoder
{
public:
    void setLength(int outputLength)
    {
        _outputLength = outputLength;
    }
    void calculateBurst(Byte* burst, Byte* active = 0)
    {
        Complex<float> iq;
        iq.x = static_cast<float>(burst[0] - burst[2]);
        iq.y = static_cast<float>(burst[1] - burst[3]);
        _iqAdjust = -iq.conjugate()*unit((33 + 90 + _hue)/360.0f)*_saturation*
            _contrast/iq.modulus();
// define to 1 to use the floating-point filter, 0 for integer
#define FIR_FP 0
#if FIR_FP
        _brightness2 = _brightness*256.0f;
#else
        _brightness2 = static_cast<int>(_brightness*256.0f +
            128*_inputScaling*_contrast);
#endif
        float lumaHigh = _lumaBandwidth/2;
        float chromaBandwidth = _chromaBandwidth / 8;
        float chromaLow = (4 - _chromaBandwidth) / 8;
        float chromaHigh = (4 + _chromaBandwidth)  / 8;
        float rollOff = _rollOff / 4;
        float width = _lobes*4;
        int right = static_cast<int>(width);
        int left = -right;

#if FIR_FP
        _output.ensure(_outputLength*3*sizeof(float), 1);
#else
        _output.ensure(_outputLength*3*sizeof(UInt16), 1);
#endif

        int n = 1 + right - left;
        _lumaKernel.ensure(n);
        _chromaKernel.ensure(n);
        _diffKernel.ensure(n);
        float pi = static_cast<float>(tau/2);
        float lumaTotal = 0;
        float chromaTotal = 0;
        for (int i = 0; i < n; ++i) {
            int ii = i + left;
            float i1 = static_cast<float>(ii);
            float r = sinc(i1*rollOff);
            float l = r*lumaHigh*sinc(i1*lumaHigh);
            float c = r*chromaBandwidth*sinc(i1*chromaBandwidth);
            float diff;
            if (lumaHigh > chromaHigh) {
                diff = r*(chromaHigh*sinc(i1*chromaHigh) -
                    chromaLow*sinc(i1*chromaLow));
            }
            else {
                if (lumaHigh > chromaLow) {
                    diff = r*(lumaHigh*sinc(i1*lumaHigh) -
                        chromaLow*sinc(i1*chromaLow));
                }
                else
                    diff = 0;
            }

            lumaTotal += l;
            _lumaKernel[i] = l;
            chromaTotal += c;
            _chromaKernel[i] = c;
            _diffKernel[i] = diff;
        }
        if (lumaTotal == 0)
            lumaTotal = 1;
        if (chromaTotal == 0)
            chromaTotal = 1;
        for (int i = 0; i < n; ++i) {
            _lumaKernel[i] /= lumaTotal;
            _chromaKernel[i] /= chromaTotal;
        }

        static const float channelPositions[3] = {0, 0, 0};

        _filter.generate(Vector2<int>(_outputLength, 1), 2,
            channelPositions, 3, channelPositions, width,
            [=](float distance, int inputChannel, int outputChannel)
        {
            int d = static_cast<int>(distance) - left;
            float r;
            if ((inputChannel & 1) == 0) {
                // Luma
                r = _contrast*_lumaKernel[d];
            }
            else {
                // Chroma
                Complex<float> iq = 0;
                switch (inputChannel & 6) {
                    case 0: iq.y = 1; break;
                    case 2: iq.x = -1; break;
                    case 4: iq.y = -1; break;
                    case 6: iq.x = 1; break;
                }
                iq *= _iqAdjust;
                static const float i[3] = {0.9563f, -0.2721f, -1.1069f};
                static const float q[3] = {0.6210f, -0.6474f, 1.7046f};

                r = (i[outputChannel]*iq.x + q[outputChannel]*iq.y)*
                    _chromaKernel[d] - _contrast*_diffKernel[d];
            }
            if (active != 0 && active[inputChannel >> 1] == 0)
                r = 0;
            return Tuple<float, float>(r, distance == 0 ? 1.0f : 0.0f);
        },
            &_inputLeft, &_inputRight, 1, 0);

#if FIR_FP
        _input.ensure((_inputRight - _inputLeft)*8*sizeof(float), 1);
#else
        _input.ensure((_inputRight - _inputLeft)*8*sizeof(UInt16), 1);
#endif

        _filter.setBuffers(_input, _output);
    }

    void decodeBlock(SRGB* srgb)
    {
#if FIR_FP
        _filter.execute();
        Colour* output = reinterpret_cast<Colour*>(_output.data());
        for (int i = 0; i < _outputLength; ++i) {
            Colour c = output[i] +
                Colour(_brightness2, _brightness2, _brightness2);
            srgb[i] = SRGB(byteClamp(c.x), byteClamp(c.y), byteClamp(c.z));
        }
#else
        _filter.execute();
        SInt16* output = reinterpret_cast<SInt16*>(_output.data());
        static const int shift = 6;
        int bias = (_brightness2 << shift) + (1 << (shift - 1));
        for (int i = 0; i < _outputLength; ++i) {
            SInt16 r = (output[0] + bias) >> shift;
            SInt16 g = (output[1] + bias) >> shift;
            SInt16 b = (output[2] + bias) >> shift;
            output += 3;
            srgb[i] = SRGB(byteClamp(r), byteClamp(g), byteClamp(b));
        }
#endif
    }

    void decodeNTSC(const Byte* ntsc, SRGB* srgb)
    {
        auto input = inputData();
#if FIR_FP
        for (int i = 0; i < (_inputRight - _inputLeft)*4; ++i)
            input[i] = ntsc[i];
#else
        for (int i = 0; i < (_inputRight - _inputLeft)*8; ++i)
            input[i] = ntsc[i];
#endif
        decodeBlock(srgb);
    }

    void encodeNTSC(const Colour* input, Byte* output, int n,
        const Linearizer* linearizer, int phase)
    {
        Complex<float> iqAdjust = Complex<float>(1)/_iqAdjust;
        float contrast = 1/_contrast;
        for (int i = 0; i < n; ++i) {
            SRGB srgb = linearizer->srgb(*input);
            Complex<float> iq;
            float y = 0.299f*srgb.x + 0.587f*srgb.y + 0.114f*srgb.z;
            iq.x = 0.596f*srgb.x - 0.275f*srgb.y - 0.321f*srgb.z;
            iq.y = 0.212f*srgb.x - 0.528f*srgb.y + 0.311f*srgb.z;
            iq *= iqAdjust;
            y = (y - _brightness2)*contrast;
            switch (phase) {
                case 0:
                    *output = byteClamp(y + iq.y);
                    break;
                case 1:
                    *output = byteClamp(y - iq.x);
                    break;
                case 2:
                    *output = byteClamp(y - iq.y);
                    break;
                case 3:
                    *output = byteClamp(y + iq.x);
                    break;
            }
            ++output;
            phase = (phase + 1) & 3;
        }
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
    double getRollOff() { return _rollOff; }
    void setRollOff(double rollOff) { _rollOff = static_cast<float>(rollOff); }
    double getLobes() { return _lobes; }
    void setLobes(double lobes) { _lobes = static_cast<float>(lobes); }
    void setInputScaling(int scaling) { _inputScaling = scaling; }

    int inputLeft() { return _inputLeft; }
    int inputRight() { return _inputRight; }
#if FIR_FP
    float* inputData() { return reinterpret_cast<float*>(_input.data()); }
#else
    UInt16* inputData() { return reinterpret_cast<UInt16*>(_input.data()); }
#endif
private:
    float _hue;
    float _saturation;
    float _contrast;
    float _brightness;
    float _chromaBandwidth;
    float _lumaBandwidth;
    float _rollOff;
    float _lobes;

#if FIR_FP
    ImageFilterHorizontal _filter;
#else
    ImageFilter16 _filter;
#endif
    AlignedBuffer _input;
    AlignedBuffer _output;
    int _outputLength;
    int _inputLeft;
    int _inputRight;
#if FIR_FP
    float _brightness2;
#else
    int _brightness2;
#endif
    Complex<float> _iqAdjust;
    int _inputScaling;
    Array<float> _lumaKernel;
    Array<float> _chromaKernel;
    Array<float> _diffKernel;
};

#endif // INCLUDED_NTSC_DECODE_H
