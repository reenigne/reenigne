#include "alfe/main.h"
#include "alfe/file.h"
#include "alfe/thread.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        if (_arguments.count() == 1) {
            console.write("Usage: run [-c] [-r] <name of file to send>\n");
            console.write("-c for .com file (0x100 offset)\n");
            console.write("-r to reset XT first\n");
            return;
        }
        int fileNameArgument = 1;
        bool comFile = false;
        if (_arguments[1] == String("-c")) {
            comFile = true;
            fileNameArgument = 2;
        }
        bool reset = false;
        if (_arguments[fileNameArgument] == String("-r")) {
            reset = true;
            ++fileNameArgument;
        }
        String fileName = _arguments[fileNameArgument];
        String data = File(fileName, true).contents();
        int l = data.length();

        _com = AutoHandle(CreateFile(
            L"COM2",
            GENERIC_READ | GENERIC_WRITE,
            0,              // must be opened with exclusive-access
            NULL,           // default security attributes
            OPEN_EXISTING,  // must use OPEN_EXISTING
            0,              // not overlapped I/O
            NULL),          // hTemplate must be NULL for comm devices
            String("COM port"));

        //IF_ZERO_THROW(SetupComm(_com, 1024, 1024));

        DCB deviceControlBlock;
        SecureZeroMemory(&deviceControlBlock, sizeof(DCB));
        IF_ZERO_THROW(GetCommState(_com, &deviceControlBlock));
        deviceControlBlock.DCBlength = sizeof(DCB);
        deviceControlBlock.BaudRate = 115200;
        deviceControlBlock.fBinary = TRUE;
        deviceControlBlock.fParity = FALSE;
        deviceControlBlock.fOutxCtsFlow = FALSE;
        deviceControlBlock.fOutxDsrFlow = FALSE;
        // DTR_CONTROL_ENABLE causes Arduino to reset on connect
        deviceControlBlock.fDtrControl = DTR_CONTROL_ENABLE;
        //deviceControlBlock.fDtrControl = DTR_CONTROL_DISABLE;
        deviceControlBlock.fDsrSensitivity = FALSE;
        deviceControlBlock.fTXContinueOnXoff = TRUE;
        deviceControlBlock.fOutX = FALSE; //TRUE;
        deviceControlBlock.fInX = FALSE; //TRUE;
        deviceControlBlock.fErrorChar = FALSE;
        deviceControlBlock.fNull = FALSE;
        deviceControlBlock.fRtsControl = RTS_CONTROL_DISABLE;
        deviceControlBlock.fAbortOnError = TRUE;
        deviceControlBlock.wReserved = 0;
        deviceControlBlock.XonLim = 1;
        deviceControlBlock.XoffLim = 1;
        deviceControlBlock.ByteSize = 8;
        deviceControlBlock.Parity = NOPARITY;
        deviceControlBlock.StopBits = ONESTOPBIT;
        deviceControlBlock.XonChar = 17;
        deviceControlBlock.XoffChar = 19;
        IF_ZERO_THROW(SetCommState(_com, &deviceControlBlock));

        //COMMTIMEOUTS timeOuts;
        //SecureZeroMemory(&timeOuts, sizeof(COMMTIMEOUTS));
        //timeOuts.ReadIntervalTimeout = 0;
        //timeOuts.ReadTotalTimeoutMultiplier = 0;
        //timeOuts.ReadTotalTimeoutConstant = 0;
        //IF_ZERO_THROW(SetCommTimeouts(_com, &timeOuts));

        //IF_ZERO_THROW(SetCommMask(_com, EV_RXCHAR));

        COMMTIMEOUTS timeOuts;
        SecureZeroMemory(&timeOuts, sizeof(COMMTIMEOUTS));
        timeOuts.ReadIntervalTimeout = 10*1000;
        timeOuts.ReadTotalTimeoutMultiplier = 0;
        timeOuts.ReadTotalTimeoutConstant = 10*1000;
        timeOuts.WriteTotalTimeoutConstant = 10*1000;
        timeOuts.WriteTotalTimeoutMultiplier = 0;
        IF_ZERO_THROW(SetCommTimeouts(_com, &timeOuts));

        //IF_ZERO_THROW(ClearCommBreak(_com));
        //IF_ZERO_THROW(PurgeComm(_com, PURGE_RXCLEAR | PURGE_TXCLEAR));
        //IF_ZERO_THROW(FlushFileBuffers(_com));
        //DWORD error;
        //IF_ZERO_THROW(ClearCommError(_com, &error, NULL));

        //_com.set(CreateFile(L"run.output", GENERIC_WRITE, 0, NULL,
        //    CREATE_ALWAYS, 0, NULL));

        // Reset the Arduino
        EscapeCommFunction(_com, CLRDTR);
        //EscapeCommFunction(_com, CLRRTS);
        Sleep(250);
        EscapeCommFunction(_com, SETDTR);
        //EscapeCommFunction(_com, SETRTS);
        // The Arduino bootloader waits a bit to see if it needs to
        // download a new program.

        expect(">");

        if (reset) {
            IF_ZERO_THROW(FlushFileBuffers(_com));
            IF_ZERO_THROW(PurgeComm(_com, PURGE_RXCLEAR | PURGE_TXCLEAR));
            _com.write<Byte>(0x7f);
            _com.write<Byte>(0x77);
            //IF_ZERO_THROW(FlushFileBuffers(_com));

            expect("resetting");
        }

        //int rate = 115200;
        //deviceControlBlock.BaudRate = rate;
        //int baudDivisor = static_cast<int>(2000000.0 / rate + 0.5);
        //sendByte(0x7f);
        //sendByte(0x7c);
        //sendByte(0x04);
        //sendByte((baudDivisor - 1) & 0xff);
        //sendByte((baudDivisor - 1) >> 8);
        //IF_ZERO_THROW(FlushFileBuffers(_com));
        //IF_ZERO_THROW(SetCommState(_com, &deviceControlBlock));
        //Sleep(2000);

        //ReaderThread thread(this);
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
        console.write(hex(l) + "\n");
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
            //if ((i & 0xff) == 0)
            //    console.write(".");
        }
        addByte(checkSum);
        flush();
        IF_ZERO_THROW(FlushFileBuffers(_com));

        console.write("Upload complete.\n");
        // Dump bytes from COM port to stdout until we receive ^Z
        //thread.join();

        int i = 0;
        do {
            int b = _com.tryReadByte();
            if (b != -1 && b != 17 && b != 19) {
                console.write(String(hex(b,2)) + " ");
                i = 0;
            }
            if (b == 26)
                return;
            ++i;
        } while (true);
    }
private:
    void expect(int c)
    {
        int i = 0;
        do {
            int b = _com.tryReadByte();
            if (b != -1 && b != 17 && b != 19) {
                console.write<Byte>(b);
                i = 0;
            }
            if (b == c)
                return;
            ++i;
        } while (true);

        throw Exception("No response from QuickBoot");
    }
    void expect(String s)
    {
        for (int i = 0; i < s.length(); ++i)
            expect(s[i]);
    }

    //class ReaderThread : public Thread
    //{
    //public:
    //    ReaderThread(Program* program) : _program(program) { }
    //    void threadProc()
    //    {
    //        int c = 0;
    //        do {
    //            DWORD eventMask = 0;
    //            if (WaitCommEvent(_program->_com, &eventMask, NULL) == 0)
    //                throw Exception::systemError(String("Reading COM port"));
    //            do {
    //                c = _program->_com.tryReadByte();
    //                if (c == 26 || c == -1)
    //                    break;
    //                console.write<Byte>(c);
    //            } while (true);
    //        } while (c != 26);
    //    }
    //private:
    //    Program* _program;
    //};

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
        _bufferCount2 = 0;
        Byte checkSum = _bufferCount;
        addByte2(0x75);            // "Send raw data" command
        addByte2(_bufferCount);    // Send length
        for (int i = 0; i < _bufferCount; ++i) {
            Byte b = _buffer[i];
            addByte2(b);  // Send data byte
            checkSum += b;
        }
        addByte2(checkSum);
        _com.write(reinterpret_cast<const void*>(&_buffer2[0]), _bufferCount2);
        _bufferCount = 0;

        expect("K");
    }

    void addByte2(Byte value)
    {
        if (value == 0 || value == 17 || value == 19) {
            _buffer2[_bufferCount2] = 0;
            ++_bufferCount2;
        }
        _buffer2[_bufferCount2] = value;
        ++_bufferCount2;
    }

    void sendByte(Byte value)
    {
        // Escape for XON/XOFF
        if (value == 0 || value == 17 || value == 19)
            _com.write<Byte>(0);
        _com.write<Byte>(value);
    }

    Handle _com;
    Byte _buffer[0xff+3];
    Byte _buffer2[(0xff+3)*2];
    int _bufferCount;
    int _bufferCount2;

    friend class ReaderThread;
};
