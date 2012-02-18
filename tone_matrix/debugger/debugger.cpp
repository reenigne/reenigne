#include "alfe/main.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        AutoHandle com(CreateFile(
            L"COM3",
            GENERIC_READ | GENERIC_WRITE,
            0,                            // must be opened with exclusive-access
            NULL,                         // default security attributes
            OPEN_EXISTING,                // must use OPEN_EXISTING
            0,                            // not overlapped I/O
            NULL));                       // hTemplate must be NULL for comm devices

        DCB deviceControlBlock;
        SecureZeroMemory(&deviceControlBlock, sizeof(DCB));

        IF_ZERO_THROW(GetCommState(com, &deviceControlBlock));

        deviceControlBlock.DCBlength = sizeof(DCB);
        deviceControlBlock.BaudRate = CBR_9600;
        //deviceControlBlock.fBinary = TRUE;
        //deviceControlBlock.fParity = FALSE;
        deviceControlBlock.fOutxCtsFlow = FALSE;
        deviceControlBlock.fOutxDsrFlow = FALSE;
        deviceControlBlock.fDtrControl = DTR_CONTROL_ENABLE;  // Causes Arduino to reset on connect
        //deviceControlBlock.fDsrSensitivity = FALSE;
        //deviceControlBlock.fTXContinueOnXoff = TRUE;
        //deviceControlBlock.fOutX = FALSE;
        //deviceControlBlock.fInX = FALSE;
        //deviceControlBlock.fErrorChar = FALSE;
        //deviceControlBlock.fNull = TRUE;
        //deviceControlBlock.fRtsControl = RTS_CONTROL_DISABLE;
        //deviceControlBlock.fAbortOnError = FALSE;
        //deviceControlBlock.wReserved = 0;
        deviceControlBlock.ByteSize = 8;
        deviceControlBlock.Parity = NOPARITY;
        deviceControlBlock.StopBits = ONESTOPBIT;

        IF_ZERO_THROW(SetCommState(com, &deviceControlBlock));

        Sleep(2000);

        do {
            DWORD bytesWritten, bytesRead;
            char buffer[80];
            void* b = reinterpret_cast<void*>(buffer);
            buffer[2] = 0;

            //WriteFile(com, reinterpret_cast<const void*>("011?"), 4, &bytesWritten, NULL);
            //ReadFile(com, b, 2, &bytesRead, NULL);
            //printf("%s ", buffer);

            //WriteFile(com, reinterpret_cast<const void*>("00c?"), 4, &bytesWritten, NULL);
            //ReadFile(com, b, 2, &bytesRead, NULL);
            //printf("%s ", buffer);

            //WriteFile(com, reinterpret_cast<const void*>("00d?"), 4, &bytesWritten, NULL);
            //ReadFile(com, b, 2, &bytesRead, NULL);
            //printf("%s ", buffer);

            //WriteFile(com, reinterpret_cast<const void*>("600?"), 4, &bytesWritten, NULL);
            //ReadFile(com, b, 2, &bytesRead, NULL);
            //printf("%s ", buffer);

            //WriteFile(com, reinterpret_cast<const void*>("608?"), 4, &bytesWritten, NULL);
            //ReadFile(com, b, 2, &bytesRead, NULL);
            //printf("%s ", buffer);

            //WriteFile(com, reinterpret_cast<const void*>("680?"), 4, &bytesWritten, NULL);
            //ReadFile(com, b, 2, &bytesRead, NULL);
            //printf("%s ", buffer);

            //WriteFile(com, reinterpret_cast<const void*>("688?"), 4, &bytesWritten, NULL);
            //ReadFile(com, b, 2, &bytesRead, NULL);
            //printf("%s\n", buffer);

            buffer[6]=0;
            for (int i = 0; i < 0x100; ++i) {
                sprintf(buffer, "8%02x?", i);
            //	printf("%s\n", buffer);
                WriteFile(com, b, 6, &bytesWritten, NULL);
                buffer[2] = 0;
                ReadFile(com, b, 2, &bytesRead, NULL);
                printf("%s", buffer);
                if ((i & 0x0f) == 0x0f)
                    printf("\n");
            }
            printf("\n");
            //for (int i = 0; i < 0x100; ++i) {
            //    sprintf(buffer, "7%02x?", i);
            ////	printf("%s\n", buffer);
            //    WriteFile(com, b, 6, &bytesWritten, NULL);
            //    buffer[2] = 0;
            //    ReadFile(com, b, 2, &bytesRead, NULL);
            //    printf("%s", buffer);
            //    if ((i & 0x0f) == 0x0f)
            //        printf("\n");
            //}

            //	sprintf(buffer, "4%02x=00", i);
            //	printf("%s\n", buffer);
            //	WriteFile(com, b, 6, &bytesWritten, NULL);
            //	sprintf(buffer, "5%02x=00", i);
            //	printf("%s\n", buffer);
            //	WriteFile(com, b, 6, &bytesWritten, NULL);
            //}
            //for (int i = 0; i < 0x10; ++i) {
            //	sprintf(buffer, "6%01x%01x=01", i, 15-i);
            //	printf("%s\n", buffer);
            //	WriteFile(com, b, 6, &bytesWritten, NULL);
            //}

        } while (true);
    }
};
