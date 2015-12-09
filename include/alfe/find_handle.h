#include "alfe/main.h"

#ifndef INCLUDED_FIND_HANDLE_H
#define INCLUDED_FIND_HANDLE_H

#ifdef _WIN32
template<class T> class FindHandleT
{
public:
    FindHandleT(const Directory& directory, const String& wildcard)
      : _directory(directory), _wildcard(wildcard), _complete(false),
        _handle(INVALID_HANDLE_VALUE)
    {
        _path = directory.child(wildcard).path();
        NullTerminatedWideString data(_path);
        _handle = FindFirstFile(data, &_data);
        if (_handle == INVALID_HANDLE_VALUE) {
            if (GetLastError() == ERROR_FILE_NOT_FOUND)
                _complete = true;
            else
                throwError();
        }
        String n = name();
        if (n == "." || n == "..")
            next();
    }
    void next()
    {
        do {
            if (FindNextFile(_handle, &_data) == 0)
                if (GetLastError() == ERROR_NO_MORE_FILES) {
                    _complete = true;
                    return;
                }
                else
                    throwError();
            String n = name();
            if (n == "." || n == "..")
                continue;
            break;
        } while (true);
    }
    ~FindHandleT()
    {
        if (_handle != INVALID_HANDLE_VALUE)
            FindClose(_handle);
    }
    bool isDirectory() const
    {
        return (_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }
    String name() const
    {
        return _data.cFileName;
    }
    FileSystemObject object() const
    {
        return _directory.child(name());
    }
    DirectoryT<T> directory() const
    {
        return _directory.subDirectory(name());
    }
    FileT<T> file() const
    {
        return _directory.file(name());
    }
    bool complete() { return _complete; }
private:
    void throwError()
    {
        throw Exception::systemError("Finding files " + _path);
    }

    HANDLE _handle;
    WIN32_FIND_DATA _data;
    Directory _directory;
    String _wildcard;
    String _path;
    bool _complete;
};
#else
template<class T> class FindHandleT
{
public:
    FindHandleT(const Directory& directory, const String& wildcard)
      : _directory(directory), _wildcard(wildcard), _complete(false),
        _dir(NULL)
    {
        _path = directory.child(wildcard).path();
        NullTerminatedString data(_path);
        _dir = opendir(data);
        if (_dir == NULL)
            throw Exception::systemError("Opening directory " + _path);
        next();
    }
    void next()
    {
        do {
            errno = 0;
            _data = readdir(_dir);
            if (_data == NULL)
                if (errno == 0)
                    _complete = true;
                else
                    throw Exception::systemError("Reading directory " + _path);
            String n = name();
            if (n == "." || n == "..")
                continue;
            if (!matches(n, _wildcard))
                continue;
            break;
        } while (true);
    }
    ~FindHandleT()
    {
        if (_dir != NULL)
            closedir(_dir);
    }
    bool isDirectory() const
    {
        return _data->d_type == DT_DIR;
    }
    String name() const
    {
        return _data->d_name;
    }
    FileSystemObject object() const
    {
        return _directory.child(name());
    }
    Directory directory() const
    {
        return _directory.subDirectory(name());
    }
    File file() const
    {
        return _directory.file(name());
    }
    bool complete() { return _complete; }
private:
    static bool matches(String name, String wildcard)
    {
        CharacterSourceT<T> sw(wildcard);
        CharacterSourceT<T> sn(name);
        do {
            int cs = sw.get();
            int cn = sn.get();
            switch (cs) {
                case '?':
                    if (cn == -1)
                        return false;
                    continue;
                case '*':
                    // TODO: this code is O(n^p) where p is number of stars, we
                    // can do better using dynamic programming.
                    if (cn == -1)
                        continue;
                    do {
                        if (matches(name.subString(sn.offset(),
                            name.length() - sn.offset()), sw.offset(),
                            wildcard.length() - sw.offset()))
                            return true;
                        cn = sn.get();
                    } while (cn != -1);
                    return false;
                case -1:
                    return (cn == -1);
                default:
                    return (cn == cs);
            }
        } while (true);
    }

    struct dirent* _data;
    DIR* _dir;
    Directory _directory;
    String _wildcard;
    String _path;
    bool _complete;
};
#endif

#endif // INCLUDED_FIND_HANDLE_H
