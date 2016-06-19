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

float sinint(float x)
{
    float mr = 1;
    if (x < 0) {
        x = -x;
        mr = -1;
    }
    if (x < 10) {
        float i = 3;
        float r = x;
        float x2 = -x*x;
        float t = x;
        static const float eps = 1.0f/(1 << 16);
        do {
            t *= x2/(i*(i - 1));
            r += t/i;
            i += 2;
        } while (t < -eps || t > eps);
        return r * mr;
    }
    float cr = 1;
    float sr = 1/x;
    float ct = 1;
    float st = 1/x;
    float i = 2;
    float x2 = -1/(x*x);
    do {
        float lct = ct;
        ct *= x2*i*(i - 1);
        st *= x2*i*(i + 1);
        if (abs(ct) > abs(lct) || ct == 0)
            break;
        cr += ct;
        sr += st;
        i += 2;
    } while (true);
    return (static_cast<float>(tau)/4.0f - (cos(x)/x)*cr - (sin(x)/x)*sr) * mr;
}

class FIRScanlineRenderer
{
public:
    void init()
    {
        float kernelRadiusVertical = 16;
        std::function<float(float)> verticalKernel;
        Vector2<float> bandLimit(0.5f, 0.5f);
        if (_zoom.x < 1)
            bandLimit.x = _zoom.x;
        if (_zoom.y < 1)
            bandLimit.y = _zoom.y;
        //if (_zoom.x > 1)
        //    bandLimit.x = _zoom.x;
        //if (_zoom.y > 1)
        //    bandLimit.y = _zoom.y;

        switch (_profile) {
            case 0:
                // Rectangle
                {
                    float a = 2.0f/(static_cast<float>(tau)*_width);
                    float b = 0.5f*_zoom.y*static_cast<float>(tau)*_width/2.0f;
                    float c = 0.5f*_zoom.y*static_cast<float>(tau);
                    verticalKernel = [=](float d)
                    {
                        //return (abs(d) < _width/2 ? 1 : 0);
                        return a*(sinint(b + c*d) + sinint(b - c*d));
                    };
                }
                break;
            case 1:
                // Triangle
                verticalKernel = [=](float distance)
                {
                    float b = 0.5f*_zoom.y;
                    float w = 0.5f*_width;
                    float f = distance;
                    float t = static_cast<float>(tau);
                    return (
                        +2*t*(f-w)*b*sinint(t*(f-w)*b)
                        +2*t*(f+w)*b*sinint(t*(f+w)*b)
                        -4*t*f*b*sinint(t*f*b)
                        +2*cos(b*t*(f-w))
                        +2*cos(b*t*(f+w))
                        -4*cos(b*t*f)
                        )/(t*t*w*w*b);
                };
                break;
            case 2:
                // Circle
                verticalKernel = [=](float distance)
                {
                    return 0.0f;
                };
                break;
            case 3:
                // Gaussian
                {
                    float a = -1/(2*_width*_width);
                    float b = 1/(sqrt(static_cast<float>(tau))*_width);
                    verticalKernel = [=](float distance)
                    {
                        return b*exp(a*distance*distance);
                    };
                }
                break;
        }

        _output.ensure(_size.x*3*sizeof(float), _size.y);

        _vertical.generate(_size, 3, kernelRadiusVertical, verticalKernel,
            &_inputTL.y, &_inputBR.y, _zoom.y, _offset.y);

        int inputHeight = _inputBR.y - _inputTL.y;
        _intermediate.ensure(_size.x*3*sizeof(float), inputHeight);

        _vertical.setBuffers(_intermediate, _output);

        static const float inputChannelPositions[3] = {0, 0, 0};
        //static const float outputChannelPositions[3] = {0, 1.0f/3, 2.0f/3};
        static const float outputChannelPositions[3] = {0, 0, 0};

        float kernelRadiusHorizontal = 16;
        _horizontal.generate(Vector(_size.x, inputHeight), 3,
            inputChannelPositions, 3, outputChannelPositions,
            kernelRadiusHorizontal,
            [=](float distance, int inputChannel, int outputChannel)
            {
                if (inputChannel != outputChannel)
                    return 0.0f;
                return bandLimit.x*sinc(distance*bandLimit.x);
            },
            &_inputTL.x, &_inputBR.x, _zoom.x, _offset.x);

        _input.ensure((_inputBR.x - _inputTL.x)*3*sizeof(float), inputHeight);

        _horizontal.setBuffers(_input, _intermediate);
    }
    void render()
    {
        _horizontal.execute();
        _vertical.execute();
        int s = _output.stride();
        Vector size = _size;
        size.x *= 3;
        if (_bleeding == 1) {
            Byte* outputColumn = _output.data();
            for (int x = 0; x < size.x; ++x) {
                Byte* output = outputColumn;
                float bleed = 0;
                for (int y = 0; y < size.y; ++y) {
                    float o = bleed + *reinterpret_cast<float*>(output);
                    bleed = 0;
                    if (o < 0) {
                        bleed = o;
                        o = 0;
                    }
                    if (o > 1) {
                        bleed = o - 1;
                        o = 1;
                    }
                    *reinterpret_cast<float*>(output) = o;
                    output += s;
                }
                outputColumn += sizeof(float);
            }
            return;
        }
        if (_bleeding == 2) {
            Byte* outputColumn = _output.data();
            for (int x = 0; x < size.x; ++x) {
                Byte* output = outputColumn;
                float bleed = 0;
                for (int y = 0; y < size.y; ++y) {
                    float o = *reinterpret_cast<float*>(output);
                    if (o < 0) {
                        Byte* output1 = output + s;
                        int y1;
                        float t = -o;
                        for (y1 = y + 1; y1 < size.y; ++y1, output1 += s) {
                            float o1 = *reinterpret_cast<float*>(output1);
                            if (o1 > 0)
                                break;
                            t -= o1;
                        }
                        float bleed = t/2;
                        Byte* output2 = output - s;
                        for (int y2 = y - 1; y2 >= 0; --y2, output2 -= s) {
                            float* p = reinterpret_cast<float*>(output2);
                            float o2 = *p;
                            if (o2 > 0) {
                                if (bleed > o2) {
                                    bleed -= o2;
                                    *p = 0;
                                }
                                else {
                                    *p = o2 - bleed;
                                    break;
                                }
                            }
                        }
                        bleed = t/2;
                        output2 = output1;
                        for (int y2 = y1; y2 < size.y; ++y2, output2 += s) {
                            float* p = reinterpret_cast<float*>(output2);
                            float o2 = *p;
                            if (o2 > 0) {
                                if (bleed > o2) {
                                    bleed -= o2;
                                    *p = 0;
                                }
                                else {
                                    *p = o2 - bleed;
                                    break;
                                }
                            }
                        }
                        // We need to restart at y1 here rather than y2 because
                        // there might be some more <0 values between y1 and
                        // y2.
                        y = y1 - 1;
                        output = output1;
                        continue;
                    }
                    if (o > 1) {
                        Byte* output1 = output + s;
                        int y1;
                        float t = o - 1;
                        for (y1 = y + 1; y1 < size.y; ++y1, output1 += s) {
                            float o1 = *reinterpret_cast<float*>(output1);
                            if (o1 < 1)
                                break;
                            t += o1 - 1;
                        }
                        float bleed = t/2;
                        Byte* output2 = output - s;
                        for (int y2 = y - 1; y2 >= 0; --y2, output2 -= s) {
                            float* p = reinterpret_cast<float*>(output2);
                            float o2 = *p;
                            if (o2 < 1) {
                                if (bleed + o2 > 1) {
                                    bleed -= 1 - o2;
                                    *p = 1;
                                }
                                else {
                                    *p = o2 + bleed;
                                    break;
                                }
                            }
                        }
                        bleed = t/2;
                        output2 = output1;
                        for (int y2 = y1; y2 < size.y; ++y2, output2 += s) {
                            float* p = reinterpret_cast<float*>(output2);
                            float o2 = *p;
                            if (o2 < 1) {
                                if (bleed + o2 > 1) {
                                    bleed -= 1 - o2;
                                    *p = 1;
                                }
                                else {
                                    *p = o2 + bleed;
                                    break;
                                }
                            }
                        }
                        // We need to restart at y1 here rather than y2 because
                        // there might be some more >1 values between y1 and
                        // y2.
                        y = y1 - 1;
                        output = output1;
                        continue;
                    }
                    output += s;
                }
                outputColumn += sizeof(float);
            }
        }
    }
    int getProfile() { return _profile; }
    void setProfile(int profile) { _profile = profile; }
    float getWidth() { return _width; }
    void setWidth(float width) { _width = width; }
    int getBleeding() { return _bleeding; }
    void setBleeding(int bleeding) { _bleeding = bleeding; }
    void setZoom(Vector2<float> zoom) { _zoom = zoom; }
    void setOffset(Vector2<float> offset) { _offset = offset; }
    void setOutputSize(Vector size) { _size = size; }
    Vector inputTL() { return _inputTL; }
    Vector inputBR() { return _inputBR; }
    AlignedBuffer input() { return _input; }
    AlignedBuffer output() { return _output; }

private:
    int _profile;
    float _width;
    int _bleeding;
    Vector2<float> _offset;
    Vector2<float> _zoom;
    Vector _size;
    AlignedBuffer _input;
    AlignedBuffer _intermediate;
    AlignedBuffer _output;
    Vector _inputTL;
    Vector _inputBR;

    ImageFilterHorizontal _horizontal;
    ImageFilterVertical _vertical;
};

#endif // INCLUDED_SCANLINES_H
