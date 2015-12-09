#include "alfe/main.h"

#ifndef INCLUDED_FILE_STREAM_H
#define INCLUDED_FILE_STREAM_H

template<class T> class FileStreamT : public AutoStream
{
public:
#ifdef _WIN32
    FileStreamT(HANDLE handle, const File& file) : AutoStream(handle, file) { }
#else
    FileStreamT(int fileDescriptor, const File& file)
      : AutoStream(fileDescriptor, file) { }
#endif
#ifndef _WIN32
    void sync()
    {
        if (fsync(operator int()) != 0)
            throw Exception::systemError(
                "Synchronizing file " + file().path());
    }
#endif
    UInt64 size()
    {
#ifdef _WIN32
        LARGE_INTEGER size;
        if (GetFileSizeEx(operator HANDLE(), &size) == 0)
            throw Exception::systemError(
                "Obtaining length of file " + file().path());
        return size.QuadPart;
#else
        off_t o = seek(0, SEEK_CUR);
        off_t n = seek(0, SEEK_END);
        seek(o, SEEK_SET);
        return n;
#endif
    }
    void seek(UInt64 position)
    {
#ifdef _WIN32
        LARGE_INTEGER p;
        p.QuadPart = position;
        if (SetFilePointerEx(operator HANDLE(), p, NULL, FILE_BEGIN) == 0)
            throw Exception::systemError("Seeking file " + file().path());
#else
        seek(position, SEEK_SET);
#endif
    }
private:
#ifndef _WIN32
    off_t seek(off_t offset, int whence)
    {
        off_t n = lseek(operator int(), offset, whence);
        if (n == (off_t)(-1))
            throw Exception::systemError("Seeking file " + file().path());
        return n;
    }
#endif
};

#endif // INCLUDED_FILE_STREAM_H
