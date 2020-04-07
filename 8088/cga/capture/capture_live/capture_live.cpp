#include "alfe/main.h"
#include "alfe/complex.h"
#include "alfe/terminal6.h"
#include "alfe/bitmap_png.h"
#include "alfe/evaluate.h"
#include "alfe/ntsc_decode.h"

template<class T> class CaptureBitmapWindowT;
typedef CaptureBitmapWindowT<void> CaptureBitmapWindow;

template<class T> class DecoderThreadT : public ThreadTask
{
public:
    DecoderThreadT() : _window(0) { }
    void setWindow(CaptureBitmapWindow* window)
    {
        _window = window;
        restart();
    }
private:
    void run()
    {
        while (true) {
            if (cancelling())
                return;
            _window->update();
        }
    }

    CaptureBitmapWindow* _window;
};

typedef DecoderThreadT<void> DecoderThread;

class CaptureWindow;

template<class T> class CaptureBitmapWindowT : public BitmapWindow
{
public:
    ~CaptureBitmapWindowT()
    {
        _thread.cancel();
    }
    void setCaptureWindow(CaptureWindow* captureWindow)
    {
        _captureWindow = captureWindow;
    }
    void create()
    {
        setInnerSize(Vector(960, 720));

        _vbiCapPipe = File("\\\\.\\pipe\\vbicap", true).openPipe();
        _vbiCapPipe.write<int>(1);

        int samples = 450*1024;
        int sampleSpaceBefore = 256;
        int sampleSpaceAfter = 256;
        _buffer.allocate(sampleSpaceBefore + samples + sampleSpaceAfter);
        _b = &_buffer[0] + sampleSpaceBefore;
        for (int i = 0; i < sampleSpaceBefore; ++i)
            _b[i - sampleSpaceBefore] = 0;
        for (int i = 0; i < sampleSpaceAfter; ++i)
            _b[i + samples] = 0;

        _decoder.setInputBuffer(_b);
        _decoder.setOutputPixelsPerLine(1140);
        _decoder.setYScale(3);

        BitmapWindow::create();
        _thread.setWindow(this);
    }

    void update()
    {
        _vbiCapPipe.read(_b, 1024*450);
        _decoder.setOutputBuffer(_bitmap);
        _decoder.decode();
        Bitmap<DWORD> nextBitmap = setNextBitmap(_bitmap);
        invalidate();
        _bitmap = nextBitmap;
    }

    void draw()
    {
        if (!_bitmap.valid())
            _bitmap = Bitmap<DWORD>(Vector(960, 720));
    }

    void paint()
    {
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
    void setSharpness(double sharpness)
    {
        _decoder.setChromaSamples(sharpness);
    }

private:
    Bitmap<DWORD> _bitmap;

    CaptureWindow* _captureWindow;

    NTSCCaptureDecoder<DWORD> _decoder;

    Stream _vbiCapPipe;

    Array<Byte> _buffer;
    Byte* _b;

    DecoderThread _thread;
};

template<class T> class BrightnessSliderWindowT : public Slider
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
typedef BrightnessSliderWindowT<void> BrightnessSliderWindow;

template<class T> class SaturationSliderWindowT : public Slider
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
typedef SaturationSliderWindowT<void> SaturationSliderWindow;

template<class T> class ContrastSliderWindowT : public Slider
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
typedef ContrastSliderWindowT<void> ContrastSliderWindow;

template<class T> class HueSliderWindowT : public Slider
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
typedef HueSliderWindowT<void> HueSliderWindow;

template<class T> class SharpnessSliderWindowT : public Slider
{
public:
    void setHost(CaptureWindow* host) { _host = host; }
    void valueSet(double value) { _host->setSharpness(value); }
    void create()
    {
        setRange(1, 16);
        Slider::create();
    }
private:
    CaptureWindow* _host;
};
typedef SharpnessSliderWindowT<void> SharpnessSliderWindow;

class CaptureWindow : public RootWindow
{
public:
    CaptureWindow()
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
        add(&_sharpnessCaption);
        add(&_sharpness);
        add(&_sharpnessText);

        _animated.setDrawWindow(&_output);
        _animated.setRate(60);
    }
    void restart() { _animated.restart(); }
    void create()
    {
        _brightnessCaption.setText("Brightness: ");
        _saturationCaption.setText("Saturation: ");
        _contrastCaption.setText("Contrast: ");
        _hueCaption.setText("Hue: ");
        _sharpnessCaption.setText("Sharpness: ");

        _brightness.setHost(this);
        _saturation.setHost(this);
        _contrast.setHost(this);
        _hue.setHost(this);
        _sharpness.setHost(this);

        setText("NTSC capture and decode");
        setInnerSize(Vector(1321, 760 + 23));
        RootWindow::create();
        _animated.start();

        _brightness.setValue(-26);
        _contrast.setValue(1.65);
        _saturation.setValue(0.30);
        _hue.setValue(0);
        _sharpness.setValue(8);
    }
    void innerSizeSet(Vector size)
    {
        _output.setPosition(Vector(20, 20));
        int w = _output.right() + 20;

        Vector vSpace(0, 15);

        _brightness.setInnerSize(Vector(301, 24));
        _brightness.setPosition(Vector(w, 20));
        _brightnessCaption.setPosition(_brightness.bottomLeft() + vSpace);
        _brightnessText.setPosition(_brightnessCaption.topRight());

        _saturation.setInnerSize(Vector(301, 24));
        _saturation.setPosition(_brightnessCaption.bottomLeft() + 2*vSpace);
        _saturationCaption.setPosition(_saturation.bottomLeft() + vSpace);
        _saturationText.setPosition(_saturationCaption.topRight());

        _contrast.setInnerSize(Vector(301, 24));
        _contrast.setPosition(_saturationCaption.bottomLeft() + 2*vSpace);
        _contrastCaption.setPosition(_contrast.bottomLeft() + vSpace);
        _contrastText.setPosition(_contrastCaption.topRight());

        _hue.setInnerSize(Vector(301, 24));
        _hue.setPosition(_contrastCaption.bottomLeft() + 2*vSpace);
        _hueCaption.setPosition(_hue.bottomLeft() + vSpace);
        _hueText.setPosition(_hueCaption.topRight());

        _sharpness.setInnerSize(Vector(301, 24));
        _sharpness.setPosition(_hueCaption.bottomLeft() + 2*vSpace);
        _sharpnessCaption.setPosition(_sharpness.bottomLeft() + vSpace);
        _sharpnessText.setPosition(_sharpnessCaption.topRight());

        RootWindow::innerSizeSet(size);
    }
    void setBrightness(double brightness)
    {
        _output.setBrightness(brightness);
        _brightnessText.setText(format("%f", brightness));
        _brightnessText.autoSize();
    }
    void setSaturation(double saturation)
    {
        _output.setSaturation(saturation);
        _saturationText.setText(format("%f", saturation));
        _saturationText.autoSize();
    }
    void setContrast(double contrast)
    {
        _output.setContrast(contrast);
        _contrastText.setText(format("%f", contrast));
        _contrastText.autoSize();
    }
    void setHue(double hue)
    {
        _output.setHue(hue);
        _hueText.setText(format("%f", hue));
        _hueText.autoSize();
    }
    void setSharpness(double sharpness)
    {
        _output.setSharpness(sharpness);
        _sharpnessText.setText(format("%f", sharpness));
        _sharpnessText.autoSize();
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
    TextWindow _sharpnessCaption;
    SharpnessSliderWindow _sharpness;
    TextWindow _sharpnessText;
};

class Program : public WindowProgram<CaptureWindow> { };
