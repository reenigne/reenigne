#include "alfe/main.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        for (int divisor = 104; divisor >= 0; --divisor) {
            try {
                console.write(String(String::Decimal(divisor)) + ": ");
                runtest(divisor);
            } catch (...) { }
            console.write(String("\n"));
            _com = AutoHandle();
        }
    }

    void runtest(int divisor)
    {
        _com = AutoHandle(CreateFile(
            L"COM1",
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
        deviceControlBlock.fOutxDsrFlow = FALSE;//TRUE;
        deviceControlBlock.fDtrControl = DTR_CONTROL_ENABLE; //HANDSHAKE;//DTR_CONTROL_DISABLE; //
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

        COMMTIMEOUTS timeOuts;
        SecureZeroMemory(&timeOuts, sizeof(COMMTIMEOUTS));
        timeOuts.ReadIntervalTimeout = 10*1000;
        timeOuts.ReadTotalTimeoutMultiplier = 0;
        timeOuts.ReadTotalTimeoutConstant = 10*1000;
        timeOuts.WriteTotalTimeoutConstant = 10*1000;
        timeOuts.WriteTotalTimeoutMultiplier = 0;
        IF_ZERO_THROW(SetCommTimeouts(_com, &timeOuts));

        int i = 0;
        do {
            int b = getByte();
            if (b == '>')
                break;
            if (b != -1)
                i = 0;
            else {
                console.write(String("Resetting\n"));
                // Reset the Arduino
                EscapeCommFunction(_com, CLRDTR);
                EscapeCommFunction(_com, CLRRTS);
                Sleep(250);
                EscapeCommFunction(_com, SETDTR);
                EscapeCommFunction(_com, SETRTS);
            }
            ++i;
        } while (i < 10);
        if (i == 10)
            console.write(String("Timeout waiting for >\n"));

        int baudDivisor = divisor;
        deviceControlBlock.BaudRate =
            static_cast<int>(2000000.0 / baudDivisor + 0.5);
        //if (deviceControlBlock.BaudRate < 28800)
        //    deviceControlBlock.BaudRate = 19200;
        //else
        //    if (deviceControlBlock.BaudRate < 48000)
        //        deviceControlBlock.BaudRate = 38400;
        //    else
        //        if (deviceControlBlock.BaudRate < 86400)
        //            deviceControlBlock.BaudRate = 57600;
        //        else
        //            if (deviceControlBlock.BaudRate < 121600)
        //                deviceControlBlock.BaudRate = 115200;
        //            else
        //                if (deviceControlBlock.BaudRate < 192000)
        //                    deviceControlBlock.BaudRate = 128000;
        //                else
        //                    deviceControlBlock.BaudRate = 256000;

        writeByte(0x7f);
        writeByte(0x7c);
        writeByte(0x04);
        writeByte((baudDivisor - 1) & 0xff);
        writeByte((baudDivisor - 1) >> 8);
        IF_ZERO_THROW(FlushFileBuffers(_com));

        IF_ZERO_THROW(SetCommState(_com, &deviceControlBlock));

        for (int j = 0; j < 10; ++j) {
            Byte buffer[0x400];
            for (int i = 0; i < 0x400; ++i)
                buffer[i] = rand() % 0x100;
            writeByte(0x73);
            writeByte(0x00);
            writeByte(0x04);
            int b = getByte();
            if (b != 'p')
                throw Exception();
                //console.write(String("Expected 'p' after length, received " + hex(b, 2) + "\n"));
            for (int i = 0; i < 0x400; ++i)
                writeByte(buffer[i]);
            b = getByte();
            if (b != 'd')
                throw Exception();
                //console.write(String("Expected 'd' after data, received " + hex(b, 2) + "\n"));
            writeByte(0x7d);
            b = getByte();
            if (b != 0x00)
                throw Exception();
                //console.write(String("Expected low length byte 0, received " + hex(b, 2) + "\n"));
            b = getByte();
            if (b != 0x04)
                throw Exception();
                //console.write(String("Expected high length byte 4, received " + hex(b, 2) + "\n"));
            for (int i = 0; i < 0x400; ++i) {
                b = getByte();
                if (b != buffer[i])
                    throw Exception();
                    //console.write(String(String::Decimal(i)) + ": Expected " +
                    //hex(buffer[i], 2) + ", received " + hex(b, 2) + ".\n");
            }
            console.write(".");
        }
    }
    void writeByte(UInt8 value)
    {
        if (value == 0 || value == 0x11 || value == 0x13)
            _com.write<Byte>(0);
        _com.write<Byte>(value);
    }
    int getByte()
    {
        int b = getByte2();
        if (b == 0)
            b = getByte2();
        // console.write(String(String::CodePoint(b)));
        return b;
    }
    int getByte2()
    {
        int b = _com.tryReadByte();
        if (b == -1)
            console.write(String("Timeout\n"));
        // console.write(String(String::CodePoint(b)));
        return b;
    }
    Handle _com;
    bool _escape;
};
