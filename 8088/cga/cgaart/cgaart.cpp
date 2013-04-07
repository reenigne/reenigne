#include "alfe/main.h"
#include "alfe/bitmap_png.h"

class MainWindow;

class CharacterData
{
public:
    CharacterData()
      : _memoryAddress(0), _rowAddress(0), _memoryAddressSpecified(false),
        _rowAddressSpecified(false), _modesSpecified(0), _palettesSpecified(0),
        _data(0), _dataSpecified(0)
    {
        for (int i = 0; i < 8; ++i) {
            _modes[i] = 0;
            _palettes[i] = 0;
        }
    }

    // _memoryAddress can have some special values:
    // -1 = overscan
    // -2 = horizontal sync
    // -3 = vertical sync
    // -4 = horizontal and vertical sync
    // -5 = vertical adjust
    // -6 = vertical adjust and horizontal sync
    // -7 = not part of selection
    DWord _memoryAddress;
    Byte _rowAddress;
    bool _memoryAddressSpecified;
    bool _rowAddressSpecified;
    Byte _modes[8];
    Byte _palettes[8];
    Byte _modesSpecified;
    Byte _palettesSpecified;
    Word _data;
    bool _dataSpecified;
};

//class MainImage : public Image
//{
//public:
//    void setWindow(MainWindow* window)
//    {
//        _window = window;
//
//        _displayed = Vector(-34, -62);
//        _syncStart = Vector(-24, -38);
//        _syncEnd = Vector(100, 240);
//        _total = Vector(114, 262);
//        _scanlinesPerRow = 8;
//        _mode = 8;  // 40x25 text
//        _palette = 0;
//        _startAddress = 0;
//
//        _cgaRAM.allocate(0x2000);
//        for (int i = 0; i < 0x2000; ++i)
//            _cgaRAM[i] = 0x0720;
//        _characters.allocate(_total.x * _total.y);
//        int memoryAddress = _startAddress;
//        CharacterData* c = &_characters[0];
//        int shift = 0;
//        if ((_mode & 1) == 0)
//            shift = 1;
//        for (int y = 0; y < _total.y; ++y) {
//            for (int x = 0; x < _total.x; ++x) {
//                c->_memoryAddressSpecified = false;
//                c->_rowAddressSpecified = false;
//                c->_modesSpecified = 0;
//                c->_palettesSpecified = 0;
//                c->_rowAddress = y % _scanlinesPerRow;
//                for (int xx = 0; xx < 8; ++xx) {
//                    c->_modes[xx] = _mode;
//                    c->_palettes[xx] = _palette;
//                }
//                c->_memoryAddress = memoryAddress + (x >> shift);
//                if (x >= _displayed.x || y >= _displayed.y) {
//                    c->_memoryAddress = -1;
//                    if (y >= _syncStart.y && y < _syncEnd.y)
//                        c->_memoryAddress = -3;
//                    else
//                        if (y >= _total.y - _total.y % _scanlinesPerRow)
//                            c->_memoryAddress = -5;
//                    if (x >= _syncStart.x && x < _syncEnd.x)
//                        c->_memoryAddress -= 1;
//                }
//            }
//            if (y % _scanlinesPerRow == _scanlinesPerRow - 1)
//                memoryAddress += _displayed.x >> shift;
//        }
//    }
//
//    void paint(const PaintHandle& paint)
//    {
//        draw();
//        Image::paint(paint);
//    }
//
//    virtual void draw()
//    {
//    }
//
//    bool lButtonDown(Vector position)
//    {
//        return false;
//    }
//
//    void lButtonUp(Vector position)
//    {
//    }
//
//    void mouseMove(Vector position)
//    {
//    }
//
//private:
//    MainWindow* _window;
//
//    // Screen measurements and register defaults.
//    // Horizontal measurements are in hchars (even if we're not using +HRES).
//    // Vertical measurements are in scanlines.
//    // All these may be overridden by the _characters entries.
//    Vector _displayed;
//    Vector _syncStart;
//    Vector _syncEnd;
//    Vector _total;
//    int _scanlinesPerRow;
//    Byte _mode;
//    Byte _palette;
//    int _startAddress;
//
//    Array<CharacterData> _characters;
//    Array<Word> _cgaRAM;
//};

//class TestButton : public Button
//{
//public:
//    virtual LPCWSTR initialCaption() const { return L"Press me!"; }
//    virtual Vector initialSize() const { return Vector(100, 20); }
//    virtual Vector initialPosition() const { return Vector(100, 100); }
//    void clicked() const
//    {
//        console.write("Button clicked!\n");
//    }
//};
//

class CanvasWindow : public Window
{
public:
    void create()
    {
    }

};

class MainWindow : public WindowsWindow
{
public:
    MainWindow() { }
    void create()
    {
//        add(new TestButton);
    }
    virtual LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        Vector p = vectorFromLParam(lParam);
        switch (uMsg) {
            case WM_LBUTTONDOWN:
                _lButtonDown = true;
                break;
            case WM_LBUTTONUP:
                _lButtonDown = false;
                releaseCapture();
                break;
            case WM_MBUTTONDOWN:
                _mButtonDown = true;
                break;
            case WM_MBUTTONUP:
                _mButtonDown = false;
                releaseCapture();
                break;
            case WM_RBUTTONDOWN:
                _rButtonDown = true;
                break;
            case WM_RBUTTONUP:
                _rButtonDown = false;
                releaseCapture();
                break;
            case WM_MOUSEMOVE:
                break;
            case WM_CHAR:
                break;
            case WM_KILLFOCUS:
                _capture = false;
                ReleaseCapture();
                break;
        }
        return WindowsWindow::handleMessage(uMsg, wParam, lParam);
    }
protected:
    LPCWSTR initialCaption() const { return L"CGA Art"; }
    Vector initialSize() const { return Vector(1024, 768); }
private:
    void setCapture()
    {
        _capture = true;
        SetCapture(_hWnd);
    }
    void releaseCapture()
    {
        if (_capture && !_lButtonDown && !_mButtonDown && !_rButtonDown)
            ReleaseCapture();
    }
    static Vector vectorFromLParam(LPARAM lParam)
    {
        return Vector(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    }
    bool _capture;
    bool _lButtonDown;
    bool _mButtonDown;
    bool _rButtonDown;
};

class Program : public WindowProgram<MainWindow> { };
