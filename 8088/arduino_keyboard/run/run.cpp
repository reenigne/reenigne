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

        Handle console = Handle::consoleOutput();

        sendByte(0x7f);      // Put Arduino in raw mode
        sendByte(0x76);      // Clear keyboard buffer
        sendByte(0x72);      // Put Arduino in tester mode

        // The buffer in the Arduino only holds 255 bytes, so we have to send
        // it in chunks. We do this by buffering the data on the host PC side,
        // and sending the buffer when it's full or when we're done.
        _bufferCount = 0;
        // When running a .com file, we need the instruction pointer to start
        // at 0x100. We do this by prepending 0x100 NOP bytes at the beginning.
        // In DOS this area would contain the Program Segment Prefix structure.
        (String::hexadecimal(l, 8) + String("\n")).write(console);
        Byte checkSum = 0;
        if (comFile) {
            addLength(l + 0x100);
            for (int i = 0; i < 100; ++i) {
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
                String(".").write(console);
        }
        flush();

        // Dump bytes from COM port to stdout until we receive ^Z
        String("Upload complete.\n").write(console);
        do {
            int c = _com.tryReadByte();
            if (c == 26)
                break;
            console.write<Byte>(c);
        } while (true);
    }
private:
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
        sendByte(0x75);           // "Send raw data" command
        sendByte(_bufferCount);   // Send length
        for (int i = 0; i < _bufferCount; ++i)
            sendByte(_buffer[i]);  // Send data byte
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