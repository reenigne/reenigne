#ifndef INCLUDED_HANDLE_H
#define INCLUDED_HANDLE_H

#include "unity/integer_types.h"

template<class T> class HandleTemplate;
typedef HandleTemplate<void> Handle;

#include "unity/uncopyable.h"
#include "unity/string.h"

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

template<class T> class HandleTemplate : Uncopyable
{
public:
#ifdef _WIN32
    HandleTemplate() : _handle(INVALID_HANDLE_VALUE) { }
    HandleTemplate(HANDLE handle, const String& name) : _handle(handle), _name(name) { }
    operator HANDLE() const { return _handle; }
    bool valid() const { return _handle != INVALID_HANDLE_VALUE; }
    static Handle consoleOutput()
    {
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        if (h == INVALID_HANDLE_VALUE || h == NULL) {
            static String openingConsole("Getting console handle ");
            Exception::throwSystemError(openingConsole);
        }
        static String console("console");
        return Handle(h, console);
    }
#else
    HandleTemplate() : _fileDescriptor(-1) { }
    HandleTemplate(int fileDescriptor) : _fileDescriptor(fileDescriptor) { }
    operator int() const { return _fileDescriptor; }
    bool valid() const { return _fileDescriptor != -1; }
    static Handle consoleOutput()
    {
        static String console("console");
        return Handle(STDOUT_FILENO, console);
    }
#endif
    StringTemplate<T> name() const { return _name; }
private:
#ifdef _WIN32
    HANDLE _handle;
#else
    int _fileDescriptor;
#endif
    StringTemplate<T> _name;
};

class AutoHandle : public Handle
{
public:
    AutoHandle() { }
#ifdef _WIN32
    AutoHandle(HANDLE handle, const String& name) : Handle(handle, name) { }
    ~AutoHandle() { if (valid()) CloseHandle(*this); }
#else
    AutoHandle(int fileDescriptor) : Handle(fileDescriptor) { }
    ~AutoHandle() { if (valid()) close(*this); }
#endif
};

#endif // INCLUDED_HANDLE_H
