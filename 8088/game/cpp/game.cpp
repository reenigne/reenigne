#include "alfe/main.h"
#include "alfe/cga.h"

class GameWindow : public RootWindow
{
public:
    GameWindow()
      : _wisdom(File("wisdom")), _output(&_data, &_sequencer, &_bitmap)
    {
        _output.setConnector(1);          // old composite
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
        Byte* cgaRegisters = &_cgaBytes[_regs];
        cgaRegisters[CGAData::registerLogCharactersPerBank] = 12;
        cgaRegisters[CGAData::registerScanlinesRepeat] = 1;
        cgaRegisters[CGAData::registerHorizontalTotalHigh] = 0;
        cgaRegisters[CGAData::registerHorizontalDisplayedHigh] = 0;
        cgaRegisters[CGAData::registerHorizontalSyncPositionHigh] = 0;
        cgaRegisters[CGAData::registerVerticalTotalHigh] = 0;
        cgaRegisters[CGAData::registerVerticalDisplayedHigh] = 0;
        cgaRegisters[CGAData::registerVerticalSyncPositionHigh] = 0;
        cgaRegisters[CGAData::registerMode] = 9;
        cgaRegisters[CGAData::registerPalette] = 0;
        cgaRegisters[CGAData::registerHorizontalTotal] = 114 - 1;
        cgaRegisters[CGAData::registerHorizontalDisplayed] = 80;
        cgaRegisters[CGAData::registerHorizontalSyncPosition] = 90;
        cgaRegisters[CGAData::registerHorizontalSyncWidth] = 16;
        cgaRegisters[CGAData::registerVerticalTotal] = 128 - 1;
        cgaRegisters[CGAData::registerVerticalTotalAdjust] = 6;
        cgaRegisters[CGAData::registerVerticalDisplayed] = 100;
        cgaRegisters[CGAData::registerVerticalSyncPosition] = 112;
        cgaRegisters[CGAData::registerInterlaceMode] = 2;
        cgaRegisters[CGAData::registerMaximumScanline] = 1;
        cgaRegisters[CGAData::registerCursorStart] = 6;
        cgaRegisters[CGAData::registerCursorEnd] = 7;
        cgaRegisters[CGAData::registerStartAddressHigh] = 0;
        cgaRegisters[CGAData::registerStartAddressLow] = 0;
        cgaRegisters[CGAData::registerCursorAddressHigh] = 0;
        cgaRegisters[CGAData::registerCursorAddressLow] = 0;
        _data.setTotals(238944, 910, 238875);
        _data.change(0, -_regs, _regs + 0x4000, &_cgaBytes[0]);

        _outputSize = _output.requiredSize();

        add(&_bitmap);
        add(&_animated);

        _animated.setDrawWindow(this);
        _animated.setRate(60);

        _background.allocate(0x10000);
        _foreground.allocate(0x10000);
        _buffer.allocate(0x10000);
        _tiles.allocate(0x10000);
        _tileWidth = 16;
        _tileHeight = 16;
        _bufferStride = 256;
        _screenColumns = 160;
        _screenRows = 100;
        _mapStride = 256;

        for (int i = 0; i < 0x10000; ++i) {
            _background[i] = rand() & 0xff;
            _foreground[i] = rand() & 0xff;
            _tiles[i] = rand() & 0xff;
        }
        drawInitialScreen(0);
    }
    void create()
    {
        setText("CGA game");
        setInnerSize(_outputSize);
        _bitmap.setTopLeft(Vector(0, 0));
        _bitmap.setInnerSize(_outputSize);
        RootWindow::create();
        _animated.start();
    }
    virtual void draw()
    {
        cgaRegisters[CGAData::registerStartAddressHigh] = _startAddress >> 8;
        cgaRegisters[CGAData::registerStartAddressLow] = _startAddress & 0xff;
        _data.change(0, -_regs, _regs + 0x4000, &_cgaBytes[0]);
        _output.restart();
        _animated.restart();
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
            case VK_SPACE:
                _spacePressed = !up;
                return true;
        }
        return false;
    }
private:
    void drawTileToBuffer(Word tl, int tileIndex)
    {
        const Byte* p = &_tiles[tileIndex*_tileWidth*_tileHeight];
        int rowIncrement = _bufferStride - _tileWidth;
        for (int y = 0; y < _tileHeight; ++y) {
            for (int x = 0; x < _tileWidth; x += 2) {
                Word c = *reinterpret_cast<Word*>(p);
                *reinterpret_cast<Word*>(_buffer + tl) = c;
                p += 2;
                tl += 2;
            }
            tl += rowIncrement;
        }
    }
    void drawTransparentTileToBuffer(Word tl, int tileIndex)
    {
        const Byte* p = &_tiles[tileIndex*_tileWidth*_tileHeight];
        int rowIncrement = _bufferStride - _tileWidth;
        for (int y = 0; y < _tileHeight; ++y) {
            for (int x = 0; x < _tileWidth; x += 2) {
                Word c = *reinterpret_cast<Word*>(p);
                if (c != 0xffff)
                    *reinterpret_cast<Word*>(_buffer + tl) = c;
                p += 2;
                tl += 2;
            }
            tl += rowIncrement;
        }
    }
    void drawInitialScreen(int tl)  // tl here is index into foreground/background, not buffer
    {
        int bufferRow = -_tileHeight*_bufferStride - _tileWidth;
        int mapRow = -_mapStride - 1;
        for (int y = -_tileHeight; y < _screenRows + _tileHeight; y += _tileHeight) {
            int buffer = bufferRow;
            int map = mapRow;
            for (int x = -_tileWidth; x < _screenColumns + _tileWidth; x += _tileWidth) {
                drawTileToBuffer(buffer, _background[map]);
                drawTransparentTiletoBuffer(buffer, _foreground[map]);
                buffer += _tileWidth;
                ++map;
            }
            bufferRow += _bufferStride*_tileHeight;
            mapRow += _mapStride;
        }
    }


    FFTWWisdom<float> _wisdom;
    CGAData _data;
    CGASequencer _sequencer;
    CGAOutput _output;
    AnimatedWindow _animated;
    BitmapWindow _bitmap;
    Array<Byte> _cgaBytes;

    Byte* _vram;
    Word _startAddress;

    Array<Byte> _background;
    Array<Byte> _foreground;
    Array<Byte> _buffer;
    Array<Byte> _tiles;

    int _tileWidth;
    int _tileHeight;
    int _bufferStride;
    int _screenColumns;
    int _screenRows;
    int _mapStride;
    int _regs;

    bool _upPressed;
    bool _downPressed;
    bool _leftPressed;
    bool _rightPressed;
    bool _spacePressed;
};

class Program : public WindowProgram<GameWindow>
{
};
