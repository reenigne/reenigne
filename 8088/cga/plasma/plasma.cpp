#include "alfe/main.h"
#include "alfe/cga.h"
#include "alfe/config_file.h"

int shadesGeometry[] = {
    45,   51,   73,   87,
    41,   57,   71,   91,
    39,   59,   69,   95,
    39,   59,   67,   95,
    37,   61,   67,   97,
    37,   61,   67,   97,
    37,   61,   67,   97,
    37,   61,   67,   99,
    37,   61,   69,   99,
    37,   59,   69,   99,
    37,   59,   69,   99,
    37,   59,   71,   99,
    37,   57,   71,   99,
    37,   57,   71,   99,
    39,   57,   73,   99,
    39,   55,   73,   99,
    39,   55,   75,   97,
    41,   53,   77,   97,
    41,   53,   79,   95,
    43,   51,   81,   95,
    45,   49,   83,   93};

class PlasmaWindow : public RootWindow
{
public:
    void setOutput(CGAOutput* output) { _output = output; }
    void setConfig(ConfigFile* configFile, File configPath)
    {
        _configFile = configFile;
        _sequencer.setROM(
            File(configFile->get<String>("cgaROM"), configPath.parent()));

        _output->setConnector(0);          // RGBI
        _output->setScanlineProfile(0);    // rectangle
        _output->setHorizontalProfile(0);  // rectangle
        _output->setScanlineWidth(1);
        _output->setScanlineBleeding(2);   // symmetrical
        _output->setHorizontalBleeding(2); // symmetrical
        _output->setZoom(2);
        _output->setHorizontalRollOff(0);
        _output->setHorizontalLobes(4);
        _output->setVerticalRollOff(0);
        _output->setVerticalLobes(4);
        _output->setSubPixelSeparation(1);
        _output->setPhosphor(0);           // colour
        _output->setMask(0);
        _output->setMaskSize(0);
        _output->setAspectRatio(5.0/6.0);
        _output->setOverscan(0);
        _output->setCombFilter(0);         // no filter
        _output->setHue(0);
        _output->setSaturation(100);
        _output->setContrast(100);
        _output->setBrightness(0);
        _output->setShowClipping(false);
        _output->setChromaBandwidth(1);
        _output->setLumaBandwidth(1);
        _output->setRollOff(0);
        _output->setLobes(1.5);
        _output->setPhase(1);

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

        _outputSize = _output->requiredSize();

        add(&_bitmap);
        add(&_animated);

        _animated.setDrawWindow(this);
        _animated.setRate(60);

        _sin.allocate(512);
        for (int i = 0; i < 512; ++i)
            _sin[i] = static_cast<int>(1024*sin(i*tau/512));
        _frame = 0;

        _gradient.allocate(16);
        _gradient[0] = 0x08b1;
        _gradient[1] = 0x88b1;
        _gradient[2] = 0x84b1;
        _gradient[3] = 0x44b1;
        _gradient[4] = 0x45b1;
        _gradient[5] = 0x55b1;
        _gradient[6] = 0x59b1;
        _gradient[7] = 0x99b1;
        _gradient[8] = 0x93b1;
        _gradient[9] = 0x33b1;
        _gradient[10] = 0x3ab1;
        _gradient[11] = 0xaab1;
        _gradient[12] = 0xaeb1;
        _gradient[13] = 0xeeb1;
        _gradient[14] = 0xefb1;
        _gradient[15] = 0xffb1;

        String image = File("yuiShades.bin", true).contents().
            subString(0, 16000);
        for (int i = 0; i < 16000; ++i)
            _vram[i] = image[i];

        String asmOutput;
        asmOutput += "%macro updateRoutine 0\n";
        String asmOutput2;
        int lastX = (shadesGeometry[0] - 1)/2;
        for (int i = 0; i < 21; ++i) {
            int x0 = shadesGeometry[i*4];
            int x1 = shadesGeometry[i*4 + 1];
            int x2 = shadesGeometry[i*4 + 2];
            int x3 = shadesGeometry[i*4 + 3];
            for (int x = x0; x < x1; x += 2) {
                _vram[(i + 38)*160 + x - 1] = 0xb1;
                _vram[(i + 38)*160 + x] = 0;
                asmOutput += "  movsb\n";
                if (x != x1 - 2)
                    asmOutput += "  inc di\n";
                int nx = (x - 1)/2;
                if (edgeShade((x-1)/2, i + 38))
                    asmOutput2 += "  plasmaIteration 1," + decimal(nx - lastX) + "\n";
                else
                    asmOutput2 += "  plasmaIteration 0," + decimal(nx - lastX) + "\n";
                lastX = nx;
            }
            asmOutput += "  add di," + decimal(x2 - x1) + "\n";
            for (int x = x2; x < x3; x += 2) {
                _vram[(i + 38)*160 + x - 1] = 0xb1;
                _vram[(i + 38)*160 + x] = 0;
                asmOutput += "  movsb\n";
                if (x != x3 - 2)
                    asmOutput += "  inc di\n";
                int nx = (x - 1)/2;
                if (edgeShade((x-1)/2, i + 38))
                    asmOutput2 += "  plasmaIteration 1," + decimal(nx - lastX) + "\n";
                else
                    asmOutput2 += "  plasmaIteration 0," + decimal(nx - lastX) + "\n";
                lastX = nx;
            }
            if (i != 20) {
                int d = shadesGeometry[i*4 + 4] - x3;
                asmOutput += "  add di," + decimal(d + 160) + "\n";
                asmOutput2 += "  plasmaIncrementY\n";
            }
        }
        asmOutput += "%endmacro\n";
        asmOutput += "%macro plasmaRoutine 0\n";
        asmOutput += asmOutput2;
        asmOutput += "%endmacro\n";
        asmOutput += "%endmacro\n";
        asmOutput += "%macro dataTables 0\n";
        asmOutput += "image:\n";
        for (int i = 0; i < 8000; ++i) {
            if ((i & 15) == 0)
                asmOutput += "  dw ";
            asmOutput += hex(_vram[i*2] + _vram[i*2 + 1]*256, 4);
            if ((i & 15) != 15)
                asmOutput += ", ";
            else
                asmOutput += "\n";
        }
        asmOutput += "sinTable:\n";
        for (int i = 0; i < 512; ++i) {
            if ((i & 15) == 0)
                asmOutput += "  dw ";
            asmOutput += decimal(_sin[i]);
            if ((i & 15) != 15)
                asmOutput += ", ";
            else
                asmOutput += "\n";
        }

        asmOutput += "%endmacro\n";
        asmOutput += "initlalUpdateOffset equ " +
            decimal(38*160 + shadesGeometry[0]) + "\n";
        File("tables.inc").openWrite().write(asmOutput);
    }
    ~PlasmaWindow() { join(); }
    void join() { _output->join(); }
    void create()
    {
        setText("CGA plasma");
        setInnerSize(_outputSize);
        _bitmap.setTopLeft(Vector(0, 0));
        _bitmap.setInnerSize(_outputSize);
        RootWindow::create();
        _animated.start();
    }
    virtual void draw()
    {
        _data.change(0, -_regs, _regs + 0x4000, &_cgaBytes[0]);
        _output->restart();
        _animated.restart();

        int c = 0;
        for (int y = 0; y < 100; ++y) {                     
            int vy = _sin[(_frame*16 + y*24) & 0x1ff] + _sin[(-_frame + y*3) & 0x1ff];
            for (int x = 0; x < 80; ++x) {
                if (!inShade(x, y))
                    continue;
                int v = _sin[(_frame*8 + x*40) & 0x1ff] + _sin[(_frame*2 + x*5) & 0x1ff] + vy;
                Word w = _gradient[clamp(0, 8 + (v >> 8), 15)];
                int p = y*160 + x*2;
                _vram[p] = w;
                if (edgeShade(x, y))
                    w &= 0xf0ff;
                _vram[p + 1] = (w >> 8);
                ++c;
            }
        }
        ++_frame;
    }
    BitmapWindow* outputWindow() { return &_bitmap; }
    CGAData* getData() { return &_data; }
    CGASequencer* getSequencer() { return &_sequencer; }
private:
    bool edgeShade(int x, int y)
    {
        return !inShade(x - 1, y) || !inShade(x + 1, y) || !inShade(x, y - 1) || !inShade(x, y + 1);
    }
    bool inShade(int x, int y)
    {
        y -= 38;
        if (y < 0 || y > 20)
            return false;
        int* sg = &shadesGeometry[y*4];
        int xx = x*2 + 1;
        return (xx >= sg[0] && xx < sg[1]) || (xx >= sg[2] && xx < sg[3]);
    }

    CGAData _data;
    CGASequencer _sequencer;
    CGAOutput* _output;
    ConfigFile* _configFile;
    AnimatedWindow _animated;
    BitmapWindow _bitmap;
    Vector _outputSize;
    Array<Byte> _cgaBytes;
    Byte* _vram;
    int _regs;
    int _frame;

    Array<int> _sin;
    Array<Word> _gradient;
};

class Program : public WindowProgram<PlasmaWindow>
{
public:
    void run()
    {
        ConfigFile configFile;
        configFile.addDefaultOption("cgaROM", String("5788005.u33"));
        configFile.addDefaultOption("fftWisdom", String("wisdom"));

        String configName = "default.config";
        if (_arguments.count() >= 2)
            configName = _arguments[1];
        File configPath(configName, true);
        configFile.load(configPath);
        FFTWWisdom<float> wisdom(File(configFile.get<String>("fftWisdom"),
            configPath.parent()));

        CGAOutput output(_window.getData(), _window.getSequencer(),
            _window.outputWindow());
        _window.setOutput(&output);

        _window.setConfig(&configFile, configPath);

        WindowProgram::run();
        _window.join();
    }
};
