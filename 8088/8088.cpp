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
        _state(stateFetch),
        _ip(0xfff0),
        _prefetchOffset(0),
        _prefetched(0),
        _segmentOverride(-1),
        _prefetchAddress(_ip),
        _ioRequested(ioNone),
        _ioInProgress(ioInstructionFetch),
        _busState(t1),
        _abandonFetch(false),
        _useIO(false)
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

                case stateFetch: fetch(stateFetch2, false); break;
                case stateFetch2:
                    {
                        _opcode = _data;
                        _wordSize = ((_opcode & 1) != 0);
                        _sourceIsRM = ((_opcode & 2) != 0);
                        static State stateForOpcode[0x100] = {
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
                            stateInvalid,      stateInvalid,      stateInvalid,      stateInvalid,      stateInvalid,      stateInvalid,      stateInvalid,      stateInvalid,
                            stateInvalid,      stateInvalid,      stateInvalid,      stateInvalid,      stateInvalid,      stateInvalid,      stateInvalid,      stateInvalid,
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
                            stateInvalid,      stateInvalid,      stateRet,          stateRet,          stateLoadFar,      stateLoadFar,      stateMovRMImm,     stateMovRMImm,
                            stateInvalid,      stateInvalid,      stateRet,          stateRet,          stateInt,          stateInt,          stateIntO,         stateIRet,
                            stateShift,        stateShift,        stateShift,        stateShift,        stateAAM,          stateAAD,          stateSALC,         stateXlatB,
                            stateEscape,       stateEscape,       stateEscape,       stateEscape,       stateEscape,       stateEscape,       stateEscape,       stateEscape,
                            stateLoop,         stateLoop,         stateLoop,         stateLoop,         stateIn,           stateIn,           stateOut,          stateOut,
                            stateCallCW,       stateJmpCW,        stateJmpCP,        stateJmpCb,        stateIn,           stateIn,           stateOut,          stateOut,
                            stateLock,         stateInvalid,      stateRep,          stateRep,          stateHlt,          stateCmC,          stateMath,         stateMath,
                            stateLoadC,        stateLoadC,        stateLoadI,        stateLoadI,        stateLoadD,        stateLoadD,        stateMisc,         stateMisc};
                        _state = stateForOpcode[_opcode];
                    }
                    break;
                case stateEndInstruction:
                    _segmentOverride = -1;
                    _state = stateFetch;
                    _rep = 0;
                    _useIO = false;
                    break;

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
                    initIO(_afterIO, ioWrite, true);
                    break;
                case statePop:
                    _address = sp();
                    sp() += 2;
                    _segment = 2;
                    initIO(_afterIO, ioRead, true);
                    break;

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
                        if (!_sourceIsRM)
                            writeEA(_data, 0);
                        else {
                            setReg(_data);
                            _state = stateEndInstruction;
                        }
                    }
                    else
                        _state = stateEndInstruction;
                    break;

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

                case statePushSegReg: push(_segmentRegisters[_opcode >> 3]); break;

                case statePopSegReg: pop(statePopSegReg2); break;
                case statePopSegReg2: _segmentRegisters[_opcode >> 3] = _data; end(4); break;

                case stateSegOverride: _segmentOverride = (_opcode >> 3) & 3; _state = stateFetch; _wait = 2; break;

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
                    _state = stateAA;
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
                    end(4);
                    break;

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
                    end(2);
                    break;
                case statePushRW: push(rw()); _wait += 1; break;
                case statePopRW: pop(statePopRW2); break;
                case statePopRW2: rw() = _data; end(4); break;

                case stateInvalid: end(0); break;

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
                            case 0x7c: jump = (sf() != of()); break;
                            case 0x7e: jump = (sf() != of()) ||  zf(); break;
                        }
                        if ((_opcode & 1) != 0)
                            jump = !jump;
                        if (jump)
                            jumpShort();
                        end(jump ? 16 : 4);
                    }
                    break;

                case stateALURMImm: readEA(stateALURMImm2); break;
                case stateALURMImm2: _destination = _data; fetch(stateALURMImm3, _opcode == 0x81); break;
                case stateALURMImm3:
                    if (_opcode != 0x83)
                        _source = _data;
                    else
                        _source = signExtend(_data);
                    _aluOperation = modRMReg();
                    _wait = 9;
                    if (_aluOperation == 7)
                        _wait = 6;
                    if (!_useMemory)
                        _wait = 4;
                    doALUOperation();
                    if (_aluOperation != 7)
                        writeEA(stateEndInstruction);
                    else
                        _state = stateEndInstruction;
                    break;

                case stateTestRMReg: readEA(stateTestRMReg2); break;
                case stateTestRMReg2: test(_data, getReg()); end(_useMemory ? 5 : 3); break;
                case stateXchgRMReg: readEA(stateXchgRMReg2); break;
                case stateXchgRMReg2: _source = getReg(); setReg(_data); writeEA(_source, _useMemory ? 9 : 4); break;
                case stateMovRMReg: loadEA(stateMovRMReg2); break;
                case stateMovRMReg2: writeEA(getReg(), _useMemory ? 5 : 2); break;
                case stateMovRegRM: readEA(stateMovRegRM2); break;
                case stateMovRegRM2: setReg(_data); end(_useMemory ? 4 : 2); break;
                case stateMovRMWSegReg: _wordSize = true; loadEA(stateMovRMWSegReg2); break;
                case stateMovRMWSegReg2: writeEA(_segmentRegisters[modRMReg() & 3], _useMemory ? 5 : 2); break;
                case stateLEA: loadEA(stateLEA2); break;
                case stateLEA2: setReg(_address); end(2); break;
                case stateMovSegRegRMW: _wordSize = true; readEA(stateMovSegRegRMW2); break;
                case stateMovSegRegRMW2: _segmentRegisters[modRMReg() & 3] = _data; end(_useMemory ? 4 : 2); break;
                case statePopMW: _afterIO = statePopMW2; loadEA(statePop); break;
                case statePopMW2: writeEA(_data, _useMemory ? 9 : 12); break;
                case stateXchgAxRW: _data = rw(); rw() = ax(); ax() = _data; end(3); break;
                case stateCBW: ax() = signExtend(al()); end(2); break;
                case stateCWD: dx() = ((ax() & 0x8000) == 0 ? 0x0000 : 0xffff); end(5); break;
                case stateCallCP: fetch(stateCallCP2, true); break;
                case stateCallCP2: _savedIP = _data; fetch(stateCallCP3, true); break;
                case stateCallCP3: _savedCS = _data; _wait = 2; push(cs(), stateCallCP4); break;
                case stateCallCP4: push(_ip, stateJmpCP4); break;
                case stateWait: end(4); break;
                case statePushF: push(_flags & 0x0fd7); break;
                case statePopF: pop(statePopF2); break;
                case statePopF2: _flags = _data | 2; end(4); break;
                case stateSAHF: _flags = (_flags & 0xff02) | ah(); end(4); break;
                case stateLAHF: ah() = _flags & 0xd7; end(4); break;

                case stateMovAccumInd: fetch(stateMovAccumInd2, true); break;
                case stateMovAccumInd2: _address = _data; initIO(stateMovAccumInd3, ioRead, _wordSize); break;
                case stateMovAccumInd3: setAccum(); end(6); break;
                case stateMovIndAccum: fetch(stateMovIndAccum2, true); break;
                case stateMovIndAccum2: _address = _data; _data = getAccum(); _wait = 6; initIO(stateEndInstruction, ioWrite, _wordSize); break;

                case stateMovS: _wait = (_rep != 0 ? 9 : 10); lodS(stateMovS2); break;
                case stateMovS2: _afterRep = stateMovS; stoS(stateRepAction); break;
                case stateRepAction:
                    _state = stateEndInstruction;
                    if (_rep != 0) {
                        --cx();
                        if (cx() == 0 || (zf() == (_rep == 1)))
                            _state = _afterRep;
                    }
                    break;
                case stateCmpS: _wait = 14; lodS(stateCmpS2); break;
                case stateCmpS2: _destination = _data; lodDIS(stateCmpS3); break;
                case stateCmpS3: _source = _data; sub(); _afterRep = stateCmpS; _state = stateRepAction; break;

                case stateTestAccumImm: fetch(stateTestAccumImm2, _wordSize); break;
                case stateTestAccumImm2: test(getAccum(), _data); end(4); break;

                case stateStoS: _wait = (_rep != 0 ? 6 : 7); _data = getAccum(); _afterRep = stateStoS; stoS(stateRepAction); break;
                case stateLodS: _wait = (_rep != 0 ? 9 : 8); lodS(stateLodS2); break;
                case stateLodS2: setAccum(); _afterRep = stateLodS; _state = stateRepAction; break;
                case stateScaS: _wait = 11; lodDIS(stateScaS2); break;
                case stateScaS2: _destination = getAccum(); _source = _data; sub(); _afterRep = stateScaS; _state = stateRepAction; break;

                case stateMovRegImm: _wordSize = ((_opcode & 8) != 0); fetch(stateMovRegImm2, _wordSize); break;
                case stateMovRegImm2: setAccum(); end(4); break;

                case stateRet: pop(stateRet2); break;
                case stateRet2:
                    _savedIP = _data;
                    if ((_opcode & 8) == 0) {
                        _savedCS = _segmentRegisters[1];
                        _state = stateRet4;
                        _wait = (!_wordSize ? 16 : 12);
                    }
                    else {
                        pop(stateRet3);
                        _wait = (!_wordSize ? 17 : 18);
                    }
                    break;
                case stateRet3:
                    _savedCS = _data;
                    _state = stateRet4;
                    break;
                case stateRet4:
                    if (!_wordSize)
                        fetch(stateRet5, true);
                    else
                        _state = stateRet6;
                    break;
                case stateRet5:
                    sp() += _data;
                    _state = stateRet6;
                    break;
                case stateRet6:
                    _segmentRegisters[1] = _savedCS;
                    setIP(_savedIP);
                    end(0);
                    break;

                case stateLoadFar: readEA(stateLoadFar2); break;
                case stateLoadFar2: setReg(_data); _segment[_afterIO = stateLoadFar3; _state = stateLoadFar3; break;
                case stateLoadFar3: _segmentRegisters[!_wordSize ? 0 : 3] = _data; end(8); break;

                case stateMovRMImm: loadEA(stateMovRMImm2); break;
                case stateMovRMImm2: fetch(stateMovRMImm2, _wordSize); break;
                case stateMovRMImm3: writeEA(_data, _useMemory ? 6 : 4); break;

                case stateInt: interrupt(3); _wait = 1; break;
                case stateInt3: fetch(stateIntAction, false); break;
                case stateIntAction: _source = _data; push(_flags & 0x0fd7, stateIntAction2); break;
                case stateIntAction2: setIF(false); setTF(false); push(cs(), stateIntAction3); break;
                case stateIntAction3: push(_ip, stateIntAction4); break;
                case stateIntAction4:
                    _address = _source << 2;
                    _segment = 1;
                    cs() = 0;
                    initIO(stateIntAction5, ioRead, true);
                    break;
                case stateIntAction5:
                    _savedIP = _data;
                    _address += 2;
                    initIO(stateIntAction6, ioRead, true);
                    break;
                case stateIntAction6:
                    cs() = _data;
                    setIP(_savedIP);
                    _wait = 13;
                    break;  
                case stateIntO:
                    if (of()) {
                        interrupt(4);
                        _wait = 2;
                    }
                    else
                        end(4);
                    break;
                case stateIRet: pop(stateIRet2); break;
                case stateIRet2: _savedIP = _data; pop(stateIRet3); break;
                case stateIRet3: _savedCS = _data; pop(stateIRet4); break;
                case stateIRet4: _flags = _data | 2; cs() = _savedCS; setIP(_savedIP); _wait = 20; break;

                case stateShift: readEA(stateShift2); break;
                case stateShift2:
                    if ((_opcode & 2) == 0) {
                        _source = 1;
                        _wait = (_useMemory ? 7 : 2);
                    }
                    else {
                        _source = cl();
                        _wait = (_useMemory ? 12 : 8);
                    }
                    _state = stateShift3;
                    break;
                case stateShift3:
                    if (_source == 0) {
                        writeEA(_data, 0);
                        break;
                    }
                    _destination = _data;
                    switch (modRMReg()) {
                        case 0:  // ROL
                            _data <<= 1;
                            doCF();
                            _data |= (cf() ? 1 : 0);
                            setOFRotate();
                            break;
                        case 1:  // ROR
                            setCF((_data & 1) != 0);
                            _data >>= 1;
                            if (cf())
                                _data |= (!_wordSize ? 0x80 : 0x8000);
                            setOFRotate();
                            break;
                        case 2:  // RCL
                            _data = (_data << 1) | (cf() ? 1 : 0);
                            doCF();
                            setOFRotate();
                            break;
                        case 3:  // RCR
                            _data >>= 1;
                            if (cf())
                                _data |= (!_wordSize ? 0x80 : 0x8000);
                            setCF((_destination & 1) != 0);
                            setOFRotate();
                            break;
                        case 4:  // SHL
                        case 6:
                            _data <<= 1;
                            doCF();
                            setOFRotate();
                            setPZS();
                            break;
                        case 5:  // SHR
                            setCF((_data & 1) != 0);
                            _data >>= 1;
                            setOFRotate();
                            setAF(true);
                            setPZS();
                            break;
                        case 7:  // SAR
                            setCF((_data & 1) != 0);
                            _data >>= 1;
                            if (!_wordSize)
                                _data |= (_destination & 0x80);
                            else
                                _data |= (_destination & 0x8000);
                            setOFRotate();
                            setAF(true);
                            setPZS();
                            break;
                    }
                    if ((_opcode & 2) != 0)
                        _wait = 4;
                    --_source;
                    break;

                case stateAAM: fetch(stateAAM2, false); break;
                case stateAAM2:
                    if (_data == 0)
                        interrupt(0);
                    else {
                        ah() = al() / _data;
                        al() %= _data;
                        _wordSize = true;
                        setPZS();
                        end(83);
                    }
                    break;
                case stateAAD: fetch(stateAAD2, false); break;
                case stateAAD2:
                    al() += ah()*_data;
                    ah() = 0;
                    setPZS();
                    end(60);
                    break;

                case stateSALC: al() = (cf() ? 0xff : 0x00); end(4); break;

                case stateXlatB:
                    _address = bx() + al();
                    initIO(stateXlatB2, ioRead, false);
                    break;
                case stateXlatB2:
                    al() = _data;
                    end(7);
                    break;

                case stateEscape: loadEA(stateEscape2); break;
                case stateEscape2: end(2); break;

                case stateLoop: fetch(stateLoop2, false); break;
                case stateLoop2:
                    {
                        bool jump;
                        if (_opcode != 0xe3) {
                            _wait = 5;
                            --cx();
                            jump = (cx() != 0);
                            switch (_opcode) {
                                case 0xe0: if (zf()) jump = false; _wait = 6; break;
                                case 0xe1: if (!zf()) jump = false; break;
                            }
                        }
                        else {
                            _wait = 6;
                            jump = (cx() == 0);
                        }
                        if (jump) {
                            jumpShort();
                            _wait = (_opcode == 0xe0 ? 19 : (_opcode == 0xe2 ? 17 : 18));
                        }
                    }
                    end(_wait);
                    break;

                case stateInOut:
                    if ((_opcode & 8) == 0) {
                        fetch(stateInOut2, false);
                        _wait = 2;
                    }
                    else {
                        _data = dx();
                        _state = stateInOut2;
                    }
                    break;
                case stateInOut2:
                    _useIO = true;
                    _ioType = ((_opcode & 2) == 0 ? ioRead : ioWrite);
                    initIO(stateInOut3, _ioType, _wordSize);
                    break;
                case stateInOut3:
                    if (_ioType == ioRead)
                        setAccum();
                    end(4);
                    break;

                case stateCallCW: fetch(stateCallCW2, true); break;
                case stateCallCW2: _savedIP = _data + _ip; _wait = 3; _state = stateCallCW3; break;
                case stateCallCW3: push(_ip, stateJmpCW3); break;

                case stateJmpCW: fetch(stateJmpCW2, true); break;
                case stateJmpCW2: _savedIP = _data + _ip; _wait = 9; _state = stateJmpCW3; break;
                case stateJmpCW3: setIP(_savedIP); end(6); break;

                case stateJmpCP: fetch(stateJmpCP2, true); break;
                case stateJmpCP2: _savedIP = _data; fetch(stateJmpCP3, true); break;
                case stateJmpCP3: _savedCS = _data; _wait = 9; _state = stateJmpCP4; break;
                case stateJmpCP4: cs() = _savedCS; _state = stateJmpCW3; break;

                case stateJmpCB: fetch(stateJmpCB2, false); break;
                case stateJmpCB2: jumpShort(); end(15); break;

                case stateLock: end(2); break;
                case stateRep: _rep = (_opcode == 0xf2 ? 1 : 2); _wait = 9; _state = stateFetch; break;
                case stateHlt: end(2); break;
                case stateCmC: _flags ^= 1; end(2); break;

                case stateMath: readEA(stateMath2); break;
                case stateMath2:
                    if ((modRMReg() & 6) == 0) {
                        _destination = _data;
                        fetch(stateMath3, _wordSize);
                    }
                    else
                        _state = stateMath3;
                    break;
                case stateMath3:
                    switch (modRMReg()) {
                        case 0:
                        case 1:  // TEST
                            test(_destination, _data);
                            end(_useMemory ? 7 : 5);
                            break;
                        case 2:  // NOT
                            writeEA(~_data, _useMemory ? 8 : 3);
                            break;
                        case 3:  // NEG
                            _source = _data;
                            _destination = 0;
                            sub();
                            writeEA(_data, _useMemory ? 8 : 3);
                            break;
                        case 4:  // MUL
                        case 5:  // IMUL
                            _source = _data;
                            _destination = getAccum();
                            _data = _destination;
                            setSF();
                            setPF();
                            _data *= _source;
                            ax() = _data;
                            if (!_wordSize)
                                if (modRMReg() == 4) {
                                    setCF(ah() != 0);
                                    _wait = 70;
                                }
                                else {
                                    if ((_source & 0x80) != 0)
                                        ah() += _destination;
                                    if ((_destination & 0x80) != 0)
                                        ah() += _source;
                                    setCF(ah() == ((al() & 0x80) == 0 ? 0 : 0xff));
                                    _wait = 80;
                                }
                            else
                                if (modRMReg() == 4) {
                                    dx() = _data >> 16;
                                    _data |= dx();
                                    setCF(dx() != 0);
                                    _wait = 118;
                                }
                                else {
                                    dx() = _data >> 16;
                                    if ((_source & 0x8000) != 0)
                                        dx() += _destination;
                                    if ((_destination & 0x8000) != 0)
                                        dx() += _source;
                                    _data |= dx();
                                    setCF(dx() == ((ax() & 0x8000) == 0 ? 0 : 0xffff));
                                    _wait = 128;
                                }
                            setZF();
                            setOF(cf());
                            if (_useMemory)
                                _wait += 2;
                            _state = stateEndInstruction;
                            break;
                        case 6:  // DIV
                        case 7:  // IDIV
                            _source = _data;
                            if (_source == 0) {
                                interrupt(0);
                                break;
                            }
                            if (!_wordSize) {
                                _destination = ax();
                                if (modRMReg() == 4) {
                                    div();
                                    if (_data > 0xff) {
                                        interrupt(0);
                                        break;
                                    }
                                    _wait = 80;
                                }
                                else {
                                    _destination = ax();
                                    if ((_destination & 0x8000) != 0)
                                        _destination |= 0xffff0000;
                                    _source = signExtend(_source);
                                    div();
                                    if (_data > 0x7f && _data < 0xffffff80) {
                                        interrupt(0);
                                        break;
                                    }
                                    _wait = 101;
                                }
                                ah() = _remainder;
                                al() = _data;
                            }
                            else {
                                _destination = (dx() << 16) + ax();
                                div();
                                if (modRMReg() == 4) {
                                    if (_data > 0xffff) {
                                        interrupt(0);
                                        break;
                                    }
                                    _wait = 144;
                                }
                                else {
                                if (_data > 0x7fff && _data <  0xffff8000) {
                                    interrupt(0);
                                    break;
                                }
                                _wait = 165;
                                }
                                dx() = _remainder;
                                ax() = _data;
                            }
                            if (_useMemory)
                                _wait += 2;
                            _state = stateEndInstruction;
                            break;
                    }
                    break;

                case stateLoadC: setCF(_wordSize); end(2); break;
                case stateLoadI: setIF(_wordSize); end(2); break;
                case stateLoadD: setDF(_wordSize); end(2); break;

                case stateMisc: readEA(stateMisc2); break;
                case stateMisc2:
                    _savedIP = _data;
                    switch (modRMReg()) {
                        case 0: 
                        case 1:
                            _destination = _data;
                            _source = 1;
                            if (modRMReg() == 0) {
                                _data = _destination + _source;
                                setOFAdd();
                            }
                            else {
                                _data = _destination - _source;
                                setOFSub();
                            }
                            doAF();
                            setPZS();
                            writeEA(_data, _useMemory ? 7 : 3);
                            break;
                        case 2:
                            _wait = (_useMemory ? 1 : 0);
                            _state = stateCallCW3;
                            break;
                        case 3:
                            if (_useMemory) {
                                _address += 2;
                                _wait = 1;
                                readEA(stateCallCP3);
                            }
                            else
                                end(0);
                            break;
                        case 4:
                            _wait = (_useMemory ? 8 : 5);
                            _state = stateJmpCW3;
                            break;
                        case 5:
                            if (_useMemory) {
                                _address += 2;
                                _wait = 1;
                                readEA(stateJmpCP3);
                            }
                            else
                                end(0);
                            break;
                        case 6:
                            push(_data, stateEndInstruction);
                            _wait += (_useMemory ? 2 : 1);
                            break;
                        case 7:
                            end(0);
                            break;
                    }
                    break;
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
        stateFetch, stateFetch2,
        stateEndInstruction,
        stateModRM, stateModRM2,
        stateEAOffset,
        stateEARegisters,
        stateEAByte,
        stateEAWord,
        stateEASetSegment,
        stateIO,
        statePush,
        statePop,

        stateALU, stateALU2, stateALU3,
        stateALUAccumImm, stateALUAccumImm2,
        statePushSegReg,
        statePopSegReg, statePopSegReg2,
        stateSegOverride,
        stateDAA, stateDAS, stateDA, stateAAA, stateAAS, stateAA,
        stateIncDecRW, statePushRW, statePopRW, statePopRW2,
        stateInvalid,
        stateJCond, stateJCond2,
        stateALURMImm, stateALURMImm2, stateALURMImm3,
        stateTestRMReg, stateTestRMReg2,
        stateXchgRMReg, stateXchgRMReg2,
        stateMovRMReg, stateMovRMReg2,
        stateMovRegRM, stateMovRegRM2,
        stateMovRMWSegReg, stateMovRMWSegReg2,
        stateLEA, stateLEA2,
        stateMovSegRegRMW, stateMovSegRegRMW2,
        statePopMW, statePopMW2,
        stateXchgAxRW,
        stateCBW, stateCWD,
        stateCallCP, stateCallCP2, stateCallCP3, stateCallCP4, stateCallCP5,
        stateWait,
        statePushF, statePopF, statePopF2,
        stateSAHF, stateLAHF,
        stateMovAccumInd, stateMovAccumInd2, stateMovAccumInd3,
        stateMovIndAccum, stateMovIndAccum2,
        stateMovS, stateMovS2, stateRepAction,
        stateCmpS, stateCmpS2, stateCmpS3,
        stateTestAccumImm, stateTestAccumImm2,
        stateStoS,
        stateLodS, stateLodS2,
        stateScaS, stateScaS2,
        stateMovRegImm, stateMovRegImm2,
        stateRet, stateRet2, stateRet3, stateRet4, stateRet5, stateRet6,
        stateLoadFar, stateLoadFar2, stateLoadFar3,
        stateMovRMImm, stateMovRMImm2, stateMovRMImm3,
        stateInt, stateInt3, stateIntAction, stateIntAction2, stateIntAction3,
        stateIntAction4, stateIntAction5, stateIntAction6,
        stateIntO,
        stateIRet, stateIRet2, stateIRet3, stateIRet4,
        stateShift, stateShift2, stateShift3,
        stateAAM, stateAAM2,
        stateAAD, stateAAD2,
        stateSALC,
        stateXlatB, stateXlatB2,
        stateEscape, stateEscape2,
        stateLoop, stateLoop2,
        stateInOut, stateInOut2, stateInOut3,
        stateCallCW, stateCallCW2, stateCallCW3, stateCallCW4,
        stateJmpCW, stateJmpCW2, stateJmpCW3,
        stateJmpCP, stateJmpCP2, stateJmpCP3, stateJmpCP4,
        stateJmpCB, stateJmpCB2,
        stateLock, stateRep, stateHlt, stateCmC,
        stateMath, stateMath2, stateMath3,
        stateLoadC, stateLoadI, stateLoadD,
        stateMisc, stateMisc2
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
    void div()
    {
        bool negative = false;
        if (modRMReg() == 7) {
            if ((_destination & 0x80000000) != 0) {
                _destination = -_destination;
                negative = !negative;
            }
            if ((_source & 0x8000) != 0) {
                _source = (-_source) & 0xffff;
                negative = !negative;
            }
        }
        _data = _destination / _source;
        UInt32 product = _data * _source;
        // ISO C++ 2003 does not specify a rounding mode, but the x86 always
        // rounds towards zero.
        if (product > _destination) {
            --_data;
            product -= _source;
        }
        _remainder = _destination - product;
        if (negative) {
            _data = -_data;
            _remainder = -_remainder;
        }
    }
    void jumpShort() { setIP(_ip + signExtend(_data)); }
    void interrupt(UInt8 number) { _data = number; _state = stateIntAction; }
    void test(UInt16 destination, UInt16 source)
    {
        _destination = destination;
        _source = source;
        bitwise(_destination & _source);
    }
    int stringIncrement()
    {
        int r = (_wordSize ? 2 : 1);
        return !df() ? r : -r;
    }
    void lodS(State state)
    {
        _address = si();
        si() += stringIncrement();
        _segment = 3;
        initIO(state, ioRead, _wordSize); 
    }
    void lodDIS(State state)
    {
        _address = di();
        di() += stringIncrement();
        _segment = 0;
        initIO(state, ioRead, _wordSize);
    }
    void stoS(State state)
    {
        _address = di();
        di() += stringIncrement();
        _segment = 0;
        initIO(state, ioWrite, _wordSize);
    }
    void end(int wait) { _wait = wait; _state = stateEndInstruction; }
    void push(UInt16 data, State state = stateEndInstruction)
    {
        _data = data;
        _afterIO = state;
        _state = statePush;
        _wait += 6;
    }
    void pop(State state) { _afterIO = state; _state = statePop; }
    void loadEA(State state) { _afterEA = state; _state = stateModRM; }
    void readEA(State state)
    {
        _afterIO = state;
        _ioType = ioRead;
        loadEA(stateIO);
    }
    void fetch(State state, bool wordSize)
    {
        initIO(state, ioInstructionFetch, wordSize);
    }
    void writeEA(UInt16 data, int wait)
    {
        _data = data;
        _wait = wait;
        _afterIO = stateEndInstruction;
        _ioType = ioWrite;
        _state = stateIO;
    }
    void setCA() { setCF(true); setAF(true); }
    void clearCA() { setCF(false); setAF(false); }
    void clearCAO() { clearCA(); setOF(false); }
    void setPZS() { setPF(); setZF(); setSF(); }
    void bitwise(UInt16 data) { _data = data; clearCAO(); setPZS(); }
    void doAF() { setAF(((_data ^ _source ^ _destination) & 0x10) != 0); }
    void doCF() { setCF((_data & (!_wordSize ? 0x100 : 0x10000)) != 0); }
    void setCAPZS() { setPZS(); doAF(); doCF(); }
    void setOFAdd()
    {
        UInt16 t = (_data ^ _source) & (_data ^ _destination);
        setOF((t & (!_wordSize ? 0x80 : 0x8000)) != 0);
    }
    void add() { _data = _destination + _source; setCAPZS(); setOFAdd(); }
    void setOFSub()
    {
        UInt16 t = (_destination ^ _source) & (_data ^ _destination);
        setOF((t & (!_wordSize ? 0x80 : 0x8000)) != 0);
    }
    void sub() { _data = _destination - _source; setCAPZS(); setOFSub(); }
    void setOFRotate()
    {
        setOF(((_data ^ _destination) & (!_wordSize ? 0x80 : 0x8000)) != 0);
    }

    void doALUOperation()
    {
        switch (_aluOperation) {
            case 0: add(); break;
            case 1: bitwise(_destination | _source); break;
            case 2: _source += cf() ? 1 : 0; add(); break;
            case 3: _source += cf() ? 1 : 0; sub(); break;
            case 4: test(_destination, _source); break;
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
    UInt16& cs() { return _segmentRegisters[1]; }
    bool cf() { return (_flags & 1) != 0; }
    void setCF(bool cf) { _flags = (_flags & ~1) | (cf ? 1 : 0); }
    bool pf() { return (_flags & 4) != 0; }
    void setPF()
    {
        static UInt8 table[0x100] = {
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
    void setZF()
    { 
        _flags = (_flags & ~0x40) | 
            ((_data & (!_wordSize ? 0xff : 0xffff)) != 0 ? 0x40 : 0);
    }
    bool sf() { return (_flags & 0x80) != 0; }
    void setSF()
    {
        _flags = (_flags & ~0x80) |
            ((_data & (!_wordSize ? 0x80 : 0x8000)) != 0 ? 0x80 : 0);
    }
    bool tf() { return (_flags & 0x100) != 0; }
    void setTF(bool tf) { _flags = (_flags & ~0x100) | (tf ? 0x100 : 0); }
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
    void setReg(UInt16 value)
    { 
        if (!_wordSize)
            modRMRB() = value;
        else
            modRMRW() = value;
    }
    UInt8& byteRegister(int n) /* AL CL DL BL AH CH DH BH */
    {
        return *(reinterpret_cast<UInt8*>(
            &_registers[n & 3]) + (n >= 4 ? 1 : 0));
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
    UInt32 _remainder;
    UInt16 _address;
    bool _useMemory;
    bool _wordSize;
    int _aluOperation;
    State _afterEA;
    State _afterIO;
    State _afterRep;
    bool _sourceIsRM;
    UInt16 _savedCS;
    UInt16 _savedIP;
    int _rep;
    bool _useIO;
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
