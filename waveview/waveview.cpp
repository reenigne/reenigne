#include "alfe/main.h"

template<class T> class WaveViewThreadT : public ThreadTask
{
public:
    WaveViewThreadT() : _program(0), _zoom(0), _firstSample(0) { }
    void setProgram(Program* program) { _program = program; restart(); }
    void run()
    {
        Vector size = _size;
        int n = size.x*size.y;
        Program* program = _program;
        if (program == 0 || n == 0)
            return;
        UInt32* hits = &_hits[0];

        float offset = _offset;
        float e = (sqrtf(5) - 1)/2;
        int nSamples = _program->nSamples();
        double scale = nSamples*exp(_zoom) / size.x;
        float yScale = size.y / 2.2f;
        while (!cancelling()) {
            offset += e;
            if (offset >= 1.0)
                offset -= 1.0;
            for (int x = 0; x < size.x; ++x) {
                double xx = x;
                xx += offset;
                float y = program->getSampleInterpolated(xx*scale +
                    _firstSample);
                int yy = static_cast<int>((y + 1.1f)*yScale);
                ++hits[yy*size.x + x];
            }
        }
        _offset = offset;
    }
    void setSize(Vector size, UInt32* hits)
    {
        _size = size;
        _hits = hits;
    }
    void changeZoom(int amount, int x)
    {
        float nz = _zoom - amount / 1200.0f;
        _firstSample +=
            x*(exp(_zoom) - exp(nz))*_program->nSamples() / _size.x;
        _zoom = nz;
        int n = _size.x * _size.y;
        UInt32* hits = _hits;
        for (int i = 0; i < n; ++i)
            hits[i] = 0;
        restart();
    }
private:
    UInt32* _hits;
    Vector _size;
    Program* _program;
    float _zoom;
    double _firstSample;

    float _offset;
};

class RenderThread : public ThreadTask
{
public:
    RenderThread(BitmapWindow* window) : _window(window) { }
    void run()
    {
        int nn = 0;
        double exposure = 0;
        UInt32* h = _hits;
        Vector size = _size;
        int n = size.x * size.y;
        for (int i = 0; i < n; ++i) {
            int hits = h[i];
            if (hits > 0) {
                ++nn;
                exposure -= hits;
            }
        }
        float s = -0.01f;
        if (nn != 0)
            s = static_cast<float>(nn/exposure);

        _bitmap.ensure(_size);
        auto buffer = _bitmap.data();
        int stride = _bitmap.stride();
        for (int ys = 0; ys < size.y; ++ys) {
            Byte* p = buffer;
            for (int xs = 0; xs < size.x; ++xs) {
                int c = byteClamp(255 - static_cast<int>(255.0f*exp((*h)*s)));
                p[0] = c;
                p[1] = c;
                p[2] = c;
                p += 4;
                ++h;
            }
            buffer += stride;
        }

        _lastBitmap = _bitmap;
        _bitmap = _window->setNextBitmap(_bitmap);
    }
    void setSize(Vector size, UInt32* hits)
    {
        _size = size;
        _hits = hits;
    }
private:
    BitmapWindow* _window;
    Bitmap<DWORD> _bitmap;
    Bitmap<DWORD> _lastBitmap;
    Vector _size;
    UInt32* _hits;
};

typedef WaveViewThreadT<void> WaveViewThread;

class WaveViewWindow : public RootWindow
{
public:
    WaveViewWindow() : _renderThread(&_bitmap)
    {
        setText("Wave viewer");
        add(&_bitmap);
        add(&_animated);
        _animated.setDrawWindow(this);
        _animated.setRate(60);
    }
    void create()
    {
        RootWindow::create();
        _animated.start();
    }
    virtual void draw()
    {
        _renderThread.restart();
        _animated.restart();
    }
    virtual void innerSizeSet(Vector size)
    {
        join();
        _bitmap.setInnerSize(size);
        int n = size.x*size.y;
        _hits.ensure(n);
        UInt32* hits = &_hits[0];
        for (int i = 0; i < n; ++i)
            hits[i] = 0;
        _waveViewThread.setSize(size, &_hits[0]);
        _renderThread.setSize(size, &_hits[0]);
        _waveViewThread.restart();
        _renderThread.restart();
    }
    void setProgram(Program* program)
    {
        _program = program;
        _waveViewThread.setProgram(program);
    }
    void join()
    {
        _waveViewThread.cancel();
        _renderThread.cancel();
        _waveViewThread.join();
        _renderThread.join();
    }
    bool mouseInput(Vector position, int buttons, int wheel)
    {
        if (wheel != 0)
            _waveViewThread.changeZoom(wheel, position.x);
        return false;
    }
private:
    BitmapWindow _bitmap;
    AnimatedWindow _animated;
    WaveViewThread _waveViewThread;
    RenderThread _renderThread;
    Array<UInt32> _hits;

    Program* _program;
};

class Program : public WindowProgram<WaveViewWindow>
{
public:
    void run()
    {                
        if (_arguments.count() < 2)
            throw Exception("Syntax: waveview <filename>");

        File(_arguments[1], true).readIntoArray(&_data);
        _wav = &_data[0];
        if (!verify()) {
            _nChannels = 1;
            _wBitsPerSample = 8;
            _nSamples = _data.count();
        }
        else {
            _nSamples = dLen() / nBlockAlign();
            _wav += 44;
        }
        _window.setProgram(this);

        WindowProgram::run();

        _window.join();
    }
    float getSample(int sample)
    {
        if (_nChannels == 1) {
            if (_wBitsPerSample == 8)
                return static_cast<float>((_wav[sample] - 127.5)/127.5);
            return static_cast<float>(
                (static_cast<SInt16>(getWord(sample*2)) + 0.5)/32767.5);
        }
        if (_wBitsPerSample == 8) {
            return static_cast<float>(
                (_wav[sample*2] + _wav[sample*2 + 1] - 255)/255.5);
        }
        return static_cast<float>((static_cast<SInt16>(getWord(sample*4))
            + static_cast<SInt16>(getWord(sample*4 + 2)) + 1)/65535.0);
    }
    float getSampleInterpolated(double sample)
    {
        int s = static_cast<int>(sample);
        if (s < 0 || s >= nSamples())
            return 0;
        float s0 = getSample(s);
        float s1 = getSample(s + 1);
        float o = static_cast<float>(sample - s);
        return s0*(1 - o) + s1*o;
    }
    int nSamples() { return _nSamples - 1; }
private:
    UInt16 getWord(int o) { return _wav[o] + (_wav[o + 1] << 8); }
    UInt32 getDWord(int o) { return getWord(o) + (getWord(o + 2) << 16); }
    bool verifyTag(const char* expected, int o)
    {
        for (int i = 0; i < 4; ++i)
            if (expected[i] != _wav[i + o])
                return false;
        return true;
    }
    int nSamplesPerSec() { return getDWord(24); }
    int nBlockAlign() { return getWord(32); }
    int dLen() { return getDWord(40); }
    bool verify()
    {
        if (!verifyTag("RIFF", 0))
            return false;
        if (!verifyTag("WAVE", 8))
            return false;
        if (!verifyTag("fmt ", 12))
            return false;
        if (getDWord(16) != 16)  // Length of "fmt " subchunk
            return false;
        if (getWord(20) != 1)  // wFormatTag
            return false;
        _nChannels = getWord(22);
        if (_nChannels < 1 || _nChannels > 2)
            return false;
        if (getDWord(28) != nSamplesPerSec() * nBlockAlign()) // nAvgBytesPerSec
            return false;
        if (!verifyTag("data", 36))
            return false;
        if (getDWord(4) != dLen() + 36)  // length of "RIFF" chunk
            return false;
        _wBitsPerSample = getWord(34);
        if (nBlockAlign() * 8 != _wBitsPerSample * _nChannels)
            return false;
        if (_wBitsPerSample != 8 && _wBitsPerSample != 16)
            return false;
        if (dLen() % nBlockAlign() != 0)
            return false;
        return true;
    }

    Byte* _wav;
    int _nChannels;
    int _wBitsPerSample;
    int _nSamples;

    Array<Byte> _data;
};