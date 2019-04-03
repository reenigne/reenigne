#include "alfe/main.h"
#include "alfe/config_file.h"

class StateVectorAllocator
{
public:
	StateVectorAllocator() : _nextAddress(0) { }
	int allocate(int bytes)
	{
		int r = _nextAddress;
		_nextAddress += bytes;
		return r;
	}
private:
	int _nextAddress;
};

class StateVector
{
public:
	StateVector(StateVectorAllocator allocator)
	{
		_data.allocate(allocator.allocate(0));
	}
	StateVector(const StateVector& other) { _data = other._data.copy();	}
	Byte* data(int address) { return &_data[address]; }
private:
	Array<Byte> _data;
};

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