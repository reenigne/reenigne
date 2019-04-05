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
    StateVector() { }
	StateVector(StateVectorAllocator allocator)
	{
		_data.allocate(allocator.allocate(0));
	}
	StateVector(const StateVector& other) { _data = other._data.copy();	}
	Byte* data(int address) { return &_data[address]; }
private:
	Array<Byte> _data;
};

class Decompiler
{
public:
    Decompiler(ConfigFile* configFile) : _configFile(configFile)
    {
        StateVectorAllocator allocator;
        _registers = allocator.allocate(8*2 + 4*2 + 2);
        _ram = allocator.allocate(640*1024);
        _v = StateVector(allocator);
    }
    Byte* ram() { }
private:
    StateVector _v;
    int _registers;
    int _ram;
    ConfigFile* _configFile;
};

class DOSLoader
{
public:
    DOSLoader(ConfigFile* configFile) : _configFile(configFile) { }
    void load(File file, Decompiler* decompiler)
    {
        int loadAddress = _configFile->get<int>("loadAddress");
        int imageSegment = loadAddress >> 4;

        file.readIntoArray(&_program);
        int length = _program.count();
        if (length >= 2 && readWord(0) == 0x5a4d) {  // .exe file?
            if (length < 0x21) {
                throw Exception(file.path() +
                    " is too short to be an .exe file\n");
            }
            Word bytesInLastBlock = readWord(2);
            int exeLength = ((readWord(4) - (bytesInLastBlock == 0 ? 0 : 1))
                << 9) + bytesInLastBlock;
            int headerParagraphs = readWord(8);
            int headerLength = headerParagraphs << 4;
            if (exeLength > length || headerLength > length ||
                headerLength > exeLength)
                throw Exception(file.path() + " is corrupt\n");
            int relocationCount = readWord(6);

            //Word imageSegment = loadSegment + headerParagraphs;
            int relocationData = readWord(0x18);
            for (int i = 0; i < relocationCount; ++i) {
                Word offset = readWord(relocationData);
                Word segment = readWord(relocationData + 2) + imageSegment;
                writeWord(readWord(offset, 1) + imageSegment, offset, 1);
                relocationData += 4;
            }
            loadSegment = imageSegment;  // Prevent further access to header
            Word ss = readWord(0xe) + loadSegment;  // SS
            setSS(ss);
            setSP(readWord(0x10));
            stackLow = ((exeLength - headerLength + 15) >> 4) + loadSegment;
            if (stackLow < ss)
                stackLow = 0;
            else
                stackLow = (stackLow - (int)ss) << 4;
            ip = readWord(0x14);
            setCS(readWord(0x16) + loadSegment);  // CS
        }
        else {
            if (length > 0xff00) {
                throw Exception(file.path() +
                    " is too long to be a .com file\n");
            }
            setSP(0xFFFE);
            stackLow = length + 0x100;
        }


    }
private:
    ConfigFile* _configFile;
    Array<Byte> _program;

    Byte readByte(int offset) { return _program[offset]; }
    Word readWord(int offset)
    {
        Word r = readByte(offset);
        return r + (readByte(offset + 1) << 8);
    }

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

        Decompiler decompiler(&configFile);

        String inputProgram = configFile.get<String>("inputProgram");
        File inputProgramFile(inputProgram, configFilePath.parent());

        DOSLoader loader(&configFile);
        loader.load(inputProgramFile, &decompiler);

    }
private:


};