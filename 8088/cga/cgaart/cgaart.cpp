#include "alfe/main.h"
#include "alfe/bitmap_png.h"
#include "alfe/complex.h"
#include "alfe/space.h"
#include "alfe/set.h"
#include "alfe/config_file.h"
#include "alfe/cga.h"
#include "alfe/ntsc_decode.h"
#include "alfe/knob.h"
#include "alfe/scanlines.h"
#include "alfe/image_filter.h"
#include "alfe/wrap.h"

template<class T> class CGAArtWindowT;
typedef CGAArtWindowT<void> CGAArtWindow;

class CGAData : Uncopyable
{
public:
    CGAData() : _total(1) { reset(); }
    void reset()
    {
        _root.reset();
        _endAddress = 0;
    }
    // Output RGBI values:
    //   0-15: normal active data
    //   16-54: blanking
    //     bit 0: CRT hsync
    //     bit 1: CRT vsync
    //     bit 2: composite sync (CRT hsync ^ CRT vsync)
    //     bit 3: colour burst
    //     bit 4: CRTC hsync
    //     bit 5: CRTC vsync
    //     bit 6: CRTC hsync | CRTC vsync
    //   Only the following blanking values actually occur:
    //     0x50: CRTC hsync
    //     0x55: CRT hsync (composite sync)
    //     0x58: Colour burst (suppressed during CRTC vsync)
    //     0x60: CRTC vsync
    //     0x66: CRT vsync (composite sync)
    //     0x70: CRTC hsync + CRTC vsync
    //     0x73: CRT hsync + CRT vsync (no composite sync)
    //     0x75: CRT hsync + CRTC vsync (composite sync)
    //     0x76: CRTC hsync + CRT vsync (composite sync)
    void output(int t, int n, Byte* rgbi, CGASequencer* sequencer, int phase)
    {
        Lock lock(&_mutex);

        State state;
        state._n = n;
        state._rgbi = rgbi;
        state._t = t;
        state._addresses = _endAddress - registerLogCharactersPerBank;
        state._data.allocate(state._addresses);
        state._sequencer = sequencer;
        state._phase = phase != 0 ? 0x40 : 0;
        for (const auto& c : _root._changes)
            c.getData(&state._data, 0, registerLogCharactersPerBank,
                state._addresses, 0);
        state.reset();
        if (_root._right != 0)
            _root._right->output(0, 0, _total, &state);
        state.runTo(t + n);
    }
    enum {
        registerLogCharactersPerBank = -26,  // log(characters per bank)/log(2)
        registerScanlinesRepeat,
        registerHorizontalTotalHigh,
        registerHorizontalDisplayedHigh,
        registerHorizontalSyncPositionHigh,
        registerVerticalTotalHigh,
        registerVerticalDisplayedHigh,
        registerVerticalSyncPositionHigh,
        registerMode,                        // port 0x3d8
        registerPalette,                     // port 0x3d9
        registerHorizontalTotal,             // CRTC register 0x00
        registerHorizontalDisplayed,         // CRTC register 0x01
        registerHorizontalSyncPosition,      // CRTC register 0x02
        registerHorizontalSyncWidth,         // CRTC register 0x03
        registerVerticalTotal,               // CRTC register 0x04
        registerVerticalTotalAdjust,         // CRTC register 0x05
        registerVerticalDisplayed,           // CRTC register 0x06
        registerVerticalSyncPosition,        // CRTC register 0x07
        registerInterlaceMode,               // CRTC register 0x08
        registerMaximumScanline,             // CRTC register 0x09
        registerCursorStart,                 // CRTC register 0x0a
        registerCursorEnd,                   // CRTC register 0x0b
        registerStartAddressHigh,            // CRTC register 0x0c
        registerStartAddressLow,             // CRTC register 0x0d
        registerCursorAddressHigh,           // CRTC register 0x0e
        registerCursorAddressLow             // CRTC register 0x0f
    };                                       // 0 onwards: VRAM
    void change(int t, int address, Byte data)
    {
        change(t, address, 1, &data);
    }
    void change(int t, int address, int count, const Byte* data)
    {
        Lock lock(&_mutex);
        changeNoLock(t, address, count, data);
    }
    void remove(int t, int address, int count = 1)
    {
        Lock lock(&_mutex);
        _root.remove(t, address, count, 0, _total);
    }
    void setTotals(int total, int pllWidth, int pllHeight)
    {
        Lock lock(&_mutex);
        if (_root._right != 0)
            _root._right->resize(_total, total);
        _total = total;
        _pllWidth = pllWidth;
        _pllHeight = pllHeight;
    }
    int getTotal()
    {
        Lock lock(&_mutex);
        return _total;
    }
    int getPLLWidth() { return _pllWidth; }
    int getPLLHeight() { return _pllHeight; }
    void save(File file)
    {
        Lock lock(&_mutex);
        AppendableArray<Byte> data;
        data.append(reinterpret_cast<const Byte*>("CGAD"), 4);
        DWord version = 0;
        data.append(reinterpret_cast<const Byte*>(&version), 4);
        DWord total = _total;
        data.append(reinterpret_cast<const Byte*>(&total), 4);
        DWord pllWidth = _pllWidth;
        data.append(reinterpret_cast<const Byte*>(&pllWidth), 4);
        DWord pllHeight = _pllHeight;
        data.append(reinterpret_cast<const Byte*>(&pllHeight), 4);
        _root.save(&data, 0, 0, _total);
        file.openWrite().write(data);
    }
    void load(File file)
    {
        Lock lock(&_mutex);
        _root.reset();
        Array<Byte> data;
        file.readIntoArray(&data);
        if (deserialize(&data, 0) != *reinterpret_cast<const DWord*>("CGAD"))
            throw Exception(file.path() + " is not a CGAData file.");
        if (deserialize(&data, 4) != 0)
            throw Exception(file.path() + " is too new for this program.");
        _total = deserialize(&data, 8);
        _pllWidth = deserialize(&data, 12);
        _pllHeight = deserialize(&data, 16);
        int offset = 20;
        do {
            if (offset == data.count())
                return;
            int t = deserialize(&data, offset);
            int address = deserialize(&data, offset + 4);
            int count = deserialize(&data, offset + 8);
            if (offset + 12 + count < data.count())
                throw Exception(file.path() + " is truncated.");
            changeNoLock(t, address, count, &data[offset + 12]);
        } while (true);
    }
    void saveVRAM(File file)
    {
        file.openWrite().write(getData(0, _endAddress, 0));
    }
    void loadVRAM(File file)
    {
        Lock lock(&_mutex);
        _root.reset();
        Array<Byte> data;
        file.readIntoArray(&data);
        changeNoLock(0, 0, data.count(), &data[0]);
    }
    Byte getDataByte(int address, int t = 0)
    {
        return getData(address, 1, t)[0];
    }
    Array<Byte> getData(int address, int count, int t = 0)
    {
        Lock lock(&_mutex);
        Array<Byte> result(count);
        Array<bool> gotResult(count);
        for (int i = 0; i < count; ++i)
            gotResult[i] = false;
        _root.getData(&result, &gotResult, t, address, count, 0, _total, 0);
        return result;
    }

private:
    void changeNoLock(int t, int address, int count, const Byte* data)
    {
        _root.change(t, address, count, data, 0, _total);
        if (address + count > _endAddress) {
            _endAddress = address + count;
            _root.ensureAddresses(registerLogCharactersPerBank, _endAddress);
        }
    }
    int deserialize(Array<Byte>* data, int offset)
    {
        if (data->count() < offset + 4)
            return -1;
        return *reinterpret_cast<DWord*>(&(*data)[offset]);
    }

    struct Change
    {
        Change() { }
        Change(int address, const Byte* data, int count)
          : _address(address), _data(count)
        {
            memcpy(&_data[0], data, count);
        }
        int count() const { return _data.count(); }
        int start() const { return _address; }
        int end() const { return _address + count(); }
        int getData(Array<Byte>* result, Array<bool>* gotResult, int address,
            int count, int gotCount) const
        {
            int s = max(address, start());
            int e = min(address + count, end());
            for (int a = s; a < e; ++a) {
                int i = a - address;
                if (gotResult == 0 || !(*gotResult)[i]) {
                    (*result)[i] = _data[a - _address];
                    if (gotResult != 0)
                        (*gotResult)[i] = true;
                    ++gotCount;
                }
            }
            return gotCount;
        }

        int _address;
        Array<Byte> _data;
    };
    struct State
    {
        void latch()
        {
            int vRAMAddress = _memoryAddress << 1;
            int bytesPerBank = 2 << dat(registerLogCharactersPerBank);
            if ((dat(registerMode) & 2) != 0) {
                if ((_rowAddress & 1) != 0)
                    vRAMAddress |= bytesPerBank;
                else
                    vRAMAddress &= ~bytesPerBank;
            }
            vRAMAddress &= (bytesPerBank << 1) - 1;
            _latch = (_latch << 16) + dat(vRAMAddress) +
                (dat(vRAMAddress + 1) << 8);
        }
        void startOfFrame()
        {
            _memoryAddress = (dat(registerStartAddressHigh) << 8) +
                dat(registerStartAddressLow);
            _leftMemoryAddress = _memoryAddress;
            _rowAddress = 0;
            _row = 0;
            _scanlineIteration = 0;
        }
        void reset()
        {
            startOfFrame();
            _character = 0;
            _hdot = 0;
            _state = 0;
            latch();
        }
        void runTo(int t)
        {
            Byte mode = dat(registerMode);
            int hdots = (mode & 1) != 0 ? 8 : 16;
            while (_t < t) {
                int c = min(hdots, _hdot + t - _t);
                if (_state == 0) {
                    UInt64 r = _sequencer->process(_latch, mode | _phase,
                        dat(registerPalette), _rowAddress, false, 0);
                    for (; _hdot < c; ++_hdot) {
                        *_rgbi = (r >> (_hdot * 4)) & 0x0f;
                        ++_rgbi;
                    }
                }
                else {
                    int v = 0;
                    if ((_state & 0x18) == 0) {
                        v = (mode & 0x10) != 0 ? 0 :
                            dat(registerPalette) & 0xf;
                    }
                    else {
                        v = (_state & 0x10) + ((_state & 0x20) >> 1);
                        static Byte sync[48] = {
                            0x50, 0x50, 0x55, 0x55, 0x55, 0x55, 0x50, 0x58,
                            0x58, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50,

                            0x70, 0x70, 0x75, 0x75, 0x75, 0x75, 0x70, 0x70,
                            0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70,

                            0x76, 0x76, 0x73, 0x73, 0x73, 0x73, 0x76, 0x76,
                            0x76, 0x76, 0x76, 0x76, 0x76, 0x76, 0x76, 0x76};
                        if ((_state & 8) != 0) {
                            if ((mode & 1) != 0) {
                                v = sync[((_hSync + (_phase >> 6)) >> 1) + v];
                            }
                            else
                                v = sync[_hSync + v];
                        }
                        else
                            v = (_state & 0x20) != 0 ? 0x66 : 0x60;
                    }
                    memset(_rgbi, v, c);
                    _rgbi += c;
                    _hdot += c;
                }
                _t += c;
                if (_t == t)
                    break;
                _hdot = 0;

                // Emulate CRTC
                if (_character == dat(registerHorizontalTotal) +
                    (dat(registerHorizontalTotalHigh) << 8)) {
                    _state &= ~1;
                    _character = 0;
                    if ((_state & 0x10) != 0) {
                        // Vertical sync active
                        ++_vSync;
                        if ((_vSync & 0x0f) == 0) {
                            // End of vertical sync
                            _state &= ~0x30;
                        }
                    }
                    ++_scanlineIteration;
                    if (_scanlineIteration == dat(registerScanlinesRepeat)) {
                        _scanlineIteration = 0;
                        if (_rowAddress == dat(registerMaximumScanline)) {
                            // End of row
                            _rowAddress = 0;
                            if (_row == dat(registerVerticalTotal) +
                                (dat(registerVerticalTotalHigh) << 8)) {
                                _state |= 4;
                                _adjust = 0;
                                _latch = 0;
                            }
                            ++_row;
                            if (_row == dat(registerVerticalDisplayed) +
                                (dat(registerVerticalDisplayedHigh) << 8)) {
                                _state |= 2;
                                _latch = 0;
                            }
                            if (_row == dat(registerVerticalSyncPosition) +
                                (dat(registerVerticalSyncPositionHigh) << 8)) {
                                _state |= 0x10;
                                _vSync = 0;
                            }
                            _memoryAddress = _nextRowMemoryAddress;
                            _leftMemoryAddress = _nextRowMemoryAddress;
                        }
                        else {
                            ++_rowAddress;
                            _memoryAddress = _leftMemoryAddress;
                        }
                    }
                    else
                        _memoryAddress = _leftMemoryAddress;
                    if ((_state & 4) != 0) {
                        // Vertical total adjust active
                        if (_adjust == dat(registerVerticalTotalAdjust)) {
                            startOfFrame();
                            _state &= ~4;
                        }
                        else
                            ++_adjust;
                    }
                }
                else {
                    ++_character;
                    ++_memoryAddress;
                }
                _phase ^= 0x40;
                if (_character == dat(registerHorizontalDisplayed) +
                    (dat(registerHorizontalDisplayedHigh) << 8)) {
                    _state |= 1;
                    _nextRowMemoryAddress = _memoryAddress;
                    _latch = 0;
                }
                if ((_state & 8) != 0) {
                    // Horizontal sync active
                    ++_hSync;
                    bool crtSync = _hSync == 2;
                    if ((mode & 1) != 0)
                        crtSync = (((_hSync + (_phase >> 6)) >> 1) == 2);
                    if (crtSync && (_state & 0x10) != 0) {
                        if (_vSync == 0)
                            _state |= 0x20;
                        else {
                            if (_vSync == 3)
                                _state &= ~0x20;
                        }
                    }
                    if ((_hSync & 0x0f) == dat(registerHorizontalSyncWidth))
                        _state &= ~8;
                }
                if (_character == dat(registerHorizontalSyncPosition) +
                    (dat(registerHorizontalSyncPositionHigh) << 8)) {
                    _state |= 8;
                    _hSync = 0;
                }
                if (_state == 0)
                    latch();
            }
        }
        Byte dat(int address)
        {
            return _data[address - registerLogCharactersPerBank];
        }

        int _memoryAddress;
        int _leftMemoryAddress;
        int _nextRowMemoryAddress;
        int _rowAddress;
        int _character;
        int _adjust;
        int _hSync;
        int _vSync;
        int _row;
        int _hdot;
        int _n;
        int _t;
        int _phase;
        int _addresses;
        int _scanlineIteration;

        // _state bits:
        //    0 = we are in horizontal overscan
        //    1 = we are in vertical overscan
        //    2 = we are in vertical total adjust
        //    3 = we are in horizontal sync
        //    4 = we are in vertical CRTC sync
        //    5 = we are in vertical CRT sync
        int _state;
        UInt32 _latch;
        Byte* _rgbi;
        Array<Byte> _data;
        CGASequencer* _sequencer;
    };
    struct Node
    {
        Node() : _left(0), _right(0) { }
        void reset()
        {
            if (_left != 0)
                delete _left;
            if (_right != 0)
                delete _right;
        }
        ~Node() { reset(); }
        void findChanges(int address, int count, int* start, int* end)
        {
            int lowStart = 0;
            int lowEnd = _changes.count() - 1;
            while (lowStart < lowEnd) {
                int lowTest = lowStart + (lowEnd - lowStart)/2;
                if (_changes[lowTest].end() <= address) {
                    lowStart = lowTest + 1;
                    continue;
                }
                if (_changes[lowTest].start() >= address + count) {
                    lowEnd = lowTest - 1;
                    continue;
                }
                lowStart = lowTest;
                break;
            }
            int highStart = lowStart;
            int highEnd = _changes.count() - 1;
            while (highStart < highEnd) {
                int highTest = highStart + (highEnd - highStart)/2;
                if (_changes[highTest].start() >= address + count) {
                    highStart = highTest - 1;
                    continue;
                }
                if (_changes[highTest].end() <= address) {
                    highEnd = highTest + 1;
                    continue;
                }
                highStart = highTest;
                break;
            }
            *start = lowStart;
            *end = highEnd;
        }
        void change(int t, int address, int count, const Byte* data,
            int leftTotal, int rightTotal)
        {
            if (t > 0) {
                if (_right == 0)
                    _right = new Node();
                int rlTotal = roundUpToPowerOf2(rightTotal) / 2;
                _right->change(t - rlTotal, address, count, data, rlTotal,
                    rightTotal - rlTotal);
                return;
            }
            if (t == 0) {
                int start, end;
                findChanges(address, count, &start, &end);
                if (start <= end) {
                    int startAddress = min(address, _changes[start].start());
                    Change e = _changes[end];
                    int e2 = address + count;
                    int endAddress = max(e2, e.end());
                    Change c;
                    c._data.allocate(endAddress - startAddress);
                    c._address = startAddress;
                    int a = 0;
                    if (startAddress < address) {
                        a = address - startAddress;
                        memcpy(&c._data[0], &_changes[start]._data[0],
                            min(a, _changes[start].count()));
                    }
                    memcpy(&c._data[a], data, count);
                    if (endAddress > e2) {
                        int offset = e2 - e.start();
                        if (offset >= 0) {
                            memcpy(&c._data[a + count],
                                &e._data[e2 - e.start()], endAddress - e2);
                        }
                        else {
                            memcpy(&c._data[a + count - offset],
                                &e._data[0], endAddress - e.start());
                        }
                    }
                    if (start < end) {
                        Array<Change> changes(_changes.count() + start - end);
                        for (int i = 0; i < start; ++i)
                            changes[i] = _changes[i];
                        changes[start] = c;
                        for (int i = start + 1; i < changes.count(); ++i)
                            changes[i] = _changes[i + end - start];
                        _changes = changes;
                    }
                    else
                        _changes[start] = c;
                }
                else {
                    Array<Change> changes(_changes.count() + 1);
                    for (int i = 0; i < start; ++i)
                        changes[i] = _changes[i];
                    changes[start] = Change(address, data, count);
                    for (int i = start; i < _changes.count(); ++i)
                        changes[i + 1] = _changes[i];
                    _changes = changes;
                }
                return;
            }
            if (_left == 0)
                _left = new Node();
            int llTotal = roundUpToPowerOf2(leftTotal) / 2;
            int lrTotal = leftTotal - llTotal;
            _left->change(t + lrTotal, address, count, data, llTotal, lrTotal);
        }
        void remove(int t, int address, int count, int leftTotal,
            int rightTotal)
        {
            if (t > 0) {
                if (_right != 0) {
                    int rlTotal = roundUpToPowerOf2(rightTotal) / 2;
                    _right->remove(t - rlTotal, address, count, rlTotal,
                        rightTotal - rlTotal);
                }
                return;
            }
            if (t == 0) {
                int start, end;
                findChanges(address, count, &start, &end);
                int deleteStart = end, deleteEnd = start;
                for (int i = start; i < end; ++i) {
                    int newCount;
                    Change c = _changes[i];
                    int e2 = address + count;
                    if (address < c._address) {
                        newCount = max(0, c.end() - e2);
                        int offset = c.count() - newCount;
                        _changes[i] = Change(c._address + offset,
                            &c._data[offset], newCount);
                    }
                    else {
                        newCount = address - c.start();
                        if (newCount < c.count()) {
                            _changes[i] =
                                Change(c._address, &c._data[0], newCount);
                            if (e2 < c.end()) {
                                Array<Change> changes(_changes.count() + 1);
                                for (int j = 0; j <= i; ++j)
                                    changes[j] = _changes[j];
                                changes[i + 1] =
                                    Change(e2, &c._data[newCount + count],
                                    c.end() - e2);
                                for (int j = i + 1; j < _changes.count(); ++j)
                                    changes[j + 1] = _changes[j];
                                _changes = changes;
                            }
                        }
                    }
                    if (_changes[i].count() == 0) {
                        deleteStart = min(deleteStart, i);
                        deleteEnd = max(deleteStart, i + 1);
                    }
                }
                int deleteCount = deleteEnd - deleteStart;
                if (deleteCount > 0) {
                    Array<Change> changes(_changes.count() - deleteCount);
                    for (int i = 0; i < deleteStart; ++i)
                        changes[i] = _changes[i];
                    for (int i = deleteEnd; i < _changes.count(); ++i)
                        changes[i - deleteCount] = _changes[i];
                    _changes = changes;
                }
                return;
            }
            if (_left != 0) {
                int llTotal = roundUpToPowerOf2(leftTotal) / 2;
                int lrTotal = leftTotal - llTotal;
                _left->remove(t + lrTotal, address, count, llTotal, lrTotal);
            }
        }
        int getData(Array<Byte>* result, Array<bool>* gotResult, int t,
            int address, int count, int leftTotal, int rightTotal,
            int gotCount)
        {
            if (t > 0 && _right != 0) {
                int rlTotal = roundUpToPowerOf2(rightTotal) / 2;
                int c = _right->getData(result, gotResult, t - rlTotal,
                    address, count, rlTotal, rightTotal - rlTotal, gotCount);
                if (c == gotCount)
                    return c;
                gotCount = c;
            }
            if (t >= 0 && _changes.count() != 0) {
                int start, end;
                findChanges(address, count, &start, &end);
                for (int i = start; i <= end; ++i) {
                    gotCount = _changes[i].getData(result, gotResult, address,
                        count, gotCount);
                }
            }
            if (_left != 0) {
                int llTotal = roundUpToPowerOf2(leftTotal) / 2;
                int lrTotal = leftTotal - llTotal;
                gotCount = _left->getData(result, gotResult, t + lrTotal,
                    address, count, llTotal, lrTotal, gotCount);
            }
            return gotCount;
        }
        void resize(int oldTotal, int newTotal)
        {
            int lTotal = roundUpToPowerOf2(oldTotal) / 2;
            do {
                if (lTotal < newTotal)
                    break;
                if (_right != 0)
                    delete _right;
                Node* left = _left;
                *this = *left;
                left->_left = 0;
                left->_right = 0;
                delete left;
                oldTotal = lTotal;
                lTotal /= 2;
            } while (true);
            do {
                int rTotal = oldTotal - lTotal;
                int newLTotal = roundUpToPowerOf2(newTotal) / 2;
                if (lTotal >= newLTotal)
                    break;
                Node* newNode = new Node();
                newNode->_left = this;
                *this = *newNode;
                oldTotal += lTotal;
                lTotal *= 2;
            } while (true);
            if (_right != 0)
                _right->resize(oldTotal - lTotal, newTotal - lTotal);
        }
        void save(AppendableArray<Byte>* array, int t, int leftTotal,
            int rightTotal)
        {
            if (_left != 0) {
                int llTotal = roundUpToPowerOf2(leftTotal) / 2;
                int lrTotal = leftTotal - llTotal;
                _left->save(array, t - lrTotal, llTotal, lrTotal);
            }
            for (auto c : _changes) {
                DWord tt = t;
                array->append(reinterpret_cast<const Byte*>(&tt), 4);
                array->append(reinterpret_cast<const Byte*>(&c._address), 4);
                DWord count = c._data.count();
                array->append(reinterpret_cast<const Byte*>(&count), 4);
                array->append(&c._data[0], count);
                DWord zero = 0;
                array->append(reinterpret_cast<const Byte*>(&zero),
                    ((~count) + 1) & 3);
            }
            if (_right != 0) {
                int rlTotal = roundUpToPowerOf2(rightTotal) / 2;
                _right->
                    save(array, t + rlTotal, rlTotal, rightTotal - rlTotal);
            }
        }
        void output(int t, int leftTotal, int rightTotal, State* state)
        {
            if (_left != 0) {
                int llTotal = roundUpToPowerOf2(leftTotal) / 2;
                int lrTotal = leftTotal - llTotal;
                _left->output(t - lrTotal, llTotal, lrTotal, state);
            }
            if (t >= state->_t + state->_n) {
                state->runTo(state->_t + state->_n);
                return;
            }
            state->runTo(t);
            for (const auto& c : _changes) {
                c.getData(&state->_data, 0, registerLogCharactersPerBank,
                    state->_addresses, 0);
            }
            if (_right != 0) {
                int rlTotal = roundUpToPowerOf2(rightTotal) / 2;
                _right->output(t + rlTotal, rlTotal, rightTotal - rlTotal,
                    state);
            }
        }
        void ensureAddresses(int startAddress, int endAddress)
        {
            if (_changes.count() == 0)
                _changes.allocate(1);
            int count = endAddress - startAddress;
            if (_changes[0]._data.count() < count) {
                Array<Byte> data(count);
                memcpy(&data[0] + _changes[0]._address - startAddress,
                    &_changes[0]._data[0], _changes[0]._data.count());
                _changes[0]._data = data;
                _changes[0]._address = startAddress;
            }
        }

        Node* _left;
        Array<Change> _changes;
        Node* _right;
    };
    // The root of the tree always has a 0 _left branch.
    Node _root;
    int _total;
    int _pllWidth;
    int _pllHeight;
    int _endAddress;
    Mutex _mutex;
};

class MatcherTable
{
public:
    MatcherTable() : _next(0x10000), _patterns(0x10000) { }
    void setSize(int entries)
    {
        _table.ensure(entries);
        _entries = entries;
        for (int i = 0; i < _entries; ++i) {
            _table[i]._count = 0;
            _table[i]._pattern = 1;
        }
    }

    void add(Word pattern, int position)
    {
        Entry* e = &_table[position];
        _next[pattern] = e->_pattern;
        e->_pattern = pattern;
        ++e->_count;
    }
    void finalize()
    {
        int p = 0;
        for (int i = 0; i < _entries; ++i) {
            int c = _table[i]._count;
            if (c == 0 && _table[i]._pattern != 1)
                c = 0x10000;
            if (c == 0)
                continue;
            int pattern = _table[i]._pattern;
            _table[i]._pattern = p;
            for (int j = 0; j < c; ++j) {
                _patterns[p] = pattern;
                pattern = _next[pattern];
                ++p;
            }
        }
    }
    int get(int position, Word** p)
    {
        Entry* e = &_table[position];
        *p = &_patterns[e->_pattern];
        if (e->_count == 0 && e->_pattern != 1)
            return 0x10000;
        return e->_count;
    }
private:
    struct Entry
    {
        Word _pattern;
        Word _count;
    };
    Array<Entry> _table;
    Array<Word> _next;
    Array<Word> _patterns;
    int _entries;
};

class Linearizer
{
public:
    Linearizer() : _gamma(0), _showClipping(false) { init(); }
    void setShowClipping(bool showClipping)
    {
        _showClipping = showClipping;
        init();
    }
    void setGamma(float gamma) { _gamma = gamma; init(); }
    Colour linear(SRGB srgb)
    {
        return Colour(linear(srgb.x), linear(srgb.y), linear(srgb.z));
    }
    SRGB srgb(Colour linear)
    {
        return SRGB(srgb(linear.x), srgb(linear.y), srgb(linear.z));
    }
    float linear(Byte srgb) { return _linear[srgb]; }
    Byte srgb(float linear)
    {
        return _srgb[clamp(0, static_cast<int>(linear*_multiplier), 6589)];
    }
private:
    void init()
    {
        if (_gamma == 0) {
            for (int i = 1; i < 255; ++i) {
                float t = i/255.0f;
                if (t <= 0.04045f)
                    _linear[i] = t/12.92f;
                else
                    _linear[i] = pow((t + 0.055f)/(1 + 0.055f), 2.4f);
            }
            for (int i = 0; i < 6590; ++i) {
                float l = i/_multiplier;
                float s;
                if (l <= 0.0031308)
                    s = 12.92f*l;
                else
                    s = 1.055f*pow(l, 1/2.4f) - 0.055f;
                _srgb[i] = byteClamp(0.5f + 255.0f*s);
            }
        }
        else {
            for (int i = 1; i < 255; ++i)
                _linear[i] = pow(i/255.0f, _gamma);
            for (int i = 0; i < 6590; ++i) {
                _srgb[i] = byteClamp(0.5f +
                    255.0f*pow(i/_multiplier, 1/_gamma));
            }
        }
        if (_showClipping) {
            _linear[0] = 1.0f;
            _linear[255] = 0.0f;
        }
        else {
            _linear[0] = 0.0f;
            _linear[255] = 1.0f;
        }
    }

    float _linear[256];
    Byte _srgb[6590];
    float _gamma;
    bool _showClipping;
    constexpr static const float _multiplier = 2*255.0f*12.92f;
};

template<class T> class CGAMatcherT : public ThreadTask
{
public:
    CGAMatcherT()
      : _rgbiPalette(3*0x10), _decoder(128), _active(false), _size(0, 0),
        _skip(0x100)
    {
        _scaler.setProfile(4);
        _scaler.setWidth(1);
        _scaler.setBleeding(2);
        _scaler.setOffset(Vector2<float>(0, 0));
    }
    void setInput(Bitmap<SRGB> input, Vector activeSize)
    {
        _activeSize = activeSize;
        _input = input;
        _active = true;
        initData();
    }
    void setSize(Vector size)
    {
        _activeSize = size;
        initData();
    }
    void setProgram(Program* program) { _program = program; }
    void setSequencer(CGASequencer* sequencer) { _sequencer = sequencer; }
    void setData(CGAData* data) { _data = data; }
    void run()
    {
        _program->setProgress(0);
        int blockLap = (_connector == 0 ? 0 : 3);
        // Resample input image to desired size
        Vector size(_hdotsPerChar*_horizontalDisplayed,
            _scanlinesPerRow*_scanlinesRepeat*_verticalDisplayed);
        _linearizer.setGamma(static_cast<float>(_gamma));
        if (size != _size) {
            _scaler.setZoom(Vector2Cast<float>(_activeSize*
                Vector(1, _interlaceSync ? 2 : 1))/
                Vector2Cast<float>(_input.size()));
            _scaler.setProfile(_prescalerProfile);
            _scaler.setOutputSize(size + Vector(4 + 2*blockLap, 0));
            _scaler.setHorizontalLobes(3);
            _scaler.setVerticalLobes(3);
            _scaler.init();
            _size = size;
            AlignedBuffer input = _scaler.input();
            Vector tl = _scaler.inputTL() - Vector(blockLap, 0);
            Vector br = _scaler.inputBR() - Vector(blockLap, 0);
            Byte* unscaledRow = input.data() - tl.y*input.stride();
            Byte* inputRow = _input.data();
            int height = _input.size().y;
            if (tl.y > 0) {
                inputRow += _input.stride()*tl.y;
                height -= tl.y;
            }
            int below = br.y - _input.size().y;
            if (below < 0)
                height += below;
            int width = _input.size().x;
            if (tl.x > 0) {
                inputRow += sizeof(SRGB)*tl.x;
                width -= tl.x;
            }
            int right = br.x - _input.size().x;
            if (right < 0)
                width += right;
            for (int y = 0; y < height; ++y) {
                Colour* unscaled = reinterpret_cast<Colour*>(unscaledRow);
                SRGB* p = reinterpret_cast<SRGB*>(inputRow);
                for (int x = 0; x < -tl.x; ++x) {
                    *unscaled = _linearizer.linear(*p);
                    ++unscaled;
                }
                for (int x = 0; x < width; ++x) {
                    *unscaled = _linearizer.linear(*p);
                    ++unscaled;
                    ++p;
                }
                --p;
                for (int x = 0; x < right; ++x) {
                    *unscaled = _linearizer.linear(*p);
                    ++unscaled;
                }
                unscaledRow += input.stride();
                inputRow += _input.stride();
            }
            unscaledRow = input.data();
            Byte* pp = input.data() - tl.y*input.stride();
            for (int y = 0; y < -tl.y; ++y) {
                memcpy(unscaledRow, pp, (br.x - tl.x)*sizeof(Colour));
                unscaledRow += input.stride();
            }
            unscaledRow =
                input.data() + (_input.size().y - tl.y)*input.stride();
            pp = unscaledRow - input.stride();
            for (int y = 0; y < below; ++y) {
                memcpy(unscaledRow, pp, (br.x - tl.x)*sizeof(Colour));
                unscaledRow += input.stride();
            }
            _scaler.render();
            _scaled = _scaler.output();
        }
        double quality = _quality;
        int mode1 = (_mode & 0x13) + (_scanlinesPerRow > 2 ? 0x100 : 0) +
            (_connector != 0 ? 0x400 : 0);
        if (mode1 == 0x13 || (mode1 & 0x503) == 2)
            quality = 1;

        // Set up gamut table
        float gDivisions = static_cast<float>(64.0/pow(2, quality*6));
        Vector3<float> srgbScale;
        srgbScale.y = gDivisions/256;
        srgbScale.x = srgbScale.y*0.84f;
        srgbScale.z = srgbScale.y*0.55f;
        Vector3<int> srgbDiv = Vector3Cast<int>(255.0f*srgbScale) + 1;
        int entries = srgbDiv.x*srgbDiv.y*srgbDiv.z;
        _table.setSize(entries);
        _blockHeight = _scanlinesPerRow*_scanlinesRepeat;
        if ((_mode & 2) != 0) {
            _incrementWidth = 4;
            _rgbiWidth = ((_connector == 0 || (_mode & 1) != 0) ? 4 : 8);
            _blockHeight = (_scanlinesPerRow <= 2 ? 1 : _scanlinesPerRow)*
                _scanlinesRepeat;
            for (int i = 0; i < 0x100; ++i)
                _skip[i] = false;
        }
        else {
            _incrementWidth = (_mode & 1) != 0 ? 8 : 16;
            _rgbiWidth = _incrementWidth;
            bool blink = ((_mode & 0x20) != 0);
            auto cgaROM = _sequencer->romData();
            int lines = _scanlinesPerRow*_scanlinesRepeat;
            for (int i = 0; i < 0x100; ++i) {
                _skip[i] = false;
                if (_characterSet == 0) {
                    _skip[i] = (i != 0xdd);
                    continue;
                }
                if (_characterSet == 1) {
                    _skip[i] = (i != 0x13 && i != 0x55);
                    continue;
                }
                if (_characterSet == 2) {
                    _skip[i] =
                        (i != 0x13 && i != 0x55 && i != 0xb0 && i != 0xb1);
                    continue;
                }
                if (_characterSet == 4) {
                    _skip[i] = (i != 0xb1);
                    continue;
                }
                if (_characterSet == 5) {
                    _skip[i] = (i != 0xb0 && i != 0xb1);
                    continue;
                }
                if (_characterSet == 6) {
                    _skip[i] = (i != 0x06 && i != 0x13 && i != 0x19 &&
                        i != 0x22 && i != 0x27 && i != 0x55 && i != 0x57 &&
                        i != 0x60 && i != 0xb6 && i != 0xdd);
                }
                if ((_mode & 0x10) != 0)
                    continue;
                bool isBackground = true;
                bool isForeground = true;
                for (int y = 0; y < lines; ++y) {
                    Byte b = cgaROM[i*8 + y];
                    if (b != 0x00)
                        isBackground = false;
                    if (b != 0xff)
                        isForeground = false;
                }
                if (isBackground || (isForeground && blink)) {
                    _skip[i] = true;
                    continue;
                }
                int j;
                for (j = 0; j < i; ++j) {
                    int y;
                    for (y = 0; y < lines; ++y)
                        if (cgaROM[i*8 + y] != cgaROM[j*8 + y])
                            break;
                    if (y == lines)
                        break;
                }
                if (j != i)
                    _skip[i] = true;
                if (blink)
                    continue;
                for (j = 0; j < i; ++j) {
                    int y;
                    for (y = 0; y < lines; ++y)
                        if (cgaROM[i*8 + y] != (cgaROM[j*8 + y]^0xff))
                            break;
                    if (y == lines)
                        break;
                }
                if (j != i)
                    _skip[i] = true;
            }
        }
        _compareWidth = _rgbiWidth + 2*blockLap;
        float blockArea = static_cast<float>(_compareWidth*_blockHeight);
        int banks = ((_mode & 2) != 0 && _scanlinesPerRow > 1) ? 2 : 1;
        int bytesPerRow = 2*_horizontalDisplayed;
        Byte burst[4];
        bool newCGA = _connector == 2;
        double saturation = _saturation*1.45*(newCGA ? 1.5 : 1.0)/100;
        if (_connector == 0) {
            Byte levels[4];
            for (int i = 0; i < 4; ++i) {
                levels[i] = byteClamp(2.55*_brightness + 0.85*i*_contrast +
                    0.5);
            }
            int palette[3*0x10] = {
                0, 0, 0,  0, 0, 2,  0, 2, 0,  0, 2, 2,
                2, 0, 0,  2, 0, 2,  2, 1, 0,  2, 2, 2,
                1, 1, 1,  1, 1, 3,  1, 3, 1,  1, 3, 3,
                3, 1, 1,  3, 1, 3,  3, 3, 1,  3, 3, 3};
            for (int i = 0; i < 3*0x10; ++i)
                _rgbiPalette[i] = levels[palette[i]];
        }
        else {
            _composite.setBW((_mode & 4) != 0);
            _composite.setNewCGA(newCGA);
            _composite.initChroma();
            double black = _composite.black();
            double white = _composite.white();
            _decoder.setLength(_rgbiWidth, _rgbiWidth);
            _decoder.setPadding(0);
            _decoder.setLumaBandwidth(_lumaBandwidth);
            _decoder.setChromaBandwidth(_chromaBandwidth);
            _decoder.setRollOff(_rollOff);
            _decoder.setLobes(_lobes);
            _decoder.setChromaNotch(true);
            _decoder.setHue(_hue + ((_mode & 1) != 0 ? 14 : 4) - 90);
            _decoder.setSaturation(saturation);
            double c = _contrast*256*(newCGA ? 1.2 : 1)/(white - black)/100;
            _decoder.setContrast(c);
            _decoder.setBrightness(
                (-black*c + _brightness*5 + (newCGA ? -50 : 0))/256.0);
            for (int i = 0; i < 4; ++i)
                burst[i] = _composite.simulateCGA(6, 6, i);
            _decoder.calculateBurst(burst);
            _ntscPattern.ensure(_rgbiWidth);
        }
        _srgbPattern.ensure(_rgbiWidth);
        static const Byte hres1bpp[0x10] = {0x00, 0x01, 0x04, 0x05, 0x10, 0x11,
            0x14, 0x15, 0x40, 0x41, 0x44, 0x45, 0x50, 0x51, 0x54, 0x55};
        memcpy(_hres1bpp, hres1bpp, 0x10);

        // Populate gamut table
        int skipSolidColour = 0xf00;
        for (UInt32 pattern = 0;; ++pattern) {
            Colour rgb(0, 0, 0);
            UInt32 dataBits[2];
            int patternCount = 0x100;
            int yMask = 1;
            switch (mode1) {
                // -HRES+GRPH
                case 0x002:
                case 0x012:
                    dataBits[0] = pattern * 0x1111;
                    yMask = 0;
                    patternCount = 0x10;
                    break;
                case 0x402:
                case 0x412:
                    dataBits[0] = pattern * 0x0101;
                    yMask = 0;
                    break;
                case 0x102:
                case 0x112:
                    dataBits[0] = (pattern & 0xf) * 0x1111;
                    dataBits[1] = (pattern >> 4) * 0x1111;
                    break;
                case 0x502:
                case 0x512:
                    dataBits[0] = (pattern & 0xff) * 0x0101;
                    dataBits[1] = (pattern >> 8) * 0x0101;
                    patternCount = 0x10000;
                    break;

                //  +HRES+GRPH
                case 0x103:
                case 0x503:
                    dataBits[0] = (pattern & 0xff) * 0x01010101;
                    dataBits[1] = (pattern >> 8) * 0x01010101;
                    patternCount = 0x10000;
                    break;
                case 0x113:
                case 0x513:
                    dataBits[0] = _hres1bpp[pattern & 0xf] * 0x01010101;
                    dataBits[1] = _hres1bpp[pattern >> 4] * 0x01010101;
                    break;
                case 0x003:
                case 0x403:
                    dataBits[0] = pattern * 0x01010101;
                    yMask = 0;
                    break;
                case 0x013:
                case 0x413:
                    dataBits[0] = _hres1bpp[pattern]*0x01010101;
                    yMask = 0;
                    patternCount = 0x10;
                    break;

                // -GRPH
                default:
                    dataBits[0] = pattern * 0x00010001;
                    patternCount = 0x10000;
                    yMask = 0;
            }
            if (pattern == patternCount)
                break;
            if ((_mode & 2) == 0) {
                if (_skip[pattern & 0xff])
                    continue;
                int foreground = pattern & 0xf00;
                if (foreground == ((pattern >> 4) & 0x0f00)) {
                    if (foreground == skipSolidColour)
                        continue;
                    skipSolidColour = foreground;
                }
            }
            int blockLines = _blockHeight;
            if ((_mode & 2) == 2)
                blockLines = _scanlinesPerRow <= 2 ? 1 : 2;
            for (int y = 0; y < blockLines; ++y) {
                UInt64 rgbi = _sequencer->process(dataBits[y & yMask], _mode,
                    _palette, y, false, 0);
                if (_connector == 0) {
                    SRGB* srgb = &_srgbPattern[0];
                    for (int x = 0; x < _rgbiWidth; ++x) {
                        Byte* p =
                            &_rgbiPalette[3 * ((rgbi >> (x * 4)) & 0xf)];
                        *srgb = SRGB(p[0], p[1], p[2]);
                        ++srgb;
                    }
                }
                else {
                    Byte* ntsc = &_ntscPattern[0];
                    int x;
                    for (x = 0; x < _rgbiWidth - 1; ++x) {
                        *ntsc = _composite.simulateCGA((rgbi >> (x * 4)) & 0xf,
                            (rgbi >> ((x + 1) * 4)) & 0xf, x & 3);
                        ++ntsc;
                    }
                    *ntsc = _composite.simulateCGA((rgbi >> (x * 4)) & 0xf,
                        rgbi & 0xf, x & 3);
                    _decoder.decodeNTSC(&_ntscPattern[0], &_srgbPattern[0]);
                }
                SRGB* srgb = &_srgbPattern[0];
                for (int x = 0; x < _rgbiWidth; ++x)
                    rgb += _linearizer.linear(srgb[x]);
            }
            SRGB srgb = _linearizer.srgb(rgb/blockArea);
            auto s = Vector3Cast<int>(Vector3Cast<float>(srgb)*srgbScale);
            _table.add(pattern, s.x + srgbDiv.x*(s.y + srgbDiv.y*s.z));
        }
        _table.finalize();

        // Set up data structures for matching
        int rowDataStride = 2*_horizontalDisplayed + 1;
        _rowData.ensure(rowDataStride*2);
        _rowData[0] = 0;
        _rowData[rowDataStride] = 0;
        _errorStride = size.x + 1 + 2*blockLap + _rgbiWidth - _incrementWidth;
        int errorSize = _errorStride*(size.y + 1);
        _error.ensure(errorSize);
        for (int x = 0; x < errorSize; ++x)
            _error[x] = Colour(0, 0, 0);
        _rgbiStride = size.x + 1;
        _rgbi.ensure(_rgbiStride*size.y + 1);
        int row = 0;
        int scanline = 0;
        int scanlineIteration = 0;
        int horizontalBlocks = size.x/_incrementWidth;
        int overscan = (_mode & 0x10) != 0 ? 0 : _palette & 0xf;
        Byte* rgbi = &_rgbi[0];
        int phaseRow = _phase << 6;
        int phaseRowFlip =
            (_data->getDataByte(CGAData::registerHorizontalTotal) & 1) << 6;
        for (int y = 0;; ++y) {
            *rgbi = overscan;
            if (y == size.y)
                break;
            int bank = y & 1;
            if ((_mode & 2) == 0)
                bank = 0;
            Array<Byte> rowData = _data->getData(row*bytesPerRow, bytesPerRow);
            memcpy(&_rowData[1], &rowData[0], bytesPerRow);
            Byte* p = &_rowData[1];
            int phase = phaseRow;
            for (int x = 0; x < size.x; x += _hdotsPerChar) {
                UInt64 rgbis = _sequencer->process(
                    p[0] + (p[1] << 8) + (p[-1] << 24), _mode + phase,
                    _palette, scanline, false, 0);
                for (int xx = 0; xx < _hdotsPerChar; ++xx) {
                    *rgbi = (rgbis >> (xx * 4)) & 0xf;
                    ++rgbi;
                }
                phase ^= 0x40;
            }
            ++scanlineIteration;
            if (scanlineIteration == _scanlinesPerRow) {
                scanlineIteration = 0;
                ++scanline;
                if (scanline == _scanlinesPerRow) {
                    scanline = 0;
                    ++row;
                    phaseRow ^= phaseRowFlip;
                }
            }
        }

        _decoderLength = 128;
        int padding = 0;
        if (_connector != 0)
            padding = 32;
        _srgbStride = 2*padding + size.x + 2*blockLap;
        _srgb.ensure(size.y*_srgbStride);
        _ntscStride = size.x + 64;
        _weights.ensure(_compareWidth);
        if (_connector == 0) {
            for (int i = 0; i < _compareWidth; ++i)
                _weights[i] = 1;
        }
        else {
            _ntsc.ensure(64 + size.y*_ntscStride);
            for (int x = 0; x < 63; ++x)
                _ntsc[x] = _composite.simulateCGA(overscan, overscan, x & 3);
            for (int y = 0; y < size.y; ++y) {
                for (int x = 0; x < size.x + 1; ++x) {
                    _ntsc[x + 63 + y*_ntscStride] = _composite.simulateCGA(
                        _rgbi[x + y*_rgbiStride], _rgbi[x + 1 + y*_rgbiStride],
                        (x - 1) & 3);
                }
                for (int x = 0; x < 63; ++x)
                    _ntsc[x + (y + 1)*_ntscStride] = _ntsc[x & 3];
            }

            _decoder.setLength(_decoderLength, _compareWidth);
            _decoder.setPadding(64 - blockLap);
            _decoder.setSaturation(0);
            _decoder.calculateBurst(burst);

            int weightWidth = _incrementWidth; // _rgbiWidth;
            _ntscPattern.ensure(_decoderLength);
            for (int x = 0; x < _decoderLength; ++x)
                _ntscPattern[x] = _composite.simulateCGA(0, 0, x & 3);
            _srgbPattern.ensure(_compareWidth);
            _decoder.decodeNTSC(&_ntscPattern[0], &_srgbPattern[0]);
            for (int x = 0; x < _compareWidth; ++x)
                _weights[x] = static_cast<float>(-_srgbPattern[x].x);

            _ntscPattern[63] = _composite.simulateCGA(0, 15, 3);
            for (int x = 0; x < weightWidth - 1; ++x)
                _ntscPattern[x + 64] = _composite.simulateCGA(15, 15, x & 3);
            _ntscPattern[weightWidth + 63] =
                _composite.simulateCGA(15, 0, 3);
            _decoder.decodeNTSC(&_ntscPattern[0], &_srgbPattern[0]);
            for (int x = 0; x < _compareWidth; ++x)
                _weights[x] += static_cast<float>(_srgbPattern[x].x);

            _decoder.setSaturation(saturation);
            _decoder.calculateBurst(burst);
        }

        const Byte* inputRow = _scaled.data();
        Colour* errorRow = &_error[_errorStride + 1];
        Byte* rgbiRow = &_rgbi[1];
        Byte* ntscRow = &_ntsc[0];
        SRGB* srgbRow = &_srgb[0];
        phaseRow = _phase << 6;
        int bankShift =
            _data->getDataByte(CGAData::registerLogCharactersPerBank) + 1;
        int bank = 0;
        row = 0;

        // Perform matching
        while (!cancelling()) {
            if ((mode1 & 0x102) == 0x102) {
                // Graphics mode with more than 2 scanlines per row
                Array<Byte> rowData = _data->getData(row*bytesPerRow,
                    bytesPerRow);
                memcpy(&_rowData[1], &rowData[0], bytesPerRow);
                rowData = _data->getData(row*bytesPerRow + (1 << bankShift),
                    bytesPerRow);
                memcpy(&_rowData[1 + rowDataStride], &rowData[0], bytesPerRow);
            }
            else {
                // Text mode (bank == 0) and graphics modes with 2 scanlines
                // per row or fewer.
                Array<Byte> rowData = _data->getData(
                    row*bytesPerRow + (bank << bankShift), bytesPerRow);
                memcpy(&_rowData[1], &rowData[0], bytesPerRow);
            }

            _d0 = &_rowData[1];
            _d1 = &_rowData[rowDataStride + 1];
            _inputBlock = inputRow;
            _errorBlock = errorRow;
            _rgbiBlock = rgbiRow;
            _ntscBlock = ntscRow;
            _srgbBlock = srgbRow;
            _phaseBlock = phaseRow;
            for (int column = 0; column < horizontalBlocks; ++column) {
                _mode2 = mode1 + (column << 9 & 0x200);
                int bestPattern = 0;
                float bestMetric = std::numeric_limits<float>::max();
                Colour rgb(0, 0, 0);
                const Byte* inputLine = _inputBlock;
                Colour* errorLine = _errorBlock;

                // Compute average target colour for block to look up in table.
                for (int scanline = 0; scanline < _blockHeight; ++scanline) {
                    auto input = reinterpret_cast<const Colour*>(inputLine);
                    Colour* error = errorLine;
                    for (int x = 0; x < _rgbiWidth; ++x) {
                        Colour target = *input - _diffusionHorizontal*error[-1]
                            - _diffusionVertical*error[-_errorStride];
                        target.x = clamp(0.0f, target.x, 1.0f);
                        target.y = clamp(0.0f, target.y, 1.0f);
                        target.z = clamp(0.0f, target.z, 1.0f);
                        rgb += target;
                        *error = Colour(0, 0, 0);
                        ++input;
                        ++error;
                    }
                    inputLine += _scaled.stride();
                    errorLine += _errorStride;
                }
                SRGB srgb = _linearizer.srgb(rgb/blockArea);
                auto s = Vector3Cast<int>(
                    Vector3Cast<float>(srgb)*srgbScale - 0.5f);
                // Iterate through closest patterns to find the best match.
                for (int z = 0;; ++z) {
                    bool foundPatterns = false;
                    // Always search at least a 2x2x2 region of the gamut in
                    // case we're on the boundary between two entries on any
                    // given access.
                    int rMin = max(s.x - z, 0);
                    int rMax = min(s.x + 1 + z, srgbDiv.x - 1);
                    int gMin = max(s.y - z, 0);
                    int gMax = min(s.y + 1 + z, srgbDiv.y - 1);
                    int bMin = max(s.z - z, 0);
                    int bMax = min(s.z + 1 + z, srgbDiv.z - 1);
                    for (int r = rMin; r <= rMax; ++r) {
                        for (int g = gMin; g <= gMax; ++g) {
                            for (int b = bMin; b <= bMax; ++b) {
                                Word* patterns;
                                int n = _table.get(r +
                                    srgbDiv.x*(g + srgbDiv.y*b), &patterns);
                                for (int i = 0; i < n; ++i) {
                                    int pattern = *patterns;
                                    foundPatterns = true;
                                    float metric = tryPattern(pattern);
                                    if (metric < bestMetric) {
                                        bestPattern = pattern;
                                        bestMetric = metric;
                                    }
                                    ++patterns;
                                }
                                if (r > rMin && r < rMax && g > gMin &&
                                    g < gMax)
                                    b += z*2;
                            }
                        }
                    }
                    if (foundPatterns)
                        break;
                }
                tryPattern(bestPattern);

                switch (_mode2) {
                    // -HRES+GRPH
                    case 0x002:
                    case 0x012:
                        *_d0 = (*_d0 & 0xf) + (bestPattern << 4);
                        break;
                    case 0x202:
                    case 0x212:
                        *_d0 = (*_d0 & 0xf0) + (bestPattern & 0xf);
                        ++_d0;
                        break;
                    case 0x402:
                    case 0x412:
                        *_d0 = (*_d0 & 0xf) + (bestPattern & 0xf0);
                        break;
                    case 0x602:
                    case 0x612:
                        *_d0 = (*_d0 & 0xf0) + ((bestPattern >> 4) & 0xf);
                        ++_d0;
                        break;
                    case 0x102:
                        case 0x112:
                        *_d0 = (*_d0 & 0xf) + (bestPattern << 4);
                        *_d1 = (*_d1 & 0xf) + (bestPattern & 0xf0);
                        break;
                    case 0x302:
                    case 0x312:
                        *_d0 = (*_d0 & 0xf0) + (bestPattern & 0xf);
                        *_d1 = (*_d1 & 0xf0) + (bestPattern >> 4);
                        ++_d0;
                        ++_d1;
                        break;
                    case 0x502:
                    case 0x512:
                        *_d0 = (*_d0 & 0xf) + (bestPattern & 0xf0);
                        *_d1 = (*_d1 & 0xf) + ((bestPattern >> 8) & 0xf0);
                        break;
                    case 0x702:
                    case 0x712:
                        *_d0 = (*_d0 & 0xf0) + ((bestPattern >> 4) & 0xf);
                        *_d1 = (*_d1 & 0xf0) + ((bestPattern >> 12) & 0xf);
                        ++_d0;
                        ++_d1;
                        break;

                    // +HRES+GRPH
                    case 0x003:
                    case 0x203:
                    case 0x403:
                    case 0x603:
                        *_d0 = bestPattern;
                        ++_d0;
                        _phaseBlock ^= 0x40;
                        break;
                    case 0x013:
                    case 0x213:
                    case 0x413:
                    case 0x613:
                        *_d0 = _hres1bpp[bestPattern];
                        ++_d0;
                        _phaseBlock ^= 0x40;
                        break;
                    case 0x103:
                    case 0x303:
                    case 0x503:
                    case 0x703:
                        *_d0 = bestPattern;
                        *_d1 = bestPattern >> 8;
                        ++_d0;
                        ++_d1;
                        _phaseBlock ^= 0x40;
                        break;
                    case 0x113:
                    case 0x313:
                    case 0x513:
                    case 0x713:
                        *_d0 = _hres1bpp[bestPattern & 0xf];
                        *_d1 = _hres1bpp[bestPattern >> 4];
                        ++_d0;
                        ++_d1;
                        _phaseBlock ^= 0x40;
                        break;

                    // -GRPH
                    default:
                        *_d0 = bestPattern;
                        _d0[1] = bestPattern >> 8;
                        _d0 += 2;
                }
                _inputBlock += _incrementWidth*3*sizeof(float);
                _errorBlock += _incrementWidth;
                _rgbiBlock += _incrementWidth;
                _ntscBlock += _incrementWidth;
                _srgbBlock += _incrementWidth;
            } // column

            if ((mode1 & 0x102) == 0x102) {
                // Graphics mode with more than 2 scanlines per row
                _data->change(0, row*bytesPerRow, bytesPerRow, &_rowData[1]);
                _data->change(0, row*bytesPerRow + (1 << bankShift),
                    bytesPerRow, &_rowData[1 + rowDataStride]);
                ++bank;
            }
            else {
                // Text mode (bank == 0) and graphics modes with 2 scanlines
                // per row or fewer.
                _data->change(0, row*bytesPerRow + (bank << bankShift),
                    bytesPerRow, &_rowData[1]);
            }
            _program->updateOutput();
            _program->setProgress(static_cast<float>(row*banks + bank)/
                (_verticalDisplayed*banks));

            ++bank;
            inputRow += _blockHeight*_scaled.stride();
            errorRow += _errorStride*_blockHeight;
            phaseRow ^= phaseRowFlip;
            rgbiRow += _rgbiStride*_blockHeight;
            ntscRow += _ntscStride*_blockHeight;
            srgbRow += _srgbStride*_blockHeight;
            if (bank == banks) {
                bank = 0;
                ++row;
                if (row >= _verticalDisplayed)
                    break;
            }
        } // while (!cancelling())
        _program->setProgress(-1);
    }

    void setDiffusionHorizontal(double diffusionHorizontal)
    {
        _diffusionHorizontal = static_cast<float>(diffusionHorizontal);
    }
    double getDiffusionHorizontal() { return _diffusionHorizontal; }
    void setDiffusionVertical(double diffusionVertical)
    {
        _diffusionVertical = static_cast<float>(diffusionVertical);
    }
    double getDiffusionVertical() { return _diffusionVertical; }
    void setDiffusionTemporal(double diffusionTemporal)
    {
        _diffusionTemporal = static_cast<float>(diffusionTemporal);
    }
    double getDiffusionTemporal() { return _diffusionTemporal; }
    void setMode(int mode) { _mode = mode; initData(); }
    int getMode() { return _mode; }
    void setPalette(int palette) { _palette = palette; initData(); }
    int getPalette() { return _palette; }
    void setScanlinesPerRow(int v) { _scanlinesPerRow = v; initData(); }
    int getScanlinesPerRow() { return _scanlinesPerRow; }
    void setScanlinesRepeat(int v) { _scanlinesRepeat = v; initData(); }
    int getScanlinesRepeat() { return _scanlinesRepeat; }
    void setPhase(int phase) { _phase = phase; initData(); }
    int getPhase() { return _phase; }
    void setInterlace(int interlace) { _interlace = interlace; initData(); }
    int getInterlace() { return _interlace; }
    void setInterlaceSync(bool interlaceSync)
    {
        _interlaceSync = interlaceSync;
        initData();
    }
    bool getInterlaceSync() { return _interlaceSync; }
    void setInterlacePhase(bool interlacePhase)
    {
        _interlacePhase = interlacePhase;
        initData();
    }
    bool getInterlacePhase() { return _interlacePhase; }
    void setFlicker(bool flicker) { _flicker = flicker; initData(); }
    bool getFlicker() { return _flicker; }
    void setQuality(double quality) { _quality = quality; }
    double getQuality() { return _quality; }
    void setGamma(double gamma) { _gamma = gamma; _size = Vector(0, 0); }
    double getGamma() { return _gamma; }
    void setClipping(int clipping) { _clipping = clipping; }
    int getClipping() { return _clipping; }
    void setMetric(int metric) { _metric = metric; }
    int getMetric() { return _metric; }
    void setCharacterSet(int characterSet) { _characterSet = characterSet; }
    int getCharacterSet() { return _characterSet; }
    double getHue() { return _hue; }
    void setHue(double hue) { _hue = hue; }
    double getSaturation() { return _saturation; }
    void setSaturation(double saturation) { _saturation = saturation; }
    double getContrast() { return _contrast; }
    void setContrast(double contrast) { _contrast = contrast; }
    double getBrightness() { return _brightness; }
    void setBrightness(double brightness) { _brightness = brightness; }
    void setConnector(int connector) { _connector = connector; }
    void setChromaBandwidth(double chromaBandwidth)
    {
        _chromaBandwidth = chromaBandwidth;
    }
    double getChromaBandwidth() { return _chromaBandwidth; }
    void setLumaBandwidth(double lumaBandwidth)
    {
        _lumaBandwidth = lumaBandwidth;
    }
    double getLumaBandwidth() { return _lumaBandwidth; }
    void setRollOff(double rollOff) { _rollOff = rollOff; }
    double getRollOff() { return _rollOff; }
    void setLobes(double lobes) { _lobes = lobes; }
    double getLobes() { return _lobes; }
    void setPrescalerProfile(int profile)
    {
        _prescalerProfile = profile;
        _size = Vector(0, 0);
    }
    int getPrescalerProfile() { return _prescalerProfile; }

private:
    float tryPattern(int pattern)
    {
        float metric = 0;
        UInt32 v[2];
        int yMask = (_mode2 & 0x102) == 0x102 ? 1 : 0;
        switch (_mode2) {
            // -HRES+GRPH
            case 0x002:
            case 0x012:
            case 0x202:
            case 0x212:
                v[0] = pattern << 4;
                break;
            case 0x402:
            case 0x412:
            case 0x602:
            case 0x612:
                v[0] = pattern;
                break;
            case 0x102:
            case 0x112:
            case 0x302:
            case 0x312:
                v[0] = pattern << 4;
                v[1] = pattern & 0xf0;
                break;
            case 0x502:
            case 0x512:
            case 0x702:
            case 0x712:
                v[0] = pattern;
                v[1] = pattern >> 8;
                break;

            // +HRES+GRPH
            case 0x103:
            case 0x503:
                v[0] = (_d0[1] << 8) + pattern;
                v[1] = (_d1[1] << 8) + (pattern >> 8);
                break;
            case 0x303:
            case 0x703:
                v[0] = _d0[1] + (pattern << 8);
                v[1] = _d1[1] + (pattern & 0xff00);
                break;
            case 0x113:
            case 0x513:
                v[0] = (_d0[1] << 8) + _hres1bpp[pattern & 0xf];
                v[1] = (_d1[1] << 8) + _hres1bpp[pattern >> 4];
                break;
            case 0x313:
            case 0x713:
                v[0] = _d0[1] + (_hres1bpp[pattern & 0xf] << 8);
                v[1] = _d1[1] + (_hres1bpp[pattern >> 4] << 8);
                break;

            case 0x003:
            case 0x403:
                v[0] = (_d0[1] << 8) + pattern;
                break;
            case 0x203:
            case 0x603:
                v[0] = _d0[1] + (pattern << 8);
                break;
            case 0x013:
            case 0x413:
                v[0] = (_d0[1] << 8) + _hres1bpp[pattern];
                break;
            case 0x213:
            case 0x613:
                v[0] = _d0[1] + (_hres1bpp[pattern] << 8);
                break;
            default:
                v[0] = pattern;
        }
        v[0] = (v[0] & 0xffff) + (_d0[-1] << 24);
        v[1] = (v[1] & 0xffff) + (_d1[-1] << 24);
        const Byte* inputLine = _inputBlock;
        Colour* errorLine = _errorBlock;
        Byte* rgbiLine = _rgbiBlock;
        Byte* ntscLine = _ntscBlock;
        SRGB* srgbLine = _srgbBlock;
        for (int scanline = 0; scanline < _blockHeight; ++scanline) {
            int s = scanline / _scanlinesRepeat;
            UInt64 rgbis = _sequencer->process(v[s & yMask],
                _mode + _phaseBlock, _palette, s, false, 0);
            SRGB* srgb = srgbLine;
            auto input = reinterpret_cast<const Colour*>(inputLine);
            auto error = errorLine;
            if (_connector == 0) {
                for (int x = 0; x < _rgbiWidth; ++x) {
                    Byte* p = &_rgbiPalette[3*((rgbis >> (x * 4)) & 0xf)];
                    *srgb = SRGB(p[0], p[1], p[2]);
                    ++srgb;
                }
            }
            else {
                Byte* rgbi = rgbiLine;
                int x;
                for (x = 0; x < _rgbiWidth; ++x)
                    rgbi[x] = ((rgbis >> (x * 4)) & 0xf);
                Byte* ntsc = ntscLine + 64;
                for (x = -1; x < _rgbiWidth; ++x) {
                    ntsc[x] = _composite.simulateCGA(rgbi[x], rgbi[x + 1],
                        x & 3);
                }
                _decoder.decodeNTSC(ntscLine, srgb);
            }
            srgb = srgbLine;
            for (int x = 0; x < _compareWidth; ++x) {
                SRGB o = *srgb;
                Colour output = _linearizer.linear(o);
                Colour target = *input - _diffusionHorizontal*error[-1] -
                    _diffusionVertical*error[-_errorStride];
                switch (_clipping) {
                    case 1:
                        target.x = clamp(0.0f, target.x, 1.0f);
                        target.y = clamp(0.0f, target.y, 1.0f);
                        target.z = clamp(0.0f, target.z, 1.0f);
                        break;
                    case 2:
                        if (target.x < 0.0f) {
                            float scale = 0.5f/(0.5f - target.x);
                            target.x = 0.0f;
                            target.y = 0.5f + (target.y - 0.5f)*scale;
                            target.z = 0.5f + (target.z - 0.5f)*scale;
                        }
                        if (target.x > 1.0f) {
                            float scale = 0.5f/(target.x - 0.5f);
                            target.x = 1.0f;
                            target.y = 0.5f + (target.y - 0.5f)*scale;
                            target.z = 0.5f + (target.z - 0.5f)*scale;
                        }
                        if (target.y < 0.0f) {
                            float scale = 0.5f/(0.5f - target.y);
                            target.x = 0.5f + (target.x - 0.5f)*scale;
                            target.y = 0.0f;
                            target.z = 0.5f + (target.z - 0.5f)*scale;
                        }
                        if (target.y > 1.0f) {
                            float scale = 0.5f/(target.y - 0.5f);
                            target.x = 0.5f + (target.x - 0.5f)*scale;
                            target.y = 1.0f;
                            target.z = 0.5f + (target.z - 0.5f)*scale;
                        }
                        if (target.z < 0.0f) {
                            float scale = 0.5f/(0.5f - target.z);
                            target.x = 0.5f + (target.x - 0.5f)*scale;
                            target.y = 0.5f + (target.y - 0.5f)*scale;
                            target.z = 0.0f;
                        }
                        if (target.z > 1.0f) {
                            float scale = 0.5f/(target.z - 0.5f);
                            target.x = 0.5f + (target.x - 0.5f)*scale;
                            target.y = 0.5f + (target.y - 0.5f)*scale;
                            target.z = 1.0f;
                        }
                        break;
                    case 3:
                        target.x = clamp(-1.0f, target.x, 2.0f);
                        target.y = clamp(-1.0f, target.y, 2.0f);
                        target.z = clamp(-1.0f, target.z, 2.0f);
                        break;
                }

                Colour e = output - target;
                *error = e;

                switch (_metric) {
                    case 1:
                        metric += e.modulus2();
                        break;
                    case 0:
                    case 2:
                        {
                            SRGB t = _linearizer.srgb(target);
                            float dr = static_cast<float>(o.x - t.x);
                            float dg = static_cast<float>(o.y - t.y);
                            float db = static_cast<float>(o.z - t.z);
                            if (_metric == 0)
                                metric += dr*dr + dg*dg + db*db;
                            else {
                                // Fast colour distance metric from
                                // http://www.compuphase.com/cmetric.htm .
                                float mr = (o.x + t.x)/512.0f;
                                metric += _weights[x]*(4.0f*dg*dg +
                                    (2.0f + mr)*dr*dr + (3.0f - mr)*db*db);
                            }
                        }
                        break;
                    case 3:
                        metric += deltaE2Luv(output, target);
                        break;
                    case 4:
                        metric += deltaE2CIE76(output, target);
                        break;
                    case 5:
                        metric += deltaE2CIE94(output, target);
                        break;
                    case 6:
                        metric += deltaE2CIEDE2000(output, target);
                        break;
                }

                ++input;
                ++error;
                ++srgb;
            }
            inputLine += _scaled.stride();
            errorLine += _errorStride;
            ntscLine += _ntscStride;
            rgbiLine += _rgbiStride;
            srgbLine += _srgbStride;
        }
        return metric;
    }
    void initData()
    {
        if (!_active)
            return;
        static const int regs = -CGAData::registerLogCharactersPerBank;
        Byte cgaRegistersData[regs] = { 0 };
        Byte* cgaRegisters = &cgaRegistersData[regs];
        _hdotsPerChar = (_mode & 1) != 0 ? 8 : 16;
        _horizontalDisplayed =
            (_activeSize.x + _hdotsPerChar - 1)/_hdotsPerChar;
        int scanlinesPerRow = _scanlinesPerRow*_scanlinesRepeat;
        _verticalDisplayed =
            (_activeSize.y + scanlinesPerRow - 1)/scanlinesPerRow;
        _logCharactersPerBank = 0;
        while ((1 << _logCharactersPerBank) <
            _horizontalDisplayed*_verticalDisplayed)
            ++_logCharactersPerBank;
        int horizontalTotal = _horizontalDisplayed + 272/_hdotsPerChar;
        int horizontalSyncPosition = _horizontalDisplayed + 80/_hdotsPerChar;
        int totalScanlines = _activeSize.y + 62;
        int verticalTotal = totalScanlines/scanlinesPerRow;
        int verticalTotalAdjust =
            totalScanlines - verticalTotal*scanlinesPerRow;
        if (verticalTotal > 128 &&
            verticalTotal < (32 - verticalTotalAdjust)/scanlinesPerRow + 128) {
            verticalTotalAdjust += (verticalTotal - 128)*scanlinesPerRow;
            verticalTotal = 128;
        }
        int verticalSyncPosition = _verticalDisplayed + 24/scanlinesPerRow;
        int hdotsPerScanline = horizontalTotal*_hdotsPerChar;
        cgaRegisters[CGAData::registerLogCharactersPerBank] =
            _logCharactersPerBank;
        cgaRegisters[CGAData::registerScanlinesRepeat] = _scanlinesRepeat;
        cgaRegisters[CGAData::registerHorizontalTotalHigh] =
            (horizontalTotal - 1) >> 8;
        cgaRegisters[CGAData::registerHorizontalDisplayedHigh] =
            _horizontalDisplayed >> 8;
        cgaRegisters[CGAData::registerHorizontalSyncPositionHigh] =
            horizontalSyncPosition >> 8;
        cgaRegisters[CGAData::registerVerticalTotalHigh] =
            (verticalTotal - 1) >> 8;
        cgaRegisters[CGAData::registerVerticalDisplayedHigh] =
            _verticalDisplayed >> 8;
        cgaRegisters[CGAData::registerVerticalSyncPositionHigh] =
            verticalSyncPosition >> 8;
        cgaRegisters[CGAData::registerMode] = _mode;
        cgaRegisters[CGAData::registerPalette] = _palette;
        cgaRegisters[CGAData::registerHorizontalTotal] =
            (horizontalTotal - 1) & 0xff;
        cgaRegisters[CGAData::registerHorizontalDisplayed] =
            _horizontalDisplayed & 0xff;
        cgaRegisters[CGAData::registerHorizontalSyncPosition] =
            horizontalSyncPosition & 0xff;
        cgaRegisters[CGAData::registerHorizontalSyncWidth] = 10;
        cgaRegisters[CGAData::registerVerticalTotal] =
            (verticalTotal - 1) & 0xff;
        cgaRegisters[CGAData::registerVerticalTotalAdjust] =
            verticalTotalAdjust;
        cgaRegisters[CGAData::registerVerticalDisplayed] =
            _verticalDisplayed & 0xff;
        cgaRegisters[CGAData::registerVerticalSyncPosition] =
            verticalSyncPosition & 0xff;
        cgaRegisters[CGAData::registerInterlaceMode] = 2;
        cgaRegisters[CGAData::registerMaximumScanline] = _scanlinesPerRow - 1;
        cgaRegisters[CGAData::registerCursorStart] = 6;
        cgaRegisters[CGAData::registerCursorEnd] = 7;
        _data->change(0, -regs, regs, &cgaRegistersData[0]);
        int last = _horizontalDisplayed*_verticalDisplayed*2 - 1;
        if ((_mode & 2) != 0)
            last += 2 << _logCharactersPerBank;
        _data->change(0, last, 0);
        _data->setTotals(hdotsPerScanline*totalScanlines, hdotsPerScanline - 2,
            static_cast<int>((hdotsPerScanline - 2)*(totalScanlines + 0.5)));
    }

    MatcherTable _table;
    Program* _program;
    CGAData* _data;
    CGASequencer* _sequencer;
    NTSCDecoder _decoder;
    //MatchingNTSCDecoder _decoder;
    CGAComposite _composite;
    Linearizer _linearizer;

    int _phase;
    int _mode;
    int _palette;
    int _scanlinesPerRow;
    int _scanlinesRepeat;
    int _connector;
    float _diffusionHorizontal;
    float _diffusionVertical;
    float _diffusionTemporal;
    int _interlace;
    bool _interlaceSync;
    bool _interlacePhase;
    bool _flicker;
    double _quality;
    double _gamma;
    int _clipping;
    int _metric;
    int _characterSet;
    double _hue;
    double _saturation;
    double _contrast;
    double _brightness;
    double _chromaBandwidth;
    double _lumaBandwidth;
    double _rollOff;
    double _lobes;
    int _prescalerProfile;

    bool _active;
    Vector _size;
    Vector _activeSize;
    ScanlineRenderer _scaler;
    AlignedBuffer _scaled;
    int _horizontalDisplayed;
    int _verticalDisplayed;
    int _hdotsPerChar;
    int _logCharactersPerBank;

    Array<Byte> _rgbiPalette;
    Array<bool> _skip;

    Array<Byte> _ntscPattern;
    Array<SRGB> _srgbPattern;
    Array<Byte> _rowData;
    Array<Byte> _rgbi;
    Array<Byte> _ntsc;
    Array<SRGB> _srgb;
    Bitmap<SRGB> _input;
    Array<Colour> _output;
    Array<Colour> _error;

    int _mode2;
    Byte* _d0;
    Byte* _d1;
    UInt32 _hres1bpp[0x10];
    const Byte* _inputBlock;
    Colour* _errorBlock;
    Byte* _rgbiBlock;
    Byte* _ntscBlock;
    SRGB* _srgbBlock;
    int _rgbiWidth;
    int _compareWidth;
    int _incrementWidth;
    int _blockHeight;
    int _phaseBlock;
    int _decoderLength;
    int _errorStride;
    int _ntscStride;
    int _rgbiStride;
    int _srgbStride;
    Array<float> _weights;
};

typedef CGAMatcherT<void> CGAMatcher;

template<class T> class CGAOutputT : public ThreadTask
{
public:
    CGAOutputT(CGAData* data, CGASequencer* sequencer, CGAArtWindow* window)
      : _data(data), _sequencer(sequencer), _window(window), _zoom(0),
        _aspectRatio(1), _inputTL(0, 0), _outputSize(0, 0), _active(false)
    { }
    void run()
    {
        int connector;
        Vector outputSize;
        int combFilter;
        bool showClipping;
        float brightness;
        float contrast;
        Vector2<float> inputTL;
        float overscan;
        double zoom;
        double aspectRatio;
        static const int decoderPadding = 32;
        Vector2<float> zoomVector;
        {
            Lock lock(&_mutex);
            if (!_active)
                return;

            int total = _data->getTotal();
            _rgbi.ensure(total);
            _data->output(0, total, &_rgbi[0], _sequencer, _phase);

            connector = _connector;
            combFilter = _combFilter;
            showClipping = _showClipping;
            outputSize = _outputSize;
            inputTL = _inputTL;
            overscan = static_cast<float>(_overscan);
            zoom = _zoom;
            aspectRatio = _aspectRatio;
            Byte mode = _data->getDataByte(CGAData::registerMode);
            _composite.setBW((mode & 4) != 0);
            bool newCGA = connector == 2;
            _composite.setNewCGA(newCGA);
            _composite.initChroma();
            double black = _composite.black();
            double white = _composite.white();
            _decoder.setHue(_hue + ((mode & 1) != 0 ? 14 : 4));
            _decoder.setSaturation(_saturation*1.45*(newCGA ? 1.5 : 1.0)/100);
            contrast = static_cast<float>(_contrast/100);
            static const double combDivisors[3] = {1, 2, 4};
            double c = _contrast*256*(newCGA ? 1.2 : 1)/((white - black)*100);
            _decoder.setContrast(c/combDivisors[combFilter]);
            brightness = static_cast<float>(_brightness/100);
            _decoder.setBrightness((-black*c +
                _brightness*5 + (newCGA ? -50 : 0))/256.0);
            _decoder.setChromaBandwidth(_chromaBandwidth);
            _decoder.setLumaBandwidth(_lumaBandwidth);
            _decoder.setRollOff(_rollOff);
            _decoder.setLobes(_lobes);
            _decoder.setChromaNotch(combFilter == 0);
            _scaler.setProfile(_scanlineProfile);
            _scaler.setHorizontalProfile(_horizontalProfile);
            _scaler.setWidth(static_cast<float>(_scanlineWidth));
            _scaler.setBleeding(_scanlineBleeding);
            _scaler.setHorizontalBleeding(_horizontalBleeding);
            _scaler.setHorizontalRollOff(
                static_cast<float>(_horizontalRollOff));
            _scaler.setVerticalRollOff(static_cast<float>(_verticalRollOff));
            _scaler.setHorizontalLobes(static_cast<float>(_horizontalLobes));
            _scaler.setVerticalLobes(static_cast<float>(_verticalLobes));
            _scaler.setSubPixelSeparation(
                static_cast<float>(_subPixelSeparation));
            _scaler.setPhosphor(_phosphor);
            _scaler.setMask(_mask);
            _scaler.setMaskSize(static_cast<float>(_maskSize));
            zoomVector = scale();
            _scaler.setZoom(zoomVector);
        }

        int srgbSize = _data->getTotal();
        _srgb.ensure(srgbSize*3);
        int pllWidth = _data->getPLLWidth();
        int pllHeight = _data->getPLLHeight();
        static const int driftHorizontal = 8;
        static const int driftVertical = 14*pllWidth;
        _scanlines.clear();
        _fields.clear();
        _fieldOffsets.clear();

        Byte hSync = 0x41;
        Byte vSync = 0x42;
        if (connector != 0) {
            hSync = 0x44;
            vSync = 0x44;
        }
        int lastScanline = 0;
        int i;
        Vector2<float> activeSize(0, 0);
        do {
            int offset = (lastScanline + pllWidth - driftHorizontal) %
                srgbSize;
            Byte* p = &_rgbi[offset];
            int n = driftHorizontal*2;
            if (offset + n <= srgbSize) {
                for (i = 0; i < n; ++i) {
                    if ((p[i] & hSync) == hSync)
                        break;
                }
            }
            else {
                for (i = 0; i < n; ++i) {
                    if ((_rgbi[(offset + i) % srgbSize] & hSync) == hSync)
                        break;
                }
            }
            i = (i + offset) % srgbSize;
            activeSize.x = max(activeSize.x,
                static_cast<float>(wrap(i - lastScanline, srgbSize)));
            if ((_rgbi[i] & 0x80) != 0)
                break;
            _rgbi[i] |= 0x80;
            _scanlines.append(i);
            lastScanline = i;
        } while (true);
        int firstScanline = _scanlines.count() - 1;
        for (; firstScanline > 0; --firstScanline) {
            if (_scanlines[firstScanline] == i)
                break;
        }
        int scanlines = _scanlines.count() - firstScanline;

        for (auto& s : _scanlines)
            _rgbi[s] &= ~0x80;
        int lastField = 0;
        do {
            int offset = (lastField + pllHeight - driftVertical) %
                srgbSize;
            Byte* p = &_rgbi[offset];
            int n = driftVertical*2;
            int j;
            int s = 0;
            if (offset + n <= srgbSize) {
                for (j = 0; j < n; j += 57) {
                    if ((p[j] & vSync) == vSync) {
                        ++s;
                        if (s == 3)
                            break;
                    }
                    else
                        s = 0;
                }
            }
            else {
                for (j = 0; j < n; j += 57) {
                    if ((_rgbi[(offset + j) % srgbSize] & vSync) == vSync) {
                        ++s;
                        if (s == 3)
                            break;
                    }
                    else
                        s = 0;
                }
            }
            j = (j + offset) % srgbSize;
            lastField = j;
            int s0;
            int s1;
            float fieldOffset;
            for (i = 0; i < scanlines; ++i) {
                s0 = _scanlines[
                    (firstScanline + scanlines + i - 1) % scanlines];
                if (s0 < 0)
                    s0 = -1 - s0;
                s1 = _scanlines[firstScanline + i];
                if (s1 < 0)
                    s1 = -1 - s1;
                if (s0 < s1) {
                    if (j >= s0 && j < s1) {
                        fieldOffset = static_cast<float>(j - s0) / (s1 - s0);
                        break;
                    }
                }
                else {
                    if (j >= s0) {
                        fieldOffset =
                            static_cast<float>(j - s0) / (s1 + srgbSize - s0);
                        break;
                    }
                    else {
                        if (j < s1) {
                            fieldOffset = static_cast<float>(j + srgbSize - s0)
                                / (s1 + srgbSize - s0);
                            break;
                        }
                    }
                }
            }
            int fo = static_cast<int>(fieldOffset * 8 + 0.5);
            if (fo == 8) {
                fo = 0;
                i = (i + 1) % scanlines;
            }
            float f = fo/8.0f;
            int c = _fields.count() - 1;
            if (c >= 0) {
                int iLast = _fields[c];
                float fLast = _fieldOffsets[c];
                int lines = (i - iLast + scanlines - 1) % scanlines + 1;
                activeSize.y = max(activeSize.y,
                    static_cast<float>(lines) + f - fLast);
            }
            if (_scanlines[firstScanline + i] < 0)
                break;
            _fields.append(i);
            _fieldOffsets.append(f);
            _scanlines[firstScanline + i] = -1 - _scanlines[firstScanline + i];
        } while (true);
        int firstField = _fields.count() - 1;
        for (; firstField > 0; --firstField) {
            if (_fields[firstField] == i)
                break;
        }
        for (auto& f : _fields)
            _scanlines[firstScanline + f] = -1 - _scanlines[firstScanline + f];

        // Assume standard overscan/blank/sync areas
        activeSize -= Vector2<float>(272, 62);

        if (outputSize.zeroArea()) {
            inputTL = Vector2<float>(160.5f, 38) - overscan*activeSize;
            double o = 1 + 2*overscan;
            double y = zoom*activeSize.y*o;
            double x = zoom*activeSize.x*o*aspectRatio/2;
            outputSize = Vector(static_cast<int>(x + 0.5),
                static_cast<int>(y + 0.5));

            Lock lock(&_mutex);
            _inputTL = inputTL;
            _outputSize = outputSize;
        }
        Vector2<float> offset(0, 0);
        if (connector != 0) {
            offset = Vector2<float>(-decoderPadding - 0.5f, 0) +
                static_cast<float>(combFilter)*Vector2<float>(1, -0.5f);
        }
        _scaler.setOffset(inputTL + offset +
            Vector2<float>(0.5f, 0.5f)/zoomVector);
        _scaler.setOutputSize(outputSize);

        _bitmap.ensure(outputSize);
        _scaler.init();
        _unscaled = _scaler.input();
        _scaled = _scaler.output();
        Vector tl = _scaler.inputTL();
        Vector br = _scaler.inputBR();
        _unscaledSize = br - tl;
        Vector activeTL = Vector(0, 0) - tl;

        if (connector == 0) {
            // Convert from RGBI to 9.7 fixed-point sRGB
            Byte levels[4];
            for (int i = 0; i < 4; ++i) {
                int l = static_cast<int>(255.0f*brightness + 85.0f*i*contrast);
                if (showClipping) {
                    if (l < 0)
                        l = 255;
                    else {
                        if (l > 255)
                            l = 0;
                    }
                }
                else
                    l = clamp(0, l, 255);
                levels[i] = l;
            }
            // On the RGBI connector we don't show composite sync, colour
            // burst, or blanking since these are not output on the actual
            // connector. Blanking is visible as black if the overscan is a
            // non-black colour.
            int palette[3*0x78] = {
                0, 0, 0,  0, 0, 2,  0, 2, 0,  0, 2, 2,
                2, 0, 0,  2, 0, 2,  2, 1, 0,  2, 2, 2,
                1, 1, 1,  1, 1, 3,  1, 3, 1,  1, 3, 3,
                3, 1, 1,  3, 1, 3,  3, 3, 1,  3, 3, 3};
            static int overscanPalette[3*4] = {
                0, 0, 0,  0, 1, 0,  1, 0, 0,  1, 1, 0};
            for (int i = 3*0x10; i < 3*0x78; i += 3*4)
                memcpy(palette + i, overscanPalette, 3*4*sizeof(int));
            Byte srgbPalette[3*0x77];
            for (int i = 0; i < 3*0x77; ++i)
                srgbPalette[i] = levels[palette[i]];
            const Byte* rgbi = &_rgbi[0];
            Byte* srgb = &_srgb[0];
            for (int x = 0; x < srgbSize; ++x) {
                Byte* p = &srgbPalette[3 * *rgbi];
                ++rgbi;
                srgb[0] = p[0];
                srgb[1] = p[1];
                srgb[2] = p[2];
                srgb += 3;
            }
        }
        else {
            _decoder.setPadding(decoderPadding);
            Byte burst[4];
            for (int i = 0; i < 4; ++i)
                burst[i] = _composite.simulateCGA(6, 6, (i + 3) & 3);
            _decoder.calculateBurst(burst);

            int combedSize = srgbSize + 2*decoderPadding;
            Vector combTL = Vector(2, 1)*combFilter;
            int ntscSize = combedSize + combTL.y*pllWidth;
            _ntsc.ensure(ntscSize);
            int rgbiSize = ntscSize + 1;
            if (_rgbi.count() < rgbiSize) {
                Array<Byte> rgbi(rgbiSize);
                memcpy(&rgbi[0], &_rgbi[0], srgbSize);
                _rgbi = rgbi;
            }
            memcpy(&_rgbi[srgbSize], &_rgbi[0], rgbiSize - srgbSize);

            // Convert from RGBI to composite
            const Byte* rgbi = &_rgbi[0];
            Byte* ntsc = &_ntsc[0];
            for (int x = 0; x < ntscSize; ++x) {
                *ntsc = _composite.simulateCGA(*rgbi, rgbi[1], x & 3);
                ++rgbi;
                ++ntsc;
            }
            // Apply comb filter and decode to sRGB.
            ntsc = &_ntsc[0];
            Byte* srgb = &_srgb[0];
            static const int fftLength = 512;
            int stride = fftLength - 2*decoderPadding;
            Byte* ntscBlock = &_ntsc[0];
            switch (combFilter) {
                case 0:
                    // No comb filter
                    for (int j = 0; j < srgbSize; j += stride) {
                        if (j + stride > srgbSize) {
                            // The last block is a small one, so we'll decode
                            // it by overlapping the previous one.
                            j = srgbSize - stride;
                            ntscBlock = &_ntsc[j];
                            srgb = &_srgb[3*j];
                        }
                        _decoder.decodeNTSC(ntscBlock,
                            reinterpret_cast<SRGB*>(srgb));
                        srgb += stride*3;
                        ntscBlock += stride;
                    }
                    break;
                case 1:
                    // 1 line. Standard NTSC comb filters will have a delay of
                    // 227.5 color carrier cycles (1 standard scanline) but a
                    // CGA scanline is 228 color carrier cycles, so instead of
                    // sharpening vertical detail a comb filter applied to CGA
                    // will sharpen 1-ldot-per-scanline diagonals.
                    for (int j = 0; j < srgbSize; j += stride) {
                        if (j + stride > srgbSize) {
                            // The last block is a small one, so we'll decode
                            // it by overlapping the previous one.
                            j = srgbSize - stride;
                            ntscBlock = &_ntsc[j];
                            srgb = &_srgb[3*j];
                        }
                        Byte* n0 = ntscBlock;
                        Byte* n1 = n0 + pllWidth;
                        float* y = _decoder.yData();
                        float* i = _decoder.iData();
                        float* q = _decoder.qData();
                        for (int x = 0; x < fftLength; x += 4) {
                            y[0] = static_cast<float>(n0[0] + n1[0]);
                            y[1] = static_cast<float>(n0[1] + n1[1]);
                            y[2] = static_cast<float>(n0[2] + n1[2]);
                            y[3] = static_cast<float>(n0[3] + n1[3]);
                            i[0] = -static_cast<float>(n0[1] - n1[1]);
                            i[1] = static_cast<float>(n0[3] - n1[3]);
                            q[0] = static_cast<float>(n0[0] - n1[0]);
                            q[1] = -static_cast<float>(n0[2] - n1[2]);
                            n0 += 4;
                            n1 += 4;
                            y += 4;
                            i += 2;
                            q += 2;
                        }
                        _decoder.decodeBlock(reinterpret_cast<SRGB*>(srgb));
                        srgb += stride*3;
                        ntscBlock += stride;
                    }
                    break;
                case 2:
                    // 2 line.
                    for (int j = 0; j < srgbSize; j += stride) {
                        if (j + stride > srgbSize) {
                            // The last block is a small one, so we'll decode
                            // it by overlapping the previous one.
                            j = srgbSize - stride;
                            ntscBlock = &_ntsc[j];
                            srgb = &_srgb[3*j];
                        }
                        Byte* n0 = ntscBlock;
                        Byte* n1 = n0 + pllWidth;
                        Byte* n2 = n1 + pllWidth;
                        float* y = _decoder.yData();
                        float* i = _decoder.iData();
                        float* q = _decoder.qData();
                        for (int x = 0; x < fftLength; x += 4) {
                            y[0] = static_cast<float>(n0[0] + n2[0] + 2*n1[0]);
                            y[1] = static_cast<float>(n0[1] + n2[1] + 2*n1[1]);
                            y[2] = static_cast<float>(n0[2] + n2[2] + 2*n1[2]);
                            y[3] = static_cast<float>(n0[3] + n2[3] + 2*n1[3]);
                            i[0] = static_cast<float>(2*n1[1] - n0[1] - n2[1]);
                            i[1] = static_cast<float>(n0[3] + n2[3] - 2*n1[3]);
                            q[0] = static_cast<float>(n0[0] + n2[0] - 2*n1[0]);
                            q[1] = static_cast<float>(2*n1[2] - n0[2] - n2[2]);
                            n0 += 4;
                            n1 += 4;
                            n2 += 4;
                            y += 4;
                            i += 2;
                            q += 2;
                        }
                        _decoder.decodeBlock(reinterpret_cast<SRGB*>(srgb));
                        srgb += stride*3;
                        ntscBlock += stride;
                    }
                    break;
            }
        }
        // Shift, clip, show clipping and linearization
        _linearizer.setShowClipping(showClipping && _connector != 0);
        tl.y = wrap(tl.y + _fields[firstField], scanlines);
        Byte* unscaledRow = _unscaled.data();
        int scanlineChannels = _unscaledSize.x*3;
        for (int y = 0; y < _unscaledSize.y; ++y) {
            int offsetTL = wrap(
                tl.x + _scanlines[(tl.y + y)%scanlines + firstScanline],
                srgbSize);
            const Byte* srgbRow = &_srgb[offsetTL*3];
            float* unscaled = reinterpret_cast<float*>(unscaledRow);
            const Byte* srgb = srgbRow;
            if (offsetTL + _unscaledSize.x > srgbSize) {
                int endChannels = max(0, (srgbSize - offsetTL)*3);
                for (int x = 0; x < endChannels; ++x)
                    unscaled[x] = _linearizer.linear(srgb[x]);
                for (int x = 0; x < scanlineChannels - endChannels; ++x)
                    unscaled[x + endChannels] = _linearizer.linear(_srgb[x]);
            }
            else {
                for (int x = 0; x < scanlineChannels; ++x)
                    unscaled[x] = _linearizer.linear(srgb[x]);
            }
            unscaledRow += _unscaled.stride();
        }

        // Scale to desired size and apply scanline filter
        _scaler.render();

        // Delinearization and float-to-byte conversion
        const Byte* scaledRow = _scaled.data();
        Byte* outputRow = _bitmap.data();
        for (int y = 0; y < outputSize.y; ++y) {
            const float* scaled = reinterpret_cast<const float*>(scaledRow);
            DWORD* output = reinterpret_cast<DWORD*>(outputRow);
            for (int x = 0; x < outputSize.x; ++x) {
                SRGB srgb =
                    _linearizer.srgb(Colour(scaled[0], scaled[1], scaled[2]));
                *output = (srgb.x << 16) | (srgb.y << 8) | srgb.z;
                ++output;
                scaled += 3;
            }
            scaledRow += _scaled.stride();
            outputRow += _bitmap.stride();
        }
        _lastBitmap = _bitmap;
        _bitmap = _window->setNextBitmap(_bitmap);
    }

    void save(String outputFileName)
    {
        setOutputSize(Vector(0, 0));
        join();
        _lastBitmap.save(PNGFileFormat<DWORD>(),
            File(outputFileName + ".png", true));

        if (_connector != 0) {
            FileStream s = File(outputFileName + ".ntsc", true).openWrite();
            s.write(_ntsc);
        }
    }

    void setConnector(int connector)
    {
        {
            Lock lock(&_mutex);
            _connector = connector;
        }
        restart();
    }
    int getConnector() { return _connector; }
    void setScanlineProfile(int profile)
    {
        {
            Lock lock(&_mutex);
            _scanlineProfile = profile;
        }
        restart();
    }
    int getScanlineProfile() { return _scanlineProfile; }
    void setHorizontalProfile(int profile)
    {
        {
            Lock lock(&_mutex);
            _horizontalProfile = profile;
        }
        restart();
    }
    int getHorizontalProfile() { return _horizontalProfile; }
    void setScanlineWidth(double width)
    {
        {
            Lock lock(&_mutex);
            _scanlineWidth = width;
        }
        restart();
    }
    double getScanlineWidth() { return _scanlineWidth; }
    void setScanlineBleeding(int bleeding)
    {
        {
            Lock lock(&_mutex);
            _scanlineBleeding = bleeding;
        }
        restart();
    }
    int getScanlineBleeding() { return _scanlineBleeding; }
    void setHorizontalBleeding(int bleeding)
    {
        {
            Lock lock(&_mutex);
            _horizontalBleeding = bleeding;
        }
        restart();
    }
    int getHorizontalBleeding() { return _horizontalBleeding; }
    void setZoom(double zoom)
    {
        if (zoom == 0)
            zoom = 1.0;
        {
            Lock lock(&_mutex);
            if (_window->hWnd() != 0) {
                Vector mousePosition = _window->outputMousePosition();
                Vector size = _outputSize;
                Vector2<float> position = Vector2Cast<float>(size)/2.0f;
                if (_dragging || (mousePosition.inside(size) &&
                    (GetAsyncKeyState(VK_LBUTTON) & 0x8000) == 0 &&
                    (GetAsyncKeyState(VK_RBUTTON) & 0x8000) == 0))
                    position = Vector2Cast<float>(mousePosition);
                _inputTL += position*static_cast<float>(zoom - _zoom)/(
                    Vector2<float>(static_cast<float>(_aspectRatio)/2.0f, 1.0f)
                    *static_cast<float>(_zoom*zoom));
            }
            _zoom = zoom;
        }
        restart();
    }
    double getZoom() { return _zoom; }
    void setHorizontalRollOff(double rollOff)
    {
        {
            Lock lock(&_mutex);
            _horizontalRollOff = rollOff;
        }
        restart();
    }
    double getHorizontalRollOff() { return _horizontalRollOff; }
    void setHorizontalLobes(double lobes)
    {
        {
            Lock lock(&_mutex);
            _horizontalLobes = lobes;
        }
        restart();
    }
    double getHorizontalLobes() { return _horizontalLobes; }
    void setVerticalRollOff(double rollOff)
    {
        {
            Lock lock(&_mutex);
            _verticalRollOff = rollOff;
        }
        restart();
    }
    double getVerticalRollOff() { return _verticalRollOff; }
    void setVerticalLobes(double lobes)
    {
        {
            Lock lock(&_mutex);
            _verticalLobes = lobes;
        }
        restart();
    }
    double getVerticalLobes() { return _verticalLobes; }
    void setSubPixelSeparation(double separation)
    {
        {
            Lock lock(&_mutex);
            _subPixelSeparation = separation;
        }
        restart();
    }
    double getSubPixelSeparation() { return _subPixelSeparation; }
    void setPhosphor(int phosphor)
    {
        {
            Lock lock(&_mutex);
            _phosphor = phosphor;
        }
        restart();
    }
    int getPhosphor() { return _phosphor; }
    void setMask(int mask)
    {
        {
            Lock lock(&_mutex);
            _mask = mask;
        }
        restart();
    }
    int getMask() { return _mask; }
    void setMaskSize(double size)
    {
        {
            Lock lock(&_mutex);
            _maskSize = size;
        }
        restart();
    }
    double getMaskSize() { return _maskSize; }
    void setAspectRatio(double ratio)
    {
        if (ratio == 0)
            ratio = 1.0;
        {
            Lock lock(&_mutex);
            if (_window->hWnd() != 0) {
                Vector mousePosition = _window->outputMousePosition();
                Vector size = _outputSize;
                Vector2<float> position = Vector2Cast<float>(size)/2.0f;
                if (_dragging || (mousePosition.inside(size) &&
                    (GetAsyncKeyState(VK_LBUTTON) & 0x8000) == 0 &&
                    (GetAsyncKeyState(VK_RBUTTON) & 0x8000) == 0))
                    position = Vector2Cast<float>(mousePosition);
                _inputTL.x += position.x*2.0f*static_cast<float>(
                    (ratio - _aspectRatio)/(_zoom*ratio*_aspectRatio));
            }
            _aspectRatio = ratio;
        }
        restart();
    }
    double getAspectRatio() { return _aspectRatio; }
    void setOverscan(double overscan)
    {
        {
            Lock lock(&_mutex);
            _overscan = overscan;
        }
        restart();
    }
    void setOutputSize(Vector outputSize)
    {
        {
            Lock lock(&_mutex);
            _outputSize = outputSize;
            _active = true;
        }
        restart();
    }
    void setCombFilter(int combFilter)
    {
        {
            Lock lock(&_mutex);
            _combFilter = combFilter;
        }
        restart();
    }
    int getCombFilter() { return _combFilter; }

    void setHue(double hue)
    {
        {
            Lock lock(&_mutex);
            _hue = hue;
        }
        restart();
    }
    double getHue() { return _hue; }
    void setSaturation(double saturation)
    {
        {
            Lock lock(&_mutex);
            _saturation = saturation;
        }
        restart();
    }
    double getSaturation() { return _saturation; }
    void setContrast(double contrast)
    {
        {
            Lock lock(&_mutex);
            _contrast = contrast;
        }
        restart();
    }
    double getContrast() { return _contrast; }
    void setBrightness(double brightness)
    {
        {
            Lock lock(&_mutex);
            _brightness = brightness;
        }
        restart();
    }
    double getBrightness() { return _brightness; }
    void setShowClipping(bool showClipping)
    {
        {
            Lock lock(&_mutex);
            _showClipping = showClipping;
        }
        restart();
    }
    bool getShowClipping() { return _showClipping; }
    void setChromaBandwidth(double chromaBandwidth)
    {
        {
            Lock lock(&_mutex);
            _chromaBandwidth = chromaBandwidth;
        }
        restart();
    }
    double getChromaBandwidth() { return _chromaBandwidth; }
    void setLumaBandwidth(double lumaBandwidth)
    {
        {
            Lock lock(&_mutex);
            _lumaBandwidth = lumaBandwidth;
        }
        restart();
    }
    double getLumaBandwidth() { return _lumaBandwidth; }
    void setRollOff(double rollOff)
    {
        {
            Lock lock(&_mutex);
            _rollOff = rollOff;
        }
        restart();
    }
    double getRollOff() { return _rollOff; }
    void setLobes(double lobes)
    {
        {
            Lock lock(&_mutex);
            _lobes = lobes;
        }
        restart();
    }
    double getLobes() { return _lobes; }
    void setPhase(int phase)
    {
        {
            Lock lock(&_mutex);
            _phase = phase;
        }
        restart();
    }

    Vector requiredSize()
    {
        {
            Lock lock(&_mutex);
            if (!_outputSize.zeroArea())
                return _outputSize;
            _active = true;
        }
        restart();
        join();
        {
            Lock lock(&_mutex);
            return _outputSize;
        }
    }
    void mouseInput(Vector position, bool button)
    {
        _mousePosition = position;
        if (button) {
            if (!_dragging) {
                _dragStart = position;
                _dragStartInputPosition = _inputTL +
                    Vector2Cast<float>(position)/scale();
            }
            _inputTL =
                _dragStartInputPosition - Vector2Cast<float>(position)/scale();
            restart();
        }
       _dragging = button;
    }
    void saveRGBI(File outputFile)
    {
        outputFile.openWrite().write(static_cast<const void*>(&_rgbi[0]),
            _data->getTotal());
    }

private:
    // Output pixels per input pixel
    Vector2<float> scale()
    {
        return Vector2<float>(static_cast<float>(_aspectRatio)/2.0f, 1.0f)*
            static_cast<float>(_zoom);
    }

    CGAData* _data;
    CGASequencer* _sequencer;
    AppendableArray<int> _scanlines;    // hdot positions of scanline starts
    AppendableArray<int> _fields;       // scanline numbers of field starts
    AppendableArray<float> _fieldOffsets; // fractional scanline numbers

    int _connector;
    int _phase;
    int _scanlineProfile;
    int _horizontalProfile;
    double _scanlineWidth;
    int _scanlineBleeding;
    int _horizontalBleeding;
    double _horizontalRollOff;
    double _verticalRollOff;
    double _horizontalLobes;
    double _verticalLobes;
    double _subPixelSeparation;
    int _phosphor;
    int _mask;
    double _maskSize;
    double _zoom;
    double _aspectRatio;
    double _overscan;
    Vector _outputSize;
    Vector2<float> _inputTL;  // input position of top-left of output
    double _hue;
    double _saturation;
    double _contrast;
    double _brightness;
    double _chromaBandwidth;
    double _lumaBandwidth;
    double _rollOff;
    double _lobes;
    int _combFilter;
    bool _showClipping;
    bool _active;

    Bitmap<DWORD> _bitmap;
    Bitmap<DWORD> _lastBitmap;
    CGAComposite _composite;
    NTSCDecoder _decoder;
    Linearizer _linearizer;
    CGAArtWindow* _window;
    Mutex _mutex;

    ScanlineRenderer _scaler;
    Vector _combedSize;
    Array<Byte> _rgbi;
    Array<Byte> _ntsc;
    Array<Byte> _srgb;
    Vector _unscaledSize;
    AlignedBuffer _unscaled;
    AlignedBuffer _scaled;

    bool _dragging;
    Vector _dragStart;
    Vector2<float> _dragStartInputPosition;
    Vector _mousePosition;
};

typedef CGAOutputT<void> CGAOutput;

template<class T> class CGAArtWindowT : public RootWindow
{
public:
    CGAArtWindowT() : _outputWindow(this), _monitor(this), _videoCard(this)
    {
        setText("CGA Art");
        add(&_outputWindow);
        add(&_monitor);
        add(&_videoCard);
        add(&_knobSliders);
    }
    void create()
    {
        _monitor._connector.set(_output->getConnector());
        _monitor._colour._brightness.setValue(_output->getBrightness());
        _monitor._colour._saturation.setValue(_output->getSaturation());
        _monitor._colour._contrast.setValue(_output->getContrast());
        _monitor._colour._hue.setValue(_output->getHue());
        _monitor._colour._showClipping.setCheckState(
            _output->getShowClipping());
        _monitor._filter._chromaBandwidth.setValue(
            _output->getChromaBandwidth());
        _monitor._filter._lumaBandwidth.setValue(_output->getLumaBandwidth());
        _monitor._filter._rollOff.setValue(_output->getRollOff());
        _monitor._filter._lobes.setValue(_output->getLobes());
        _monitor._filter._combFilter.set(_output->getCombFilter());
        _monitor._phosphors._phosphor.set(_output->getPhosphor());
        _monitor._phosphors._mask.set(_output->getMask());
        _monitor._phosphors._maskSize.setValue(_output->getMaskSize());
        _monitor._horizontal._profile.set(_output->getHorizontalProfile());
        _monitor._horizontal._bleeding.set(_output->getHorizontalBleeding());
        _monitor._horizontal._rollOff.setValue(
            _output->getHorizontalRollOff());
        _monitor._horizontal._lobes.setValue(_output->getHorizontalLobes());
        _monitor._horizontal._subPixelSeparation.setValue(
            _output->getSubPixelSeparation());
        _monitor._scanlines._profile.set(_output->getScanlineProfile());
        _monitor._scanlines._width.setValue(_output->getScanlineWidth());
        _monitor._scanlines._bleeding.set(_output->getScanlineBleeding());
        _monitor._scanlines._rollOff.setValue(_output->getVerticalRollOff());
        _monitor._scanlines._lobes.setValue(_output->getVerticalLobes());
        _monitor._scaling._zoom.setValue(_output->getZoom());
        _monitor._scaling._aspectRatio.setValue(_output->getAspectRatio());
        int mode = _matcher->getMode();
        int m;
        if ((mode & 0x80) != 0)
            m = 8 + (mode & 1);
        else {
            switch (mode & 0x13) {
                case 0: m = 0; break;
                case 1: m = 1; break;
                case 2: m = 3; break;
                case 3: m = 7; break;
                case 0x10: m = 4; break;
                case 0x11: m = 5; break;
                case 0x12: m = 2; break;
                case 0x13: m = 6; break;
            }
        }
        _videoCard._registers._mode.set(m);
        _videoCard._registers._bw.setCheckState((mode & 4) != 0);
        _videoCard._registers._blink.setCheckState((mode & 0x20) != 0);
        int palette = _matcher->getPalette();
        if (palette == 0xff) {
            _paletteSelected = 0;
            _backgroundSelected = 0x10;
        }
        else {
            _paletteSelected = (palette >> 4) & 3;
            _backgroundSelected = palette & 0xf;
        }
        _videoCard._registers._palette.set(_paletteSelected);
        _videoCard._registers._background.set(_backgroundSelected);
        _videoCard._registers._scanlinesPerRow.set(
            _matcher->getScanlinesPerRow() - 1);
        _videoCard._registers._scanlinesRepeat.set(
            _matcher->getScanlinesRepeat() - 1);
        _videoCard._registers._phase.setCheckState(_matcher->getPhase() == 0);
        _videoCard._registers._interlace.set(_matcher->getInterlace());
        _videoCard._registers._interlaceSync.setCheckState(
            _matcher->getInterlaceSync());
        _videoCard._registers._interlacePhase.setCheckState(
            _matcher->getInterlacePhase());
        _videoCard._registers._flicker.setCheckState(_matcher->getFlicker());
        bool matchMode = _program->getMatchMode();
        _videoCard._matching._matchMode.setCheckState(matchMode);
        if (!matchMode)
            _videoCard._matching._matchMode.enableWindow(false);
        _videoCard._matching._diffusionHorizontal.setValue(
            _matcher->getDiffusionHorizontal());
        _videoCard._matching._diffusionVertical.setValue(
            _matcher->getDiffusionVertical());
        _videoCard._matching._diffusionTemporal.setValue(
            _matcher->getDiffusionTemporal());
        _videoCard._matching._quality.setValue(_matcher->getQuality());
        _videoCard._matching._gamma.setValue(_matcher->getGamma());
        _videoCard._matching._clipping.set(_matcher->getClipping());
        _videoCard._matching._metric.set(_matcher->getMetric());
        _videoCard._matching._profile.set(_matcher->getPrescalerProfile());
        _videoCard._matching._characterSet.set(_matcher->getCharacterSet());
        setInnerSize(Vector(0, 0));
        _outputWindow.setInnerSize(_output->requiredSize());
        RootWindow::create();
        updateApplicableControls();
    }
    void innerSizeSet(Vector size)
    {
        if (size.x <= 0 || size.y <= 0)
            return;
        RootWindow::innerSizeSet(size);
        int owx = max(_videoCard.outerSize().x,
            size.x - (_monitor.outerSize().x + 3*pad().x));
        int owy = max(0, size.y - (_videoCard.outerSize().y + 3*pad().y));
        _outputWindow.setInnerSize(Vector(owx, owy));
        layout();
        invalidate();
    }
    void layout()
    {
        Vector p = pad();
        _outputWindow.setTopLeft(p);
        int r = _outputWindow.right();
        _videoCard.setTopLeft(_outputWindow.bottomLeft() + Vector(0, p.y));
        r = max(r, _videoCard.right());
        _monitor.setTopLeft(_outputWindow.topRight() + Vector(p.x, 0));
        setInnerSize(p + Vector(_monitor.right(),
            max(_videoCard.bottom(), _monitor.bottom())));
    }
    void keyboardCharacter(int character)
    {
        if (character == VK_ESCAPE)
            remove();
    }
    void setConfig(ConfigFile* config)
    {
        _monitor._colour._brightness.setConfig(config);
        _monitor._colour._saturation.setConfig(config);
        _monitor._colour._contrast.setConfig(config);
        _monitor._colour._hue.setConfig(config);
        _monitor._filter._chromaBandwidth.setConfig(config);
        _monitor._filter._lumaBandwidth.setConfig(config);
        _monitor._filter._rollOff.setConfig(config);
        _monitor._filter._lobes.setConfig(config);
        _monitor._phosphors._maskSize.setConfig(config);
        _monitor._horizontal._rollOff.setConfig(config);
        _monitor._horizontal._lobes.setConfig(config);
        _monitor._horizontal._subPixelSeparation.setConfig(config);
        _monitor._scanlines._width.setConfig(config);
        _monitor._scanlines._rollOff.setConfig(config);
        _monitor._scanlines._lobes.setConfig(config);
        _monitor._scaling._zoom.setConfig(config);
        _monitor._scaling._aspectRatio.setConfig(config);
        _videoCard._matching._diffusionHorizontal.setConfig(config);
        _videoCard._matching._diffusionVertical.setConfig(config);
        _videoCard._matching._diffusionTemporal.setConfig(config);
        _videoCard._matching._quality.setConfig(config);
        _videoCard._matching._gamma.setConfig(config);
    }
    void setMatcher(CGAMatcher* matcher) { _matcher = matcher; }
    void setOutput(CGAOutput* output) { _output = output; }
    void setProgram(Program* program) { _program = program; }
    Bitmap<DWORD> setNextBitmap(Bitmap<DWORD> bitmap)
    {
        return _outputWindow.setNextBitmap(bitmap);
    }
    void beginConvert() { _program->beginConvert(); }

    void updateApplicableControls()
    {
        bool matchMode = _program->getMatchMode();
        int mode = _matcher->getMode();
        int scanlinesPerRow = _matcher->getScanlinesPerRow();
        _videoCard._registers._blink.enableWindow((mode & 2) == 0);
        _videoCard._registers._palette.enableWindow((mode & 0x12) == 2);
        _videoCard._registers._phase.enableWindow((mode & 1) == 1);
        bool composite = (_output->getConnector() != 0);
        _videoCard._matching._quality.enableWindow(matchMode &&
            (((((mode & 3) != 2 || composite) && (mode & 0x13) != 0x13) ||
            scanlinesPerRow > 2)));
        _videoCard._matching._gamma.enableWindow(matchMode);
        _videoCard._matching._clipping.enableWindow(matchMode);
        _videoCard._matching._metric.enableWindow(matchMode);
        _videoCard._matching._characterSet.enableWindow(matchMode &&
            (mode & 2) == 0);
        _videoCard._matching._diffusionHorizontal.enableWindow(matchMode);
        _videoCard._matching._diffusionVertical.enableWindow(matchMode);
        _videoCard._matching._diffusionTemporal.enableWindow(matchMode);
        _monitor._colour._saturation.enableWindow(composite);
        _monitor._colour._hue.enableWindow(composite);
        _monitor._filter.enableWindow(composite);
    }
    void modeSet(int value)
    {
        static const int modes[10] = {0, 1, 0x12, 2, 0x10, 0x11, 0x13, 3, 0x80,
            0x81};
        int mode = modes[value] | 8 |
            (_videoCard._registers._bw.checked() ? 4 : 0) |
            (_videoCard._registers._blink.checked() ? 0x20 : 0);
        _matcher->setMode(mode);
        updateApplicableControls();
        beginConvert();
    }
    void backgroundSet(int value)
    {
        _backgroundSelected = value;
        setPaletteAndBackground();
    }
    void paletteSet(int value)
    {
        _paletteSelected = value;
        setPaletteAndBackground();
    }
    void setPaletteAndBackground()
    {
        if (_backgroundSelected == 0x10)
            _matcher->setPalette(0xff);
        else {
            _matcher->setPalette(
                _backgroundSelected + (_paletteSelected << 4));
        }
        beginConvert();
    }
    void scanlinesPerRowSet(int value)
    {
        _matcher->setScanlinesPerRow(value + 1);
        updateApplicableControls();
        beginConvert();
    }
    void scanlinesRepeatSet(int value)
    {
        _matcher->setScanlinesRepeat(value + 1);
        beginConvert();
    }
    void diffusionHorizontalSet(double value)
    {
        _matcher->setDiffusionHorizontal(value);
        beginConvert();
    }
    void diffusionVerticalSet(double value)
    {
        _matcher->setDiffusionVertical(value);
        beginConvert();
    }
    void diffusionTemporalSet(double value)
    {
        _matcher->setDiffusionTemporal(value);
        beginConvert();
    }
    void qualitySet(double value)
    {
        _matcher->setQuality(value);
        beginConvert();
    }
    void gammaSet(double value)
    {
        _matcher->setGamma(value);
        beginConvert();
    }
    void clippingSet(int value)
    {
        _matcher->setClipping(value);
        beginConvert();
    }
    void metricSet(int value)
    {
        _matcher->setMetric(value);
        beginConvert();
    }
    void scanlineWidthSet(double value) { _output->setScanlineWidth(value); }
    void scanlineProfileSet(int value) { _output->setScanlineProfile(value); }
    void zoomSet(double value) { _output->setZoom(value); }
    void scanlineBleedingSet(int value)
    {
        _output->setScanlineBleeding(value);
    }
    void verticalRollOffSet(double value)
    {
        _output->setVerticalRollOff(value);
    }
    void verticalLobesSet(double value)
    {
        _output->setVerticalLobes(value);
    }
    void horizontalProfileSet(int value)
    {
        _output->setHorizontalProfile(value);
    }
    void prescalerProfileSet(int value)
    {
        _matcher->setPrescalerProfile(value);
        beginConvert();
    }
    void horizontalBleedingSet(int value)
    {
        _output->setHorizontalBleeding(value);
    }
    void horizontalRollOffSet(double value)
    {
        _output->setHorizontalRollOff(value);
    }
    void horizontalLobesSet(double value)
    {
        _output->setHorizontalLobes(value);
    }
    void subPixelSeparationSet(double value)
    {
        _output->setSubPixelSeparation(value);
    }
    void phosphorSet(int value) { _output->setPhosphor(value); }
    void maskSet(int value) { _output->setMask(value); }
    void maskSizeSet(double value) { _output->setMaskSize(value); }
    void aspectRatioSet(double value) { _output->setAspectRatio(value); }
    void combFilterSet(int value) { _output->setCombFilter(value); }
    void bwSet(bool value)
    {
        _matcher->setMode((_matcher->getMode() & ~4) | (value ? 4 : 0));
        beginConvert();
    }
    void blinkSet(bool value)
    {
        int mode = _matcher->getMode();
        _matcher->setMode((mode & ~0x20) | (value ? 0x20 : 0));
        if ((mode & 2) == 0)
            beginConvert();
    }
    void phaseSet(bool value)
    {
        _matcher->setPhase(value ? 0 : 1);
        _output->setPhase(value ? 0 : 1);
    }
    void interlaceSet(int value)
    {
        _matcher->setInterlace(value);
        beginConvert();
    }
    void interlaceSyncSet(bool value)
    {
        _matcher->setInterlaceSync(value);
        beginConvert();
    }
    void interlacePhaseSet(bool value)
    {
        _matcher->setInterlacePhase(value);
        beginConvert();
    }
    void flickerSet(bool value)
    {
        _matcher->setFlicker(value);
        beginConvert();
    }
    void characterSetSet(int value)
    {
        _matcher->setCharacterSet(value);
        beginConvert();
    }
    void matchModeSet(bool value)
    {
        _program->setMatchMode(value);
        updateApplicableControls();
        beginConvert();
        _output->restart();
    }

    void brightnessSet(double brightness)
    {
        _output->setBrightness(brightness);
        _matcher->setBrightness(brightness);
        beginConvert();
    }
    void saturationSet(double saturation)
    {
        _output->setSaturation(saturation);
        _matcher->setSaturation(saturation);
        beginConvert();
    }
    void contrastSet(double contrast)
    {
        _output->setContrast(contrast);
        _matcher->setContrast(contrast);
        beginConvert();
    }
    void hueSet(double hue)
    {
        _output->setHue(hue);
        _matcher->setHue(hue);
        beginConvert();
    }
    void showClippingSet(bool showClipping)
    {
        _output->setShowClipping(showClipping);
        _output->restart();
    }
    void chromaBandwidthSet(double chromaBandwidth)
    {
        _output->setChromaBandwidth(chromaBandwidth);
        _matcher->setChromaBandwidth(chromaBandwidth);
    }
    void lumaBandwidthSet(double lumaBandwidth)
    {
        _output->setLumaBandwidth(lumaBandwidth);
        _matcher->setLumaBandwidth(lumaBandwidth);
    }
    void rollOffSet(double rollOff)
    {
        _output->setRollOff(rollOff);
        _matcher->setRollOff(rollOff);
    }
    void lobesSet(double lobes)
    {
        _output->setLobes(lobes);
        _matcher->setLobes(lobes);
    }
    void connectorSet(int connector)
    {
        _output->setConnector(connector);
        _matcher->setConnector(connector);
        updateApplicableControls();
        beginConvert();
    }

    Vector outputMousePosition()
    {
        POINT point;
        IF_ZERO_THROW(GetCursorPos(&point));
        return Vector(point.x, point.y) -
            _outputWindow.clientToScreen(Vector(0, 0));
    }
    void setProgress(float progress)
    {
        _videoCard._matching._progressBar.show(
            progress >= 0 ? SW_SHOW : SW_HIDE);
        _videoCard._matching._progressBar.setValue(progress);
    }
private:
    Vector vSpace() { return Vector(0, 15); }
    Vector hSpace() { return Vector(15, 0); }
    Vector groupTL() { return Vector(15, 20); }
    Vector groupBR() { return Vector(15, 15); }
    Vector pad() { return Vector(20, 20); }
    Vector groupVSpace() { return Vector(0, 10); }
    Vector groupHSpace() { return Vector(10, 0); }

    class OutputWindow : public BitmapWindow
    {
    public:
        OutputWindow(CGAArtWindow* host) : _host(host) { }
        bool mouseInput(Vector position, int buttons, int wheel)
        {
            bool lButton = (buttons & MK_LBUTTON) != 0;
            _host->_output->mouseInput(position, lButton);
            if (wheel != 0)
                _host->_monitor._scaling._zoom.changeValue(wheel/1200.0f);
            return lButton;
        }
        void innerSizeSet(Vector size) { _host->_output->setOutputSize(size); }
    private:
        CGAArtWindow* _host;
    };
    OutputWindow _outputWindow;
    struct ProfileDropDown : public CaptionedDropDownList
    {
        ProfileDropDown()
        {
            setText("Profile: ");
            add("rectangle");
            add("triangle");
            add("circle");
            add("gaussian");
            add("sinc");
            add("box");
        }
    };
    struct MonitorGroup : public GroupBox
    {
        MonitorGroup(CGAArtWindow* host)
          : _host(host), _colour(host), _filter(host), _scanlines(host),
            _horizontal(host), _phosphors(host), _scaling(host)
        {
            setText("Monitor");
            _connector.setChanged(
                [&](int value) { _host->connectorSet(value); });
            _connector.setText("Connector: ");
            _connector.add("RGBI");
            _connector.add("Composite (old)");
            _connector.add("Composite (new)");
            _connector.set(1);
            add(&_connector);
            add(&_colour);
            add(&_filter);
            add(&_phosphors);
            add(&_horizontal);
            add(&_scanlines);
            add(&_scaling);
        }
        void layout()
        {
            Vector vSpace = _host->vSpace();
            Vector gv = _host->groupVSpace();
            _connector.setTopLeft(_host->groupTL());
            int r = _connector.right();
            _colour.setTopLeft(_connector.bottomLeft() + gv);
            r = max(r, _colour.right());
            _filter.setTopLeft(_colour.bottomLeft() + gv);
            r = max(r, _filter.right());
            _phosphors.setTopLeft(_filter.bottomLeft() + gv);
            r = max(r, _phosphors.right());
            _horizontal.setTopLeft(
                Vector(r, _colour.top()) + _host->groupHSpace());
            r = _horizontal.right();
            _scanlines.setTopLeft(_horizontal.bottomLeft() + gv);
            r = max(r, _scanlines.right());
            _scaling.setTopLeft(_scanlines.bottomLeft() + gv);
            r = max(r, _scaling.right());
            setInnerSize(
                Vector(r, max(_phosphors.bottom(), _scaling.bottom())) +
                _host->groupBR());
        }
        CaptionedDropDownList _connector;
        struct ColourGroup : public GroupBox
        {
            ColourGroup(CGAArtWindow* host) : _host(host)
            {
                KnobSliders* sliders = &host->_knobSliders;
                setText("Colour");
                _brightness.setSliders(sliders);
                _brightness.setValueSet(
                    [&](double value) { _host->brightnessSet(value); });
                _brightness.setText("Brightness: ");
                _brightness.setRange(-50, 50);
                add(&_brightness);
                _saturation.setSliders(sliders);
                _saturation.setValueSet(
                    [&](double value) { _host->saturationSet(value); });
                _saturation.setText("Saturation: ");
                _saturation.setRange(0, 400);
                add(&_saturation);
                _contrast.setSliders(sliders);
                _contrast.setValueSet(
                    [&](double value) { _host->contrastSet(value); });
                _contrast.setText("Contrast: ");
                _contrast.setRange(0, 400);
                add(&_contrast);
                _hue.setSliders(sliders);
                _hue.setValueSet([&](double value) { _host->hueSet(value); });
                _hue.setText("Hue: ");
                _hue.setRange(-180, 180);
                add(&_hue);
                _showClipping.setClicked(
                    [&](bool value) { _host->showClippingSet(value); });
                _showClipping.setText("Show clipping");
                add(&_showClipping);
            }
            void layout()
            {
                Vector vSpace = _host->vSpace();
                _brightness.setTopLeft(_host->groupTL());
                int r = _brightness.right();
                _saturation.setTopLeft(_brightness.bottomLeft() + vSpace);
                r = max(r, _saturation.right());
                _contrast.setTopLeft(_saturation.bottomLeft() + vSpace);
                r = max(r, _contrast.right());
                _hue.setTopLeft(_contrast.bottomLeft() + vSpace);
                r = max(r, _hue.right());
                _showClipping.setTopLeft(_hue.bottomLeft() + vSpace);
                r = max(r, _showClipping.right());
                setInnerSize(
                    Vector(r, _showClipping.bottom()) + _host->groupBR());
            }
            CGAArtWindow* _host;
            KnobSlider _brightness;
            KnobSlider _saturation;
            KnobSlider _contrast;
            KnobSlider _hue;
            CheckBox _showClipping;
        };
        ColourGroup _colour;
        struct FilterGroup : public GroupBox
        {
            FilterGroup(CGAArtWindow* host) : _host(host)
            {
                KnobSliders* sliders = &host->_knobSliders;
                setText("Filter");
                _chromaBandwidth.setSliders(sliders);
                _chromaBandwidth.setValueSet(
                    [&](double value) { _host->chromaBandwidthSet(value); });
                _chromaBandwidth.setText("Chroma bandwidth: ");
                _chromaBandwidth.setRange(0, 2);
                add(&_chromaBandwidth);
                _lumaBandwidth.setSliders(sliders);
                _lumaBandwidth.setValueSet(
                    [&](double value) { _host->lumaBandwidthSet(value); });
                _lumaBandwidth.setText("Luma bandwidth: ");
                _lumaBandwidth.setRange(0, 2);
                add(&_lumaBandwidth);
                _rollOff.setSliders(sliders);
                _rollOff.setValueSet(
                    [&](double value) { _host->rollOffSet(value); });
                _rollOff.setText("Roll-off: ");
                _rollOff.setRange(0, 1);
                add(&_rollOff);
                _lobes.setSliders(sliders);
                _lobes.setValueSet(
                    [&](double value) { _host->lobesSet(value); });
                _lobes.setText("Lobes: ");
                _lobes.setRange(1, 10);
                add(&_lobes);
                _combFilter.setChanged(
                    [&](int value) { _host->combFilterSet(value); });
                _combFilter.setText("Comb filter: ");
                _combFilter.add("none");
                _combFilter.add("1 line");
                _combFilter.add("2 line");
                add(&_combFilter);
            }
            void layout()
            {
                Vector vSpace = _host->vSpace();
                _chromaBandwidth.setTopLeft(_host->groupTL());
                int r = _chromaBandwidth.right();
                _lumaBandwidth.setTopLeft(
                    _chromaBandwidth.bottomLeft() + vSpace);
                r = max(r, _lumaBandwidth.right());
                _combFilter.setTopLeft(
                    _lumaBandwidth.bottomLeft() + vSpace);
                r = max(r, _combFilter.right());
                _rollOff.setTopLeft(_combFilter.bottomLeft() + vSpace);
                r = max(r, _rollOff.right());
                _lobes.setTopLeft(_rollOff.bottomLeft() + vSpace);
                r = max(r, _lobes.right());
                setInnerSize(Vector(r, _lobes.bottom()) + _host->groupBR());
            }
            CGAArtWindow* _host;
            KnobSlider _chromaBandwidth;
            KnobSlider _lumaBandwidth;
            CaptionedDropDownList _combFilter;
            KnobSlider _rollOff;
            KnobSlider _lobes;
        };
        FilterGroup _filter;
        struct PhosphorsGroup : public GroupBox
        {
            PhosphorsGroup(CGAArtWindow* host) : _host(host)
            {
                setText("Phosphors");
                _phosphor.setChanged(
                    [&](int value) { _host->phosphorSet(value); });
                _phosphor.setText("Colour: ");
                _phosphor.add("RGB");
                _phosphor.add("green");
                _phosphor.add("amber");
                _phosphor.add("white");
                _phosphor.add("blue");
                add(&_phosphor);
                _mask.setChanged([&](int value) { _host->maskSet(value); });
                _mask.setText("Mask: ");
                _mask.add("shadow mask");
                _mask.add("aperture grille");
                add(&_mask);
                _maskSize.setSliders(&_host->_knobSliders);
                _maskSize.setValueSet(
                    [&](double value) { _host->maskSizeSet(value); });
                _maskSize.setText("Size: ");
                _maskSize.setRange(0, 2);
                add(&_maskSize);
            }
            void layout()
            {
                Vector vSpace = _host->vSpace();
                _phosphor.setTopLeft(_host->groupTL());
                int r = _phosphor.right();
                _mask.setTopLeft(_phosphor.bottomLeft() + vSpace);
                r = max(r, _mask.right());
                _maskSize.setTopLeft(_mask.bottomLeft() + vSpace);
                r = max(r, _maskSize.right());
                setInnerSize(Vector(r, _maskSize.bottom()) + _host->groupBR());
            }
            CGAArtWindow* _host;
            CaptionedDropDownList _phosphor;
            CaptionedDropDownList _mask;
            KnobSlider _maskSize;
        };
        PhosphorsGroup _phosphors;
        struct BleedingDropDown : public CaptionedDropDownList
        {
            BleedingDropDown()
            {
                setText("Bleeding: ");
                add("none");
                add("down");
                add("symmetrical");
            }
        };
        struct HorizontalGroup : public GroupBox
        {
            HorizontalGroup(CGAArtWindow* host) : _host(host)
            {
                setText("Horizontal");
                _profile.setChanged(
                    [&](int value) { _host->horizontalProfileSet(value); });
                add(&_profile);
                _bleeding.setChanged(
                    [&](int value) { _host->horizontalBleedingSet(value); });
                add(&_bleeding);
                _rollOff.setSliders(&_host->_knobSliders);
                _rollOff.setValueSet(
                    [&](double value) { _host->horizontalRollOffSet(value); });
                _rollOff.setText("Roll-off: ");
                _rollOff.setRange(0, 1);
                add(&_rollOff);
                _lobes.setSliders(&_host->_knobSliders);
                _lobes.setValueSet(
                    [&](double value) { _host->horizontalLobesSet(value); });
                _lobes.setText("Lobes: ");
                _lobes.setRange(1, 10);
                add(&_lobes);
                _subPixelSeparation.setSliders(&_host->_knobSliders);
                _subPixelSeparation.setValueSet([&](double value) {
                    _host->subPixelSeparationSet(value); });
                _subPixelSeparation.setText("Sub-pixel separation: ");
                _subPixelSeparation.setRange(-1, 1);
                add(&_subPixelSeparation);
            }
            void layout()
            {
                Vector vSpace = _host->vSpace();
                _profile.setTopLeft(_host->groupTL());
                int r = _profile.right();
                _bleeding.setTopLeft(_profile.bottomLeft() + vSpace);
                r = max(r, _bleeding.right());
                _rollOff.setTopLeft(_bleeding.bottomLeft() + vSpace);
                r = max(r, _rollOff.right());
                _lobes.setTopLeft(_rollOff.bottomLeft() + vSpace);
                r = max(r, _lobes.right());
                _subPixelSeparation.setTopLeft(_lobes.bottomLeft() + vSpace);
                r = max(r, _subPixelSeparation.right());
                setInnerSize(Vector(r,
                    _subPixelSeparation.bottom()) + _host->groupBR());
            }
            CGAArtWindow* _host;
            ProfileDropDown _profile;
            BleedingDropDown _bleeding;
            KnobSlider _rollOff;
            KnobSlider _lobes;
            KnobSlider _subPixelSeparation;
        };
        HorizontalGroup _horizontal;
        struct ScanlinesGroup : public GroupBox
        {
            ScanlinesGroup(CGAArtWindow* host) : _host(host)
            {
                setText("Scanlines");
                _profile.setChanged(
                    [&](int value) { _host->scanlineProfileSet(value); });
                add(&_profile);
                _width.setSliders(&_host->_knobSliders);
                _width.setValueSet(
                    [&](double value) { _host->scanlineWidthSet(value); });
                _width.setText("Width: ");
                _width.setRange(0, 1);
                add(&_width);
                _bleeding.setChanged(
                    [&](int value) { _host->scanlineBleedingSet(value); });
                add(&_bleeding);
                _rollOff.setSliders(&_host->_knobSliders);
                _rollOff.setValueSet(
                    [&](double value) { _host->verticalRollOffSet(value); });
                _rollOff.setText("Roll-off: ");
                _rollOff.setRange(0, 1);
                add(&_rollOff);
                _lobes.setSliders(&_host->_knobSliders);
                _lobes.setValueSet(
                    [&](double value) { _host->verticalLobesSet(value); });
                _lobes.setText("Lobes: ");
                _lobes.setRange(1, 10);
                add(&_lobes);
            }
            void layout()
            {
                Vector vSpace = _host->vSpace();
                _profile.setTopLeft(_host->groupTL());
                int r = _profile.right();
                _width.setTopLeft(_profile.bottomLeft() + vSpace);
                r = max(r, _width.right());
                _bleeding.setTopLeft(_width.bottomLeft() + vSpace);
                r = max(r, _bleeding.right());
                _rollOff.setTopLeft(_bleeding.bottomLeft() + vSpace);
                r = max(r, _rollOff.right());
                _lobes.setTopLeft(_rollOff.bottomLeft() + vSpace);
                r = max(r, _lobes.right());
                setInnerSize(Vector(r, _lobes.bottom()) + _host->groupBR());
            }
            CGAArtWindow* _host;
            ProfileDropDown _profile;
            KnobSlider _width;
            BleedingDropDown _bleeding;
            KnobSlider _rollOff;
            KnobSlider _lobes;
        };
        ScanlinesGroup _scanlines;
        struct ScalingGroup : public GroupBox
        {
            ScalingGroup(CGAArtWindow* host) : _host(host)
            {
                KnobSliders* sliders = &host->_knobSliders;
                setText("Scaling");
                _zoom.setSliders(sliders);
                _zoom.setValueSet(
                    [&](double value) { _host->zoomSet(value); });
                _zoom.setText("Zoom: ");
                _zoom.setRange(1, 10);
                _zoom.setLogarithmic(true);
                add(&_zoom);
                _aspectRatio.setSliders(sliders);
                _aspectRatio.setValueSet(
                    [&](double value) { _host->aspectRatioSet(value); });
                _aspectRatio.setText("Aspect Ratio: ");
                _aspectRatio.setRange(0.5, 2);
                _aspectRatio.setLogarithmic(true);
                add(&_aspectRatio);
            }
            void layout()
            {
                Vector vSpace = _host->vSpace();
                _zoom.setTopLeft(_host->groupTL());
                int r = _zoom.right();
                _aspectRatio.setTopLeft(_zoom.bottomLeft() + vSpace);
                r = max(r, _aspectRatio.right());
                setInnerSize(Vector(r, _aspectRatio.bottom()) +
                    _host->groupBR());
            }
            CGAArtWindow* _host;
            KnobSlider _zoom;
            KnobSlider _aspectRatio;
        };
        ScalingGroup _scaling;
        CGAArtWindow* _host;
    };
    MonitorGroup _monitor;
    struct VideoCardGroup : public GroupBox
    {
        VideoCardGroup(CGAArtWindow* host)
          : _host(host), _registers(host), _matching(host)
        {
            setText("Video card");
            add(&_registers);
            add(&_matching);
        }
        void layout()
        {
            Vector vSpace = _host->vSpace();
            _registers.setTopLeft(_host->groupTL());
            int r = _registers.right();
            _matching.setTopLeft(
                _registers.bottomLeft() + _host->groupVSpace());
            r = max(r, _matching.right());
            setInnerSize(Vector(r, _matching.bottom()) + _host->groupBR());
        }
        struct RegistersGroup : public GroupBox
        {
            RegistersGroup(CGAArtWindow* host) : _host(host)
            {
                setText("Registers");
                _mode.setChanged([&](int value) { _host->modeSet(value); });
                _mode.setText("Mode: ");
                _mode.add("low-resolution text");
                _mode.add("high-resolution text");
                _mode.add("1bpp graphics");
                _mode.add("2bpp graphics");
                _mode.add("low-res text with 1bpp");
                _mode.add("high-res text with 1bpp");
                _mode.add("high-res 1bpp graphics");
                _mode.add("high-res 2bpp graphics");
                _mode.add("Auto -HRES");
                _mode.add("Auto +HRES");
                _mode.set(2);
                add(&_mode);
                _bw.setClicked(
                    [&](bool value) { _host->bwSet(value); });
                _bw.setText("+BW");
                add(&_bw);
                _blink.setClicked(
                    [&](bool value) { _host->blinkSet(value); });
                _blink.setText("+BLINK");
                add(&_blink);
                _palette.setChanged(
                    [&](int value) { _host->paletteSet(value); });
                _palette.setText("Palette: ");
                _palette.add("2/4/6");
                _palette.add("10/12/14");
                _palette.add("3/5/7");
                _palette.add("11/13/15");
                _palette.set(3);
                add(&_palette);
                _background.setChanged(
                    [&](int value) { _host->backgroundSet(value); });
                _background.setText("Background: ");
                for (int i = 0; i < 16; ++i)
                    _background.add(decimal(i));
                _background.add("Auto");
                _background.set(15);
                add(&_background);
                _scanlinesPerRow.setChanged(
                    [&](int value) { _host->scanlinesPerRowSet(value); });
                _scanlinesPerRow.setText("Scanlines per row: ");
                for (int i = 1; i <= 32; ++i) {
                    _scanlinesPerRow.add(decimal(i));
                    _scanlinesRepeat.add(decimal(i));
                }
                _scanlinesPerRow.set(1);
                add(&_scanlinesPerRow);
                _scanlinesRepeat.setChanged(
                    [&](int value) { _host->scanlinesRepeatSet(value); });
                _scanlinesRepeat.setText("Scanlines repeat: ");
                add(&_scanlinesRepeat);
                _phase.setClicked(
                    [&](bool value) { _host->phaseSet(value); });
                _phase.setText("Phase 0");
                add(&_phase);
                _interlace.setChanged(
                    [&](int value) { _host->interlaceSet(value); });
                _interlace.setText("Interlace: ");
                _interlace.add("None");
                _interlace.add("Even odd");
                _interlace.add("Odd even");
                _interlace.add("Even even");
                _interlace.add("Odd odd");
                add(&_interlace);
                _interlaceSync.setClicked(
                    [&](bool value) { _host->interlaceSyncSet(value); });
                _interlaceSync.setText("Interlace sync");
                add(&_interlaceSync);
                _interlacePhase.setClicked(
                    [&](bool value) { _host->interlacePhaseSet(value); });
                _interlacePhase.setText("Interlace phase");
                add(&_interlacePhase);
                _flicker.setClicked(
                    [&](bool value) { _host->flickerSet(value); });
                _flicker.setText("Flicker");
                add(&_flicker);
            }
            void layout()
            {
                Vector vSpace = _host->vSpace();
                Vector hSpace = _host->hSpace();
                _mode.setTopLeft(_host->groupTL());
                _bw.setTopLeft(_mode.topRight() + hSpace);
                _blink.setTopLeft(_bw.topRight() + hSpace);
                int r = _blink.right();
                _palette.setTopLeft(_mode.bottomLeft() + vSpace);
                _background.setTopLeft(_palette.topRight() + hSpace);
                r = max(r, _background.right());
                _scanlinesPerRow.setTopLeft(_palette.bottomLeft() + vSpace);
                _scanlinesRepeat.setTopLeft(
                    _scanlinesPerRow.topRight() + hSpace);
                r = max(r, _scanlinesRepeat.right());
                _phase.setTopLeft(_scanlinesPerRow.bottomLeft() + vSpace);
                _interlace.setTopLeft(_phase.topRight() + hSpace);
                _interlaceSync.setTopLeft(_phase.bottomLeft() + vSpace);
                _interlacePhase.setTopLeft(_interlaceSync.topRight() + hSpace);
                _flicker.setTopLeft(_interlacePhase.topRight() + hSpace);
                r = max(r, _flicker.right());
                setInnerSize(Vector(r, _flicker.bottom()) + _host->groupBR());
            }
            CGAArtWindow* _host;
            CaptionedDropDownList _mode;
            CheckBox _bw;
            CheckBox _blink;
            CaptionedDropDownList _palette;
            CaptionedDropDownList _background;
            CaptionedDropDownList _scanlinesPerRow;
            CaptionedDropDownList _scanlinesRepeat;
            CheckBox _phase;
            CaptionedDropDownList _interlace;
            CheckBox _interlaceSync;
            CheckBox _interlacePhase;
            CheckBox _flicker;
        };
        RegistersGroup _registers;
        struct MatchingGroup : public GroupBox
        {
            MatchingGroup(CGAArtWindow* host) : _host(host)
            {
                KnobSliders* sliders = &host->_knobSliders;
                setText("Matching");
                _matchMode.setClicked(
                    [&](bool value) { _host->matchModeSet(value); });
                _matchMode.setText("Match");
                add(&_matchMode);
                _progressBar.setText("Progress");
                add(&_progressBar);
                _diffusionHorizontal.setSliders(sliders);
                _diffusionHorizontal.setValueSet(
                    [&](double value) {
                        _host->diffusionHorizontalSet(value); });
                _diffusionHorizontal.setText("Diffusion: Horizontal: ");
                _diffusionHorizontal.setRange(0, 1);
                add(&_diffusionHorizontal);
                _diffusionVertical.setSliders(sliders);
                _diffusionVertical.setValueSet(
                    [&](double value) { _host->diffusionVerticalSet(value); });
                _diffusionVertical.setText("Vertical: ");
                _diffusionVertical.setRange(0, 1);
                _diffusionVertical.setCaptionWidth(0);
                add(&_diffusionVertical);
                _diffusionTemporal.setSliders(sliders);
                _diffusionTemporal.setValueSet(
                    [&](double value) { _host->diffusionTemporalSet(value); });
                _diffusionTemporal.setText("Temporal: ");
                _diffusionTemporal.setRange(0, 1);
                _diffusionTemporal.setCaptionWidth(0);
                add(&_diffusionTemporal);
                _quality.setSliders(sliders);
                _quality.setValueSet(
                    [&](double value) { _host->qualitySet(value); });
                _quality.setText("Quality: ");
                _quality.setRange(0, 1);
                _quality.setCaptionWidth(0);
                add(&_quality);
                _gamma.setSliders(sliders);
                _gamma.setValueSet(
                    [&](double value) { _host->gammaSet(value); });
                _gamma.setText("Gamma: ");
                _gamma.setRange(0, 3);
                _gamma.setCaptionWidth(0);
                add(&_gamma);
                _clipping.setChanged(
                    [&](int value) { _host->clippingSet(value); });
                _clipping.setText("Clipping: ");
                _clipping.add("None");
                _clipping.add("Separate");
                _clipping.add("Project");
                _clipping.add("Wide");
                _clipping.set(2);
                add(&_clipping);
                _metric.setChanged(
                    [&](int value) { _host->metricSet(value); });
                _metric.setText("Metric: ");
                _metric.add("sRGB");
                _metric.add("linear");
                _metric.add("fast");
                _metric.add("Luv");
                _metric.add("CIE76");
                _metric.add("CIE94");
                _metric.add("CIEDE2000");
                _metric.set(2);
                add(&_metric);
                _characterSet.setChanged(
                    [&](int value) { _host->characterSetSet(value); });
                _characterSet.setText("Character set: ");
                _characterSet.add("0xdd");
                _characterSet.add("0x13/0x55");
                _characterSet.add("1K");
                _characterSet.add("all");
                _characterSet.add("0xb1");
                _characterSet.add("0xb0/0xb1");
                _characterSet.add("ISAV");
                _characterSet.set(3);
                add(&_characterSet);
                _profile.setChanged(
                    [&](int value) { _host->prescalerProfileSet(value); });
                add(&_profile);
            }
            void layout()
            {
                Vector vSpace = _host->vSpace();
                Vector hSpace = _host->hSpace();
                _matchMode.setTopLeft(_host->groupTL());
                int r = _matchMode.right();
                _diffusionHorizontal.setTopLeft(
                    _matchMode.bottomLeft() + vSpace);
                _diffusionVertical.setTopLeft(
                    _diffusionHorizontal.topRight() + hSpace);
                _diffusionTemporal.setTopLeft(
                    _diffusionVertical.topRight() + hSpace);
                r = max(r, _diffusionTemporal.right());
                _quality.setTopLeft(
                    _diffusionHorizontal.bottomLeft() + vSpace);
                _gamma.setTopLeft(_quality.topRight() + hSpace);
                _clipping.setTopLeft(_gamma.topRight() + hSpace);
                _metric.setTopLeft(_clipping.topRight() + hSpace);
                r = max(r, _metric.right());
                _characterSet.setTopLeft(_quality.bottomLeft() + vSpace);
                r = max(r, _characterSet.right());
                int b = _characterSet.bottom();
                _profile.setTopLeft(_characterSet.topRight() + hSpace);
                r = max(r, _profile.right());
                b = max(b, _profile.bottom());
                setInnerSize(Vector(r, b) + _host->groupBR());
                _progressBar.setTopLeft(_matchMode.topRight() + hSpace);
                _progressBar.setInnerSize(Vector(r - _progressBar.topLeft().x,
                    _matchMode.outerSize().y));
            }
            CGAArtWindow* _host;
            ToggleButton _matchMode;
            ProgressBar _progressBar;
            KnobSlider _diffusionHorizontal;
            KnobSlider _diffusionVertical;
            KnobSlider _diffusionTemporal;
            KnobSlider _quality;
            KnobSlider _gamma;
            CaptionedDropDownList _clipping;
            CaptionedDropDownList _metric;
            CaptionedDropDownList _characterSet;
            ProfileDropDown _profile;
        };
        MatchingGroup _matching;
        CGAArtWindow* _host;
    };
    VideoCardGroup _videoCard;
    KnobSliders _knobSliders;

    CGAMatcher* _matcher;
    CGAOutput* _output;
    Program* _program;
    int _paletteSelected;
    int _backgroundSelected;
    ConfigFile* _config;

    friend class OutputWindow;
};

bool endsIn(String s, String suffix)
{
    int l = suffix.length();
    int o = s.length() - l;
    if (o < 0)
        return false;
    for (int i = 0; i < l; ++i)
        if (tolower(s[i + o]) != tolower(suffix[i]))
            return false;
    return true;
}

class BitmapValue : public Structure
{
public:
    void load(String filename)
    {
        _name = filename;
        _file = File(filename, true);
        // We parse the filename relative to the current directory here instead
        // of relative to the config file path because the filename usually
        // comes from the command line.
        Vector size(0, 0);
        _isPNG = endsIn(filename, ".png");
        if (_isPNG) {
            _bitmap = PNGFileFormat<SRGB>().load(_file);
            size = _bitmap.size();
        }
        _size.set("x", size.x, Span());
        _size.set("y", size.y, Span());
    }
    Bitmap<SRGB> bitmap() { return _bitmap; }
    Value getValue(Identifier identifier) const
    {
        if (identifier == Identifier("size"))
            return Value(VectorType(), &_size, Span());
        return Structure::getValue(identifier);
    }
    String name() { return _name; }
    bool operator==(const BitmapValue& other) const
    {
        return _name == other._name;
    }
    bool isPNG() { return _isPNG; }
    File file() { return _file; }
private:
    bool _isPNG;
    Structure _size;
    String _name;
    File _file;
    Bitmap<SRGB> _bitmap;
};

class BitmapType : public StructuredType
{
public:
    BitmapType(BitmapValue* bitmapValue)
      : StructuredType(create<Body>(bitmapValue)) { }
    static String name() { return "Bitmap"; }
    class Body : public StructuredType::Body
    {
    public:
        Body(BitmapValue* bitmapValue)
          : StructuredType::Body("Bitmap", members()),
            _bitmapValue(bitmapValue)
        { }
        List<StructuredType::Member> members()
        {
            List<StructuredType::Member> vectorMembers;
            vectorMembers.add(StructuredType::member<Vector>("size"));
            return vectorMembers;
        }
        bool canConvertFrom(const Type& from, String* reason) const
        {
            return from == StringType();
        }
        Value convert(const Value& value) const
        {
            _bitmapValue->load(value.value<String>());
            return Value(type(), static_cast<Structure*>(_bitmapValue),
                value.span());
        }
        Value defaultValue() const
        {
            return Value(type(), static_cast<Structure*>(_bitmapValue),
                Span());
        }
    private:
        BitmapValue* _bitmapValue;
    };
};

class BitmapIsRGBIFunction : public Function
{
public:
    BitmapIsRGBIFunction(BitmapType bitmapType)
      : Function(create<Body>(bitmapType)) { }
    class Body : public Function::Body
    {
    public:
        Body(BitmapType bitmapType) : _bitmapType(bitmapType) { }
        Value evaluate(List<Value> arguments, Span span) const
        {
            static const SRGB rgbiPalette[16] = {
                SRGB(0x00, 0x00, 0x00), SRGB(0x00, 0x00, 0xaa),
                SRGB(0x00, 0xaa, 0x00), SRGB(0x00, 0xaa, 0xaa),
                SRGB(0xaa, 0x00, 0x00), SRGB(0xaa, 0x00, 0xaa),
                SRGB(0xaa, 0x55, 0x00), SRGB(0xaa, 0xaa, 0xaa),
                SRGB(0x55, 0x55, 0x55), SRGB(0x55, 0x55, 0xff),
                SRGB(0x55, 0xff, 0x55), SRGB(0x55, 0xff, 0xff),
                SRGB(0xff, 0x55, 0x55), SRGB(0xff, 0x55, 0xff),
                SRGB(0xff, 0xff, 0x55), SRGB(0xff, 0xff, 0xff)};

            auto bitmap = static_cast<BitmapValue*>(
                arguments.begin()->value<Structure*>())->bitmap();
            Vector size = bitmap.size();

            int maxDistance = 0;
            const Byte* inputRow = bitmap.data();

            for (int y = 0; y < size.y; ++y) {
                const SRGB* inputPixel =
                    reinterpret_cast<const SRGB*>(inputRow);
                for (int x = 0; x < size.x; ++x) {
                    SRGB s = *inputPixel;
                    ++inputPixel;
                    int bestDistance = 0x7fffffff;
                    Byte bestRGBI = 0;
                    for (int i = 0; i < 16; ++i) {
                        int distance = (Vector3Cast<int>(rgbiPalette[i]) -
                            Vector3Cast<int>(s)).modulus2();
                        if (distance < bestDistance) {
                            bestDistance = distance;
                            if (distance < 42*42)
                                break;
                        }
                    }
                    maxDistance = max(bestDistance, maxDistance);
                }
                inputRow += bitmap.stride();
            }

            return Value(maxDistance < 15*15*3);
        }
        Identifier identifier() const { return "bitmapIsRGBI"; }
        FunctionType type() const
        {
            return FunctionType(BooleanType(), _bitmapType);
        }
    private:
        BitmapType _bitmapType;
    };
};

class Program : public WindowProgram<CGAArtWindow>
{
public:
    void run()
    {
        BitmapValue bitmapValue;
        BitmapType bitmapType(&bitmapValue);

        ConfigFile configFile;
        configFile.addType(VectorType());
        configFile.addOption("inputPicture", bitmapType);
        configFile.addDefaultOption("mode", 0x1a);
        configFile.addDefaultOption("palette", 0x0f);
        configFile.addDefaultOption("interlaceMode", 0);
        configFile.addDefaultOption("interlaceSync", false);
        configFile.addDefaultOption("interlacePhase", false);
        configFile.addDefaultOption("flicker", false);
        configFile.addDefaultOption("scanlinesPerRow", 2);
        configFile.addDefaultOption("scanlinesRepeat", 1);
        configFile.addDefaultOption("contrast", 100.0);
        configFile.addDefaultOption("brightness", 0.0);
        configFile.addDefaultOption("saturation", 100.0);
        configFile.addDefaultOption("hue", 0.0);
        configFile.addDefaultOption("showClipping", false);
        configFile.addDefaultOption("chromaBandwidth", 1.0);
        configFile.addDefaultOption("lumaBandwidth", 1.0);
        configFile.addDefaultOption("rollOff", 0.0);
        configFile.addDefaultOption("lobes", 4.0);
        configFile.addDefaultOption("horizontalDiffusion", 0.647565);
        configFile.addDefaultOption("verticalDiffusion", 0.352435);
        configFile.addDefaultOption("temporalDiffusion", 0.0);
        configFile.addDefaultOption("quality", 0.5);
        configFile.addDefaultOption("gamma", 0.0);
        configFile.addDefaultOption("clipping", 1);
        configFile.addDefaultOption("metric", 2);
        configFile.addDefaultOption("connector", 1);
        configFile.addDefaultOption("characterSet", 3);
        configFile.addDefaultOption("cgaROM", String("5788005.u33"));
        configFile.addDefaultOption("aspectRatio", 5.0/6.0);
        configFile.addDefaultOption("scanlineWidth", 0.5);
        configFile.addDefaultOption("scanlineProfile", 0);
        configFile.addDefaultOption("horizontalProfile", 0);
        configFile.addDefaultOption("prescalerProfile", 4);
        configFile.addDefaultOption("scanlineBleeding", 2);
        configFile.addDefaultOption("horizontalBleeding", 2);
        configFile.addDefaultOption("zoom", 2.0);
        configFile.addDefaultOption("horizontalRollOff", 0.0);
        configFile.addDefaultOption("verticalRollOff", 0.0);
        configFile.addDefaultOption("horizontalLobes", 4.0);
        configFile.addDefaultOption("verticalLobes", 4.0);
        configFile.addDefaultOption("subPixelSeparation", 1.0);
        configFile.addDefaultOption("phosphor", 0);
        configFile.addDefaultOption("mask", 0);
        configFile.addDefaultOption("maskSize", 0.0);
        configFile.addDefaultOption("overscan", 0.1);
        configFile.addDefaultOption("phase", 1);
        configFile.addDefaultOption("interactive", true);
        configFile.addDefaultOption("combFilter", 0);
        configFile.addDefaultOption("fftWisdom", String("wisdom"));
        configFile.addDefaultOption("activeSize", Vector(640, 200));

        configFile.addFunco(BitmapIsRGBIFunction(bitmapType));

        List<Value> arguments;

        if (_arguments.count() < 2) {
            console.write("Syntax: " + _arguments[0] +
                " <input file name>(.png|.dat|.cgad|.config)\n");
            return;
        }
        String configPath = _arguments[1];
        int n = configPath.length();
        if (endsIn(configPath, ".png") || endsIn(configPath, ".dat") ||
            endsIn(configPath, ".cgad")) {
            configPath = "default.config";
            arguments.add(_arguments[0] + " " + configPath);
            for (int i = 1; i < _arguments.count(); ++i)
                arguments.add(_arguments[i]);
        }
        else {
            arguments.add(_arguments[0] + " " + configPath);
            for (int i = 1; i < _arguments.count() - 1; ++i)
                arguments.add(_arguments[i + 1]);
        }

        configFile.addDefaultOption("arguments",
            ArrayType(StringType(), IntegerType()), arguments);

        File config(configPath, true);
        configFile.load(config);

        String fftWisdomFile = configFile.get<String>("fftWisdom");
        FFTWWisdom<float> wisdom(File(fftWisdomFile, config.parent()));

        CGAOutput output(&_data, &_sequencer, &_window);
        _output = &output;
        CGAMatcher matcher;
        _matcher = &matcher;
        matcher.setProgram(this);
        matcher.setData(&_data);
        matcher.setSequencer(&_sequencer);
        _window.setConfig(&configFile);
        _window.setMatcher(_matcher);
        _window.setOutput(&output);
        _window.setProgram(this);

        matcher.setDiffusionHorizontal(
            configFile.get<double>("horizontalDiffusion"));
        matcher.setDiffusionVertical(
            configFile.get<double>("verticalDiffusion"));
        matcher.setDiffusionTemporal(
            configFile.get<double>("temporalDiffusion"));
        matcher.setQuality(configFile.get<double>("quality"));
        matcher.setGamma(configFile.get<double>("gamma"));
        matcher.setClipping(configFile.get<int>("clipping"));
        matcher.setMetric(configFile.get<int>("metric"));
        matcher.setInterlace(configFile.get<int>("interlaceMode"));
        matcher.setInterlaceSync(configFile.get<bool>("interlaceSync"));
        matcher.setInterlacePhase(configFile.get<bool>("interlacePhase"));
        matcher.setFlicker(configFile.get<bool>("flicker"));
        matcher.setCharacterSet(configFile.get<int>("characterSet"));
        matcher.setMode(configFile.get<int>("mode"));
        matcher.setPalette(configFile.get<int>("palette"));
        matcher.setScanlinesPerRow(configFile.get<int>("scanlinesPerRow"));
        int scanlinesRepeat = configFile.get<int>("scanlinesRepeat");
        matcher.setScanlinesRepeat(scanlinesRepeat);
        String cgaROM = configFile.get<String>("cgaROM");
        _sequencer.setROM(File(cgaROM, config.parent()));

        double brightness = configFile.get<double>("brightness");
        output.setBrightness(brightness);
        matcher.setBrightness(brightness);
        double saturation = configFile.get<double>("saturation");
        output.setSaturation(saturation);
        matcher.setSaturation(saturation);
        double hue = configFile.get<double>("hue");
        output.setHue(hue);
        matcher.setHue(hue);
        double contrast = configFile.get<double>("contrast");
        output.setContrast(contrast);
        matcher.setContrast(contrast);
        output.setShowClipping(configFile.get<bool>("showClipping"));
        output.setChromaBandwidth(configFile.get<double>("chromaBandwidth"));
        output.setLumaBandwidth(configFile.get<double>("lumaBandwidth"));
        output.setRollOff(configFile.get<double>("rollOff"));
        output.setLobes(configFile.get<double>("lobes"));
        int connector = configFile.get<int>("connector");
        output.setConnector(connector);
        matcher.setConnector(connector);
        output.setScanlineWidth(configFile.get<double>("scanlineWidth"));
        output.setScanlineProfile(configFile.get<int>("scanlineProfile"));
        output.setHorizontalProfile(configFile.get<int>("horizontalProfile"));
        matcher.setPrescalerProfile(configFile.get<int>("prescalerProfile"));
        output.setZoom(configFile.get<double>("zoom"));
        output.setScanlineBleeding(configFile.get<int>("scanlineBleeding"));
        output.setHorizontalBleeding(
            configFile.get<int>("horizontalBleeding"));
        output.setHorizontalRollOff(
            configFile.get<double>("horizontalRollOff"));
        output.setVerticalRollOff(configFile.get<double>("verticalRollOff"));
        output.setHorizontalLobes(configFile.get<double>("horizontalLobes"));
        output.setVerticalLobes(configFile.get<double>("verticalLobes"));
        output.setSubPixelSeparation(
            configFile.get<double>("subPixelSeparation"));
        output.setPhosphor(configFile.get<int>("phosphor"));
        output.setMask(configFile.get<int>("mask"));
        output.setMaskSize(configFile.get<double>("maskSize"));
        output.setAspectRatio(configFile.get<double>("aspectRatio"));
        double overscan = configFile.get<double>("overscan");
        output.setOverscan(overscan);
        output.setCombFilter(configFile.get<int>("combFilter"));

        Bitmap<SRGB> input = bitmapValue.bitmap();
        Vector activeSize = configFile.get<Vector>("activeSize");
        matcher.setInput(input, activeSize);
        bool isPNG = bitmapValue.isPNG();
        String inputName = bitmapValue.name();
        setMatchMode(isPNG);
        if (!isPNG) {
            File file(inputName, true);
            if (endsIn(inputName, ".cgad")) {
                _data.load(file);
                int regs = -CGAData::registerLogCharactersPerBank;
                Array<Byte> data = _data.getData(-regs, regs);
                matcher.setMode(data[CGAData::registerMode]);
                matcher.setPalette(data[CGAData::registerPalette]);
                matcher.setScanlinesPerRow(1 +
                    data[CGAData::registerMaximumScanline]);
            }
            else {
                _data.loadVRAM(file);
                matcher.setSize(Vector(640, 200));
            }
        }

        beginConvert();

        bool interactive = configFile.get<bool>("interactive");
        if (interactive)
            WindowProgram::run();

        if (!_matchMode)
            matcher.cancel();
        matcher.join();

        String inputFileName = bitmapValue.name();
        int i;
        for (i = inputFileName.length() - 1; i >= 0; --i)
            if (inputFileName[i] == '.')
                break;
        if (i != -1)
            inputFileName = inputFileName.subString(0, i);

        output.save(inputFileName + "_out");
        _data.save(File(inputFileName + "_out.cgad", true));
        _data.saveVRAM(File(inputFileName + "_out.dat", true));
        output.saveRGBI(File(inputFileName + "_out.rgbi", true));
        String s;
        s = "inputPicture = " + enquote(bitmapValue.file().path()) + ";\n";
        s += "cgaROM = " + enquote(cgaROM) + ";\n";
        s += "activeSize = Vector(" + decimal(activeSize.x) + ", " +
            decimal(activeSize.y) + ");\n";
        s += "mode = " + hex(matcher.getMode(), 2) + ";\n";
        s += "palette = " + hex(matcher.getPalette(), 2) + ";\n";
        s += "scanlinesPerRow = " + decimal(matcher.getScanlinesPerRow()) +
            ";\n";
        s += "scanlinesRepeat = " + decimal(matcher.getScanlinesRepeat()) +
            ";\n";
        s += "interlaceMode = " + decimal(matcher.getInterlace()) + ";\n";
        s += "interlaceSync = " + String::Boolean(matcher.getInterlaceSync()) +
            ";\n";
        s += "interlacePhase = " +
            String::Boolean(matcher.getInterlacePhase()) + ";\n";
        s += "flicker = " + String::Boolean(matcher.getFlicker()) + ";\n";
        s += "phase = " + decimal(matcher.getPhase()) + ";\n";
        s += "characterSet = " + decimal(matcher.getCharacterSet()) + ";\n";
        s += "quality = " + format("%6f", matcher.getQuality()) + ";\n";
        s += "horizontalDiffusion = " +
            format("%6f", matcher.getDiffusionHorizontal()) + ";\n";
        s += "verticalDiffusion = " +
            format("%6f", matcher.getDiffusionVertical()) + ";\n";
        s += "temporalDiffusion = " +
            format("%6f", matcher.getDiffusionTemporal()) + ";\n";
        s += "gamma = " + format("%6f", matcher.getGamma()) + ";\n";
        s += "clipping = " + decimal(matcher.getClipping()) + ";\n";
        s += "metric = " + decimal(matcher.getMetric()) + ";\n";
        s += "connector = " + decimal(output.getConnector()) + ";\n";
        s += "contrast = " + format("%6f", matcher.getContrast()) + ";\n";
        s += "brightness = " + format("%6f", matcher.getBrightness()) + ";\n";
        s += "saturation = " + format("%6f", matcher.getSaturation()) + ";\n";
        s += "hue = " + format("%6f", matcher.getHue()) + ";\n";
        s += "chromaBandwidth = " +
            format("%6f", matcher.getChromaBandwidth()) + ";\n";
        s += "lumaBandwidth = " + format("%6f", matcher.getLumaBandwidth()) +
            ";\n";
        s += "rollOff = " + format("%6f", matcher.getRollOff()) + ";\n";
        s += "lobes = " + format("%6f", matcher.getLobes()) + ";\n";
        s += "showClipping = " + String::Boolean(output.getShowClipping()) +
            ";\n";
        s += "combFilter = " + decimal(output.getCombFilter()) + ";\n";
        s += "aspectRatio = " + format("%6f", output.getAspectRatio()) + ";\n";
        s += "scanlineWidth = " + format("%6f", output.getScanlineWidth()) +
            ";\n";
        s += "overscan = " + format("%6f", overscan) + ";\n";
        s += "scanlineProfile = " + decimal(output.getScanlineProfile()) +
            ";\n";
        s += "horizontalProfile = " + decimal(output.getHorizontalProfile()) +
            ";\n";
        s += "prescalerProfile = " + decimal(matcher.getPrescalerProfile()) +
            ";\n";
        s += "scanlineBleeding = " + decimal(output.getScanlineBleeding()) +
            ";\n";
        s += "horizontalBleeding = " +
            decimal(output.getHorizontalBleeding()) + ";\n";
        s += "zoom = " + format("%6f", output.getZoom()) + ";\n";
        s += "horizontalRollOff = " +
            format("%6f", output.getHorizontalRollOff()) + ";\n";
        s += "verticalRollOff = " +
            format("%6f", output.getVerticalRollOff()) + ";\n";
        s += "horizontalLobes = " +
            format("%6f", output.getHorizontalLobes()) + ";\n";
        s += "verticalLobes = " +
            format("%6f", output.getVerticalLobes()) + ";\n";
        s += "subPixelSeparation = " + format("%6f",
            output.getSubPixelSeparation()) + ";\n";
        s += "phosphor = " + decimal(output.getPhosphor()) + ";\n";
        s += "mask = " + decimal(output.getMask()) + ";\n";
        s += "maskSize = " + format("%6f", output.getMaskSize()) + ";\n";
        s += "interactive = " + String::Boolean(interactive) + ";\n";
        s += "fftWisdom = " + enquote(fftWisdomFile) + ";\n";
        File(inputFileName + "_out.config", true).save(s);
    }
    bool getMatchMode() { return _matchMode; }
    void setMatchMode(bool matchMode) { _matchMode = matchMode; }
    void updateOutput()
    {
        _updateNeeded = true;
        _interruptMessageLoop.signal();
    }
    void setProgress(float progress) { _window.setProgress(progress); }
    bool idle()
    {
        if (_updateNeeded) {
            _updateNeeded = false;
            _output->restart();
        }
        return false;
    }
    void beginConvert()
    {
        if (_matchMode)
            _matcher->restart();
        else
            _output->restart();
    }
private:
    CGAData _data;
    CGAMatcher* _matcher;
    CGASequencer _sequencer;
    CGAOutput* _output;
    bool _matchMode;
    bool _updateNeeded;
};
