#include "alfe/main.h"
#include "alfe/bitmap.h"
#include "alfe/complex.h"
#include "alfe/fft.h"

#ifndef INCLUDED_SCANLINES_H
#define INCLUDED_SCANLINES_H

#include "alfe/image_filter.h"

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

        _profileFrequency.ensure(outputFrequencyLength);
        float sigma = _width/inputTimeLength;
        float scale = 1.0f/inputTimeLength;
        float pi = static_cast<float>(tau/2);
        switch (_profile) {
            case 0:
                // Rectangular
                {
                    scale *= 1/(sigma*pi);
                    for (int i = 1; i < outputFrequencyLength; ++i) {
                        float f = pi*i*sigma;
                        _profileFrequency[i] = sin(f)/i;
                    }
                    _profileFrequency[0] = pi*sigma;
                }
                break;
            case 1:
                // Triangle
                {
                    for (int i = 1; i < outputFrequencyLength; ++i) {
                        float f = pi*i*sigma;
                        float s = sin(f)/f;
                        _profileFrequency[i] = s*s;
                    }
                    _profileFrequency[0] = 1;
                }
                break;
            case 2:
                // Semicircle
                {
                    float w = _width*outputTimeLength/inputTimeLength;
                    float r2 = w*w;
                    scale *= 2/(pi*r2);
                    FFTWRealArray<float> profileTime(outputTimeLength);
                    _profileFrequency.ensure(outputFrequencyLength);
                    FFTWPlanDFTR2C1D<float> transform(outputTimeLength,
                        profileTime, _profileFrequency, _rigor);
                    for (int i = 0; i < outputTimeLength; ++i) {
                        int y = i;
                        if (i > outputTimeLength/2)
                            y = outputTimeLength - i;
                        if (y > w)
                            profileTime[i] = 0;
                        else
                            profileTime[i] = sqrt(r2 - y*y);
                    }
                    transform.execute();
                }
                break;
            case 3:
                // Gaussian
                {
                    float a = -pi*pi*2*sigma*sigma;
                    for (int i = 0; i < outputFrequencyLength; ++i)
                        _profileFrequency[i] = exp(a*i*i);
                }
                break;
        }
        if (_width == 0)
            scale = 0;
        for (int i = 0; i < outputFrequencyLength; ++i) {
            _profileFrequency[i] *=
                scale*unit(i*_offset/outputFrequencyLength);
        }

        _oPad = static_cast<int>(
            static_cast<float>(padding)*outputLength/inputLength);
    }

    void renderColumn(const Byte* input, int inputStride, Byte* output,
        int outputStride)
    {
        int inputFrequencyLength = _inputTimeLength/2 + 1;
        int outputFrequencyLength = _outputTimeLength/2 + 1;

        Byte* outputColumn = output;
        for (int i = 0; i < _outputLength; ++i) {
            *reinterpret_cast<DWORD*>(outputColumn) = 0;
            outputColumn += outputStride;
        }
        for (int channel = 0; channel < 3; ++channel) {
            int shift = channel * 8;
            const Byte* inputColumn = input;
            for (int i = 0; i < _inputLength; ++i) {
                _inputTime[i + padding] = pow((
                    (*reinterpret_cast<const DWORD*>(inputColumn) >> shift) &
                    0xff)/255.0f, 2.2f);
                inputColumn += inputStride;
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
            if (_bleeding) {
                float bleed = 0;
                for (int i = 0; i < _outputTimeLength; ++i) {
                    float o = _outputTime[i] + bleed;
                    bleed = 0;
                    if (o < 0) {
                        bleed = o;
                        o = 0;
                    }
                    if (o > 1) {
                        bleed = o - 1;
                        o = 1;
                    }
                    _outputTime[i] = o;
                }
            }
            outputColumn = output;
            for (int i = 0; i < _outputLength; ++i) {
                *reinterpret_cast<DWORD*>(outputColumn) |= byteClamp(
                    pow(_outputTime[i + _oPad], 1/2.2f)*255.0f) << shift;
                outputColumn += outputStride;
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
    int _oPad;
    FFTWRealArray<float> _inputTime;
    FFTWPlanDFTR2C1D<float> _forward;
    FFTWComplexArray<float> _frequency;
    FFTWComplexArray<float> _profileFrequency;
    FFTWPlanDFTC2R1D<float> _backward;
    int _outputTimeLength;
    FFTWRealArray<float> _outputTime;
};

class FIRScanlineRenderer
{
public:
    void init()
    {
        float kernelRadiusVertical = 16;
        int inputTop;
        int inputBottom;
        std::function<float(float)> verticalKernel;

        switch (_profile) {
            case 0:
                // Rectangle
                verticalKernel = [&](float distance)
                {
                    return 0.0f;
                };

                //{
                //    scale *= 1/(sigma*pi);
                //    for (int i = 1; i < outputFrequencyLength; ++i) {
                //        float f = pi*i*sigma;
                //        _profileFrequency[i] = sin(f)/i;
                //    }
                //    _profileFrequency[0] = pi*sigma;
                //}
                break;
            case 1:
                // Triangle
                verticalKernel = [&](float distance)
                {
                    return 0.0f;
                };

                //{
                //    for (int i = 1; i < outputFrequencyLength; ++i) {
                //        float f = pi*i*sigma;
                //        float s = sin(f)/f;
                //        _profileFrequency[i] = s*s;
                //    }
                //    _profileFrequency[0] = 1;
                //}
                break;
            case 2:
                // Circle
                verticalKernel = [&](float distance)
                {
                    return 0.0f;
                };

                //{
                //    float w = _width*outputTimeLength/inputTimeLength;
                //    float r2 = w*w;
                //    scale *= 2/(pi*r2);
                //    FFTWRealArray<float> profileTime(outputTimeLength);
                //    _profileFrequency.ensure(outputFrequencyLength);
                //    FFTWPlanDFTR2C1D<float> transform(outputTimeLength,
                //        profileTime, _profileFrequency, _rigor);
                //    for (int i = 0; i < outputTimeLength; ++i) {
                //        int y = i;
                //        if (i > outputTimeLength/2)
                //            y = outputTimeLength - i;
                //        if (y > w)
                //            profileTime[i] = 0;
                //        else
                //            profileTime[i] = sqrt(r2 - y*y);
                //    }
                //    transform.execute();
                //}
                break;
            case 3:
                // Gaussian
                {
                    float a = -1/(2*_width*_width);
                    float b = 1/(sqrt(tau)*_width);
                    verticalKernel = [&](float distance)
                    {
                        return b*exp(a*distance*distance);
                    };
                }
                break;
        }

        _vertical.generate(_size, kernelRadiusVertical, verticalKernel,
            &inputTop, &inputBottom, _zoom.y, _offset.y);

        static const float inputChannelPositions[3] = {0, 0, 0};
        static const float outputChannelPositions[3] = {0, 1.0f/3, 2.0f/3};

        float kernelRadiusHorizontal = 16;
        int inputLeft;
        int inputRight;
        float bandLimit = 1;
        if (_zoom.x < 1)
            bandLimit = _zoom.x;
        _horizontal.generate(Vector(_size.x, inputBottom - inputTop), 3,
            inputChannelPositions, 3, outputChannelPositions,
            kernelRadiusHorizontal,
            [&](float distance, int inputChannel, int outputChannel)
            {
                if (inputChannel != outputChannel)
                    return 0.0f;
                return sinc(distance*bandLimit);
            },
            &inputLeft, &inputRight, _zoom.x, _offset.x);
    }
    void render()
    {
        _horizontal.execute();
        _vertical.execute();
    }
    int getProfile() { return _profile; }
    void setProfile(int profile) { _profile = profile; }
    float getWidth() { return _width; }
    void setWidth(float width) { _width = width; }
    bool getBleeding() { return _bleeding; }
    void setBleeding(bool bleeding) { _bleeding = bleeding; }
    void setZoom(Vector2<float> zoom) { _zoom = zoom; }
    void setOffset(Vector2<float> offset) { _offset = offset; }
    void setOutputSize(Vector size) { _size = size; }

private:
    int _profile;
    float _width;
    bool _bleeding;
    Vector2<float> _offset;
    Vector2<float> _zoom;
    Vector _size;

    ImageFilterHorizontal _horizontal;
    ImageFilterVertical _vertical;
};

#endif // INCLUDED_SCANLINES_H
