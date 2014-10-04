#include "alfe/main.h"                             
#include "alfe/bitmap_png.h"
#include "alfe/complex.h"
#include "alfe/space.h"
#include "alfe/set.h"

class CGASimulator
{
public:
    void initChroma()
    {
        static Byte chromaData[256] = {
             65, 11, 62,  6, 121, 87, 63,  6,  60,  9,120, 65,  61, 59,129,  5,
            121,  6, 58, 58, 134, 65, 62,  6,  57,  9,108, 72, 126, 72,125, 77,
             60, 98,160,  6, 113,195,194,  8,  53, 94,218, 64,  56,152,225,  5,
            118, 90,147, 56, 115,154,156,  0,  52, 92,197, 73, 107,156,213, 62,
            119, 10, 97,122, 178, 77, 60, 87, 119, 12,174,205, 119, 58,135, 88,
            185,  6, 54,158, 194, 67, 57, 87, 114, 10,101,168, 181, 67,114,160,
             64,  8,156,109, 121, 73,177,122,  58,  8,244,207,  65, 58,251,137,
            127,  5,141,156, 126, 58,144, 97,  57,  7,189,168, 106, 55,201,162,
            163,124, 62, 10, 185,159, 59,  8, 135,104,128, 80, 119,142,140,  5,
            241,141, 59, 57, 210,160, 61,  5, 137,108,103, 61, 177,140,110, 65,
             59,107,124,  4, 180,201,122,  6,  52,104,194, 77,  55,159,197,  3,
            130,128,121, 51, 174,197,123,  3,  52,100,162, 62, 101,156,171, 51,
            173, 11, 60,113, 199, 93, 58, 77, 167, 11,118,196, 132, 63,129, 74,
            256,  9, 54,195, 192, 55, 59, 74, 183, 14,103,199, 206, 74,118,154,
            153,108,156,105, 255,202,188,123, 143,107,246,203, 164,208,250,129,
            209,103,148,157, 253,195,171,120, 163,106,196,207, 245,202,249,208
        };
        for (int i = 0; i < 256; ++i)
            _chroma[i] = chromaData[i]*(0.727546-0.070565)/256.0+0.070565;

        calculateBurst();
    }
    void calculateBurst()
    {
        double burst[4];
        for (int i = 0; i < 4; ++i)
            burst[i] = simulateCGA(6, 6, i);
        Complex<double> iq;
        iq.x = burst[0] - burst[2];
        iq.y = burst[1] - burst[3];
        _iqAdjust =
            -iq.conjugate()*unit((33 + 90 + _hue)/360.0)*_saturation*_contrast/
            iq.modulus();
    }
    double simulateCGA(int left, int right, int phase)
    {
        static double intensity[4] = {
            0, 0.047932237386703491, 0.15110087022185326, 0.18384206667542458};

        double c = _chroma[((left & 7) << 5) | ((right & 7) << 2) | phase];
        double i = intensity[(left >> 3) | ((right >> 2) & 2)];
        if (!_newCGA)
            return c+i;
        double r = intensity[((left >> 2) & 1) | ((right >> 1) & 2)];
        double g = intensity[((left >> 1) & 1) | (right & 2)];
        double b = intensity[(left & 1) | ((right << 1) & 1)];
        return (c/0.72)*0.29 + (i/0.28)*0.32 + (r/0.28)*0.1 + (g/0.28)*0.22 +
            (b/0.28)*0.07;
    }
    Colour decode(int pixels)
    {
        int rgbi[4];
        rgbi[0] = pixels & 15;
        rgbi[1] = (pixels >> 4) & 15;
        rgbi[2] = (pixels >> 8) & 15;
        rgbi[3] = (pixels >> 12) & 15;
        double s[4];
        for (int t = 0; t < 4; ++t)
            s[t] = simulateCGA(rgbi[t], rgbi[(t+1)&3], t);
        double dc = (s[0] + s[1] + s[2] + s[3])/4;
        Complex<double> iq;
        iq.x = (s[0] - s[2])/2;
        iq.y = (s[1] - s[3])/2;
        return decode(dc, iq);
    }
    Colour decode(const double* n, int phase)
    {
        // Filter kernel must be divisible by (1,1,1,1) so that all phases
        // contribute equally.
        double y = n[0] +n[1]*4 +n[2]*7 +n[3]*8 +n[4]*7 +n[5]*4 +n[6];
        Complex<double> iq;
        switch (phase) {
            case 0: 
                iq.x =  n[0]   -n[2]*7 +n[4]*7 -n[6];
                iq.y =  n[1]*4 -n[3]*8 +n[5]*4; 
                break;
            case 1:
                iq.x = -n[1]*4 +n[3]*8 -n[5]*4;
                iq.y =  n[0]   -n[2]*7 +n[4]*7 -n[6];
                break;
            case 2:
                iq.x = -n[0]   +n[2]*7 -n[4]*7 +n[6];
                iq.y = -n[1]*4 +n[3]*8 -n[5]*4; 
                break;
            case 3:
                iq.x = +n[1]*4 -n[3]*8 +n[5]*4;
                iq.y = -n[0]   +n[2]*7 -n[4]*7 +n[6];
                break;
        }
        return decode(y, iq);
    }

    bool _newCGA;
    bool _fixPrimaries;
    double _hue;
    double _saturation;
    double _contrast;
    double _brightness;
private:
    Colour decode(double y, Complex<double> iq)
    {
        y = y*_contrast + _brightness;
        iq *= _iqAdjust;
        double r = y + 0.9563*iq.x + 0.6210*iq.y;
        double g = y - 0.2721*iq.x - 0.6474*iq.y;
        double b = y - 1.1069*iq.x + 1.7046*iq.y;
        if (_fixPrimaries)
            return Colour(
                 1.5073*r -0.3725*g -0.0832*b,
                -0.0275*r +0.9350*g +0.0670*b,
                -0.0272*r -0.0401*g +1.1677*b);
        return Colour(r, g, b)*256;
    }

    double _chroma[256];
    Complex<double> _iqAdjust;
};

class Particle
{
public:
    void plot(Bitmap<DWORD> bitmap, Vector rPosition)
    {
        Vector size = bitmap.size();
        Byte* buffer = bitmap.data();
        int byteWidth = bitmap.stride();
        double zOffset = rPosition.x*0.01;
        double scale = rPosition.y*0.01;
        double x = _position.x/(_position.z + zOffset)*scale;
        double y = _position.y/(_position.z + zOffset)*scale;
        int x0 = static_cast<int>(size.x*x/5.0 + size.x/2);
        int y0 = static_cast<int>(size.x*y/5.0 + size.y/2);
        int r = byteClamp(_colour.x);
        int g = byteClamp(_colour.y);
        int b = byteClamp(_colour.z);
        DWord c = (r << 16) | (g << 8) | b;
        plot(bitmap, Vector(x0,     y0    ), c);
        if (!_big) {
            if (r < 16 && g < 16 && b < 16) {
                c = 0xffffff;
                plot(bitmap, Vector(x0 - 1, y0 - 1), c);
                plot(bitmap, Vector(x0 + 1, y0 - 1), c);
                plot(bitmap, Vector(x0 - 1, y0 + 1), c);
                plot(bitmap, Vector(x0 + 1, y0 + 1), c);
            }
            return;
        }
        plot(bitmap, Vector(x0 - 1, y0 - 2), c);
        plot(bitmap, Vector(x0,     y0 - 2), c);
        plot(bitmap, Vector(x0 + 1, y0 - 2), c);
        plot(bitmap, Vector(x0 - 2, y0 - 1), c);
        plot(bitmap, Vector(x0 - 1, y0 - 1), c);
        plot(bitmap, Vector(x0,     y0 - 1), c);
        plot(bitmap, Vector(x0 + 1, y0 - 1), c);
        plot(bitmap, Vector(x0 + 2, y0 - 1), c);
        plot(bitmap, Vector(x0 - 2, y0    ), c);
        plot(bitmap, Vector(x0 - 1, y0    ), c);
        plot(bitmap, Vector(x0 + 1, y0    ), c);
        plot(bitmap, Vector(x0 + 2, y0    ), c);
        plot(bitmap, Vector(x0 - 2, y0 + 1), c);
        plot(bitmap, Vector(x0 - 1, y0 + 1), c);
        plot(bitmap, Vector(x0,     y0 + 1), c);
        plot(bitmap, Vector(x0 + 1, y0 + 1), c);
        plot(bitmap, Vector(x0 + 2, y0 + 1), c);
        plot(bitmap, Vector(x0 - 1, y0 + 2), c);
        plot(bitmap, Vector(x0,     y0 + 2), c);
        plot(bitmap, Vector(x0 + 1, y0 + 2), c);
        if (r < 16 && g < 16 && b < 16) {
            c = 0xffffff;
            plot(bitmap, Vector(x0 - 1, y0 - 3), c);
            plot(bitmap, Vector(x0,     y0 - 3), c);
            plot(bitmap, Vector(x0 + 1, y0 - 3), c);
            plot(bitmap, Vector(x0 - 1, y0 + 3), c);
            plot(bitmap, Vector(x0,     y0 + 3), c);
            plot(bitmap, Vector(x0 + 1, y0 + 3), c);
            plot(bitmap, Vector(x0 - 2, y0 - 2), c);
            plot(bitmap, Vector(x0 + 2, y0 - 2), c);
            plot(bitmap, Vector(x0 - 3, y0 - 1), c);
            plot(bitmap, Vector(x0 + 3, y0 - 1), c);
            plot(bitmap, Vector(x0 - 3, y0    ), c);
            plot(bitmap, Vector(x0 + 3, y0    ), c);
            plot(bitmap, Vector(x0 - 3, y0 + 1), c);
            plot(bitmap, Vector(x0 + 3, y0 + 1), c);
            plot(bitmap, Vector(x0 - 2, y0 + 2), c);
            plot(bitmap, Vector(x0 + 2, y0 + 2), c);
        }
    }
    void plot(Bitmap<DWORD> bitmap, Vector p, DWord c)
    {
        if (p.inside(bitmap.size()))
            bitmap[p] = c;
    }
    bool operator<(const Particle& other) const { return _position.z > other._position.z; }
    void transform(double* matrix)
    {
        Vector3<double> c = (_colour - Vector3<double>(128.0, 128.0, 128.0))/128.0;
        _position.x = c.x*matrix[0] + c.y*matrix[1] + c.z*matrix[2];
        _position.y = c.x*matrix[3] + c.y*matrix[4] + c.z*matrix[5];
        _position.z = c.x*matrix[6] + c.y*matrix[7] + c.z*matrix[8];
    }

    Colour _colour;
    Vector3<double> _position;
    bool _big;
};

class GamutWindow : public BitmapWindow
{
public:
    GamutWindow()
      : _lButton(false), _rButton(false), _rPosition(1000, 1000), _particle(0)
    {
        _matrix[0] = 1; _matrix[1] = 0; _matrix[2] = 0;
        _matrix[3] = 0; _matrix[4] = 1; _matrix[5] = 0;
        _matrix[6] = 0; _matrix[7] = 0; _matrix[8] = 1;
    }
    void create()
    {
        setSize(Vector(640, 480));

        BitmapWindow::create();
        _animation.setWindow(this);
        _animation.start();

        reset();
    }
    void setSimulator(CGASimulator* simulator) { _simulator = simulator; }
    void paint(PaintHandle* paint)
    {
        if (_delta.modulus2() >= 0.000001)
            _animation.onPaint();
        draw();
        BitmapWindow::paint(paint);
    }
    void draw()
    {
        _bitmap.fill(0);
        for (int i = 0; i < _particle; ++i)
            _particles[i].transform(_matrix);
        std::sort(&_particles[0], &_particles[_particle]);
        for (int i = 0; i < _particle; ++i)
            _particles[i].plot(_bitmap, _rPosition);
    }
    void line(Colour c1, Colour c2)
    {
        for (int i = 0; i < 101; ++i)
            add(c1*(i*0.01) + c2*((100-i)*0.01), false);
    }
    void reset()
    {
        _particle = 0;
        line(Colour(0, 0, 0), Colour(255, 0, 0));
        line(Colour(0, 0, 0), Colour(0, 255, 0));
        line(Colour(0, 0, 0), Colour(0, 0, 255));
        line(Colour(255, 0, 0), Colour(255, 255, 0));
        line(Colour(255, 0, 0), Colour(255, 0, 255));
        line(Colour(0, 255, 0), Colour(255, 255, 0));
        line(Colour(0, 255, 0), Colour(0, 255, 255));
        line(Colour(0, 0, 255), Colour(0, 255, 255));
        line(Colour(0, 0, 255), Colour(255, 0, 255));
        line(Colour(255, 255, 0), Colour(255, 255, 255));
        line(Colour(255, 0, 255), Colour(255, 255, 255));
        line(Colour(0, 255, 255), Colour(255, 255, 255));
    }
    void add(Colour c, bool big = true)
    {
        Particle p;
        p._colour = c;
        p._big = big;
        if (_particle >= _particles.count())
            _particles.append(p);
        else
            _particles[_particle] = p;
        ++_particle;
    }
    bool mouseInput(Vector position, int buttons)
    {
        bool mouseDown = false;
        if ((buttons & MK_LBUTTON) != 0 && !_lButton) {
            _lButton = true;
            mouseDown = true;
            _lastPosition = position;
        }
        if ((buttons & MK_LBUTTON) == 0)
            _lButton = false;
        if ((buttons & MK_RBUTTON) != 0 && !_rButton) {
            _rButton = true;
            mouseDown = true;
            _lastPosition = position;
        }
        if ((buttons & MK_RBUTTON) == 0)
            _rButton = false;
        if (_lButton && position != _lastPosition) {
            _delta = Vector2Cast<double>(position - _lastPosition)*0.01;
            _lastPosition = position;
            animate();
        }
        if (_rButton && position != _lastPosition) {
            _rPosition += (position - _lastPosition);
            _lastPosition = position;
        }

        return mouseDown;
    }
    void animate()
    {
        _rotor = Rotor3<double>::yz(-_delta.y)*Rotor3<double>::zx(_delta.x)*_rotor;
        _rotor.toMatrix(_matrix);
        _delta *= 0.99;
    }

    AnimationThread _animation;
    Rotor3<double> _rotor;
    double _matrix[9];
    AppendableArray<Particle> _particles;
    Vector _lastPosition;
    Vector _rPosition;
    bool _lButton;
    bool _rButton;          
    CGASimulator* _simulator;
    Vector2<double> _delta;

    int _particle;
};

class CGA2NTSCWindow;

template<class T> class BrightnessSliderWindowTemplate : public SliderWindow
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void valueSet(double value) { _host->setBrightness(value); }
    void create()
    {
        setValue(0);
        setRange(-1, 1);
        SliderWindow::create();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef BrightnessSliderWindowTemplate<void> BrightnessSliderWindow;

template<class T> class SaturationSliderWindowTemplate : public SliderWindow
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void valueSet(double value) { _host->setSaturation(value); }
    void create()
    {
        setValue(1);
        setRange(0, 2);
        SliderWindow::create();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef SaturationSliderWindowTemplate<void> SaturationSliderWindow;

template<class T> class ContrastSliderWindowTemplate : public SliderWindow
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void valueSet(double value) { _host->setContrast(value); }
    void create()
    {
        setValue(1);
        setRange(0, 2);
        SliderWindow::create();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef ContrastSliderWindowTemplate<void> ContrastSliderWindow;

template<class T> class HueSliderWindowTemplate : public SliderWindow
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void valueSet(double value) { _host->setHue(value); }
    void create()
    {
        setValue(0);
        setRange(-180, 180);
        SliderWindow::create();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef HueSliderWindowTemplate<void> HueSliderWindow;

template<class T> class AutoBrightnessButtonWindowTemplate : public Button
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void clicked() { _host->autoBrightness(); }
    void create()
    {
        setText("Auto");
        Button::create();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef AutoBrightnessButtonWindowTemplate<void> AutoBrightnessButtonWindow;

template<class T> class AutoContrastClipButtonWindowTemplate : public Button
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void clicked() { _host->autoContrastClip(); }
    void create()
    {
        setText("No clipping");
        Button::create();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef AutoContrastClipButtonWindowTemplate<void>
    AutoContrastClipButtonWindow;

template<class T> class AutoSaturationButtonWindowTemplate : public Button
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void clicked() { _host->autoSaturation(); }
    void create()
    {
        setText("Auto");
        Button::create();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef AutoSaturationButtonWindowTemplate<void> AutoSaturationButtonWindow;

template<class T> class AutoContrastMonoButtonWindowTemplate : public Button
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void clicked() { _host->autoContrastMono(); }
    void create()
    {
        setText("Fix black and white");
        Button::create();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef AutoContrastMonoButtonWindowTemplate<void>
    AutoContrastMonoButtonWindow;

class OutputWindow : public BitmapWindow
{
public:
    void setSimulator(CGASimulator* simulator) { _simulator = simulator; }
    void setNTSC(Bitmap<double> ntsc)
    {
        _ntsc = ntsc;
        setSize(ntsc.size() - Vector(6, 0));
    }
    void draw()
    {
        const Byte* ntscRow = _ntsc.data();
        Byte* outputRow = _bitmap.data();
        for (int yy = 0; yy < _ntsc.size().y; ++yy) {
            const double* n = reinterpret_cast<const double*>(ntscRow);
            DWORD* outputPixel = reinterpret_cast<DWORD*>(outputRow);
            for (int x = 0; x < _ntsc.size().x - 6; ++x) {
                if (x == 24 && yy == 24)
                    x = 24;
                Colour s = _simulator->decode(n, (x + 1) & 3);
                ++n;

                *outputPixel = (byteClamp(s.x) << 16) | 
                    (byteClamp(s.y) << 8) | byteClamp(s.z);
                ++outputPixel;
            }
            ntscRow += _ntsc.stride();
            outputRow += _bitmap.stride();
        }
    }
    void save(String outputFileName)
    {
        _bitmap.save(PNGFileFormat<DWORD>(), File(outputFileName, true));
    }

private:
    Bitmap<double> _ntsc;
    CGASimulator* _simulator;
};

class CGA2NTSCWindow : public RootWindow
{
public:
    void setWindows(Windows* windows)
    {
        add(&_output);
        add(&_brightnessCaption);
        add(&_brightness);
        add(&_autoBrightness);
        add(&_brightnessText);
        add(&_saturationCaption);
        add(&_saturation);
        add(&_autoSaturation);
        add(&_saturationText);
        add(&_contrastCaption);
        add(&_contrast);
        add(&_contrastText);
        add(&_autoContrastClip);
        add(&_autoContrastMono);
        add(&_hueCaption);
        add(&_hue);
        add(&_hueText);
        add(&_blackText);
        add(&_whiteText);
        add(&_mostSaturatedText);
        add(&_clippedColoursText);
        add(&_gamut);
        RootWindow::setWindows(windows);
    }
    void create()
    {
        _brightnessCaption.left = _output.right;
        _brightnessCaption.top = top;
        _brightness.left = _output.right;
        _brightness.top = _brightnessCaption.bottom;
        _autoBrightness.left = _output.right;
        _autoBrightness.top = _brightness.bottom;
        _brightnessText.left = _autoBrightness.right;
        _brightnessText.bottom = _brightness.bottom;


        _brightnessCaption.setText("Brightness:");
        _saturationCaption.setText("Saturation:");
        _contrastCaption.setText("Contrast:");
        _hueCaption.setText("Hue:");

        _brightness.setHost(this);
        _saturation.setHost(this);
        _contrast.setHost(this);
        _hue.setHost(this);
        _autoBrightness.setHost(this);
        _autoSaturation.setHost(this);
        _autoContrastClip.setHost(this);
        _autoContrastMono.setHost(this);

        setText("CGA to NTSC");
        setSize(Vector(640, 480));

        RootWindow::create();

        update();
    }
    void keyboardCharacter(int character)
    {
        if (character == VK_ESCAPE)
            remove();
    }
    void setNTSC(Bitmap<double> ntsc) { _output.setNTSC(ntsc); }
    void setSimulator(CGASimulator* simulator)
    { 
        _simulator = simulator;
        _output.setSimulator(simulator);
        _gamut.setSimulator(simulator);
    }
    void update()
    {
        Colour black = _simulator->decode(0);
        _black = 0.299*black.x + 0.587*black.y + 0.114*black.z;
        _blackText.setText(format("Black level: %f", _black));
        Colour white = _simulator->decode(0xffff);
        _white = 0.299*white.x + 0.587*white.y + 0.114*white.z;
        _whiteText.setText(format("White level: %f", _white));
        _clips = 0;
        _maxSaturation = 0;
        _gamut.reset();
        for (Set<UInt64>::Iterator i = _colours.begin(); i != _colours.end();
            ++i) {
            UInt64 seq = *i;
            double ntsc[7];
            int phase = (seq >> 32) & 3;
            for (int x = 0; x < 7; ++x) {
                ntsc[x] = _simulator->simulateCGA(seq & 15, (seq >> 4) & 15,
                    (x + phase) & 3);
                seq >>= 4;
            }
            Colour c = _simulator->decode(ntsc, phase);
            double r = c.x;
            double g = c.y;
            double b = c.z;
            if (r < 0 || r >= 256 || g < 0 || g >= 256 || b < 0 || b >= 256) {
                ++_clips;
                _clipped = seq;
            }
            double y = 0.299*r + 0.587*g + 0.114*b;
            _maxSaturation =
                max(_maxSaturation, (c - Colour(y, y, y)).modulus());
            _gamut.add(c);
        }
        _mostSaturatedText.setText(
            format("Most saturated: %f", _maxSaturation));
        _clippedColoursText.setText(format("%i colours clipped", _clips));
    }

    void setBrightness(double brightness)
    {
        _simulator->_brightness = brightness;
        update();
    }
    void setSaturation(double saturation)
    {
        _simulator->_saturation = saturation;
        update();
    }
    void setContrast(double contrast)
    {
        _simulator->_contrast = contrast;
        update();
    }
    void setHue(double hue)
    {
        _simulator->_hue = hue;
        update();
    }
    void autoBrightness()
    {
        _simulator->_brightness =
            (_black + _white - 256)/512 - _simulator->_brightness;
        _brightness.setValue(_simulator->_brightness);
        update();
    }
    void autoSaturation()
    {
        _simulator->_saturation *= (_white - _black)/(2*_maxSaturation);
        update();
    }
    void autoContrastClip()
    {
        double minContrast = 0;
        double maxContrast = 1;
        do {
            double contrast = (maxContrast + minContrast)/2;
            _simulator->_contrast = contrast;
            update();
            autoBrightness();
            if (_clips == 0)
                minContrast = contrast;
            else
                if (_clips == 1 || minContrast == maxContrast)
                    break;
                else
                    maxContrast = contrast;
        } while (true);
        double midPoint = (_white - _black)/2;
        double fudge = 0.99999;
        while (_clips != 0) {
            UInt64 seq = _clipped;
            double ntsc[7];
            for (int x = 0; x < 7; ++x) {
                ntsc[x] = _simulator->simulateCGA(seq & 15, (seq >> 4) & 15,
                    x & 3);
                seq >>= 4;
            }
            Colour c = _simulator->decode(ntsc, (seq >> 32) & 3);
            double r = c.x;
            double g = c.y;
            double b = c.z;
            bool found = false;
            if (r < 0) {
                _simulator->_contrast *= fudge*midPoint/(midPoint - r);
                found = true;
            }
            if (!found && r >= 256) {
                _simulator->_contrast *= fudge*midPoint/(r - midPoint);
                found = true;
            }
            if (!found && g < 0) {
                _simulator->_contrast *= fudge*midPoint/(midPoint - g);
                found = true;
            }
            if (!found && g >= 256) {
                _simulator->_contrast *= fudge*midPoint/(g - midPoint);
                found = true;
            }
            if (!found && b < 0) {
                _simulator->_contrast *= fudge*midPoint/(midPoint - b);
                found = true;
            }
            if (!found && b >= 256)
                _simulator->_contrast *= fudge*midPoint/(b - midPoint);
            update();
            autoBrightness();
        }
    }
    void autoContrastMono()
    {
        _simulator->_contrast *= 256/(_white - _black);
        _contrast.setValue(_simulator->_contrast);
        update();
    }
    void save(String outputFileName) { _output.save(outputFileName); }
    void addColour(UInt32 seq) { _colours.add(seq); }

private:
    OutputWindow _output;
    TextWindow _brightnessCaption;
    BrightnessSliderWindow _brightness;
    AutoBrightnessButtonWindow _autoBrightness;
    TextWindow _brightnessText;
    TextWindow _saturationCaption;
    SaturationSliderWindow _saturation;
    AutoSaturationButtonWindow _autoSaturation;
    TextWindow _saturationText;
    TextWindow _contrastCaption;
    ContrastSliderWindow _contrast;
    TextWindow _contrastText;
    AutoContrastClipButtonWindow _autoContrastClip;
    AutoContrastMonoButtonWindow _autoContrastMono;
    TextWindow _hueCaption;
    HueSliderWindow _hue;
    TextWindow _hueText;
    TextWindow _blackText;
    TextWindow _whiteText;
    TextWindow _mostSaturatedText;
    TextWindow _clippedColoursText;
    GamutWindow _gamut;
    CGASimulator* _simulator;
    double _black;
    double _white;
    Set<UInt64> _colours;
    int _clips;
    double _maxSaturation;
    UInt64 _clipped;
};

class Program : public WindowProgram<CGA2NTSCWindow>
{
public:
    void run()
    {
        CGASimulator simulator;
        simulator._newCGA = false;
        simulator._fixPrimaries = false;

        if (_arguments.count() < 2) {
            console.write("Syntax: " + _arguments[0] +
                " <input file name.png>\n");
            return;
        }

        String inputFileName = _arguments[1];

        Bitmap<SRGB> input =
            PNGFileFormat<SRGB>().load(File(inputFileName, true));
        Bitmap<SRGB> input2 = input;
        Vector size = input.size();
        if (size.x <= 456) {
            // Image is most likely 2bpp or LRES text mode with 1 pixel per
            // ldot. Rescale it to 1 pixel per hdot.
            input2 = Bitmap<SRGB>(size * 2);
            const Byte* inputRow = input.data();
            Byte* outputRow = input2.data();
            for (int y = 0; y < size.y; ++y) {
                SRGB* outputPixel = reinterpret_cast<SRGB*>(outputRow);
                const SRGB* inputPixel =
                    reinterpret_cast<const SRGB*>(inputRow);
                for (int x = 0; x < size.x; ++x) {
                    SRGB s = *inputPixel;
                    ++inputPixel;
                    *outputPixel = s;
                    ++outputPixel;
                    *outputPixel = s;
                    ++outputPixel;
                }
                outputRow += input2.stride();
                outputPixel = reinterpret_cast<SRGB*>(outputRow);
                inputPixel = reinterpret_cast<const SRGB*>(inputRow);
                for (int x = 0; x < size.x; ++x) {
                    SRGB s = *inputPixel;
                    ++inputPixel;
                    *outputPixel = s;
                    ++outputPixel;
                    *outputPixel = s;
                    ++outputPixel;
                }
                inputRow += input.stride();
                outputRow += input2.stride();
            }
            input = input2;
            size = input.size();
        }

        // Convert to RGBI indexes and add left and right borders.
        static const SRGB inputPalette[16] = {
            SRGB(0x00, 0x00, 0x00), SRGB(0x00, 0x00, 0xaa),
            SRGB(0x00, 0xaa, 0x00), SRGB(0x00, 0xaa, 0xaa),
            SRGB(0xaa, 0x00, 0x00), SRGB(0xaa, 0x00, 0xaa),
            SRGB(0xaa, 0x55, 0x00), SRGB(0xaa, 0xaa, 0xaa),
            SRGB(0x55, 0x55, 0x55), SRGB(0x55, 0x55, 0xff),
            SRGB(0x55, 0xff, 0x55), SRGB(0x55, 0xff, 0xff),
            SRGB(0xff, 0x55, 0x55), SRGB(0xff, 0x55, 0xff),
            SRGB(0xff, 0xff, 0x55), SRGB(0xff, 0xff, 0xff)};

        Bitmap<Byte> rgbi(size + Vector(14, 0));
        {
            rgbi.fill(0);
            const Byte* inputRow = input.data();
            Byte* rgbiRow = rgbi.data();
            for (int y = 0; y < size.y; ++y) {
                const SRGB* inputPixel =
                    reinterpret_cast<const SRGB*>(inputRow);
                Byte* rgbiPixel = rgbiRow + 7;
                for (int x = 0; x < size.x; ++x) {
                    SRGB s = *inputPixel;
                    ++inputPixel;
                    int bestDistance = 0x7fffffff;
                    Byte bestRGBI = 0;
                    for (int i = 0; i < 16; ++i) {
                        int distance =
                            (Vector3Cast<int>(inputPalette[i]) - 
                            Vector3Cast<int>(s)).modulus2();
                        if (distance < bestDistance) {
                            bestDistance = distance;
                            bestRGBI = i;
                            if (distance < 42*42)
                                break;
                        }
                    }
                    *rgbiPixel = bestRGBI;
                    ++rgbiPixel;
                }
                inputRow += input.stride();
                rgbiRow += rgbi.stride();
            }
        }

        // Find all different composite colours (sequences of 8 consecutive
        // RGBI pixels).
        {
            const Byte* inputRow = rgbi.data();
            for (int y = 0; y < size.y; ++y) {
                const Byte* inputPixel = inputRow + 7;
                UInt32 seq = 0;
                for (int x = 0; x < size.x + 7; ++x) {
                    seq = (seq >> 4) | ((*inputPixel) << 28);
                    ++inputPixel;
                    _window.addColour(static_cast<UInt64>(seq) |
                        (static_cast<UInt64>(x & 3) << 32));
                }
                inputRow += rgbi.stride();
            }
        }

        // Convert to raw NTSC
        Bitmap<double> ntsc(size + Vector(13, 0));
        {
            const Byte* rgbiRow = rgbi.data();
            Byte* ntscRow = ntsc.data();
            for (int y = 0; y < size.y; ++y) {
                const Byte* rgbiPixel = rgbiRow;
                double* ntscPixel = reinterpret_cast<double*>(ntscRow);
                for (int x = 0; x < size.x + 13; ++x) {
                    int left = *rgbiPixel;
                    ++rgbiPixel;
                    int right = *rgbiPixel;
                    *ntscPixel = simulator.simulateCGA(left, right, (x+1)&3);
                    ++ntscPixel;
                }
                rgbiRow += rgbi.stride();
                ntscRow += ntsc.stride();
            }
        }
        _window.setNTSC(ntsc);
        _window.setSimulator(&simulator);

        WindowProgram::run();

        String outputFileName;
        int i;
        for (i = inputFileName.length() - 1; i >= 0; --i)
            if (inputFileName[i] == '.')
                break;
        if (i == -1)
            outputFileName = inputFileName + "_out.png";
        else
            outputFileName = inputFileName.subString(0, i) + "_out.png";

        _window.save(outputFileName);
    }
};
