#include "alfe/main.h"
#include "alfe/complex.h"
#include "alfe/terminal6.h"
#include "alfe/bitmap_png.h"
#include "alfe/evaluate.h"
#include "alfe/ntsc_decode.h"

class CaptureWindow;

template<class T> class CaptureBitmapWindowTemplate : public BitmapWindow
{
public:
    void setCaptureWindow(CaptureWindow* captureWindow)
    {
        _captureWindow = captureWindow;
    }
    void create()
    {
        setSize(Vector(960, 720));
        _output = Bitmap<SRGB>(Vector(960, 720));

        _vbiCapPipe = File("\\\\.\\pipe\\vbicap", true).openPipe();
        _vbiCapPipe.write<int>(1);

        _decoded = Bitmap<SRGB>(Vector(960, 240));

        int samples = 450*1024;
        int sampleSpaceBefore = 256;
        int sampleSpaceAfter = 256;
        _buffer.allocate(sampleSpaceBefore + samples + sampleSpaceAfter);
        _b = &_buffer[0] + sampleSpaceBefore;
        for (int i = 0; i < sampleSpaceBefore; ++i)
            _b[i - sampleSpaceBefore] = 0;
        for (int i = 0; i < sampleSpaceAfter; ++i)
            _b[i + samples] = 0;

        _decoder.setBuffers(_b, _decoded);
        _decoder.setOutputPixelsPerLine(1140);

        BitmapWindow::create();
    }

    virtual void draw()
    {
        _vbiCapPipe.read(_b, 1024*450);

        _decoder.decode();

        {
            Byte* row = _output.data();
            const Byte* otherRow = _decoded.data();
            for (int y = 0; y < 240; ++y) {
                for (int yy = 0; yy < 3; ++yy) {
                    const SRGB* op = reinterpret_cast<const SRGB*>(otherRow);
                    SRGB* p = reinterpret_cast<SRGB*>(row);
                    for (int x = 0; x < 960; ++x) {
                        *p = *op;
                        ++p;
                        ++op;
                    }
                    row += _output.stride();
                }
                otherRow += _decoded.stride();
            }
        }

        Vector sz = size();
        if (sz.x > 1536)
            sz.x = 1536;
        if (sz.y > 1024)
            sz.y = 1024;

        Byte* row = data();
        const Byte* otherRow = _output.data();
        for (int y = 0; y < sz.y; ++y) {
            DWORD* p = reinterpret_cast<DWORD*>(row);
            const SRGB* op = reinterpret_cast<const SRGB*>(otherRow);
            for (int x = 0; x < sz.x; ++x) {
                *p = (op->x << 16) | (op->y << 8) | op->z;
                ++p;
                ++op;
            }
            row += stride();
            otherRow += _output.stride();
        }
        invalidate();
    }

    void paint()
    {
        draw();
        _captureWindow->restart();
    }

    void setBrightness(double brightness)
    {
        _decoder.setBrightness(brightness);
    }
    void setSaturation(double saturation)
    {
        _decoder.setSaturation(saturation);
    }
    void setContrast(double contrast) { _decoder.setContrast(contrast); }
    void setHue(double hue) { _decoder.setHue(hue); }

private:
    CaptureWindow* _captureWindow;

    Bitmap<SRGB> _output;
    Bitmap<SRGB> _decoded;

    NTSCCaptureDecoder<SRGB> _decoder;

    Vector _dragStart;
    int _dragStartX;

    AutoHandle _vbiCapPipe;

    Array<Byte> _buffer;
    Byte* _b;
};

typedef CaptureBitmapWindowTemplate<void> CaptureBitmapWindow;

template<class T> class BrightnessSliderWindowTemplate : public Slider
{
public:
    void setHost(CaptureWindow* host) { _host = host; }
    void valueSet(double value) { _host->setBrightness(value); }
    void create()
    {
        setRange(-255, 255);
        Slider::create();
    }
private:
    CaptureWindow* _host;
};
typedef BrightnessSliderWindowTemplate<void> BrightnessSliderWindow;

template<class T> class SaturationSliderWindowTemplate : public Slider
{
public:
    void setHost(CaptureWindow* host) { _host = host; }
    void valueSet(double value) { _host->setSaturation(value); }
    void create()
    {
        setRange(0, 2);
        Slider::create();
    }
private:
    CaptureWindow* _host;
};
typedef SaturationSliderWindowTemplate<void> SaturationSliderWindow;

template<class T> class ContrastSliderWindowTemplate : public Slider
{
public:
    void setHost(CaptureWindow* host) { _host = host; }
    void valueSet(double value) { _host->setContrast(value); }
    void create()
    {
        setRange(0, 4);
        Slider::create();
    }
private:
    CaptureWindow* _host;
};
typedef ContrastSliderWindowTemplate<void> ContrastSliderWindow;

template<class T> class HueSliderWindowTemplate : public Slider
{
public:
    void setHost(CaptureWindow* host) { _host = host; }
    void valueSet(double value) { _host->setHue(value); }
    void create()
    {
        setRange(-180, 180);
        Slider::create();
    }
private:
    CaptureWindow* _host;
};
typedef HueSliderWindowTemplate<void> HueSliderWindow;

class CaptureWindow : public RootWindow
{
public:
    void restart() { _animated.restart(); }
    void setWindows(Windows* windows)
    {
        _output.setCaptureWindow(this);

        add(&_output);
        add(&_animated);

        add(&_brightnessCaption);
        add(&_brightness);
        add(&_brightnessText);
        add(&_saturationCaption);
        add(&_saturation);
        add(&_saturationText);
        add(&_contrastCaption);
        add(&_contrast);
        add(&_contrastText);
        add(&_hueCaption);
        add(&_hue);
        add(&_hueText);

        _animated.setInvalidationWindow(&_output);
        _animated.setRate(60);
        RootWindow::setWindows(windows);
    }
    void create()
    {
        _brightnessCaption.setText("Brightness: ");
        _saturationCaption.setText("Saturation: ");
        _contrastCaption.setText("Contrast: ");
        _hueCaption.setText("Hue: ");

        _brightness.setHost(this);
        _saturation.setHost(this);
        _contrast.setHost(this);
        _hue.setHost(this);

        setText("NTSC capture and decode");
        setSize(Vector(1321, 760));
        RootWindow::create();
        _animated.start();

        _brightness.setValue(-26);
        _contrast.setValue(1.65);
        _saturation.setValue(0.30);
        _hue.setValue(0);
    }
    void sizeSet(Vector size)
    {
        _output.setPosition(Vector(20, 20));
        int w = _output.right() + 20;

        Vector vSpace(0, 15);

        _brightness.setSize(Vector(301, 24));
        _brightness.setPosition(Vector(w, 20));
        _brightnessCaption.setPosition(_brightness.bottomLeft() + vSpace);
        _brightnessText.setPosition(_brightnessCaption.topRight());

        _saturation.setSize(Vector(301, 24));
        _saturation.setPosition(_brightnessCaption.bottomLeft() + 2*vSpace);
        _saturationCaption.setPosition(_saturation.bottomLeft() + vSpace);
        _saturationText.setPosition(_saturationCaption.topRight());

        _contrast.setSize(Vector(301, 24));
        _contrast.setPosition(_saturationCaption.bottomLeft() + 2*vSpace);
        _contrastCaption.setPosition(_contrast.bottomLeft() + vSpace);
        _contrastText.setPosition(_contrastCaption.topRight());

        _hue.setSize(Vector(301, 24));
        _hue.setPosition(_contrastCaption.bottomLeft() + 2*vSpace);
        _hueCaption.setPosition(_hue.bottomLeft() + vSpace);
        _hueText.setPosition(_hueCaption.topRight());

        RootWindow::sizeSet(size);
    }
    void setBrightness(double brightness)
    {
        _output.setBrightness(brightness);
        _brightnessText.setText(format("%f", brightness));
        _brightnessText.size();
    }
    void setSaturation(double saturation)
    {
        _output.setSaturation(saturation);
        _saturationText.setText(format("%f", saturation));
        _saturationText.size();
    }
    void setContrast(double contrast)
    {
        _output.setContrast(contrast);
        _contrastText.setText(format("%f", contrast));
        _contrastText.size();
    }
    void setHue(double hue)
    {
        _output.setHue(hue);
        _hueText.setText(format("%f", hue));
        _hueText.size();
    }

    void keyboardCharacter(int character)
    {
        if (character == VK_ESCAPE)
            remove();
    }
private:
    CaptureBitmapWindow _output;
    AnimatedWindow _animated;
    TextWindow _brightnessCaption;
    BrightnessSliderWindow _brightness;
    TextWindow _brightnessText;
    TextWindow _saturationCaption;
    SaturationSliderWindow _saturation;
    TextWindow _saturationText;
    TextWindow _contrastCaption;
    ContrastSliderWindow _contrast;
    TextWindow _contrastText;
    TextWindow _hueCaption;
    HueSliderWindow _hue;
    TextWindow _hueText;

};

class Program : public WindowProgram<CaptureWindow> { };
