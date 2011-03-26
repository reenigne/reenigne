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
    enum IOType
    {
        ioNone,
        ioRead,
        ioWrite,
        ioInstructionFetch
    };
public:
    class BusInterfaceUnit;
    class ExecutionUnit
    {
        enum State
        {
            stateWaitingForBIU,

            stateFetchOpcode,
            stateOpcodeAvailable,
            stateEndInstruction,

            stateModRM,
            stateModRM2,
            stateEAOffset,
            stateEARegisters,
            stateEAByte,
            stateEAWord,
            stateEASetSegment,
            stateIO,
            statePush,
            statePop,

            stateALU,
            stateALU2,
            stateALU3,

            stateALUAccumImm,
            stateALUAccumImm2,

            statePushSegReg,

            statePopSegReg,
            statePopSegReg2,

            stateSegOverride,

            stateDAA,
            stateDAS,
            stateDA,
            stateAAA,
            stateAAS,
            stateAA,

            stateIncDecRW,
            statePushRW,
            statePopRW,
            statePopRW2,

            stateInvalid60,

            stateJCond,
            stateJCond2,

            stateALURMImm,
            stateALURMImm2,
            stateALURMImm3,

            stateTestRMReg,
            stateTestRMReg2,

            stateXchgRMReg,
            stateXchgRMReg2,

            stateMovRMReg,
            stateMovRMReg2,
            stateMovRegRM,
            stateMovRegRM2,

            stateMovRMWSegReg,
            stateMovRMWSegReg2,
            stateLEA,
            stateLEA2,
            stateMovSegRegRMW,
            stateMovSegRegRMW2,
            statePopMW,
            statePopMW2,

            stateXchgAxRW,
            
            stateCBW,
            stateCWD,
        };
    public:
        ExecutionUnit()
          : _flags(0x0002),  // ?
            _state(stateFetchOpcode)
        {
            for (int i = 0; i < 8; ++i)
                _registers[i] = 0;  // ?
        }
        void setSimulator(Simulator* simulator)
        {
            _simulator = simulator;
            _biu = &_simulator->_biu;
        }
        void simulateCycle()
        {
            do {
                if (_wait > 0) {
                    --_wait;
                    return;
                }
                switch (_state) {
                    case stateWaitingForBIU:
                        return;


                    // Handle opcode

                    case stateFetchOpcode:
                        initIO(stateOpcodeAvailable, ioInstructionFetch, false);
                        break;
                    case stateOpcodeAvailable:
                        {
                            _opcode = _data;
                            _wordSize = ((_opcode & 1) != 0);
                            _sourceIsRM = ((_opcode & 2) != 0);
                            _afterEA = stateIO;
                            static State stateForOpcode[256] = {
                                stateALU,          stateALU,          stateALU,          stateALU,          stateALUAccumImm,  stateALUAccumImm,  statePushSegReg,   statePopSegReg,
                                stateALU,          stateALU,          stateALU,          stateALU,          stateALUAccumImm,  stateALUAccumImm,  statePushSegReg,   statePopSegReg,
                                stateALU,          stateALU,          stateALU,          stateALU,          stateALUAccumImm,  stateALUAccumImm,  statePushSegReg,   statePopSegReg,
                                stateALU,          stateALU,          stateALU,          stateALU,          stateALUAccumImm,  stateALUAccumImm,  statePushSegReg,   statePopSegReg,
                                stateALU,          stateALU,          stateALU,          stateALU,          stateALUAccumImm,  stateALUAccumImm,  stateSegOverride,  stateDAA,
                                stateALU,          stateALU,          stateALU,          stateALU,          stateALUAccumImm,  stateALUAccumImm,  stateSegOverride,  stateDAS,
                                stateALU,          stateALU,          stateALU,          stateALU,          stateALUAccumImm,  stateALUAccumImm,  stateSegOverride,  stateAAA,
                                stateALU,          stateALU,          stateALU,          stateALU,          stateALUAccumImm,  stateALUAccumImm,  stateSegOverride,  stateAAS,
                                stateIncDecRW,     stateIncDecRW,     stateIncDecRW,     stateIncDecRW,     stateIncDecRW,     stateIncDecRW,     stateIncDecRW,     stateIncDecRW, 
                                stateIncDecRW,     stateIncDecRW,     stateIncDecRW,     stateIncDecRW,     stateIncDecRW,     stateIncDecRW,     stateIncDecRW,     stateIncDecRW,                                 
                                statePushRW,       statePushRW,       statePushRW,       statePushRW,       statePushRW,       statePushRW,       statePushRW,       statePushRW,
                                statePopRW,        statePopRW,        statePopRW,        statePopRW,        statePopRW,        statePopRW,        statePopRW,        statePopRW,    
                                stateInvalid60,    stateInvalid60,    stateInvalid60,    stateInvalid60,    stateInvalid60,    stateInvalid60,    stateInvalid60,    stateInvalid60, 
                                stateInvalid60,    stateInvalid60,    stateInvalid60,    stateInvalid60,    stateInvalid60,    stateInvalid60,    stateInvalid60,    stateInvalid60, 
                                stateJCond,        stateJCond,        stateJCond,        stateJCond,        stateJCond,        stateJCond,        stateJCond,        stateJCond,     
                                stateJCond,        stateJCond,        stateJCond,        stateJCond,        stateJCond,        stateJCond,        stateJCond,        stateJCond,     
                                stateALURMImm,     stateALURMImm,     stateALURMImm,     stateALURMImm,     stateTestRMReg,    stateTestRMReg,    stateXchgRMReg,    stateXchgRMReg,
                                stateMovRMReg,     stateMovRMReg,     stateMovRegRM,     stateMovRegRM,     stateMovRMWSegReg, stateLEA,          stateMovSegRegRMW, statePopMW,
                                stateXchgAxRW,     stateXchgAxRW,     stateXchgAxRW,     stateXchgAxRW,     stateXchgAxRW,     stateXchgAxRW,     stateXchgAxRW,     stateXchgAxRW,     
                                stateCBW,          stateCWD,          0,  0,  0,  0,  0,  0,
                                 0,  0,  0,  0,  0,  0,  0,  0,
                                 0,  0,  0,  0,  0,  0,  0,  0,
                                 0,  0,  0,  0,  0,  0,  0,  0,
                                 0,  0,  0,  0,  0,  0,  0,  0,
                                 0,  0,  0,  0,  0,  0,  0,  0,
                                 0,  0,  0,  0,  0,  0,  0,  0,
                                 0,  0,  0,  0,  0,  0,  0,  0,
                                 0,  0,  0,  0,  0,  0,  0,  0,
                                 0,  0,  0,  0,  0,  0,  0,  0,
                                 0,  0,  0,  0,  0,  0,  0,  0,
                                 0,  0,  0,  0,  0,  0,  0,  0,
                                 0,  0,  0,  0,  0,  0,  0,  0};
                            _state = stateForOpcode[_opcode];
                        }
                        break;
                    case stateEndInstruction:
                        _biu->clearSegmentOverride();
                        _state = stateFetchOpcode;                         
                        break;


                    // Handle effective address reads and writes

                    case stateModRM:
                        initIO(stateModRM2, ioInstructionFetch, false);
                        break;
                    case stateModRM2:
                        _modRM = _data;
                        _useMemory = true;
                        switch (_modRM & 7) {
                            case 0: _wait = 7; _address = bx() + si(); break;
                            case 1: _wait = 8; _address = bx() + di(); break;
                            case 2: _wait = 8; _address = bp() + si(); break;
                            case 3: _wait = 7; _address = bp() + di(); break;
                            case 4: _wait = 5; _address =        si(); break;
                            case 5: _wait = 5; _address =        di(); break;
                            case 6: _wait = 5; _address = bp();        break;
                            case 7: _wait = 5; _address = bx();        break;
                        }
                        switch (_modRM & 0xc0) {
                            case 0x00:
                                if ((_modRM & 7) == 6)
                                    initIO(stateEAOffset, ioInstructionFetch, true);
                                else
                                    _state = stateEASetSegment;
                                break;
                            case 0x40:
                                initIO(stateEAByte, ioInstructionFetch, false);
                                break;
                            case 0x80:
                                initIO(stateEAWord, ioInstructionFetch, true);
                                break;
                            case 0xc0:
                                _useMemory = false;
                                _address = _modRM & 7;
                                _state = _afterEA;
                                break;
                        }
                        break;
                    case stateEAOffset:
                        _address = _data;
                        _biu->setSegment(3);
                        _wait = 6;
                        _state = _afterEA;
                        break;
                    case stateEAByte:
                        _address += signExtend(_data);
                        _wait += 4;
                        _state = stateEASetSegment;
                        break;
                    case stateEAWord:
                        _address += _data;
                        _wait += 4;
                        _state = stateEASetSegment;
                        break;
                    case stateEASetSegment:
                        switch (_modRM & 7) {
                            case 0: _biu->setSegment(3);
                            case 1: _biu->setSegment(3);
                            case 2: _biu->setSegment(2);
                            case 3: _biu->setSegment(2);
                            case 4: _biu->setSegment(3);
                            case 5: _biu->setSegment(3);
                            case 6: _biu->setSegment(2);
                            case 7: _biu->setSegment(3);
                        }
                        _state = _afterEA;
                        break;

                    case stateIO:
                        if (_useMemory) {
                            _biu->setAddress(_address);
                            initIO(_afterIO, _ioType, _wordSize);
                        }
                        else {
                            if (!_wordSize)
                                if (_ioType == ioRead)
                                    _data = byteRegister(_address);
                                else
                                    byteRegister(_address) = _data;
                            else
                                if (_ioType == ioRead)
                                    _data = _registers[_address];
                                else
                                    _registers[_address] = _data;
                            _state = _afterIO;
                        }
                        break;

                    case statePush:
                        sp() -= 2;
                        _biu->setAddress(sp());
                        _biu->setSegment(2);
                        _wait = 7;
                        initIO(stateEndInstruction, ioWrite, true);
                        break;
                    case statePop:
                        _biu->setAddress(sp());
                        sp() += 2;
                        _biu->setSegment(2);
                        initIO(_afterIO, ioRead, true);
                        break;


                    // alu reg<->regmem

                    case stateALU:
                        _afterIO = stateALU2;
                        _ioType = ioRead;
                        _afterEA = stateIO;
                        _state = stateModRM;
                        break;
                    case stateALU2:
                        _wait = 5;
                        if (!_sourceIsRM) {
                            _destination = _data;                        
                            _source = getReg();
                            _wait += 3;
                        }
                        else {
                            _destination = getReg();
                            _source = _data;
                        }
                        if (!_useMemory)
                            _wait = 3;
                        _aluOperation = (_opcode >> 3) & 7;
                        _state = stateALU3;
                        break;
                    case stateALU3:
                        doALUOperation();
                        if (_aluOperation != 7) {
                            if (!_sourceIsRM) {
                                _ioType = ioWrite;
                                _afterIO = stateEndInstruction;
                                _state = stateIO;
                            }
                            else
                                _state = stateEndInstruction;
                        }
                        else
                            _state = stateEndInstruction;
                        break;


                    // alu accum, imm

                    case stateALUAccumImm:
                        _wait = 4;
                        initIO(stateALUAccumImm2, ioInstructionFetch, _wordSize);
                        break;
                    case stateALUAccumImm2:
                        _destination = getAccum();
                        _source = _data;
                        _aluOperation = (_opcode >> 3) & 7;
                        doALUOperation();
                        if (_aluOperation != 7)
                            setAccum();
                        _state = stateEndInstruction;
                        break;


                    // PUSH segreg

                    case statePushSegReg:
                        _data = _biu->getSegmentRegister(_opcode >> 3);
                        _state = statePush;
                        break;


                    // POP segreg

                    case statePopSegReg:
                        _afterIO = statePopSegReg2;
                        _state = statePop;
                        break;
                    case statePopSegReg2:
                        _biu->setSegmentRegister(_opcode >> 3, _data);
                        _state = stateEndInstruction;
                        break;


                    // segreg:

                    case stateSegOverride:
                        _biu->setSegmentOverride((_opcode >> 3) & 3);
                        _state = stateFetchOpcode;
                        _wait = 2;
                        break;


                    // BCD instructions

                    case stateDAA:
                        if (af() || (al() & 0x0f) > 9) {
                            _data = al() + 6;
                            al() = _data;
                            setAF(true);
                            if ((_data & 0x100) != 0)
                                setCF(true);
                        }
                        if (cf() || al() > 0x9f) {
                            al() += 0x60;
                            setCF(true);
                        }
                        _state = stateDA;
                        break;
                    case stateDAS:
                        {
                            UInt8 t = al();
                            if (af() || ((al() & 0xf) > 9)) {
                                _data = al() - 6;
                                al() = _data;
                                setAF(true);
                                if ((_data & 0x100) != 0)
                                    setCF(true);
                            }
                            if (cf() || t > 0x9f) {
                                al() -= 0x60;
                                setCF(true);
                            }
                        }
                        _state = stateDA;
                        break;
                    case stateDA:
                        _wordSize = false;
                        _data = al();
                        setPZS();
                        _wait = 4;
                        _state = stateEndInstruction;
                        break;
                    case stateAAA:
                        if (af() || ((al() & 0xf) > 9)) {
                            al() += 6;
                            ++ah();
                            setCA();
                        }
                        else
                            clearCA();
                        al() &= 0x0f;
                        _wait = 8;
                        _state = stateEndInstruction;
                        break;
                    case stateAAS:
                        if (af() || ((al() & 0xf) > 9)) {
                            al() -= 6;
                            --ah();
                            setCA();
                        }
                        else
                            clearCA();
                        _state = stateAA;
                        break;
                    case stateAA:
                        al() &= 0x0f;
                        _wait = 8;
                        _state = stateEndInstruction;
                        break;


                    // INC/DEC/PUSH/POP rw

                    case stateIncDecRW:
                        _destination = rw();
                        _source = 1;
                        _wordSize = true;
                        if ((_opcode & 8) == 0) {
                            _data = _destination + _source;
                            setOFAdd();
                        }
                        else {
                            _data = _destination - _source;
                            setOFSub();
                        }
                        setAFResult();
                        setPZS();
                        rw() = _data;
                        _wait = 4;
                        _state = stateEndInstruction;
                        break;
                    case statePushRW:
                        sp() -= 2;
                        _biu->setAddress(sp());
                        _biu->setSegment(2);
                        _wait = 7;
                        _data = rw();
                        initIO(stateEndInstruction, ioWrite, true);
                        break;
                    case statePopRW:
                        _biu->setAddress(sp());
                        sp() += 2;
                        _biu->setSegment(2);
                        initIO(statePopRW2, ioRead, true);
                        break;
                    case statePopRW2:
                        rw() = _data;
                        _state = stateEndInstruction;
                        break;


                    // Invalid opcodes

                    case stateInvalid60:
                        _state = stateEndInstruction;
                        break;


                    // JCond cb

                    case stateJCond:
                        initIO(stateJCond2, ioInstructionFetch, false);
                        break;
                    case stateJCond2:
                        {
                            bool jump;
                            switch (_opcode & 0x0e) {
                                case 0x00: jump =  of(); break;
                                case 0x72: jump =  cf(); break;
                                case 0x74: jump =  zf(); break;
                                case 0x76: jump =  cf() || zf(); break;
                                case 0x78: jump =  sf(); break;
                                case 0x7a: jump =  pf(); break;
                                case 0x7c: jump = (sf() != of()) && !zf(); break;
                                case 0x7e: jump = (sf() != of()) ||  zf(); break;
                            }
                            if ((_opcode & 1) != 0)
                                jump = !jump;
                            _wait = 4;
                            if (jump) {
                                _biu->setIP(_biu->getIP() + signExtend(_data));
                                _wait += 12;
                            }
                            _state = stateEndInstruction;
                        }
                        break;


                    // alu regmem, imm

                    case stateALURMImm:
                        _ioType = ioRead;
                        _afterIO = stateALURMImm2;
                        _afterEA = stateIO;
                        _state = stateModRM;
                        break;
                    case stateALURMImm2:
                        _destination = _data;
                        initIO(stateALURMImm3, ioInstructionFetch, _opcode == 0x81);
                        break;
                    case stateALURMImm3:
                        if (_opcode != 0x83)
                            _source = _data;
                        else
                            _source = signExtend(_data);
                        _aluOperation = (_modRM >> 3) & 7;
                        _wait = 9;
                        if (_aluOperation == 7)
                            _wait = 6;
                        if (_wordSize && (_aluOperation == 0 || _aluOperation == 2 || _aluOperation == 4))
                            _wait = 7;
                        if (!_useMemory)
                            _wait = 4;
                        doALUOperation();
                        if (_aluOperation != 7) {
                            _ioType = ioWrite;
                            _afterIO = stateEndInstruction;
                            _state = stateIO;
                        }
                        else
                            _state = stateEndInstruction;
                        break;


                    // TEST regmem, reg

                    case stateTestRMReg:
                        _afterIO = stateTestRMReg2;
                        _ioType = ioRead;
                        _afterEA = stateIO;
                        _state = stateModRM;
                        break;
                    case stateTestRMReg2:
                        _destination = _data;
                        _source = getReg();
                        _data = _destination & _source;
                        doFlagsBitwise();
                        _wait = (_useMemory ? 5 : 3);
                        _state = stateEndInstruction;
                        break;
                        
                   
                    // XCHG regmem, reg

                    case stateXchgRMReg:
                        _afterIO = stateXchgRMReg2;
                        _ioType = ioRead;
                        _afterEA = stateIO;
                        _state = stateModRM;
                        break;
                    case stateXchgRMReg2:
                        _source = getReg();
                        setReg(_data);
                        _data = _source;
                        _ioType = ioWrite;
                        _wait = (_useMemory ? 9 : 4);
                        _afterIO = stateEndInstruction;
                        _state = stateIO;
                        break;


                    // MOV regmem, reg
                       
                    case stateMovRMReg:
                        _afterEA = stateMovRMReg2;
                        _state = stateModRM;
                        break;
                    case stateMovRMReg2:
                        _data = getReg();
                        _ioType = ioWrite;
                        _afterIO = stateEndInstruction;
                        _state = stateIO;
                        _wait = (_useMemory ? 5 : 2);
                        break;


                    // MOV reg, regmem

                    case stateMovRegRM:
                        _afterEA = stateIO;
                        _afterIO = stateMovRegRM2;
                        _ioType = ioRead;
                        _state = stateModRM;
                        break;
                    case stateMovRegRM2:
                        _wait = (_useMemory ? 4 : 2);
                        setReg(_data);
                        _state = stateEndInstruction;
                        break;


                    // MOV rmw, segreg

                    case stateMovRMWSegReg:
                        _afterEA = stateMovRMWSegReg2;
                        _wordSize = true;
                        _state = stateModRM;
                        break;
                    case stateMovRMWSegReg2:
                        _ioType = ioWrite;
                        _data = _biu->getSegmentRegister(modRMReg() & 3);
                        _wait = (_useMemory ? 5 : 2);
                        _afterIO = stateEndInstruction;
                        _state = stateIO;
                        break;


                    // LEA rw, mem

                    case stateLEA:
                        _afterEA = stateLEA2;
                        _state = stateModRM;
                        _wait = 2;
                        break;
                    case stateLEA2:
                        setReg(_address);
                        _state = stateEndInstruction;
                        break;


                    // MOV segreg, rmw

                    case stateMovSegRegRMW:
                        _afterIO = stateMovSegRegRMW2;
                        _afterEA = stateIO;
                        _wordSize = true;
                        _ioType = ioRead;
                        _state = stateModRM;
                        break;
                    case stateMovSegRegRMW2:
                        _biu->setSegmentRegister(modRMReg() & 3, _data);
                        _wait = (_useMemory ? 4 : 2);
                        _state = stateEndInstruction;
                        break;


                    // POP mw

                    case statePopMW:
                        _afterIO = statePopMW2;
                        _afterEA = statePop;
                        _state = stateModRM;
                        break;
                    case statePopMW2:
                        _afterIO = stateEndInstruction;
                        _state = stateIO;
                        _wait = 1;
                        break;


                    // XCHG AX,rw

                    case stateXchgAxRW:
                        _data = rw();
                        rw() = ax();
                        ax() = _data;
                        _wait = 3;
                        _state = stateEndInstruction;
                        break;
                    

                    // CBW

                    case stateCBW:
                        ax() = signExtend(al());
                        _wait = 2;
                        _state = stateEndInstruction;
                        break;


                    // CWD

                    case stateCWD:
                        dx() = ((ax() & 0x8000) == 0 ? 0x0000 : 0xffff);
                        _wait = 5;
                        _state = stateEndInstruction;
                        break;


                    // CALL cp

                    case stateCallCP:
                        initIO(stateCallCP2, ioInstructionFetch, true);
                        break;
                    case stateCallCP2:

                        initIO(stateC


                    //void o9A() { /* TODO: CALL cp */ }
                    //void o9B() { /* TODO: WAIT */ _wait = 4; }
                    //void o9C() { /* PUSHF */ push(_flags & 0x0fd7); _wait = 14; }
                    //void o9D() { /* TODO: POPF */ }
                    //void o9E() { /* SAHF */ _flags = (_flags & 0xff02) | ah(); _wait = 4; }
                    //void o9F() { /* LAHF */ ah() = _flags & 0xd7; _wait = 4; }
                    //void oA0() { /* TODO: MOV  accum<->[imm] */ }
                    //void oA4() { /* TODO: MOVS */ }
                    //void oA6() { /* TODO: CMPS */ }
                    //void oA8() { /* TODO: TEST accum,imm */ }
                    //void oAA() { /* TODO: STOS */ }
                    //void oAC() { /* TODO: LODS */ }
                    //void oAE() { /* TODO: SCAS */ }
                    //void oB0() { /* TODO: MOV reg,imm */ _wait = 4; }
                    //void oC0() { /* TODO: invalid */ }
                    //void oC2() { /* TODO: RET/RETF */ }
                    //void oC4() { /* TODO: Lsegreg rw,m */ _state = 4; }
                    //void oC6() { /* TODO: MOV  rm,imm */ _state = 4; }
                    //void oCC() { /* TODO: INT  3 */ }
                    //void oCD() { /* TODO: INT  ib */ }
                    //void oCE() { /* TODO: INTO */ }
                    //void oCF() { /* TODO: IRET */ }
                    //void oD0() { /* TODO: shift */ _state = 4; }
                    //void oD4() { /* TODO: AAM  ib */ }
                    //void oD5() { /* TODO: AAD  ib */ }
                    //void oD6() { /* SALC */ al() = carry() ? 0xff : 0x00; _wait = 4; }
                    //void oD7() { /* TODO: XLATB */ }
                    //void oD8() { /* TODO: ESC */ _wait = 2; _state = 4; }
                    //void oE0() { /* TODO: loop cb */ }
                    //void oE4() { /* TODO: IN/OUT */ }
                    //void oE8() { /* TODO: CALL cw */ }
                    //void oE9() { /* TODO: JMP  cw */ }
                    //void oEA() { /* TODO: JMP  cp */ }
                    //void oEB() { /* TODO: JMP  cb */ }
                    //void oF0() { /* TODO: LOCK */ _wait = 2; }
                    //void oF1() { /* TODO: invalid */ }
                    //void oF2() { /* TODO: REPNE/REP */ _wait = 2; }
                    //void oF4() { /* TODO: HLT */ _wait = 2; }
                    //void oF5() { /* CMC */ _flags ^= 1; _wait = 2; }
                    //void oF6() { /* TODO: misc1 */ _state = 4; }
                    //void oF8() { /* CLC/STC */ _flags = (_flags & 0xfffe) | (_opcode & 1); _wait = 2; }
                    //void oFA() { /* CLI/STI */ _flags = (_flags & 0xfdff) | ((_opcode & 1) << 9); _wait = 2; }
                    //void oFC() { /* CLD/STD */ _flags = (_flags & 0xfbff) | ((_opcode & 1) << 10); _wait = 2; }
                    //void oFE() { /* TODO: misc2 */ _state = 4; }

                }
            } while (true);
        }
        void ioComplete(State newState, UInt16 data)
        {
            _state = newState;
            _data = data;
        }
    private:
        void setWordSize() {  }

        void setCA()
        {
            setCF(true);
            setAF(true);
        }
        void clearCA()
        {
            setCF(false);
            setAF(false);
        }
        void clearCAO()
        {
            clearCA();
            setOF(false);
        }
        void setPZS()
        {
            setPF();
            setZF();
            setSF();
        }
        void doFlagsBitwise()
        {
            clearCAO();
            setPZS();
        }
        void setAFResult()
        {
            setAF(((_data ^ _source ^ _destination) & 0x10) != 0);
        }
        void setAPZSClearCO()
        {
            doFlagsBitwise();
            setAFResult();
        }
        void setCAPZS()
        {
            setAPZSClearCO();
            setCF((_data & (!_wordSize ? 0x100 : 0x10000)) != 0);
        }
        void setOFAdd()
        {
            UInt16 t = (_data ^ _source) & (_data ^ _destination);
            setOF((t & (!_wordSize ? 0x80 : 0x8000)) != 0);
        }
        void doFlagsAdd()
        {
            setCAPZS();
            setOFAdd();
        }
        void setOFSub()
        {
            UInt16 t = (_destination ^ _source) & (_data ^ _destination);
            setOF((t & (!_wordSize ? 0x80 : 0x8000)) != 0);
        }
        void doFlagsSub()
        {
            setCAPZS();
            setOFSub();
        }

        void doALUOperation()
        {
            switch (_aluOperation) {
                case 0:  // ADD
                    _data = _destination + _source;
                    doFlagsAdd();
                    break;
                case 1:  // OR
                    _data = _destination | _source;
                    doFlagsBitwise();
                    break;
                case 2:  // ADC
                    _source += cf() ? 1 : 0;
                    _data = _destination + _source;
                    doFlagsAdd();
                    break;
                case 3:  // SBB
                    _source += cf() ? 1 : 0;
                    _data = _destination - _source;
                    doFlagsSub();
                    break;
                case 4:  // AND
                    _data = _destination & _source;
                    doFlagsBitwise();
                    break;
                case 5:  // SUB
                case 7:  // CMP
                    _data = _destination - _source;
                    doFlagsSub();
                    break;
                case 6:  // XOR
                    _data = _destination ^ _source;
                    doFlagsBitwise();
                    break;
            }
        }

        UInt16 signExtend(UInt8 data) { return data + (data < 0x80 ? 0 : 0xff00); }

        void initIO(int nextState, IOType ioType, bool wordSize)
        {
            _biu->initIO(nextState, ioType, wordSize, _data);
            _state = stateWaitingForBIU;
        }

        UInt16& rw() { return _registers[_opcode & 7]; }
        UInt16& ax() { return _registers[0]; }
        UInt16& cx() { return _registers[1]; }
        UInt16& dx() { return _registers[2]; }
        UInt16& bx() { return _registers[3]; }
        UInt16& sp() { return _registers[4]; }
        UInt16& bp() { return _registers[5]; }
        UInt16& si() { return _registers[6]; }
        UInt16& di() { return _registers[7]; }
        UInt8& rb() { return byteRegister(_opcode & 7); }
        UInt8& al() { return byteRegister(0); }
        UInt8& cl() { return byteRegister(1); }
        UInt8& ah() { return byteRegister(4); }

        bool cf() { return (_flags & 1) != 0; }
        void setCF(bool cf) { _flags = (_flags & ~1) | (cf ? 1 : 0); }
        bool pf() { return (_flags & 4) != 0; }
        void setPF()
        { 
            static UInt8 table[256] = {
                4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
                0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
                0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
                4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
                0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
                4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
                4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
                0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
                0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
                4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
                4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
                0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
                4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
                0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
                0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
                4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4};
            _flags = (_flags & ~4) | table[_data & 0xff]; 
        }
        bool af() { return (_flags & 0x10) != 0; }
        void setAF(bool af) { _flags = (_flags & ~0x10) | (af ? 0x10 : 0); }
        bool zf() { return (_flags & 0x40) != 0; }
        void setZF() { _flags = (_flags & ~0x40) | ((_data & (!_wordSize ? 0xff : 0xffff)) != 0 ? 0x40 : 0); }
        bool sf() { return (_flags & 0x80) != 0; }
        void setSF() { _flags = (_flags & ~0x80) | ((_data & (!_wordSize ? 0x80 : 0x8000)) != 0 ? 0x80 : 0); }
        bool intf() { return (_flags & 0x200) != 0; }
        void setIF(bool intf) { _flags = (_flags & ~0x200) | (intf ? 0x200 : 0); }
        bool df() { return (_flags & 0x400) != 0; }
        void setDF(bool df) { _flags = (_flags & ~0x400) | (df ? 0x400 : 0); }
        bool of() { return (_flags & 0x800) != 0; }
        void setOF(bool of) { _flags = (_flags & ~0x800) | (of ? 0x800 : 0); }

        int modRMReg() { return (_modRM >> 3) & 7; }
        UInt16& modRMRW() { return _registers[modRMReg()]; }
        UInt8& modRMRB() { return byteRegister(modRMReg()); }
        UInt16 getReg() { return !_wordSize ? modRMRB() : modRMRW(); }
        UInt16 getAccum() { return !_wordSize ? al() : ax(); }
        void setAccum() { if (!_wordSize) al() = _data; else ax() = _data; }
        void setReg(UInt16 value) { if (!_wordSize) modRMRB() = value; else modRMRW() = value; }

        UInt8& byteRegister(int n) /* AL CL DL BL AH CH DH BH */
        {
            return *(reinterpret_cast<UInt8*>(&_registers[n & 3]) + (n >= 4 ? 1 : 0));
        }

        UInt16 _registers[8];      /* AX CX DX BX SP BP SI DI */
        UInt16 _flags;
            //   0: CF = unsigned overflow?
            //   2: PF = parity: even number of 1 bits in result?
            //   4: AF = unsigned overflow for low nybble
            //   6: ZF = zero result?
            //   7: SF = result is negative?
            //   8: TF = interrupt after every instruction?
            //   9: IF = interrupts enabled?
            //  10: DF = SI/DI decrement in string operations
            //  11: OF = signed overflow?

        Simulator* _simulator;
        BusInterfaceUnit* _biu;
        int _wait;
        State _state;
        UInt8 _opcode;
        UInt8 _modRM;
        UInt32 _data;
        IOType _ioType;
        UInt32 _source;
        UInt32 _destination;
        UInt16 _address;
        bool _useMemory;
        bool _wordSize;
        int _aluOperation;
        State _afterEA;
        State _afterIO;
        bool _sourceIsRM;

        static int _parityTable[256];
    };
    class BusInterfaceUnit
    {
    public:
        BusInterfaceUnit()
          : _ip(0xfff0),
            _prefetchOffset(0),
            _prefetched(0),
            _segmentOverride(-1),
            _prefetchAddress(_ip),
            _ioRequested(ioNone),
            _ioInProgress(ioInstructionFetch),
            _state(t1),
            _abandonFetch(false)
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
            do {
                switch (_state) {
                    case t1:
                        _state = t2;
                        return;
                    case t2:
                        _state = t3;
                        return;
                    case t3:
                        _state = t4;  // or tWait
                        return;
                    case tWait:
                        _state = t4;
                        return;
                    case t4:
                        switch (_ioInProgress) {
                            case ioRead:
                                {
                                    _ioInProgress = ioNone;
                                    _ioRequested = ioNone;
                                    switch (_byte) {
                                        case ioSingleByte:
                                            _eu->ioComplete(_nextEUState, *physicalAddress(_address));
                                            break;
                                        case ioWordFirst:
                                            _data = *physicalAddress(_address);
                                            _ioInProgress = ioRead;
                                            _byte = ioWordSecond;
                                            break;
                                        case ioWordSecond:
                                            _data |= static_cast<UInt16>(*physicalAddress(_address + 1)) << 8;
                                            _eu->ioComplete(_nextEUState, _data);
                                            break;
                                    }
                                }
                                break;
                            case ioWrite:
                                {
                                    _ioInProgress = ioNone;
                                    _ioRequested = ioNone;
                                    switch (_byte) {
                                        case ioSingleByte:
                                            *physicalAddress(_address) = _data;
                                            _eu->ioComplete(_nextEUState, 0);
                                            break;
                                        case ioWordFirst:
                                            *physicalAddress(_address) = _data;
                                            _ioInProgress = ioWrite;
                                            _byte = ioWordSecond;
                                            break;
                                        case ioWordSecond:
                                            *physicalAddress(_address + 1) = _data >> 8;
                                            _eu->ioComplete(_nextEUState, 0);
                                            break;
                                    }
                                }
                                break;
                            case ioInstructionFetch:
                                if (!_abandonFetch) {
                                    _prefetchQueue[(_prefetchOffset + _prefetched) & 3] =
                                        _memory[((_segmentRegisters[1] << 4) + _prefetchAddress) & 0xfffff];
                                    ++_prefetched;
                                    ++_prefetchAddress;
                                }
                                completeInstructionFetch();
                                break;
                        }
                        _state = tIdle;
                        break;
                    case tIdle:
                        _abandonFetch = false;
                        if (_ioInProgress == ioNone && _ioRequested != ioNone) {
                            _ioInProgress = _ioRequested;
                            _state = t1;
                        }
                        if (_ioInProgress == ioNone && _prefetched < 4) {
                            _ioInProgress = ioInstructionFetch;
                            _state = t1;
                        }
                        return;
                }
            } while (true);
        }
        void setAddress(UInt16 address) { _address = address; }
        void setSegment(int segment) { _segment = segment; }
        void initIO(int nextEUState, IOType ioType, bool wordSize, UInt16 data)
        {
            _nextEUState = nextEUState;
            _ioRequested = ioType;
            _byte = (!wordSize ? ioSingleByte : ioWordFirst);
            if (ioType == ioInstructionFetch)
                completeInstructionFetch();
            _data = data;
        }
        void setSegmentOverride(int segment) { _segmentOverride = segment; }
        void clearSegmentOverride() { _segmentOverride = -1; }
        UInt16 getSegmentRegister(int segment) { return _segmentRegisters[segment]; }
        void setSegmentRegister(int segment, UInt16 value) { _segmentRegisters[segment] = value; }
        UInt16 getIP() { return _ip; }
        void setIP(UInt16 value) { _ip = value; _abandonFetch = true; }
    private:
        enum State
        {
            t1,
            t2,
            t3,
            tWait,
            t4,
            tIdle
        };
        enum IOByte
        {
            ioSingleByte,
            ioWordFirst,
            ioWordSecond
        };

        UInt8* physicalAddress(UInt16 address)
        {
            int segment = _segment;
            if (_segmentOverride != -1)
                segment = _segmentOverride;
            return &_memory[((_segmentRegisters[segment] << 4) + address) & 0xfffff];
        }
        UInt8 getInstructionByte()
        {
            UInt8 byte = _prefetchQueue[_prefetchOffset & 3];
            _prefetchOffset = (_prefetchOffset + 1) & 3;
            --_prefetched;
            return byte;
        }
        void completeInstructionFetch()
        {
            if (_ioRequested != ioInstructionFetch)
                return;
            if (_byte == ioSingleByte) {
                if (_prefetched > 0) {
                    _ioRequested = ioNone;
                    _eu->ioComplete(_nextEUState, getInstructionByte());
                    ++_ip;
                }
            }
            else {
                if (_prefetched > 1) {
                    UInt16 data = getInstructionByte();
                    data |= static_cast<UInt16>(getInstructionByte()) << 8;
                    _ioRequested = ioNone;
                    _eu->ioComplete(_nextEUState, data);
                    _ip += 2;
                }
            }
        }

        UInt16 _segmentRegisters[4];  /* ES CS SS DS */
        UInt16 _ip;
        UInt8 _prefetchQueue[4];
        UInt8 _prefetchOffset;
        UInt8 _prefetched;
        int _segment;
        int _segmentOverride;
        UInt16 _data;
        Simulator* _simulator;
        ExecutionUnit* _eu;
        UInt16 _address;
        Array<UInt8> _memory;
        UInt16 _prefetchAddress;
        ExecutionUnit::State _nextEUState;
        IOType _ioRequested;
        IOType _ioInProgress;
        State _state;
        IOByte _byte;
        bool _abandonFetch;
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
