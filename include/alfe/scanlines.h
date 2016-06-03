#include "alfe/main.h"
#include "alfe/bitmap.h"
#include "alfe/complex.h"
#include "alfe/fft.h"

#ifndef INCLUDED_SCANLINES_H
#define INCLUDED_SCANLINES_H

class ScanlineRenderer
{
    static const int padding = 32;
public:
    ScanlineRenderer() : _rigor(FFTW_MEASURE) { }

    void init(int inputLength, int outputLength)
    {
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
            _outputTime.ensure(outputTimeLength);
            _backward = FFTWPlanDFTC2R1D<float>(outputTimeLength, _frequency,
                _outputTime, _rigor);
            _outputTimeLength = outputTimeLength;
        }
        _inputLength = inputLength;
        _outputLength = outputLength;

        float sigma = _width*outputTimeLength/inputTimeLength;
        switch (_profile) {
            case 0:
                // Rectangular
                {
                    float scale = sigma/M_PI;
                    for (int i = 0; i < outputFrequencyLength; ++i) {
                        float f = M_PI*i/sigma;
                        _profileFrequency[i] = scale*sin(f)/i;
                    }
                }
                break;
            case 1:
                // Triangle
                {
                    for (int i = 0; i < outputFrequencyLength; ++i) {
                        float f = M_PI*i/sigma;
                        float s = sin(f)/f;
                        _profileFrequency[i] = s*s;
                    }
                }
                break;
            case 2:
                // Semicircle
                {
                    FFTWRealArray<float> profileTime(outputFrequencyLength);
                    _profileFrequency.ensure(outputTimeLength);
                    FFTWPlanDFTR2C1D<float> transform(outputTimeLength,
                        profileTime, _profileFrequency, _rigor);
                    for (int i = 0; i < outputTimeLength; ++i) {
                        int y = i;
                        if (i > outputTimeLength/2)
                            y = i - outputTimeLength;
                        if (y > sigma)
                            profileTime[i] = 0;
                        else
                            profileTime[i] = sqrt(sigma*sigma - y*y);
                    }
                    transform.execute();
                }
                break;
            case 3:
                // Gaussian
                {
                    float a = 1/(2*sigma*sigma);
                    float scale = sqrt(M_PI/a) / (sigma*sqrt(2*M_PI));
                    a = -M_PI*M_PI/(a * outputTimeLength * outputTimeLength);
                    for (int i = 0; i < outputFrequencyLength; ++i)
                        _profileFrequency[i] = scale*exp(a*i*i);
                }
                break;
        }
        for (int i = 0; i < outputFrequencyLength; ++i)
            _profileFrequency[i] *= unit(i*_offset/outputFrequencyLength);
    }

    void renderColumn(const DWORD* input, int inputStride, DWORD* output,
        int outputStride)
    {
        int inputFrequencyLength = _inputTimeLength/2 + 1;
        int outputFrequencyLength = _outputTimeLength/2 + 1;
        for (int channel = 0; channel < 3; ++channel) {
            int shift = channel * 8;
            for (int i = 0; i < _inputLength; ++i) {
                _inputTime[i + padding] =
                    pow(((input[i] >> shift) & 0xff)/255.0f, 2.2f);
            }
            for (int i = 0; i < padding; ++i) {
                _inputTime[i] = _inputTime[padding];
                _inputTime[padding + _inputLength + i] =
                    _inputTime[padding + _inputLength - 1];
            }
            _forward.execute(_inputTime, _frequency);
            for (int i = inputFrequencyLength; i < outputFrequencyLength;
                ++i) {
                int j = i % _inputTimeLength;
                if (j > inputFrequencyLength) {
                    _frequency[i] =
                        _frequency[_inputTimeLength - j].conjugate();
                }
                else
                    _frequency[i] = _frequency[j];
            }
            _frequency[outputFrequencyLength - 1].y = 0;
            for (int i = 0; i < outputFrequencyLength; ++i)
                _frequency[i] *= _profileFrequency[i];
            _backward.execute();
            for (int i = 0; i < _outputLength; ++i) {
                output[i] = (output[i] & ~(0xff << shift)) | (byteClamp(
                    pow(_outputTime[i + padding], 1/2.2f)*255.0f) << shift);
            }
        }
    }

    int getProfile() { return _profile; }
    void setProfile(int profile) { _profile = profile; }
    float getWidth() { return _width; }
    void setWidth(float width) { _width = width; }
    bool getBleeding() { return _bleeding; }
    void setBleeding(bool bleeding) { _bleeding = bleeding; }
    void setOffset(float offset) { _offset = offset; }
private:
    int _profile;
    float _width;
    bool _bleeding;
    float _offset;
    int _rigor;

    int _inputLength;
    int _outputLength;
    int _inputTimeLength;
    FFTWRealArray<float> _inputTime;
    FFTWPlanDFTR2C1D<float> _forward;
    FFTWComplexArray<float> _frequency;
    FFTWComplexArray<float> _profileFrequency;
    FFTWPlanDFTC2R1D<float> _backward;
    int _outputTimeLength;
    FFTWRealArray<float> _outputTime;
};

#endif // INCLUDED_SCANLINES_H
