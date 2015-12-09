#include "alfe/main.h"

#ifndef INCLUDED_STREAM_H
#define INCLUDED_STREAM_H

template<class T> class StreamT : public Handle
{
public:
#ifdef _WIN32
    StreamT()
      : Handle(create<Body>()), _handle(INVALID_HANDLE_VALUE) { }
    StreamT(HANDLE handle, const File& file = File())
      : Handle(create<Body>()), _handle(handle), _file(file) { }
    operator HANDLE() const { return _handle; }
    bool valid() const
    {
        return _handle != INVALID_HANDLE_VALUE && _handle != NULL;
    }
#else
    StreamT() : Handle(create<Body>()), _fileDescriptor(-1) { }
    StreamT(int fileDescriptor, const File& file = File())
      : Handle(create<Body>()), _fileDescriptor(fileDescriptor), _file(file)
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
    template<class U> void write(const AppendableArray<U>& value) const
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
        ssize_t writeResult = ::write(_fileDescriptor, buffer, bytes);
        if (writeResult != bytes)
            throw Exception::systemError("Writing file " + _file.path());
#endif
    }
    template<class U> U read()
    {
        U value;
        read(reinterpret_cast<Byte*>(&value), sizeof(U));
        return value;
    }
    String readLengthString()
    {
        int length = read<int>();
        String data(length);
        read(data.data(), length);
        return data;
    }
    // Read a string from the file. The end of the line (any line ending) or
    // the end of the file delimits the string. EOL characters are not
    // returned.
    String readString(bool* eof = 0, int maxLength = 0x7fffffff)
    {
        String s;
        int l = 0;
        if (eof != 0)
            *eof = false;
        do {
            int b = peekByte(0);
            if (b == 10) {
                read<Byte>();
                if (peekByte(0) == 13)
                    read<Byte>();
                return s;
            }
            if (b == 13) {
                read<Byte>();
                if (peekByte(0) == 10)
                    read<Byte>();
                return s;
            }
            if (b == -1) {
                if (eof != 0) {
                    *eof = true;
                    return s;
                }
                if (l != 0)
                    return s;
                // We're at EOF, but caller was sure we wouldn't be. Throw.
            }
            if (l == maxLength)
                return s;
            read<Byte>();
            ++l;
            s += String::Byte(b);
        } while (true);
    }
    // Returns number of bytes actually read
    int tryRead(Byte* destination, int bytes) const
    {
        // Read any buffered bytes first
        CircularBuffer<Byte>* buffer = &body()->_buffer;
        int c = buffer->count();
        if (c != 0) {
            if (c > bytes)
                c = bytes;
            int b = buffer->readBoundary();
            if (b > c)
                b = c;
            memcpy(destination, buffer->readPointer(), b);
            memcpy(destination + b, buffer->lowPointer(), c-b);
            buffer->remove(c);
            destination += c;
            bytes -= c;
        }
        return c + tryReadUnbuffered(destination, bytes);
    }
    int tryReadByte()
    {
        Byte b;
        int bytesRead = tryRead(&b, 1);
        if (bytesRead != 1)
            return -1;
        return b;
    }
    void read(Byte* destination, int bytes) const
    {
        int bytesRead = tryRead(destination, bytes);
        if (bytesRead != bytes)
            throw Exception("End of file reading file " + _file.path());
    }
    int peekByte(int n) { return peek<Byte, int>(n, -1); }
    template<class U> U peek(int n, const U& defaultValue = U())
    {
        return peek<U, U>(n, defaultValue);
    }
    template<class U, class R> R peek(int n, const R& defaultValue = R())
    {
        // Make sure we have enough data in the buffer
        CircularBuffer<Byte>* buffer = &body()->_buffer;
        int r = sizeof(U)*(n + 1) - buffer->count();
        if (r > 0) {
            buffer->add(r);
            int b = buffer->writeBoundary();
            int c;
            if (b > r)
                b = r;
            c = tryReadUnbuffered(buffer->writePointer(), b);
            buffer->added(c);
            if (c < b)
                return defaultValue;
            c = tryReadUnbuffered(buffer->lowPointer(), r - b);
            buffer->added(c);
            if (c < r - b)
                return defaultValue;
        }

        // The object we want to peek at may be unaligned in the buffer or even
        // split across the boundary, so copy it out to return it.
        U x;
        buffer->copyOut(&x, n*sizeof(U), sizeof(U));
        return x;
    }
    void close()
    {
#ifdef _WIN32
        if (_handle != INVALID_HANDLE_VALUE)
            CloseHandle(_handle);
        _handle = INVALID_HANDLE_VALUE;
#else
        if (_fileDescriptor != -1)
            ::close(_fileDescriptor);
        _fileDescriptor = -1;
#endif
    }
private:
    int tryReadUnbuffered(Byte* destination, int bytes) const
    {
        if (bytes == 0)
            return 0;
#ifdef _WIN32
        DWORD bytesRead;
        if (ReadFile(_handle, destination, bytes, &bytesRead, NULL) == 0) {
            DWORD error = GetLastError();
            if (error == ERROR_HANDLE_EOF || error == ERROR_BROKEN_PIPE ||
                error == ERROR_PIPE_NOT_CONNECTED)
                return bytesRead;
            throw Exception::systemError("Reading file " + _file.path());
        }
        return bytesRead;
#else
        ssize_t readResult = ::read(_fileDescriptor, destination, bytes);
        if (readResult == -1)
            throw Exception::systemError("Reading file " + _file.path());
        return readResult;
#endif
    }
    class Body : public Handle::Body
    {
    public:
        mutable CircularBuffer<Byte> _buffer;
    };
    const Body* body() const { return as<Body>(); }
    class OwningBody : public Body
    {
    public:
        OwningBody(Stream stream) : _stream(stream) { }
        ~OwningBody()
        {
            _stream.close();
        }
        StreamT<T> _stream;
    };

#ifdef _WIN32
    StreamT(HANDLE handle, const File& file, const Handle& other)
      : Handle(other), _handle(handle), _file(file) { }
#else
    StreamT(int fileDescriptor, const File& file, const Handle& other)
      : Handle(other), _fileDescriptor(fileDescriptor), _file(file) { }
#endif

#ifdef _WIN32
    HANDLE _handle;
#else
    int _fileDescriptor;
#endif
    File _file;

    friend class Body;
    friend class AutoStreamT<void>;
};

template<class T> class AutoStreamT : public Stream
{
public:
    AutoStreamT() { }
#ifdef _WIN32
    AutoStreamT(HANDLE handle, const File& file = File())
      : Stream(handle, file, create<OwningBody>(Stream(handle, file))) { }
#else
    AutoStreamT(int fileDescriptor, const File& file = File())
      : Stream(fileDescriptor, file,
        create<OwningBody>(Stream(fileDescriptor, file))) { }
#endif
};

#endif // INCLUDED_STREAM_H
