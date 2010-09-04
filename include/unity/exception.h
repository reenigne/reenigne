#ifndef INCLUDED_EXCEPTION_H
#define INCLUDED_EXCEPTION_H

class Exception;

#include "unity/string.h"
#include "unity/uncopyable.h"

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class LocalString : Uncopyable
{
public:
    LocalString() : _str(NULL) { }
    ~LocalString() { LocalFree(_str); }
    LPWSTR* operator&() { return &_str; }
    String string() { return String(_str); }
    WSTR* operator WSTR*() { return _str; }
private:
    LPWSTR _str;
};

#else
#include <errno.h>
#include <string.h>
#endif

class Exception
{
public:
    Exception(const String& message) : _message(message) { }
    void write(const Handle& handle) const { _message.write(handle); }

    static void throwSystemError(const String& message)
    {
#ifdef _WIN32
        DWORD error = GetLastError();
        if (error == 0) {
            // If there was really no error we wouldn't be here. Avoid emitting
            // messages like "Error: Success"
            error = E_FAIL;
        }
        LocalString strMessage;
        DWORD formatted = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            error,
            0,
            &strMessage,
            0,
            NULL);
        if (formatted == 0) {
            error = GetLastError();
            static String formatMessageFailed("FormatMessage failed: 0x");
            _message = formatMessageFailed + String(error, 8);
        }
        else
            _message = strMessage.string();
        static String colon(" : ");
        _message = message + colon + _message;
#else
        _message = String(strerror(errno));
#endif
    }
private:
    String _message;
};

#define BEGIN_CHECKED \
    try { \
        try

#define END_CHECKED \
        catch(std::bad_alloc&) { \
            throw Exception(E_OUTOFMEMORY); \
        } \
        catch(std::exception&) { \
            throw Exception(E_FAIL); \
        } \
    } \
    catch


#endif // INCLUDED_EXCEPTION_H
