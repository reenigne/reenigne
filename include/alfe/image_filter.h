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
        if (useSSE2()) {
            Byte* inputRow = reinterpret_cast<Byte*>(_input);
            __m128i* outputRow = reinterpret_cast<__m128i*>(_output);
            __m128i* kernel = reinterpret_cast<__m128i*>(_kernelBuffer.data());
            int* offsets = &_offsets[0];
            for (int y = 0; y < _height; ++y) {
                for (int x = 0; x < _width; ++x) {
                    __m128i total = _mm_set1_epi16(0);
                    float* input =
                        reinterpret_cast<float*>(inputRow + offsets[x]);
                    for (int k = 0; k < _kernelSize; ++k) {
                        // We need to use an unaligned load here because we
                        // need it to be possible for any input position to
                        // affect any output position. We could do this by
                        // duplicating each input position eight times, but
                        // this would probably be slower than the unaligned
                        // loads.
                        total = _mm_add_epi16(total, _mm_mullo_epi16(
                            _mm_castps_si128(_mm_loadu_ps(input + k)),
                            *kernel));
                        ++kernel;
                    }
                    outputRow[x] = total;
                }
                inputRow += _inputStride;
                outputRow += _outputStride;
            }
        }
        else {
            UInt16* inputRow = reinterpret_cast<UInt16*>(_input);
            UInt16* outputRow = reinterpret_cast<UInt16*>(_output);
            UInt16* kernel = reinterpret_cast<UInt16*>(_kernelBuffer.data());
            int* offsets = &_offsets[0];
            for (int y = 0; y < _height; ++y) {
                for (int x = 0; x < _width; ++x) {
                    UInt16 total = 0;
                    UInt16* input = inputRow + offsets[x];
                    for (int k = 0; k < _kernelSize; ++k) {
                        total += input[k] * *kernel;
                        ++kernel;
                    }
                    outputRow[x] = total;
                }
                inputRow += _inputStride;
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

// Filters and resamples an image horizontally using single-precision
// floating-point arithmetic.
class ImageFilterHorizontal
{
    void execute()
    {
        if (useSSE2()) {
            Byte* inputRow = reinterpret_cast<Byte*>(_input);
            __m128* outputRow = reinterpret_cast<__m128*>(_output);
            __m128* kernel = reinterpret_cast<__m128*>(_kernelBuffer.data());
            int* offsets = &_offsets[0];
            for (int y = 0; y < _height; ++y) {
                for (int x = 0; x < _width; ++x) {
                    __m128 total = _mm_set1_ps(0.0f);
                    float* input =
                        reinterpret_cast<float*>(inputRow + offsets[x]);
                    for (int k = 0; k < _kernelSize; ++k) {
                        // We need to use an unaligned load here because we
                        // need it to be possible for any input position to
                        // affect any output position. We could do this by
                        // duplicating each input position eight times, but
                        // this would probably be slower than the unaligned
                        // loads.
                        total = _mm_add_ps(total, _mm_mul_ps(
                            _mm_loadu_ps(input + k), *kernel));
                        ++kernel;
                    }
                    outputRow[x] = total;
                }
                inputRow += _inputStride;
                outputRow += _outputStride;
            }
        }
        else {
            float* inputRow = reinterpret_cast<float*>(_input);
            float* outputRow = reinterpret_cast<float*>(_output);
            float* kernel = reinterpret_cast<float*>(_kernelBuffer.data());
            int* offsets = &_offsets[0];
            for (int y = 0; y < _height; ++y) {
                for (int x = 0; x < _width; ++x) {
                    float total = 0;
                    float* input =
                        reinterpret_cast<float*>(inputRow + offsets[x]);
                    for (int k = 0; k < _kernelSize; ++k) {
                        total += input[k] * *kernel;
                        ++kernel;
                    }
                    outputRow[x] = total;
                }
                inputRow += _inputStride;
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
        if (useSSE2()) {
            __m128* inputStart = reinterpret_cast<__m128*>(_input);
            __m128* outputRow = reinterpret_cast<__m128*>(_output);
            __m128* kernel = reinterpret_cast<__m128*>(_kernel);
            int* offsets = &_offsets[0];
            for (int y = 0; y < _height; ++y) {
                __m128* input = inputStart + offsets[y];
                for (int x = 0; x < _width; ++x) {
                    __m128 total = _mm_set1_ps(0.0f);
                    for (int k = 0; k < _kernelSize; ++k) {
                        total =
                            _mm_add_ps(total, _mm_mul_ps(*input, kernel[k]));
                        input += _inputStride;
                    }
                    outputRow[x] = total;
                }
                outputRow += _outputStride;
                kernel += _kernelStride;
            }
        }
        else {
            float* inputStart = reinterpret_cast<float*>(_input);
            float* outputRow = reinterpret_cast<float*>(_output);
            float* kernel = reinterpret_cast<float*>(_kernel);
            int* offsets = &_offsets[0];
            for (int y = 0; y < _height; ++y) {
                float* input = inputStart + offsets[y];
                for (int x = 0; x < _width; ++x) {
                    float total = 0;
                    for (int k = 0; k < _kernelSize; ++k) {
                        total += *input * kernel[k];
                        input += _inputStride;
                    }
                    outputRow[x] = total;
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
