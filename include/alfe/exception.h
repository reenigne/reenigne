#include "alfe/string.h"

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

template<class T> class ExceptionTemplate
{
public:
#ifdef _WIN32
    ExceptionTemplate()
      : _message(messageFromErrorCode(E_FAIL)),
        _implementation(new OwningImplementation(_message))
    { }
#else
    ExceptionTemplate()
      : _message("Unspecified error"),
        _implementation(new StaticImplementation(_message))
    { }
#endif
    ExceptionTemplate(const String& message)
      : _message(message),
        _implementation(new OwningImplementation(message))
    { }
    void write(const Handle& handle) const
    {
        handle.write(_message + codePoint(10));
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
    class Implementation : public ReferenceCounted
    {
    public:
#ifdef UTF16_MESSAGES
        virtual const WCHAR* message() = 0;
#else
        virtual const char* message() = 0;
#endif
    };
    class OwningImplementation : public Implementation
    {
    public:
        OwningImplementation(String string) : _string(string) { }
#ifdef UTF16_MESSAGES
        const WCHAR* message() { return _string; }
#else
        const char* message() { return _string; }
#endif
    private:
#ifdef UTF16_MESSAGES
        NullTerminatedWideString _string;
#else
        NullTerminatedString _string;
#endif
    };
    class StaticImplementation : public Implementation
    {
#ifdef UTF16_MESSAGES
    public:
        StaticImplementation(const WCHAR* string) : _string(string) { }
        const WCHAR* message() { return _string; }
    private:
        const WCHAR* _string;
#else
    public:
        StaticImplementation(const char* string) : _string(string) { }
        const char* message() { return _string; }
    private:
        const char* _string;
#endif
    };
#ifdef _WIN32
    static String messageFromErrorCode(DWORD error)
    {
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
            reinterpret_cast<LPWSTR>(&strMessage),
            0,
            NULL);
        if (formatted == 0)
            return String("FormatMessage failed: 0x") +
                hex(GetLastError(), 8);
        return strMessage.string();
    }
#endif
    String _message;
    Reference<Implementation> _implementation;
};

#endif // INCLUDED_EXCEPTION_H
