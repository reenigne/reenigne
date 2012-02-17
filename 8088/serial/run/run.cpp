#include "unity/main.h"
#include "unity/file.h"
#include "unity/thread.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        if (_arguments.count() == 1) {
            _console.write("Usage: run [-c] <name of file to send>\n");
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

        //// Reset the machine
        //{
        //    AutoHandle arduinoCom;
        //    arduinoCom.set(CreateFile(
        //        L"COM3",
        //        GENERIC_READ | GENERIC_WRITE,
        //        0,              // must be opened with exclusive-access
        //        NULL,           // default security attributes
        //        OPEN_EXISTING,  // must use OPEN_EXISTING
        //        0,              // not overlapped I/O
        //        NULL),          // hTemplate must be NULL for comm devices
        //        String("Arduino COM port"));

        //    DCB deviceControlBlock;
        //    SecureZeroMemory(&deviceControlBlock, sizeof(DCB));
        //    IF_ZERO_THROW(GetCommState(arduinoCom, &deviceControlBlock));
        //    deviceControlBlock.DCBlength = sizeof(DCB);
        //    deviceControlBlock.BaudRate = 19200;
        //    deviceControlBlock.fBinary = TRUE;
        //    deviceControlBlock.fParity = FALSE;
        //    deviceControlBlock.fOutxCtsFlow = FALSE;
        //    deviceControlBlock.fOutxDsrFlow = FALSE;
        //    deviceControlBlock.fDtrControl = DTR_CONTROL_DISABLE;
        //    deviceControlBlock.fDsrSensitivity = FALSE;
        //    deviceControlBlock.fTXContinueOnXoff = TRUE;
        //    deviceControlBlock.fOutX = TRUE;
        //    deviceControlBlock.fInX = TRUE;
        //    deviceControlBlock.fErrorChar = FALSE;
        //    deviceControlBlock.fNull = FALSE;
        //    deviceControlBlock.fRtsControl = RTS_CONTROL_DISABLE;
        //    deviceControlBlock.fAbortOnError = TRUE;
        //    deviceControlBlock.wReserved = 0;
        //    deviceControlBlock.ByteSize = 8;
        //    deviceControlBlock.Parity = NOPARITY;
        //    deviceControlBlock.StopBits = ONESTOPBIT;
        //    deviceControlBlock.XonChar = 17;
        //    deviceControlBlock.XoffChar = 19;
        //    IF_ZERO_THROW(SetCommState(arduinoCom, &deviceControlBlock));

        //    IF_ZERO_THROW(SetCommMask(arduinoCom, EV_RXCHAR));

        //    Sleep(2000);

        //    arduinoCom.write<Byte>(0x72);
        //    arduinoCom.write<Byte>(0x7f);
        //    IF_ZERO_THROW(FlushFileBuffers(arduinoCom));

        //}

        //Sleep(3000);

        _com.set(CreateFile(
            L"COM1",
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
        deviceControlBlock.BaudRate = 115200; //38400;
        deviceControlBlock.fBinary = TRUE;
        deviceControlBlock.fParity = FALSE;
        deviceControlBlock.fOutxCtsFlow = FALSE;
        deviceControlBlock.fOutxDsrFlow = TRUE;
        //deviceControlBlock.fDtrControl = DTR_CONTROL_ENABLE;
        deviceControlBlock.fDtrControl = DTR_CONTROL_HANDSHAKE;
        deviceControlBlock.fDsrSensitivity = FALSE; //TRUE;
        deviceControlBlock.fTXContinueOnXoff = TRUE;
        deviceControlBlock.fOutX = FALSE;
        deviceControlBlock.fInX = FALSE;
        deviceControlBlock.fErrorChar = FALSE;
        deviceControlBlock.fNull = FALSE;
        deviceControlBlock.fRtsControl = RTS_CONTROL_DISABLE;
        deviceControlBlock.fAbortOnError = TRUE;
        deviceControlBlock.wReserved = 0;
        deviceControlBlock.ByteSize = 8;
        deviceControlBlock.Parity = NOPARITY;
        deviceControlBlock.StopBits = ONESTOPBIT;
        deviceControlBlock.XonChar = 17;
        deviceControlBlock.XoffChar = 19;
        IF_ZERO_THROW(SetCommState(_com, &deviceControlBlock));

        COMMTIMEOUTS timeOuts;
        SecureZeroMemory(&timeOuts, sizeof(COMMTIMEOUTS));
        timeOuts.ReadIntervalTimeout = MAXDWORD;
        timeOuts.ReadTotalTimeoutMultiplier = 0;
        timeOuts.ReadTotalTimeoutConstant = MAXDWORD;
        IF_ZERO_THROW(SetCommTimeouts(_com, &timeOuts));

        IF_ZERO_THROW(SetCommMask(_com, EV_RXCHAR));
        //IF_ZERO_THROW(ClearCommBreak(_com));
        //IF_ZERO_THROW(PurgeComm(_com, PURGE_RXCLEAR | PURGE_TXCLEAR));
        //IF_ZERO_THROW(FlushFileBuffers(_com));
        //DWORD error;
        //IF_ZERO_THROW(ClearCommError(_com, &error, NULL));

        //_com.set(CreateFile(L"run.output", GENERIC_WRITE, 0, NULL,
        //    CREATE_ALWAYS, 0, NULL));

        // When running a .com file, we need the instruction pointer to start
        // at 0x100. We do this by prepending 0x100 NOP bytes at the beginning.
        // In DOS this area would contain the Program Segment Prefix structure.
        //_console.write(hex(l, 8) + "\n");
        Byte checkSum = 0;
        if (comFile) {
            sendLength(l + 0x100);
            for (int i = 0; i < 0x100; ++i) {
                sendByte(0x90);
                checkSum += 0x90;
            }
        }
        else
            sendLength(l);
        for (int i = 0; i < l; ++i) {
            sendByte(data[i]);       // Send data byte
            checkSum += data[i];
            if ((i & 0xff) == 0)
                _console.write(".");
        }
        sendByte(checkSum);
        //IF_ZERO_THROW(FlushFileBuffers(_com));

        //_console.write("Upload complete.\n");
        // Dump bytes from COM port to stdout until we receive ^Z
        do {
            int c = _com.read<Byte>();
            if (c == 26)
                break;
            _console.write<Byte>(c);
        } while (true);
    }
private:
    void sendLength(int l)
    {
        sendByte(l & 0xff);          // Send length low byte
        sendByte((l >> 8) & 0xff);   // Send length middle byte
        sendByte((l >> 16) & 0xff);  // Send length high byte
    }
    void sendByte(Byte value)
    {
        _com.write<Byte>(value);
    }

    AutoHandle _com;

    friend class ReaderThread;
};
