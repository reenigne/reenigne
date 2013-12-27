#include "alfe/main.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        _com = AutoHandle(CreateFile(
            L"COM4",
            GENERIC_READ | GENERIC_WRITE,
            0,              // must be opened with exclusive-access
            NULL,           // default security attributes
            OPEN_EXISTING,  // must use OPEN_EXISTING
            0,              // not overlapped I/O
            NULL),          // hTemplate must be NULL for comm devices
            String("Quickboot COM port"));

        DCB deviceControlBlock;
        SecureZeroMemory(&deviceControlBlock, sizeof(DCB));
        IF_ZERO_THROW(GetCommState(_com, &deviceControlBlock));
        deviceControlBlock.DCBlength = sizeof(DCB);
        deviceControlBlock.BaudRate = 19200;
        deviceControlBlock.fBinary = TRUE;
        deviceControlBlock.fParity = FALSE;
        deviceControlBlock.fOutxCtsFlow = FALSE;
        deviceControlBlock.fOutxDsrFlow = TRUE; //FALSE;
        deviceControlBlock.fDtrControl = DTR_CONTROL_HANDSHAKE; //DTR_CONTROL_DISABLE;
        deviceControlBlock.fDsrSensitivity = FALSE;
        deviceControlBlock.fTXContinueOnXoff = TRUE;
        deviceControlBlock.fOutX = FALSE; //TRUE;
        deviceControlBlock.fInX = FALSE; //TRUE;
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

        IF_ZERO_THROW(SetCommMask(_com, EV_RXCHAR));

        int i = 0;
        do {
            int b = _com.tryReadByte();
            if (b == '>')
                break;
            if (b != -1)
                i = 0;
            ++i;
        } while (i < 10);

        int baudDivisor = 104;
        deviceControlBlock.BaudRate =
            static_cast<int>(2000000.0 / baudDivisor + 0.5);

        writeByte(0x7f);
        writeByte(0x7d);
        writeByte(0x04);
        writeByte((baudDivisor - 1) & 0xff);
        writeByte((baudDivisor - 1) >> 8);
        IF_ZERO_THROW(FlushFileBuffers(_com));

        IF_ZERO_THROW(SetCommState(_com, &deviceControlBlock));

        do {
            Byte buffer[0x400];
            for (int i = 0; i < 0x400; ++i)
                buffer[i] = rand() % 0x100;
            writeByte(0x73);
            writeByte(0x00);
            writeByte(0x04);
            int b = _com.tryReadByte();
            if (b != 'p')
                console.write(String("Expected 'p' after length"));
            for (int i = 0; i < 0x400; ++i)
                writeByte(buffer[i]);
            b = _com.tryReadByte();
            if (b != 'd')
                console.write(String("Expected 'd' after data"));
            writeByte(0x7d);
            for (int i = 0; i < 0x400; ++i) {
                Byte b = _com.tryReadByte();
                if (b != buffer[i])
                    console.write(String(String::Decimal(i)) + ": Expected " +
                    hex(buffer[i], 2) + ", received " + hex(b, 2) + ".\n");
            }
        } while (true);
    }
    void writeByte(UInt8 value)
    {
        if (value == 0 || value == 0x11 || value == 0x13)
            _com.write<Byte>(0);
        _com.write<Byte>(value);
    }
    Handle _com;
};
