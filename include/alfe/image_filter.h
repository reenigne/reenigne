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
    AlignedBuffer(int n) { ensure(n); }
    void ensure(int n)
    {
        int alignment = useSSE2() ? 16 : 4;
        _buffer.ensure(n + alignment - 1);
        size_t space;
        void* b = static_cast<void*>(&_buffer[0]);
        std::align(alignment, _buffer.count(), b, space);
        _aligned = static_cast<Byte*>(b);
    }
    Byte* data() { return _aligned; }
private:
    Array<Byte> _buffer;
    Byte* _aligned;
};

// Filters an image horizontally using 16-bit integer arithmetic.
class ImageFilter16
{
    void execute()
    {
        Byte* input = _input;
        Byte* outputRow = _output;
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
                input += _inputStride;
                outputRow += _outputStride;
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
                input += _inputStride;
                outputRow += _outputStride;
            }
        }
    }
    // outputSize.x is in positions (a position is a pixel/channel combination)
    // kernelWidth is in input positions: range is (-kernelWidth, kernelWidth)
    // kernelFunction's arguments are inputPosition, outputPosition.
    // kernelFunction's results should sum to 1 for each outputPosition.
    // In inputLeft we return the leftmost inputPosition that will be accessed.
    // In inputRight we return the rightmost inputPosition that will be
    // accessed plus one.
    // zoom is number of output positions per input position
    // offset is inputPosition of outputPosition 0.
    void generate(Vector outputSize, float kernelWidth,
        std::function<float(int, int)> kernelFunction, int* inputLeft,
        int* inputRight, float zoom, float offset)
    {
        int pixelAlign = (useSSE2() ? 8 : 1);
        _height = outputSize.y;
        _width = (outputSize.x + pixelAlign - 1)/pixelAlign;
        _kernelSizes.ensure(_width);
        int kWidth = static_cast<int>(kernelWidth*2 + 1);
        _kernelBuffer.ensure(kWidth*pixelAlign*_width*sizeof(UInt16));
        _offsets.ensure(kWidth*_width*sizeof(int));
        _kernelSizes.ensure(_width);
        if (useSSE2()) {
        }
        else {
            UInt16* kernel = reinterpret_cast<UInt16*>(_kernelBuffer.data());
            float scale = 32767.0f/255.0f;
            for (int x = 0; x < _width; ++x) {
                //int offset = x - kWidth;
                //_offsets[x] = offset;
                float inputPosition = offset + x/zoom;
                int minK = kWidth;
                int maxK = -1;
                for (int k = 0; k < kWidth; ++k) {
                    int v =
                        static_cast<int>(kernelFunction(offset + k, x)*scale);
                    if (v != 0) {
                        maxK = max(maxK, k);
                        minK = min(minK, k);
                    }
                    if (maxK >= 0) {
                        *kernel = v;
                        ++kernel;
                    }
                }
            }

        }
    }
    // Strides are measured in bytes.
    void setBuffers(Byte* input, int inputStride, Byte* output,
        int outputStride)
    {
        _input = input;
        _inputStride = inputStride;
        _output = output;
        _outputStride = outputStride;
    }

    // Buffers
    AlignedBuffer _kernelBuffer;
    Array<int> _offsets;
    Array<int> _kernelSizes;

    // Correctly-aligned pointers
    Byte* _input;
    Byte* _output;

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
        Byte* outputRow = _output;
        if (useSSE2()) {
            __m128* kernel = reinterpret_cast<__m128*>(_kernelBuffer.data());
            for (int y = 0; y < _height; ++y) {
                __m128* output = reinterpret_cast<__m128*>(outputRow);
                int* offsets = &_offsets[0];
                for (int x = 0; x < _width; ++x) {
                    __m128 total = _mm_set1_ps(0.0f);
                    for (int k = 0; k < _kernelSize; ++k) {
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
                input += _inputStride;
                outputRow += _outputStride;
            }
        }
        else {
            float* kernel = reinterpret_cast<float*>(_kernelBuffer.data());
            for (int y = 0; y < _height; ++y) {
                float* output = reinterpret_cast<float*>(outputRow);
                int* offsets = &_offsets[0];
                for (int x = 0; x < _width; ++x) {
                    float total = 0;
                    for (int k = 0; k < _kernelSize; ++k) {
                        total += *reinterpret_cast<float*>(input + *offsets) *
                            *kernel;
                        ++kernel;
                        ++offsets;
                    }
                    output[x] = total;
                }
                input += _inputStride;
                outputRow += _outputStride;
            }
        }
    }
    void generate(int width, int height, Byte* input, int inputStride,
        Byte* output, int outputStride, int kernelWidth,
        std::function<float(float)> kernel)
    {
        _height = height;
        _width = width;
    }

    // Buffers
    AlignedBuffer _kernelBuffer;
    Array<int> _offsets;

    // Correctly-aligned pointers
    Byte* _kernel;
    Byte* _input;
    Byte* _output;

    // Parameters
    int _width;
    int _height;
    int _kernelSize;
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
        Byte* outputRow = _output;
        Byte* kernel = _kernelBuffer.data();
        int* offsets = &_offsets[0];
        if (useSSE2()) {
            for (int y = 0; y < _height; ++y) {
                Byte* input = inputStart + offsets[y];
                for (int x = 0; x < _width; ++x) {
                    __m128 total = _mm_set1_ps(0.0f);
                    for (int k = 0; k < _kernelSize; ++k) {
                        total = _mm_add_ps(total, _mm_mul_ps(
                            reinterpret_cast<__m128*>(kernel)[k],
                            *reinterpret_cast<__m128*>(input)));
                        input += _inputStride;
                    }
                    reinterpret_cast<__m128*>(outputRow)[x] = total;
                }
                outputRow += _outputStride;
                kernel += _kernelStride;
            }
        }
        else {
            for (int y = 0; y < _height; ++y) {
                Byte* input = inputStart + offsets[y];
                for (int x = 0; x < _width; ++x) {
                    float total = 0;
                    for (int k = 0; k < _kernelSize; ++k) {
                        total += reinterpret_cast<float*>(kernel)[k] *
                            *reinterpret_cast<float*>(input);
                        input += _inputStride;
                    }
                    reinterpret_cast<float*>(outputRow)[x] = total;
                }
                outputRow += _outputStride;
                kernel += _kernelStride;
            }
        }
    }
    void generate(int width, int height, Byte* input, int inputStride,
        Byte* output, int outputStride, int kernelWidth,
        std::function<float(float)> kernel)
    {
        _height = height;
        _width = width;
    }

    // Buffers
    AlignedBuffer _kernelBuffer;
    Array<int> _offsets;

    // Correctly-aligned pointers
    Byte* _kernel;
    Byte* _input;
    Byte* _output;

    // Parameters
    int _width;
    int _height;
    int _kernelSize;
    int _kernelStride;
    int _inputStride;
    int _outputStride;
};

#endif // INCLUDED_IMAGE_FILTER_H
