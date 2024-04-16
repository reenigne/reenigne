#include "alfe/main.h"

#ifndef INCLUDED_FILE_H
#define INCLUDED_FILE_H

template<class T> class CurrentDirectoryT;
typedef CurrentDirectoryT<void> CurrentDirectory;

template<class T> class FileSystemObjectT;
typedef FileSystemObjectT<void> FileSystemObject;

template<class T> class FindHandleT;
typedef FindHandleT<void> FindHandle;

template<class T> class DirectoryT;
typedef DirectoryT<void> Directory;

template<class T> class FileT;
typedef FileT<void> File;

template<class T> class RootDirectoryT;
typedef RootDirectoryT<void> RootDirectory;

#ifdef _WIN32
template<class T> class DriveRootDirectoryT;
typedef DriveRootDirectoryT<void> DriveRootDirectory;

template<class T> class UNCRootDirectoryT;
typedef UNCRootDirectoryT<void> UNCRootDirectory;

template<class T> class DriveCurrentDirectoryT;
typedef DriveCurrentDirectoryT<void> DriveCurrentDirectory;
#endif

template<class T> class CharacterSourceT;
typedef CharacterSourceT<void> CharacterSource;

template<class T> class FileSystemObjectT : public ConstHandle
{
public:
    FileSystemObjectT() { }
    FileSystemObjectT(const ConstHandle& other) : ConstHandle(other) { }
    FileSystemObjectT(const String& path,
        const DirectoryT<T>& relativeTo = CurrentDirectoryT<T>(),
        bool windowsParsing = false)
    {
        *this = FileSystemObject::parse(path, relativeTo, windowsParsing);
    }

    DirectoryT<T> parent() const { return body()->parent(); }
    String name() const { return body()->name(); }
    bool isRoot() const { return body()->isRoot(); }
    String path() const
    {
        if (!valid())
            return "(unknown path)";
        return body()->path();
    }

    class Body : public ConstHandle::Body
    {
    public:
        virtual DirectoryT<T> parent() const = 0;
        virtual String name() const = 0;
        virtual String path() const = 0;
        virtual bool isRoot() const = 0;
    };
    bool exists()
    {
#ifdef _WIN32
        NullTerminatedWideString data(path());
        DWORD dwAttr = GetFileAttributes(data);
        if (dwAttr == 0xffffffff) {
            DWORD dwError = GetLastError();
            if (dwError == ERROR_FILE_NOT_FOUND)
                return false;
            if (dwError == ERROR_PATH_NOT_FOUND)
                return false;
            IF_ZERO_CHECK_THROW_LAST_ERROR(0);
        }
        return true;
#else
        NullTerminatedString data(path());
        return access(data, F_OK) == 0;
#endif
    }
protected:
    const Body* body() const { return as<Body>(); }

    class NamedBody : public Body
    {
    public:
        NamedBody(const Directory& parent, const String& name)
          : _parent(parent), _name(name) { }
#ifdef _WIN32
        String path() const
        {
            return _parent.path() + "\\" + _name;
        }
#else
        String path() const
        {
            return _parent.path() + "/" + _name;
        }
#endif
        DirectoryT<T> parent() const { return _parent; }
        String name() const { return _name; }
        bool isRoot() const { return false; }
        Hash hash() const
        {
            return Body::hash().mixin(_parent.hash()).mixin(_name.hash());
        }
        bool equals(const ConstHandle::Body* other) const
        {
            auto o = other->to<NamedBody>();
            return o != 0 && _parent == o->_parent & _name == o->_name;
        }
    private:
        DirectoryT<T> _parent;
        String _name;
    };

private:
    static FileSystemObject parse(const String& path,
        const Directory& relativeTo, bool windowsParsing)
    {
        if (path.empty())
            throw Exception("Invalid path");

#ifdef _WIN32
        if (windowsParsing)
            return windowsParse(path, relativeTo);
#endif
        return parse(path, relativeTo);
    }

#ifdef _WIN32
    static DirectoryT<T> windowsParseRoot(const String& path,
        const Directory& relativeTo, CharacterSource& s)
    {
        CharacterSourceT<T> s2 = s;
        int c = s2.get();
        DirectoryT<T> dir = relativeTo;

        // Process initial slashes
        if (c == '/' || c == '\\') {
            s = s2;
            dir = RootDirectoryT<T>();
            c = s2.get();
            if (c == -1)
                return dir;
            if (c == '/' || c == '\\') {
                int serverStart = s2.offset();
                int p;
                do {
                    p = s2.offset();
                    c = s2.get();
                    if (c == -1)
                        throw Exception("Invalid path");
                    // TODO: What characters are actually legal in server
                    // names?
                } while (c != '\\' && c != '/');
                String server = s2.subString(serverStart, p);
                int shareStart;
                do {
                    shareStart = s2.offset();
                    c = s2.get();
                } while (c == '/' || c == '\\');
                do {
                    p = s2.offset();
                    c = s2.get();
                    // TODO: What characters are actually legal in share names?
                } while (c != '\\' && c != '/' && c != -1);
                String share = s2.subString(shareStart, p);
                dir = UNCRootDirectory(server, share);
                do {
                    s = s2;
                    c = s2.get();
                } while (c == '/' || c == '\\');
            }
            // TODO: In paths starting \\?\, only \ and \\ are allowed
            // separators, and ?*:"<> are allowed. See
            // http://docs.racket-lang.org/reference/windowspaths.html for more
            // details.
            return dir;
        }
        int drive = (c >= 'a' ? (c - 'a') : (c - 'A'));
        if (drive < 0 || drive >= 26)
            return dir;
        c = s2.get();
        if (c != ':')
            return dir;
        s = s2;
        c = s2.get();
        if (c == '/' || c == '\\') {
            dir = DriveRootDirectory(drive);
            while (c == '/' || c == '\\') {
                s = s2;
                c = s2.get();
            }
            return dir;
        }
        return DriveCurrentDirectory(drive);
    }

    static FileSystemObject windowsParse(const String& path,
        const Directory& relativeTo)
    {
        CharacterSourceT<T> s(path);
        DirectoryT<T> dir = windowsParseRoot(path, relativeTo, s);
        int subDirectoryStart = s.offset();
        int c = s.get();

        String name;
        do {
            if (c == -1)
                break;
            int p;
            while (c != '/' && c != '\\') {
                if (c < 32 || c == '?' || c == '*' || c == ':' || c == '"' ||
                    c == '<' || c == '>')
                    throw Exception("Invalid path");
                p = s.offset();
                c = s.get();
                if (c == -1)
                    break;
            }
            name = s.subString(subDirectoryStart, p);
            if (name == ".")
                name = "";
            if (name == "..") {
                dir = dir.parent();
                name = "";
            }
            if (name != "") {
                int l = name[name.length() - 1];
                if (l == '.' || l == ' ')
                    throw Exception("Invalid path");
            }
            if (c == -1)
                break;
            while (c == '/' || c == '\\') {
                subDirectoryStart = s.offset();
                c = s.get();
                if (c == -1)
                    break;
            }
            if (c == -1)
                break;
            if (name != "")
                dir = dir.subDirectory(name);
        } while (true);
        if (name == "") {
            if (dir.isRoot())
                return dir;
            return FileSystemObject(dir.parent(), dir.name());
        }
        return FileSystemObject(dir, name);
    }
#endif

    static DirectoryT<T> parseRoot(const String& path,
        const DirectoryT<T>& relativeTo, CharacterSource& s)
    {
        CharacterSourceT<T> s2 = s;
        int c = s2.get();
        Directory dir = relativeTo;

        // Process initial slashes
        if (c == '/') {
            dir = RootDirectoryT<T>();
            while (c == '/') {
                s = s2;
                c = s2.get();
            }
        }
        return dir;
    }

    static FileSystemObject parse(const String& path,
        const Directory& relativeTo)
    {
        CharacterSourceT<T> s(path);
        DirectoryT<T> dir = parseRoot(path, relativeTo, s);
        int subDirectoryStart = s.offset();
        int c = s.get();

        String name;
        do {
            int p;
            while (c != '/') {
                if (c == 0)
                    throw Exception("Invalid path");
                p = s.offset();
                c = s.get();
                if (c == -1)
                    break;
            }
            name = s.subString(subDirectoryStart, p);
            if (name == String("."))
                name = String();
            if (name == String("..")) {
                dir = dir.parent();
                name = String();
            }
            if (c == -1)
                break;
            while (c == '/') {
                subDirectoryStart = s.offset();
                c = s.get();
                if (c == -1)
                    break;
            }
            if (c == -1)
                break;
            if (name != "")
                dir = dir.subDirectory(name);
        } while (true);
        if (name == "") {
            if (dir.isRoot())
                return dir;
            return FileSystemObject(dir.parent(), dir.name());
        }
        return FileSystemObject(dir, name);
    }

    FileSystemObjectT(const DirectoryT<T>& parent, const String& name)
      : ConstHandle(create<NamedBody>(parent, name)) { }

    friend class NamedBody;
    template<class U> friend class CurrentDirectoryT;
    template<class U> friend class DirectoryT;
    friend class Console;

    template<class U> friend void applyToWildcard(U& functor,
        const String& wildcard, int recurseIntoDirectories,
        const DirectoryT<T>& relativeTo);
};

template<class T> class DirectoryT : public FileSystemObject
{
public:
    DirectoryT() { }
    DirectoryT(const ConstHandle& other) : FileSystemObject(other) { }

    DirectoryT(const String& path,
        const Directory& relativeTo = CurrentDirectoryT<T>(),
        bool windowsParsing = false)
      : FileSystemObject(path, relativeTo, windowsParsing) { }
    DirectoryT(const String& path, bool windowsParsing)
      : FileSystemObject(path, CurrentDirectoryT<T>(), windowsParsing) { }

    FileSystemObject child(const String& name) const
    {
        return FileSystemObject(*this, name);
    }
    Directory subDirectory(const String& subDirectoryName) const
    {
        return Directory(child(subDirectoryName));
    }
    FileT<T> file(const String& fileName) const
    {
        return File(child(fileName));
    }
    template<class F> void applyToContents(F& functor, bool recursive,
        const String& wildcard = "*") const
    {
        FindHandleT<T> handle(*this, wildcard);
        while (!handle.complete()) {
            if (handle.isDirectory()) {
                if (!handle.isSymlink()) {
                    Directory child = handle.directory();
                    if (recursive)
                        child.applyToContents(functor, true);
                    else
                        functor(child);
                }
            }
            else
                functor(handle.file());
            handle.next();
        }
    }
};

// This is the current directory at the time of first instantiation - avoid
// changing the current directory.
template<class T> class CurrentDirectoryT : public Directory
{
public:
    CurrentDirectoryT() : Directory(directory()) { }
private:
    CurrentDirectoryT(const FileSystemObject& other)
      : Directory(other) { }
    static CurrentDirectory directory()
    {
        static CurrentDirectory d = currentDirectory();
        return d;
    }

    static CurrentDirectory currentDirectory()
    {
#ifdef _WIN32
        int n = GetCurrentDirectory(0, NULL);
        if (n == 0)
            throw Exception::systemError("Obtaining current directory");
        Array<WCHAR> buf(n);
        if (GetCurrentDirectory(n, &buf[0]) == 0)
            throw Exception::systemError("Obtaining current directory");
        String path(&buf[0]);
        return FileSystemObject::parse(path, RootDirectoryT<T>(), true);
#else
        size_t size = 100;
        do {
            String buffer(size);
            char* p = reinterpret_cast<char*>(buffer.data());
            if (getcwd(p, size) != 0) {
                String path = buffer.subString(0, strlen(p));
                return FileSystemObject::parse(path, RootDirectory(), false);
            }
            if (errno != ERANGE)
                throw Exception::systemError("Obtaining current directory");
            size *= 2;
        } while (true);
#endif
    }

#ifdef _WIN32
    template<class T> friend class DriveCurrentDirectoryT;
#endif
};

#ifdef _WIN32
template<class T> class DriveCurrentDirectoryT : public Directory
{
public:
    DriveCurrentDirectoryT() { }
    DriveCurrentDirectoryT(int drive) : Directory(directory(drive)) { }
private:
    static Directory _directories[26];
    static Directory directory(int drive)
    {
        if (!_directories[drive].valid()) {
            // Make sure the current directory has been retrieved
            CurrentDirectory();

            // Change to this drive
            WCHAR buf[3];
            buf[0] = drive + 'A';
            buf[1] = ':';
            buf[2] = 0;
            if (SetCurrentDirectory(&buf[0]) == 0)
                throw Exception::systemError("Setting current directory");

            // Retrieve current directory
            _directories[drive] = CurrentDirectory::currentDirectory();
        }
        return _directories[drive];
    }
};

Directory DriveCurrentDirectory::_directories[26];

#endif

template<class T> class RootDirectoryT : public Directory
{
public:
    RootDirectoryT() : Directory(directory()) { }
    RootDirectoryT(const ConstHandle& other) : Directory(other) { }

    class Body : public FileSystemObject::Body
    {
    public:
        Body() { }

        Directory parent() const { return RootDirectory(); }
        String name() const { return String(); }
        String path() const
        {
#ifdef _WIN32
            // TODO: Use \\?\ to avoid MAX_PATH limit?
            // If we do this we need to know the current drive, which can be
            // found from CurrentDirectory().
#endif
            return String();
        }
        bool isRoot() const { return true; }
    };
private:
    static RootDirectory directory()
    {
        static RootDirectory d = create<Body>();
        return d;
    }
};


#ifdef _WIN32
template<class T> class DriveRootDirectoryT : public Directory
{
public:
    DriveRootDirectoryT() { }
    DriveRootDirectoryT(int drive) : Directory(directory(drive)) { }
    DriveRootDirectoryT(const ConstHandle& other) : Directory(other) { }
private:
    static DriveRootDirectory _directories[26];
    static DriveRootDirectory directory(int drive)
    {
        if (!_directories[drive].valid())
            _directories[drive] = create<Body>(drive);
        return _directories[drive];
    }
    class Body : public RootDirectory::Body
    {
    public:
        Body(int drive) : _drive(drive) { }

        Directory parent() const { return DriveRootDirectory(_drive); }
        String path() const
        {
            // TODO: Use \\?\ to avoid MAX_PATH limit?
            return codePoint('A' + _drive) + ":";
        }

        Hash hash() const { return RootDirectory::Body::hash().mixin(_drive); }

        bool equals(const ConstHandle::Body* other) const
        {
            auto o = other->to<Body>();
            return o != 0 && _drive == o->_drive;
        }
    private:
        int _drive;
    };
};

DriveRootDirectory DriveRootDirectory::_directories[26];

template<class T> class UNCRootDirectoryT : public Directory
{
public:
    UNCRootDirectoryT(const String& server, const String& share)
      : Directory(create<Body>(server, share)) { }
private:
    class Body : public RootDirectory::Body
    {
    public:
        Body(const String& server, const String& share)
          : _server(server), _share(share) { }

        Directory parent() const { return UNCRootDirectory(_server, _share); }
        String path() const { return "\\\\" + _server + "\\" + _share; }

        Hash hash() const
        {
            return RootDirectory::Body::hash().mixin(_server.hash()).
                mixin(_share.hash());
        }

        bool equals(const ConstHandle::Body* other) const
        {
            auto o = other->to<Body>();
            return o != 0 && _server == o->_server && _share == o->_share;
        }
    private:
        String _server;
        String _share;
    };
};
#endif

template<class T> class FileStreamT;
typedef FileStreamT<void> FileStream;

template<class T> class FileT : public FileSystemObject
{
public:
    FileT() { }
    FileT(const ConstHandle& other) : FileSystemObject(other) { }
    FileT(const String& path,
        const Directory& relativeTo = CurrentDirectory(),
        bool windowsParsing = false)
      : FileSystemObject(path, relativeTo, windowsParsing) { }

    FileT(const String& path, bool windowsParsing)
      : FileSystemObject(path, CurrentDirectory(), windowsParsing) { }

    String contents() const
    {
        FileStreamT<T> f = openRead();
        UInt64 size = f.size();
        if (size >= 0x80000000)
            throw Exception("2Gb or more in file " + path());
        int intSize = static_cast<int>(size);
        String buffer(intSize);
        f.read(buffer.data(), intSize);
        return buffer;
    }
    template<class U> void readIntoArray(Array<U>* array)
    {
        FileStreamT<T> f = openRead();
        UInt64 size = f.size();
        if (size >= 0x80000000)
            throw Exception("2Gb or more in file " + path());
        int intSize = static_cast<int>(size);
        int n = intSize/sizeof(U);
        array->allocate(n);
        f.read(&(*array)[0], n*sizeof(U));
    }

    template<class U> void save(const U& contents) const
    {
        openWrite().write(contents);
    }
    void save(const Byte* data, int length) const
    {
        openWrite().write(data, length);
    }
    template<class U> void secureSave(const U& contents) const
    {
        // TODO: Backup file?
        File temp;
        {
            FileStreamT<T> f = openWriteTemporary();
            f.write(contents);
#ifndef _WIN32
            f.sync();
#endif
            temp = f.file();
        }
#ifdef _WIN32
        NullTerminatedWideString data(path());
        NullTerminatedWideString tempData(temp.path());
        if (ReplaceFile(data, tempData, NULL, REPLACEFILE_WRITE_THROUGH |
            REPLACEFILE_IGNORE_MERGE_ERRORS, NULL, NULL) == 0) {
            {
                PreserveSystemError p;
                DeleteFile(tempData);  // Ignore any errors
            }
            throw Exception::systemError("Replacing file " + path());
        }
#else
        NullTerminatedString data(path());
        NullTerminatedString tempData(temp.path());
        if (rename(tempData, data) != 0) {
            {
                PreserveSystemError p;
                unlink(tempData);  // Ignore any errors
            }
            throw Exception::systemError("Replacing file " + path());
        }
#endif
    }
    template<class U> void append(const U& contents) const
    {
        openAppend().write(contents);
    }
    FileStreamT<T> openRead() const
    {
#ifdef _WIN32
        return open(GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING,
            FILE_FLAG_SEQUENTIAL_SCAN);
#else
        return open(O_RDONLY);
#endif
    }
    FileStreamT<T> openWrite() const
    {
#ifdef _WIN32
        return open(GENERIC_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL);
#else
        return openWrite(O_WRONLY | O_CREAT | O_TRUNC,
            S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
#endif
    }
    FileStreamT<T> tryOpenRead() const
    {
#ifdef _WIN32
        return tryOpen(GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING,
            FILE_FLAG_SEQUENTIAL_SCAN);
#else
        return tryOpen(O_RDONLY);
#endif
    }
    FileStreamT<T> tryOpenWrite() const
    {
#ifdef _WIN32
        return tryOpen(GENERIC_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL);
#else
        return tryOpenWrite(O_WRONLY | O_CREAT | O_TRUNC,
            S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
#endif
    }
    FileStreamT<T> openWriteTemporary() const
    {
        int i = 0;
        do {
            File temp = parent().file(name() + hex(i, 8, false));
#ifdef _WIN32
            FileStreamT<T> f = temp.open(GENERIC_WRITE, 0, CREATE_NEW,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, false);
#else
            FileStreamT<T> f = temp.open(O_WRONLY | O_CREAT | O_EXCL,
                false);
#endif
            if (f.valid())
                return f;
            ++i;
        } while (true);
    }
    FileStreamT<T> openAppend() const
    {
#ifdef _WIN32
        return open(GENERIC_WRITE, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL);
#else
        return open(name(), O_WRONLY | O_APPEND);
#endif
    }
    void remove()
    {
#ifdef _WIN32
        NullTerminatedWideString data(path());
        IF_ZERO_THROW(DeleteFile(data));
#else
        NullTerminatedString data(path());
        IF_MINUS_ONE_THROW(unlink(data));
#endif
    }
private:
#ifdef _WIN32
    FileStreamT<T> open(DWORD dwDesiredAccess, DWORD dwShareMode,
        DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
        bool throwIfExists = true) const
    {
        FileStream f = tryOpen(dwDesiredAccess, dwShareMode,
            dwCreationDisposition, dwFlagsAndAttributes);
        if (!f.valid() &&
            (throwIfExists || GetLastError() == ERROR_FILE_EXISTS))
            throw Exception::systemError("Opening file " + path());
        return f;
    }
    FileStreamT<T> tryOpen(DWORD dwDesiredAccess, DWORD dwShareMode,
        DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes) const
    {
        NullTerminatedWideString data(path());
        return FileStream(CreateFile(
            data,   // lpFileName
            dwDesiredAccess,
            dwShareMode,
            NULL,   // lpSecurityAttributes
            dwCreationDisposition,
            dwFlagsAndAttributes,
            NULL),  // hTemplateFile
            *this);
    }
public:
    StreamT<T> openPipe()
    {
        StreamT<T> f = tryOpen(GENERIC_READ | GENERIC_WRITE, 0,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL);
        if (!f.valid())
            throw Exception::systemError("Opening pipe " + path());
        return f;
    }
    StreamT<T> createPipe(bool overlapped = false)
    {
        NullTerminatedWideString data(path());
        StreamT<T> f(CreateNamedPipe(
            data,                // lpName
            PIPE_ACCESS_DUPLEX |
                (overlapped ? FILE_FLAG_OVERLAPPED : 0),  // dwOpenMode
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,  // dwPipeMode
            PIPE_UNLIMITED_INSTANCES,  // nMaxInstances
            512,   // nOutBufferSize
            512,   // nInBufferSize
            0,     // nDefaultTimeOut
            NULL), // lpSecurityAttributes
            *this);
        if (!f.valid())
            throw Exception::systemError("Creating pipe " + path());
        return f;
    }
private:
#else
    FileStreamT<T> open(int flags, bool throwIfExists = true) const
    {
        FileStreamT<T> f = tryOpen(flags);
        if (!f.valid() && (throwIfExists || errno != EEXIST))
            throw Exception::systemError("Opening file " + path());
        return f;
    }
    FileStreamT<T> tryOpen(int flags) const
    {
        NullTerminatedString data(path());
        return FileStream(::open(data, flags), *this);
    }
    FileStreamT<T> openWrite(int flags, mode_t mode,
        bool throwIfExists = true) const
    {
        FileStreamT<T> f = tryOpenWrite(flags, mode);
        if (!f.valid() && (throwIfExists || errno != EEXIST))
            throw Exception::systemError("Opening file " + path());
        return f;
    }
    FileStreamT<T> tryOpenWrite(int flags, mode_t mode) const
    {
        NullTerminatedString data(path());
        return FileStream(::open(data, flags, mode), *this);
    }
#endif

    friend class DirectoryT<T>;
    friend class Console;
};

template<class T, class V = Void> void applyToWildcard(T& functor,
	CharacterSourceT<V> s, int recurseIntoDirectories, Directory directory)
{
    int subDirectoryStart = s.offset();
    int c = s.get();
    int p = s.offset();
#ifdef _WIN32
    while (c != '/' && c != '\\' && c != -1) {
        if (c < 32 || c == ':' || c == '"' || c == '<' || c == '>')
            throw Exception("Invalid path");
        p = s.offset();
        c = s.get();
    }
    String name = s.subString(subDirectoryStart, p);
    CharacterSource s2 = s;
    while (c == '/' || c == '\\') {
        s = s2;
        c = s2.get();
    }
    if (name == ".") {
        if (c == -1)
            if (recurseIntoDirectories)
                name = String("*");
            else {
                functor(directory);
                return;
            }
        else {
            applyToWildcard(functor, s, recurseIntoDirectories, directory);
            return;
        }
    }
    if (name == "..") {
        if (c == -1)
            if (recurseIntoDirectories) {
                name = String("*");
                directory = directory.parent();
            }
            else {
                functor(directory.parent());
                return;
            }
        else {
            applyToWildcard(functor, s, recurseIntoDirectories,
                directory.parent());
            return;
        }
    }
    if (name != "") {
        int l = name[name.length() - 1];
        if (l == '.' || l == ' ')
            throw Exception("Invalid path");
    }
    else
        if (recurseIntoDirectories)
            name = String("*");
        else {
            functor(directory);
            return;
        }
#else
    while (c != '/' && c != -1) {
        p = s.offset();
        c = s.get();
    }
    String name = s.subString(subDirectoryStart, p);
    while (c == '/')
        c = s.get();
    if (name == ".") {
        applyToWildcard(functor, s, recurseIntoDirectories, directory);
        return;
    }
    if (name == "..") {
        applyToWildcard(functor, s, recurseIntoDirectories,
            directory.parent());
        return;
    }
#endif
    FindHandleT<T> handle(directory, name);
    while (!handle.complete()) {
        if (handle.isDirectory() && !handle.isJunction()) {
            Directory child = handle.directory();
            if (c == -1)
                if (recurseIntoDirectories)
                    child.applyToContents(functor, true);
                else
                    functor(child);
            else
                applyToWildcard(functor, s, recurseIntoDirectories, child);
        }
        else
            if (c == -1)
                functor((File)handle.file());
        handle.next();
    }
}

template<class T> void applyToWildcard(T& functor, const String& wildcard,
    int recurseIntoDirectories = true,
    const Directory& relativeTo = CurrentDirectory())
{
    CharacterSource s(wildcard);
#ifdef _WIN32
    Directory dir =
        FileSystemObject::windowsParseRoot(wildcard, relativeTo, s);
#else
    Directory dir = FileSystemObject::parseRoot(wildcard, relativeTo, s);
#endif
    applyToWildcard(functor, s, recurseIntoDirectories, dir);
}

class Console : public File
{
public:
    Console() : File(create<Body>()) { }
private:
    class Body : public FileSystemObject::Body
    {
    public:
        String path() const { return "(console)"; }
        Directory parent() const { return RootDirectory(); }
        String name() const { return path(); }
        bool isRoot() const { return false; }
    };
};

#endif // INCLUDED_FILE_H
