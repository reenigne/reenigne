#include "alfe/main.h"
#include "alfe/bitmap.h"
#include "alfe/complex.h"
#include "alfe/fft.h"

#ifndef INCLUDED_SCANLINES_H
#define INCLUDED_SCANLINES_H

class ScanlineRenderer
{
public:
    ScanlineRenderer() : _rigor(FFTW_MEASURE) { }

    void renderColumn(const DWORD* input, int inputLength, int inputStride,
        DWORD* output, int outputLength, int outputStride)
    {
    }

    int getProfile() { return _profile; }
    void setProfile(int profile) { _profile = profile; }
    float getWidth() { return _width; }
    void setWidth(float width) { _width = width; }
    bool getBleeding() { return _bleeding; }
    void setBleeding(bool bleeding) { _bleeding = bleeding; }
private:
    int _profile;
    float _width;
    bool _bleeding;
    int _rigor;
};

#endif // INCLUDED_SCANLINES_H
