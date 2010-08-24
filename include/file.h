#ifndef INCLUDED_FILE_H
#define INCLUDED_FILE_H

#include "string.h"
#include "platform.h"

/*
TODO:
  What should we do with, e.g. "q:foo"
    Need to substitute in the current directory on the q: drive
    This only makes sense on the command line, not in a source file, since in a source file the "current directory" is the directory the source file is in.
      Directories in source files are in unix format, so 'include "q:foo"' means the file named q:foo in the same directory.

  FileSystemObject::directory
  FileSystemObject::name
      Reserved characters:
        Unix:
          null is end of string
          / is separator
        Windows:
          null is end of string
          / is separator
          \ is separator
          ?, *, :, |, ", <, > are disallowed
          . and space are disallowed at end
          1-31 are disallowed
    RelativeUnixPath := (UnixPathPart '/')* UnixPathPart ['/'];
    AbsoluteUnixPath := '/' RelativeUnixPath;
    UnixPathPart := (! '/')*;         // Can be ".." or "."

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
    FileSystemObject(const String& path, const Directory& relativeTo = CurrentDirectory(), bool windowsParsing = false) : _implementation(new FileSystemObjectImplementation(path, relativeTo, windowsParsing)) { }

    Directory parent() const { return _implementation->parent(); }
protected:
    FileSystemObject(Reference<FileSystemObjectImplementation> implementation) : _implementation(implementation) { }

    Reference<FileSystemObjectImplementation> _implementation;
};

class Directory : public FileSystemObject
{
public:
    Directory(const String& path, const Directory& relativeTo = CurrentDirectory(), bool windowsParsing = false) : FileSystemObject(new DirectoryImplementation(path, relativeTo, windowsParsing)) { }

private:
    Directory(Reference<DirectoryImplementation> implementation) : FileSystemObject(implementation) { }

    Reference<DirectoryImplementation> implementation() const { return _implementation; }

    friend class DirectoryImplementation;
};

class CurrentDirectory : public FileSystemObject
{
public:
    CurrentDirectory() : FileSystemObject(implementation()) { }

private:
    static Reference<CurrentDirectoryImplementation> _implementation;
    static Reference<CurrentDirectoryImplementation> implementation()
    {
        if (!_implementation.valid())
            _implementation = new CurrentDirectoryImplementation();
        return _implementation;
    }
};

class RootDirectory : public FileSystemObject
{
public:
    RootDirectory() : FileSystemObject(implementation()) { }
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
    File(const String& path, const Directory& relativeTo = CurrentDirectory(), bool windowsParsing = false) : FileSystemObject(new FileImplementation(path, relativeTo, windowsParsing)) { }

private:
    Reference<FileImplementation> implementation() const { return _implementation; }
};

class FileSystemObjectImplementation : public ReferenceCounted
{
public:
    virtual String windowsPath() = 0;
    virtual String path() = 0;
};

class NormalFileSystemObjectImplementation : public FileSystemObjectImplementation
{
public:
    NormalFileSystemObjectImplementation(const String& path, const Directory& relativeTo, bool windowsParsing)
    {
        init(path, relativeTo, windowsParsing);
    }

    Directory parent() const { return _parent; }

    String windowsPath()
    {
        static String windowsPathSeparator("\\");
        return _parent.windowsPath() + windowsPathSeparator + _name;
    }

    String path()
    {
        static String pathSeparator("/");
        return _parent.path() + pathSeparator + _name;
    }

protected:
    void init(const String& path, const Directory& relativeTo, bool windowsParsing)
    {
        _parent = relativeTo;
        if (windowsParsing)
            windowsParse(path);
        else
            unixParse(path);
    }

private:
    void windowsParse(String path)
    {
        CharacterSource s = path.start();
//    WindowsSeparator := '/' | '\';
//    RelativeWindowsPath := (WindowsPathPart WindowsSeparator)* WindowsPathPart [WindowsSeparator];
//    AbsoluteWindowsPath := [DriveRoot | UNCRoot] WindowsSeparator RelativeWindowsPart;
//    WindowsForbidden := '?' | '*' | ':' | '"' | '<' | '>' | 0-31;
//    WindowsPathPart := (!WindowsForbidden)* (! (WindowsForbidden | " " | "."));   // Can be ".." or "."
//    DriveRoot := 'A'..'Z' ':'
//    UNCRoot := WindowsSeparator WindowsSeparator (WindowsPathPart | '?' | '.');

        int state = 0;
        int subDirectoryStart = 0;
        int p = 0;
        int c0, c1;
        do {
            switch (state) {
                case 0:  // Start of path
                    c0 = s.get();
                    ++p;
                    if (c0 == '/' || c0 == '\\') {
                        state = 1;
                        break;
                    }
                    if ((c0 >= 'a' && c0 <= 'z') || (c0 >= 'A' && c0 <= 'Z')) {
                        state = 2;
                        break;
                    }
                    state = 3;
                    break;
                case 1:  // After initial slash or backslash
                    c0 = s.get();
                    ++p;
                    if (c0 == '/' || c0 == '\\') {
                        state = 4;
                        break;
                    }
                    _parent = RootDirectory();
                    subDirectoryStart = 1;
                    state = 3;
                    break;
                case 2:  // After initial possible drive letter
                    c1 = s.get();
                    ++p;
                    if (c1 == ':')
                        _parent = DriveRootDirectory(c0);






    }
    void unixParse(CharacterSource s)
    {
    }

    Directory _parent;
    String _name;
};

class FileImplementation : public NormalFileSystemObjectImplementation
{
public:
    File(const String& path, const Directory& relativeTo) : FileSystemObjectImplementation(path, relativeTo) { }
    String contents() const
    {
#ifdef _WIN32
        Array<UInt8> data;
        windowsName.copyToUTF16(&data);
        CreateFile(
           reinterpret_cast<LPCWSTR>(&data[0]),
           GENERIC_READ,
           FILE_SHARE_READ,
           NULL,
           OPEN_EXISTING,
           FILE_FLAG_SEQUENTIAL_SCAN,
           NULL);
        // TODO
#else
        // TODO
#endif
    }
    void save(const String& contents)
    {
        // TODO: How do we do this reliably? (Particularly important for the editor case)
    }
};

class DirectoryImplementation : public NormalFileSystemObjectImplementation
{
public:
    Directory(const String& path, const Directory& relativeTo) : FileSystemObject(path, relativeTo) { }
    Directory subDirectory(const String& subDirectoryName) const
    {
        return Directory(subDirectoryName, Directory(this));
    }
    File file(const String& fileName) const
    {
        return File(fileName, Directory(this));
    }
};

class CurrentDirectoryImplementation : public NormalFileSystemObjectImplementation
{
public:
    CurrentDirectoryImplementation()
    {
#ifdef _WIN32
        static String obtainingCurrentDirectory("Obtaining current directory");
        int n = GetCurrentDirectory(0, NULL);
        if (n == 0)
            throwWindowsError(obtainingCurrentDirectory);
        Array<WCHAR> buf(n);
        if (GetCurrentDirectory(n, &buf[0]) == 0)
            throwWindowsError(obtainingCurrentDirectory);
        String path(buf);
        init(path, RootDirectory(), true);
#else
        // TODO: use getcwd()
#endif
    }
};

class RootDirectoryImplementation : public FileSystemObjectImplementation
{
public:
    RootDirectory() { }
};

#endif // INCLUDED_FILE_H
