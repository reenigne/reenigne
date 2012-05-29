#include "alfe/main.h"
#include "alfe/thread.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        if (_arguments.count() == 1) {
            console.write("Usage: run [-c] <name of file to send>\n");
            return;
        }
        int fileNameArgument = 1;
        bool comFile = false;
        if (_arguments[1] == "-c") {
            comFile = true;
            fileNameArgument = 2;
        }
        String data = File(_arguments[fileNameArgument]).contents();
        int l = data.length();

        //// Reset the machine
        //{
        //    Handle arduinoCom = AutoHandle(CreateFile(
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

        _com = AutoHandle(CreateFile(
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
        deviceControlBlock.BaudRate = 57600; //115200; //38400; //
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
        timeOuts.ReadTotalTimeoutConstant = 100; //MAXDWORD;
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
        //console.write(hex(l, 8) + "\n");

        _packet.allocate(0x101);

        Byte checksum;
        if (comFile) {
            // Send 0x100 NOPs to pad out the file so that execution starts at
            // the expected IP.
            int bytes = 0xff;
            _packet[0] = bytes;
            checksum = 0;
            for (int i = 0; i < bytes; ++i) {
                Byte d = 0x90;
                _packet[i + 1] = d;
                checksum += d;
            }
            _packet[bytes + 1] = checksum;
            sendPacket();
            _packet[0] = 1;
            _packet[2] = 0x90;
            sendPacket();
        }

        int p = 0;
        int bytes;
        do {
            bytes = min(l, 0xff);
            _packet[0] = bytes;
            checksum = 0;
            for (int i = 0; i < bytes; ++i) {
                Byte d = data[p];
                ++p;
                _packet[i + 1] = d;
                checksum += d;
            }
            _packet[bytes + 1] = checksum;
            sendPacket();
            l -= bytes;
        } while (bytes != 0);

        //IF_ZERO_THROW(FlushFileBuffers(_com));

        console.write("\nUpload complete.\n");
        // Dump bytes from COM port to stdout until we receive ^Z

        timeOuts.ReadTotalTimeoutConstant = MAXDWORD;
        IF_ZERO_THROW(SetCommTimeouts(_com, &timeOuts));

        do {
            int c = _com.read<Byte>();
            if (c == 26)
                break;
            console.write<Byte>(c);
        } while (true);
    }
private:
    void sendPacket()
    {
        bool ok;
        do {
            ok = true;
            _com.write(&_packet[0], 2 + _packet[0]);
            IF_ZERO_THROW(FlushFileBuffers(_com));
            int c;
            do {
                c = _com.tryReadByte();
                switch (c) {
                    case 'K':
                        if (!ok)
                            console.write("\nXT returned OK but lost data\n");
                        else
                            console.write("K");
                        break;
                    case 'F':
                        console.write("F");
                        ok = false;
                        break;
                    case -1:
                        console.write(".");
                        ok = false;
                        _com.write<Byte>(0xa5);
                        break;
                    default:
                        console.write("\nUnrecognized byte " + hex(c, 2) +
                            " from XT\n");
                        break;
                }
            } while (c == -1);
        } while (!ok);
    }

    Handle _com;
    Array<Byte> _packet;

    friend class ReaderThread;
};
