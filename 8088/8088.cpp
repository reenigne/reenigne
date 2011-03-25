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
    class ExecutionUnit;
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
            _state(t1)
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
                                _prefetchQueue[(_prefetchOffset + _prefetched) & 3] =
                                    _memory[((_segmentRegisters[1] << 4) + _prefetchAddress) & 0xfffff];
                                ++_prefetched;
                                ++_prefetchAddress;
                                completeInstructionFetch();
                                break;
                        }
                        _state = tIdle;
                        break;
                    case tIdle:
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
        void setIP(UInt16 value) { _ip = value; }
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
        int _nextEUState;
        IOType _ioRequested;
        IOType _ioInProgress;
        State _state;
        IOByte _byte;
    };
    class ExecutionUnit
    {
        enum State
        {
            stateWaitingForBIU,

            stateFetchOpcode,
            stateOpcodeAvailable,
            stateEndInstruction,

            stateFetchModRm,
            stateModRmAvailable,
            stateEAOffset,
            stateEARegisters,
            stateEAByte,
            stateEAWord,
            stateEASetSegment,
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
                            static State stateForOpcode[256] = {
                                12, 12, 12, 12, 15, 15, 17, 18,
                                12, 12, 12, 12, 15, 15, 17, 18,
                                12, 12, 12, 12, 15, 15, 17, 18,
                                12, 12, 12, 12, 15, 15, 17, 18,
                                12, 12, 12, 12, 15, 15, 20,  0,
                                12, 12, 12, 12, 15, 15, 20,  0,
                                12, 12, 12, 12, 15, 15, 20,  0,
                                12, 12, 12, 12, 15, 15, 20,  0,
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

                    case stateFetchModRm:  // request mod R/M byte
                        initIO(stateModRmAvailable, ioInstructionFetch, false);
                        _ioData = _data;
                        break;
                    case stateModRmAvailable:
                        _useMemory = true;
                        switch (_modRm & 0xc0) {
                            case 0x00:
                                if ((_modRm & 7) == 6)
                                    initIO(stateEAOffset, ioInstructionFetch, true);
                                else
                                    _state = stateEARegisters;
                                break;
                            case 0x40:
                                initIO(stateEAByte, ioInstructionFetch, false);
                                break;
                            case 0x80:
                                initIO(stateEAWord, ioInstructionFetch, true);
                                break;
                            case 0xc0:
                                _useMemory = false;
                                registerIO(_modRm & 7);
                                _state = _opState;
                                break;
                        }
                        break;
                    case stateEAOffset:
                        _biu->setAddress(_data);
                        _biu->setSegment(3);
                        _wait = 6;
                        _state = 11;
                        break;
                    case stateEARegisters:
                        _biu->setAddress(eaDisplacement());
                        _state = stateEASetSegment;
                        break;
                    case stateEAByte:
                        _biu->setAddress(eaDisplacement() + signExtend(_data));
                        _wait += 4;
                        _state = stateEASetSegment;
                        break;
                    case stateEAWord:
                        _biu->setAddress(eaDisplacement() + _data);
                        _wait += 4;
                        _state = stateEASetSegment;
                        break;
                    case stateEASetSegment:
                        switch (_modRm & 7) {
                            case 0: _biu->setSegment(3);
                            case 1: _biu->setSegment(3);
                            case 2: _biu->setSegment(2);
                            case 3: _biu->setSegment(2);
                            case 4: _biu->setSegment(3);
                            case 5: _biu->setSegment(3);
                            case 6: _biu->setSegment(2);
                            case 7: _biu->setSegment(3);
                        }
                        _state = 11;
                        break;
                    case 11:
                        initIO(_opState, _ioType, wordSize());
                        break;
                    case 12:  // Initiate a second IO with the same address
                        if (_useMemory)
                            _state = 11;
                        else {
                            registerIO(_modRm & 7);
                            _state = _opState;
                        }
                        break;


                    // alu reg<->regmem

                    case 13:  // read register/memory
                        _opState = 14;
                        _state = stateRequestModRm;
                        _ioType = ioRead;
                        break;
                    case 14:  // register/memory data available
                        _wait = 5;
                        if ((_opcode & 2) == 0) {
                            _destination = _ioData;
                            _source = getReg();
                            _wait += 3;
                        }
                        else {
                            _destination = getReg();
                            _source = _ioData;
                        }
                        doALUOperation((_opcode >> 3) & 7);
                        if ((_opcode & 0x38) != 0x38) {
                            _ioType = ioWrite;
                            _ioData = _result;
                            _opState = stateEndInstruction;
                            _state = 12;
                        }
                        else
                            _state = stateEndInstruction;
                        break;


                    // alu accum, imm

                    case 15:  // fetch imm
                        _wait = 4;
                        initIO(16, ioInstructionFetch, wordSize());
                        break;
                    case 16:  // imm available
                        _destination = getAccum();
                        _source = _data;
                        doALUOperation((_opcode >> 3) & 7);
                        if ((_opcode & 0x38) != 0x38) 
                            setAccum();
                        _state = stateEndInstruction;
                        break;


                    // PUSH segreg

                    case 17:
                        sp() -= 2;
                        _biu->setAddress(sp());
                        _biu->setSegment(2);
                        _wait = 6;
                        _ioData = _biu->getSegmentRegister(_opcode >> 3);
                        initIO(stateEndInstruction, ioWrite, true);
                        break;

                    
                    // POP segreg

                    case 18:
                        _biu->setAddress(sp());
                        sp() += 2;
                        _biu->setSegment(2);
                        initIO(19, ioRead, true);
                        break;
                    case 19:
                        _biu->setSegmentRegister(_opcode >> 3, _data);
                        _state = stateEndInstruction;
                        break;

                    
                    // segreg:

                    case 20:
                        _biu->setSegmentOverride((_opcode >> 3) & 3);
                        _state = stateFetchOpcode;
                        _wait = 2;
                        break;

                    //void o27() { /* TODO: DAA */ }
                    //void o2F() { /* TODO: DAS */ }
                    //void o37() { /* TODO: AAA */ }
                    //void o3F() { /* TODO: AAS */ }
                    //void o40() { if ((_opcode & 0x08) == 0) ++rw(); else --rw(); _wait = 3; /* TODO: flags */ }
                    //void o50() { /* PUSH rw */ push(rw()); _wait = 15; }
                    //void o58() { /* TODO: POP  rw */ }
                    //void o60() { /* TODO: invalid */ }
                    //void o70() { /* TODO: Jcond cb */ }
                    //void o80() { /* TODO: alu regmem, imm */ _state = 4; }
                    //void o84() { /* TODO: TEST rm,r */ _state = 4; }
                    //void o86() { /* TODO: XCHG rm,r */ _state = 4; }
                    //void o88() { /* TODO: MOV  modrm */ _state = 4; }
                    //void o8C() { /* TODO: MOV  segreg */ }
                    //void o8D() { /* TODO: LEA  rw,m */ _state = 4; }
                    //void o8F() { /* TODO: POP  mw */ _state = 4; }
                    //void o90() { /* XCHG AX,rw */ UInt16 t = rw(); rw() = ax(); ax() = t; _wait = 3; }
                    //void o98() { /* CBW */ ah() = (al() >= 8 ? 0xff : 0x00); _wait = 2; }
                    //void o99() { /* CWD */ dx() = (ax() >= 0x8000 ? 0xffff : 0x0000); _wait = 5; }
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
        void setPZS()
        {
            _flags &= 0x8d5;
            if (!wordSize()) {
                _flags |= _parityTable[_result & 0xff];
                if ((_result & 0xff) == 0)
                    _flags |= 0x40;
                if ((_result & 0x80) != 0)
                    _flags |= 0x80;
            }
            else {
                _flags |= _parityTable[_result & 0xff]^_parityTable[(_result >> 8) & 0xff];
                if ((_result & 0xffff) == 0)
                    _flags |= 0x40;
                if ((_result & 0x8000) != 0)
                    _flags |= 0x80;
            }
        }
        void setAPZS()
        {
            _flags |= (((_result ^ _source ^ _destination) & 0x10) != 0 ? 0x10 : 0);
            setPZS();
        }
        void setCAPZS()
        {
            setAPZS();
            if (!wordSize()) {
                if ((_result & 0x100) != 0)
                    _flags |= 1;
            }
            else {
                if ((_result & 0x10000) != 0)
                    _flags |= 1;
            }
        }
        void setFlagsAdd()
        {
            setCAPZS();
            UInt16 t = (_result ^ _source) & (_result ^ _destination);
            if (!wordSize())
                _flags |= ((t & 0x80) != 0 ? 0x800 : 0);
            else
                _flags |= ((t & 0x8000) != 0 ? 0x800 : 0);
        }
        void setFlagsSub()
        {
            setCAPZS();
            UInt16 t = (_destination ^ _source) & (_result ^ _destination);
            if (!wordSize())
                _flags |= ((t & 0x80) != 0 ? 0x800 : 0);
            else
                _flags |= ((t & 0x8000) != 0 ? 0x800 : 0);
        }

        void doALUOperation(int op)
        {
            switch (op) {
                case 0:  // ADD
                    _result = _destination + _source;
                    setFlagsAdd();
                    break;
                case 1:  // OR
                    _result = _destination | _source;
                    setPZS();
                    break;
                case 2:  // ADC
                    _source += _flags & 1;
                    _result = _destination + _source;
                    setFlagsAdd();
                    break;
                case 3:  // SBB
                    _source += _flags & 1;
                    _result = _destination - _source;
                    setFlagsSub();
                    break;
                case 4:  // AND
                    _result = _destination & _source;
                    setPZS();
                    break;
                case 5:  // SUB
                case 7:  // CMP
                    _result = _destination - _source;
                    setFlagsSub();
                    break;
                case 6:  // XOR
                    _result = _destination ^ _source;
                    setPZS();
                    break;
            }
        }

        UInt16 signExtend(UInt8 data) { return data + (data < 0x80 ? 0 : 0xff00); }

        void initIO(int nextState, IOType ioType, bool wordSize)
        {
            _biu->initIO(nextState, ioType, wordSize, _data);
            _state = stateWaitingForBIU;
        }

        void registerIO(int reg)
        {
            if (!wordSize())
                if (_ioType == ioRead)
                    _ioData = byteRegister(reg);
                else
                    byteRegister(reg) = _ioData;
            else
                if (_ioType == ioRead)
                    _ioData = _registers[reg];
                else
                    _registers[reg] = _ioData;
        }

        //UInt16& rw() { return _registers[_opcode & 7]; }
        UInt16& ax() { return _registers[0]; }
        //UInt16& cx() { return _registers[1]; }
        //UInt16& dx() { return _registers[2]; }
        UInt16& bx() { return _registers[3]; }
        UInt16& sp() { return _registers[4]; }
        UInt16& bp() { return _registers[5]; }
        UInt16& si() { return _registers[6]; }
        UInt16& di() { return _registers[7]; }
        //UInt8& rb() { return byteRegister(_opcode & 7); }
        UInt8& al() { return byteRegister(0); }
        //UInt8& cl() { return byteRegister(1); }
        //UInt8& ah() { return byteRegister(4); }
        //bool carry() { return (_flags & 1) != 0; }

        bool wordSize() { return (_opcode & 1) != 0; }
        UInt16 eaDisplacement()
        {
            switch (_modRm & 7) {
                case 0: _wait = 7; return bx() + si();
                case 1: _wait = 8; return bx() + di();
                case 2: _wait = 8; return bp() + si();
                case 3: _wait = 7; return bp() + di();
                case 4: _wait = 5; return        si();
                case 5: _wait = 5; return        di();
                case 6: _wait = 5; return bp();
                case 7: _wait = 5; return bx();
            }
        }
        //bool eaSource() { return (_opcode & 2) != 0; }
        int modRmReg() { return (_modRm >> 3) & 7; }
        UInt16& modRmRw() { return _registers[modRmReg()]; }
        UInt8& modRmRb() { return byteRegister(modRmReg()); }
        UInt16 getReg() { return !wordSize() ? modRmRb() : modRmRw(); }
        UInt16 getAccum() { return !wordSize() ? al() : ax(); }
        void setAccum() { if (!wordSize()) al() = _result; else ax() = _result; }
        //void setReg(UInt16 value) { if (!wordSize()) modRmRb() = value : modRmRw() = value; }
        //UInt16 getEaValue() { /* TODO */ }
        //void setEaValue(UInt16 value) { /* TODO */ }

        //UInt16 alu(int operation, UInt16 a, UInt16 b)
        //{
        //    switch (operation) {
        //        case 0: return add(a, b);
        //        case 1: return or(a, b);
        //        case 2: return adc(a, b);
        //        case 3: return sbb(a, b);
        //        case 4: return and(a, b);
        //        case 5: return sub(a, b);
        //        case 6: return xor(a, b);
        //        case 7: return cmp(a, b);
        //    }
        //}

        //UInt16 add(UInt16 a, UInt16 b)
        //{
        //    return a + b;
        //    // TODO: set the flags
        //}
        //UInt16 or(UInt16 a, UInt16 b)
        //{
        //    return a | b;
        //    // TODO: set the flags
        //}

        //UInt16 getDestination() { if (eaSource()) return getReg(); else return getEaValue(); }
        //UInt16 getSource() { if (eaSource()) return getEaValue(); else return getReg(); }
        //void setDestination(UInt16 value) { if (eaSource()) setReg(value); else setEaValue(value); }

        //void push(UInt16 value) { /* TODO */ }

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
            //   8: TF = ?
            //   9: IF = interrupts enabled?
            //  10: DF = SI/DI decrement in string operations
            //  11: OF = signed overflow?

        Simulator* _simulator;
        BusInterfaceUnit* _biu;
        int _wait;
        State _state;
        UInt8 _opcode;
        UInt8 _modRm;
        //bool _useModRm;
        //UInt16 _eaOffset;
        UInt16 _data;  // Data from BIU
        State _opState;  // State to continue with after EA work complete
        IOType _ioType;
        UInt16 _ioData;
        UInt32 _source;
        UInt32 _destination;
        UInt32 _result;
        UInt16 _address;
        bool _useMemory;

        static int _parityTable[256];
    };

private:
    BusInterfaceUnit _biu;
    ExecutionUnit _eu;

    friend class BusInterfaceUnit;
    friend class ExecutionUnit;
};

int Simulator::ExecutionUnit::_parityTable[256] = {
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
