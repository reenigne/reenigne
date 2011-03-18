#include "unity/string.h"
#include "unity/array.h"
#include "unity/file.h"
#include "unity/stack.h"
#include "unity/hash_table.h"
#include "unity/character_source.h"
#include "unity/command_line.h"

#include <stdlib.h>

class SourceProgram
{
};

class Simulator;

typedef void (Simulator::*opcodeFunction)();

class Simulator
{
public:
    Simulator()
    {
        _biu.setSimulator(this);
        _eu.setSimulator(this);
    }
    void simulateCycle()
    {
        _biu.simulateCycle();
        _eu.simulateCycle();
    }
    class ExecutionUnit;
    class BusInterfaceUnit
    {
    public:
        BusInterfaceUnit()
          : _ip(0xfff0),
            _prefetchOffset(0),
            _prefetched(0),
            _segment(0),  // ?
            _readRequested(false),
            _writeRequested(false)
        {
            _segmentRegisters[0] = 0x0000;  // ?
            _segmentRegisters[1] = 0xf000;
            _segmentRegisters[2] = 0x0000;  // ?
            _segmentRegisters[3] = 0x0000;  // ?
            _memory.allocate(0x100000);
        }
        void setSimulator(Simulator* simulator)
        { 
            _simulator = simulator;
            _eu = &_simulator->_eu;
        }
        void simulateCycle()
        {
            switch (_tState) {
                case 0:  // T1
                    switch (_pendingType) {
                        case 1:
                            _data = *physicalAddress();
                            _eu->readComplete();
                            break;
                        case 2:
                            *physicalAddress() = _data;
                            _eu->writeComplete();
                            break;
                        case 3:
                            _prefetchQueue[(_prefetchOffset + _prefetched) & 3] = *physicalAddress();
                            ++_prefetched;
                            break;
                    }
                    _pendingType = 0;
                    if (_readRequested) {
                        _readRequested = false;
                        _pendingType = 1;
                    }
                    else
                        if (_writeRequested) {
                            _writeRequested = false;
                            _pendingType = 2;
                        }
                        else
                            if (_prefetched < 4)
                                _pendingType = 3;
                    if (_pendingType != 0)
                        _tState = 1;
                    break;
                case 1:  // T2
                    _tState = 2;
                    break;
                case 2:  // T3
                    _tState = 3;  // 4 for Twait
                    break;
                case 3:  // T4
                    _tState = 0;
                    break;
                case 4:  // Twait
                    _tState = 3;
                    break;
            }
        }
        UInt8* physicalAddress()
        {
            return &_memory[((_segmentRegisters[_segment] << 4) + _address) & 0xfffff];
        }
        UInt8 getInstructionByte()
        {
            UInt8 byte = _prefetchQueue[_prefetchOffset & 3];
            _prefetchOffset = (_prefetchOffset + 1) & 3;
            --_prefetched;
            return byte;
        }
        bool instructionByteAvailable() const { return _prefetched > 1; }
    private:
        UInt16 _segmentRegisters[4];
        UInt16 _ip;
        UInt8 _prefetchQueue[4];
        UInt8 _prefetchOffset;
        UInt8 _prefetched;
        int _segment;
        bool _readRequested;
        bool _writeRequested;
        int _tState;
        UInt8 _data;
        Simulator* _simulator;
        ExecutionUnit* _eu;
        UInt16 _address;
        Array<UInt8> _memory;
        int _pendingType;
        UInt16 _prefetchAddress;
    };
    class ExecutionUnit
    {
    public:
        ExecutionUnit()
          : _flags(0x0002)  // ?
        {
            for (int i = 0; i < 8; ++i)
                _registers[i] = 0;  // ?
        }
        void setSimulator(Simulator* simulator) { _simulator = simulator; }
        void simulateCycle()
        {
            if (_wait > 0) {
                --_wait;
                return;
            }
            switch (_state) {
                case 0:


            }
        }
        void readComplete()
        {
        }
        void writeComplete()
        {
        }
        void gotInstuctionByte()
        {
        }
    private:
        void o00() { /* TODO: alu modrm */ _useModRM = true; }
        void o04() { /* TODO: alu accum, imm */ }
        void o06() { /* TODO: PUSH segreg */ }
        void o07() { /* TODO: POP  segreg */ }
        void o26() { /* TODO: segment override */ }
        void o27() { /* TODO: DAA */ }
        void o2f() { /* TODO: DAS */ }
        void o37() { /* TODO: AAA */ }
        void o3f() { /* TODO: AAS */ }
        void o40() { if ((_opcode & 0x08) == 0) ++rw(); else --rw(); _wait = 3; /* TODO: flags */ }
        void o50() { /* PUSH rw */ push(rw()); _wait = 15; }
        void o58() { /* TODO: POP  rw */ }
        void o60() { /* TODO: invalid */ }
        void o70() { /* TODO: Jcond cb */ }
        void o80() { /* TODO: alu regmem, imm */ }
        void o84() { /* TODO: TEST rm,r */ }
        void o86() { /* TODO: XCHG rm,r */ }
        void o88() { /* TODO: MOV  modrm */ }
        void o8c() { /* TODO: MOV  segreg */ }
        void o8d() { /* TODO: LEA  rw,m */ }
        void o8f() { /* TODO: POP  mw */ } 
        void o90() { /* XCHG AX,rw */ UInt16 t = rw(); rw() = ax(); ax() = t; _wait = 3; }
        void o98() { /* CBW */ ah() = (al() >= 8 ? 0xff : 0x00); _wait = 2; }
        void o99() { /* CWD */ dx() = (ax() >= 0x8000 ? 0xffff : 0x0000); _wait = 5; }
        void o9A() { /* TODO: CALL cp */ }
        void o9B() { /* TODO: WAIT */ _wait = 4; }
        void o9C() { /* PUSHF */ push(_flags & 0x0fd7); _wait = 14; } 
        void o9D() { /* TODO: POPF */ }
        void o9E() { /* SAHF */ _flags = (_flags & 0xff02) | ah(); _wait = 4; }
        void o9F() { /* LAHF */ ah() = _flags & 0xd7; _wait = 4; }
        void oA0() { /* TODO: MOV  accum<->[imm] */ }
        void oA4() { /* TODO: MOVS */ }
        void oA6() { /* TODO: CMPS */ }
        void oA8() { /* TODO: TEST accum,imm */ }
        void oAA() { /* TODO: STOS */ }
        void oAC() { /* TODO: LODS */ }
        void oAE() { /* TODO: SCAS */ }
        void oB0() { /* TODO: MOV reg,imm */ _wait = 4; }
        void oC0() { /* TODO: invalid */ }
        void oC2() { /* TODO: RET/RETF */ }
        void oC4() { /* TODO: Lsegreg rw,m */ }
        void oC6() { /* TODO: MOV rm, imm */ }
        void oCC() { /* TODO: INT 3 */ }
        void oCD() { /* TODO: INT ib */ }
        void oCE() { /* TODO: INTO */ }
        void oCF() { /* TODO: IRET */ }
        void oD0() { /* TODO: shift */ }
        void oD4() { /* TODO: AAM ib */ }
        void oD5() { /* TODO: AAD ib */ }
        void oD6() { /* SALC */ al() = carry() ? 0xff : 0x00; _wait = 4; }
        void oD7() { /* TODO: XLATB */ }
        void oD8() { /* TODO: ESC */ _wait = 2; }
        void oE0() { /* TODO: loop cb */ }
        void oE4() { /* TODO: IN/OUT */ }
        void oE8() { /* TODO: CALL cw */ }
        void oE9() { /* TODO: JMP cw */ }
        void oEA() { /* TODO: JMP cp */ }
        void oEB() { /* TODO: JMP cb */ }
        void oF0() { /* TODO: LOCK */ _wait = 2; }
        void oF1() { /* TODO: invalid */ }
        void oF2() { /* TODO: REPNE/REP */ _wait = 2; }
        void oF4() { /* TODO: HLT */ _wait = 2; }
        void oF5() { /* CMC */ _flags ^= 1; _wait = 2; }
        void oF6() { /* TODO: misc1 */ }
        void oF8() { /* CLC/STC */ _flags = (_flags & 0xfffe) | (_opcode & 1); _wait = 2; }
        void oFA() { /* CLI/STI */ _flags = (_flags & 0xfdff) | ((_opcode & 1) << 9); _wait = 2; }
        void oFC() { /* CLD/STD */ _flags = (_flags & 0xfbff) | ((_opcode & 1) << 10); _wait = 2; }
        void oFE() { /* TODO: misc2 */ }

        UInt16& rw() { return _registers[_opcode & 7]; }
        UInt16& ax() { return rw(0); }
        UInt16& dx() { return rw(2); }
        UInt8& rb() { return byteRegister(_opcode & 7); }
        UInt8& al() { return rb(0); }
        UInt8& ah() { return rb(4); }
        bool carry() { return (_flags & 1) != 0; }

        void push(UInt16 value) { /* TODO */ }

        UInt8& byteRegister(int n)
        {
            return *(reinterpret_cast<UInt8*>(&_registers[n & 3]) + (n >= 4 ? 1 : 0));
        }
            

        UInt16 _registers[8];  /* AX CX DX BX SP BP SI DI */
        UInt16 _flags;
        Simulator* _simulator;
        int _wait;
        int _state;
        UInt8 _opcode;
        UInt8 _modrm;
        bool _useModRM;

        static opcodeFunction _opcodeTable[256];
    };

private:
    BusInterfaceUnit _biu;
    ExecutionUnit _eu;

    friend class BusInterfaceUnit;
    friend class ExecutionUnit;
};

typedef Simulator::ExecutionUnit EU;

opcodeFunction EU::_opcodeTable[256] = {
    EU::o00, EU::o00, EU::o00, EU::o00, EU::o04, EU::o04, EU::o06, EU::o07,
    EU::o00, EU::o00, EU::o00, EU::o00, EU::o04, EU::o04, EU::o06, EU::o07,
    EU::o00, EU::o00, EU::o00, EU::o00, EU::o04, EU::o04, EU::o06, EU::o07,
    EU::o00, EU::o00, EU::o00, EU::o00, EU::o04, EU::o04, EU::o06, EU::o07,
    EU::o00, EU::o00, EU::o00, EU::o00, EU::o04, EU::o04, EU::o26, EU::o27,
    EU::o00, EU::o00, EU::o00, EU::o00, EU::o04, EU::o04, EU::o26, EU::o2F,
    EU::o00, EU::o00, EU::o00, EU::o00, EU::o04, EU::o04, EU::o26, EU::o37,
    EU::o00, EU::o00, EU::o00, EU::o00, EU::o04, EU::o04, EU::o26, EU::o3F,
    EU::o40, EU::o40, EU::o40, EU::o40, EU::o40, EU::o40, EU::o40, EU::o40, 
    EU::o40, EU::o40, EU::o40, EU::o40, EU::o40, EU::o40, EU::o40, EU::o40, 
    EU::o50, EU::o50, EU::o50, EU::o50, EU::o50, EU::o50, EU::o50, EU::o50, 
    EU::o58, EU::o58, EU::o58, EU::o58, EU::o58, EU::o58, EU::o58, EU::o58, 
    EU::o60, EU::o60, EU::o60, EU::o60, EU::o60, EU::o60, EU::o60, EU::o60, 
    EU::o60, EU::o60, EU::o60, EU::o60, EU::o60, EU::o60, EU::o60, EU::o60, 
    EU::o70, EU::o70, EU::o70, EU::o70, EU::o70, EU::o70, EU::o70, EU::o70, 
    EU::o70, EU::o70, EU::o70, EU::o70, EU::o70, EU::o70, EU::o70, EU::o70, 
    EU::o80, EU::o80, EU::o80, EU::o80, EU::o84, EU::o84, EU::o86, EU::o86,
    EU::o88, EU::o88, EU::o88, EU::o88, EU::o8C, EU::o8D, EU::o8C, EU::o8F,
    EU::o90, EU::o90, EU::o90, EU::o90, EU::o90, EU::o90, EU::o90, EU::o90, 
    EU::o98, EU::o99, EU::o9A, EU::o9B, EU::o9C, EU::o9D, EU::o9E, EU::o9F,
    EU::oA0, EU::oA0, EU::oA0, EU::oA0, EU::oA4, EU::oA4, EU::oA6, EU::oA6,
    EU::oA8, EU::oA8, EU::oAA, EU::oAA, EU::oAC, EU::oAC, EU::oAE, EU::oAE,
    EU::oB0, EU::oB0, EU::oB0, EU::oB0, EU::oB0, EU::oB0, EU::oB0, EU::oB0, 
    EU::oB0, EU::oB0, EU::oB0, EU::oB0, EU::oB0, EU::oB0, EU::oB0, EU::oB0, 
    EU::oC0, EU::oC0, EU::oC2, EU::oC2, EU::oC4, EU::oC4, EU::oC6, EU::oC6,
    EU::oC0, EU::oC0, EU::oC2, EU::oC2, EU::oCC, EU::oCD, EU::oCE, EU::oCF,
    EU::oD0, EU::oD0, EU::oD0, EU::oD0, EU::oD4, EU::oD5, EU::oD6, EU::oD7,
    EU::oD8, EU::oD8, EU::oD8, EU::oD8, EU::oD8, EU::oD8, EU::oD8, EU::oD8, 
    EU::oE0, EU::oE0, EU::oE0, EU::oE0, EU::oE4, EU::oE4, EU::oE4, EU::oE4, 
    EU::oE8, EU::oE9, EU::oEA, EU::oEB, EU::oE4, EU::oE4, EU::oE4, EU::oE4, 
    EU::oF0, EU::oF1, EU::oF2, EU::oF2, EU::oF4, EU::oF5, EU::oF6, EU::oF6,
    EU::oF8, EU::oF8, EU::oFA, EU::oFa, EU::oFC, EU::oFC, EU::oFE, EU::oFE };
                     

#ifdef _WIN32
int main()
#else
int main(int argc, char* argv[])
#endif
{
    BEGIN_CHECKED {
#ifdef _WIN32
        CommandLine commandLine;
#else
        CommandLine commandLine(argc, argv);
#endif
        if (commandLine.arguments() < 2) {
            static String syntax1("Syntax: ");
            static String syntax2(" <input file name>\n");
            (syntax1 + commandLine.argument(0) + syntax2).write(Handle::consoleOutput());
            exit(1);
        }
        File file(commandLine.argument(1));
        String contents = file.contents();
        CharacterSource source(contents, file.path());
        Space::parse(&source);
        SourceProgram sourceProgram = parseSourceProgram(&source);
        Simulator simulator;
        sourceProgram.assemble(&simulator);
        intel8088.simulate();
    }
    END_CHECKED(Exception& e) {
        e.write(Handle::consoleOutput());
    }
}

/* TODO:

Read input assembly source
Compile to binary
Simulate run
Output one line per cycle
  CPU state
  Bus address/data
  Instruction being executed


*/