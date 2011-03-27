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
      : _flags(0x0002),  // ?
        _state(stateFetchOpcode),
        _ip(0xfff0),
        _prefetchOffset(0),
        _prefetched(0),
        _segmentOverride(-1),
        _prefetchAddress(_ip),
        _ioRequested(ioNone),
        _ioInProgress(ioInstructionFetch),
        _busState(t1),
        _abandonFetch(false)
    {
        for (int i = 0; i < 8; ++i)
            _registers[i] = 0;  // ?

        _segmentRegisters[0] = 0x0000;  // ?
        _segmentRegisters[1] = 0xf000;
        _segmentRegisters[2] = 0x0000;  // ?
        _segmentRegisters[3] = 0x0000;  // ?
        _memory.allocate(0x100000);
    }
    void simulateCycle()
    {
        bool busDone;
        do {
            busDone = true;
            switch (_busState) {
                case t1:
                    _busState = t2;
                    break;
                case t2:
                    _busState = t3;
                    break;
                case t3:
                    _busState = t4;  // or tWait
                    return;
                case tWait:
                    _busState = t4;
                    return;
                case t4:
                    switch (_ioInProgress) {
                        case ioRead:
                            {
                                _ioInProgress = ioNone;
                                _ioRequested = ioNone;
                                switch (_byte) {
                                    case ioSingleByte:
                                        _data = *physicalAddress(_address);
                                        _state = _afterIO;
                                        break;
                                    case ioWordFirst:
                                        _data = *physicalAddress(_address);
                                        _ioInProgress = ioRead;
                                        _byte = ioWordSecond;
                                        break;
                                    case ioWordSecond:
                                        _data |= static_cast<UInt16>(*physicalAddress(_address + 1)) << 8;
                                        _state = _afterIO;
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
                                        _state = _afterIO;
                                        break;
                                    case ioWordFirst:
                                        *physicalAddress(_address) = _data;
                                        _ioInProgress = ioWrite;
                                        _byte = ioWordSecond;
                                        break;
                                    case ioWordSecond:
                                        *physicalAddress(_address + 1) = _data >> 8;
                                        _state = _afterIO;
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
                    _busState = tIdle;
                    busDone = false;
                    break;
                case tIdle:
                    _abandonFetch = false;
                    if (_ioInProgress == ioNone && _ioRequested != ioNone) {
                        _ioInProgress = _ioRequested;
                        _busState = t1;
                    }
                    if (_ioInProgress == ioNone && _prefetched < 4) {
                        _ioInProgress = ioInstructionFetch;
                        _busState = t1;
                    }
                    busDone = true;
                    return;
            }
        } while (!busDone);
        do {
            if (_wait > 0) {
                --_wait;
                return;
            }
            switch (_state) {
                case stateWaitingForBIU:
                    return;


                // Handle opcode

                case stateFetch: fetch(stateFetch2, false); break;
                case stateFetch2:
                    {
                        _opcode = _data;
                        _wordSize = ((_opcode & 1) != 0);
                        _sourceIsRM = ((_opcode & 2) != 0);
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
                            stateCBW,          stateCWD,          stateCallCP,       stateWait,         statePushF,        statePopF,         stateSAHF,         stateLAHF,
                            stateMovAccumInd,  stateMovAccumInd,  stateMovIndAccum,  stateMovIndAccum,  stateMovS,         stateMovS,         stateCmpS,         stateCmpS,
                            stateTestAccumImm, stateTestAccumImm, stateStoS,         stateStoS,         stateLodS,         stateLodS,         stateScaS,         stateScaS,
                            stateMovRegImm,    stateMovRegImm,    stateMovRegImm,    stateMovRegImm,    stateMovRegImm,    stateMovRegImm,    stateMovRegImm,    stateMovRegImm,    
                            stateMovRegImm,    stateMovRegImm,    stateMovRegImm,    stateMovRegImm,    stateMovRegImm,    stateMovRegImm,    stateMovRegImm,    stateMovRegImm,    
                            stateInvalidC0,    stateInvalidC0,    stateRet,          stateRet,          stateLoadFar,      stateLoadFar,      stateMovRMImm,     stateMovRMImm,     
                            stateInvalidC0,    stateInvalidC0,    stateRet,          stateRet,          stateInt,          stateInt,          stateIntO,         stateIRet, 
                            stateShift,        stateShift,        stateShift,        stateShift,        stateAAM,          stateAAD,          stateSALC,         stateXlatB,
                            stateEscape,       stateEscape,       stateEscape,       stateEscape,       stateEscape,       stateEscape,       stateEscape,       stateEscape,       
                            stateLoop,         stateLoop,         stateLoop,         stateLoop,         stateIn,           stateIn,           stateOut,          stateOut,
                            stateCallCW,       stateJmpCW,        stateJmpCP,        stateJmpCb,        stateIn,           stateIn,           stateOut,          stateOut,
                            stateLock,         stateInvalidF1,    stateRep,          stateRep,          stateHlt,          stateCmC,          stateMath,         stateMath,
                            stateCLC,          stateSTC,          stateCLI,          stateSTI,          stateCLD,          stateSTD,          stateMisc,         stateMisc};
                        _state = stateForOpcode[_opcode];
                    }
                    break;
                case stateEndInstruction:
                    _segmentOverride = -1;
                    _state = stateFetch;
                    break;


                // Handle effective address reads and writes

                case stateModRM: fetch(stateModRM2, false); break;
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
                                fetch(stateEAOffset, true);
                            else
                                _state = stateEASetSegment;
                            break;
                        case 0x40: fetch(stateEAByte, false); break;
                        case 0x80: fetch(stateEAWord, true); break;
                        case 0xc0:
                            _useMemory = false;
                            _address = _modRM & 7;
                            _state = _afterEA;
                            break;
                    }
                    break;
                case stateEAOffset:
                    _address = _data;
                    _segment = 3;
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
                    {
                        static int segments[8] = {3, 3, 2, 2, 3, 3, 2, 3};
                        _segment = segments[_modRM & 7];
                    }
                    _state = _afterEA;
                    break;

                case stateIO:
                    if (_useMemory)
                        initIO(_afterIO, _ioType, _wordSize);
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
                    _address = sp();
                    _segment = 2;
                    _wait = 6;
                    initIO(stateEndInstruction, ioWrite, true);
                    break;
                case statePop:
                    _address = sp();
                    sp() += 2;
                    _segment = 2;
                    initIO(_afterIO, ioRead, true);
                    break;


                // alu reg<->regmem

                case stateALU: readEA(stateALU2); break;
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

                case stateALUAccumImm: fetch(stateALUAccumImm2, _wordSize); break;
                case stateALUAccumImm2:
                    _destination = getAccum();
                    _source = _data;
                    _aluOperation = (_opcode >> 3) & 7;
                    doALUOperation();
                    if (_aluOperation != 7)
                        setAccum();
                    end(4);
                    break;


                // PUSH segreg

                case statePushSegReg: push(_segmentRegisters[_opcode >> 3]); break;


                // POP segreg

                case statePopSegReg: pop(statePopSegReg2); break;
                case statePopSegReg2: _segmentRegisters[_opcode >> 3] = _data; end(0); break;


                // segreg:

                case stateSegOverride: _segmentOverride = (_opcode >> 3) & 3; _state = stateFetch; _wait = 2; break;


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
                    end(4);
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
                    end(8);
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
                    doAF();
                    setPZS();
                    rw() = _data;
                    end(4);
                    break;
                case statePushRW: push(rw()); break;
                case statePopRW: pop(statePopRW2); break;
                case statePopRW2: rw() = _data; end(0); break;


                // Invalid opcodes

                case stateInvalid60: end(0); break;


                // JCond cb

                case stateJCond: fetch(stateJCond2, false); break;
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
                        if (jump)
                            setIP(_ip + signExtend(_data));
                        end(jump ? 16 : 4);
                    }
                    break;


                // alu regmem, imm

                case stateALURMImm: readEA(stateALURMImm2); break;
                case stateALURMImm2: _destination = _data; fetch(stateALURMImm3, _opcode == 0x81); break;
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
                    if (_aluOperation != 7)
                        writeEA(stateEndInstruction);
                    else
                        _state = stateEndInstruction;
                    break;


                // TEST regmem, reg

                case stateTestRMReg: readEA(stateTestRMReg2); break;
                case stateTestRMReg2:
                    _destination = _data;
                    _source = getReg();
                    bitwise(_destination & _source);
                    end(_useMemory ? 5 : 3);
                    break;
                        
                   
                // XCHG regmem, reg

                case stateXchgRMReg: readEA(stateXchgRMReg2); break;
                case stateXchgRMReg2:
                    _source = getReg();
                    setReg(_data);
                    _data = _source;
                    _wait = (_useMemory ? 9 : 4);
                    writeEA(stateEndInstruction);
                    break;


                // MOV regmem, reg
                       
                case stateMovRMReg: loadEA(stateMovRMReg2); break;
                case stateMovRMReg2:
                    _data = getReg();
                    _wait = (_useMemory ? 5 : 2);
                    writeEA(stateEndInstruction);
                    break;


                // MOV reg, regmem

                case stateMovRegRM: readEA(stateMovRegRM2); break;
                case stateMovRegRM2: setReg(_data); end(_useMemory ? 4 : 2); break;


                // MOV rmw, segreg

                case stateMovRMWSegReg: loadEA(stateMovRMWSegReg2); break;
                case stateMovRMWSegReg2:
                    _wordSize = true;
                    _data = _segmentRegisters[modRMReg() & 3];
                    _wait = (_useMemory ? 5 : 2);
                    writeEA(stateEndInstruction);
                    break;


                // LEA rw, mem

                case stateLEA: loadEA(stateLEA2); break;
                case stateLEA2: setReg(_address); end(2); break;


                // MOV segreg, rmw

                case stateMovSegRegRMW: _wordSize = true; readEA(stateMovSegRegRMW2); break;
                case stateMovSegRegRMW2: _segmentRegisters[modRMReg() & 3] = _data; end(_useMemory ? 4 : 2); break;


                // POP mw

                case statePopMW:
                    _afterIO = statePopMW2;
                    loadEA(statePop);
                    break;
                case statePopMW2:
                    _wait = 1;
                    writeEA(stateEndInstruction);
                    break;


                // XCHG AX,rw

                case stateXchgAxRW: _data = rw(); rw() = ax(); ax() = _data; end(3); break;
                    

                // CBW

                case stateCBW: ax() = signExtend(al()); end(2); break;


                // CWD

                case stateCWD: dx() = ((ax() & 0x8000) == 0 ? 0x0000 : 0xffff); end(5); break;


                // CALL cp

                case stateCallCP: fetch(stateCallCP2, true); break;
                case stateCallCP2: _savedIP = _data; fetch(stateCallCP3, true); break;
                case stateCallCP3: _savedCS = _data;
                    _afterIO = stateCallCP4;
                    _state = statePush;
                    _data = _segmentRegisters[1];
                    break;
                case stateCallCP4:
                    _afterIO = stateCallCP5;
                    _state = statePush;
                    _data = _ip;
                    break;
                case stateCallCP5:
                    _segmentRegisters[1] = _savedCS;
                    setIP(_savedIP);
                    end(12);
                    break;


                // WAIT

                case stateWait: end(4); break;


                // PUSHF

                case statePushF: push(_flags & 0x0fd7, 0); break;


                // POPF

                case statePopF: pop(statePopF2); break;
                case statePopF2: _flags = _data | 2; end(4); break;


                // SAHF

                case stateSAHF: _flags = (_flags & 0xff02) | ah(); end(4); break;


                // LAHF

                case stateLAHF: ah() = _flags & 0xd7; end(4); break;


                // MOV accum, [iw]

                case stateMovAccumInd: fetch(stateMovAccumInd2, true); break;
                case stateMovAccumInd2: _address = _data; initIO(stateMovAccumInd3, ioRead, _wordSize); break;
                case stateMovAccumInd3: setAccum(); end(6); break;


                // MOV [iw], accum

                case stateMovIndAccum: fetch(stateMovIndAccum2, true); break;
                case stateMovIndAccum2:
                    _address = _data;
                    _data = getAccum();
                    _wait = 6;
                    initIO(stateEndInstruction, ioWrite, _wordSize);
                    break;


                // MOVSB/MOVSW

                case stateMovS:
                    _address = si();
                    _segment = 3;
                    initIO(stateMovS, ioRead, _wordSize);

                    break;
                //void oA4() { /* TODO: MOVS */ }

                case stateCmpS:
                //void oA6() { /* TODO: CMPS */ }

                case stateTestAccumImm:
                //void oA8() { /* TODO: TEST accum,imm */ }

                case stateStoS:
                //void oAA() { /* TODO: STOS */ }

                case stateLodS:
                //void oAC() { /* TODO: LODS */ }

                case stateScaS:
                //void oAE() { /* TODO: SCAS */ }

                case stateMovRegImm:
                //void oB0() { /* TODO: MOV reg,imm */ _wait = 4; }

                case stateInvalidC0:
                //void oC0() { /* TODO: invalid */ }

                case stateRet:
                //void oC2() { /* TODO: RET/RETF */ }

                case stateLoadFar:
                //void oC4() { /* TODO: Lsegreg rw,m */ _state = 4; }

                case stateMovRMImm:
                //void oC6() { /* TODO: MOV  rm,imm */ _state = 4; }

                case stateInt:
                //void oCC() { /* TODO: INT  3 */ }
                //void oCD() { /* TODO: INT  ib */ }

                case stateIntO:
                //void oCE() { /* TODO: INTO */ }

                case stateIRet:
                //void oCF() { /* TODO: IRET */ }

                case stateShift:
                //void oD0() { /* TODO: shift */ _state = 4; }

                case stateAAM:
                //void oD4() { /* TODO: AAM  ib */ }

                case stateAAD:
                //void oD5() { /* TODO: AAD  ib */ }

                case stateSALC:
                    al() = cf() ? 0xff : 0x00;
                    _wait = 4;
                    _state = stateEndInstruction;
                    break;

                case stateXlatB:
                //void oD7() { /* TODO: XLATB */ }

                case stateEscape:
                //void oD8() { /* TODO: ESC */ _wait = 2; _state = 4; }

                case stateLoop:
                //void oE0() { /* TODO: loop cb */ }

                case stateIn:
                case stateOut:
                //void oE4() { /* TODO: IN/OUT */ }

                case stateCallCW:
                //void oE8() { /* TODO: CALL cw */ }
                case stateJmpCW:
                //void oE9() { /* TODO: JMP  cw */ }
                case stateJmpCP:
                //void oEA() { /* TODO: JMP  cp */ }
                case stateJmpCb:
                //void oEB() { /* TODO: JMP  cb */ }

                case stateLock:
                //void oF0() { /* TODO: LOCK */ _wait = 2; }
                case stateInvalidF1:
                //void oF1() { /* TODO: invalid */ }
                case stateRep:
                //void oF2() { /* TODO: REPNE/REP */ _wait = 2; }
                case stateHlt:
                //void oF4() { /* TODO: HLT */ _wait = 2; }


                // CMC

                case stateCmC:
                    _flags ^= 1;
                    _wait = 2;
                    _state = stateEndInstruction;
                    break;


                case stateMath:
                //void oF6() { /* TODO: misc1 */ _state = 4; }


                // CLC/STC

                case stateCLC:
                case stateSTC:
                    _flags = (_flags & 0xfffe) | (_opcode & 1);
                    _wait = 2;
                    _state = stateEndInstruction;
                    break;


                // CLI/STI

                case stateCLI:
                case stateSTI:
                    _flags = (_flags & 0xfdff) | ((_opcode & 1) << 9);
                    _wait = 2;
                    _state = stateEndInstruction;
                    break;


                // CLD/STD

                case stateCLD:
                case stateSTD:
                    _flags = (_flags & 0xfbff) | ((_opcode & 1) << 10);
                    _wait = 2; 
                    _state = stateEndInstruction;
                    break;

                case stateMisc:
                //void oFE() { /* TODO: misc2 */ _state = 4; }
            }
        } while (true);
    }
private:
    enum IOType
    {
        ioNone,
        ioRead,
        ioWrite,
        ioInstructionFetch
    };
    enum State
    {
        stateWaitingForBIU,

        stateFetch,
        stateFetch2,
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

        stateCallCP,
        stateCallCP2,
        stateCallCP3,
        stateCallCP4,
        stateCallCP5,

        stateWait,

        statePushF,
        statePopF,
        statePopF2,
        stateSAHF,
        stateLAHF,

        stateMovAccumInd,
        stateMovAccumInd2,
        stateMovAccumInd3,
        stateMovIndAccum,
        stateMovIndAccum2,

        stateMovS,
        stateCmpS,

        stateTestAccumImm,

        stateStoS,
        stateLodS,
        stateScaS,

        stateMovRegImm,

        stateInvalidC0,

        stateRet,

        stateLoadFar,

        stateMovRMImm,

        stateInt,
        stateIntO,
        stateIRet,

        stateShift,

        stateAAM,
        stateAAD,
        stateSALC,
        stateXlatB,

        stateEscape,

        stateLoop,

        stateIn,
        stateOut,

        stateCallCW,
        stateJmpCW,
        stateJmpCP,
        stateJmpCb,

        stateLock,
        stateInvalidF1,
        stateRep,
        stateHlt,
        stateCmC,
        stateMath,
        stateCLC,
        stateSTC,
        stateCLI,
        stateSTI,
        stateCLD,
        stateSTD,
        stateMisc
    };
    enum BusState
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

    void end(int wait) { _wait = wait; _state = stateEndInstruction); }
    void push(UInt16 data, int wait = 1)
    {
        _data = data;
        _wait = wait;
        _state = statePush;
    }
    void pop(State state) { _afterIO = state; _state = statePop; }
    void loadEA(State state) { _afterEA = state; _state = stateModRM; }
    void readEA(State state) { _afterIO = state; _ioType = ioRead; loadEA(stateIO); }
    void fetch(State state, bool wordSize) { initIO(state, ioInstructionFetch, wordSize); }
    void writeEA(State state) { _afterIO = state; _ioType = ioWrite; _state = stateIO; }
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
    void bitwise(UInt16 data)
    {
        _data = data;
        clearCAO();
        setPZS();
    }
    void doAF() { setAF(((_data ^ _source ^ _destination) & 0x10) != 0); }
    void setCAPZS()
    {
        setPZS();
        doAF();
        setCF((_data & (!_wordSize ? 0x100 : 0x10000)) != 0);
    }
    void setOFAdd()
    {
        UInt16 t = (_data ^ _source) & (_data ^ _destination);
        setOF((t & (!_wordSize ? 0x80 : 0x8000)) != 0);
    }
    void add()
    {
        _data = _destination + _source;
        setCAPZS();
        setOFAdd();
    }
    void setOFSub()
    {
        UInt16 t = (_destination ^ _source) & (_data ^ _destination);
        setOF((t & (!_wordSize ? 0x80 : 0x8000)) != 0);
    }
    void sub()
    {
        _data = _destination - _source;
        setCAPZS();
        setOFSub();
    }

    void doALUOperation()
    {
        switch (_aluOperation) {
            case 0: add(); break;
            case 1: bitwise(_destination | _source); break;
            case 2: _source += cf() ? 1 : 0; add(); break;
            case 3: _source += cf() ? 1 : 0; sub(); break;
            case 4: bitwise(_destination & _source); break;
            case 5: 
            case 7: sub(); break;
            case 6: bitwise(_destination ^ _source); break;
        }
    }

    UInt16 signExtend(UInt8 data) { return data + (data < 0x80 ? 0 : 0xff00); }

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

    void initIO(State nextState, IOType ioType, bool wordSize)
    {
        _state = stateWaitingForBIU;
        _afterIO = nextState;
        _ioRequested = ioType;
        _byte = (!wordSize ? ioSingleByte : ioWordFirst);
        if (ioType == ioInstructionFetch)
            completeInstructionFetch();
    }
    UInt16 getIP() { return _ip; }
    void setIP(UInt16 value) { _ip = value; _abandonFetch = true; }

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
                _data = getInstructionByte();
                _state = _afterIO;
                ++_ip;
            }
        }
        else {
            if (_prefetched > 1) {
                _data = getInstructionByte();
                _data |= static_cast<UInt16>(getInstructionByte()) << 8;
                _ioRequested = ioNone;
                _state = _afterIO;
                _ip += 2;
            }
        }
    }

    UInt16 _registers[8];      /* AX CX DX BX SP BP SI DI */
    UInt16 _flags;
        //   0: CF = unsigned overflow?
        //   1:  1
        //   2: PF = parity: even number of 1 bits in result?
        //   3:  0
        //   4: AF = unsigned overflow for low nybble
        //   5:  0
        //   6: ZF = zero result?
        //   7: SF = result is negative?
        //   8: TF = interrupt after every instruction?
        //   9: IF = interrupts enabled?
        //  10: DF = SI/DI decrement in string operations
        //  11: OF = signed overflow?
    UInt16 _segmentRegisters[4];  /* ES CS SS DS */
    UInt16 _ip;
    UInt8 _prefetchQueue[4];
    UInt8 _prefetchOffset;
    UInt8 _prefetched;
    int _segment;
    int _segmentOverride;
    UInt16 _data;
    Array<UInt8> _memory;
    UInt16 _prefetchAddress;
    IOType _ioType;
    IOType _ioRequested;
    IOType _ioInProgress;
    BusState _busState;
    IOByte _byte;
    bool _abandonFetch;
    int _wait;
    State _state;
    UInt8 _opcode;
    UInt8 _modRM;
    UInt32 _data;
    UInt32 _source;
    UInt32 _destination;
    UInt16 _address;
    bool _useMemory;
    bool _wordSize;
    int _aluOperation;
    State _afterEA;
    State _afterIO;
    bool _sourceIsRM;
    UInt16 _savedCS;
    UInt16 _savedIP;
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
