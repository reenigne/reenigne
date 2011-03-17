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
private:
    class ExecutionUnit;
    class BusInterfaceUnit
    {
    public:
        BusInterfaceUnit()
          : _es(0x0000),  // ?
            _cs(0xf000),
            _ss(0x0000),  // ?
            _ds(0x0000),  // ?
            _ip(0xfff0),
            _prefetchOffset(0),
            _prefetched(0),
            _segment(0),  // ?
            _readRequested(false),
            _writeRequested(false)
        {
            _memory.allocate(0x100000);
        }
        void setSimulator(Simulator* simulator)
        { 
            _simulator = simulator;
            _eu = &_simulator->_eu;
        }
        void simulateCycle()
        {
            if (_tState == 0) {  // T1
                if (_readPending) {
                    _readPending = false;
                    _data = *physicalAddress();
                    _eu->readComplete();
                }
                else
                    if (_writePending) {
                        _writePending = false;
                        *physicalAddress() = _data;
                        _eu->writeComplete();
                    }
                    else
                        if (_prefetchPending) {
                            _prefetchPending = false;


                if (_readRequested) {
                    _readRequested = false;
                    _readPending = true;
                    _tState = 1;
                }
                else
                    if (_writeRequested) {
                        _writeRequested = false;
                        _writePending = true;
                        _tState = 1;
                    }
                    else
                        if (_prefetched < 4) {
                            ++_prefetched;
                            _tState = 1;
                        }
            }
            else
                if (_tState == 1)  // T2
                    _tState = 2;
                else
                    if (_tState == 2)
                
        }
        UInt16* segmentRegister()
        {
            switch (_segment) {
                case 0:
                    return &_es;
                case 1:
                    return &_cs;
                case 2:
                    return &_ss;
                case 3:
                    return &_ds;
            }
            return 0;
        }
        UInt8* physicalAddress()
        {
            return &_memory[(((*segmentRegister()) << 4) + _address) & 0xfffff];
        }
        UInt8 getInstructionByte()
        {
            if (_prefetched > 1) {
                return _
        }
    private:
        UInt16 _es;
        UInt16 _cs;
        UInt16 _ss;
        UInt16 _ds;
        UInt16 _ip;
        UInt8 _prefetchQueue[4];
        UInt8 _prefetchOffset;
        UInt8 _prefetched;
        int _segment;
        bool _readRequested;
        bool _writeRequested;
        bool _readPending;
        bool _writePending;
        bool _prefetchPending;
        int _tState;
        UInt8 _data;
        Simulator* _simulator;
        ExecutionUnit* _eu;
        UInt16 _address;
        Array<UInt8> _memory;
    };
    class ExecutionUnit
    {
    public:
        ExecutionUnit()
          : _ax(0x0000),  // ?
            _bx(0x0000),  // ?
            _cx(0x0000),  // ?
            _dx(0x0000),  // ?
            _sp(0x0000),  // ?
            _bp(0x0000),  // ?
            _si(0x0000),  // ?
            _di(0x0000),  // ?
            _flags(0x0000)  // ?
        {
        }
        void setSimulator(Simulator* simulator) { _simulator = simulator; }
        void simulateCycle()
        {
        }
    private:
        UInt16 _ax;
        UInt16 _bx;
        UInt16 _cx;
        UInt16 _dx;
        UInt16 _sp;
        UInt16 _bp;
        UInt16 _si;
        UInt16 _di;
        UInt16 _flags;
        Simulator* _simulator;
    };

    BusInterfaceUnit _biu;
    ExecutionUnit _eu;

    friend class BusInterfaceUnit;
    friend class ExecutionUnit;
};



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