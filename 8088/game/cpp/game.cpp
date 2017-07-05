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
        _tileWidthCharacters = _tileWidth >> 1;
        _tileHeight = 16;
        _bufferStride = 256;
        _screenColumns = 80;
        _screenRows = 100;
        _mapStride = 256;
        _horizontalAcceleration = 0x100;
        _verticalAcceleration = 0x100;
        _horizontalMaxVelocity = 0x100;
        _verticalMaxVelocity = 0x100;

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
        // Time: immediately before active region starts
        // IRQ0 fires to stop VRAM accessing
        // (not emulated here - we assume there's time for all VRAM writes)

        // Time: active region starts
        // CRTC latches start address
        // (VRAM data is also latched here but we don't write to that in
        // active region anyway so it's fine)
        _data.change(0, -_regs, _regs + 0x4000, &_cgaBytes[0]);
        _output.restart();
        _animated.restart();

        // onScreenHandler
        // EOI
        // Switch IRQ0 handler to offScreenHandler
        // Set count for PIT channel 0 to inactive cycles
        // Start foreground sound
        // Set start address:
        _cgaBytes[_regs + CGAData::registerStartAddressHigh] =
            _startAddress >> 8;
        _cgaBytes[_regs + CGAData::registerStartAddressLow] =
            _startAddress & 0xff;
        // Check keyboard
        if (_leftPressed) {
            if (!_rightPressed) {
                // Speed up leftwards
                _xVelocity -= _horizontalAcceleration;
                if (_xVelocity < -_horizontalMaxVelocity)
                    _xVelocity = -_horizontalMaxVelocity;
            }
            // If both left and right are pressed, maintain horizontal velocity
        }
        else {
            if (_rightPressed) {
                // Speed up rightwards
                _xVelocity += _horizontalAcceleration;
                if (_xVelocity > _horizontalMaxVelocity)
                    _xVelocity = _horizontalMaxVelocity;
            }
            else {
                // Slow down
                if (_xVelocity > 0) {
                    _xVelocity -= _horizontalAcceleration;
                    if (_xVelocity < 0)
                        _xVelocity = 0;
                }
                else {
                    _xVelocity += _horizontalAcceleration;
                    if (_xVelocity > 0)
                        _xVelocity = 0;
                }
            }
        }
        if (_upPressed) {
            if (!_downPressed) {
                // Speed up downwards
                _yVelocity -= _verticalAcceleration;
                if (_yVelocity < -_verticalMaxVelocity)
                    _yVelocity = -_verticalMaxVelocity;
            }
            // If both up and down are pressed, maintain vertical velocity
        }
        else {
            if (_downPressed) {
                // Speed up downwards
                _yVelocity += _verticalAcceleration;
                if (_yVelocity > _verticalMaxVelocity)
                    _yVelocity = _verticalMaxVelocity;
            }
            else {
                // Slow down
                if (_yVelocity > 0) {
                    _yVelocity -= _verticalAcceleration;
                    if (_yVelocity < 0)
                        _yVelocity = 0;
                }
                else {
                    _yVelocity += _verticalAcceleration;
                    if (_yVelocity > 0)
                        _yVelocity = 0;
                }
            }
        }
        // Move
        int xSubTileHighOld = _xSubTile >> 8;
        int ySubTileHighOld = _ySubTile >> 8;
        if (_xVelocity > 0) {
            _xSubTile += _xVelocity;
            if ((_xSubTile >> 8) > _tileWidthCharacters) {
                _xSubTile -= _tileWidthCharacters << 8;
                ++_xTile;
            }
        }
        else {
            _xSubTile += _xVelocity;
            if ((_xSubTile >> 8) < 0) {
                _xSubTile += _tileWidthCharacters << 8;
                --_xTile;
            }
        }
        if (_yVelocity > 0) {
            _ySubTile += _yVelocity;
            if ((_ySubTile >> 8) > _tileHeight) {
                _ySubTile -= _tileHeight << 8;
                ++_yTile;
            }
        }
        else {
            _ySubTile += _yVelocity;
            if ((_ySubTile >> 8) < 0) {
                _ySubTile += _tileHeight << 8;
                --_yTile;
            }
        }
        int deltaX = (_xSubTile >> 8) - xSubTileHighOld;
        int deltaY = (_ySubTile >> 8) - ySubTileHighOld;
        int delta = deltaY*screenColumns + deltaX;
        _startAddress += delta;
        switch (delta) {
            case 0:

        }






        // Time: inactive region starts
        // IRQ0 fires to start VRAM update

        // offScreenHandler
        // EOI
        // Switch IRQ0 handler to onScreenHandler
        // Set count for PIT channel 0 to active cycles
        // Start background sound
        // Start interrupts
        // Switch stack to VRAM update list
        // Copy from buffer to VRAM
        for (auto i : _updateBlocks)
            updateBlock(i);
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
    struct Node
    {
        Byte drawn;
        Byte undrawn;
        Word next;
    };
    struct UpdateBlock
    {
        Word bufferTopLeft;
        Word vramTopLeft;
        Word sourceAdd;
        Word destinationAdd;
        Word columns;
        Word rows;
    };

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
            for (int x = -_tileWidth; x < _screenColumns*2 + _tileWidth; x += _tileWidth) {
                drawTileToBuffer(buffer, _background[map]);
                drawTransparentTiletoBuffer(buffer, _foreground[map]);
                buffer += _tileWidth;
                ++map;
            }
            bufferRow += _bufferStride*_tileHeight;
            mapRow += _mapStride;
        }
    }
    void updateBlock(UpdateBlock b)
    {
        Word s = b.bufferTopLeft;
        Word d = b.vramTopLeft;
        Byte* buffer = &_buffer[0];
        for (int y = 0; y < b.rows; ++y) {
            for (int x = 0; x < b.columns; ++x) {
                *reinterpret_cast<Word*>(_vram + d) =
                    *reinterpret_cast<Word*>(buffer + s);
                s += 2;
                d += 2;
            }
            s += b.sourceAdd;
            d += b.destinationAdd;
        }
    }


    FFTWWisdom<float> _wisdom;
    CGAData _data;
    CGASequencer _sequencer;
    CGAOutput _output;
    AnimatedWindow _animated;
    BitmapWindow _bitmap;
    Array<Byte> _cgaBytes;
    int _regs;

    Byte* _vram;
    Word _startAddress;

    Array<Byte> _background;
    Array<Byte> _foreground;
    Array<Byte> _buffer;
    Array<Byte> _tiles;

    int _tileWidth;
    int _tileWidthCharacters;
    int _tileHeight;
    int _bufferStride;
    int _screenColumns;
    int _screenRows;
    int _mapStride;
    int _horizontalAcceleration;
    int _verticalAcceleration;
    int _horizontalMaxVelocity;
    int _verticalMaxVelocity;

    bool _upPressed;
    bool _downPressed;
    bool _leftPressed;
    bool _rightPressed;
    bool _spacePressed;

    int _xVelocity;
    int _yVelocity;
    int _xSubtile;
    int _ySubtile;
    int _xTile;
    int _yTile;

    AppendableArray<UpdateBlock> _updateBlocks;
};

class Program : public WindowProgram<GameWindow>
{
};
