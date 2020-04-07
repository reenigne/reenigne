#include "alfe/main.h"

#ifndef INCLUDED_WINDOWS_HANDLE_H
#define INCLUDED_WINDOWS_HANDLE_H

#ifdef _WIN32

class WindowsHandle : public ConstHandle
{
public:
    WindowsHandle() : _handle(INVALID_HANDLE_VALUE) { }
    WindowsHandle(HANDLE handle, bool own = true)
      : _handle(handle),
        ConstHandle((own && handle != INVALID_HANDLE_VALUE)
            ? create<Body>(handle) : ConstHandle())
    { }
    bool valid() const { return _handle != INVALID_HANDLE_VALUE; }
    operator HANDLE() const { return _handle; }
    void setHandleInformation(DWORD dwMask, DWORD dwFlags)
    {
        IF_ZERO_THROW(SetHandleInformation(*this, dwMask, dwFlags));
    }
protected:
    WindowsHandle(const ConstHandle& other, HANDLE handle)
      : ConstHandle(other), _handle(handle) { }
    class Body : public ConstHandle::Body
    {
        Body(HANDLE handle) : _handle(handle) { }
        ~Body()
        {
            if (_handle != INVALID_HANDLE_VALUE)
                CloseHandle(_handle);
        }
    private:
        HANDLE _handle;
    };
private:
    HANDLE _handle;
};

#endif

#endif // INCLUDED_WINDOWS_HANDLE_H
