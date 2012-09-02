#include "alfe/main.h"

#ifndef INCLUDED_FILE_H
#define INCLUDED_FILE_H

template<class T> class CurrentDirectoryTemplate;
typedef CurrentDirectoryTemplate<void> CurrentDirectory;

template<class T> class FileSystemObjectTemplate;
typedef FileSystemObjectTemplate<void> FileSystemObject;

template<class T> class FindHandleTemplate;
typedef FindHandleTemplate<void> FindHandle;

template<class T> class DirectoryTemplate;
typedef DirectoryTemplate<void> Directory;

template<class T> class FileTemplate;
typedef FileTemplate<void> File;

template<class T> class RootDirectoryImplementationTemplate;
typedef RootDirectoryImplementationTemplate<void> RootDirectoryImplementation;

template<class T> class RootDirectoryTemplate;
typedef RootDirectoryTemplate<void> RootDirectory;

#ifdef _WIN32
template<class T> class DriveRootDirectoryTemplate;
typedef DriveRootDirectoryTemplate<void> DriveRootDirectory;

template<class T> class UNCRootDirectoryTemplate;
typedef UNCRootDirectoryTemplate<void> UNCRootDirectory;

template<class T> class DriveCurrentDirectoryTemplate;
typedef DriveCurrentDirectoryTemplate<void> DriveCurrentDirectory;
#endif

class CharacterSource;

template<class T> class FileSystemObjectTemplate
{
public:
    FileSystemObjectTemplate() { }
    FileSystemObjectTemplate(const String& path,
        const Directory& relativeTo = CurrentDirectory(),
        bool windowsParsing = false)
    {
        *this = FileSystemObject::parse(path, relativeTo, windowsParsing);
    }

    DirectoryTemplate<T> parent() const { return _implementation->parent(); }
    String name() const { return _implementation->name(); }
    bool isRoot() const { return _implementation->isRoot(); }
    String path() const { return _implementation->path(); }
    bool valid() const { return _implementation.valid(); }

    bool operator==(const FileSystemObject& other) const
    {
        return _implementation->compare(other._implementation) == 0;
    }
    bool operator!=(const FileSystemObject& other) const
    {
        return !operator==(other);
    }
    int hash() const { return _implementation->hash(); }
    class Implementation : public ReferenceCounted
    {
    public:
        virtual Directory parent() const = 0;
        virtual String name() const = 0;
        virtual String path() const = 0;
        virtual bool isRoot() const = 0;
        virtual int hash() const = 0;
        virtual int compare(const Implementation* other) const = 0;
    };
protected:
    FileSystemObjectTemplate(const Implementation* implementation)
      : _implementation(implementation) { }

    ConstReference<Implementation> _implementation;
    class NamedImplementation : public Implementation
    {
    public:
        NamedImplementation(const Directory& parent,
            const String& name) : _parent(parent), _name(name) { }
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
        DirectoryTemplate<T> parent() const { return _parent; }
        String name() const { return _name; }
        bool isRoot() const { return false; }
        int hash() const { return _parent.hash()*67 + _name.hash(); }
        int compare(const Implementation* other) const
        {
            const NamedImplementation* o =
                other->constCast<NamedImplementation>();
            if (o == 0)
                return 1;
            if (_parent != o->_parent)
                return 1;
            if (_name != o->_name)
                return 1;
            return 0;
        }
    private:
        Directory _parent;
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
    static DirectoryTemplate<T> windowsParseRoot(const String& path,
        const Directory& relativeTo, CharacterSource& s)
    {
        CharacterSource s2 = s;
        int c = s2.get();
        Directory dir = relativeTo;

        // Process initial slashes
        if (c == '/' || c == '\\') {
            s = s2;
            dir = RootDirectory();
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
        CharacterSource s(path);
        Directory dir = windowsParseRoot(path, relativeTo, s);
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

    static DirectoryTemplate<T> parseRoot(const String& path,
        const Directory& relativeTo, CharacterSource& s)
    {
        CharacterSource s2 = s;
        int c = s2.get();
        Directory dir = relativeTo;

        // Process initial slashes
        if (c == '/') {
            dir = RootDirectory();
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
        CharacterSource s(path);
        Directory dir = parseRoot(path, relativeTo, s);
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

    FileSystemObjectTemplate(const Directory& parent, const String& name)
      : _implementation(new NamedImplementation(parent, name)) { }

    friend class NamedImplementation;
    template<class T> friend class CurrentDirectoryTemplate;
    template<class T> friend class DirectoryTemplate;
    friend class Console;

    template<class T> friend void applyToWildcard(T functor,
        const String& wildcard, int recurseIntoDirectories,
        const Directory& relativeTo);
};

template<class T> class DirectoryTemplate : public FileSystemObject
{
public:
    DirectoryTemplate() { }
    DirectoryTemplate(const String& path,
        const Directory& relativeTo = CurrentDirectory(),
        bool windowsParsing = false)
      : FileSystemObject(path, relativeTo, windowsParsing) { }
    FileSystemObject child(const String& name) const
    {
        return FileSystemObject(*this, name);
    }
    Directory subDirectory(const String& subDirectoryName) const
    {
        return Directory(child(subDirectoryName));
    }
    FileTemplate<T> file(const String& fileName) const
    {
        return File(child(fileName));
    }
    template<class F> void applyToContents(F functor, bool recursive,
        const String& wildcard = "*") const
    {
        FindHandle handle(*this, wildcard);
        while (!handle.complete()) {
            if (handle.isDirectory()) {
                Directory child = handle.directory();
                if (recursive)
                    child.applyToContents(functor, true);
                else
                    functor(child);
            }
            else
                functor(handle.file());
            handle.next();
        }
    }
protected:
    DirectoryTemplate(FileSystemObject object) : FileSystemObject(object) { }
    DirectoryTemplate(const Implementation* implementation)
      : FileSystemObject(implementation) { }
};

template<class T> class CurrentDirectoryTemplate : public Directory
{
public:
    CurrentDirectoryTemplate() : Directory(directory()) { }

private:
    CurrentDirectoryTemplate(const Implementation* implementation)
      : Directory(implementation) { }

    static CurrentDirectory _directory;
    static CurrentDirectory directory()
    {
        if (!_directory.valid())
            _directory = currentDirectory();
        return _directory;
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
        return CurrentDirectory(
            FileSystemObject::parse(path, RootDirectory(), true).
            _implementation);
#else
        size_t size = 100;
        do {
            Array<char> buf(size);
            if (getcwd(&buf[0], size) != 0) {
                String path(&buf[0]);
                return FileSystemObject::parse(path, RootDirectory(), false);
            }
            if (errno != ERANGE)
                throw Exception::systemError("Obtaining current directory");
            size *= 2;
        } while (true);
#endif
    }

#ifdef _WIN32
    template<class T> friend class DriveCurrentDirectoryTemplate;
#endif
};

CurrentDirectory CurrentDirectory::_directory;

#ifdef _WIN32
template<class T> class DriveCurrentDirectoryTemplate : public Directory
{
public:
    DriveCurrentDirectoryTemplate() { }
    DriveCurrentDirectoryTemplate(int drive) : Directory(directory(drive)) { }
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

template<class T> class RootDirectoryTemplate : public Directory
{
public:
    RootDirectoryTemplate() : Directory(directory()) { }

    class Implementation : public FileSystemObject::Implementation
    {
    public:
        Implementation() { }

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

        int hash() const { return 0; }

        int compare(const FileSystemObject::Implementation* other) const
        {
            const Implementation* root =
                dynamic_cast<const Implementation*>(other);
            if (root == 0)
                return 1;
            return 0;
        }
    };
private:
    RootDirectoryTemplate(const Implementation* implementation)
      : Directory(implementation) { }

    static RootDirectory _directory;
    static RootDirectory directory()
    {
        if (!_directory.valid())
            _directory = new Implementation();
        return _directory;
    }
};


RootDirectory RootDirectory::_directory;

#ifdef _WIN32
template<class T> class DriveRootDirectoryTemplate : public Directory
{
public:
    DriveRootDirectoryTemplate() { }
    DriveRootDirectoryTemplate(int drive) : Directory(directory(drive)) { }
private:
    DriveRootDirectoryTemplate(const Implementation* implementation)
      : Directory(implementation) { }

    static DriveRootDirectory _directories[26];
    static DriveRootDirectory directory(int drive)
    {
        if (!_directories[drive].valid())
            _directories[drive] = new Implementation(drive);
        return _directories[drive];
    }
    class Implementation : public RootDirectory::Implementation
    {
    public:
        Implementation(int drive) : _drive(drive) { }

        Directory parent() const { return DriveRootDirectory(_drive); }
        String path() const
        {
            // TODO: Use \\?\ to avoid MAX_PATH limit?
            return codePoint('A' + _drive) + ":";
        }

        int hash() const { return _drive + 1; }

        int compare(const FileSystemObject::Implementation* other) const
        {
            const Implementation* root = other->constCast<Implementation>();
            if (root == 0)
                return 1;
            if (_drive != root->_drive)
                return 1;
            return 0;
        }
    private:
        int _drive;
    };
};

DriveRootDirectory DriveRootDirectory::_directories[26];

template<class T> class UNCRootDirectoryTemplate : public Directory
{
public:
    UNCRootDirectoryTemplate(const String& server, const String& share)
      : Directory(new Implementation(server, share)) { }
private:
    class Implementation : public RootDirectory::Implementation
    {
    public:
        Implementation(const String& server, const String& share)
          : _server(server), _share(share) { }

        Directory parent() const { return UNCRootDirectory(_server, _share); }
        String path() const { return "\\\\" + _server + "\\" + _share; }

        int hash() const { return _server.hash()*67 + _share.hash(); }

        int compare(const FileSystemObject::Implementation* other) const
        {
            const Implementation* root =
                dynamic_cast<const Implementation*>(other);
            if (root == 0)
                return 1;
            if (_server != root->_server)
                return 1;
            if (_share != root->_share)
                return 1;
            return 0;
        }
    private:
        String _server;
        String _share;
    };
};
#endif

template<class T> class FileHandleTemplate;
typedef FileHandleTemplate<void> FileHandle;

template<class T> class FileTemplate : public FileSystemObject
{
public:
    FileTemplate() { }
    FileTemplate(const String& path,
        const Directory& relativeTo = CurrentDirectory(),
        bool windowsParsing = false)
      : FileSystemObject(path, relativeTo, windowsParsing) { }

    String contents() const
    {
        FileHandle f = openRead();
        UInt64 size = f.size();
        if (size >= 0x80000000)
            throw Exception("2Gb or more in file " + path());
        int intSize = static_cast<int>(size);
        String buffer(intSize);
        f.read(static_cast<void*>(buffer.data()), intSize);
        buffer._length = intSize;
        return buffer;
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
            f = openWriteTemporary();
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
            REPLACEFILE_IGNORE_MERGE_ERRORS) == 0) {
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
    FileHandleTemplate<T> openRead() const
    {
#ifdef _WIN32
        return open(GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING,
            FILE_FLAG_SEQUENTIAL_SCAN);
#else
        return open(O_RDONLY);
#endif
    }
    FileHandleTemplate<T> openWrite() const
    {
#ifdef _WIN32
        return open(GENERIC_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL);
#else
        return open(O_WRONLY | O_CREAT | O_TRUNC);
#endif
    }
    FileHandleTemplate<T> tryOpenRead() const
    {
#ifdef _WIN32
        return tryOpen(GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING,
            FILE_FLAG_SEQUENTIAL_SCAN);
#else
        return tryOpen(O_RDONLY);
#endif
    }
    FileHandleTemplate<T> tryOpenWrite() const
    {
#ifdef _WIN32
        return tryOpen(GENERIC_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL);
#else
        return tryOpen(O_WRONLY | O_CREAT | O_TRUNC);
#endif
    }
    FileHandleTemplate<T> openWriteTemporary() const
    {
        int i = 0;
        do {
            File temp = parent().file(name() + hex(i, 8, false));
#ifdef _WIN32
            FileHandle f = open(tempPath, GENERIC_WRITE, 0, CREATE_NEW,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, false);
#else
            FileHandle f = open(tempPath, O_WRONLY | O_CREAT | O_EXCL, false);
#endif
            if (f.valid())
                return f;
            ++i;
        } while (true);
    }
    FileHandleTemplate<T> openAppend() const
    {
#ifdef _WIN32
        return open(name(), GENERIC_WRITE, 0, OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL);
#else
        return open(name(), O_WRONLY | O_APPEND);
#endif
    }
private:
#ifdef _WIN32
    FileHandleTemplate<T> open(DWORD dwDesiredAccess, DWORD dwShareMode,
        DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
        bool throwIfExists = true) const
    {
        FileHandle f = tryOpen(dwDesiredAccess, dwShareMode,
            dwCreationDisposition, dwFlagsAndAttributes);
        if (!f.valid() &&
            (throwIfExists || GetLastError() == ERROR_FILE_EXISTS))
            throw Exception::systemError("Opening file " + path());
        return f;
    }
    FileHandleTemplate<T> tryOpen(DWORD dwDesiredAccess, DWORD dwShareMode,
        DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes) const
    {
        NullTerminatedWideString data(path());
        return FileHandle(CreateFile(
            data,   // lpFileName
            dwDesiredAccess,
            dwShareMode,
            NULL,   // lpSecurityAttributes
            dwCreationDisposition,
            dwFlagsAndAttributes,
            NULL),  // hTemplateFile
            *this);
    }
#else
    FileHandle open(int flags, bool throwIfExists = true) const
    {
        FileHandle f = tryOpen(flags);
        if (!f.valid() && (throwIfExists || errno != EEXIST))
            throw Exception::systemError("Opening file " + path());
        return f;
    }
    FileHandle tryOpen(int flags) const
    {
        NullTerminatedString data(path());
        return FileHandle(open(data, flags), *this);
    }
#endif

    FileTemplate(FileSystemObject object) : FileSystemObject(object) { }

    friend class DirectoryTemplate<T>;
    friend class Console;
};

template<class T> void applyToWildcard(T functor, CharacterSource s,
    int recurseIntoDirectories, Directory directory)
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
    FindHandle handle(directory, name);
    while (!handle.complete()) {
        if (handle.isDirectory()) {
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
                functor(handle.file());
        handle.next();
    }
}

template<class T> void applyToWildcard(T functor, const String& wildcard,
    int recurseIntoDirectories = true,
    const Directory& relativeTo = CurrentDirectory())
{
    CharacterSource s(wildcard);
#ifdef _WIN32
    Directory dir =
        FileSystemObject::windowsParseRoot(wildcard, relativeTo, s);
#else
    Directory dir = FileSystemObject::parse(wildcard, relativeTo, s);
#endif
    applyToWildcard(functor, s, recurseIntoDirectories, dir);
}

class Console : public File
{
public:
    Console() : File(FileSystemObject(new Implementation)) { }
private:
    class Implementation : public FileSystemObject::Implementation
    {
    public:
        String path() const { return "(console)"; }
        Directory parent() const { return RootDirectory(); }
        String name() const { return path(); }
        bool isRoot() const { return false; }
        int hash() const { return 27; }
        int compare(const FileSystemObject::Implementation* other) const
        {
            const Implementation* c =
                dynamic_cast<const Implementation*>(other);
            if (c == 0)
                return 1;
            return 0;
        }
    };
};

#endif // INCLUDED_FILE_H
