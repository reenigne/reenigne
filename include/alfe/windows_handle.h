#include "alfe/main.h"

#ifndef INCLUDED_WINDOWS_HANDLE_H
#define INCLUDED_WINDOWS_HANDLE_H

#ifdef _WIN32

class WindowsHandle : public ConstHandle
{
public:
    WindowsHandle() : _handle(INVALID_HANDLE_VALUE) { }
    WindowsHandle(HANDLE handle, bool own = true) : _handle(handle),
        ConstHandle(own ? create<OwningBody>(handle) : create<NonOwningBody>())
    { }
    bool valid() const { return _handle != INVALID_HANDLE_VALUE; }
    operator HANDLE() const { return _handle; }
private:
    class Body : public ConstHandle::Body { };
    class NonOwningBody : public Body { };
    class OwningBody : public Body
    {
    public:
        OwningBody(HANDLE handle) : _handle(handle) { }
        ~OwningBody()
        {
            if (_handle != INVALID_HANDLE_VALUE)
                CloseHandle(_handle);
        }
    private:
        HANDLE _handle;
    };
    HANDLE _handle;
};

#endif

#endif // INCLUDED_WINDOWS_HANDLE_H
