#ifndef INCLUDED_FILE_H
#define INCLUDED_FILE_H

#include "string.h"
#include "platform.h"

/*
TODO:
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
    FileSystemObject(String path, const Directory& relativeTo, bool windowsParsing)
    {
        if (windowsParsing) {
            int c = path.front();
            if (c == '/' || c == '\\') {
                // Path is absolute
                path.popFront();



            if (tryParseWindowsAbsolutePath(path))
                return;
            if (



    WindowsSeparator := '/' | '\';
    RelativeWindowsPath := (WindowsPathPart WindowsSeparator)* WindowsPathPart [WindowsSeparator];
    AbsoluteWindowsPath := [DriveRoot | UNCRoot] WindowsSeparator RelativeWindowsPart;
    WindowsForbidden := '?' | '*' | ':' | '"' | '<' | '>' | 0-31;
    WindowsPathPart := (!WindowsForbidden)* (! (WindowsForbidden | " " | "."));   // Can be ".." or "."
    DriveRoot := 'A'..'Z' ':'
    UNCRoot := WindowsSeparator WindowsSeparator (WindowsPathPart | '?' | '.');


    }

     : Directory(parseDirectory(path, relativeTo, windowsParsing)), _name(parseName(path, windowsParsing)) { }
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

private:
    Directory parseDirectory(const String& path, const Directory& relativeTo, bool windowsParsing)
    {
        // TODO
    }

    String parseName(const String& path, bool windowsParsing)
    {
        // TODO
    }

    Directory _parent;
    String _name;
};

class FileImplementation : public FileSystemObjectImplementation
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

class DirectoryImplementation : public FileSystemObjectImplementation
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

class CurrentDirectoryImplementation : public DirectoryImplementation
{
public:
    CurrentDirectory() { }
};

#endif // INCLUDED_FILE_H
