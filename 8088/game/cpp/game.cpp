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

        // Compute all the static data.
        _tileColumns = 8;
        _tileWidthBytes = _tileColumns << 1;
        _tileRows = 16;
        _bufferStride = 0x100;
        _screenColumns = 80;
        _screenWidthBytes = _screenColumns << 1;
        _screenRows = 100;
        _mapStride = 0x100;
        _horizontalAcceleration = 0x100;
        _verticalAcceleration = 0x100;
        _horizontalMaxVelocity = 0x100;
        _verticalMaxVelocity = 0x100;
        _tilePointers.allocate(0x100);
        Word tilePointer = 0;
        int tileBytes = _tileRows*_tileWidthBytes;
        for (int i = 0; i < 0x100; ++i) {
            _tilePointers[i] = tilePointer;
            tilePointer += tileBytes;
        }
        _tilesPerScreenHorizontally =
            (_screenColumns + 2*_tileColumns - 2) / _tileColumns;
        _tilesPerScreenVertically =
            (_screenRows + 2*_tileRows - 2) / _tileRows;
        _midTileHorizontally = _tilesPerScreenHorizontally/2;
        _midTileVertically = _tilesPerScreenVertically/2;
        _bufferLeft.allocate(_tilesPerScreenVertically);
        _mapLeft.allocate(_tilesPerScreenVertically);
        _bufferTop.allocate(_tilesPerScreenHorizontally + 2);
        _bufferRight.allocate(_tilesPerScreenVertically);
        _mapRight.allocate(_tilesPerScreenVertically);
        _bufferBottom.allocate(_tilesPerScreenHorizontally + 2);
        _mapBottom.allocate(_tilesPerScreenHorizontally + 2);
        int bufferTileStride = _bufferStride*_tileRows;
        int bufferLeft = bufferTileStride;
        int mapLeft = _mapStride;
        int bufferRight = bufferLeft +
            (_tilesPerScreenHorizontally + 1)*_tileColumns;
        int mapRight = mapLeft + _tilesPerScreenHorizontally + 1;
        for (int i = 0; i < _tilesPerScreenVertically; ++i) {
            _bufferLeft[i] = bufferLeft;
            _mapLeft[i] = mapLeft;
            _bufferRight[i] = bufferRight;
            _mapRight[i] = mapRight;
            bufferLeft += bufferTileStride;
            bufferRight += bufferTileStride;
            mapLeft += _mapStride;
            mapRight += _mapStride;
        }
        int bufferTop = 0;
        int bufferBottom = (_tilesPerScreenVertically + 1)*bufferTileStride;
        int mapBottom = (_tilesPerScreenVertically + 1)*_mapStride;
        for (int i = 0; i < _tilesPerScreenHorizontally + 2; ++i) {
            _bufferTop[i] = bufferTop;
            _bufferBottom[i] = bufferBottom;
            _mapBottom[i] = mapBottom;
            bufferTop += _tileColumns;
            bufferBottom += _tileColumns;
            ++mapBottom;
        }
        _topLeftBuffer = _tileColumns + bufferTileStride;
        _topLeftMap = 1 + _mapStride;
        _topRightBuffer = _tileColumns*_tilesPerScreenHorizontally +
            bufferTileStride;
        _topRightMap = _tilesPerScreenHorizontally + _mapStride;
        _bottomLeftBuffer = _tileColumns +
            bufferTileStride*_tilesPerScreenVertically;
        _bottomLeftMap = 1 + _mapStride*_tilesPerScreenVertically;
        _bottomRightBuffer = _tileColumns*_tilesPerScreenHorizontally +
            bufferTileStride*_tilesPerScreenVertically;
        _bottomRightMap = _tilesPerScreenHorizontally +
            _mapStride*_tilesPerScreenVertically;

        _transitionCountLeft.allocate(_tileColumns);
        _transitionCountTop.allocate(_tileRows);
        _transitionCountRight.allocate(_tileColumns);
        _transitionCountBottom.allocate(_tileRows);
        for (int i = 0; i < _tileColumns; ++i) {
            _transitionCountLeft[i] = (_tileColumns - i)*
                (_tilesPerScreenVertically + 1)/_tileColumns - 1;
            _transitionCountRight[i] =
                (1 + i)*(_tilesPerScreenVertically + 1)/_tileColumns - 1;
        }
        for (int i = 0; i < _tileRows; ++i) {
            _transitionCountTop[i] = (_tileRows - i)*
                (_tilesPerScreenHorizontally + 1)/_tileRows - 1;
            _transitionCountBottom[i] =
                (1 + i)*(_tilesPerScreenHorizontally + 1)/_tileRows - 1;
        }


        _background.allocate(0x10000);
        _foreground.allocate(0x10000);
        _buffer.allocate(0x10000);
        _tiles.allocate(0x10000);
        for (int i = 0; i < 0x10000; ++i) {
            _background[i] = rand() & 0xff;
            _foreground[i] = rand() & 0xff;
            _tiles[i] = rand() & 0xff;
        }

        // Draw initial screen
        _bufferTL = 0;
        _mapTL = 0;
        int bufferRow = -_tileRows*_bufferStride - _tileWidthBytes;
        int mapRow = -_mapStride - 1;
        for (int y = -1; y < _tilesPerScreenVertically + 1; ++y) {
            int buffer = bufferRow;
            int map = mapRow;
            for (int x = -1; x < _tilesPerScreenHorizontally + 1; ++x) {
                drawTile(buffer, map);
                buffer += _tileWidthBytes;
                ++map;
            }
            bufferRow += _bufferStride*_tileRows;
            mapRow += _mapStride;
        }
        _bufferTopLeft = _tileColumns + _tileRows*_bufferStride;
        _vramTopLeft = 0;
        _startAddress = 0;
        _leftStart = 0;
        _leftEnd = _tilesPerScreenVertically;
        _topStart = 0;
        _topEnd = _tilesPerScreenHorizontally;
        _rightStart = 0;
        _rightEnd = _tilesPerScreenVertically;
        _bottomStart = 0;
        _bottomEnd = _tilesPerScreenHorizontally;
        _xSubTile = 0;
        _ySubTile = 0;
        _xVelocity = 0;
        _yVelocity = 0;

        addUpdateBlock(0, 0, _screenColumns, _screenRows);
        for (auto i : _updateBlocks)
            updateBlock(i);
        _updateBlocks.clear();

        _leftPressed = false;
        _upPressed = false;
        _rightPressed = false;
        _downPressed = false;
        _spacePressed = false;
    }
    ~GameWindow() { _output.join(); }
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
        _vram[CGAData::registerStartAddressHigh] = _startAddress >> 8;
        _vram[CGAData::registerStartAddressLow] = _startAddress & 0xff;
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
        int tileBoundary = 0;
        if (_xVelocity > 0) {
            _xSubTile += _xVelocity;
            if ((_xSubTile >> 8) > _tileColumns) {
                _xSubTile -= _tileColumns << 8;
                ++_mapTL;
                _leftStart = 0;
                _leftEnd = _tilesPerScreenVertically;
                _rightStart = _midTileVertically;
                _rightEnd = _midTileVertically;
                if (_topStart > 0)
                    --_topStart;
                if (_topEnd > 0)
                    --_topEnd;
                if (_topStart == _topEnd) {
                    _topStart = _tilesPerScreenHorizontally;
                    _topEnd = _tilesPerScreenHorizontally;
                }
                if (_bottomStart > 0)
                    --_bottomStart;
                if (_bottomEnd > 0)
                    --_bottomEnd;
                if (_bottomStart == _bottomEnd) {
                    _bottomStart = _tilesPerScreenHorizontally;
                    _bottomEnd = _tilesPerScreenHorizontally;
                }
                tileBoundary = 1;
            }
        }
        else {
            _xSubTile += _xVelocity;
            if ((_xSubTile >> 8) < 0) {
                _xSubTile += _tileColumns << 8;
                --_mapTL;
                _leftStart = _midTileVertically;
                _leftEnd = _midTileVertically;
                _rightStart = 0;
                _rightEnd = _tilesPerScreenVertically;
                if (_topStart < _tilesPerScreenHorizontally)
                    ++_topStart;
                if (_topEnd < _tilesPerScreenHorizontally)
                    ++_topEnd;
                if (_topStart == _topEnd) {
                    _topStart = 0;
                    _topEnd = 0;
                }
                if (_bottomStart < _tilesPerScreenHorizontally)
                    ++_bottomStart;
                if (_bottomEnd < _tilesPerScreenHorizontally)
                    ++_bottomEnd;
                if (_bottomStart == _bottomEnd) {
                    _bottomStart = 0;
                    _bottomEnd = 0;
                }
                tileBoundary = -1;
            }
        }
        if (_yVelocity > 0) {
            _ySubTile += _yVelocity;
            if ((_ySubTile >> 8) > _tileRows) {
                _ySubTile -= _tileRows << 8;
                _mapTL += _mapStride;
                _topStart = 0;
                _topEnd = _tilesPerScreenHorizontally;
                _bottomStart = _midTileHorizontally;
                _bottomEnd = _midTileHorizontally;
                if (_leftStart > 0)
                    --_leftStart;
                if (_leftEnd > 0)
                    --_leftEnd;
                if (_leftStart == _leftEnd) {
                    _leftStart = _tilesPerScreenVertically;
                    _leftEnd = _tilesPerScreenVertically;
                }
                if (_rightStart > 0)
                    --_rightStart;
                if (_rightEnd > 0)
                    --_rightEnd;
                if (_rightStart == _rightEnd) {
                    _rightStart = _tilesPerScreenVertically;
                    _rightEnd = _tilesPerScreenVertically;
                }
                if (tileBoundary != 0) {
                    if (tileBoundary < 0)
                        drawTile(_bottomLeftBuffer, _bottomLeftMap);
                    else
                        drawTile(_bottomRightBuffer, _bottomRightMap);
                }
            }
        }
        else {
            _ySubTile += _yVelocity;
            if ((_ySubTile >> 8) < 0) {
                _ySubTile += _tileRows << 8;
                _mapTL -= _mapStride;
                _topStart = _midTileHorizontally;
                _topEnd = _midTileHorizontally;
                _bottomStart = 0;
                _bottomEnd = _tilesPerScreenHorizontally;
                if (_leftStart < _tilesPerScreenVertically)
                    ++_leftStart;
                if (_leftEnd < _tilesPerScreenVertically)
                    ++_leftEnd;
                if (_leftStart == _leftEnd) {
                    _leftStart = 0;
                    _leftEnd = 0;
                }
                if (_rightStart < _tilesPerScreenVertically)
                    ++_rightStart;
                if (_rightEnd < _tilesPerScreenVertically)
                    ++_rightEnd;
                if (_rightStart == _rightEnd) {
                    _rightStart = 0;
                    _rightEnd = 0;
                }
                if (tileBoundary != 0) {
                    if (tileBoundary < 0)
                        drawTile(_topLeftBuffer, _topLeftMap);
                    else
                        drawTile(_topRightBuffer, _topRightMap);
                }
            }
        }
        int deltaX = (_xSubTile >> 8) - xSubTileHighOld;
        int deltaY = (_ySubTile >> 8) - ySubTileHighOld;
        if (deltaX < 0) {
            if (deltaY < 0) {
                // Move up and left
                _startAddress -= _screenColumns + 1;
                _vramTopLeft -= _screenWidthBytes + 2;
                _bufferTopLeft -= _bufferStride + 2;
                addUpdateBlock(0, 0, _screenColumns, 1);
                addUpdateBlock(0, 1, 1, _screenRows - 1);
                leftTiles();
                topTiles();
            }
            else {
                if (deltaY > 0) {
                    // Move down and left
                    _startAddress += _screenColumns - 1;
                    _vramTopLeft += _screenWidthBytes - 2;
                    _bufferTopLeft += _bufferStride - 2;
                    addUpdateBlock(_screenRows - 1, 0, _screenColumns, 1);
                    addUpdateBlock(0, 0, 1, _screenRows - 1);
                    leftTiles();
                    bottomTiles();
                }
                else {
                    // Move left
                    --_startAddress;
                    _vramTopLeft -= 2;
                    _bufferTopLeft -= 2;
                    addUpdateBlock(0, 0, 1, _screenRows);
                    leftTiles();
                    topTiles();
                    bottomTiles();
                }
            }
        }
        else {
            if (deltaX > 0) {
                if (deltaY < 0) {
                    // Move up and right
                    _startAddress -= _screenColumns - 1;
                    _vramTopLeft -= _screenWidthBytes - 2;
                    _bufferTopLeft -= _bufferStride - 2;
                    addUpdateBlock(0, 0, _screenColumns, 1);
                    addUpdateBlock(_screenColumns - 1, 1, 1, _screenRows - 1);
                    topTiles();
                    rightTiles();
                }
                else {
                    if (deltaY > 0) {
                        // Move down and right
                        _startAddress += _screenColumns + 1;
                        _vramTopLeft += _screenWidthBytes + 2;
                        _bufferTopLeft += _bufferStride + 2;
                        addUpdateBlock(0, _screenRows - 1, _screenColumns, 1);
                        addUpdateBlock(_screenColumns - 1, 0, 1,
                            _screenRows - 1);
                        bottomTiles();
                        rightTiles();
                    }
                    else {
                        // Move right
                        ++_startAddress;
                        _vramTopLeft += 2;
                        _bufferTopLeft += 2;
                        addUpdateBlock(_screenColumns - 1, 0, 1, _screenRows);
                        rightTiles();
                        topTiles();
                        bottomTiles();
                    }
                }
            }
            else {
                if (deltaY < 0) {
                    // Move up
                    _startAddress -= _screenColumns;
                    _vramTopLeft -= _screenWidthBytes;
                    _bufferTopLeft -= _bufferStride;
                    addUpdateBlock(0, 0, _screenColumns, 1);
                    topTiles();
                }
                else {
                    if (deltaY > 0) {
                        // Move down
                        _startAddress += _screenColumns;
                        _vramTopLeft += _screenWidthBytes;
                        _bufferTopLeft += _bufferStride;
                        addUpdateBlock(0, _screenRows - 1, _screenColumns, 1);
                        bottomTiles();
                    }
                    else {
                        // No change
                    }
                }
            }
        }

        // Move other entities
        // For each moving entity:
        //   Erase sprite at old position
        //   Capture background for erasing
        //   Draw sprite at new position
        //   Add update
        // Collision detection
        // Update for modified tiles
        // Other game logic



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
        _updateBlocks.clear();
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
    struct UpdateBlock
    {
        Word bufferTopLeft;
        Word vramTopLeft;
        Word sourceAdd;
        Word destinationAdd;
        Word columns;
        Word rows;
    };

    void drawTile(Word tl, Word map)
    {
        tl += _bufferTL;
        map += _mapTL;
        const Byte* f = &_tiles[_tilePointers[_foreground[map]]];
        const Byte* b = &_tiles[_tilePointers[_background[map]]];
        int rowIncrement = _bufferStride - _tileWidthBytes;
        Byte* buffer = &_buffer[0];
        for (int y = 0; y < _tileRows; ++y) {
            for (int x = 0; x < _tileColumns; ++x) {
                Word c = *reinterpret_cast<const Word*>(f);
                Word* p = reinterpret_cast<Word*>(buffer + tl);
                if (c != 0xffff)
                    *p = c;
                else
                    *p = *reinterpret_cast<const Word*>(b);
                f += 2;
                b += 2;
                tl += 2;
            }
            tl += rowIncrement;
        }
    }

    void updateBlock(UpdateBlock b)
    {
        Word s = b.bufferTopLeft;
        Word d = b.vramTopLeft;
        Byte* buffer = &_buffer[0];
        for (int y = 0; y < b.rows; ++y) {
            for (int x = 0; x < b.columns; ++x) {
                *reinterpret_cast<Word*>(_vram + (d & 0x3fff)) =
                    *reinterpret_cast<Word*>(buffer + s);
                s += 2;
                d += 2;
            }
            s += b.sourceAdd;
            d += b.destinationAdd;
        }
    }
    void addUpdateBlock(int left, int top, int columns, int rows)
    {
        UpdateBlock b;
        b.bufferTopLeft = left*2 + top*_bufferStride + _bufferTopLeft;
        b.vramTopLeft = left*2 + top*_screenWidthBytes + _vramTopLeft;
        b.sourceAdd = _bufferStride - columns*2;
        b.destinationAdd = _screenWidthBytes - columns*2;
        b.columns = columns;
        b.rows = rows;
        _updateBlocks.append(b);
    }
    void leftTiles()
    {
        while (_leftEnd - _leftStart < _transitionCountLeft[_xSubTile >> 8]) {
            int y;
            if (_yVelocity > 0) {
                if (_leftEnd < _tilesPerScreenVertically) {
                    y = _leftEnd;
                    ++_leftEnd;
                }
                else {
                    y = _leftStart - 1;
                    --_leftStart;
                }
            }
            else {
                if (_leftStart > 0) {
                    y = _leftStart - 1;
                    --_leftStart;
                }
                else {
                    y = _leftEnd;
                    ++_leftEnd;
                }
            }
            drawTile(_bufferLeft[y], _mapLeft[y]);
        }
    }
    void topTiles()
    {
        while (_topEnd - _topStart < _transitionCountTop[_ySubTile >> 8]) {
            int x;
            if (_xVelocity > 0) {
                if (_topEnd < _tilesPerScreenHorizontally) {
                    x = _topEnd;
                    ++_topEnd;
                }
                else {
                    x = _topStart - 1;
                    --_topStart;
                }
            }
            else {
                if (_topStart > 0) {
                    x = _topStart - 1;
                    --_topStart;
                }
                else {
                    x = _topEnd;
                    ++_topEnd;
                }
            }
            drawTile(_bufferTop[x], x);
        }
    }
    void rightTiles()
    {
        while (_rightEnd - _rightStart <
            _transitionCountRight[_xSubTile >> 8]) {
            int y;
            if (_yVelocity > 0) {
                if (_rightEnd < _tilesPerScreenVertically) {
                    y = _rightEnd;
                    ++_rightEnd;
                }
                else {
                    y = _rightStart - 1;
                    --_rightStart;
                }
            }
            else {
                if (_rightStart > 0) {
                    y = _rightStart - 1;
                    --_rightStart;
                }
                else {
                    y = _rightEnd;
                    ++_rightEnd;
                }
            }
            drawTile(_bufferRight[y], _mapRight[y]);
        }
    }
    void bottomTiles()
    {
        while (_bottomEnd - _bottomStart <
            _transitionCountBottom[_ySubTile >> 8]) {
            int x;
            if (_xVelocity > 0) {
                if (_bottomEnd < _tilesPerScreenHorizontally) {
                    x = _bottomEnd;
                    ++_bottomEnd;
                }
                else {
                    x = _bottomStart - 1;
                    --_bottomStart;
                }
            }
            else {
                if (_bottomStart > 0) {
                    x = _bottomStart - 1;
                    --_bottomStart;
                }
                else {
                    x = _bottomEnd;
                    ++_bottomEnd;
                }
            }
            drawTile(_bufferBottom[x], _mapBottom[x]);
        }
    }


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

    Array<Byte> _background;
    Array<Byte> _foreground;
    Array<Byte> _buffer;
    Array<Byte> _tiles;

    int _tileColumns;
    int _tileWidthBytes;
    int _tileRows;
    int _bufferStride;
    int _screenColumns;
    int _screenWidthBytes;
    int _screenRows;
    int _mapStride;
    int _tilesPerScreenHorizontally;
    int _tilesPerScreenVertically;
    int _midTileHorizontally;
    int _midTileVertically;
    int _horizontalAcceleration;
    int _verticalAcceleration;
    int _horizontalMaxVelocity;
    int _verticalMaxVelocity;
    Array<Byte> _transitionCountLeft;
    Array<Byte> _transitionCountTop;
    Array<Byte> _transitionCountRight;
    Array<Byte> _transitionCountBottom;
    Array<Word> _bufferLeft;
    Array<Word> _mapLeft;
    Array<Word> _bufferTop;
    Array<Word> _bufferRight;
    Array<Word> _mapRight;
    Array<Word> _bufferBottom;
    Array<Word> _mapBottom;
    Array<Word> _tilePointers;
    Word _topLeftBuffer;
    Word _topLeftMap;
    Word _topRightBuffer;
    Word _topRightMap;
    Word _bottomLeftBuffer;
    Word _bottomLeftMap;
    Word _bottomRightBuffer;
    Word _bottomRightMap;

    bool _upPressed;
    bool _downPressed;
    bool _leftPressed;
    bool _rightPressed;
    bool _spacePressed;

    int _xVelocity;
    int _yVelocity;
    int _xSubTile;
    int _ySubTile;
    Word _startAddress;   // CRTC character number for TL of screen
    Word _vramTopLeft;    // Position in VRAM corresponding to TL of screen
    Word _bufferTopLeft;  // Position in buffer corresponding to TL of screen
    Word _bufferTL; // Position in buffer corresponding to TL of tilescreen
    Word _mapTL;    // Position in map corresponding to TL of tilescreen
    int _leftStart;
    int _leftEnd;
    int _topStart;
    int _topEnd;
    int _rightStart;
    int _rightEnd;
    int _bottomStart;
    int _bottomEnd;

    AppendableArray<UpdateBlock> _updateBlocks;
};

class Program : public WindowProgram<GameWindow>
{
};
