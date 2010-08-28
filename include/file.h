#ifndef INCLUDED_FILE_H
#define INCLUDED_FILE_H

#include "string.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

/*
TODO:
  We have two conflicting inheritance hierarchies:
    FileSystemObject
      File
      Directory
        RootDirectory

    FileSystemObject
      NamedFileSystemObject
        File
        Directory
      RootDirectory

  We want the first one for the "outer" FileSystemObject/Directory/File classes
  What about the "inner" Implementation classes?


  FileSystemObjectImplementation::windowsParse
  FileSystemObjectImplementation::parse
  File::contents
  File::save

  Want to be able to compare FileSystemObjects to see if they correspond to the same file?
    Do need this for "include" idempotency
    Don't need to worry about UNC paths, hard-links etc. but we should deal with relative paths correctly
      Usual case will be the filename is specified relative to the source file which did the "include".
      Need == and hash?
        On Windows, paths are generally not case sensitive, on unix they generally are, but not always
          => compare as case sensitive for this purpose
*/

extern String pathDelimiter;

class FileSystemObject
{
public:
    FileSystemObject(const String& path, const Directory& relativeTo = CurrentDirectory(), bool windowsParsing = false)
    {
        *this = NamedFileSystemObject::parse(path, relativeTo, windowsParsing));
    }

    Directory parent() const { return _implementation->parent(); }
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
    static Reference<CurrentDirectoryImplementation> _implementation;
    static Reference<CurrentDirectoryImplementation> implementation()
    {
        if (!_implementation.valid())
            _implementation = new CurrentDirectoryImplementation();
        return _implementation;
    }
};

#ifdef _WIN32
class DriveCurrentDirectory : public Directory
{
public:
    DriveCurrentDirectory(int drive) : FileSystemObject(implementation(drive)) { }
private:
    static Reference<DriveCurrentDirectoryImplementation> _implementations[26];
    static Reference<DriveCurrentDirectoryImplementation> implementation(int drive)
    {
        if (!_implementations[drive].valid())
            _implementations[drive] = new DriveCurrentDirectoryImplementation(drive);
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
}

class File : public FileSystemObject
{
public:
    File(const String& path, const Directory& relativeTo = CurrentDirectory(), bool windowsParsing = false) : FileSystemObject(path, relativeTo, windowsParsing) { }

    String contents() const
    {
#ifdef _WIN32
        Array<UInt8> data;
        windowsName().copyToUTF16(&data);
//        CreateFile(
//           reinterpret_cast<LPCWSTR>(&data[0]),
//           GENERIC_READ,
//           FILE_SHARE_READ,
//           NULL,
//           OPEN_EXISTING,
//           FILE_FLAG_SEQUENTIAL_SCAN,
//           NULL);
//        // TODO
//#else
//        // TODO
//#endif
//    }
//    void save(const String& contents)
//    {
//        // TODO: How do we do this reliably? (Particularly important for the editor case)
//    }

private:
};


// Implementation classes

class FileSystemObjectImplementation : public ReferenceCounted
{
public:
    virtual Directory parent() const = 0;
    virtual String windowsPath() const = 0;
    virtual String path() const = 0;
    virtual bool isRoot() const = 0;
};

class NamedFileSystemObjectImplementation : public FileSystemObjectImplementation
{
public:
    NamedFileSystemObjectImplementation(const Directory& parent, const String& name) : _parent(parent), _name(name) { }

    static FileSystemObject parse(const String& path, const Directory& relativeTo, bool windowsParsing)
    {
#ifdef _WIN32
        if (windowsParsing)
            return windowsParse(path, relativeTo);
#endif
        return parse(path, relativeTo);
    }

#ifdef _WIN32
    static FileSystemObject windowsParse(const String& path, const Directory& relativeTo)
    {
        // TODO
////          null is end of string
////          / is separator
////          \ is separator
////          ?, *, :, |, ", <, > are disallowed
////          . and space are disallowed at end
////          1-31 are disallowed
////    WindowsSeparator := '/' | '\';
////    RelativeWindowsPath := (WindowsPathPart WindowsSeparator)* WindowsPathPart [WindowsSeparator];
////    AbsoluteWindowsPath := [DriveRoot | UNCRoot] WindowsSeparator RelativeWindowsPart;
////    WindowsForbidden := '?' | '*' | ':' | '"' | '<' | '>' | 0-31;
////    WindowsPathPart := (!WindowsForbidden)* (! (WindowsForbidden | " " | "."));   // Can be ".." or "."
////    DriveRoot := 'A'..'Z' ':'
////    UNCRoot := WindowsSeparator WindowsSeparator (WindowsPathPart | '?' | '.');
//
//        CharacterSource s = path.start();
//        int state = 0;
//        int subDirectoryStart = 0;
//        int p = 0;
//        int c0, c1;
//        do {
//            switch (state) {
//                case 0:  // Start of path
//                    c0 = s.get();
//                    ++p;
//                    if (c0 == '/' || c0 == '\\') {
//                        state = 1;
//                        break;
//                    }
//                    if ((c0 >= 'a' && c0 <= 'z') || (c0 >= 'A' && c0 <= 'Z')) {
//                        state = 2;
//                        break;
//                    }
//                    state = 3;
//                    break;
//                case 1:  // After initial slash or backslash
//                    c0 = s.get();
//                    ++p;
//                    if (c0 == '/' || c0 == '\\') {
//                        state = 4;
//                        break;
//                    }
//                    _parent = RootDirectory();
//                    subDirectoryStart = 1;
//                    state = 3;
//                    break;
//                case 2:  // After initial possible drive letter
//                    c1 = s.get();
//                    ++p;
//                    if (c1 == ':')
//                        _parent = DriveRootDirectory(c0);
    }
#endif

    static FileSystemObject parse(const String& path, const Directory& relativeTo)
    {
        static String currentDirectory(".");
        static String parentDirectory("..");
        static String empty;

        CharacterSource s = path.start();
        if (s.empty()) {
            static String invalidPath("Invalid path");
            throw Exception(invalidPath);
        }

        int c = s.get();
        int p = 1;  // p always points to the character after c
        int subDirectoryStart = 0;
        Directory dir = relativeTo;

        // Process initial slashes
        while (c == '/') {
            subDirectoryStart = p;
            dir = RootDirectory();
            if (s.empty())
                return dir;
            c = s.get();
            ++p;
        }

        do {
            while (c != '/') {
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
            }
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
private:
    Directory _parent;
    String _name;
};

//class CurrentDirectoryImplementation : public DirectoryImplementation
//{
//public:
//    CurrentDirectoryImplementation()
//    {
//        static String obtainingCurrentDirectory("Obtaining current directory");
//#ifdef _WIN32
//        int n = GetCurrentDirectory(0, NULL);
//        if (n == 0)
//            throwSystemError(obtainingCurrentDirectory);
//        Array<WCHAR> buf(n);
//        if (GetCurrentDirectory(n, &buf[0]) == 0)
//            throwSystemError(obtainingCurrentDirectory);
//        String path(buf);
//        init(path, RootDirectory(), true);
//#else
//        size_t size = 100;
//        do {
//            Array<char> buf(size);
//            if (getcwd(&buf[0], size) != 0) {
//                String path(buf);
//                init(path, RootDirectory(), false);
//                return;
//            }
//            if (errno != ERANGE)
//                throwSystemError(obtainingCurrentDirectory);
//            size *= 2;
//        } while (true);
//#endif
//    }
//};
//
//#ifdef _WIN32
//class DriveCurrentDirectoryImplementation : public DirectoryImplementation
//{
//public:
//    DriveCurrentDirectoryImplementation(int drive)
//    {
//        static String settingCurrentDirectory("Setting current directory");
//        static String obtainingCurrentDirectory("Obtaining current directory");  // TODO: can this be shared with the copy in CurrentDirectoryImplementation?
//
//        // Make sure the current directory has been retrieved
//        CurrentDirectory();
//
//        // Change to this drive
//        Array<WCHAR> buf(3);
//        buf[0] = drive + 'A';
//        buf[1] = ':';
//        buf[2] = 0;
//        if (SetCurrentDirectory(&buf[0]) ==0)
//            throwSystemError(settingCurrentDirectory);
//
//        // Retrieve current directory
//        int n = GetCurrentDirectory(0, NULL);
//        if (n == 0)
//            throwSystemError(obtainingCurrentDirectory);
//        Array<WCHAR> buf(n);
//        if (GetCurrentDirectory(n, &buf[0]) == 0)
//            throwSystemError(obtainingCurrentDirectory);
//        String path(buf);
//        init(path, RootDirectory(), true);
//    }
//};
//#endif

class RootDirectoryImplementation : public FileSystemObjectImplementation
{
public:
    RootDirectory() { }

    Directory parent() const { return RootDirectory(); }
    String windowsPath() const
    {
        static String backslash("\\");
        return backslash;
    }
    String path() const
    {
        static String slash("/");
        return slash;
    }
    bool isRoot() const { return true; }
};

#endif // INCLUDED_FILE_H
