#include "alfe/main.h"
#include "alfe/config_file.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        String configPath;
        if (_arguments.count() < 2)
            configPath = "harness.config";
        else
            configPath = _arguments[1];

        ConfigFile configFile;
        configFile.addOption("tests", StringType());
        configFile.load(File(configPath, true));

        String tests = configFile.get<String>("tests");


        console.write("FAIL\n");
    }
};