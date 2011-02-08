#ifndef INCLUDED_COM_H
#define INCLUDED_COM_H

#include <ObjBase.h>

class COMInitializer : Uncopyable
{
public:
    COMInitializer(DWORD dwCoInit = COINIT_MULTITHREADED)
    {
        HRESULT hr = CoInitializeEx(NULL, dwCoInit);
        if (FAILED(hr))
            Exception::throwSystemError(String("COM initialization failed"));
    }
    ~COMInitializer() { CoUninitialize(); }
};

#endif // INCLUDED_COM_H
