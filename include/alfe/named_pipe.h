#ifndef INCLUDED_NAMED_PIPE_H
#define INCLUDED_NAMED_PIPE_H

#include "alfe/windows_handle.h"

class NamedPipe
{
public:
    NamedPipe(String name = "", int size = 4096, int timeOut = 120)
    {
        if (name == "") {
            static volatile long unique;
            name = format("\\\\.\\Pipe\\ALFEAnonymousPipe.%08x.%08x",
                GetCurrentProcessId(), InterlockedIncrement(&unique));
        }

        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = NULL;

        NullTerminatedWideString data(name);
        HANDLE r = CreateNamedPipe(
            data,
            PIPE_ACCESS_INBOUND /*| readMode */,
            PIPE_TYPE_BYTE | PIPE_WAIT,
            1,
            size,
            size,
            timeOut * 1000,
            &sa);
        IF_FALSE_THROW(r != INVALID_HANDLE_VALUE);
        _read = Stream(r, File(name));

        HANDLE w = CreateFile(
            data,
            GENERIC_WRITE,
            0,                         // No sharing
            &sa,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL /*| dwWriteMode*/,
            NULL                       // Template file
        );
        IF_FALSE_THROW(w != INVALID_HANDLE_VALUE);
        _write = Stream(w, File(name));
    }
    Stream read() { return _read; }
    Stream write() { return _write; }
private:
    Stream _read;
    Stream _write;
};

#endif // INCLUDED_NAMED_PIPE_H
