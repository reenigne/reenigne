#include "alfe/main.h"

#ifndef INCLUDED_EXCEPTION_H
#define INCLUDED_EXCEPTION_H

#define CODE_MACRO(x) do { x } while (false)

#define IF_TRUE_THROW(expr,exception) CODE_MACRO( \
    if (expr) \
        throw exception; \
)

#define IF_FALSE_THROW(expr) \
    IF_TRUE_THROW(!(expr), Exception::systemError())
#define IF_MINUS_ONE_THROW(expr) \
    IF_TRUE_THROW((expr) == -1, Exception::systemError())
#define IF_ZERO_THROW(expr) \
    IF_TRUE_THROW((expr) == 0, Exception::systemError())
#define IF_NULL_THROW(expr) \
    IF_TRUE_THROW((expr) == NULL, Exception::systemError())
#define IF_NONZERO_THROW(expr) \
    IF_TRUE_THROW((expr) != 0, Exception::systemError())

#define IF_ZERO_CHECK_THROW_LAST_ERROR(expr) CODE_MACRO( \
    if ((expr) == 0) \
        IF_FALSE_THROW(GetLastError() == 0); \
)

#define BEGIN_CHECKED \
    try { \
        try

#define END_CHECKED \
        catch(std::bad_alloc&) { \
            throw Exception::outOfMemory(); \
        } \
        catch(std::exception&) { \
            throw Exception::unknown(); \
        } \
    } \
    catch

template<class T> class ExceptionT
{
public:
#ifdef _WIN32
    ExceptionT() : _message(messageFromErrorCode(E_FAIL)) { }
#else
    ExceptionT() : _message("Unspecified error") { }
#endif
    ExceptionT(const String& message) : _message(message) { }
    void write(const StreamT<T>& stream) const
    {
        stream.write(_message + codePoint(10));
    }
    static Exception systemError(const String& message = String())
    {
        String m;
#ifdef _WIN32
        m = messageFromErrorCode(GetLastError());
#else
        m = String(strerror(errno));
#endif
        if (message.empty())
            return Exception(m);
        return Exception(message + " : " + m);
    }
    static Exception outOfMemory()
    {
#ifdef _WIN32
        return Exception(messageFromErrorCode(E_OUTOFMEMORY));
#else
        return Exception(strerror(ENOMEM));
#endif
    }
    static Exception unknown() { return Exception(); }
    String message() const { return _message; }

#ifdef _WIN32
    static Exception fromErrorCode(DWORD error)
    {
        return Exception(messageFromErrorCode(error));
    }
#else
    static Exception fromErrorCode(int error)
    {
        return Exception(strerror(error));
    }
#endif
private:
#ifdef _WIN32
    static String messageFromErrorCode(DWORD error)
    {
        if (error == 0) {
            // If there was really no error we wouldn't be here. Avoid emitting
            // messages like "Error: Success"
            error = E_FAIL;
        }
        LocalString strMessage;
        // The reinterpret_cast<> here is necessary because of the
        // FORMAT_MESSAGE_ALLOCATE_BUFFER flag, which causes Windows to put a
        // LPWSTR value in a *LPWSTR variable.
        DWORD formatted = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            error,
            0,
            reinterpret_cast<LPWSTR>(&strMessage),
            0,
            NULL);
        if (formatted == 0)
            return String("Error code: ") + hex(error, 8);
        // It's safe to destruct the LocalString because it uses wide
        // characters and the String constructor will allocate its own buffer
        // for the UTF-8 conversion.
        return strMessage.string();
    }
#endif
    String _message;
};

class NotYetImplementedException : public Exception
{
public:
    NotYetImplementedException() : Exception("Not yet implemented") { }
};

class PreserveSystemError
{
public:
    PreserveSystemError()
    {
#ifdef _WIN32
        _lastError = GetLastError();
#else
        _errno = errno;
#endif
    }
    ~PreserveSystemError()
    {
#ifdef _WIN32
        SetLastError(_lastError);
#else
        errno = _errno;
#endif
    }
private:
#ifdef _WIN32
    DWORD _lastError;
#else
    int _errno;
#endif
};

#endif // INCLUDED_EXCEPTION_H
