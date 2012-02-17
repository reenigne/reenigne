#define _CRT_RAND_S
#include "unity/file.h"
#include "unity/com.h"
#include <stdio.h>
#include <stdlib.h>
#include <ShellAPI.h>
#include "unity/main.h"

class Collection
{
public:
    void append(const File& file)
    {
        _array.append(file);
    }
    void runRandom()
    {
        int n = _array.count();
        if (n == 0)
            throw Exception("No files to choose from");
        unsigned int r;
        if (rand_s(&r) != 0)
            throw Exception::systemError("Random number generation failed");
        r %= n;
        File f = _array[r];
        String p = f.path();
        HINSTANCE h = ShellExecute(NULL, NULL, NullTerminatedWideString(p),
            NULL, NULL, SW_SHOWMAXIMIZED);
        if (reinterpret_cast<unsigned int>(h) <= 32)
            throw Exception::systemError("Execution of " + p + " failed");
    }
private:
    AppendableArray<File> _array;
};

class Collect
{
public:
    Collect(Collection* collection) : _collection(collection) { }
    void operator()(const File& file)
    {
        _collection->append(file);
    }
    void operator()(const Directory& directory) { }
private:
    Collection* _collection;
};

class Program : public ProgramBase
{
public:
    void run()
    {
        COMInitializer com(COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        Collection collection;
        if (_arguments.count() == 1) {
            _console.write("Usage: run_random <path>\n");
            return;
        }
        for (int i = 1; i < _arguments.count(); ++i)
            applyToWildcard(Collect(&collection), _arguments[i]);
        collection.runRandom();
    }
};
