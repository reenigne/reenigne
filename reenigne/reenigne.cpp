#include "alfe/main.h"
#include "alfe/config_file.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        ConfigFile configFile;
        configFile.addOption("inputProgram", StringType());
        configFile.addDefaultOption("loadAddress", 0x10000);
        configFile.addDefaultOption("targetArchitecture",
            String("x86.Generic8086"));

        String configPath = "default.config";
        if (_arguments.count() >= 2)
            configPath = _arguments[1];
        File configFilePath(configPath, true);
        configFile.load(configFilePath);

        String inputProgram = configFile.get<String>("inputProgram");
        File inputProgramPath(inputProgram, configFilePath.parent());


    }
};