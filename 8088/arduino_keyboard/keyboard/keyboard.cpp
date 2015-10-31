#include "alfe/main.h"
#include "alfe/file.h"

template<class T> class KeyboardWindowTemplate;
typedef KeyboardWindowTemplate<void> KeyboardWindow;

template<class T> class KeyboardWindowTemplate : public RootWindow
{
public:
    void setProgram(Program* program)
    {
        _program = program;
        _lShift = false;
        _rShift = false;
    }

    void create()
    {
        RootWindow::create();
        setText("Virtual keyboard");
    }

    bool keyboardEvent(int key, bool up)
    {
        int scanCode = 0;
        switch (key) {
            case VK_BACK:     scanCode = 0x0e; break;
            case VK_TAB:      scanCode = 0x0f; break;
            case VK_RETURN:   scanCode = 0x1c; break;
            case VK_CONTROL:  scanCode = 0x1d; break;
            case VK_MENU:     scanCode = 0x38; break;
            case VK_CAPITAL:  scanCode = 0x3a; break;
            case VK_ESCAPE:   scanCode = 0x01; break;
            case VK_SPACE:    scanCode = 0x39; break;
            case VK_PRIOR:    scanCode = 0x49; break;
            case VK_NEXT:     scanCode = 0x51; break;
            case VK_END:      scanCode = 0x4f; break;
            case VK_HOME:     scanCode = 0x47; break;
            case VK_LEFT:     scanCode = 0x4b; break;
            case VK_UP:       scanCode = 0x48; break;
            case VK_RIGHT:    scanCode = 0x4d; break;
            case VK_DOWN:     scanCode = 0x50; break;
            case VK_SNAPSHOT: scanCode = 0x37; break;
            case VK_INSERT:   scanCode = 0x52; break;
            case VK_DELETE:   scanCode = 0x53; break;
            case '0':         scanCode = 0x0b; break;
            case '1':         scanCode = 0x02; break;
            case '2':         scanCode = 0x03; break;
            case '3':         scanCode = 0x04; break;
            case '4':         scanCode = 0x05; break;
            case '5':         scanCode = 0x06; break;
            case '6':         scanCode = 0x07; break;
            case '7':         scanCode = 0x08; break;
            case '8':         scanCode = 0x09; break;
            case '9':         scanCode = 0x0a; break;
            case 'A':         scanCode = 0x1e; break;
            case 'B':         scanCode = 0x30; break;
            case 'C':         scanCode = 0x2e; break;
            case 'D':         scanCode = 0x20; break;
            case 'E':         scanCode = 0x12; break;
            case 'F':         scanCode = 0x21; break;
            case 'G':         scanCode = 0x22; break;
            case 'H':         scanCode = 0x23; break;
            case 'I':         scanCode = 0x17; break;
            case 'J':         scanCode = 0x24; break;
            case 'K':         scanCode = 0x25; break;
            case 'L':         scanCode = 0x26; break;
            case 'M':         scanCode = 0x32; break;
            case 'N':         scanCode = 0x31; break;
            case 'O':         scanCode = 0x18; break;
            case 'P':         scanCode = 0x19; break;
            case 'Q':         scanCode = 0x10; break;
            case 'R':         scanCode = 0x13; break;
            case 'S':         scanCode = 0x1f; break;
            case 'T':         scanCode = 0x14; break;
            case 'U':         scanCode = 0x16; break;
            case 'V':         scanCode = 0x2f; break;
            case 'W':         scanCode = 0x11; break;
            case 'X':         scanCode = 0x2d; break;
            case 'Y':         scanCode = 0x15; break;
            case 'Z':         scanCode = 0x2c; break;
            case VK_NUMPAD0:  scanCode = 0x52; break;
            case VK_NUMPAD1:  scanCode = 0x4f; break;
            case VK_NUMPAD2:  scanCode = 0x50; break;
            case VK_NUMPAD3:  scanCode = 0x51; break;
            case VK_NUMPAD4:  scanCode = 0x4b; break;
            case VK_NUMPAD5:  scanCode = 0x4c; break;
            case VK_NUMPAD6:  scanCode = 0x4d; break;
            case VK_NUMPAD7:  scanCode = 0x47; break;
            case VK_NUMPAD8:  scanCode = 0x48; break;
            case VK_NUMPAD9:  scanCode = 0x49; break;
            case VK_MULTIPLY: scanCode = 0x37; break;
            case VK_ADD:      scanCode = 0x4e; break;
            case VK_SUBTRACT: scanCode = 0x4a; break;
            case VK_DECIMAL:  scanCode = 0x53; break;
            case VK_DIVIDE:   scanCode = 0x35; break;
            case VK_F1:       scanCode = 0x3b; break;
            case VK_F2:       scanCode = 0x3c; break;
            case VK_F3:       scanCode = 0x3d; break;
            case VK_F4:       scanCode = 0x3e; break;
            case VK_F5:       scanCode = 0x3f; break;
            case VK_F6:       scanCode = 0x40; break;
            case VK_F7:       scanCode = 0x41; break;
            case VK_F8:       scanCode = 0x42; break;
            case VK_F9:       scanCode = 0x43; break;
            case VK_F10:      scanCode = 0x44; break;
            case VK_F11:      // Ctrl+Alt+Del
                if (up) {
                    _program->sendByte(0x1d);
                    _program->sendByte(0x38);
                    _program->sendByte(0x53);
                }
                else {
                    _program->sendByte(0x9d);
                    _program->sendByte(0xb8);
                    _program->sendByte(0xd3);
                }
                return false;
            case VK_F12:      // Power cycle
                if (up)
                    return false;
                scanCode = 0x77;
                break;
            case VK_NUMLOCK:  scanCode = 0x45; break;
            case VK_SCROLL:   scanCode = 0x46; break;
            case VK_SHIFT:
                {
                    bool lShift = ((GetKeyState(VK_LSHIFT) & 0x80000000) != 0);
                    bool rShift = ((GetKeyState(VK_RSHIFT) & 0x80000000) != 0);
                    if (lShift != _lShift) {
                        _program->sendByte(lShift ? 0x2a : 0xaa);
                        _lShift = lShift;
                    }
                    if (rShift != _rShift) {
                        _program->sendByte(rShift ? 0x36 : 0xb6);
                        _rShift = rShift;
                    }
                }
                return false;
            case VK_OEM_1:    scanCode = 0x27; break;
            case VK_OEM_PLUS: scanCode = 0x0d; break;
            case VK_OEM_COMMA: scanCode = 0x33; break;
            case VK_OEM_MINUS: scanCode = 0x0c; break;
            case VK_OEM_PERIOD: scanCode = 0x34; break;
            case VK_OEM_2:    scanCode = 0x35; break;
            case VK_OEM_3:    scanCode = 0x29; break;
            case VK_OEM_4:    scanCode = 0x1a; break;
            case VK_OEM_5:    scanCode = 0x2b; break;
            case VK_OEM_6:    scanCode = 0x1b; break;
            case VK_OEM_7:    scanCode = 0x28; break;
            default: return false;
        }
        _program->sendByte(scanCode | (up ? 0x80 : 0));
        return false;
    }
    Program* _program;
    bool _lShift;
    bool _rShift;
};

class Program : public WindowProgram<KeyboardWindow>
{
public:
    void run()
    {
        _com = AutoStream(CreateFile(
            L"COM3",
            GENERIC_READ | GENERIC_WRITE,
            0,              // must be opened with exclusive-access
            NULL,           // default security attributes
            OPEN_EXISTING,  // must use OPEN_EXISTING
            0,              // not overlapped I/O
            NULL));         // hTemplate must be NULL for comm devices

        DCB deviceControlBlock;
        SecureZeroMemory(&deviceControlBlock, sizeof(DCB));

        IF_ZERO_THROW(GetCommState(_com, &deviceControlBlock));

        deviceControlBlock.DCBlength = sizeof(DCB);
        deviceControlBlock.BaudRate = 115200;
        //deviceControlBlock.fBinary = TRUE;
        //deviceControlBlock.fParity = FALSE;
        deviceControlBlock.fOutxCtsFlow = FALSE;
        deviceControlBlock.fOutxDsrFlow = FALSE;
        // DTR_CONTROL_ENABLE causes Arduino to reset on connect
        //deviceControlBlock.fDtrControl = DTR_CONTROL_ENABLE;
        deviceControlBlock.fDtrControl = DTR_CONTROL_DISABLE;
        //deviceControlBlock.fDsrSensitivity = FALSE;
        //deviceControlBlock.fTXContinueOnXoff = TRUE;
        deviceControlBlock.fOutX = TRUE;
        deviceControlBlock.fInX = TRUE;
        //deviceControlBlock.fErrorChar = FALSE;
        deviceControlBlock.fNull = FALSE;
        //deviceControlBlock.fRtsControl = RTS_CONTROL_DISABLE;
        //deviceControlBlock.fAbortOnError = FALSE;
        //deviceControlBlock.wReserved = 0;
        deviceControlBlock.ByteSize = 8;
        deviceControlBlock.Parity = NOPARITY;
        deviceControlBlock.StopBits = ONESTOPBIT;
        deviceControlBlock.XonChar = 17;
        deviceControlBlock.XoffChar = 19;

        IF_ZERO_THROW(SetCommState(_com, &deviceControlBlock));

        sendByte(0x7f);      // Put Arduino in raw mode

        _window.setProgram(this);
        WindowProgram::run();
    }

    void sendByte(Byte value)
    {
        // Escape for XON/XOFF
        if (value == 0 || value == 17 || value == 19)
            _com.write<Byte>(0);
        _com.write<Byte>(value);
    }

private:
    Stream _com;
};
