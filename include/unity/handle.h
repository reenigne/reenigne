#include "unity/string.h"

#ifndef INCLUDED_HANDLE_H
#define INCLUDED_HANDLE_H

class Handle : Uncopyable
{
public:
#ifdef _WIN32
    Handle() : _handle(INVALID_HANDLE_VALUE), _name("") { }
    Handle(HANDLE handle, const String& name = "")
      : _handle(handle), _name(name)
    { }
    operator HANDLE() const { return _handle; }
    bool valid() const { return _handle != INVALID_HANDLE_VALUE; }
    void set(HANDLE handle, const String& name = "")
    {
        _handle = handle;
        _name = name;
    }
#else
    Handle() : _fileDescriptor(-1) { }
    Handle(int fileDescriptor, const String& name = "")
      : _fileDescriptor(fileDescriptor), _name(name)
    { }
    operator int() const { return _fileDescriptor; }
    bool valid() const { return _fileDescriptor != -1; }
    void set(int fileDescriptor, const String& name = "")
    {
        _fileDescriptor = fileDescriptor;
        _name = name;
    }
#endif
    String name() const { return _name; }
    // Be careful using the template read() and write() functions with types
    // other than single bytes and arrays thereof - they are not endian-safe.
    void write(const char* string) const
    {
        write(static_cast<const void*>(string), strlen(string));
    }
    template<class U> void write(const U& value) const
    {
        write(static_cast<const void*>(&value), sizeof(U));
    }
    template<class U> void write(const Array<U>& value) const
    {
        write(static_cast<const void*>(&value[0]), value.count()*sizeof(U));
    }
    void write(const String& s) const { write(s.data(), s.length()); }
    void write(const Exception& e) const { e.write(*this); }
    void write(const void* buffer, int bytes) const
    {
        if (bytes == 0)
            return;
#ifdef _WIN32
        DWORD bytesWritten;
        if (WriteFile(_handle, buffer, bytes, &bytesWritten, NULL) == 0 ||
            bytesWritten != bytes)
            throw Exception::systemError(String("Writing file ") + _name);
#else
        ssize_t writeResult = write(_fileDescriptor, buffer, bytes);
        if (writeResult < length())
            throw Exception::systemError(String("Writing file ") + _name);
#endif
    }
    template<class U> U read()
    {
        U value;
        read(static_cast<void*>(&value), sizeof(U));
        return value;
    }
    int tryReadByte()
    {
        Byte b;
#ifdef _WIN32
        DWORD bytesRead;
        if (ReadFile(_handle, static_cast<void*>(&b), 1, &bytesRead, NULL)
            == 0) {
            if (GetLastError() == ERROR_HANDLE_EOF)
                return -1;
            throw Exception::systemError(String("Reading file ") + _name);
        }
        if (bytesRead != 1)
            return -1;
#else
        ssize_t readResult = read(_fileDescriptor, static_cast<void*>(&b), 1);
        if (readResult < 1) {
            if (_eof(_fileDescriptor))
                return -1;
            throw Exception::systemError(String("Reading file ") + _name);
        }
#endif
        return b;
    }
    void read(void* buffer, int bytes) const
    {
        if (bytes == 0)
            return;
#ifdef _WIN32
        DWORD bytesRead;
        if (ReadFile(_handle, buffer, bytes, &bytesRead, NULL) == 0)
            throw Exception::systemError(String("Reading file ") + _name);
        if (bytesRead != bytes)
            throw Exception(String("End of file reading file ") + _name);
#else
        ssize_t readResult = read(_fileDescriptor, buffer, bytes);
        if (readResult < bytes)
            throw Exception::systemError(String("Reading file ") + _name);
#endif
    }
private:
#ifdef _WIN32
    HANDLE _handle;
#else
    int _fileDescriptor;
#endif
    String _name;
};

class AutoHandle : public Handle
{
public:
    AutoHandle() { }
#ifdef _WIN32
    AutoHandle(HANDLE handle, const String& name = "")
      : Handle(handle, name)
    { }
    ~AutoHandle() { if (valid()) CloseHandle(operator HANDLE()); }
#else
    AutoHandle(int fileDescriptor, const String& name = "")
      : Handle(fileDescriptor, name)
    { }
    ~AutoHandle() { if (valid()) close(operator int()); }
#endif
};

#endif // INCLUDED_HANDLE_H
