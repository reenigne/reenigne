#include "unity/main.h"
#include "unity/file.h"
#include "unity/thread.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        if (_arguments.count() == 1) {
            _console.write(String("Usage: run [-c] <name of file to send>\n"));
            return;
        }
        int fileNameArgument = 1;
        bool comFile = false;
        if (_arguments[1] == String("-c")) {
            comFile = true;
            fileNameArgument = 2;
        }
        String fileName = _arguments[fileNameArgument];
        String data = File(fileName).contents();
        int l = data.length();

        _com.set(CreateFile(
            L"COM3",
            GENERIC_READ | GENERIC_WRITE,
            0,              // must be opened with exclusive-access
            NULL,           // default security attributes
            OPEN_EXISTING,  // must use OPEN_EXISTING
            0,              // not overlapped I/O
            NULL),          // hTemplate must be NULL for comm devices
            String("COM port"));

        DCB deviceControlBlock;
        SecureZeroMemory(&deviceControlBlock, sizeof(DCB));

        IF_ZERO_THROW(GetCommState(_com, &deviceControlBlock));

        deviceControlBlock.DCBlength = sizeof(DCB);
        deviceControlBlock.BaudRate = 19200;
        //deviceControlBlock.fBinary = TRUE;
        //deviceControlBlock.fParity = FALSE;
        deviceControlBlock.fOutxCtsFlow = FALSE;
        deviceControlBlock.fOutxDsrFlow = FALSE;
        // DTR_CONTROL_ENABLE causes Arduino to reset on connect
        //deviceControlBlock.fDtrControl = DTR_CONTROL_ENABLE;
        deviceControlBlock.fDtrControl = DTR_CONTROL_DISABLE;
        //deviceControlBlock.fDsrSensitivity = FALSE;
        //deviceControlBlock.fTXContinueOnXoff = TRUE;
        deviceControlBlock.fOutX = FALSE;  // TRUE
        deviceControlBlock.fInX = FALSE;   // TRUE
        //deviceControlBlock.fErrorChar = FALSE;
        deviceControlBlock.fNull = FALSE;
        //deviceControlBlock.fRtsControl = RTS_CONTROL_DISABLE;
        deviceControlBlock.fAbortOnError = TRUE;
        //deviceControlBlock.wReserved = 0;
        deviceControlBlock.ByteSize = 8;
        deviceControlBlock.Parity = NOPARITY;
        deviceControlBlock.StopBits = ONESTOPBIT;
        //deviceControlBlock.XonChar = 17;
        //deviceControlBlock.XoffChar = 19;

        IF_ZERO_THROW(SetCommState(_com, &deviceControlBlock));
        IF_ZERO_THROW(SetCommMask(_com, EV_RXCHAR));
        //IF_ZERO_THROW(ClearCommBreak(_com));
        //IF_ZERO_THROW(PurgeComm(_com, PURGE_RXCLEAR | PURGE_TXCLEAR));
        //IF_ZERO_THROW(FlushFileBuffers(_com));
        //DWORD error;
        //IF_ZERO_THROW(ClearCommError(_com, &error, NULL));

        //_com.set(CreateFile(L"run.output", GENERIC_WRITE, 0, NULL,
        //    CREATE_ALWAYS, 0, NULL));

        Sleep(2000);

        ReaderThread thread(this);
        //thread.start();

        sendByte(0x7f);      // Put Arduino in raw mode
        sendByte(0x76);      // Clear keyboard buffer
        sendByte(0x72);      // Put Arduino in tester mode
        //sendByte(0x78);      // Put Arduino in tester raw mode

        // The buffer in the Arduino only holds 255 bytes, so we have to send
        // it in chunks. We do this by buffering the data on the host PC side,
        // and sending the buffer when it's full or when we're done.
        _bufferCount = 0;
        // When running a .com file, we need the instruction pointer to start
        // at 0x100. We do this by prepending 0x100 NOP bytes at the beginning.
        // In DOS this area would contain the Program Segment Prefix structure.
        _console.write(String::hexadecimal(l, 8) + String("\n"));
        Byte checkSum = 0;
        if (comFile) {
            addLength(l + 0x100);
            for (int i = 0; i < 0x100; ++i) {
                addByte(0x90);
                checkSum += 0x90;
            }
        }
        else
            addLength(l);
        for (int i = 0; i < l; ++i) {
            addByte(data[i]);       // Send data byte
            checkSum += data[i];
            if ((i & 0xff) == 0)
                _console.write(dot);
        }
        addByte(checkSum);
        flush();
        IF_ZERO_THROW(FlushFileBuffers(_com));

        _console.write(String("Upload complete.\n"));
        // Dump bytes from COM port to stdout until we receive ^Z
        //thread.join();
    }
private:
    class ReaderThread : public Thread
    {
    public:
        ReaderThread(Program* program) : _program(program) { }
        void threadProc()
        {
            int c = 0;
            do {
                DWORD eventMask = 0;
                if (WaitCommEvent(_program->_com, &eventMask, NULL) == 0)
                    throw Exception::systemError(String("Reading COM port"));
                do {
                    c = _program->_com.tryReadByte();
                    if (c == 26 || c == -1)
                        break;
                    _program->_console.write<Byte>(c);
                } while (true);
            } while (c != 26);
        }
    private:
        Program* _program;
    };

    void addLength(int l)
    {
        addByte(l & 0xff);          // Send length low byte
        addByte((l >> 8) & 0xff);   // Send length middle byte
        addByte((l >> 16) & 0xff);  // Send length high byte
    }
    void addByte(Byte value)
    {
        if (_bufferCount == 0xff)
            flush();
        _buffer[_bufferCount] = value;
        ++_bufferCount;
    }
    void flush()
    {
        if (_bufferCount == 0)
            return;
        Byte checkSum = _bufferCount;
        sendByte(0x75);           // "Send raw data" command
        sendByte(_bufferCount);   // Send length
        for (int i = 0; i < _bufferCount; ++i) {
            sendByte(_buffer[i]);  // Send data byte
            checkSum += _buffer[i];
        }
        sendByte(checkSum);
        _bufferCount = 0;
    }

    void sendByte(Byte value)
    {
        // Escape for XON/XOFF
        if (value == 0 || value == 17 || value == 19)
            _com.write<Byte>(0);
        _com.write<Byte>(value);
    }

    AutoHandle _com;
    Byte _buffer[0xff];
    int _bufferCount;

    friend class ReaderThread;
};