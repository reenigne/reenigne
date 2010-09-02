#ifndef INCLUDED_FILE_H
#define INCLUDED_FILE_H

#include "string.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

extern String pathDelimiter;

class FileSystemObject
{
public:
    FileSystemObject(const String& path, const Directory& relativeTo = CurrentDirectory(), bool windowsParsing = false)
    {
        *this = FileSystemObject::parse(path, relativeTo, windowsParsing));
    }

    Directory parent() const { return _implementation->parent(); }

    String path() const { return _implementation->path(); }

    bool operator==(const FileSystemObject& other)
    {
        return _implementation->compare(other._implementation) == 0;
    }
    bool operator!=(const FileSystemObject& other) const { return !operator==(other); }
    int hash() const { return _implementation->hash(0); }
#ifdef _WIN32
    String windowsPath() const { return _implementation->windowsPath(); }
#endif
protected:
    FileSystemObject(Reference<FileSystemObjectImplementation> implementation) : _implementation(implementation) { }

    Reference<FileSystemObjectImplementation> _implementation;
private:
    FileSystemObject(const Directory& parent, const String& name) : _implementation(new NamedFileSystemObjectImplementation(parent, name)) { }

    friend class NamedFileSystemObjectImplementation;
};

class Directory : public FileSystemObject
{
public:
    Directory(const String& path, const Directory& relativeTo = CurrentDirectory(), bool windowsParsing = false) : FileSystemObject(new DirectoryImplementation(path, relativeTo, windowsParsing)) { }

    Directory subDirectory(const String& subDirectoryName) const
    {
        return Directory(subDirectoryName, *this);
    }
    File file(const String& fileName) const
    {
        return File(fileName, *this);
    }
};

class CurrentDirectory : public Directory
{
public:
    CurrentDirectory() : Directory(implementation()) { }

private:
    static Reference<FileSystemObjectImplementation> _implementation;
    static Reference<FileSystemObjectImplementation> implementation()
    {
        if (!_implementation.valid()) {
            static String obtainingCurrentDirectory("Obtaining current directory");
            _implementation = currentDirectory();
        }
        return _implementation;
    }

    static Reference<CurrentDirectoryImplementation> currentDirectory()
    {
#ifdef _WIN32
        int n = GetCurrentDirectory(0, NULL);
        if (n == 0)
            Exception::throwSystemError(obtainingCurrentDirectory);
        Array<WCHAR> buf(n);
        if (GetCurrentDirectory(n, &buf[0]) == 0)
            Exception::throwSystemError(obtainingCurrentDirectory);
        String path(&buf[0]);
        return FileSystemObject::parse(path, RootDirectory(), true)._implementation;
#else
        size_t size = 100;
        do {
            Array<char> buf(size);
            if (getcwd(&buf[0], size) != 0) {
                String path(&buf[0]);
                return FileSystemObject::parse(path, RootDirectory(), false)._implementation;
            }
            if (errno != ERANGE)
                Exception::throwSystemError(obtainingCurrentDirectory);
            size *= 2;
        } while (true);
#endif
    }

#ifdef _WIN32
    friend class DriveCurrentDirectory
#endif
};

#ifdef _WIN32
class DriveCurrentDirectory : public Directory
{
public:
    DriveCurrentDirectory(int drive) : FileSystemObject(implementation(drive)) { }
private:
    static Reference<FileSystemObjectImplementation> _implementations[26];
    static Reference<FileSystemObjectImplementation> implementation(int drive)
    {
        if (!_implementations[drive].valid()) {
            static String settingCurrentDirectory("Setting current directory");
            static String obtainingCurrentDirectory("Obtaining current directory");  // TODO: can this be shared with the copy in CurrentDirectoryImplementation?

            // Make sure the current directory has been retrieved
            CurrentDirectory();

            // Change to this drive
            Array<WCHAR> buf(3);
            buf[0] = drive + 'A';
            buf[1] = ':';
            buf[2] = 0;
            if (SetCurrentDirectory(&buf[0]) == 0)
                Exception::throwSystemError(settingCurrentDirectory);

            // Retrieve current directory
            _implementations[drive] = CurrentDirectory::currentDirectory();
        }
        return _implementations[drive];
    }
};
#endif

class RootDirectory : public Directory
{
public:
    RootDirectory() : Directory(implementation()) { }
private:
    static Reference<RootDirectoryImplementation> _implementation;
    static Reference<RootDirectoryImplementation> implementation()
    {
        if (!_implementation.valid())
            _implementation = new RootDirectoryImplementation();
        return _implementation;
    }
};

#ifdef _WIN32
class DriveRootDirectory : public Directory
{
public:
    DriveRootDirectory(int drive) : FileSystemObject(implementation(drive)) { }
private:
    static Reference<FileSystemObjectImplementation> _implementations[26];
    static Reference<FileSystemObjectImplementation> implementation(int drive)
    {
        if (!_implementations[drive].valid())
            _implementations[drive] = new DriveRootDirectoryImplementation(drive);
        return _implementations[drive];
    }
};

class UNCRootDirectory : public Directory
{
public:
    UNCRootDirectory(const String& server, const String& share) : FileSystemObject(new UNCRootDirectory(server, share)) { }
};
#endif


class File : public FileSystemObject
{
public:
    File(const String& path, const Directory& relativeTo = CurrentDirectory(), bool windowsParsing = false) : FileSystemObject(path, relativeTo, windowsParsing) { }

    String contents() const
    {
#ifdef _WIN32
        Array<WCHAR> data;
        String filePath = windowsPath();
        filePath.copyToUTF16(&data);
        HANDLE h = CreateFile(
           &data[0],
           GENERIC_READ,
           FILE_SHARE_READ,
           NULL,
           OPEN_EXISTING,
           FILE_FLAG_SEQUENTIAL_SCAN,
           NULL);
        if (h == INVALID_HANDLE_VALUE) {
            static String openingFile("Opening file ");
            Exception::throwSystemError(openingFile + filePath);
        }
        AutoHandle handle(h);
        LARGE_INTEGER size;
        if (GetFileSizeEx(handle, &size) == 0) {
            static String obtainingLengthOfFile("Obtaining length of file ");
            Exception::throwSystemError(obtainingLengthOfFile + filePath);
        }
        int n = size.LowPart;
        if (size.HighPart != 0 || n >= 0x80000000) {
            static String tooLargeFile("2Gb or more in file ");
            Exception::throwSystemError(tooLargeFile + filePath);
        }
        Reference<OwningBufferImplementation> bufferImplementation = new OwningBufferImplementation(filePath);
        bufferImplementation->allocate(n);
        DWORD numberOfBytesRead;
        if (ReadFile(handle, static_cast<LPVOID>(bufferImplementation->data()), n, &numberOfBytesRead, NULL) == 0 || numberOfBytesRead != n) {
            static String readingFile("Reading file ");
            Exception::throwSystemError(readingFile + filePath);
        }
        return String(Buffer(bufferImplementation), 0, n);
#else
        Array<UInt8> data;
        String filePath = path();
        filePath.copyTo(&data);
        int fileDescriptor = open(
            reinterpret_cast<const char*>(&data[0]),
            O_RDONLY);
        if (fileDescriptor == -1) {
            static String openingFile("Opening file ");
            Exception::throwSystemError(openingFile + filePath);
        }
        AutoHandle handle(fileDescriptor);
        off_t n = lseek(fileDescriptor, 0, SEEK_END);
        static String seekingFile("Seeking file ");
        if (n == (off_t)(-1))
            Exception::throwSystemError(seekingFile + filePath);
        if (n >= 0x80000000) {
            static String tooLargeFile("2Gb or more in file ");
            Exception::throwSystemError(tooLargeFile + filePath);
        }
        off_t r = lseek(fileDescriptor, 0, SEEK_SET);
        if (r == (off_t)(-1))
            Exception::throwSystemError(seekingFile + filePath);
        Reference<OwningBufferImplementation> bufferImplementation = new OwningBufferImplementation(name);
        bufferImplementation->allocate(n);
        ssize_t readResult = read(fileDescriptor, static_cast<void*>(bufferImplementation->data()), n);
        if (readResult < n) {
            static String readingFile("Reading file ");
            Exception::throwSystemError(readingFile + filePath);
        }
        return String(Buffer(bufferImplementation), 0, n);
#endif
    }
    void save(const String& contents)
    {
#ifdef _WIN32
        Array<WCHAR> data;
        String filePath = windowsPath();
        filePath.copyToUTF16(&data);
        HANDLE h = CreateFile(
            &data[0],
            GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        if (h == INVALID_HANDLE_VALUE) {
            static String openingFile("Opening file ");
            Exception::throwSystemError(openingFile + filePath);
        }
        AutoHandle handle(h);
        contents.write(handle);
#else
        Array<UInt8> data;
        String filePath = path();
        filePath.copyTo(&data);
        int fileDescriptor = open(
            reinterpret_cast<const char*>(&data[0]),
            O_WRONLY | O_CREAT | O_TRUNC);
        if (fileDescriptor == -1) {
            static String openingFile("Opening file ");
            Exception::throwSystemError(openingFile + filePath);
        }
        AutoHandle handle(fileDescriptor);
        contents.write(handle);
#endif
    }
    void secureSave(const String& contents)
    {
        // TODO: Backup file?
#ifdef _WIN32
        String filePath = windowsPath();
        Array<WCHAR> data;
        filePath.copyToUTF16(&data);
        Array<WCHAR> tempData;
        int i = 0;
        do {
            String tempPath = filePath + String::hexadecimal(i, 8);
            tempPath.copyToUTF16(&tempData);
            HANDLE h = CreateFile(
                &tempData[0],
                GENERIC_WRITE,
                0,
                NULL,
                CREATE_NEW,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
                NULL);
            if (h != INVALID_HANDLE_VALUE)
                break;
            if (GetLastError() != ERROR_FILE_EXISTS) {
                static String openingFile("Opening file ");
                Exception::throwSystemError(openingFile + tempPath);
            }
        } while (true);
        {
            AutoHandle handle(h);
            contents.write(handle);
        }
        if (ReplaceFile(&data[0], &tempData[0], NULL, REPLACEFILE_WRITE_THROUGH | REPLACEFILE_IGNORE_MERGE_ERRORS) == 0) {
            static String replacingFile("Replacing file ");
            Exception::throwSystemError(replacingFile + filePath);
        }
#else
        String filePath = path();
        Array<UInt8> data;
        filePath.copyTo(&data);
        Array<UInt8> tempData;
        int i = 0;
        do {
            String tempPath = filePath + String::hexadecimal(i, 8);
            tempPath.copyTo(tempData);
            int fileDescriptor = open(
                reinterpret_cast<const char*>(&data[0]),
                O_WRONLY | O_CREAT | O_EXCL);
            if (fileDescriptor != -1)
                break;
            if (errno != EEXIST) {
                static String openingFile("Opening file ");
                Exception::throwSystemError(openingFile + tempPath);
            }
        } while (true);
        {
            AutoHandle handle(fileDescriptor);
            contents.write(handle);
            if (fsync(handle) != 0) {
                static String synchronizingFile("Synchronizing file ");
                Exception::throwSystemError(synchronizingFile + filePath);
            }
        }
        if (rename(&tempData[0], &data[0]) != 0) {
            static String replacingFile("Replacing file ");
            Exception::throwSystemError(replacingFile + filePath);
        }
#endif
    }
    void append(const String& contents)
    {
#ifdef _WIN32
        Array<WCHAR> data;
        String filePath = windowsPath();
        filePath.copyToUTF16(&data);
        HANDLE h = CreateFile(
            &data[0],
            GENERIC_WRITE,
            0,
            NULL,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        if (h == INVALID_HANDLE_VALUE) {
            static String openingFile("Opening file ");
            Exception::throwSystemError(openingFile + filePath);
        }
        AutoHandle handle(h);
        contents.write(handle);
#else
        Array<UInt8> data;
        String filePath = path();
        filePath.copyTo(&data);
        int fileDescriptor = open(
            reinterpret_cast<const char*>(&data[0]),
            O_WRONLY | O_APPEND);
        if (fileDescriptor == -1) {
            static String openingFile("Opening file ");
            Exception::throwSystemError(openingFile + filePath);
        }
        AutoHandle handle(fileDescriptor);
        contents.write(handle);
#endif
    }
};


// Implementation classes

class FileSystemObjectImplementation : public ReferenceCounted
{
public:
    virtual Directory parent() const = 0;
    virtual String windowsPath() const = 0;
    virtual String path() const = 0;
    virtual bool isRoot() const = 0;
    virtual int hash(int h) = 0;
    virtual int compare(const FileSystemObjectImplementation* other) = 0;

    static FileSystemObject parse(const String& path, const Directory& relativeTo, bool windowsParsing)
    {
        if (path.empty()) {
            static String invalidPath("Invalid path");
            throw Exception(invalidPath);
        }

#ifdef _WIN32
        if (windowsParsing)
            return windowsParse(path, relativeTo);
#endif
        return parse(path, relativeTo);
    }

#ifdef _WIN32
    static FileSystemObject windowsParse(const String& path, const Directory& relativeTo)
    {
        static String invalidPath("Invalid path");
        static String currentDirectory(".");
        static String parentDirectory("..");
        static String empty;

        CharacterSource s = path.start();
        int c = s.get();
        int p = 1;
        int subDirectoryStart = 0;
        Directory dir = relativeTo;
        int last;

        // Process initial slashes
        if (c == '/' || c == '\\') {
            dir = RootDirectory();
            subDirectoryStart = p;
            if (s.empty())
                return dir;
            c = s.get();
            ++p;
            if (c == '/' || c == '\\') {
                int serverStart = p;
                if (s.empty())
                    throw Exception(invalidPath);
                do {
                    c = s.get();
                    ++p;
                    if (s.empty())
                        throw Exception(invalidPath);
                    // TODO: What characters are actually legal in server names?
                } while (c != '\\' && c != '/');
                String server = path.subString(serverStart, p - serverStart);
                int shareStart = p;
                do {
                    c = s.get();
                    ++p;
                    if (s.empty())
                        break;
                    // TODO: What characters are actually legal in share names?
                } while (c != '\\' && c != '/');
                String share = path.subString(shareStart, p - shareStart);
                dir = UNCRootDirectory(server, share);
                do {
                    subDirectoryStart = p;
                    if (s.empty())
                        return dir;
                    c = s.get();
                    ++p;
                } while (c == '/' || c == '\\');
            }
            // TODO: In paths starting \\?\, only \ and \\ are allowed separators, and ?*:"<> are allowed.
            // see http://docs.racket-lang.org/reference/windowspaths.html for more details
        }
        else {
            int drive = (c >= 'a' ? (c - 'a') : (c - 'A'));
            if (drive >= 0 && drive < 26) {
                if (s.empty())
                    return FileSystemObject(relativeTo, path.subString(0, 1);
                c = s.get();
                ++p;
                if (c == ':') {
                    subDirectoryStart = p;
                    dir = DriveCurrentDirectory(drive);
                    if (s.empty())
                        return dir;
                    c = s.get();
                    ++p;
                    if (c == '/' || c == '\\') {
                        dir = DriveRootDirectory(drive);
                        while (c == '/' || c == '\\') {
                            subDirectoryStart = p;
                            if (s.empty())
                                return dir;
                            c = s.get();
                            ++p;
                        }
                    }
                }
            }
        }

        do {
            while (c != '/' && c != '\\') {
                if (c < 32 || c == '?' || c == '*' || c == ':' || c == '"' || c == '<' || c == '>')
                    throw Exception(invalidPath);
                if (s.empty())
                    break;
                c = s.get();
                ++p;
            }
            String name = path.subString(subDirectoryStart, p - subDirectoryStart);
            if (name == currentDirectory)
                name = empty;
            if (name == parentDirectory) {
                dir = dir.parent();
                name = empty;
            }
            if (name != empty) {
                int l = name[name.length() - 1];
                if (l == '.' || l == ' ')
                    throw Exception(invalidPath);
            }
            if (s.empty())
                break;
            while (c == '/' || c == '\\') {
                subDirectoryStart = p;
                if (s.empty())
                    break;
                c = s.get();
                ++p;
            }
            if (s.empty())
                break;
            dir = dir.subDirectory(name);
        } while (true);
        if (name == empty) {
            if (dir.isRoot())
                return dir;
            return FileSystemObject(dir.parent(), dir.name());
        }
        return FileSystemObject(dir, name);
    }
#endif

    static FileSystemObject parse(const String& path, const Directory& relativeTo)
    {
        static String currentDirectory(".");
        static String parentDirectory("..");
        static String empty;

        CharacterSource s = path.start();
        int c = s.get();
        int p = 1;  // p always points to the character after c
        int subDirectoryStart = 0;
        Directory dir = relativeTo;

        // Process initial slashes
        if (c == '/') {
            dir = RootDirectory();
            while (c == '/') {
                subDirectoryStart = p;
                if (s.empty())
                    return dir;
                c = s.get();
                ++p;
            }
        }

        do {
            while (c != '/') {
                if (c == 0) {
                    static String invalidPath("Invalid path");
                    throw Exception(invalidPath);
                }
                if (s.empty())
                    break;
                c = s.get();
                ++p;
            }
            String name = path.subString(subDirectoryStart, p - subDirectoryStart);
            if (name == currentDirectory)
                name = empty;
            if (name == parentDirectory) {
                dir = dir.parent();
                name = empty;
            }
            if (s.empty())
                break;
            while (c == '/') {
                subDirectoryStart = p;
                if (s.empty())
                    break;
                c = s.get();
                ++p;
            }
            if (s.empty())
                break;
            dir = dir.subDirectory(name);
        } while (true);
        if (name == empty) {
            if (dir.isRoot())
                return dir;
            return FileSystemObject(dir.parent(), dir.name());
        }
        return FileSystemObject(dir, name);
    }
};

class NamedFileSystemObjectImplementation : public FileSystemObjectImplementation
{
public:
    NamedFileSystemObjectImplementation(const Directory& parent, const String& name) : _parent(parent), _name(name) { }

    String windowsPath() const
    {
        static String windowsPathSeparator("\\");
        return _parent.windowsPath() + windowsPathSeparator + _name;
    }

    String path() const
    {
        static String pathSeparator("/");
        return _parent.path() + pathSeparator + _name;
    }

    Directory parent() const { return _parent; }

    String name() const { return _name; }

    bool isRoot() const { return false; }

    int hash(int h) const { return _parent.hash(h) * 67 + _name.hash(); }

    int compare(const FileSystemObjectImplementation* other) const
    {
        const NamedFileSystemObjectImplementation* named = dynamic_cast<const NamedFileSystemObjectImplementation*>(other);
        if (named == 0)
            return 1;
        if (_parent != named->_parent)
            return 1;
        if (_name != named->_name)
            return 1;
        return 0;
    }

private:
    Directory _parent;
    String _name;
};

class RootDirectoryImplementation : public FileSystemObjectImplementation
{
public:
    RootDirectoryImplementation() { }

    Directory parent() const { return RootDirectory(); }
    String windowsPath() const
    {
        // TODO: Use \\?\ to avoid MAX_PATH limit?
        // If we do this we need to know the current drive - this is the first character of CurrentDirectory().windowsPath() .
        static String backslash("\\");
        return backslash;
    }
    String path() const
    {
        static String slash("/");
        return slash;
    }
    bool isRoot() const { return true; }

    int hash(int h) const { return 0; }

    int compare(const FileSystemObjectImplementation* other) const
    {
        const RootDirectoryImplementation* root = dynamic_cast<const RootDirectoryImplementation*>(other);
        if (root == 0)
            return 1;
        return 0;
    }
};

#ifdef _WIN32
class DriveRootDirectoryImplementation : public RootDirectoryImplementation
{
public:
    DriveRootDirectoryImplementation(int drive) { }

    Directory parent() const { return DriveRootDirectory(drive); }
    String windowsPath() const
    {
        // TODO: Use \\?\ to avoid MAX_PATH limit?
        static String system("System");
        Reference<OwningBufferImplementation> bufferImplementation = new OwningBufferImplementation(system);
        bufferImplementation->allocate(3);
        UInt8* p = bufferImplementation->data();
        p[0] = drive + 'A';
        p[1] = ':';
        p[2] = '\\';
        return String(Buffer(bufferImplementation), 0, 3);
    }

    int hash(int h) const { return _drive; }

    int compare(const FileSystemObjectImplementation* other) const
    {
        const DriveRootDirectoryImplementation* root = dynamic_cast<const DriveRootDirectoryImplementation*>(other);
        if (root == 0)
            return 1;
        if (_drive != root->_drive)
            return 1;
        return 0;
    }
private:
    int _drive;
};

class UNCRootDirectoryImplementation : public RootDirectoryImplementation
{
public:
    UNCRootDirectoryImplementation(const String& server, const String& share) : _server(server), _share(share) { }

    Directory parent() const { return UNCRootDirectory(_server); }
    String windowsPath() const
    {
        static String backslashBackslash("\\\\");
        static String backslash("\\");
        return backslashBackslash + _server + backslash + _share + backslash;
    }

    int hash(int h) const { return (h*67 + _server.hash())*67 + _share.hash(); }

    int compare(const FileSystemObjectImplementation* other) const
    {
        const UNCRootDirectoryImplementation* root = dynamic_cast<const UNCRootDirectoryImplementation*>(other);
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
#endif

#endif // INCLUDED_FILE_H
