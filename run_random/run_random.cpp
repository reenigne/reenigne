#define _CRT_RAND_S
#include "unity/file.h"
#include "unity/com.h"
#include <stdio.h>
#include <stdlib.h>
#include <ShellAPI.h>

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
            throw Exception(String("No files to choose from"));
        unsigned int r;
        if (rand_s(&r) != 0)
            Exception::throwSystemError(String("Random number generation failed"));
        r %= n;
        File f = _array[r];
        String p = f.path();
        Array<WCHAR> path;
        p.copyToUTF16(&path);
        HINSTANCE h = ShellExecute(NULL, NULL, &path[0], NULL, NULL, SW_SHOWMAXIMIZED);
        if (reinterpret_cast<unsigned int>(h) <= 32)
            Exception::throwSystemError(String("Execution of ") + p + (" failed"));
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

int wmain(int argc, wchar_t** argv)
{
    BEGIN_CHECKED {
        COMInitializer com(COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        Collection collection;
        if (argc == 1) {
            printf("Usage: run_random <path>\n");
            exit(1);
        }
        argv++;
        for (;*argv; ++argv)
            applyToWildcard(Collect(&collection), String(*argv));
        collection.runRandom();
    }
    END_CHECKED(Exception& e) {
        e.write(Handle::consoleOutput());
    }
    return 0;
}