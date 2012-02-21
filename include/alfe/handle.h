#include "alfe/main.h"

#ifndef INCLUDED_HANDLE_H
#define INCLUDED_HANDLE_H

class Handle
{
public:
#ifdef _WIN32
    Handle() : _handle(INVALID_HANDLE_VALUE) { }
    Handle(HANDLE handle, const File& file = File())
      : _handle(handle), _file(file)
    { }
    operator HANDLE() const { return _handle; }
    bool valid() const
    {
        return _handle != INVALID_HANDLE_VALUE && _handle != NULL;
    }
#else
    Handle() : _fileDescriptor(-1) { }
    Handle(int fileDescriptor, const File& file = File())
      : _fileDescriptor(fileDescriptor), _file(file)
    { }
    operator int() const { return _fileDescriptor; }
    bool valid() const { return _fileDescriptor != -1; }
#endif
    File file() const { return _file; }
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
            throw Exception::systemError("Writing file " + _file.path());
#else
        ssize_t writeResult = write(_fileDescriptor, buffer, bytes);
        if (writeResult < length())
            throw Exception::systemError("Writing file " + _file.path());
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
            throw Exception::systemError("Reading file " + _file.path());
        }
        if (bytesRead != 1)
            return -1;
#else
        ssize_t readResult = read(_fileDescriptor, static_cast<void*>(&b), 1);
        if (readResult < 1) {
            if (_eof(_fileDescriptor))
                return -1;
            throw Exception::systemError("Reading file " + _file.path());
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
            throw Exception::systemError("Reading file " + _file.path());
        if (bytesRead != bytes)
            throw Exception("End of file reading file " + _file.path());
#else
        ssize_t readResult = read(_fileDescriptor, buffer, bytes);
        if (readResult < bytes)
            throw Exception::systemError("Reading file " + _file.path());
#endif
    }
private:
    class Implementation : public ReferenceCounted
    {
    public:
#ifdef _WIN32
        Implementation(HANDLE handle) : _handle(handle) { }
        ~Implementation()
        {
            if (_handle != INVALID_HANDLE_VALUE)
                CloseHandle(_handle);
        }
        HANDLE _handle;
#else
        Implementation(int fileDescriptor)
          : _fileDescriptor(fileDescriptor) { }
        ~Implementation()
        {
            if (_fileDescriptor != -1)
                close(_fileDescriptor);
        }
        int _fileDescriptor;
#endif
    };

#ifdef _WIN32
    Handle(HANDLE handle, const File& file, Implementation* implementation)
      : _handle(handle), _file(file), _implementation(implementation) { }
#else
    Handle(int fileDescriptor, const File& file,
        Implementation* implementation)
      : _fileDescriptor(fileDescriptor), _file(file),
        _implementation(implementation)
    { }
#endif

#ifdef _WIN32
    HANDLE _handle;
#else
    int _fileDescriptor;
#endif
    File _file;
    Reference<Implementation> _implementation;

    friend class Implementation;
    friend class AutoHandle;
};

class AutoHandle : public Handle
{
public:
#ifdef _WIN32
    AutoHandle(HANDLE handle, const File& file = File())
        : Handle(handle, file, new Implementation(handle)) { }
#else
    AutoHandle(int fileDescriptor, const File& file = File())
        : Handle(handle, file, new Implementation(fileDescriptor)) { }
#endif
};


#endif // INCLUDED_HANDLE_H
