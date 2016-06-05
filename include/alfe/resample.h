#include "alfe/main.h"

#ifndef INCLUDED_RESAMPLE_H
#define INCLUDED_RESAMPLE_H

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

class Resampler16SISD
{
    void resample()
    {
        UInt16* inputRow = _input;
        UInt16* outputRow = _output;
        for (int y = 0; y < _height; ++y) {
            UInt16* kernel = &_kernel[0];
            int* offset = &_offsets[0];
            for (int x = 0; x < _width; ++x) {
                UInt16 t = 0;
                UInt16* in = inputRow + offset[x];
                for (int k = 0; k < _kernelSize; ++k)
                    t += in[k] * kernel[k];
                outputRow[x] = t;
                kernel += _kernelStride;
            }
            inputRow += _inputStride;
            outputRow += _outputStride;
        }
    }
    Array<UInt16> _kernel;
    Array<int> _offsets;
    int _width;
    int _height;
    int _kernelSize;
    int _kernelStride;
    UInt16* _input;
    UInt16* _output;
    int _inputStride;
    int _outputStride;
};

class Resampler16SIMD
{
    void resample()
    {
        __m128i* inputRow = _input;
        __m128i* outputRow = _output;
        for (int y = 0; y < _height; ++y) {
            __m128i* kernel = &_kernel[0];
            int* offset = &_offsets[0];
            for (int x = 0; x < _width; ++x) {
                __m128i t = _mm_set1_epi16(0);
                __m128i* in = inputRow + offset[x];
                for (int k = 0; k < _kernelSize; ++k)
                    t = _mm_add_epi16(t, _mm_mullo_epi16(in[k], kernel[k]));
                outputRow[x] = t;
                kernel += _kernelStride;
            }
            inputRow += _inputStride;
            outputRow += _outputStride;
        }
    }
    Array<__m128i> _kernel;
    Array<int> _offsets;
    int _width;
    int _height;
    int _kernelSize;
    int _kernelStride;
    __m128i* _input;
    __m128i* _output;
    int _inputStride;
    int _outputStride;
};

class Resampler32SISD
{
    void resample()
    {
        float* inputRow = _input;
        float* outputRow = _output;
        for (int y = 0; y < _height; ++y) {
            float* kernel = &_kernel[0];
            int* offset = &_offsets[0];
            for (int x = 0; x < _width; ++x) {
                float t = 0;
                float* in = inputRow + offset[x];
                for (int k = 0; k < _kernelSize; ++k)
                    t += in[k] * kernel[k];
                outputRow[x] = t;
                kernel += _kernelStride;
            }
            inputRow += _inputStride;
            outputRow += _outputStride;
        }
    }
    Array<float> _kernel;
    Array<int> _offsets;
    int _width;
    int _height;
    int _kernelSize;
    int _kernelStride;
    float* _input;
    float* _output;
    int _inputStride;
    int _outputStride;
};

class Resampler32SIMD
{
    void resample()
    {
        __m128* inputRow = _input;
        __m128* outputRow = _output;
        for (int y = 0; y < _height; ++y) {
            __m128* kernel = &_kernel[0];
            int* offset = &_offsets[0];
            for (int x = 0; x < _width; ++x) {
                __m128 t = _mm_set1_ps(0.0f);
                __m128* in = inputRow + offset[x];
                for (int k = 0; k < _kernelSize; ++k)
                    t = _mm_add_ps(t, _mm_mul_ps(in[k], kernel[k]));
                outputRow[x] = t;
                kernel += _kernelStride;
            }
            inputRow += _inputStride;
            outputRow += _outputStride;
        }
    }
    Array<__m128> _kernel;
    Array<int> _offsets;
    int _width;
    int _height;
    int _kernelSize;
    int _kernelStride;
    __m128* _input;
    __m128* _output;
    int _inputStride;
    int _outputStride;
};

#endif // INCLUDED_RESAMPLE_H
