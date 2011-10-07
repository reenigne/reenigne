#include "unity/main.h"
#include "unity/file.h"
#include "unity/thread.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        if (_arguments.count() == 1) {
            _console.write(String("Usage: send <name of file to send>\n"));
            return;
        }
        String fileName = _arguments[1];
        String data = File(fileName).contents();
        int l = data.length();
        if (l > 0x400) {
            _console.write(String("Error: ") + fileName + String(" is ") +
                String::decimal(l) +
                String(" bytes (must be less than 1024).\n"));
            return;
        }

        _com.set(CreateFile(
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
        deviceControlBlock.BaudRate = CBR_9600;
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
        ReaderThread thread(this);
        thread.start();

//        Sleep(2000);

        sendByte(0x7f);      // Put Arduino in raw mode
        sendByte(0x76);      // Clear keyboard buffer
        sendByte(0x72);      // "Set tester mode" command
        sendByte(0x73);      // "Set RAM program" command
        sendByte(l & 0xff);  // Send low byte of length
        sendByte(l >> 8);    // Send high byte of length
        for (int i = 0; i < l; ++i)
            sendByte(data[i]);  // Send program byte
        String("Send complete.\n").write(_console);
        thread.join();
    }
private:
    class ReaderThread : public Thread
    {
    public:
        ReaderThread(Program* program) : _program(program) { }
        void threadProc()
        {
            do {
                DWORD eventMask;
                if (WaitCommEvent(_program->_com, &eventMask, NULL) == 0)
                    throw Exception::systemError(String("Reading COM port"));
                if ((eventMask & EV_RXCHAR) != 0) {
                    int c = _program->_com.read<Byte>();
                    if (c == 26)
                        break;
                    _program->_console.write<Byte>(c);
                }
            } while (true);
        }
    private:
        Program* _program;
    };

    void sendByte(Byte value)
    {
        // Escape for XON/XOFF
        if (value == 0 || value == 17 || value == 19)
            _com.write<Byte>(0);
        _com.write<Byte>(value);
    }

    AutoHandle _com;
};