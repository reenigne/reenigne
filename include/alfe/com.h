#ifndef INCLUDED_COM_H
#define INCLUDED_COM_H

#include "unity/string.h"
#include <ObjBase.h>
#include <OleAuto.h>
#include "unity/assert.h"


#define IF_ERROR_THROW(expr) CODE_MACRO( \
    HRESULT hrMacro = (expr); \
    IF_TRUE_THROW(FAILED(hrMacro), exceptionFromHResult(hrMacro)); \
)

class Bstr : Uncopyable
{
public:
    Bstr() : m_bstr(NULL) { }
    ~Bstr() { SysFreeString(m_bstr); }
    operator String() const { return String(m_bstr); }
    BSTR* operator&()
    {
        assert(m_bstr == NULL, L"Re-initializing Bstr");
        return &m_bstr;
    }
private:
    BSTR m_bstr;
};


template<class T> class COMPointer
{
public:
    COMPointer() : _p(0) { }
    COMPointer(const COMPointer& spT) : _p(spT._p) { _p->AddRef(); }
    COMPointer(T* pT) : _p(pT) { if (valid()) _p->AddRef(); }
    const COMPointer& operator=(const COMPointer& spT)
    {
        reset();
        _p = spT._p;
        if (valid())
            _p->AddRef();
        return *this;
    }
    template<class U> COMPointer(const COMPointer<U>& spU,
        const IID* piid = &__uuidof(T))
    {
        IF_FALSE_THROW(spU.valid());
        IF_ERROR_THROW(
            spU->QueryInterface(*piid, reinterpret_cast<void**>(&_p)));
    }
    ~COMPointer() { reset(); }
    T** operator&()
    {
        if (valid())
            assert(false, L"Re-initializing COMPointer");
        return &_p;
    }
    T* operator->() const
    {
        if (!valid())
            assert(false, L"Using uninitialized COMPointer");
        return _p;
    }
    bool valid() const { return _p != 0; }
    void reset()
    {
        if (valid())
            _p->Release();
        _p = 0;
    }
    operator T*() { return _p; }
    operator const T*() const { return _p; }
private:
    T* _p;
};


Exception exceptionFromHResult(HRESULT hresult)
{
    COMPointer<IErrorInfo> spIEI;
    if (GetErrorInfo(0, &spIEI) == S_OK) {
        Bstr bstrDescription;
        spIEI->GetDescription(&bstrDescription);
        return Exception(bstrDescription);
    }
    else
        return Exception::fromErrorCode(hresult);
}


class COMInitializer : Uncopyable
{
public:
    COMInitializer(DWORD dwCoInit = COINIT_MULTITHREADED)
    {
        HRESULT hr = CoInitializeEx(NULL, dwCoInit);
        if (FAILED(hr))
            throw Exception::systemError(String("COM initialization failed"));
    }
    ~COMInitializer() { CoUninitialize(); }
};

#endif // INCLUDED_COM_H
