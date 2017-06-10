#include "alfe/main.h"

#ifndef INCLUDED_IMAGE_FILTER_H
#define INCLUDED_IMAGE_FILTER_H

#include <memory>
#include <functional>
#include <intrin.h>
#include "alfe/tuple.h"

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

bool useSSE2()
{
    //return false;

    int cpuInfo[4];
    __cpuid(cpuInfo, 0);
    if (cpuInfo[0] < 1)
        return false;
    __cpuid(cpuInfo, 1);
    return (cpuInfo[3] & (1 << 26)) != 0;
}

class AlignedBuffer
{
public:
    AlignedBuffer() { }
    AlignedBuffer(int x, int y = 1) { ensure(x, y); }
    // Ensure we have y suitably-aligned rows of x bytes each
    void ensure(int x, int y = 1)
    {
        int alignment = useSSE2() ? 16 : 4;
        _stride = (x + alignment - 1) & ~(alignment - 1);
        size_t size = _stride*y;
        size_t space = size + alignment - 1;
        _buffer.ensure(space);
        void* b = static_cast<void*>(&_buffer[0]);
        std::align(alignment, size, b, space);
        _aligned = static_cast<Byte*>(b);
    }
    Byte* data() { return _aligned; }
    int stride() { return _stride; }
private:
    Array<Byte> _buffer;
    Byte* _aligned;
    int _stride;
};

// Filters and resamples an image horizontally using 16-bit integer arithmetic.
class ImageFilter16
{
public:
    ImageFilter16() : _shift(6) { }
    void execute()
    {
        Byte* inputRow = _input.data() + _inputOffset;
        Byte* outputRow = _output.data();
        int* kernelSizes = &_kernelSizes[0];
        if (useSSE2()) {
            for (int y = 0; y < _height; ++y) {
                __m128i* kernel =
                    reinterpret_cast<__m128i*>(_kernelBuffer.data());
                __m128i* output = reinterpret_cast<__m128i*>(outputRow);
                int* offsets = &_offsets[0];
                for (int x = 0; x < _width; ++x) {
                    __m128i total = _mm_set1_epi16(0);
                    int kernelSize = kernelSizes[x];
                    for (int k = 0; k < kernelSize; ++k) {
                        // We need to use an unaligned load here because we
                        // need it to be possible for any input position to
                        // affect any output position. We could do this by
                        // duplicating each input position eight times, but
                        // this would probably be slower than the unaligned
                        // loads.
                        total = _mm_add_epi16(total, _mm_mullo_epi16(*kernel,
                            _mm_castps_si128(_mm_loadu_ps(
                            reinterpret_cast<float*>(inputRow + *offsets)))));
                        ++kernel;
                        ++offsets;
                    }
                    output[x] = total;
                }
                inputRow += _input.stride();
                outputRow += _output.stride();
            }
        }
        else {
            for (int y = 0; y < _height; ++y) {
                UInt16* kernel =
                    reinterpret_cast<UInt16*>(_kernelBuffer.data());
                UInt16* output = reinterpret_cast<UInt16*>(outputRow);
                int* offsets = &_offsets[0];
                for (int x = 0; x < _width; ++x) {
                    UInt16 total = 0;
                    int kernelSize = kernelSizes[x];
                    for (int k = 0; k < kernelSize; ++k) {
                        total += *kernel *
                            *reinterpret_cast<UInt16*>(inputRow + *offsets);
                        ++kernel;
                        ++offsets;
                    }
                    output[x] = total;
                }
                inputRow += _input.stride();
                outputRow += _output.stride();
            }
        }
    }
    // outputSize.x is measured in output pixels
    // inputChannels is number of channels in input data
    // inputChannelPositions are input pixel positions of input channels.
    // outputChannels is number of channels in output data
    // outputChannelPositions are output pixel positions of output channels.
    // The kernel spans range -kernelRadius to kernelRadius input pixels.
    // kernelFunction's arguments are distance, inputChannel, outputChannel.
    //   distance is outputPixel - inputPixel in input pixels
    // The first value returned by the kernelFunction is the actual
    // coefficient. The second value is a normalization value - the
    // coefficients will be scaled by the scale factor that makes the
    // scaled normalization values total to 1 for each output pixel channel.
    // In inputLeft we return the leftmost input pixel that will be accessed.
    // In inputRight we return the rightmost input pixel that will be
    // accessed plus one.
    // zoom is number of output pixels per input pixel
    // offset is input position of output pixel 0.
    void generate(Vector outputSize, int inputChannels,
        const float* inputChannelPositions, int outputChannels,
        const float* outputChannelPositions, float kernelRadius,
        std::function<Tuple<float, float>(float, int, int)> kernelFunction,
        int* inputLeft, int* inputRight, float zoom, float offset)
    {
        float minInputChannelPosition = std::numeric_limits<float>::max();
        float maxInputChannelPosition = -minInputChannelPosition;
        for (int c = 0; c < inputChannels; ++c) {
            float p = inputChannelPositions[c];
            minInputChannelPosition = min(minInputChannelPosition, p);
            maxInputChannelPosition = max(maxInputChannelPosition, p);
        }
        float minOutputChannelPosition = std::numeric_limits<float>::max();
        float maxOutputChannelPosition = -minOutputChannelPosition;
        for (int c = 0; c < outputChannels; ++c) {
            float p = outputChannelPositions[c];
            minOutputChannelPosition = min(minOutputChannelPosition, p);
            maxOutputChannelPosition = max(maxOutputChannelPosition, p);
        }

        int channelsPerUnit = (useSSE2() ? 8 : 1);
        _height = outputSize.y;
        _width = (outputSize.x*outputChannels + channelsPerUnit - 1)/
            channelsPerUnit;
        _kernelSizes.ensure(_width);
        int kWidth = (static_cast<int>(kernelRadius*2 + 1
            +maxInputChannelPosition - minInputChannelPosition
            +((channelsPerUnit - 1)/outputChannels
            +maxOutputChannelPosition - minOutputChannelPosition
            )/zoom)*inputChannels
            + channelsPerUnit - 1)/channelsPerUnit;
        _kernelBuffer.ensure(
            kWidth*channelsPerUnit*_width*channelsPerUnit*sizeof(UInt16));
        UInt16* kernel = reinterpret_cast<UInt16*>(_kernelBuffer.data());
        _offsets.ensure(kWidth*_width*channelsPerUnit);
        _kernelSizes.ensure(_width);
        int* offsets = &_offsets[0];
        int* sizes = &_kernelSizes[0];
        float scale = static_cast<float>(1 << _shift);
        _tempKernel.ensure(
            (channelsPerUnit + kWidth*channelsPerUnit)*channelsPerUnit);
        _totals.ensure(inputChannels*channelsPerUnit);

        _outputLeft = std::numeric_limits<int>::max();
        _outputRight = std::numeric_limits<int>::min();

        int left = std::numeric_limits<int>::max();
        int right = std::numeric_limits<int>::min();
        for (int x = 0; x < _width; ++x) {
            int o = x*channelsPerUnit;
            int kernelSize = 0;

            // Compute leftmost and rightmost possible input positions
            float lp = offset - kernelRadius + 1 +
                (static_cast<float>(o / outputChannels) +
                    minOutputChannelPosition)/zoom + minInputChannelPosition;
            if (lp < 0)
                lp -= 1;
            int leftInput = static_cast<int>(lp)*inputChannels + 1
                - channelsPerUnit;

            float rp = offset + kernelRadius +
                (static_cast<float>((o + channelsPerUnit - 1) / outputChannels)
                    + maxOutputChannelPosition)/zoom + maxInputChannelPosition;
            if (rp < 0)
                rp -= 1;
            int rightInput = static_cast<int>(rp)*inputChannels + inputChannels
                - 1;

            int multiple = 0;
            if (leftInput < 0)
                multiple = -leftInput*inputChannels;

            for (int c = 0; c < inputChannels*channelsPerUnit; ++c)
                _totals[c] = 0;
            int realLeftInput = leftInput;
            for (int i = leftInput; i <= rightInput; ++i) {
                for (int c = 0; c < channelsPerUnit; ++c) {
                    int ic = i + c;
                    int inputChannel = (ic + multiple) % inputChannels;
                    float inputPosition = inputChannelPositions[inputChannel] +
                        static_cast<float>(
                        (ic - inputChannel) / inputChannels);
                    int oc = o + c;
                    int outputChannel = oc % outputChannels;
                    float centerInputPixel =
                        (static_cast<float>(oc / outputChannels) +
                        outputChannelPositions[outputChannel])/zoom + offset;
                    float dist = centerInputPixel - inputPosition;
                    Tuple<float, float> v(0, 0);
                    if (dist >= -kernelRadius && dist <= kernelRadius)
                        v = kernelFunction(dist, ic, outputChannel);
                    _totals[inputChannel*channelsPerUnit + c] += v.second();
                    _tempKernel[(i - leftInput)*channelsPerUnit + c] =
                        v.first();
                }
            }
            for (int c = 0; c < channelsPerUnit*inputChannels; ++c)
                _totals[c] = 1/_totals[c];
            for (int i = leftInput; i <= rightInput; ++i) {
                int lastC = 0;
                for (int c = 0; c < channelsPerUnit; ++c) {
                    int v = static_cast<int>(round(scale*
                        _tempKernel[(i - leftInput)*channelsPerUnit + c]*
                        _totals[((i + c + multiple) % inputChannels)*
                        channelsPerUnit + c]));
                    if (v != 0) {
                        int op = (o + c + outputChannels - 1)/outputChannels;
                        if (op >= 0)
                            _outputLeft = min(_outputLeft, op);
                        if (op < outputSize.x)
                            _outputRight = max(_outputRight, op + 1);
                        if (lastC == 0) {
                            realLeftInput = i;
                            left = min(left, realLeftInput);
                            *offsets = realLeftInput*sizeof(UInt16);
                            ++offsets;
                            ++kernelSize;
                        }
                        for (; lastC < c; ++lastC) {
                            *kernel = 0;
                            ++kernel;
                        }
                        *kernel = v;
                        ++kernel;
                        ++lastC;
                    }
                }
                if (lastC != 0) {
                    for (; lastC < channelsPerUnit; ++lastC) {
                        *kernel = 0;
                        ++kernel;
                    }
                    right = max(right, i);
                }
            }
            sizes[x] = kernelSize;
        }
        if (left < 0)
            *inputLeft = (left - (inputChannels - 1))/inputChannels;
        else
            *inputLeft = left/inputChannels;
        if (right < 0)
            *inputRight = (right - (inputChannels - 1))/inputChannels + 1;
        else
            *inputRight = right/inputChannels + 1;

        _inputOffset = -*inputLeft*inputChannels*sizeof(UInt16);
    }
    void setBuffers(AlignedBuffer input, AlignedBuffer output)
    {
        _input = input;
        _output = output;
    }
    int outputLeft() const { return _outputLeft; }
    int outputRight() const { return _outputRight; }
    void setShift(int shift) { _shift = shift; }
    int shift() const { return _shift; }

private:
    // Buffers
    AlignedBuffer _kernelBuffer;
    Array<int> _offsets;
    Array<int> _kernelSizes;
    AlignedBuffer _input;
    AlignedBuffer _output;
    Array<float> _tempKernel;
    Array<float> _totals;

    // Parameters
    int _width;
    int _height;
    int _inputStride;
    int _outputStride;
    int _inputOffset;
    int _shift;

    int _outputLeft;
    int _outputRight;
};

// Filters and resamples an image horizontally using single-precision
// floating-point arithmetic.
class ImageFilterHorizontal
{
public:
    void execute()
    {
        Byte* inputRow = _input.data() + _inputOffset;
        Byte* outputRow = _output.data();
        int* kernelSizes = &_kernelSizes[0];
        if (useSSE2()) {
            for (int y = 0; y < _height; ++y) {
                __m128* kernel =
                    reinterpret_cast<__m128*>(_kernelBuffer.data());
                __m128* output = reinterpret_cast<__m128*>(outputRow);
                int* offsets = &_offsets[0];
                for (int x = 0; x < _width; ++x) {
                    __m128 total = _mm_set1_ps(0.0f);
                    int kernelSize = kernelSizes[x];
                    for (int k = 0; k < kernelSize; ++k) {
                        // We need to use an unaligned load here because we
                        // need it to be possible for any input position to
                        // affect any output position. We could do this by
                        // duplicating each input position four times, but this
                        // would probably be slower than the unaligned loads.
                        total = _mm_add_ps(total, _mm_mul_ps(*kernel,
                            _mm_loadu_ps(reinterpret_cast<float*>(inputRow +
                                *offsets))));
                        ++kernel;
                        ++offsets;
                    }
                    output[x] = total;
                }
                inputRow += _input.stride();
                outputRow += _output.stride();
            }
        }
        else {
            for (int y = 0; y < _height; ++y) {
                float* kernel = reinterpret_cast<float*>(_kernelBuffer.data());
                float* output = reinterpret_cast<float*>(outputRow);
                int* offsets = &_offsets[0];
                for (int x = 0; x < _width; ++x) {
                    float total = 0;
                    int kernelSize = kernelSizes[x];
                    for (int k = 0; k < kernelSize; ++k) {
                        total += *kernel *
                            *reinterpret_cast<float*>(inputRow + *offsets);
                        ++kernel;
                        ++offsets;
                    }
                    output[x] = total;
                }
                inputRow += _input.stride();
                outputRow += _output.stride();
            }
        }
    }
    // outputSize.x is measured in output pixels
    // inputChannels is number of channels in input data
    // inputChannelPositions are input pixel positions of input channels.
    // outputChannels is number of channels in output data
    // outputChannelPositions are output pixel positions of output channels.
    // The kernel spans range -kernelRadius to kernelRadius input pixels.
    // kernelFunction's arguments are distance, inputChannel, outputChannel.
    //   distance is outputPixel - inputPixel in input pixels
    // The first value returned by the kernelFunction is the actual
    // coefficient. The second value is a normalization value - the
    // coefficients will be scaled by the scale factor that makes the
    // scaled normalization values total to 1 for each output pixel channel.
    // In inputLeft we return the leftmost input pixel that will be accessed.
    // In inputRight we return the rightmost input pixel that will be
    // accessed plus one.
    // zoom is number of output pixels per input pixel
    // offset is input position of output pixel 0.
    void generate(Vector outputSize, int inputChannels,
        const float* inputChannelPositions, int outputChannels,
        const float* outputChannelPositions, float kernelRadius,
        std::function<Tuple<float, float>(float, int, int)> kernelFunction,
        int* inputLeft, int* inputRight, float zoom, float offset)
    {
        float minInputChannelPosition = std::numeric_limits<float>::max();
        float maxInputChannelPosition = -minInputChannelPosition;
        for (int c = 0; c < inputChannels; ++c) {
            float p = inputChannelPositions[c];
            minInputChannelPosition = min(minInputChannelPosition, p);
            maxInputChannelPosition = max(maxInputChannelPosition, p);
        }
        float minOutputChannelPosition = std::numeric_limits<float>::max();
        float maxOutputChannelPosition = -minOutputChannelPosition;
        for (int c = 0; c < outputChannels; ++c) {
            float p = outputChannelPositions[c];
            minOutputChannelPosition = min(minOutputChannelPosition, p);
            maxOutputChannelPosition = max(maxOutputChannelPosition, p);
        }
        int channelsPerUnit = (useSSE2() ? 4 : 1);
        _totals.ensure(inputChannels*channelsPerUnit);
        _height = outputSize.y;
        _width = (outputSize.x*outputChannels + channelsPerUnit - 1)/
            channelsPerUnit;
        _kernelSizes.ensure(_width);
        int kWidth = (static_cast<int>(kernelRadius*2 + 1
            +maxInputChannelPosition - minInputChannelPosition
            +((channelsPerUnit - 1)/outputChannels
                +maxOutputChannelPosition - minOutputChannelPosition
                )/zoom)*inputChannels
            + channelsPerUnit - 1)/channelsPerUnit;
        _kernelBuffer.ensure(
            kWidth*channelsPerUnit*_width*channelsPerUnit*sizeof(float));
        float* kernel = reinterpret_cast<float*>(_kernelBuffer.data());
        _offsets.ensure(kWidth*_width*channelsPerUnit);
        _kernelSizes.ensure(_width);
        int* offsets = &_offsets[0];
        int* sizes = &_kernelSizes[0];

        _outputLeft = std::numeric_limits<int>::max();
        _outputRight = std::numeric_limits<int>::min();

        int left = std::numeric_limits<int>::max();
        int right = std::numeric_limits<int>::min();
        for (int x = 0; x < _width; ++x) {
            int o = x*channelsPerUnit;
            int kernelSize = 0;

            // Compute leftmost and rightmost possible input positions
            float lp = offset - kernelRadius + 1 +
                (static_cast<float>(o / outputChannels) +
                minOutputChannelPosition)/zoom + minInputChannelPosition;
            if (lp < 0)
                lp -= 1;
            int leftInput = static_cast<int>(lp)*inputChannels + 1
                - channelsPerUnit;

            float rp = offset + kernelRadius +
                (static_cast<float>((o + channelsPerUnit - 1) / outputChannels)
                + maxOutputChannelPosition)/zoom + maxInputChannelPosition;
            if (rp < 0)
                rp -= 1;
            int rightInput = static_cast<int>(rp)*inputChannels + inputChannels
                - 1;

            int multiple = 0;
            if (leftInput < 0)
                multiple = -leftInput*inputChannels;

            for (int c = 0; c < inputChannels*channelsPerUnit; ++c)
                _totals[c] = 0;
            float* kernelStart = kernel;
            int* offsetsStart = offsets;
            for (int i = leftInput; i <= rightInput; ++i) {
                int lastC = 0;
                for (int c = 0; c < channelsPerUnit; ++c) {
                    int ic = i + c;
                    int inputChannel = (ic + multiple) % inputChannels;
                    float inputPosition = inputChannelPositions[inputChannel] +
                        static_cast<float>(
                        (ic - inputChannel) / inputChannels);
                    int oc = o + c;
                    int outputChannel = oc % outputChannels;
                    float centerInputPixel =
                        (static_cast<float>(oc / outputChannels) +
                        outputChannelPositions[outputChannel])/zoom + offset;
                    float dist = centerInputPixel - inputPosition;
                    Tuple<float, float> v(0, 0);
                    if (dist >= -kernelRadius && dist <= kernelRadius)
                        v = kernelFunction(dist, ic, outputChannel);
                    _totals[inputChannel*channelsPerUnit + c] += v.second();
                    if (v.first() != 0) {
                        int op = (oc + outputChannels - 1)/outputChannels;
                        if (op >= 0)
                            _outputLeft = min(_outputLeft, op);
                        if (op < outputSize.x)
                            _outputRight = max(_outputRight, op + 1);
                        if (lastC == 0) {
                            left = min(left, i);
                            *offsets = i*sizeof(float);
                            ++offsets;
                            ++kernelSize;
                        }
                        for (; lastC < c; ++lastC) {
                            *kernel = 0;
                            ++kernel;
                        }
                        *kernel = v.first();
                        ++kernel;
                        ++lastC;
                    }
                }
                if (lastC != 0) {
                    for (; lastC < channelsPerUnit; ++lastC) {
                        *kernel = 0;
                        ++kernel;
                    }
                    right = max(right, i);
                }
            }
            for (int c = 0; c < channelsPerUnit*inputChannels; ++c)
                _totals[c] = 1/_totals[c];
            for (;kernelStart != kernel; kernelStart += channelsPerUnit) {
                int i = *offsetsStart/static_cast<int>(sizeof(float)) +
                    multiple;
                for (int c = 0; c < channelsPerUnit; ++c) {
                    kernelStart[c] *= _totals[c +
                        channelsPerUnit*((i + c) % inputChannels)];
                }
                ++offsetsStart;
            }
            sizes[x] = kernelSize;
        }
        if (left < 0)
            *inputLeft = (left - (inputChannels - 1))/inputChannels;
        else
            *inputLeft = left/inputChannels;
        if (right < 0)
            *inputRight = (right - (inputChannels - 1))/inputChannels + 1;
        else
            *inputRight = right/inputChannels + 1;

        _inputOffset = -*inputLeft*inputChannels*sizeof(float);
    }
    void setBuffers(AlignedBuffer input, AlignedBuffer output)
    {
        _input = input;
        _output = output;
    }
    int outputLeft() const { return _outputLeft; }
    int outputRight() const { return _outputRight; }

private:
    // Buffers
    AlignedBuffer _kernelBuffer;
    Array<int> _offsets;
    Array<int> _kernelSizes;
    AlignedBuffer _input;
    AlignedBuffer _output;
    Array<float> _totals;

    // Parameters
    int _width;
    int _height;
    int _inputStride;
    int _outputStride;
    int _inputOffset;

    int _outputLeft;
    int _outputRight;
};

// Filters and resamples an image vertically using single-precision
// floating-point arithmetic.
class ImageFilterVertical
{
public:
    void execute()
    {
        Byte* inputStart = _input.data() + _inputOffset;
        Byte* outputRow = _output.data();
        Byte* kernel = _kernelBuffer.data();
        int* offsets = &_offsets[0];
        int* kernelSizes = &_kernelSizes[0];
        if (useSSE2()) {
            for (int y = 0; y < _height; ++y) {
                int kernelSize = kernelSizes[y];
                int offset = offsets[y];
                Byte* inputColumn = inputStart;
                for (int x = 0; x < _width; ++x) {
                    Byte* input = inputColumn + offset;
                    __m128 total = _mm_set1_ps(0.0f);
                    for (int k = 0; k < kernelSize; ++k) {
                        total = _mm_add_ps(total, _mm_mul_ps(
                            reinterpret_cast<__m128*>(kernel)[k],
                            *reinterpret_cast<__m128*>(input)));
                        input += _input.stride();
                    }
                    inputColumn += sizeof(__m128);
                    reinterpret_cast<__m128*>(outputRow)[x] = total;
                }
                outputRow += _output.stride();
                kernel += kernelSize*sizeof(__m128);
            }
        }
        else {
            for (int y = 0; y < _height; ++y) {
                int kernelSize = kernelSizes[y];
                int offset = offsets[y];
                Byte* inputColumn = inputStart;
                for (int x = 0; x < _width; ++x) {
                    Byte* input = inputColumn + offset;
                    float total = 0;
                    for (int k = 0; k < kernelSize; ++k) {
                        total += reinterpret_cast<float*>(kernel)[k] *
                            *reinterpret_cast<float*>(input);
                        input += _input.stride();
                    }
                    inputColumn += sizeof(float);
                    reinterpret_cast<float*>(outputRow)[x] = total;
                }
                outputRow += _output.stride();
                kernel += kernelSize*sizeof(float);
            }
        }
    }
    // outputSize.x is measured in output channels and should be a multiple of
    // channelsPerUnit
    // outputSize.y is measured in output pixels
    // The kernel spans range -kernelRadius to kernelRadius input pixels.
    // kernelFunction's argument is outputPixel - inputPixel in input pixels
    // The first value returned by the kernelFunction is the actual
    // coefficient. The second value is a normalization value - the
    // coefficients will be scaled by the scale factor that makes the
    // scaled normalization values total to 1 for each output pixel channel.
    // In inputTop we return the topmost input pixel that will be accessed.
    // In inputBottom we return the bottommost input pixel that will be
    // accessed plus one.
    // zoom is number of output pixels per input pixel
    // offset is input position of output pixel 0.
    void generate(Vector outputSize, int channels, float kernelRadius,
        std::function<Tuple<float,float>(float)> kernelFunction, int* inputTop,
        int* inputBottom, float zoom, float offset)
    {
        int channelsPerUnit = (useSSE2() ? 4 : 1);
        _height = outputSize.y;
        _width = (outputSize.x*channels + channelsPerUnit - 1)/
            channelsPerUnit;
        _kernelSizes.ensure(_height);
        int kWidth = static_cast<int>(kernelRadius*2 + 1);
        _kernelBuffer.ensure(kWidth*channelsPerUnit*_height*sizeof(float));
        float* kernel = reinterpret_cast<float*>(_kernelBuffer.data());
        _offsetCounts.ensure(_height);
        _offsets.ensure(_height);
        _kernelSizes.ensure(_height);
        int* offsets = &_offsetCounts[0];
        int* sizes = &_kernelSizes[0];

        int top = std::numeric_limits<int>::max();
        int bottom = std::numeric_limits<int>::min();
        for (int y = 0; y < _height; ++y) {
            float total = 0;
            // Compute topmost and bottommost possible input positions
            float tp = offset - kernelRadius + 1 + static_cast<float>(y)/zoom;
            if (tp < 0)
                tp -= 1;
            int topInput = static_cast<int>(tp);

            float bp = offset + kernelRadius + static_cast<float>(y)/zoom;
            if (bp < 0)
                bp -= 1;
            int bottomInput = static_cast<int>(bp);

            float centerInputPixel = static_cast<float>(y)/zoom + offset;

            top = min(top, topInput);
            bottom = max(bottom, bottomInput);
            float* kernelStart = kernel;
            int realTop = bottomInput + 1;
            int realBottom;
            for (int i = topInput; i <= bottomInput; ++i) {
                float dist = centerInputPixel - static_cast<float>(i);
                Tuple<float, float> v(0, 0);
                if (dist >= -kernelRadius && dist <= kernelRadius)
                    v = kernelFunction(dist);
                total += v.second();
                if (v.first() != 0) {
                    if (realTop <= bottomInput) {
                        ++realBottom;
                        for (;realBottom < i; ++realBottom) {
                            for (int x = 0; x < channelsPerUnit; ++x) {
                                *kernel = v.first();
                                ++kernel;
                            }
                        }
                    }
                    else
                        realTop = i;
                    realBottom = i;
                    for (int x = 0; x < channelsPerUnit; ++x) {
                        *kernel = v.first();
                        ++kernel;
                    }
                }
            }
            if (realTop > bottomInput) {
                realTop = bottomInput;
                realBottom = bottomInput;
                for (int x = 0; x < channelsPerUnit; ++x) {
                    *kernel = 0;
                    ++kernel;
                }
            }
            sizes[y] = 1 + realBottom - realTop;
            offsets[y] = realTop;
            float scale = 1/total;
            for (;kernelStart != kernel; ++kernelStart)
                *kernelStart *= scale;
        }
        *inputTop = top;
        *inputBottom = bottom + 1;

        _inputOffsetCount = -top;
    }
    void setBuffers(AlignedBuffer input, AlignedBuffer output)
    {
        _input = input;
        _output = output;
        for (int y = 0; y < _height; ++y)
            _offsets[y] = _offsetCounts[y]*_input.stride();
        _inputOffset = _inputOffsetCount * _input.stride();
    }

private:
    // Buffers
    AlignedBuffer _kernelBuffer;
    Array<int> _offsetCounts;
    Array<int> _offsets;
    Array<int> _kernelSizes;
    AlignedBuffer _input;
    AlignedBuffer _output;

    // Parameters
    int _width;
    int _height;
    int _inputStride;
    int _outputStride;
    int _inputOffsetCount;
    int _inputOffset;
};

#endif // INCLUDED_IMAGE_FILTER_H
