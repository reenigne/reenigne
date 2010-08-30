#ifndef INCLUDED_WINDOWS_SYSTEM_H
#deifne INCLUDED_WINDOWS_SYSTEM_H

#include "uncopyable.h"
#include <windows.h>

class Handle : Uncopyable
{
public:
    Handle() : _handle(NULL) { }
    Handle(HANDLE handle) : _handle(handle) { }
    void set(HANDLE handle) { _handle = handle; }
    operator HANDLE() const { return _handle; }
    ~Handle() { if (_handle != NULL) CloseHandle(_handle); }
private:
    HANDLE _handle;
};

#endif // INCLUDED_WINDOWS_SYSTEM_H
