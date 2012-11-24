#include "alfe/def.h"
#include <intrin.h>

class JIT
{
public:
    JIT() : _memory(0) { }
    ~JIT() { if (_memory != 0) VirtualFree(_memory, 0, MEM_RELEASE); }

    void allocate(int size)
    {
        _size = size;
        _memory = reinterpret_cast<Byte*>(VirtualAlloc(NULL, _size,
            MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    }

    Byte* memory() { return _memory; }

    void finalize()
    {
        IF_ZERO_THROW_LAST_ERROR(VirtualProtect(_memory, _size,
            PAGE_EXECUTE_READ));
        IF_ZERO_THROW_LAST_ERROR(FlushInstructionCache(GetCurrentProcess(),
            _memory, _size));
    }

    void execute(void* context)
    {
        typedef void (*FunctionPointer)(void*);
        FunctionPointer f = reinterpret_cast<FunctionPointer>(_memory);
        f(context);
    }

private:
    Byte* _memory;
    int _size;
};

class CPUInfo
{
public:
    CPUInfo()
    {
        int info[4];

        __cpuid(info, 0);
        int c = info[0] + 1;
        _info.resize(c * 4);
        for (int i = 0; i < c; ++i) {
            __cpuid(info, i);
            for (int j = 0; j < 4; ++j)
                _info[i*4 + j] = info[j];
        }

        __cpuid(info, 0x80000000);
        c = info[0];
        _extendedInfo.resize(c * 4);
        for (int i = 0; i < c; ++i) {
            __cpuid(info, 0x80000000 | i);
            for (int j = 0; j < 4; ++j)
                _extendedInfo[i*4 + j] = info[j];
        }

        _x87 = boolValue(_info[3], 0);
        _mmx = boolValue(_info[3], 23);
        _sse = boolValue(_info[3], 25);
        _sse2 = boolValue(_info[3], 26);
        _sse3 = boolValue(_info[2], 0);
        _ssse3 = boolValue(_info[2], 9);
        _sse41 = boolValue(_info[2], 19);
        _sse42 = boolValue(_info[2], 20);
        _avx = boolValue(info[2], 28);
    }

private:
    int intValue(int value, int lowBit, int highBit)
    {
        return (value >> lowBit) & ((1 << (highBit + 1 - lowBit)) - 1);
    }

    bool boolValue(int value, int bit)
    {
        return (value & (1 << bit)) != 0;
    }

    std::vector<int> _info;
    std::vector<int> _extendedInfo;

    bool _x87, _mmx, _sse, _sse2, _sse3, _ssse3, _sse41, _sse42, _avx;
};