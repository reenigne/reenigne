#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <vector>
#include <time.h>
#include "alfe/main.h"
#include "alfe/user.h"
#include "alfe/pipes.h"
#include "alfe/directx.h"

static unsigned int fastRandomData[55];
static int fastRandom1, fastRandom2;

unsigned int fastRandom()
{
    int r = fastRandomData[fastRandom1] + fastRandomData[fastRandom2];
    fastRandomData[fastRandom1] = r;
    if (++fastRandom1 >= 55)
        fastRandom1 = 0;
    if (++fastRandom2 >= 55)
        fastRandom2 = 0;
    return r;
}

void fastRandomInit()
{
    srand(static_cast<unsigned int>(time(0)));
    for (int i = 0; i < 55; ++i)
        fastRandomData[i] = (rand() & 0xff) | ((rand() & 0xff)<<8) | 
            ((rand() & 0xff)<<16) | ((rand() & 0xff)<<24);
    fastRandom1 = rand() % 55;
    fastRandom2 = (fastRandom1 + 24) % 55;
}

typedef unsigned char Sample;  // Sync = 4, Blank = 60, Black = 70, White = 200


template<class Data> class NTSCSource : public PeriodicSource<Sample>
{
public:
    NTSCSource(Data* data, int n = defaultSampleCount)
      : PeriodicSource(data, fastRandom()%data->length(), n)
    { }
};


class NoisePipe : public Pipe<Sample, Sample, NoisePipe>
{
public:
    NoisePipe(int level, int n = defaultSampleCount)
      : Pipe(this, (n + 63)&~63)  // Process 64 samples at once
    {
        float l = static_cast<float>(level);
        float m = static_cast<float>(65536 - level);
        int s = static_cast<int>(sqrt(l*l + m*m));
        _noiseLevel = 256*level/s;
        _signalLevel = 32768*(65536 - level)/s;
        // We wish to avoid calling the random number generator each
        _noise.resize(1024);
        _noise0 = &_noise[0];
        _last = static_cast<int>(fastRandom())>>24;
        for (int i = 0; i < 1024; ++i) {
            int current = static_cast<int>(fastRandom())>>24;
            _noise[i] = current*_last*_noiseLevel;
            _last = current;
        }
    }
    void produce(int n)
    {
        n = (n + 63)&~63;
        Accessor<Sample> reader = _sink.reader(n);
        Accessor<Sample> writer = _source.writer(n);
        for (int i = 0; i < (n>>6); ++i) {
            int p = fastRandom()&1023;
            for (int j = 0; j < 64; ++j) {
                int noise = _noise0[p];
                p = (p + 1)&1023;
                int signal = static_cast<int>(reader.item()) - 60;
                writer.item() = 
                    clamp(1, ((noise + signal*_signalLevel)>>15) + 60, 254);
            }
        }
        _source.written(n);
        _sink.read(n);
    }
private:
    int _last;
    int _noiseLevel;
    int _signalLevel;
    std::vector<int> _noise;
    int* _noise0;
};


// A filter which adds RF cable ghosting to a signal.
class GhostingPipe : public Pipe<Sample, Sample, GhostingPipe>
{
public:
    // The inner loop always processes 4 samples at once, so round up.
    GhostingPipe(int n = defaultSampleCount)
      : Pipe(this, (n + 3)&~3)  
    {
        for (int i = 0; i < 4; ++i)
            _d[i] = 0;
    }
    void produce(int n)
    {
        n = (n + 3)&~3;
        Accessor<Sample> reader = _sink.reader(n);
        Accessor<Sample> writer = _source.writer(n);
        for (int i = 0; i < (n>>2); ++i) {
            int s0 = static_cast<int>(reader.item()) - 60;
            int s1 = static_cast<int>(reader.item()) - 60;
            int s2 = static_cast<int>(reader.item()) - 60;
            int s3 = static_cast<int>(reader.item()) - 60;
            int g = -11*_d[2] + 5*_d[3];
            _d[3] = _d[2];
            _d[2] = _d[1];
            _d[1] = _d[0];
            _d[0] = s0 + s1 + s2 + s3;

            writer.item() = clamp(1, ((s0*278 + g)>>8) + 60, 254);
            writer.item() = clamp(1, ((s1*278 + g)>>8) + 60, 254);
            writer.item() = clamp(1, ((s2*278 + g)>>8) + 60, 254);
            writer.item() = clamp(1, ((s3*278 + g)>>8) + 60, 254);
        }
        _sink.read(n);
        _source.written(n);
    }
private:
    int _d[4];
};


// A filter which occasionally drops the signal on the floor.
template<class T> class DropOutPipe : public Pipe<T, T, DropOutPipe<T> >
{
public:
    DropOutPipe(T dropOutValue, unsigned int outFrequency,
        unsigned int inFrequency, int n = defaultSampleCount)
      : Pipe(this, (n + 63)&~63),  // Process 64 samples at once
        _dropOutValue(dropOutValue),
        _outFrequency(outFrequency),
        _inFrequency(inFrequency),
        _out(false)
    { }
    void produce(int n)
    {
        n = (n + 63)&~63;
        Accessor<Sample> reader = _sink.reader(n);
        Accessor<Sample> writer = _source.writer(n);
        for (int i = 0; i < (n>>6); ++i) {
            if (fastRandom() < _outFrequency)
                _out = true;
            if (fastRandom() < _inFrequency)
                _out = false;
            if (_out)
                for (int j = 0; j < 64; ++j)
                    writer.item() = _dropOutValue;
            else
                writer.items(CopyFrom<Buffer<Sample> >(reader), 64);
            reader.advance(64);
        }
        _sink.read(n);
        _source.written(n);
    }
private:
    unsigned int _outFrequency;
    unsigned int _inFrequency;
    bool _out;
    T _dropOutValue;
};


class ShadowMask : public Image
{
public:
    void create(Device* device)
    {
        _device = device;
        // Ideally we'd like the vertical size of our mask texture to be
        // smaller than the horizontal size by a factor of sqrt(3). However,
        // some GPUs can't cope with D3DTADDRESS_WRAP of textures that are not
        // a power of two in each dimension.
        _size = Vector(8, 8); //128, 128);
        _d = 2.0;
        float scale = 1.0f;

        _texture.create(device, _size);
        _cpuTexture.create(device, _size);
        {
            CPUTextureLock lock(&_cpuTexture);
            Byte* data = lock.data();
            int pitch = lock.stride();
            float d2 = _d*_d*2.0f;
            for (int y = 0; y < _size.y; ++y) {
                DWord* p = reinterpret_cast<DWord*>(data);
                for (int x = 0; x < _size.x; ++x) {
    #if 0  // Shadow mask
                    float h = sqrt(3.0f)/2.0f;
                    Vector2<float> z(static_cast<float>(x)*3.0f/_size.x,
                        static_cast<float>(y)*sqrt(3.0f)/_size.y);
                    if (z.y > h)
                        z.y = h + h - z.y;
                    float ba2 = (z - Vector2<float>(-0.5f, h)).modulus2();
                    float ga2 = (z - Vector2<float>(0, 0)).modulus2();
                    float ra2 = (z - Vector2<float>(0.5f, h)).modulus2();
                    float bb2 = (z - Vector2<float>(1.0f, 0)).modulus2();
                    float gb2 = (z - Vector2<float>(1.5f, h)).modulus2();
                    float rb2 = (z - Vector2<float>(2.0f, 0)).modulus2();
                    float bc2 = (z - Vector2<float>(2.5f, h)).modulus2();
                    float gc2 = (z - Vector2<float>(3.0f, 0)).modulus2();
                    float rc2 = (z - Vector2<float>(3.5f, h)).modulus2();
                    float r2 = min(ra2, rb2, rc2);
                    float g2 = min(ga2, gb2, gc2);
                    float b2 = min(ba2, bb2, bc2);
    #else
                    float z = static_cast<float>(x)*3.0f/_size.x;
                    float rb2 = (z + 1.0f)*(z + 1.0f);
                    float ga2 = z*z;
                    float ba2 = (z - 1.0f)*(z - 1.0f);
                    float ra2 = (z - 2.0f)*(z - 2.0f);
                    float gb2 = (z - 3.0f)*(z - 3.0f);
                    float bb2 = (z - 4.0f)*(z - 4.0f);
                    float r2 = min(ra2, rb2);
                    float g2 = min(ga2, gb2);
                    float b2 = min(ba2, bb2);
    #endif
                    int r = clamp(0,
                        static_cast<int>(255.0f*exp(-2.2f*r2/d2)), 255);
                    int g = clamp(0,
                        static_cast<int>(255.0f*exp(-2.2f*g2/d2)), 255);
                    int b = clamp(0,
                        static_cast<int>(255.0f*exp(-2.2f*b2/d2)), 255);
                    *(p++) = 0xff000000 | (r<<16) | (g<<8) | b;
                }
                data += pitch;
            }
        }

        _geometry.create(device, 4, D3DPT_TRIANGLEFAN);
        _geometry.setVertexUV(0, Vector2<float>(0.0f, 0.0f));
        _geometry.setVertexUV(1, Vector2<float>(308.0f/scale, 0.0f));
        _geometry.setVertexUV(2, Vector2<float>(308.0f/scale, 400.0f/scale));
        _geometry.setVertexUV(3, Vector2<float>(0.0f, 400.0f/scale));
    }

    void draw()
    {
        {
            GeometryLock lock(&_geometry);
            lock.setXY(0, Vector2<float>(0.0f, 0.0f));
            lock.setXY(1, Vector2<float>(static_cast<float>(_windowSize.x), 0.0f));
            lock.setXY(2, Vector2Cast<float>(_windowSize));
            lock.setXY(3, Vector2<float>(0.0f, static_cast<float>(_windowSize.y)));
        }

        IF_ERROR_THROW_DX(_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_DESTCOLOR));
        IF_ERROR_THROW_DX(_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO));
        IF_ERROR_THROW_DX(_device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD));

        _texture.setTexture();
        _geometry.setStreamSource();

        IF_ERROR_THROW_DX(_device->DrawPrimitive(
            D3DPT_TRIANGLEFAN,    // PrimitiveType
            0,                    // StartVertex
            2));                  // PrimitiveCount
    }

    void resize(Vector windowSize) { _windowSize = windowSize; }
private:
    Vector _windowSize;
    Vector _size;
    float _d;
    std::vector<DWord> _image;
    Device* _device;
    GPUTexture _texture;
    CPUTexture _cpuTexture;
    Geometry _geometry;
};


class ScanLines : public Image
{
public:
    void create(Device* device, float lineTop, float linesVisible)
    {
        _geometry.create(device, 4, D3DPT_TRIANGLEFAN);
        _texture.create(device, Vector(1, 1));
        _cpuTexture.create(device, Vector(1, 1));
        _height = 1;
        _device = device;
        _lineTop = lineTop;
        _linesVisible = linesVisible;

        // TODO: Make this user-settable
        _scanLineHeight = 1.7f;
    }

    void draw(float phase)
    {
        if (_height < _windowSize.y) {
            _texture.destroy();
            _texture.create(_device, Vector(1, _windowSize.y));
            _height = _windowSize.y;
        }
        _texture.lock();
        Byte* data = _texture.data();
        int pitch = _texture.pitch();
        int maxCoefficient = 1;
        for (int row = 0; row < _windowSize.y; ++row) {
            int line = static_cast<int>(0.5f + _lineTop + phase +
                static_cast<float>(row)*_linesVisible/static_cast<float>(_windowSize.y));
            // Determine the coefficient of scanline "line" in pixel row
            // "row". We model each scanline vertically as a Gaussian with
            // user-settable width (corresponding to a focus control).
            // To improve performance, scanlines do not overlap - we only
            // plot the scanline with the largest coefficient on any given
            // pixel row.
            float scale = 1;
            if (_scanLineHeight < 4.0f*_linesVisible/_windowSize.y)
                scale = _scanLineHeight*_windowSize.y/(4.0f*_linesVisible);
            float position = static_cast<float>(line) - phase - _lineTop;
            float t = static_cast<float>(M_PI)*(position - row*_linesVisible/_windowSize.y)/(_scanLineHeight/scale);
            int c = static_cast<int>(exp(-t*t/2.0f)*255.0f*scale);
            maxCoefficient = max(maxCoefficient, c);
            *reinterpret_cast<DWord*>(data) = c;
            data += pitch;
        }
        data = _texture.data();
        for (int row = 0; row < _windowSize.y; ++row) {
            DWord* p = reinterpret_cast<DWord*>(data);
            int t = ((*p)*255)/maxCoefficient;
            *p = ((255 - t)<<24) | ((31-(t>>3))<<16) | ((31-(t>>3))<<8) | (31-(t>>3));
            data += pitch;
        }
        _texture.unlock();

        _geometry.lock();
        _geometry.setVertex(0, Vector2<float>(0.0f, 0.0f));
        _geometry.setVertex(1, Vector2<float>(static_cast<float>(_windowSize.x), 0.0f));
        _geometry.setVertex(2, Vector2<float>(static_cast<float>(_windowSize.x), static_cast<float>(_height)));
        _geometry.setVertex(3, Vector2<float>(0.0f, static_cast<float>(_height)));
        _geometry.unlock();

#if 0  // glow
        IF_ERROR_THROW_DX(_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE));
        IF_ERROR_THROW_DX(_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE));
        IF_ERROR_THROW_DX(_device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_REVSUBTRACT));
#else
        IF_ERROR_THROW_DX(_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO));
        IF_ERROR_THROW_DX(_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA));
#endif

        _texture.setTexture();
        _geometry.setStreamSource();

        IF_ERROR_THROW_DX(_device->DrawPrimitive(
            D3DPT_TRIANGLEFAN,  // PrimitiveType
            0,                  // StartVertex
            2));                // PrimitiveCount
    }

    void resize(Vector windowSize) { _windowSize = windowSize; }
private:
    Device* _device;
    CPUTexture _cpuTexture;
    GPUTexture _texture;
    Geometry _geometry;
    int _height;
    Vector _windowSize;
    float _scanLineHeight;
    float _lineTop;
    float _linesVisible;
};


class CompositeMonitor : public Sink<Sample>
{
public:
    CompositeMonitor()
      : Sink(1),
        _frames(0),
        _phase(0),
        _foundVerticalSync(false),
        _line(0),
        _baseLoad(0.5),
        _verticalSync(0),
        _hysteresisCount(0),
        _colorMode(false),

        // TODO: make these user-settable
        _brightness(0.06f),
        _contrast(3.0f),
        _saturation(0.7f),
        _tint(18.0f),
        _horizontalSize(0.95f),
        _horizontalPosition(0),
        _verticalSize(0.93f),
        _verticalPosition(-0.01f),
        _verticalHold(280),
        _horizontalHold(25),
        _bloomFactor(10.0f)
    {
        float samplesPerSecond = 157500000.0f/11.0f;
        float us = samplesPerSecond/1000000.0f;  // samples per microsecond

        // Horizontal times in samples.
        float sync = 4.7f*us;
        float breezeway = 0.6f*us;
        _colorBurstStart = static_cast<int>(sync + breezeway);
        float colorBurst = 2.5f*us;
        float backPorch = 1.6f*us;
        float frontPorch = 1.5f*us;
        float blanking = sync + breezeway + colorBurst + backPorch + frontPorch;
        float line = 910.0f;
        _active = line - blanking;
        _preActive = blanking - frontPorch;
        // The following parameter specifies how many samples early or late the
        // horizontal sync pulse can be and still be recognized (assuming good
        // signal fidelity). This sets the angle of the diagonal lines that
        // vertical lines become when horizontal sync is lost.
        _driftSamples = 8;
        _minSamplesPerLine = static_cast<int>(line - _driftSamples);
        _maxSamplesPerLine = static_cast<int>(line + _driftSamples);
        // We always consume a scanline at a time, and we won't be called to
        // process until we're sure we have a scanline.
        _n = _maxSamplesPerLine;
        _linePeriod = static_cast<int>(line);

        // Vertical times in lines.
        float preSyncLines = 3.0f;
        float syncLines = 3.0f;
        float postSyncLines = 14.0f;
        float lines = 262.5f;
        float blankingLines = preSyncLines + syncLines + postSyncLines;
        float activeLines = lines - blankingLines;
        _linesVisible = activeLines*_verticalSize;
        _lineTop = postSyncLines + activeLines*(0.5f + _verticalPosition - _verticalSize/2.0f);
        // The following parameter specifies how many lines early or late the
        // vertical sync pulse can be and still be recognized (assuming good
        // signal fidelity). This sets the "roll speed" of the picture when
        // vertical sync is lost. Empirically determined from video of an IBM
        // 5153 monitor.
        _driftLines = 14;
        _minLinesPerField = static_cast<int>(lines - _driftLines);
        _maxLinesPerField = static_cast<int>(lines + _driftLines);

        _lefts.resize(_maxLinesPerField);
        _widths.resize(_maxLinesPerField);

        for (int i = 0; i < 4; ++i)
            _colorBurstPhase[i] = _lockedColorBurstPhase[i] = 0;

        _crtLoad = _baseLoad;

        _delay.resize(19 + _maxSamplesPerLine);

        _gamma.resize(256);
        for (int i = 0; i < 256; ++i)
            _gamma[i] = static_cast<int>(pow(static_cast<float>(i)/255.0f, 1.9f)*255.0f);
        _gamma0 = &_gamma[0];
    }

    // Returns the top row for a scanline
    int topRow(int line)
    {
        return static_cast<int>(
            (static_cast<float>(line) - _verticalSyncPhase - _lineTop - 0.5f)*
             static_cast<float>(_windowSize.y)/_linesVisible + 1.0f);
    }

    // We always process a single scanline here.
    void consume(int n)
    {
        Accessor<Sample> reader = Sink::reader(n);

        // Find the horizontal sync position.
        int offset = 0;
        for (int i = 0; i < _driftSamples*2; ++i, ++offset)
            if (static_cast<int>(reader.item(offset)) + static_cast<int>(reader.item(offset + 1)) < _horizontalHold*2)
                break;
        // We use a phase-locked loop like real hardware does, in order to
        // avoid losing horizontal sync if the pulse is missing for a line or
        // two, and so that we get the correct "wobble" behavior.
        int linePeriod = _maxSamplesPerLine - offset;
        _linePeriod = (2*_linePeriod + linePeriod)/3;
        _linePeriod = clamp(_minSamplesPerLine, _linePeriod, _maxSamplesPerLine);
        offset = _maxSamplesPerLine - _linePeriod;

        // Find the vertical sync position.
        if (!_foundVerticalSync)
            for (int j = 0; j < _maxSamplesPerLine; j += 57) {
                _verticalSync = ((_verticalSync*232)>>8) + static_cast<int>(reader.item(j)) - 60;
                if (_verticalSync < -_verticalHold || _line == 2*_driftLines) {
                    // To render interlaced signals correctly, we need to
                    // figure out where the vertical sync pulse happens
                    // relative to the horizontal pulse. This determines the
                    // vertical position of the raster relative to the screen.
                    _verticalSyncPhase = static_cast<float>(j)/static_cast<float>(_maxSamplesPerLine);
                    // Now we can find out which scanlines are at the top and
                    // bottom of the screen.
                    _topLine = static_cast<int>(0.5f + _lineTop + _verticalSyncPhase);
                    _bottomLine = static_cast<int>(1.5f + _linesVisible + _lineTop + _verticalSyncPhase);
                    _line = 0;
                    _foundVerticalSync = true;
                    break;
                }
            }

        // Determine the phase and strength of the color signal from the color
        // burst, which starts shortly after the horizontal sync pulse ends.
        // The color burst is 9 cycles long, and we look at the middle 5
        // cycles.
        int p = offset&~3;
        for (int i = _colorBurstStart + 8; i < _colorBurstStart + 28; ++i) {
            static const float colorBurstFadeConstant = 1.0f/128.0f;
            _colorBurstPhase[(i + _phase)&3] =
                _colorBurstPhase[(i + _phase)&3]*(1.0f - colorBurstFadeConstant) +
                (static_cast<int>(reader.item(p + i)) - 60)*colorBurstFadeConstant;
        }
        float total = 0.1f;
        for (int i = 0; i < 4; ++i)
            total += _colorBurstPhase[i]*_colorBurstPhase[i];
        float colorBurstGain = 32.0f/sqrt(total);
        int phaseCorrelation = (offset + _phase)&3;
        float colorBurstI = colorBurstGain*(_colorBurstPhase[2] - _colorBurstPhase[0])/16.0f;
        float colorBurstQ = colorBurstGain*(_colorBurstPhase[3] - _colorBurstPhase[1])/16.0f;
        float hf = colorBurstGain*(_colorBurstPhase[0] - _colorBurstPhase[1] + _colorBurstPhase[2] - _colorBurstPhase[3]);
        bool colorMode = (colorBurstI*colorBurstI + colorBurstQ*colorBurstQ) > 2.8 && hf < 16.0f;
        if (colorMode)
            for (int i = 0; i < 4; ++i)
                _lockedColorBurstPhase[i] = _colorBurstPhase[i];
        // Color killer hysteresis: We only switch between colour mode and
        // monochrome mode if we stay in the new mode for 128 consecutive
        // lines.
        if (_colorMode != colorMode) {
            _hysteresisCount++;
            if (_hysteresisCount == 128) {
                _colorMode = colorMode;
                _hysteresisCount = 0;
            }
        }
        else
            _hysteresisCount = 0;

        if (_foundVerticalSync && _line >= _topLine && _line < _bottomLine) {
            int y = _line - _topLine;

            // Lines with high amounts of brightness cause more load on the
            // horizontal oscillator which decreases horizontal deflection,
            // causing "blooming" (increase in width).
            int totalSignal = 0;
            for (int i = 0; i < _active; ++i)
                totalSignal += static_cast<int>(reader.item(offset + i)) - 60;
            _crtLoad = 0.4f*_crtLoad + 0.6f*(_baseLoad + (totalSignal - 42000.0f)/140000.0f);
            float bloom = clamp(-2.0f, _bloomFactor*_crtLoad, 10.0f);
            float horizontalSize = (1.0f - 6.3f*bloom/_active)*_horizontalSize;
            float samplesVisible = _active*horizontalSize;
            float sampleLeft = _preActive + _active*(0.5f + _horizontalPosition - horizontalSize/2.0f);
            _lefts[y] = sampleLeft;
            _widths[y] = samplesVisible;

            int start = max(static_cast<int>(sampleLeft) - 10, 0);
            int end = min(static_cast<int>(sampleLeft + samplesVisible) + 10, _maxSamplesPerLine - offset);
            int brightness = static_cast<int>(_brightness*100.0 - 7.5f*256.0f*_contrast)<<8;
            DWord* destination = reinterpret_cast<DWord*>(_dataData + y*_dataPitch) + start;

            if (_colorMode) {
                int yContrast = static_cast<int>(_contrast*1463.0f);
                float radians = static_cast<float>(M_PI)/180.0f;
                float tintI = -cos((103.0f + _tint)*radians);
                float tintQ = sin((103.0f + _tint)*radians);
                int iqMultipliers[4];
                float colorBurstI = _lockedColorBurstPhase[(2 + phaseCorrelation)&3] - _lockedColorBurstPhase[(0 + phaseCorrelation)&3];
                float colorBurstQ = _lockedColorBurstPhase[(3 + phaseCorrelation)&3] - _lockedColorBurstPhase[(1 + phaseCorrelation)&3];
                iqMultipliers[0] = static_cast<int>((colorBurstI*tintI - colorBurstQ*tintQ)*_saturation*_contrast*colorBurstGain*0.352f);
                iqMultipliers[1] = static_cast<int>((colorBurstQ*tintI + colorBurstI*tintQ)*_saturation*_contrast*colorBurstGain*0.352f);
                iqMultipliers[2] = -iqMultipliers[0];
                iqMultipliers[3] = -iqMultipliers[1];
                int* p = &_delay[_maxSamplesPerLine];
                for (int x = 0; x < 19; ++x)
                    p[x] = 0;
                int sp = offset + start;
                for (int x = start; x < end; ++x, --p) {
                    // We use a low-pass Finite Impulse Response filter to
                    // remove high frequencies (including the color carrier
                    // frequency) from the signal. We could just keep a
                    // 4-sample running average but that leads to sharp edges
                    // in the resulting image.
                    int s = static_cast<int>(reader.item(sp++)) - 60;
                    p[0] = s;
                    int y = (p[6] + p[0] + ((p[5] + p[1])<<2) + 7*(p[4] + p[2]) + (p[3]<<3))*yContrast + brightness;
                    p[6] = s*iqMultipliers[x&3];
                    int i = p[12] + p[6] + ((p[11] + p[7])<<2) + 7*(p[10] + p[8]) + (p[9]<<3);
                    p[12] = s*iqMultipliers[(x + 3)&3];
                    int q = p[18] + p[12] + ((p[17] + p[13])<<2) + 7*(p[16] + p[14]) + (p[15]<<3);
                    int r = _gamma0[clamp(0, (y + 243*i + 160*q)>>16, 255)];
                    int g = _gamma0[clamp(0, (y -  71*i - 164*q)>>16, 255)];
                    int b = _gamma0[clamp(0, (y - 283*i + 443*q)>>16, 255)];
                    *(destination++) = 0xff000000 | (r<<16) | (g<<8) | b;
                }
            }
            else {
                int sp = offset + start;
                int yContrast = static_cast<int>(_contrast*46816.0f);
                for (int x = start; x < end; ++x) {
                    int s = static_cast<int>(reader.item(sp++)) - 60;
                    int y = _gamma0[clamp(0, (s*yContrast + brightness)>>16, 255)];
                    *(destination++) = 0xff000000 | (y<<16) | (y<<8) | y;
                }
            }
        }
        offset += _minSamplesPerLine;
        _phase = (_phase + offset)&3;
        read(offset);

        ++_line;
        if (_foundVerticalSync && _line == _minLinesPerField)
            postField();
    }

    void postField()
    {
        int lines = _bottomLine - _topLine;
        _dataGeometry.lock();
        for (int y = 0; y < lines; ++y) {
            int line = y + _topLine;
            float top = static_cast<float>(topRow(line));
            float bottom = static_cast<float>(topRow(line + 1));
            _dataGeometry.setVertex(y*4,     Vector2<float>(0.0f, top));
            _dataGeometry.setVertex(y*4 + 1, Vector2<float>(static_cast<float>(_windowSize.x), top));
            _dataGeometry.setVertex(y*4 + 2, Vector2<float>(0.0f, bottom));
            _dataGeometry.setVertex(y*4 + 3, Vector2<float>(static_cast<float>(_windowSize.x), bottom));
            float yy = (static_cast<float>(y) + 0.5f)/static_cast<float>(_maxLinesPerField);
            float left = _lefts[y]/static_cast<float>(_maxSamplesPerLine);
            float right = left + _widths[y]/static_cast<float>(_maxSamplesPerLine);
            _dataGeometry.setVertexUV(y*4,     Vector2<float>(left, yy));
            _dataGeometry.setVertexUV(y*4 + 1, Vector2<float>(right, yy));
            _dataGeometry.setVertexUV(y*4 + 2, Vector2<float>(left, yy));
            _dataGeometry.setVertexUV(y*4 + 3, Vector2<float>(right, yy));
        }
        _dataGeometry.unlock();

        IF_ERROR_THROW_DX(_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE));
        IF_ERROR_THROW_DX(_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO));
        IF_ERROR_THROW_DX(_device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD));

        _dataTexture.unlock();
        _dataGeometry.setStreamSource();
        _dataTexture.setTexture();
        IF_ERROR_THROW_DX(_device->DrawPrimitive(
            D3DPT_TRIANGLESTRIP,  // PrimitiveType
            0,                    // StartVertex
            4*lines - 2));        // PrimitiveCount
        _dataTexture.lock();
        _dataData = _dataTexture.data();
        _dataPitch = _dataTexture.pitch();

        _shadowMask.draw();
        _scanLines.draw(_verticalSyncPhase);

        ++_frames;
        if (_frames == 60) {
            int time = GetTickCount();
            int delay = time - _lastTime;
            _lastTime = time;
            printf("%.2lf\n", /*60000.0/(double)delay); */ (double)delay/60.0);
            _frames = 0;
        }

        _line = 0;
        _foundVerticalSync = false;
        _crtLoad = _baseLoad;
        _verticalSync = 0;
    }

    void setDevice(IDirect3DDevice9* device)
    {
        _device = device;
        _dataTexture.create(device, Vector(_maxSamplesPerLine, _maxLinesPerField));
        _dataTexture.lock();
        _dataData = _dataTexture.data();
        _dataPitch = _dataTexture.pitch();
        _dataGeometry.create(_device, _maxLinesPerField*4);
        _shadowMask.create(_device);
        _scanLines.create(_device, _lineTop, _linesVisible);
    }

    void paint()
    {
        do {
            consume(_n);
        } while (_line != 0 || _foundVerticalSync);
    }

    void draw() { }

    void resize(Vector windowSize)
    {
        _windowSize = windowSize;
        _shadowMask.resize(windowSize);
        _scanLines.resize(windowSize);
    }

    void doneResize() { }

private:
    IDirect3DDevice9* _device;
    Direct3DTexture _dataTexture;
    Direct3DVertices _dataGeometry;
    Byte* _dataData;
    int _dataPitch;
    int _scanLinesTextureHeight;

    int _minSamplesPerLine;
    int _maxSamplesPerLine;
    int _minLinesPerField;
    int _maxLinesPerField;

    Vector _windowSize;

    float _brightness;
    float _contrast;
    float _saturation;
    float _tint;
    float _horizontalSize;
    float _horizontalPosition;
    float _verticalSize;
    float _verticalPosition;
    int _verticalHold;
    int _horizontalHold;
    float _bloomFactor;
    float _scanLineHeight;

    float _linesVisible;
    float _lineTop;
    int _driftLines;
    int _driftSamples;
    float _active;
    float _preActive;
    int _colorBurstStart;

    float _baseLoad;
    float _crtLoad;

    float _colorBurstPhase[4];
    float _lockedColorBurstPhase[4];

    int _lastTime;
    int _frames;

    int _phase;

    int _line;
    bool _foundVerticalSync;
    int _verticalSync;
    float _verticalSyncPhase;

    std::vector<float> _lefts;    // First sample on each line
    std::vector<float> _widths;   // How many samples visible on each line

    std::vector<int> _delay;

    std::vector<int> _gamma;
    int* _gamma0;

    int _topLine;
    int _bottomLine;

    int _linePeriod;

    int _hysteresisCount;
    bool _colorMode;

    ShadowMask _shadowMask;
    ScanLines _scanLines;
};


class RGBMonitor : public Sink<DWord>
{
public:
    RGBMonitor()
      : Sink(1),
        _frames(0),
        _foundVerticalSync(false),
        _line(0),
        _baseLoad(0.5),
        _verticalSync(0),

        // TODO: make these user-settable
        _brightness(0.06f),
        _contrast(3.0f),
        _horizontalSize(0.95f),
        _horizontalPosition(0),
        _verticalSize(0.93f),
        _verticalPosition(-0.01f),
        _verticalHold(280),
        _horizontalHold(25),
        _bloomFactor(10.0f)
    {
        float samplesPerSecond = 157500000.0f/11.0f;
        float us = samplesPerSecond/1000000.0f;  // samples per microsecond
        // Horizontal times in samples.
        float sync = 4.7f*us;
        float backPorch = 4.7f*us;
        float frontPorch = 1.5f*us;
        float blanking = sync + backPorch + frontPorch;
        float line = 910.0f;
        _active = line - blanking;
        _preActive = blanking - frontPorch;
        // The following parameter specifies how many samples early or late the
        // horizontal sync pulse can be and still be recognized (assuming good
        // signal fidelity). This sets the angle of the diagonal lines that
        // vertical lines become when horizontal sync is lost.
        _driftSamples = 8;
        _minSamplesPerLine = static_cast<int>(line - _driftSamples);
        _maxSamplesPerLine = static_cast<int>(line + _driftSamples);
        // We always consume a scanline at a time, and we won't be called to
        // process until we're sure we have a scanline.
        _n = _maxSamplesPerLine;
        _linePeriod = static_cast<int>(line);

        // Vertical times in lines.
        float preSyncLines = 3.0f;
        float syncLines = 3.0f;
        float postSyncLines = 14.0f;
        float lines = 262.5f;
        float blankingLines = preSyncLines + syncLines + postSyncLines;
        float activeLines = lines - blankingLines;
        _linesVisible = activeLines*_verticalSize;
        _lineTop = postSyncLines + activeLines*(0.5f + _verticalPosition - _verticalSize/2.0f);
        // The following parameter specifies how many lines early or late the
        // vertical sync pulse can be and still be recognized (assuming good
        // signal fidelity). This sets the "roll speed" of the picture when
        // vertical sync is lost. Empirically determined from video of an IBM
        // 5153 monitor.
        _driftLines = 14;
        _minLinesPerField = static_cast<int>(lines - _driftLines);
        _maxLinesPerField = static_cast<int>(lines + _driftLines);

        _lefts.resize(_maxLinesPerField);
        _widths.resize(_maxLinesPerField);

        _crtLoad = _baseLoad;
    }

    // Returns the top row for a scanline
    int topRow(int line)
    {
        return static_cast<int>(
            (static_cast<float>(line) - _verticalSyncPhase - _lineTop - 0.5f)*
             static_cast<float>(_windowSize.y)/_linesVisible + 1.0f);
    }

    // We always process a single scanline here.
    void consume(int n)
    {
        Accessor<DWord> reader = Sink::reader(n);

        // Find the horizontal sync position.
        int offset = 0;
        for (int i = 0; i < _driftSamples*2; ++i, ++offset)
            if ((reader.item(offset)&0x1000000) != 0)
                break;
        // We use a phase-locked loop like real hardware does, in order to
        // avoid losing horizontal sync if the pulse is missing for a line or
        // two, and so that we get the correct "wobble" behavior.
        int linePeriod = _maxSamplesPerLine - offset;
        _linePeriod = (2*_linePeriod + linePeriod)/3;
        _linePeriod = clamp(_minSamplesPerLine, _linePeriod, _maxSamplesPerLine);
        offset = _maxSamplesPerLine - _linePeriod;

        // Find the vertical sync position.
        if (!_foundVerticalSync)
            for (int j = 0; j < _maxSamplesPerLine; j += 57) {
                _verticalSync = ((_verticalSync*232)>>8) + ((reader.item(j)&0x2000000) != 0 ? 4 : 0x3c);
                if (_verticalSync < -_verticalHold || _line == 2*_driftLines) {
                    // To render interlaced signals correctly, we need to
                    // figure out where the vertical sync pulse happens
                    // relative to the horizontal pulse. This determines the
                    // vertical position of the raster relative to the screen.
                    _verticalSyncPhase = static_cast<float>(j)/static_cast<float>(_maxSamplesPerLine);
                    // Now we can find out which scanlines are at the top and
                    // bottom of the screen.
                    _topLine = static_cast<int>(0.5f + _lineTop + _verticalSyncPhase);
                    _bottomLine = static_cast<int>(1.5f + _linesVisible + _lineTop + _verticalSyncPhase);
                    _line = 0;
                    _foundVerticalSync = true;
                    break;
                }
            }

        if (_foundVerticalSync && _line >= _topLine && _line < _bottomLine) {
            int y = _line - _topLine;

            // Lines with high amounts of brightness cause more load on the
            // horizontal oscillator which decreases horizontal deflection,
            // causing "blooming" (increase in width).
            int totalSignal = 0;
            for (int i = 0; i < _active; ++i) {
                DWord s = reader.item(offset + i);
                totalSignal += ((s>>16)&0xff) + ((s>>8)&0xff) + (s&0xff);
            }
            totalSignal /= 6;
            _crtLoad = 0.4f*_crtLoad + 0.6f*(_baseLoad + (totalSignal - 42000.0f)/140000.0f);
            float bloom = clamp(-2.0f, _bloomFactor*_crtLoad, 10.0f);
            float horizontalSize = (1.0f - 6.3f*bloom/_active)*_horizontalSize;
            float samplesVisible = _active*horizontalSize;
            float sampleLeft = _preActive + _active*(0.5f + _horizontalPosition - horizontalSize/2.0f);
            _lefts[y] = sampleLeft;
            _widths[y] = samplesVisible;

            int start = max(static_cast<int>(sampleLeft) - 10, 0);
            int end = min(static_cast<int>(sampleLeft + samplesVisible) + 10, _maxSamplesPerLine - offset);
            int brightness = static_cast<int>(_brightness*100.0 - 7.5f*256.0f*_contrast)<<8;
            DWord* destination = reinterpret_cast<DWord*>(_dataData + y*_dataPitch) + start;

            int yContrast = static_cast<int>(_contrast*1463.0f);
            int sp = offset + start;
            for (int x = start; x < end; ++x) {
                DWord s = static_cast<int>(reader.item(sp++));
                int r = ((s >> 16)&0xff)*yContrast + brightness;
                int g = ((s >> 8)&0xff)*yContrast + brightness;
                int b = (s&0xff)*yContrast + brightness;
                r = clamp(0, r>>16, 255);
                g = clamp(0, g>>16, 255);
                b = clamp(0, b>>16, 255);
                *(destination++) = 0xff000000 | (r<<16) | (g<<8) | b;
            }
        }
        offset += _minSamplesPerLine;
        read(offset);

        ++_line;
        if (_foundVerticalSync && _line == _minLinesPerField)
            postField();
    }

    void postField()
    {
        int lines = _bottomLine - _topLine;
        _dataGeometry.lock();
        for (int y = 0; y < lines; ++y) {
            int line = y + _topLine;
            float top = static_cast<float>(topRow(line));
            float bottom = static_cast<float>(topRow(line + 1));
            _dataGeometry.setVertex(y*4,     Vector2<float>(0.0f, top));
            _dataGeometry.setVertex(y*4 + 1, Vector2<float>(static_cast<float>(_windowSize.x), top));
            _dataGeometry.setVertex(y*4 + 2, Vector2<float>(0.0f, bottom));
            _dataGeometry.setVertex(y*4 + 3, Vector2<float>(static_cast<float>(_windowSize.x), bottom));
            float yy = (static_cast<float>(y) + 0.5f)/static_cast<float>(_maxLinesPerField);
            float left = _lefts[y]/static_cast<float>(_maxSamplesPerLine);
            float right = left + _widths[y]/static_cast<float>(_maxSamplesPerLine);
            _dataGeometry.setVertexUV(y*4,     Vector2<float>(left, yy));
            _dataGeometry.setVertexUV(y*4 + 1, Vector2<float>(right, yy));
            _dataGeometry.setVertexUV(y*4 + 2, Vector2<float>(left, yy));
            _dataGeometry.setVertexUV(y*4 + 3, Vector2<float>(right, yy));
        }
        _dataGeometry.unlock();

        IF_ERROR_THROW_DX(_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE));
        IF_ERROR_THROW_DX(_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO));
        IF_ERROR_THROW_DX(_device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD));

        _dataTexture.unlock();
        _dataGeometry.setStreamSource();
        _dataTexture.setTexture();
        IF_ERROR_THROW_DX(_device->DrawPrimitive(
            D3DPT_TRIANGLESTRIP,  // PrimitiveType
            0,                    // StartVertex
            4*lines - 2));        // PrimitiveCount
        _dataTexture.lock();
        _dataData = _dataTexture.data();
        _dataPitch = _dataTexture.pitch();

        _shadowMask.draw();
        _scanLines.draw(_verticalSyncPhase);

        ++_frames;
        if (_frames == 60) {
            int time = GetTickCount();
            int delay = time - _lastTime;
            _lastTime = time;
            printf("%.2lf\n", /*60000.0/(double)delay); */ (double)delay/60.0);
            _frames = 0;
        }

        _line = 0;
        _foundVerticalSync = false;
        _crtLoad = _baseLoad;
        _verticalSync = 0;
    }

    void setDevice(IDirect3DDevice9* device)
    {
        _device = device;
        _dataTexture.create(device, Vector(_maxSamplesPerLine, _maxLinesPerField));
        _dataTexture.lock();
        _dataData = _dataTexture.data();
        _dataPitch = _dataTexture.pitch();
        _dataGeometry.create(_device, _maxLinesPerField*4);
        _shadowMask.create(_device);
        _scanLines.create(_device, _lineTop, _linesVisible);
    }

    void paint()
    {
        do {
            consume(_n);
        } while (_line != 0 || _foundVerticalSync);
    }

    void draw() { }

    void resize(Vector windowSize)
    {
        _windowSize = windowSize;
        _shadowMask.resize(windowSize);
        _scanLines.resize(windowSize);
    }

    void doneResize() { }

private:
    IDirect3DDevice9* _device;
    Direct3DTexture _dataTexture;
    Direct3DVertices _dataGeometry;
    Byte* _dataData;
    int _dataPitch;
    int _scanLinesTextureHeight;

    int _minSamplesPerLine;
    int _maxSamplesPerLine;
    int _minLinesPerField;
    int _maxLinesPerField;

    Vector _windowSize;

    float _brightness;
    float _contrast;
    float _horizontalSize;
    float _horizontalPosition;
    float _verticalSize;
    float _verticalPosition;
    int _verticalHold;
    int _horizontalHold;
    float _bloomFactor;
    float _scanLineHeight;

    float _linesVisible;
    float _lineTop;
    int _driftLines;
    int _driftSamples;
    float _active;
    float _preActive;

    float _baseLoad;
    float _crtLoad;

    int _lastTime;
    int _frames;

    int _line;
    bool _foundVerticalSync;
    int _verticalSync;
    float _verticalSyncPhase;

    std::vector<float> _lefts;    // First sample on each line
    std::vector<float> _widths;   // How many samples visible on each line

    int _topLine;
    int _bottomLine;

    int _linePeriod;

    ShadowMask _shadowMask;
    ScanLines _scanLines;
};


char* filename;
int filesize;

class FileImage: public PeriodicSourceData<Sample>
{
public:
    FileImage()
      : PeriodicSourceData(filesize)
    {
        FILE* in = fopen(filename, "rb");
        fread(buffer(), filesize, 1, in);
        fclose(in);
    }
};

class Program : public ProgramBase
{
public:
    void run()
    {
        fastRandomInit();
        COMInitializer ci;
        Direct3D direct3D;

        typedef FileImage Data;
        Data data;
        NTSCSource<Data> source(&data);
        NoisePipe noise(10000);
        GhostingPipe ghost;
        DropOutPipe<Sample> dropOut(60, 6000, 2000000);
        CompositeMonitor monitor;

        source.connect(noise.sink());
        noise.source()->connect(ghost.sink());
        ghost.source()->connect(dropOut.sink());
        dropOut.source()->connect(&monitor);

        Window::Params wp(&_windows, L"CRT Simulator");
        typedef RootWindow<Window> RootWindow;
        RootWindow::Params rwp(wp);
        typedef Direct3DWindow<RootWindow, CompositeMonitor> ImageWindow;
        ImageWindow::Params iwp(rwp, &monitor, direct3D, D3DPRESENT_INTERVAL_ONE, false);
        typedef AnimatedWindow<ImageWindow> AnimatedWindow;
        AnimatedWindow::Params awp(iwp, 60);
        typedef FixedAspectRatioWindow<AnimatedWindow> FixedAspectRatioWindow;
        FixedAspectRatioWindow::Params fwp(awp, Vector(4, 3));
        FixedAspectRatioWindow window(fwp);

        window.show(_nCmdShow);
        window.setWidth(window.getSize().x);
        pumpMessages();
    }
}


int __cdecl main(int argc, char* argv[])
{
    filename = argv[1];
    FILE* in = 0;
    if (argc >= 2 && filename != 0)
        in = fopen(filename, "rb");
    if (in == 0) {
        printf("Syntax: crtsim <name of .ntsc file>\n");
        exit(0);
    }
    fseek(in, 0, SEEK_END);
    filesize = ftell(in);
    fclose(in);
    return WinMain(GetModuleHandle(NULL), NULL, "", SW_SHOWNORMAL);
}
