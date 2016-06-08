#include "alfe/main.h"

#ifndef INCLUDED_IMAGE_FILTER_H
#define INCLUDED_IMAGE_FILTER_H

#include <memory>
#include <intrin.h>

bool useSSE2()
{
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
    AlignedBuffer(int x, int y = 1) { ensure(x, y); }
    void ensure(int x, int y = 1)
    {
        int alignment = useSSE2() ? 16 : 4;
        _stride = (x + alignment - 1) & ~(alignment - 1);
        _buffer.ensure(_stride*y + alignment - 1);
        size_t space;
        void* b = static_cast<void*>(&_buffer[0]);
        std::align(alignment, _buffer.count(), b, space);
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
    void execute()
    {
        Byte* input = _input;
        Byte* outputRow = _output.data();
        int* kernelSizes = &_kernelSizes[0];
        if (useSSE2()) {
            __m128i* kernel = reinterpret_cast<__m128i*>(_kernelBuffer.data());
            for (int y = 0; y < _height; ++y) {
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
                            reinterpret_cast<float*>(input + *offsets)))));
                        ++kernel;
                        ++offsets;
                    }
                    output[x] = total;
                }
                input += _inputBuffer.stride();
                outputRow += _output.stride();
            }
        }
        else {
            UInt16* kernel = reinterpret_cast<UInt16*>(_kernelBuffer.data());
            for (int y = 0; y < _height; ++y) {
                UInt16* output = reinterpret_cast<UInt16*>(outputRow);
                int* offsets = &_offsets[0];
                for (int x = 0; x < _width; ++x) {
                    UInt16 total = 0;
                    int kernelSize = kernelSizes[x];
                    for (int k = 0; k < kernelSize; ++k) {
                        total += *reinterpret_cast<UInt16*>(input + *offsets) *
                            *kernel;
                        ++kernel;
                        ++offsets;
                    }
                    output[x] = total;
                }
                input += _inputBuffer.stride();
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
    // kernelFunction's results should sum to 1 for each output pixel channel.
    // In inputLeft we return the leftmost input pixel that will be accessed.
    // In inputRight we return the rightmost input pixel that will be
    // accessed plus one.
    // zoom is number of output pixels per input pixel
    // offset is input position of output pixel 0.
    void generate(Vector outputSize, int inputChannels,
        float* inputChannelPositions, int outputChannels,
        float* outputChannelPositions, float kernelRadius,
        std::function<float(float, int, int)> kernelFunction, int* inputLeft,
        int* inputRight, float zoom, float offset)
    {
        int channelsPerUnit = (useSSE2() ? 8 : 1);
        _height = outputSize.y;
        _width = (outputSize.x*outputChannels + channelsPerUnit - 1)/
            channelsPerUnit;
        _kernelSizes.ensure(_width);
        int kWidth =
            (static_cast<int>(kernelRadius*2 + 1) + channelsPerUnit - 1)/
            channelsPerUnit;
        _kernelBuffer.ensure(kWidth*channelsPerUnit*_width*sizeof(UInt16));
        UInt16* kernel = reinterpret_cast<UInt16*>(_kernelBuffer.data());
        _offsets.ensure(kWidth*_width*sizeof(int));
        _kernelSizes.ensure(_width);
        int* offsets = &_offsets[0];
        int* sizes = &_kernelSizes[0];
        float scale = 128.0f;

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
        int left = std::numeric_limits<int>::max();
        int right = std::numeric_limits<int>::min();
        for (int x = 0; x < _width; ++x) {
            int o = x*channelsPerUnit;
            int kernelSize = 0;

            // Compute leftmost and rightmost possible input positions
            int leftInput = static_cast<int>(offset - kernelRadius + 1 +
                (static_cast<float>(o / outputChannels) +
                minOutputChannelPosition)/zoom + minInputChannelPosition)*
                inputChannels;

            int rightInput = static_cast<int>(offset + kernelRadius +
                (static_cast<float>((o + channelsPerUnit - 1) / outputChannels)
                + maxOutputChannelPosition)/zoom + maxInputChannelPosition)*
                inputChannels;

            for (int i = leftInput; i <= rightInput; ++i) {
                int lastC = 0;
                for (int c = 0; c < channelsPerUnit; ++c) {
                    int ic = i + c;
                    int inputChannel = ic % inputChannels;
                    float inputPosition =
                        static_cast<float>(ic / inputChannels) +
                        inputChannelPositions[inputChannel];
                    int oc = o + c;
                    int outputChannel = oc % outputChannels;
                    float centerInputPixel =
                        (static_cast<float>(oc / outputChannels) +
                        outputChannelPositions[outputChannel])/zoom + offset;
                    float dist = centerInputPixel - inputPosition;
                    int v = 0;
                    if (dist > -kernelRadius && dist < kernelRadius) {
                        v = static_cast<int>(round(kernelFunction(dist,
                            inputChannel, outputChannel)*scale));
                    }
                    if (v != 0) {
                        if (lastC == 0) {
                            left = min(left, i);
                            *offsets = i;
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
                    right = max(right, i + channelsPerUnit);
                }
            }
            sizes[x] = kernelSize;
        }
        *inputLeft = left;
        *inputRight = right;
    }
    void setBuffers(AlignedBuffer inputBuffer, AlignedBuffer output,
        Byte* input)
    {
        _inputBuffer = inputBuffer;
        _input = input;
        _output = output;
    }

    // Buffers
    AlignedBuffer _kernelBuffer;
    Array<int> _offsets;
    Array<int> _kernelSizes;
    AlignedBuffer _inputBuffer;
    AlignedBuffer _output;
    Byte* _input;

    // Parameters
    int _width;
    int _height;
    int _inputStride;
    int _outputStride;
};

// Filters and resamples an image horizontally using single-precision
// floating-point arithmetic.
class ImageFilterHorizontal
{
    void execute()
    {
        Byte* input = _input;
        Byte* outputRow = _output.data();
        int* kernelSizes = &_kernelSizes[0];
        if (useSSE2()) {
            __m128* kernel = reinterpret_cast<__m128*>(_kernelBuffer.data());
            for (int y = 0; y < _height; ++y) {
                __m128* output = reinterpret_cast<__m128*>(outputRow);
                int* offsets = &_offsets[0];
                for (int x = 0; x < _width; ++x) {
                    __m128 total = _mm_set1_ps(0.0f);
                    int kernelSize = kernelSizes[x];
                    for (int k = 0; k < kernelSize; ++k) {
                        // We need to use an unaligned load here because we
                        // need it to be possible for any input position to
                        // affect any output position. We could do this by
                        // duplicating each input position eight times, but
                        // this would probably be slower than the unaligned
                        // loads.
                        total = _mm_add_ps(total, _mm_mul_ps(*kernel,
                            _mm_loadu_ps(reinterpret_cast<float*>(input +
                                *offsets))));
                        ++kernel;
                        ++offsets;
                    }
                    output[x] = total;
                }
                input += _inputBuffer.stride();
                outputRow += _output.stride();
            }
        }
        else {
            float* kernel = reinterpret_cast<float*>(_kernelBuffer.data());
            for (int y = 0; y < _height; ++y) {
                float* output = reinterpret_cast<float*>(outputRow);
                int* offsets = &_offsets[0];
                for (int x = 0; x < _width; ++x) {
                    float total = 0;
                    int kernelSize = kernelSizes[x];
                    for (int k = 0; k < kernelSize; ++k) {
                        total += *reinterpret_cast<float*>(input + *offsets) *
                            *kernel;
                        ++kernel;
                        ++offsets;
                    }
                    output[x] = total;
                }
                input += _inputBuffer.stride();
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
    // kernelFunction's results should sum to 1 for each output pixel channel.
    // In inputLeft we return the leftmost input pixel that will be accessed.
    // In inputRight we return the rightmost input pixel that will be
    // accessed plus one.
    // zoom is number of output pixels per input pixel
    // offset is input position of output pixel 0.
    void generate(Vector outputSize, int inputChannels,
        float* inputChannelPositions, int outputChannels,
        float* outputChannelPositions, float kernelRadius,
        std::function<float(float, int, int)> kernelFunction, int* inputLeft,
        int* inputRight, float zoom, float offset)
    {
        int channelsPerUnit = (useSSE2() ? 8 : 1);
        _height = outputSize.y;
        _width = (outputSize.x*outputChannels + channelsPerUnit - 1)/
            channelsPerUnit;
        _kernelSizes.ensure(_width);
        int kWidth =
            (static_cast<int>(kernelRadius*2 + 1) + channelsPerUnit - 1)/
            channelsPerUnit;
        _kernelBuffer.ensure(kWidth*channelsPerUnit*_width*sizeof(float));
        float* kernel = reinterpret_cast<float*>(_kernelBuffer.data());
        _offsets.ensure(kWidth*_width*sizeof(int));
        _kernelSizes.ensure(_width);
        int* offsets = &_offsets[0];
        int* sizes = &_kernelSizes[0];

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
        int left = std::numeric_limits<int>::max();
        int right = std::numeric_limits<int>::min();
        for (int x = 0; x < _width; ++x) {
            int o = x*channelsPerUnit;
            int kernelSize = 0;

            // Compute leftmost and rightmost possible input positions
            int leftInput = static_cast<int>(offset - kernelRadius + 1 +
                (static_cast<float>(o / outputChannels) +
                minOutputChannelPosition)/zoom + minInputChannelPosition)*
                inputChannels;

            int rightInput = static_cast<int>(offset + kernelRadius +
                (static_cast<float>((o + channelsPerUnit - 1) / outputChannels)
                + maxOutputChannelPosition)/zoom + maxInputChannelPosition)*
                inputChannels;

            for (int i = leftInput; i <= rightInput; ++i) {
                int lastC = 0;
                for (int c = 0; c < channelsPerUnit; ++c) {
                    int ic = i + c;
                    int inputChannel = ic % inputChannels;
                    float inputPosition =
                        static_cast<float>(ic / inputChannels) +
                        inputChannelPositions[inputChannel];
                    int oc = o + c;
                    int outputChannel = oc % outputChannels;
                    float centerInputPixel =
                        (static_cast<float>(oc / outputChannels) +
                        outputChannelPositions[outputChannel])/zoom + offset;
                    float dist = centerInputPixel - inputPosition;
                    float v = 0;
                    if (dist > -kernelRadius && dist < kernelRadius)
                        v = kernelFunction(dist, inputChannel, outputChannel);
                    if (v != 0) {
                        if (lastC == 0) {
                            left = min(left, i);
                            *offsets = i;
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
                    right = max(right, i + channelsPerUnit);
                }
            }
            sizes[x] = kernelSize;
        }
        *inputLeft = left;
        *inputRight = right;
    }
    void setBuffers(AlignedBuffer inputBuffer, AlignedBuffer output,
        Byte* input)
    {
        _inputBuffer = inputBuffer;
        _input = input;
        _output = output;
    }

    // Buffers
    AlignedBuffer _kernelBuffer;
    Array<int> _offsets;
    Array<int> _kernelSizes;
    AlignedBuffer _inputBuffer;
    AlignedBuffer _output;
    Byte* _input;

    // Parameters
    int _width;
    int _height;
    int _inputStride;
    int _outputStride;
};

// Filters and resamples an image vertically using single-precision
// floating-point arithmetic.
class ImageFilterVertical
{
    void execute()
    {
        Byte* inputStart = _input;
        Byte* outputRow = _output.data();
        Byte* kernel = _kernelBuffer.data();
        int* offsets = &_offsets[0];
        int* kernelSizes = &_kernelSizes[0];
        if (useSSE2()) {
            for (int y = 0; y < _height; ++y) {
                Byte* input = inputStart + offsets[y];
                int kernelSize = kernelSizes[y];
                for (int x = 0; x < _width; ++x) {
                    __m128 total = _mm_set1_ps(0.0f);
                    for (int k = 0; k < kernelSize; ++k) {
                        total = _mm_add_ps(total, _mm_mul_ps(
                            reinterpret_cast<__m128*>(kernel)[k],
                            *reinterpret_cast<__m128*>(input)));
                        input += _inputBuffer.stride();
                    }
                    reinterpret_cast<__m128*>(outputRow)[x] = total;
                }
                outputRow += _output.stride();
                kernel += kernelSize*sizeof(__m128);
            }
        }
        else {
            for (int y = 0; y < _height; ++y) {
                Byte* input = inputStart + offsets[y];
                int kernelSize = kernelSizes[y];
                for (int x = 0; x < _width; ++x) {
                    float total = 0;
                    for (int k = 0; k < kernelSize; ++k) {
                        total += reinterpret_cast<float*>(kernel)[k] *
                            *reinterpret_cast<float*>(input);
                        input += _inputBuffer.stride();
                    }
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
    // kernelFunction's results should sum to 1 for each output pixel channel.
    // In inputTop we return the topmost input pixel that will be accessed.
    // In inputBottom we return the bottommost input pixel that will be
    // accessed plus one.
    // zoom is number of output pixels per input pixel
    // offset is input position of output pixel 0.
    void generate(Vector outputSize, float kernelRadius,
        std::function<float(float)> kernelFunction, int* inputTop,
        int* inputBottom, float zoom, float offset)
    {
        int channelsPerUnit = (useSSE2() ? 8 : 1);
        _height = outputSize.y;
        _width = (outputSize.x + channelsPerUnit - 1)/
            channelsPerUnit;
        _kernelSizes.ensure(_height);
        int kWidth = static_cast<int>(kernelRadius*2 + 1);
        _kernelBuffer.ensure(kWidth*channelsPerUnit*_height*sizeof(float));
        float* kernel = reinterpret_cast<float*>(_kernelBuffer.data());
        _offsetCounts.ensure(kWidth*_width*sizeof(int));
        _offsets.ensure(kWidth*_width*sizeof(int));
        _kernelSizes.ensure(_width);
        int* offsets = &_offsetCounts[0];
        int* sizes = &_kernelSizes[0];

        int top = std::numeric_limits<int>::max();
        int bottom = std::numeric_limits<int>::min();
        for (int y = 0; y < _height; ++y) {
            // Compute topmost and bottommost possible input positions
            int topInput = static_cast<int>(offset - kernelRadius + 1 +
                static_cast<float>(y)/zoom);

            int bottomInput = static_cast<int>(offset + kernelRadius +
                static_cast<float>(y)/zoom);

            float centerInputPixel = static_cast<float>(y)/zoom + offset;

            top = min(top, topInput);
            bottom = max(bottom, bottomInput);
            for (int i = topInput; i <= bottomInput; ++i) {
                float dist = centerInputPixel - static_cast<float>(i);
                float v = 0;
                if (dist > -kernelRadius && dist < kernelRadius)
                    v = kernelFunction(dist);
                for (int x = 0; x < channelsPerUnit; ++x) {
                    *kernel = v;
                    ++kernel;
                }
            }
            sizes[y] = 1 + bottomInput - topInput;
            offsets[y] = topInput;
        }
        *inputTop = top;
        *inputBottom = bottom;
    }
    void setBuffers(AlignedBuffer inputBuffer, AlignedBuffer output,
        Byte* input)
    {
        _inputBuffer = inputBuffer;
        _input = input;
        _output = output;
        for (int y = 0; y < _height; ++y)
            _offsets[y] = _offsetCounts[y]*_inputBuffer.stride();
    }

    // Buffers
    AlignedBuffer _kernelBuffer;
    Array<int> _offsetCounts;
    Array<int> _offsets;
    Array<int> _kernelSizes;
    AlignedBuffer _inputBuffer;
    AlignedBuffer _output;
    Byte* _input;

    // Parameters
    int _width;
    int _height;
    int _inputStride;
    int _outputStride;
};

#endif // INCLUDED_IMAGE_FILTER_H
