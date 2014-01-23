class Motorola6845CRTC
{
public:
    Motorola6845CRTC()
      : _horizontalDisplay(false), _verticalDisplay(false),
        _verticalSync(false), _horizontalSync(false), _memoryAddress(0),
        _rowAddress(0), _horizontal(0), _vertical(0), _newRow(false),
        _address(0), _newLine(false), _leftMemoryAddress(0),
        _cursorLine(false), _verticalSyncCount(0), _newScreen(false)
    {
        for (int i = 0; i < 0x10; ++i)
            _registers[i] = 0;
    }
    void simulateCycle()
    {
        _memoryAddress = (_memoryAddress + 1) & 0x3fff;
        _horizontal = (_horizontal + 1) & 0xff;
        _horizontalSyncCount = (_horizontalSyncCount + 1) & 0xf;
        if (_horizontalSyncCount == horizontalSyncWidth())
            _horizontalSync = false;
        bool newLine = _newLine;
        _newLine = (_horizontal == horizontalTotal());
        bool hh = (_horizontal == (horizontalTotal() >> 1));
        if (_horizontal == horizontalDisplayed())
            _horizontalDisplay = false;
        if (_horizontal == horizontalSyncPosition()) {
            _horizontalSync = true;
            _horizontalSyncCount = 0;
        }
        if (newLine) {
            _horizontal = 0;
            _horizontalDisplay = true;
            _rowAddress = (_rowAddress + 1) & 0x1f;
            _verticalAdjustCount++;
            bool newRow = _newRow;
            _newRow = (_rowAddress == maxScanLineAddress());
            if (_rowAddress == cursorStart())
                _cursorLine = true;
            bool newCursorEnd = _newCursorEnd;
            _newCursorEnd = (_rowAddress == cursorEnd());
            if (newCursorEnd)
                _cursorLine = false;
            ++_verticalSyncCount;
            if (_verticalSyncCount == 16)
                _verticalSync = false;
            if (newRow) {
                _rowAddress = 0;
                _vertical = (_vertical + 1) & 0x7f;
                if (_vertical == verticalSyncPosition()) {
                    _verticalSync = true;
                    _verticalSyncCount = 0;
                }
                if (_vertical == verticalDisplayed())
                    _verticalDisplay = false;
            }
            bool newScreen = _newScreen;
            _newScreen = (_verticalAdjustCount == ((verticalTotal() * maxScanLineAddress()) + verticalTotalAdjust()));
            if (newScreen) {
                _vertical = 0;
                _verticalAdjustCount = 0;
                _verticalDisplay = true;
                _leftMemoryAddress = startAddress();
            }
            _memoryAddress = _leftMemoryAddress;
        }
        if (_newRow)
            ++_leftMemoryAddress;

        if (_lightPenActivated) {
            _lightPenActivated = false;
            _lightPen = _memoryAddress;
        }
    }
    void write(bool rs, UInt8 value)
    {
        static const int masks[0x10] = {
            0xff, 0xff, 0xff, 0x0f, 0x7f, 0x1f, 0x7f, 0x7f,
            0x03, 0x1f, 0x7f, 0x1f, 0x3f, 0xff, 0x3f, 0xff};
        if (!rs)
            _address = value & 0x1f;
        else
            if (_address < 0x10)
                _registers[_address] = value & masks[_address];
    }
    UInt8 read(bool rs)
    {
        if (!rs)
            return 0xff;
        if (_address < 0x0e || _address >= 0x12)
            return 0x00;
        if (_address < 0x10)
            return _registers[_address];
        if (_address == 0x10)
            return _lightPen >> 8;
        return _lightPen & 0xff;
    }
    void activateLightPen() { _lightPenActivated = true; }
    bool displayEnable() const
    { 
        return _horizontalDisplay && _verticalDisplay;
    }
    UInt16 memoryAddress() const { return _memoryAddress; }
    UInt8 rowAddress() const { return _rowAddress; }
    bool horizontalSync() const { return _horizontalSync; }
    bool verticalSync() const { return _verticalSync; }
    bool cursorOn() const { return _cursorLine && _memoryAddress == cursor(); }

private:
    int horizontalTotal() const { return _registers[0] + 1; }
    int horizontalDisplayed() const { return _registers[1] + 1; }
    int horizontalSyncPosition() const { return _registers[2]; }
    int horizontalSyncWidth() const { return _registers[3]; }
    int verticalTotal() const { return _registers[4] + 1; }
    int verticalTotalAdjust() const { return _registers[5]; }
    int verticalDisplayed() const { return _registers[6]; }
    int verticalSyncPosition() const { return _registers[7]; }
    int interlaceMode() const { return _registers[8]; }
    int maxScanLineAddress() const { return _registers[9]; }
    int cursorStart() const { return _registers[10] & 0x1f; }
    bool cursorBlinkPeriod() const { return (_registers[10] & 0x20) != 0; }
    bool cursorBlink() const { return (_registers[10] & 0x40) != 0; }
    int cursorEnd() const { return _registers[11]; }
    int startAddress() const { return (_registers[12] << 8) | _registers[13]; }
    int cursor() const { return (_registers[14] << 8) | _registers[15]; }

    bool _horizontalDisplay;
    bool _verticalDisplay;
    bool _verticalSync;
    bool _horizontalSync;
    UInt16 _memoryAddress;
    UInt8 _rowAddress;
    int _horizontal;
    int _vertical;
    int _horizontalSyncCount;
    bool _newLine;
    bool _newRow;
    bool _newCursorEnd;
    bool _newScreen;
    bool _cursorLine;
    UInt16 _leftMemoryAddress;
    int _verticalSyncCount;
    int _verticalAdjustCount;

    UInt8 _registers[0x10];
    UInt8 _address;
    UInt16 _lightPen;
    bool _lightPenActivated;
};