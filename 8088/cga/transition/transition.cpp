#include "alfe/main.h"
#include "alfe/cga.h"

class Collect
{
public:
    Collect(AppendableArray<File>* collection) : _collection(collection) { }
    void operator()(const FileSystemObject& file)
    {
        File f(file);
        if (f.valid())
            _collection->append(f);
    }
    void operator()(const Directory& directory) { }
private:
    AppendableArray<File>* _collection;
};

class TransitionWindow : public RootWindow
{
public:
    TransitionWindow()
        : _wisdom(File("wisdom")), _output(&_data, &_sequencer, &_bitmap)
    {
        _sequencer.setROM(File("5788005.u33"));

        _output.setConnector(0);          // RGBI
        _output.setScanlineProfile(0);    // rectangle
        _output.setHorizontalProfile(0);  // rectangle
        _output.setScanlineWidth(1);
        _output.setScanlineBleeding(2);   // symmetrical
        _output.setHorizontalBleeding(2); // symmetrical
        _output.setZoom(2);
        _output.setHorizontalRollOff(0);
        _output.setHorizontalLobes(4);
        _output.setVerticalRollOff(0);
        _output.setVerticalLobes(4);
        _output.setSubPixelSeparation(1);
        _output.setPhosphor(0);           // colour
        _output.setMask(0);
        _output.setMaskSize(0);
        _output.setAspectRatio(5.0/6.0);
        _output.setOverscan(0);
        _output.setCombFilter(0);         // no filter
        _output.setHue(0);
        _output.setSaturation(100);
        _output.setContrast(100);
        _output.setBrightness(0);
        _output.setShowClipping(false);
        _output.setChromaBandwidth(1);
        _output.setLumaBandwidth(1);
        _output.setRollOff(0);
        _output.setLobes(1.5);
        _output.setPhase(1);

        _regs = -CGAData::registerLogCharactersPerBank;
        _cgaBytes.allocate(0x4000 + _regs);
        _vram = &_cgaBytes[_regs];
        _vram[CGAData::registerLogCharactersPerBank] = 12;
        _vram[CGAData::registerScanlinesRepeat] = 1;
        _vram[CGAData::registerHorizontalTotalHigh] = 0;
        _vram[CGAData::registerHorizontalDisplayedHigh] = 0;
        _vram[CGAData::registerHorizontalSyncPositionHigh] = 0;
        _vram[CGAData::registerVerticalTotalHigh] = 0;
        _vram[CGAData::registerVerticalDisplayedHigh] = 0;
        _vram[CGAData::registerVerticalSyncPositionHigh] = 0;
        _vram[CGAData::registerMode] = 9;
        _vram[CGAData::registerPalette] = 0;
        _vram[CGAData::registerHorizontalTotal] = 114 - 1;
        _vram[CGAData::registerHorizontalDisplayed] = 80;
        _vram[CGAData::registerHorizontalSyncPosition] = 90;
        _vram[CGAData::registerHorizontalSyncWidth] = 10; // 16;
        _vram[CGAData::registerVerticalTotal] = 128 - 1;
        _vram[CGAData::registerVerticalTotalAdjust] = 6;
        _vram[CGAData::registerVerticalDisplayed] = 100;
        _vram[CGAData::registerVerticalSyncPosition] = 112;
        _vram[CGAData::registerInterlaceMode] = 2;
        _vram[CGAData::registerMaximumScanline] = 1;
        _vram[CGAData::registerCursorStart] = 6;
        _vram[CGAData::registerCursorEnd] = 7;
        _vram[CGAData::registerStartAddressHigh] = 0;
        _vram[CGAData::registerStartAddressLow] = 0;
        _vram[CGAData::registerCursorAddressHigh] = 0;
        _vram[CGAData::registerCursorAddressLow] = 0;
        _data.setTotals(238944, 910, 238875);
        _data.change(0, -_regs, _regs + 0x4000, &_cgaBytes[0]);

        _outputSize = _output.requiredSize();

        add(&_bitmap);
        add(&_animated);

        _animated.setDrawWindow(this);
        _animated.setRate(60);

        AppendableArray<File> imageFiles;
        applyToWildcard(Collect(&imageFiles), "*.dat", true,
            Directory("q:\\external\\afh", true));
        _images.allocate(imageFiles.count());
        for (int i = 0; i < imageFiles.count(); ++i)
            _images[i] = imageFiles[i].contents();

        for (int i = 0; i < 16000; ++i)
            _vram[i] = 0;

        _wipes = 3;
        _gradientWipe.allocate(8000 * _wipes);
        for (int i = 0; i < 8000; ++i)
            _gradientWipe[i] = (i*256)/8000;
        for (int i = 0; i < 8000; ++i)
            _gradientWipe[i + 8000] = _gradientWipe[i];
        for (int i = 0; i < 8000 - 3; ++i) {
            int j = (rand() % (8000 - i)) + i;
            Byte t = _gradientWipe[i + 8000];
            _gradientWipe[i + 8000] = _gradientWipe[j + 8000];
            _gradientWipe[j + 8000] = t;
        }
        for (int i = 0; i < 8000; ++i)
            _gradientWipe[i + 16000] = _gradientWipe[i + 8000];
        for (int i = 0; i < 40000000; ++i) {
            int p1 = rand() % 8000;
            int p2 = rand() % 8000;
            int metricUnswapped = metric(p1, p2);
            swap(p1, p2);
            int metricSwapped = metric(p1, p2);
            if (metricSwapped > metricUnswapped)
                swap(p1, p2);

        }

        _transitionFrames = 256;
        initTransition();
    }
    ~TransitionWindow() { _output.join(); }
    void create()
    {
        setText("CGA transitions");
        setInnerSize(_outputSize);
        _bitmap.setTopLeft(Vector(0, 0));
        _bitmap.setInnerSize(_outputSize);
        RootWindow::create();
        _animated.start();
    }
    virtual void draw()
    {
        _data.change(0, -_regs, _regs + 0x4000, &_cgaBytes[0]);
        _output.restart();
        _animated.restart();
        ++_transitionFrame;
        for (int i = 0; i < 8000; ++i)
            if (_transitionFrame > _gradientWipe[i + _wipeNumber*8000]) {
                _vram[i*2] = _newImage[i*2];
                _vram[i*2 + 1] = _newImage[i*2 + 1];
            }
        if (_transitionFrame == _transitionFrames)
            initTransition();
    }
private:
    void initTransition()
    {
        int lastImage = _imageIndex;
        _imageIndex = rand() % (_images.count() - 1);
        if (_imageIndex >= lastImage)
            ++_imageIndex;
        _newImage = _images[_imageIndex];
        _transitionFrame = 0;
        _wipeNumber = 2; // rand() % _wipes;
    }

    void swap(int i, int j)
    {
        Byte t = _gradientWipe[i + 16000];
        _gradientWipe[i + 16000] = _gradientWipe[j + 16000];
        _gradientWipe[j + 16000] = t;
    }
    int metric(int i, int j)
    {
        return metricForPoint(i) + metricForPoint(j);
    }
    int metricForPoint(int i)
    {
        int x = i % 80;
        int y = i / 80;
        int h = secondDerivative(x, y, (x + 1)%80, y, (x + 79)%80, y)*3;
        int v = secondDerivative(x, y, x, (y + 1)%100, x, (y + 99)%100)*5;
        return h*h + v*v;
    }
    int secondDerivative(int x1, int y1, int x0, int y0, int x2, int y2)
    {
        return getByte(x2, y2) + getByte(x0, y0) - 2*getByte(x1, y1);
    }
    int getByte(int x, int y) { return _gradientWipe[y*80 + x + 16000]; }


    FFTWWisdom<float> _wisdom;
    CGAData _data;
    CGASequencer _sequencer;
    CGAOutput _output;
    AnimatedWindow _animated;
    BitmapWindow _bitmap;
    Vector _outputSize;
    Array<Byte> _cgaBytes;
    Byte* _vram;
    int _regs;

    Array<String> _images;
    String _newImage;
    int _imageIndex;

    int _transitionFrames;
    int _transitionFrame;
    int _wipeNumber;

    int _wipes;
    Array<Byte> _gradientWipe;
};

class Program : public WindowProgram<TransitionWindow>
{
};
