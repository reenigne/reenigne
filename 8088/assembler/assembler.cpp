#include "alfe/main.h"
#include "alfe/config_file.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        if (_arguments.count() < 2) {
            console.write("Syntax: assembler <input file name>\n");
            return;
        }
        String configPath = _arguments[1];
        File file(configPath, true);
        Directory parent = file.parent();

        ConfigFile configFile;
        configFile.load(file);
    }
};