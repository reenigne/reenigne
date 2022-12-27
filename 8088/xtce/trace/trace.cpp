#include "alfe/main.h"
#include "../xtce_microcode.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        if (_arguments.count() < 2) {
            console.write("Syntax: " + _arguments[0] +
                " <filename of program to run>\n");
            return;
        }
        Array<Byte> program;
        File(_arguments[1]).readIntoArray(&program);
        
        Word initialSegment = 0xa8;

        CPUEmulator emulator;
        emulator.reset();
        Word* registers = emulator.getRegisters();
        registers[0] = 0x0000;  // AX
        registers[1] = 0x00ff;  // CX
        registers[2] = initialSegment;  // DX
        registers[3] = 0x0000;  // BX
        registers[4] = 0xfffe;  // SP
        registers[5] = 0x0000;  // BP
        registers[6] = 0x0100;  // SI
        registers[7] = 0xfffe;  // DI
        Word* segmentRegisters = emulator.getSegmentRegisters();
        for (int i = 0; i < 4; ++i)
            segmentRegisters[i] = initialSegment;
        segmentRegisters[4] = 0x100; // PC
        Byte* ram = emulator.getRAM();
        for (int i = 0; i < 0xa0000; ++i)
            ram[i] = 0;
        int p = initialSegment << 4;
        Byte* psp = &ram[p];
        for (int i = 0; i < min(0xa0000 - (p + 0x100), program.count()); ++i)
            psp[0x100 + i] = program[i];
        psp[0] = 0xcd;
        psp[1] = 0x20;
        emulator.setExtents(
            -4,   // logStartCycle,
            INT_MAX,  // logEndCycle
            INT_MAX,  // executeEndCycle
            0,   // stopIP
            0    // stopSeg
        );
        emulator.run();
        console.write(emulator.log());
    }
};
