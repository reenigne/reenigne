#include "alfe/main.h"
#include "alfe/config_file.h"
#include "xtce.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        if (_arguments.count() < 2) {
            console.write("Syntax: " + _arguments[0] +
                " <input file name> [log start cycle] [log end cycle]"
                " [execute end cycle]\n");
            return;
        }
        File inputFile = File(_arguments[1], true);
        Array<Byte> data;
        inputFile.readIntoArray(&data);

        ConfigFile config;

        int logStartCycle = 0;
        if (_arguments.count() > 2) {
            logStartCycle = config.evaluate<int>(_arguments[2],
                logStartCycle);
        }
        int logEndCycle = 4096;
        if (_arguments.count() > 3)
            logEndCycle = config.evaluate<int>(_arguments[3], logEndCycle);
        int executeEndCycle = logEndCycle;
        if (_arguments.count() > 4) {
            executeEndCycle = config.evaluate<int>(_arguments[4],
                executeEndCycle);
        }

        CPUEmulator emulator;
        emulator.reset();
        Byte* ram = emulator.getRAM();
        Word* segmentRegisters = emulator.getSegmentRegisters();

        Word segment = 0x70;
        for (int i = 0; i < 4; ++i)
            segmentRegisters[i] = segment - 0x10;
        Byte* ram1 = ram + (segment << 4);
        Word* ram2 = reinterpret_cast<Word*>(ram1 - 0x100);
        ram2[0] = 0xff;
        ram2[1] = segment;

        int length = min(data.count(), (0x10000 - segment) << 4);
        for (int i = 0; i < length; ++i)
            ram1[i] = data[i];

        emulator.setExtents(logStartCycle, logEndCycle, executeEndCycle, 0xff,
            segment - 0x10, -1, -1);
        emulator.setConsoleLogging();
        emulator.setInitialIP(0x100);
        emulator.run();
        console.write(emulator.log());
        console.write(String(decimal(emulator.cycle())) + "\n");
    }
};
