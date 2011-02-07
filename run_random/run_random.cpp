#include "unity/file.h"
#include <stdio.h>
#include <stdlib.h>

class Collect
{
public:
    void operator()(const File& file)
    {
    }
    void operator()(const Directory& directory) { }
};

int wmain(int argc, wchar_t** argv)
{
    BEGIN_CHECKED {
        Collect collect;
        if (argc == 1) {
            printf("Usage: run_random <path>\n");
            exit(1);
        }
        argv++;
        for (;*argv; ++argv)
            applyToWildcard(collect, String(*argv));
    }
    END_CHECKED(Exception& e) {
        e.write(Handle::consoleOutput());
    }
    return 0;
}