#include "alfe/main.h"
//#include "alfe/bitmap_png.h"
#include "alfe/colour_space.h"
#include "alfe/user.h"
#include "alfe/cga.h"
#include "alfe/config_file.h"

Array<Byte> landscape;
Array<int16_t> uTable;
Array<int16_t> vTable;
Array<Byte> perspective;
static const int maxAngle = 160;
static const int radii = 32;
static const int width = 40;
static const int height = 100;
int initialAngle = 0;
int initialU = 64;
int initialV = 64;
int yMinMin;
int yMinMax;

class LandscapeWindow : public RootWindow
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
        _output->setAspectRatio(5.0 / 6.0);
        _output->setOverscan(0.1);
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
        _vram[CGAData::registerMode] = 0x08;
        _vram[CGAData::registerPalette] = 0x01;
        _vram[CGAData::registerHorizontalTotal] = 57 - 1;
        _vram[CGAData::registerHorizontalDisplayed] = 40;
        _vram[CGAData::registerHorizontalSyncPosition] = 45;
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
        _animated.setRate(11);

        for (int i = 0; i < width*height; ++i)
            _vram[i * 2] = 0xb0;

        _xPressed = false;
        _zPressed = false;
        _leftPressed = false;
        _rightPressed = false;
        _upPressed = false;
        _downPressed = false;
        _xVelocity = 0;
        _yVelocity = 0;
        _angularVelocity = 0;
        yMinMin = 9999;
        yMinMax = -9999;
    }
    ~LandscapeWindow() { join(); }
    void join() { _output->join(); }
    void create()
    {
        setText("Landscape");
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

        //int initialAngle = 0;
        //int initialU = 64;

        int angle = initialAngle;
        for (int x = 0; x < width; ++x) {
            int yMin = height;
            for (int r = 0; r < radii; ++r) {
                int v = (vTable[r*maxAngle + angle] + initialV) & 0x7f;
                int u = (uTable[r*maxAngle + angle] + initialU) & 0x7f;
                int height = *reinterpret_cast<uint16_t*>(&landscape[(u*128 + v)*4 + 2])/4;
                int y = perspective[r*256 + height];
                int colour = landscape[(u*128 + v)*4];
                while (yMin > y) {
                    _vram[yMin*80 + x*2 + 1] = colour;
                    --yMin;
                }
            }
            if (yMin < yMinMin)
                yMinMin = yMin;
            if (yMin > yMinMax)
                yMinMax = yMin;
            while (yMin > 0) {
                _vram[yMin*80 + x*2 + 1] = 0;
                --yMin;
            }
            ++angle;
            //angle += 4;
            if (angle >= maxAngle)
                angle -= maxAngle;
        }
        //++initialAngle;
        //if (initialAngle == maxAngle)
        //  initialAngle = 0;
        //++initialU;
        //if (initialU == 128)
        //  initialU = 0;

        if (_xPressed) {
            if (!_zPressed)
                _angularVelocity += 0.5;
        }
        else {
            if (_zPressed)
                _angularVelocity -= 0.5;
            else
                _angularVelocity *= 0.9;
        }
        if (_angularVelocity < -2)
            _angularVelocity = -2;
        if (_angularVelocity > 2)
            _angularVelocity = 2;
        initialAngle += _angularVelocity; // *tau/160;
        initialAngle = (initialAngle + maxAngle) % maxAngle;
        float _a = initialAngle*tau/maxAngle;
        if (_a < 0)
            _a += tau;
        if (_a >= tau)
            _a -= tau;
        static const int accel = 4;
        if (_upPressed) {
            if (!_downPressed) {
                _xVelocity += accel*sin(_a);
                _yVelocity += accel*cos(_a);
            }
        }
        else {
            if (_downPressed) {
                _xVelocity -= accel*sin(_a);
                _yVelocity -= accel*cos(_a);
            }
        }
        if (_leftPressed) {
            if (!_rightPressed) {
                _xVelocity -= accel*cos(_a);
                _yVelocity += accel*sin(_a);
            }
        }
        else {
            if (_rightPressed) {
                _xVelocity += accel*cos(_a);
                _yVelocity -= accel*sin(_a);
            }
        }
        static const int maxVelocity = 2;
        if (_xVelocity > maxVelocity)
            _xVelocity = maxVelocity;
        if (_xVelocity < -maxVelocity)
            _xVelocity = -maxVelocity;
        if (_yVelocity > maxVelocity)
            _yVelocity = maxVelocity;
        if (_yVelocity < -maxVelocity)
            _yVelocity = -maxVelocity;
        if (!_leftPressed && !_rightPressed && !_upPressed && !_downPressed) {
            _xVelocity *= 0.8;
            _yVelocity *= 0.8;
        }
        initialU = (initialU + static_cast<int>(_xVelocity)) & 0x7f;
        initialV = (initialV + static_cast<int>(_yVelocity)) & 0x7f;

        //if (_xPressed)
        //  ++initialAngle;
        //if (_zPressed)
        //  --initialAngle;
        //initialAngle = (initialAngle + maxAngle) % maxAngle;
        //if (_upPressed)
        //  --initialU;
        //if (_downPressed)
        //  ++initialU;
        //if (_leftPressed)
        //  --initialV;
        //if (_rightPressed)
        //  ++initialV;
        //initialU &= 0x7f;
        //initialV &= 0x7f;
    }
    bool keyboardEvent(int key, bool up)
    {
        switch (key) {
            case VK_RIGHT:
                _rightPressed = !up;
                return true;
            case VK_LEFT:
                _leftPressed = !up;
                return true;
            case VK_UP:
                _upPressed = !up;
                return true;
            case VK_DOWN:
                _downPressed = !up;
                return true;
            case 'Z':
                _zPressed = !up;
                return true;
            case 'X':
                _xPressed = !up;
                return true;
            case VK_ESCAPE:
                PostQuitMessage(0);
                return true;
            //case VK_SPACE:
            //  _spacePressed = !up;
            //  return true;
        }
        return false;
    }
    BitmapWindow* outputWindow() { return &_bitmap; }
    CGAData* getData() { return &_data; }
    CGASequencer* getSequencer() { return &_sequencer; }
private:
    CGAData _data;
    CGASequencer _sequencer;
    CGAOutput* _output;
    ConfigFile* _configFile;
    AnimatedWindow _animated;
    BitmapWindow _bitmap;
    Vector _outputSize;
    Array<Byte> _cgaBytes;
    Byte* _vram;
    Byte* _landscape;
    int _regs;
    bool _leftPressed;
    bool _rightPressed;
    bool _upPressed;
    bool _downPressed;
    bool _xPressed;
    bool _zPressed;
    float _angularVelocity;
    float _xVelocity;
    float _yVelocity;
};

class Program : public WindowProgram<LandscapeWindow>
{
public:
    void run()
    {
        for (int i = 0; i < 160; ++i) {
            int j = ((i*402) >> 1) & 0xff;
            //if (j >= 0x80)
            //  j -= 0x100;
            console.write(hex(j & 0xffff, 4) + "\n");
        }

        // Rotate sky 90 degrees clockwise
        // (Turn upside down and swap axes)
        Array<Byte> sky;
        Array<Byte> sky2;
        File("q:\\voxelsky.bin", true).readIntoArray(&sky);
        sky2.allocate(3200);
        for (int y = 0; y < 10; ++y)
            for (int x = 0; x < 160; ++x) {
                sky2[(x*10 + (9 - y))*2    ] = sky[(y*160 + x)*2    ];
                sky2[(x*10 + (9 - y))*2 + 1] = sky[(y*160 + x)*2 + 1];
            }
        File("q:\\voxelsky2.bin", true).openWrite().write(sky2);


        Array<Byte> colour;
        Array<Byte> heights;
        File("q:\\morwenstow_colour1.raw", true).readIntoArray(&colour);
        File("q:\\morwenstow_height.raw", true).readIntoArray(&heights);

        int minHeight = 0xff;
        int maxHeight = 0;

        landscape.ensure(0x10000);
        uTable.ensure(radii*maxAngle);
        vTable.ensure(radii*maxAngle);
        perspective.ensure(radii*256);

        for (int p = 0; p < 0x4000; ++p) {
            landscape[p * 4] = colour[p];
            landscape[p * 4 + 1] = 0;
            // (32 entries * 2 bytes per entry)/(16 bytes per paragraph)
            int h = heights[p] * 4;
            minHeight = min(h, minHeight);
            maxHeight = max(h, maxHeight);
            landscape[p * 4 + 2] = h & 0xff;
            landscape[p * 4 + 3] = h >> 8;
        }
        console.write("minHeight = " + decimal(minHeight) + ", maxHeight = " + decimal(maxHeight) + "\n");
        File("landscape.dat").openWrite().write(landscape);

        console.write("sineTable:\n");
        for (int a = 0; a < maxAngle; ++a) {
            double s = sin(a * tau / maxAngle) * 32767;
            int ss = static_cast<int>(s);
            if ((a & 7) == 0)
                console.write("  dw ");
            console.write(String(hex(ss, 4)));
            if ((a & 7) < 7)
                console.write(", ");
            else
                console.write("\n");
        }

        Array<int> rad;
        rad.ensure(32);
        for (int r = 1; r <= radii; ++r) {
            int rr = (69 * r * r + 723 * r + 138) / 930;
            rad[r-1] = rr;
            console.write(String(decimal(rr)) + "\n");

            for (int a = 0; a < maxAngle; ++a) {
                uTable[(r-1)*maxAngle + a] = static_cast<int>(rr * sin(a*tau/maxAngle));
                vTable[(r-1)*maxAngle + a] = static_cast<int>(rr * cos(a*tau/maxAngle));
            }

            for (int h = 0; h < 256; ++h)
                perspective[(r-1)*256 + h] = clamp(0, static_cast<int>((1000 - h*10)/(rr) + 10), 99);
        }

        for (int i = 0; i < 402; ++i) {
            float a = i*tau/402;
            int u = 64 + 64*sin(a);
            int v = 64 + 64*cos(a);
            float p = 3;
            float q = 0.5;
            float k = 0;
            float b = a + tau/2 -tau/8 + sin(a*p + k)*q;
            for (int j = 0; j < 40; ++j) {
                float c = b + tau*j/160;
                for (int d = 0; d < 32; ++d) {
                    int r = rad[d];
                    int uu = u + r*sin(c);
                    int vv = v + r*cos(c);
                    //if (uu <= 0 || vv >= 127 || vv <= 0 || vv >= 127) {
                    //  printf("Outside %i %i\n", i, j);
                    //  break;
                    //}
                    if (uu == 127)
                        printf("Outside %i %i %i\n", i, j, d);
                }
            }
        }

        //ConfigFile configFile;
        //configFile.addDefaultOption("cgaROM", String("5788005.u33"));
        //configFile.addDefaultOption("fftWisdom", String("wisdom"));

        //String configName = "default.config";
        //if (_arguments.count() >= 2)
        //    configName = _arguments[1];
        //File configPath(configName, true);
        //configFile.load(configPath);
        //FFTWWisdom<float> wisdom(File(configFile.get<String>("fftWisdom"),
        //    configPath.parent()));

        //CGAOutput output(_window.getData(), _window.getSequencer(),
        //    _window.outputWindow());
        //_window.setOutput(&output);

        //_window.setConfig(&configFile, configPath);

        //WindowProgram::run();
        //_window.join();

        console.write("yMinMin = " + decimal(yMinMin) + ", yMinMax = " + decimal(yMinMax) + "\n");
    }
};
