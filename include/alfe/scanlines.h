#include "alfe/main.h"
#include "alfe/bitmap.h"
#include "alfe/complex.h"
#include "alfe/fft.h"

#ifndef INCLUDED_SCANLINES_H
#define INCLUDED_SCANLINES_H

#include "alfe/image_filter.h"

class ScanlineRenderer
{
public:
    ScanlineRenderer()
      : _profile(4), _horizontalProfile(4), _width(1), _bleeding(2),
        _horizontalBleeding(2), _horizontalRollOff(0), _verticalRollOff(0),
        _subPixelSeparation(0), _phosphor(0), _mask(0), _maskSize(0),
        _needsInit(true)
    { }
    void init()
    {
        if (!_needsInit)
            return;
        _needsInit = false;
        _output.ensure(_size.x*3*sizeof(float), _size.y);

        _vertical.generate(_size, 3,
            kernelRadius(_profile, _zoom.y, _width, _verticalRollOff,
                _verticalLobes),
            kernel(_profile, _zoom.y, _width, _verticalRollOff,
                _verticalLobes),
            &_inputTL.y, &_inputBR.y, _zoom.y, _offset.y);

        int inputHeight = _inputBR.y - _inputTL.y;
        _intermediate.ensure(_size.x*3*sizeof(float), inputHeight);

        _vertical.setBuffers(_intermediate, _output);

        static const float inputChannelPositions[3] = {0, 0, 0};
        float outputChannelPositions[3] =
            {-_subPixelSeparation/3, 0, _subPixelSeparation/3};

        auto channelKernel = kernel(_horizontalProfile, _zoom.x, 1,
            _horizontalRollOff, _horizontalLobes);
        _horizontal.generate(Vector(_size.x, inputHeight), 3,
            inputChannelPositions, 3, outputChannelPositions,
            kernelRadius(_horizontalProfile, _zoom.x, 1, _horizontalRollOff,
                _horizontalLobes),
            [=](float distance, int inputChannel, int outputChannel)
            {
                if ((inputChannel - outputChannel) % 3 != 0)
                    return Tuple<float, float>(0.0f, 0.0f);
                return channelKernel(distance);
            },
            &_inputTL.x, &_inputBR.x, _zoom.x, _offset.x);

        _input.ensure((_inputBR.x - _inputTL.x)*3*sizeof(float), inputHeight);

        _horizontal.setBuffers(_input, _intermediate);
    }
    void render()
    {
        _horizontal.execute();
        Byte* d = _intermediate.data();
        for (int y = 0; y < _inputBR.y - _inputTL.y; ++y) {
            bleed(d, 12, Vector(3, _size.x), _horizontalBleeding);
            d += _intermediate.stride();
        }
        _vertical.execute();
        bleed(_output.data(), _output.stride(), _size*Vector(3, 1), _bleeding);
    }
    int getProfile() { return _profile; }
    void setProfile(int profile)
    {
        if (_profile != profile)
            _needsInit = true;
        _profile = profile;
    }
    float getWidth() { return _width; }
    void setWidth(float width)
    {
        if (_width != width)
            _needsInit = true;
        _width = width;
    }
    int getBleeding() { return _bleeding; }
    void setBleeding(int bleeding) { _bleeding = bleeding; }
    int getHorizontalProfile() { return _horizontalProfile; }
    void setHorizontalProfile(int profile) { _horizontalProfile = profile; }
    int getHorizontalBleeding() { return _horizontalBleeding; }
    void setHorizontalBleeding(int bleeding)
    {
        _horizontalBleeding = bleeding;
    }
    float getVerticalRollOff() { return _verticalRollOff; }
    void setVerticalRollOff(float rollOff)
    {
        if (_verticalRollOff != rollOff)
            _needsInit = true;
        _verticalRollOff = rollOff;
    }
    float getHorizontalRollOff() { return _horizontalRollOff; }
    void setHorizontalRollOff(float rollOff)
    {
        if (_horizontalRollOff != rollOff)
            _needsInit = true;
        _horizontalRollOff = rollOff;
    }
    float getVerticalLobes() { return _verticalLobes; }
    void setVerticalLobes(float lobes)
    {
        if (_verticalLobes != lobes)
            _needsInit = true;
        _verticalLobes = lobes;
    }
    float getHorizontalLobes() { return _horizontalLobes; }
    void setHorizontalLobes(float lobes)
    {
        if (_horizontalLobes != lobes)
            _needsInit = true;
        _horizontalLobes = lobes;
    }
    float getSubPixelSeparation() { return _subPixelSeparation; }
    void setSubPixelSeparation(float separation)
    {
        if (_subPixelSeparation != separation)
            _needsInit = true;
        _subPixelSeparation = separation;
    }
    int getPhosphor() { return _phosphor; }
    void setPhosphor(int phosphor)
    {
        if (_phosphor != phosphor)
            _needsInit = true;
        _phosphor = phosphor;
    }
    int getMask() { return _mask; }
    void setMask(int mask)
    {
        if (_mask != mask)
            _needsInit = true;
        _mask = mask;
    }
    float getMaskSize() { return _maskSize; }
    void setMaskSize(float maskSize)
    {
        if (_maskSize != maskSize)
            _needsInit = true;
        _maskSize = maskSize;
    }
    void setZoom(Vector2<float> zoom)
    {
        if (_zoom != zoom)
            _needsInit = true;
        _zoom = zoom;
    }
    void setOffset(Vector2<float> offset)
    {
        if (_offset != offset)
            _needsInit = true;
        _offset = offset;
    }
    void setOutputSize(Vector size)
    {
        if (_size != size)
            _needsInit = true;
        _size = size;
    }
    Vector inputTL() { return _inputTL; }
    Vector inputBR() { return _inputBR; }
    AlignedBuffer input() { return _input; }
    AlignedBuffer output() { return _output; }

private:
    std::function<Tuple<float, float>(float)> kernel(int profile, float zoom,
        float width, float rollOff, float cutOff)
    {
        switch (profile) {
            case 0:
                // Rectangle
                {
                    float b = 0.5f*zoom*static_cast<float>(tau)*width/2.0f;
                    float c = 0.5f*zoom*static_cast<float>(tau);
                    return [=](float d)
                    {
                        float r = (sinint(b + c*d) + sinint(b - c*d))*
                            sinc(d*rollOff);
                        return Tuple<float, float>(r, r);
                    };
                }
                break;
            case 1:
                // Triangle
                return [=](float distance)
                {
                    float b = 0.5f*zoom;
                    float w = 0.5f*width;
                    float f = distance;
                    float t = static_cast<float>(tau);
                    float r = sinc(distance*rollOff)*(
                        +2*t*(f-w)*b*sinint(t*(f-w)*b)
                        +2*t*(f+w)*b*sinint(t*(f+w)*b)
                        -4*t*f*b*sinint(t*f*b)
                        +2*cos(b*t*(f-w))
                        +2*cos(b*t*(f+w))
                        -4*cos(b*t*f));
                    return Tuple<float, float>(r, r);
                };
                break;
            case 2:
                // Circle
                {
                    float b = 4.0f/(width*width);
                    float c = width/2.0f;
                    return [=](float distance)
                    {
                        float r = 0.0f;
                        if (distance > -c && distance < c) {
                            r = sqrt(1 - b*distance*distance)*
                               sinc(distance*rollOff);
                        }
                        return Tuple<float, float>(r, r);
                    };
                }
                break;
            case 3:
                // Gaussian
                {
                    float a = -8/(width*width);
                    return [=](float distance)
                    {
                        float r = exp(a*distance*distance)*
                            sinc(distance*rollOff);
                        return Tuple<float, float>(r, r);
                    };
                }
                break;
            case 4:
                // Sinc
                {
                    float bandLimit = min(1.0f, zoom)/width;
                    if (width == 0)
                        bandLimit = 10000;
                    return [=](float distance)
                    {
                        float r = sinc(distance*bandLimit)*
                            sinc(distance*rollOff);
                        return Tuple<float, float>(r, r);
                    };
                }
                break;
            default:
                // Box
                {
                    return [=](float distance)
                    {
                        float r = sinc(distance*rollOff);
                        return Tuple<float, float>(r, r);
                    };
                }
                break;
        }
    }
    float kernelRadius(int profile, float zoom, float width, float rollOff,
        float lobes)
    {
        switch (profile) {
            case 2:
                // Circle
                return width/2.0f;
            case 3:
                // Gaussian
                return lobes;
            case 4:
                // Sinc
                return lobes*width/min(1.0f, zoom);
            case 5:
                // Box
                return 0.5f;
        }
        return lobes/min(1.0f, zoom);
    }
    void bleed(Byte* data, int s, Vector size, int bleeding)
    {
        if (bleeding == 1) {
            Byte* outputColumn = data;
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
        if (bleeding == 2) {
            Byte* outputColumn = data;
            for (int x = 0; x < size.x; ++x) {
                Byte* output = outputColumn;
                float bleed = 0;
                for (int y = 0; y < size.y; ++y) {
                    float o = *reinterpret_cast<float*>(output);
                    if (o < 0) {
                        Byte* output1 = output + s;
                        int y1;
                        float t = -o;
                        *reinterpret_cast<float*>(output) = 0;
                        for (y1 = y + 1; y1 < size.y; ++y1, output1 += s) {
                            float o1 = *reinterpret_cast<float*>(output1);
                            if (o1 > 0)
                                break;
                            t -= o1;
                            *reinterpret_cast<float*>(output1) = 0;
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
                        *reinterpret_cast<float*>(output) = 1;
                        for (y1 = y + 1; y1 < size.y; ++y1, output1 += s) {
                            float o1 = *reinterpret_cast<float*>(output1);
                            if (o1 < 1)
                                break;
                            t += o1 - 1;
                            *reinterpret_cast<float*>(output1) = 1;
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

    int _profile;
    int _horizontalProfile;
    float _width;
    int _bleeding;
    int _horizontalBleeding;
    float _horizontalRollOff;
    float _verticalRollOff;
    float _horizontalLobes;
    float _verticalLobes;
    float _subPixelSeparation;
    int _phosphor;
    int _mask;
    float _maskSize;
    Vector2<float> _offset;
    Vector2<float> _zoom;
    Vector _size;
    AlignedBuffer _input;
    AlignedBuffer _intermediate;
    AlignedBuffer _output;
    Vector _inputTL;
    Vector _inputBR;
    bool _needsInit;

    ImageFilterHorizontal _horizontal;
    ImageFilterVertical _vertical;
};

#endif // INCLUDED_SCANLINES_H
