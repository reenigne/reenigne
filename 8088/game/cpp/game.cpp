#include "alfe/main.h"
#include "alfe/cga.h"

enum CollisionHandler
{
    collisionHandlerNone = 0,
    collisionHandlerCoin,
    collisionHandlerPlatform
};

enum Direction
{
    directionUpLeft,
    directionUp,
    directionUpRight,
    directionLeft,
    directionStopped,
    directionRight,
    directionDownLeft,
    directionDown,
    directionDownRight
};

#include "u6conv/collisionData.h"

class GameWindow : public RootWindow
{
public:
    GameWindow()
      : _wisdom(File("wisdom")), _output(&_data, &_sequencer, &_bitmap)
    {
        _sequencer.setROM(File("5788005.u33"));

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
        _bufferTileStride = _bufferStride*_tileRows;
        _screenColumns = 80;
        _screenWidthBytes = _screenColumns << 1;
        _screenRows = 100;
        _mapStride = 0x100;
        _horizontalAcceleration = 0x10;
        _verticalAcceleration = 0x10;
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
        _bufferTop.allocate(_tilesPerScreenHorizontally);
        _bufferRight.allocate(_tilesPerScreenVertically);
        _mapRight.allocate(_tilesPerScreenVertically);
        _bufferBottom.allocate(_tilesPerScreenHorizontally);
        int bufferLeft = _bufferTileStride;
        int mapLeft = _mapStride;
        int bufferRight = bufferLeft +
            (_tilesPerScreenHorizontally + 1)*_tileWidthBytes;
        int mapRight = mapLeft + _tilesPerScreenHorizontally + 1;
        for (int i = 0; i < _tilesPerScreenVertically; ++i) {
            _bufferLeft[i] = bufferLeft;
            _mapLeft[i] = mapLeft;
            _bufferRight[i] = bufferRight;
            _mapRight[i] = mapRight;
            bufferLeft += _bufferTileStride;
            bufferRight += _bufferTileStride;
            mapLeft += _mapStride;
            mapRight += _mapStride;
        }
        int bufferTop = _tileWidthBytes;
        int bufferBottom = _tileWidthBytes +
            (_tilesPerScreenVertically + 1)*_bufferTileStride;
        _mapBottom = 1 + (_tilesPerScreenVertically + 1)*_mapStride;
        for (int i = 0; i < _tilesPerScreenHorizontally; ++i) {
            _bufferTop[i] = bufferTop;
            _bufferBottom[i] = bufferBottom;
            bufferTop += _tileWidthBytes;
            bufferBottom += _tileWidthBytes;
        }
        _topLeftBuffer = _tileWidthBytes + _bufferTileStride;
        _topLeftMap = 1 + _mapStride;
        _topRightBuffer = _tileWidthBytes*_tilesPerScreenHorizontally +
            _bufferTileStride;
        _topRightMap = _tilesPerScreenHorizontally + _mapStride;
        _bottomLeftBuffer = _tileWidthBytes +
            _bufferTileStride*_tilesPerScreenVertically;
        _bottomLeftMap = 1 + _mapStride*_tilesPerScreenVertically;
        _bottomRightBuffer = _tileWidthBytes*_tilesPerScreenHorizontally +
            _bufferTileStride*_tilesPerScreenVertically;
        _bottomRightMap = _tilesPerScreenHorizontally +
            _mapStride*_tilesPerScreenVertically;

        _transitionCountLeft.allocate(_tileColumns);
        _transitionCountTop.allocate(_tileRows);
        _transitionCountRight.allocate(_tileColumns);
        _transitionCountBottom.allocate(_tileRows);
        int fudge = 1;
        for (int i = 0; i < _tileColumns; ++i) {
            _transitionCountLeft[i] = max(0, (_tileColumns - i)*
                (_tilesPerScreenVertically + fudge)/_tileColumns - fudge);
            _transitionCountRight[i] = max(0, 
                (1 + i)*(_tilesPerScreenVertically + fudge)/_tileColumns - fudge);
        }
        for (int i = 0; i < _tileRows; ++i) {
            _transitionCountTop[i] = max(0, (_tileRows - i)*
                (_tilesPerScreenHorizontally + fudge)/_tileRows - fudge);
            _transitionCountBottom[i] = max(0, 
                (1 + i)*(_tilesPerScreenHorizontally + fudge)/_tileRows - fudge);
        }

        String world = File("world.dat").contents();
        _background.allocate(0x10000);
        _foreground.allocate(0x10000);
        _buffer.allocate(0x10000);
        _tiles.allocate(0x10000);
        for (int i = 0; i < 0x10000; ++i) {
            _background[i] = world[i];
            _foreground[i] = world[i + 0x20000];
            _tiles[i] = world[i + 0x10000];
        }

        // Draw initial screen
        _bufferTL = 0;
        _mapTL = 0x8080;
        int bufferRow = 0;
        int mapRow = 0;
        for (int y = 0; y < _tilesPerScreenVertically + 2; ++y) {
            int buffer = bufferRow;
            int map = mapRow;
            for (int x = 0; x < _tilesPerScreenHorizontally + 2; ++x) {
                drawTile(buffer, map);
                buffer += _tileWidthBytes;
                ++map;
            }
            bufferRow += _bufferTileStride;
            mapRow += _mapStride;
        }
        _bufferTopLeft = _tileWidthBytes + _bufferTileStride;
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

        _leftPressed = false;
        _upPressed = false;
        _rightPressed = false;
        _downPressed = false;
        _spacePressed = false;

        _xPlayer = (_screenColumns - _tileColumns)/2;
        _yPlayer = (_screenRows - _tileRows)/2;
        _playerTopLeft = _yPlayer*_bufferStride + _xPlayer*2;
        _underPlayer.allocate(_tileWidthBytes*_tileRows);

        _leftCollisionTable.allocate(8);
        _rightCollisionTable.allocate(8);
        for (int i = 0; i < 8; ++i) {
            _leftCollisionTable[i] = (0xff << ((i + _xPlayer) & 7)) & 0xff;
            _rightCollisionTable[i] = (~(0xff << ((i + _xPlayer) & 7))) & 0xff;
        }


        drawPlayer();

        addUpdateBlock(0, 0, _screenColumns, _screenRows);
        for (auto i : _updateBlocks)
            updateBlock(i);
        _updateBlocks.clear();

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
        //if (_upPressed) {
        //    if (!_downPressed) {
        //        // Speed up downwards
        //        _yVelocity -= _verticalAcceleration;
        //        if (_yVelocity < -_verticalMaxVelocity)
        //            _yVelocity = -_verticalMaxVelocity;
        //    }
        //    // If both up and down are pressed, maintain vertical velocity
        //}
        //else {
        //    if (_downPressed) {
        //        // Speed up downwards
        //        _yVelocity += _verticalAcceleration;
        //        if (_yVelocity > _verticalMaxVelocity)
        //            _yVelocity = _verticalMaxVelocity;
        //    }
        //    else {
        //        // Slow down
        //        if (_yVelocity > 0) {
        //            _yVelocity -= _verticalAcceleration;
        //            if (_yVelocity < 0)
        //                _yVelocity = 0;
        //        }
        //        else {
        //            _yVelocity += _verticalAcceleration;
        //            if (_yVelocity > 0)
        //                _yVelocity = 0;
        //        }
        //    }
        //}

        _tilesDrawn = 0;


        _yVelocity = min(_yVelocity + 0x4, 0x100);
        if (_landed && _spacePressed)
            _yVelocity = -0x100;

        _xSubTileHighOld = _xSubTile >> 8;
        _ySubTileHighOld = _ySubTile >> 8;
        _oldMapTL = _mapTL;
        _xSubTile += _xVelocity;
        _ySubTile += _yVelocity;

        if ((_ySubTile >> 8) != _ySubTileHighOld)
            _landed = false;
        do {
            normalize();
            _direction = calculateDirection();

            if (checkPlayerTileCollision(0, 0))
                continue;
            if (checkPlayerTileCollision(1, 0))
                continue;
            if (checkPlayerTileCollision(0, 1))
                continue;
            if (checkPlayerTileCollision(1, 1))
                continue;
            break;
        } while (true);

        _tileDirection = calculateTileDirection();

        if (isRight(_tileDirection)) {
            _bufferTL += _tileWidthBytes;
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
        }
        else {
            if (isLeft(_tileDirection)) {
                _bufferTL -= _tileWidthBytes;
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
            }
        }
        if (isDown(_tileDirection)) {
            _bufferTL += _bufferTileStride;
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
        }
        else {
            if (isUp(_tileDirection)) {
                _bufferTL -= _bufferTileStride;
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
            }
        }

        // When diagonally scrolling across both horizontal and vertical tile
        // boundaries, we bring an undrawn tile into the drawn area, so need
        // to draw it.
        int tilesDrawn = 0;
        switch (_tileDirection) {
        case directionUpLeft:
            drawTile(_topLeftBuffer, _topLeftMap);
            ++tilesDrawn;
            break;
        case directionUpRight:
            drawTile(_topRightBuffer, _topRightMap);
            ++tilesDrawn;
            break;
        case directionDownLeft:
            drawTile(_bottomLeftBuffer, _bottomLeftMap);
            ++tilesDrawn;
            break;
        case directionDownRight:
            drawTile(_bottomRightBuffer, _bottomRightMap);
            ++tilesDrawn;
            break;
        }
        if (_direction != directionStopped)
            _redrawPlayer = true;

        if (_redrawPlayer)
            restoreTile(_playerTopLeft, &_underPlayer[0]);

        // Do the actual scrolling
        switch (_direction) {
        case directionUpLeft:
            _startAddress -= _screenColumns + 1;
            _vramTopLeft -= _screenWidthBytes + 2;
            _bufferTopLeft -= _bufferStride + 2;
            addUpdateBlock(0, 1, 1, _screenRows - 1);
            addUpdateBlock(_xPlayer, _yPlayer, _tileColumns + 1, _tileRows + 1);
            break;
        case directionUp:
            _startAddress -= _screenColumns;
            _vramTopLeft -= _screenWidthBytes;
            _bufferTopLeft -= _bufferStride;
            addUpdateBlock(_xPlayer, _yPlayer, _tileColumns, _tileRows + 1);
            break;
        case directionUpRight:
            _startAddress -= _screenColumns - 1;
            _vramTopLeft -= _screenWidthBytes - 2;
            _bufferTopLeft -= _bufferStride - 2;
            addUpdateBlock(_screenColumns - 1, 1, 1, _screenRows - 1);
            addUpdateBlock(_xPlayer - 1, _yPlayer, _tileColumns + 1, _tileRows + 1);
            break;
        case directionLeft:
            --_startAddress;
            _vramTopLeft -= 2;
            _bufferTopLeft -= 2;
            addUpdateBlock(0, 0, 1, _screenRows);
            addUpdateBlock(_xPlayer, _yPlayer, _tileColumns + 1, _tileRows);
            break;
        case directionRight:
            ++_startAddress;
            _vramTopLeft += 2;
            _bufferTopLeft += 2;
            addUpdateBlock(_screenColumns - 1, 0, 1, _screenRows);
            addUpdateBlock(_xPlayer - 1, _yPlayer, _tileColumns + 1, _tileRows);
            break;
        case directionDownLeft:
            _startAddress += _screenColumns - 1;
            _vramTopLeft += _screenWidthBytes - 2;
            _bufferTopLeft += _bufferStride - 2;
            addUpdateBlock(0, 0, 1, _screenRows - 1);
            addUpdateBlock(_xPlayer, _yPlayer - 1, _tileColumns + 1, _tileRows + 1);
            break;
        case directionDown:
            _startAddress += _screenColumns;
            _vramTopLeft += _screenWidthBytes;
            _bufferTopLeft += _bufferStride;
            addUpdateBlock(_xPlayer, _yPlayer - 1, _tileColumns, _tileRows + 1);
            break;
        case directionDownRight:
            _startAddress += _screenColumns + 1;
            _vramTopLeft += _screenWidthBytes + 2;
            _bufferTopLeft += _bufferStride + 2;
            addUpdateBlock(_screenColumns - 1, 0, 1, _screenRows - 1);
            addUpdateBlock(_xPlayer - 1, _yPlayer - 1, _tileColumns + 1, _tileRows + 1);
            break;
        }
        if (isUp(_direction))
            addUpdateBlock(0, 0, _screenColumns, 1);
        else {
            if (isDown(_direction))
                addUpdateBlock(0, _screenRows - 1, _screenColumns, 1);
        }

        for (auto i : _tileModifications) {
            int m = i.mapLocation;
            int rm = m - _mapTL;
            int xMap = rm % _mapStride;
            int yMap = rm / _mapStride;
            int b = xMap*_tileWidthBytes + yMap*_bufferTileStride;
            drawTile(b, rm);
            int s = b + _bufferTL - _bufferTopLeft;
            addUpdateBlock((s % _bufferStride) >> 1, s / _bufferStride, _tileColumns, _tileRows);
        }
        _tileModifications.clear();

        if (_redrawPlayer)
            drawPlayer();

        // Restore tile invariants

        if (!isRight(_direction)) {
            while (_leftEnd - _leftStart < _transitionCountLeft[_xSubTile >> 8] /*&& tilesDrawn < 3*/) {
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
                ++tilesDrawn;
            }
        }
        if (!isLeft(_direction)) {
            while (_rightEnd - _rightStart <
                _transitionCountRight[_xSubTile >> 8] /*&& tilesDrawn < 3*/) {
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
                ++tilesDrawn;
            }
        }
        if (!isDown(_direction)) {
            while (_topEnd - _topStart < _transitionCountTop[_ySubTile >> 8] /*&& tilesDrawn < 3*/) {
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
                drawTile(_bufferTop[x], x + 1);
                ++tilesDrawn;
            }
        }
        if (!isUp(_direction)) {
            while (_bottomEnd - _bottomStart <
                _transitionCountBottom[_ySubTile >> 8] /*&& tilesDrawn < 3*/) {
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
                drawTile(_bufferBottom[x], _mapBottom + x);
                ++tilesDrawn;
            }
        }

        // Set start address:
        _vram[CGAData::registerStartAddressHigh] = _startAddress >> 8;
        _vram[CGAData::registerStartAddressLow] = _startAddress & 0xff;

        //printf("ST %i,%i ", _xSubTile >> 8, _ySubTile >> 8);
        //printf("tl-TL %04x\n", _bufferTopLeft-_bufferTL);
        //printf("SA %04x ", _startAddress);
        //if (_vramTopLeft != ((_startAddress << 1) & 0xffff))
        //    printf("_vramTopLeft incorrect!\n");
        ////printf("vramTopLeft = %04x  ", _vramTopLeft);
        //printf("tl %04x ", _bufferTopLeft);
        //printf("TL %04x ", _bufferTL);
        //printf("m %04x ", _mapTL);
        //printf("l %i-%i " , _leftStart, _leftEnd);
        //printf("t %i-%i ", _topStart, _topEnd);
        //printf("r %i-%i ", _rightStart, _rightEnd);
        //printf("b %i-%i\n",_bottomStart, _bottomEnd);

        //printf("%i  ",_tilesDrawn);
        if (_tilesDrawn >= 4)
            printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");

        //LARGE_INTEGER time;
        //QueryPerformanceCounter(&time);
        //time.QuadPart -= _startTime.QuadPart;
        //LARGE_INTEGER frequency;
        //QueryPerformanceFrequency(&frequency);
        //printf("%lf us\n",time.QuadPart*1000000.0/frequency.QuadPart);
        //QueryPerformanceCounter(&_startTime);

        for (int i = 0; i < 0x10000; i += 2) {
            Word p = i;
            p -= _bufferTL;
            int x = (p % _bufferStride) / _tileWidthBytes - 1;
            int y = (p / _bufferStride) / _tileRows - 1;
            if (x >= 0 && y >= 0 && x < _tilesPerScreenHorizontally && y < _tilesPerScreenVertically)
                continue;
            if (x == -1 && y >= _leftStart && y < _leftEnd)
                continue;
            if (x == _tilesPerScreenHorizontally && y >= _rightStart && y < _rightEnd)
                continue;
            if (y == -1 && x >= _topStart && x < _topEnd)
                continue;
            if (y == _tilesPerScreenVertically && x >= _bottomStart && x < _bottomEnd)
                continue;
            *reinterpret_cast<Word*>(&_buffer[i]) = 0xdddd;
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
    struct TileModification
    {
        Word mapLocation;
        Byte oldTile;
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
        ++_tilesDrawn;
    }
    // tl here is relative to top-left of screen
    void drawTransparentTile(Word tl, int tile)
    {
        tl += _bufferTopLeft;
        const Byte* f = &_tiles[_tilePointers[tile]];
        int rowIncrement = _bufferStride - _tileWidthBytes;
        Byte* buffer = &_buffer[0];
        for (int y = 0; y < _tileRows; ++y) {
            for (int x = 0; x < _tileColumns; ++x) {
                Word c = *reinterpret_cast<const Word*>(f);
                if (c != 0xffff)
                    *reinterpret_cast<Word*>(buffer + tl) = c;
                f += 2;
                tl += 2;
            }
            tl += rowIncrement;
        }
    }
    void saveTile(Word tl, Byte* under)
    {
        tl += _bufferTopLeft;
        int rowIncrement = _bufferStride - _tileWidthBytes;
        const Byte* buffer = &_buffer[0];
        for (int y = 0; y < _tileRows; ++y) {
            for (int x = 0; x < _tileColumns; ++x) {
                *reinterpret_cast<Word*>(under) =
                    *reinterpret_cast<const Word*>(buffer + tl);
                under += 2;
                tl += 2;
            }
            tl += rowIncrement;
        }
    }
    void restoreTile(Word tl, const Byte* under)
    {
        tl += _bufferTopLeft;
        int rowIncrement = _bufferStride - _tileWidthBytes;
        Byte* buffer = &_buffer[0];
        for (int y = 0; y < _tileRows; ++y) {
            for (int x = 0; x < _tileColumns; ++x) {
                *reinterpret_cast<Word*>(buffer + tl) =
                    *reinterpret_cast<const Word*>(under);
                under += 2;
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
                Word c = *reinterpret_cast<Word*>(buffer + s);
                if (c == 0xdddd)
                    printf("Error!!!!\n");
                *reinterpret_cast<Word*>(_vram + (d & 0x3fff)) = c;
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
    void addTileModification(Word mapLocation, Byte oldTile)
    {
        TileModification m;
        m.mapLocation = mapLocation;
        m.oldTile = oldTile;
        _tileModifications.append(m);
    }
    void drawPlayer()
    {
        saveTile(_playerTopLeft, &_underPlayer[0]);
        drawTransparentTile(_playerTopLeft, 0);
        _redrawPlayer = false;
    }
    Direction combineDown(Direction d)
    {
        if (d == directionRight)
            return directionDownRight;
        if (d == directionLeft)
            return directionDownLeft;
        return directionDown;
    }
    Direction combineUp(Direction d)
    {
        if (d == directionRight)
            return directionUpRight;
        if (d == directionLeft)
            return directionUpLeft;
        return directionUp;
    }
    bool isLeft(Direction d)
    {
        return d == directionLeft || d == directionDownLeft ||
            d == directionUpLeft;
    }
    bool isUp(Direction d)
    {
        return d == directionUp || d == directionUpLeft ||
            d == directionUpRight;
    }
    bool isRight(Direction d)
    {
        return d == directionRight || d == directionDownRight ||
            d == directionUpRight;
    }
    bool isDown(Direction d)
    {
        return d == directionDown || d == directionDownRight ||
            d == directionDownLeft;
    }
    Direction calculateDirection()
    {
        Direction d = directionStopped;
        int x = ((_xSubTile >> 8) - _xSubTileHighOld) & 7;
        if (x == 1)
            d = directionRight;
        else {
            if (x == 7)
                d = directionLeft;
        }
        if (x >= 2 && x <= 6)
            printf("Error: x moved %i\n",x);
        int y = ((_ySubTile >> 8) - _ySubTileHighOld) & 15;
        if (y == 1)
            d = combineDown(d);
        else {
            if (y == 15)
                d = combineUp(d);
        }
        if (y >= 2 && y <= 14)
            printf("Error: x moved %i\n",x);
        return d;
    }
    Direction calculateTileDirection()
    {
        SInt16 delta = _mapTL - _oldMapTL;
        if (delta < 0) {
            if (delta < -_mapStride)
                return directionUpLeft;
            if (delta > -_mapStride) {
                if (delta < -1)
                    return directionUpRight;
                return directionLeft;
            }
            return directionUp;
        }
        if (delta > 0) {
            if (delta < _mapStride) {
                if (delta > 1)
                    return directionDownLeft;
                return directionRight;
            }
            if (delta > _mapStride)
                return directionDownRight;
            return directionDown;
        }
        return directionStopped;
    }
    void normalize()
    {
        if ((_xSubTile >> 8) >= _tileColumns) {
            _xSubTile -= _tileColumns << 8;
            ++_mapTL;
        }
        else {
            if ((_xSubTile >> 8) < 0) {
                _xSubTile += _tileColumns << 8;
                --_mapTL;
            }
        }
        if ((_ySubTile >> 8) >= _tileRows) {
            _ySubTile -= _tileRows << 8;
            _mapTL += _mapStride;
        }
        else {
            if ((_ySubTile >> 8) < 0) {
                _ySubTile += _tileRows << 8;
                _mapTL -= _mapStride;
            }
        }
    }
    bool checkPlayerTileCollision(int x, int y)
    {
        int m = _mapTL + ((_yPlayer + (_ySubTile >> 8))/_tileRows + y + 1)*_mapStride + (_xPlayer + (_xSubTile >> 8))/_tileColumns + x + 1;
        Byte f = _foreground[m];
        const Byte* playerMask = collisionMasks[0] + ((_xSubTile >> 8) & 7)*_tileRows;
        const Byte* tileMask = collisionMasks[f];

        Byte c = 0;
        if (y == 0) {
            int yp = 0;
            for (int yy = (_yPlayer + (_ySubTile >> 8)) % _tileRows; yy < _tileRows; ++yy) {
                c |= (playerMask[yp] & tileMask[yy]);
                ++yp;
            }
        }
        else {
            int yy = 0;
            for (int yp = _tileRows - (_yPlayer + (_ySubTile >> 8))%_tileRows; yp < _tileRows; ++yp) {
                c |= (playerMask[yp] & tileMask[yy]);                
                ++yy;
            }
        }
        if (x == 0)
            c &= _leftCollisionTable[_xSubTile >> 8];
        else
            c &= _rightCollisionTable[_xSubTile >> 8];
        if (c == 0)
            return false;

        switch (collisionHandlers[f]) {
            case collisionHandlerCoin:
                _foreground[m] = 0xff;
                addTileModification(m, f);
                _redrawPlayer = true;
                break;
            case collisionHandlerPlatform:
                {
                    if (_yVelocity <= 0 || y == 0)
                        break;
                    int yy = (_yPlayer + (_ySubTile >> 8))%_tileRows;
                    if (yy > 3)
                        break;  // Already below top of tile
                    _yVelocity = 0;
                    _landed = true;
                    _ySubTile = _ySubTile - 0x100;
                    return true;
                }
                break;
        }
        return false;
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
    int _bufferTileStride;
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
    Array<Byte> _leftCollisionTable;
    Array<Byte> _rightCollisionTable;
    Array<Word> _bufferLeft;
    Array<Word> _mapLeft;
    Array<Word> _bufferTop;
    Array<Word> _bufferRight;
    Array<Word> _mapRight;
    Array<Word> _bufferBottom;
    Word _mapBottom;
    Array<Word> _tilePointers;
    Word _topLeftBuffer;
    Word _topLeftMap;
    Word _topRightBuffer;
    Word _topRightMap;
    Word _bottomLeftBuffer;
    Word _bottomLeftMap;
    Word _bottomRightBuffer;
    Word _bottomRightMap;
    int _xPlayer;
    int _yPlayer;
    int _playerTopLeft;
    bool _redrawPlayer;
    Word _oldMapTL;
    int _xSubTileHighOld;
    int _ySubTileHighOld;
    bool _landed;

    Direction _direction;
    Direction _tileDirection;

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

    Array<Byte> _underPlayer;

    int _tilesDrawn;
    LARGE_INTEGER _startTime;

    AppendableArray<UpdateBlock> _updateBlocks;
    AppendableArray<TileModification> _tileModifications;
};

class Program : public WindowProgram<GameWindow>
{
};
