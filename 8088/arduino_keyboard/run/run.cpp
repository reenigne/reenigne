#include "unity/main.h"
#include "unity/file.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        if (_arguments.count() == 1) {
            String("Usage: run [-c] <name of file to send>\n").
                write(Handle::consoleOutput());
            return;
        }
        int fileNameArgument = 1;
        bool comFile = false;
        if (_arguments[1] == String("-c"))
            comFile = true;
        String fileName = _arguments[fileNameArgument];
        String data = File(fileName).contents();
        int l = data.length();
        if (l > 0x400) {
            (String("Error: ") + fileName + String(" is ") + String::decimal(l)
                + String(" bytes (must be less than 1024).\n")).
                    write(Handle::consoleOutput());
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

        sendByte(0x7f);      // Put Arduino in raw mode

        // The buffer in the Arduino only holds 255 bytes, so we have to send
        // it in chunks. We do this by buffering the data on the host PC side,
        // and sending the buffer when it's full or when we're done.
        _bufferCount = 0;
        // When running a .com file, we need the instruction pointer to start
        // at 0x100. We do this by prepending 0x100 bytes at the beginning. In
        // DOS this area would contain the Program Segment Prefix structure.
        if (comFile)
            l += 0x100;
        addByte(l & 0xff);          // Send length low byte
        addByte((l >> 8) & 0xff);   // Send length middle byte
        addByte((l >> 16) & 0xff);  // Send length high byte
        if (comFile) {
            for (int i = 0; i < l; ++i)
                addByte(0);
            l -= 0x100;
        }
        for (int i = 0; i < l; ++i)
            addByte(data[i]);       // Send data byte
        flush();

        // Dump bytes from COM port to stdout until we receive ^Z
        Handle console = Handle::consoleOutput();
        do {
            Byte c = _com.read<Byte>();
            if (c == 26)
                break;
            console.write<Byte>(c);
        } while (true);
    }
private:
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
        sendByte(0x75);           // "Set RAM program" command
        sendByte(_bufferCount);   // Send length
        for (int i = 0; i < _bufferCount; ++i)
            sendByte(_buffer[i];  // Send data byte
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
};