#ifndef INCLUDED_HANDLE_H
#deifne INCLUDED_HANDLE_H

#include "uncopyable.h"
#include <windows.h>

class Handle : Uncopyable
{
public:
    Handle() : _handle(NULL) { }
    Handle(HANDLE handle) : _handle(handle) { }
    void set(HANDLE handle) { _handle = handle; }
    operator HANDLE() const { return _handle; }
    bool valid() const { return _handle != NULL; }
private:
    HANDLE _handle;
};

class AutoHandle : public Handle
{
public:

    ~AutoHandle() { if (valid()) CloseHandle(*this); }
};

#endif // INCLUDED_HANDLE_H
