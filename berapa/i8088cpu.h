template<class T> class DisassemblerT
{
public:
    void setBus(ISA8BitBus* bus) { _bus = bus; }
    void setCPU(Intel8088CPU* cpu) { _cpu = cpu; }
    String disassemble(UInt16 address)
    {
        _bytes = "";
        String i = disassembleInstruction(address);
        return _bytes.alignLeft(10) + " " + i;
    }
private:
    String disassembleInstruction(UInt16 address)
    {
        _address = address;
        _opcode = getByte();
        _wordSize = (_opcode & 1) != 0;
        _doubleWord = false;
        if ((_opcode & 0xc4) == 0)
            return alu(_opcode >> 3) + regMemPair();
        if ((_opcode & 0xc6) == 4)
            return alu(_opcode >> 3) + accum() + ", " + imm();
        if ((_opcode & 0xe7) == 6)
            return "PUSH " + segreg(_opcode >> 3);
        if ((_opcode & 0xe7) == 7)
            return "POP " + segreg(_opcode >> 3);
        if ((_opcode & 0xe7) == 0x26)
            return segreg((_opcode >> 3) & 3) + ":";
        if ((_opcode & 0xf8) == 0x40)
            return "INC " + wordRegs(_opcode & 7);
        if ((_opcode & 0xf8) == 0x48)
            return "DEC " + wordRegs(_opcode & 7);
        if ((_opcode & 0xf8) == 0x50)
            return "PUSH " + wordRegs(_opcode & 7);
        if ((_opcode & 0xf8) == 0x58)
            return "POP " + wordRegs(_opcode & 7);
        if ((_opcode & 0xf0) == 0x60)
            return "???";
        if ((_opcode & 0xfc) == 0x80) {
            _modRM = getByte();
            String s = alu(reg()) + ea() + ", ";
            return s + (_opcode == 0x81 ? iw() : sb());
        }
        if ((_opcode & 0xfc) == 0x88)
            return "MOV " + regMemPair();
        if ((_opcode & 0xf8) == 0x90)
            if (_opcode == 0x90)
                return "NOP";
            else
                return "XCHG AX, " + wordRegs(_opcode & 7);
        if ((_opcode & 0xf8) == 0xb0)
            return "MOV " + byteRegs(_opcode & 7) + ", " + ib();
        if ((_opcode & 0xf8) == 0xb8)
            return "MOV " + wordRegs(_opcode & 7) + ", " + iw();
        if ((_opcode & 0xf6) == 0xc0)
            return "???";
        if ((_opcode & 0xfc) == 0xd0) {
            _modRM = getByte();
            static String shifts[8] = {
                "ROL", "ROR", "RCL", "RCR", "SHL", "SHR", "SHL", "SAR"};
            return shifts[reg()] + " " + ea() + ", " +
                ((_opcode & 2) == 0 ? String("1") : byteRegs(1));
        }
        if ((_opcode & 0xf8) == 0xd8) {
            _modRM = getByte();
            _wordSize = true;
            return String("ESC ") + (_opcode & 7) + ", " + r() + ", " + ea();
        }
        if ((_opcode & 0xf6) == 0xe4)
            return "IN " + accum() + ", " +
                ((_opcode & 8) == 0 ? ib() : wordRegs(2));
        if ((_opcode & 0xf6) == 0xe6)
            return "OUT " + ((_opcode & 8) == 0 ? ib() : wordRegs(2)) + ", " +
                accum();
        if ((_opcode & 0xf0) == 0x70) {
            static String conds[16] = {
                "O", "NO", "B", "AE", "E", "NE", "BE", "A",
                "S", "NS", "P", "NP", "L", "GE", "LE", "G"};
            return "J" + conds[_opcode & 0xf] + " " + cb();
        }
        switch (_opcode) {
            case 0x27: return "DAA";
            case 0x2f: return "DAS";
            case 0x37: return "AAA";
            case 0x3f: return "AAS";
            case 0x84:
            case 0x85: return "TEST " + regMemPair();
            case 0x86:
            case 0x87: return "XCHG " + regMemPair();
            case 0x8c:
                _modRM = getByte();
                _wordSize = true;
                return "MOV " + ea() + ", " + segreg(reg());
            case 0x8d:
                _modRM = getByte();
                _doubleWord = true;
                _wordSize = false;
                return "LEA " + rw() + ", " + ea();
            case 0x8e:
                _modRM = getByte();
                _wordSize = true;
                return "MOV " + segreg(reg()) + ", " + ea();
            case 0x8f: _modRM = getByte(); return "POP " + ea();
            case 0x98: return "CBW";
            case 0x99: return "CWD";
            case 0x9a: return "CALL " + cp();
            case 0x9b: return "WAIT";
            case 0x9c: return "PUSHF";
            case 0x9d: return "POPF";
            case 0x9e: return "SAHF";
            case 0x9f: return "LAHF";
            case 0xa0:
            case 0xa1: return "MOV " + accum() + ", " + size() + "[" + iw() +
                "]";
            case 0xa2:
            case 0xa3: return "MOV " + size() + "[" + iw() + "], " + accum();
            case 0xa4:
            case 0xa5: return "MOVS" + size();
            case 0xa6:
            case 0xa7: return "CMPS" + size();
            case 0xa8:
            case 0xa9: return "TEST " + accum() + ", " +
                (!_wordSize ? ib() : iw());
            case 0xaa:
            case 0xab: return "STOS" + size();
            case 0xac:
            case 0xad: return "LODS" + size();
            case 0xae:
            case 0xaf: return "SCAS" + size();
            case 0xc2: return "RET " + iw();
            case 0xc3: return "RET";
            case 0xc4:
                _modRM = getByte();
                _doubleWord = true;
                return "LDS " + rw() + ", " + ea();
            case 0xc5:
                _modRM = getByte();
                _doubleWord = true;
                _wordSize = false;
                return "LES " + rw() + ", " + ea();
            case 0xc6:
            case 0xc7:
                _modRM = getByte();
                return "MOV " + ea() + ", " + (!_wordSize ? ib() : iw());
            case 0xca: return "RETF " + iw();
            case 0xcb: return "RETF";
            case 0xcc: return "INT 3";
            case 0xcd: return "INT " + ib();
            case 0xce: return "INTO";
            case 0xcf: return "IRET";
            case 0xd4: return "AAM " + ib();
            case 0xd5: return "AAD " + ib();
            case 0xd6: return "SALC";
            case 0xd7: return "XLATB";
            case 0xe0: return "LOOPNE " + cb();
            case 0xe1: return "LOOPE " + cb();
            case 0xe2: return "LOOP " + cb();
            case 0xe3: return "JCXZ " + cb();
            case 0xe8: return "CALL " + cw();
            case 0xe9: return "JMP " + cw();
            case 0xea: return "JMP " + cp();
            case 0xeb: return "JMP " + cb();
            case 0xf0: return "LOCK";
            case 0xf1: return "???";
            case 0xf2: return "REPNE ";
            case 0xf3: return "REP ";
            case 0xf4: return "HLT";
            case 0xf5: return "CMC";
            case 0xf6:
            case 0xf7:
                _modRM = getByte();
                switch (reg()) {
                    case 0:
                    case 1: return "TEST " + ea() + ", " +
                        (!_wordSize ? ib() : iw());
                    case 2: return "NOT " + ea();
                    case 3: return "NEG " + ea();
                    case 4: return "MUL " + ea();
                    case 5: return "IMUL " + ea();
                    case 6: return "DIV " + ea();
                    case 7: return "IDIV " + ea();
                }
            case 0xf8: return "CLC";
            case 0xf9: return "STC";
            case 0xfa: return "CLI";
            case 0xfb: return "STI";
            case 0xfc: return "CLD";
            case 0xfd: return "STD";
            case 0xfe:
            case 0xff:
                _modRM = getByte();
                switch (reg()) {
                    case 0: return "INC " + ea();
                    case 1: return "DEC " + ea();
                    case 2: return "CALL " + ea();
                    case 3: _doubleWord = true; return "CALL " + ea();
                    case 4: return "JMP " + ea();
                    case 5: _doubleWord = true; return "JMP " + ea();
                    case 6: return "PUSH " + ea();
                    case 7: return "??? " + ea();
                }
        }
        return "";
    }
    UInt8 getByte()
    {
        UInt8 v = _bus->debugReadMemory(_cpu->codeAddress(_address));
        ++_address;
        _bytes += hex(v, 2, false);
        return v;
    }
    UInt16 getWord() { UInt8 low = getByte(); return low + (getByte() << 8); }
    String regMemPair()
    {
        _modRM = getByte();
        if ((_opcode & 2) == 0)
            return ea() + String(", ") + r();
        return r() + String(", ") + ea();
    }
    String r() { return !_wordSize ? rb() : rw(); }
    String rb() { return byteRegs(reg()); }
    String rw() { return wordRegs(reg()); }
    String byteRegs(int r)
    {
        static String b[8] = {"AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH"};
        return b[r];
    }
    String wordRegs(int r)
    {
        static String w[8] = {"AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI"};
        return w[r];
    }
    String ea()
    {
        String s;
        switch (mod()) {
            case 0: s = disp(); break;
            case 1: s = disp() + sb(); break;
            case 2: s = disp() + iw(); break;
            case 3: return !_wordSize ? byteRegs(rm()) : wordRegs(rm());
        }
        return size() + String("[") + s + String("]");
    }
    String size()
    {
        if (!_doubleWord)
            return (!_wordSize ? String("B") : String("W"));
        else
            return (!_wordSize ? String("") : String("D"));
    }
    String disp()
    {
        static String d[8] = {
            "BX+SI", "BX+DI", "BP+SI", "BP+DI", "SI", "DI", "BP", "BX"};
        if (mod() == 0 && rm() == 6)
            return iw();
        return d[rm()];
    }
    String alu(int op)
    {
        static String o[8] = {
            "ADD ", "OR ", "ADC ", "SBB ", "AND ", "SUB ", "XOR ", "CMP "};
        return o[op];
    }
    int mod() { return _modRM >> 6; }
    int reg() { return (_modRM >> 3) & 7; }
    int rm() { return _modRM & 7; }
    String iw() { return hex(getWord(), 4, false); }
    String ib() { return hex(getByte(), 2, false); }
    String sb()
    {
        UInt8 byte = getByte();
        if ((byte & 0x80) == 0)
            return String("+") + hex(byte, 2, false);
        return String("-") + hex((-byte) & 0x7f, 2, false);
    }
    String imm() { return !_wordSize ? ib() : iw(); }
    String accum() { return !_wordSize ? "AL" : "AX"; }
    String segreg(int r)
    {
        static String sr[8] = {"ES", "CS", "SS", "DS", "??", "??", "??", "??"};
        return sr[r];
    }
    String cb()
    {
        SInt8 byte = static_cast<SInt8>(getByte());
        return hex(_address + byte, 4, false);
    }
    String cw()
    {
        UInt16 word = getWord();
        return hex(_address + word, 4, false);
    }
    String cp()
    {
        UInt16 offset = getWord();
        return hex(getWord(), 4, false) + ":" + hex(offset, 4, false);
    }

    ISA8BitBus* _bus;
    Intel8088CPUT<T>* _cpu;
    UInt16 _address;
    UInt8 _opcode;
    UInt8 _modRM;
    bool _wordSize;
    bool _doubleWord;
    String _bytes;
};

typedef DisassemblerT<void> Disassembler;

template<class T> class Intel8088CPUT
  : public ClockedComponentBase<Intel8088CPU>
{
public:
    static String typeName() { return "Intel8088CPU"; }
    Intel8088CPUT(Component::Type type)
      : ClockedComponentBase(type), _connector(this), _irqConnector(this),
        _nmiConnector(this)
    {
        connector("", &_connector);
        connector("nmi", &_nmiConnector);
        connector("irq", &_irqConnector);

        static String b[8] = {"AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH"};
        static String w[8] = {"AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI"};
        static String s[8] = {"ES", "CS", "SS", "DS", "??", "??", "??", "??"};
        for (int i = 0; i < 8; ++i) {
            _wordRegisters[i].init(w[i], &_registerData[i]);
            _byteRegisters[i].init(b[i], reinterpret_cast<UInt8*>(
                &_registerData[i & 3]) + (i >= 4 ? 1 : 0));
            _segmentRegisters[i].init(s[i], &_segmentRegisterData[i]);
        }
        _flags.init("F", &_flagsData);

        _segmentRegisterData[7] = 0x0000;  // For IO accesses

        _disassembler.setCPU(this);

        typename EnumerationType<State>::Helper h;
        h.add(stateWaitingForBIU,  "waitingForBIU");
        h.add(stateBegin,          "stateBegin");
        h.add(stateDecodeOpcode,   "stateDecodeOpcode");
        h.add(stateEndInstruction, "stateEndInstruction");
        h.add(stateBeginModRM,     "stateBeginModRM");
        h.add(stateDecodeModRM,    "stateDecodeModRM");
        h.add(stateEAOffset,       "stateEAOffset");
        h.add(stateEARegisters,    "stateEARegisters");
        h.add(stateEAByte,         "stateEAByte");
        h.add(stateEAWord,         "stateEAWord");
        h.add(stateEASetSegment,   "stateEASetSegment");
        h.add(stateEAIO,           "stateEAIO");
        h.add(statePush,           "statePush");
        h.add(statePush2,          "statePush2");
        h.add(statePop,            "statePop");
        h.add(stateALU,            "stateALU");
        h.add(stateALU2,           "stateALU2");
        h.add(stateALU3,           "stateALU3");
        h.add(stateALUAccumImm,    "stateALUAccumImm");
        h.add(stateALUAccumImm2,   "stateALUAccumImm2");
        h.add(statePushSegReg,     "statePushSegReg");
        h.add(statePopSegReg,      "statePopSegReg");
        h.add(statePopSegReg2,     "statePopSegReg2");
        h.add(stateSegOverride,    "stateSegOverride");
        h.add(stateDAA,            "stateDAA");
        h.add(stateDAS,            "stateDAS");
        h.add(stateDA,             "stateDA");
        h.add(stateAAA,            "stateAAA");
        h.add(stateAAS,            "stateAAS");
        h.add(stateAA,             "stateAA");
        h.add(stateIncDecRW,       "stateIncDecRW");
        h.add(statePushRW,         "statePushRW");
        h.add(statePopRW,          "statePopRW");
        h.add(statePopRW2,         "statePopRW2");
        h.add(stateInvalid,        "stateInvalid");
        h.add(stateJCond,          "stateJCond");
        h.add(stateJCond2,         "stateJCond2");
        h.add(stateALURMImm,       "stateALURMImm");
        h.add(stateALURMImm2,      "stateALURMImm2");
        h.add(stateALURMImm3,      "stateALURMImm3");
        h.add(stateTestRMReg,      "stateTestRMReg");
        h.add(stateTestRMReg2,     "stateTestRMReg2");
        h.add(stateXchgRMReg,      "stateXchgRMReg");
        h.add(stateXchgRMReg2,     "stateXchgRMReg2");
        h.add(stateMovRMReg,       "stateMovRMReg");
        h.add(stateMovRMReg2,      "stateMovRMReg2");
        h.add(stateMovRegRM,       "stateMovRegRM");
        h.add(stateMovRegRM2,      "stateMovRegRM2");
        h.add(stateMovRMWSegReg,   "stateMovRMWSegReg");
        h.add(stateMovRMWSegReg2,  "stateMovRMWSegReg2");
        h.add(stateLEA,            "stateLEA");
        h.add(stateLEA2,           "stateLEA2");
        h.add(stateMovSegRegRMW,   "stateMovSegRegRMW");
        h.add(stateMovSegRegRMW2,  "stateMovSegRegRMW2");
        h.add(statePopMW,          "statePopMW");
        h.add(statePopMW2,         "statePopMW2");
        h.add(stateXchgAxRW,       "stateXchgAxRW");
        h.add(stateCBW,            "stateCBW");
        h.add(stateCWD,            "stateCWD");
        h.add(stateCallCP,         "stateCallCP");
        h.add(stateCallCP2,        "stateCallCP2");
        h.add(stateCallCP3,        "stateCallCP3");
        h.add(stateCallCP4,        "stateCallCP4");
        h.add(stateCallCP5,        "stateCallCP5");
        h.add(stateWait,           "stateWait");
        h.add(statePushF,          "statePushF");
        h.add(statePopF,           "statePopF");
        h.add(statePopF2,          "statePopF2");
        h.add(stateSAHF,           "stateSAHF");
        h.add(stateLAHF,           "stateLAHF");
        h.add(stateMovAccumInd,    "stateMovAccumInd");
        h.add(stateMovAccumInd2,   "stateMovAccumInd2");
        h.add(stateMovAccumInd3,   "stateMovAccumInd3");
        h.add(stateMovIndAccum,    "stateMovIndAccum");
        h.add(stateMovIndAccum2,   "stateMovIndAccum2");
        h.add(stateMovS,           "stateMovS");
        h.add(stateMovS2,          "stateMovS2");
        h.add(stateRepAction,      "stateRepAction");
        h.add(stateCmpS,           "stateCmpS");
        h.add(stateCmpS2,          "stateCmpS2");
        h.add(stateCmpS3,          "stateCmpS3");
        h.add(stateTestAccumImm,   "stateTestAccumImm");
        h.add(stateTestAccumImm2,  "stateTestAccumImm2");
        h.add(stateStoS,           "stateStoS");
        h.add(stateLodS,           "stateLodS");
        h.add(stateLodS2,          "stateLodS2");
        h.add(stateScaS,           "stateScaS");
        h.add(stateScaS2,          "stateScaS2");
        h.add(stateMovRegImm,      "stateMovRegImm");
        h.add(stateMovRegImm2,     "stateMovRegImm2");
        h.add(stateRet,            "stateRet");
        h.add(stateRet2,           "stateRet2");
        h.add(stateRet3,           "stateRet3");
        h.add(stateRet4,           "stateRet4");
        h.add(stateRet5,           "stateRet5");
        h.add(stateRet6,           "stateRet6");
        h.add(stateLoadFar,        "stateLoadFar");
        h.add(stateLoadFar2,       "stateLoadFar2");
        h.add(stateLoadFar3,       "stateLoadFar3");
        h.add(stateMovRMImm,       "stateMovRMImm");
        h.add(stateMovRMImm2,      "stateMovRMImm2");
        h.add(stateMovRMImm3,      "stateMovRMImm3");
        h.add(stateCheckInt,       "stateCheckInt");
        h.add(stateHardwareInt,    "stateHardwareInt");
        h.add(stateHardwareInt2,   "stateHardwareInt2");
        h.add(stateInt,            "stateInt");
        h.add(stateInt3,           "stateInt3");
        h.add(stateIntAction,      "stateIntAction");
        h.add(stateIntAction2,     "stateIntAction2");
        h.add(stateIntAction3,     "stateIntAction3");
        h.add(stateIntAction4,     "stateIntAction4");
        h.add(stateIntAction5,     "stateIntAction5");
        h.add(stateIntAction6,     "stateIntAction6");
        h.add(stateIntO,           "stateIntO");
        h.add(stateIRet,           "stateIRet");
        h.add(stateIRet2,          "stateIRet2");
        h.add(stateIRet3,          "stateIRet3");
        h.add(stateIRet4,          "stateIRet4");
        h.add(stateShift,          "stateShift");
        h.add(stateShift2,         "stateShift2");
        h.add(stateShift3,         "stateShift3");
        h.add(stateAAM,            "stateAAM");
        h.add(stateAAM2,           "stateAAM2");
        h.add(stateAAD,            "stateAAD");
        h.add(stateAAD2,           "stateAAD2");
        h.add(stateSALC,           "stateSALC");
        h.add(stateXlatB,          "stateXlatB");
        h.add(stateXlatB2,         "stateXlatB2");
        h.add(stateEscape,         "stateEscape");
        h.add(stateEscape2,        "stateEscape2");
        h.add(stateLoop,           "stateLoop");
        h.add(stateLoop2,          "stateLoop2");
        h.add(stateInOut,          "stateInOut");
        h.add(stateInOut2,         "stateInOut2");
        h.add(stateInOut3,         "stateInOut3");
        h.add(stateCallCW,         "stateCallCW");
        h.add(stateCallCW2,        "stateCallCW2");
        h.add(stateCallCW3,        "stateCallCW3");
        h.add(stateCallCW4,        "stateCallCW4");
        h.add(stateJmpCW,          "stateJmpCW");
        h.add(stateJmpCW2,         "stateJmpCW2");
        h.add(stateJmpCW3,         "stateJmpCW3");
        h.add(stateJmpCP,          "stateJmpCP");
        h.add(stateJmpCP2,         "stateJmpCP2");
        h.add(stateJmpCP3,         "stateJmpCP3");
        h.add(stateJmpCP4,         "stateJmpCP4");
        h.add(stateJmpCB,          "stateJmpCB");
        h.add(stateJmpCB2,         "stateJmpCB2");
        h.add(stateLock,           "stateLock");
        h.add(stateRep,            "stateRep");
        h.add(stateHlt,            "stateHlt");
        h.add(stateHalted,         "stateHalted");
        h.add(stateCmC,            "stateCmC");
        h.add(stateMath,           "stateMath");
        h.add(stateMath2,          "stateMath2");
        h.add(stateMath3,          "stateMath3");
        h.add(stateLoadC,          "stateLoadC");
        h.add(stateLoadI,          "stateLoadI");
        h.add(stateLoadD,          "stateLoadD");
        h.add(stateMisc,           "stateMisc");
        h.add(stateMisc2,          "stateMisc2");
        EnumerationType<State> stateType("State", h, typeName() + ".");

        typename EnumerationType<IOType>::Helper ht;
        ht.add(ioNone, "ioNone");
        ht.add(ioRead, "ioRead");
        ht.add(ioWrite, "ioWrite");
        ht.add(ioInstructionFetch, "ioInstructionFetch");
        ht.add(ioInterruptAcknowledge, "ioInterruptAcknowledge");
        EnumerationType<IOType> ioTypeType("IOType", ht, typeName() + ".");

        typename EnumerationType<BusState>::Helper hb;
        hb.add(t1, "t1");
        hb.add(t2, "t2");
        hb.add(t3, "t3");
        hb.add(tWait, "tWait");
        hb.add(t4, "t4");
        hb.add(tIdle, "tIdle");

        typename EnumerationType<IOByte>::Helper hi;
        hi.add(ioSingleByte, "ioSingleByte");
        hi.add(ioWordFirst,  "ioWordFirst");
        hi.add(ioWordSecond, "ioWordSecond");

        persist("ip", &_ip);
        persist("registers", &_registerData[0], ArrayType(WordType(), 8)); // ?
        List<Value> initialSegments;
        initialSegments.add(Value(WordType(), 0));
        initialSegments.add(Value(WordType(), 0xffff));
        persist("segmentRegisters", &_segmentRegisterData[0],
            Value(ArrayType(WordType(), 4), initialSegments));
        persist("flags", &_flagsData, 2); // ?
        persist("prefetch", this, PersistQueueType());
        persist("segment", &_segment);
        persist("segmentOverride", &_segmentOverride, -1);
        persist("prefetchAddress", &_prefetchAddress);
        persist("ioType", &_ioType, ioTypeType);
        persist("ioRequested", &_ioRequested, ioTypeType);
        persist("ioInProgress", &_ioInProgress, ioInstructionFetch,
            ioTypeType);
        persist("busState", &_busState,
            EnumerationType<BusState>("BusState", hb, typeName() + "."));
        persist("byte", &_byte,
            EnumerationType<IOByte>("IOByte", hi, typeName() + "."));
        persist("abandonFetch", &_abandonFetch);
        persist("wait", &_wait);
        persist("state", &_state, stateBegin, stateType);
        persist("opcode", &_opcode);
        persist("modRM", &_modRM);
        persist("data", &_data, HexPersistenceType(8));
        persist("source", &_source, HexPersistenceType(8));
        persist("destination", &_destination, HexPersistenceType(8));
        persist("remainder", &_remainder, HexPersistenceType(8));
        persist("address", &_address);
        persist("useMemory", &_useMemory);
        persist("wordSize", &_wordSize);
        persist("aluOperation", &_aluOperation);
        persist("afterEA", &_afterEA, stateType);
        persist("afterIO", &_afterIO, stateType);
        persist("afterEAIO", &_afterEAIO, stateType);
        persist("afterRep", &_afterRep, stateType);
        persist("afterCheckInt", &_afterCheckInt, stateType);
        persist("sourceIsRM", &_sourceIsRM);
        persist("savedCS", &_savedCS);
        persist("savedIP", &_savedIP);
        persist("rep", &_rep);
        persist("usePortSpace", &_usePortSpace);
        persist("newInstruction", &_newInstruction, true);
        persist("newIP", &_newIP);
        persist("nmiRequested", &_nmiRequested);
        persist("interruptRequested", &_interruptRequested);
        persist("cycle", &_cycle);
        persist("ready", &_ready, true);

        _visited.allocate(0x100000);
        for (int i = 0; i < 0x100000; ++i)
            _visited[i] = false;
    }
    void load(const Value& v)
    {
        ClockedComponent::load(v);
        _disassembler.setBus(_bus);
        _pic = _bus->getPIC();
    }
    void setStopAtCycle(int stopAtCycle) { _stopAtCycle = stopAtCycle; }
    UInt32 codeAddress(UInt16 offset) { return physicalAddress(1, offset); }
    void runTo(Tick tick)
    {
        while (_tick < tick) {
            _tick += _ticksPerCycle;
            simulateCycle();
        }
    }
    void maintain(Tick ticks)
    {
        _interruptTick -= ticks;
        _readyChangeTick -= ticks;
        ClockedComponent::maintain(ticks);
    }
    void simulateCycle()
    {
        simulateCycleAction();
        if (_cycle >= 000000) {
        //    String line = String(decimal(_cycle)).alignRight(5) + " ";
        //    switch (_busState) {
        //        case t1:
        //            line += "T1 " + hex(_busAddress, 5, false) + " ";
        //            break;
        //        case t2:
        //            line += "T2 ";
        //            if (_ioInProgress == ioWrite)
        //                line += "M<-" + hex(_busData, 2, false) + " ";
        //            else
        //                line += "      ";
        //            break;
        //        case t3: line += "T3       "; break;
        //        case tWait: line += "Tw       "; break;
        //        case t4:
        //            line += "T4 ";
        //            if (_ioInProgress == ioWrite)
        //                line += "      ";
        //            else
        //                if (_abandonFetch)
        //                    line += "----- ";
        //                else
        //                    line += "M->" + hex(_busData, 2, false) + " ";
        //            break;
        //        case tIdle: line += "         "; break;
        //    }
            if (_newInstruction) {
                UInt32 a = codeAddress(_newIP);
                if (!_visited[a]) {
                    console.write(String(decimal(_cycle)).alignRight(5) + " " +
                        hex(csQuiet(), 4, false) + ":" +
                        hex(_newIP, 4, false) + " " +
                        _disassembler.disassemble(_newIP) + "\n");
                }
                _visited[a] = true;
        //        line += hex(csQuiet(), 4, false) + ":" + hex(_newIP, 4, false)
        //            + " " + _disassembler.disassemble(_newIP);
            }
        //    line = line.alignLeft(50);
        //    for (int i = 0; i < 8; ++i) {
        //        line += _byteRegisters[i].text();
        //        line += _wordRegisters[i].text();
        //        line += _segmentRegisters[i].text();
        //    }
        //    line += _flags.text();
        //    //if(_newInstruction)
        //        console.write(line + "\n");
            _newInstruction = false;
        }

        ++_cycle;
        //if (_cycle == 4793344)
        //    console.write("!");
        if (_cycle % 1000000 == 0)
            console.write(".");
        //if (_cycle == 4900000)
        //    throw Exception("Finished");
    }
    void simulateCycleAction()
    {
        bool busDone;
        do {
            busDone = true;
            switch (_busState) {
                case t1:
                    if (_ioInProgress == ioInstructionFetch) {
                        _busAddress = physicalAddress(1, _prefetchAddress);
                        _bus->setAddressReadMemory(_tick, _busAddress);
                    }
                    else {
                        int segment = _segment;
                        if (_segmentOverride != -1)
                            segment = _segmentOverride;
                        _busAddress = physicalAddress(segment, _address);
                        if (_usePortSpace) {
                            if (_ioInProgress == ioWrite)
                                _bus->setAddressWriteIO(_tick, _busAddress);
                            else
                                _bus->setAddressReadIO(_tick, _busAddress);
                        }
                        else {
                            if (_ioInProgress == ioWrite) {
                                _bus->setAddressWriteMemory(_tick,
                                    _busAddress);
                            }
                            else
                                _bus->setAddressReadMemory(_tick, _busAddress);
                        }
                    }
                    _busState = t2;
                    break;
                case t2:
                    if (_ioInProgress == ioWrite) {
                        _ioRequested = ioNone;
                        switch (_byte) {
                            case ioSingleByte:
                                _busData = _data;
                                _state = _afterIO;
                                break;
                            case ioWordFirst:
                                _busData = _data;
                                _ioInProgress = ioWrite;
                                _byte = ioWordSecond;
                                ++_address;
                                break;
                            case ioWordSecond:
                                _busData = _data >> 8;
                                _state = _afterIO;
                                _byte = ioSingleByte;
                                break;
                        }
                        if (_usePortSpace)
                            _bus->writeIO(_tick, _busData);
                        else
                            _bus->writeMemory(_tick, _busData);
                    }
                    _busState = t3;
                    break;
                case t3:
                    _busState = tWait;
                    busDone = false;
                    break;
                case tWait:
                    if (!_ready)
                        break;
                    _busState = t4;
                    if (_ioInProgress == ioInstructionFetch) {
                        if (_abandonFetch)
                            break;
                        _prefetchQueue[(_prefetchOffset + _prefetched) & 3] =
                            _bus->readMemory(_tick);
                        ++_prefetched;
                        ++_prefetchAddress;
                        completeInstructionFetch();
                        break;
                    }
                    if (_ioInProgress == ioWrite)
                        break;
                    if (_ioInProgress == ioInterruptAcknowledge)
                        _data = _pic->readAcknowledgeByte(_tick);
                    else
                        if (_usePortSpace)
                            _busData = _bus->readIO(_tick);
                        else
                            _busData = _bus->readMemory(_tick);
                    _ioRequested = ioNone;
                    switch (_byte) {
                        case ioSingleByte:
                            _data = _busData;
                            _state = _afterIO;
                            break;
                        case ioWordFirst:
                            _data = _busData;
                            _ioInProgress = ioRead;
                            _byte = ioWordSecond;
                            ++_address;
                            break;
                        case ioWordSecond:
                            _data |= static_cast<UInt16>(_busData) << 8;
                            _state = _afterIO;
                            _byte = ioSingleByte;
                            break;
                    }
                    break;
                case t4:
                    _busState = tIdle;
                    busDone = false;
                    break;
                case tIdle:
                    if (_byte == ioWordSecond)
                        _busState = t1;
                    else
                        _ioInProgress = ioNone;
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
                    break;
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

                case stateBegin: fetch(stateDecodeOpcode, false); break;
                case stateDecodeOpcode:
                    {
                        _opcode = _data;
                        _wordSize = ((_opcode & 1) != 0);
                        _sourceIsRM = ((_opcode & 2) != 0);
                        static State stateForOpcode[0x100] = {
stateALU,          stateALU,          stateALU,          stateALU,
stateALUAccumImm,  stateALUAccumImm,  statePushSegReg,   statePopSegReg,
stateALU,          stateALU,          stateALU,          stateALU,
stateALUAccumImm,  stateALUAccumImm,  statePushSegReg,   statePopSegReg,
stateALU,          stateALU,          stateALU,          stateALU,
stateALUAccumImm,  stateALUAccumImm,  statePushSegReg,   statePopSegReg,
stateALU,          stateALU,          stateALU,          stateALU,
stateALUAccumImm,  stateALUAccumImm,  statePushSegReg,   statePopSegReg,
stateALU,          stateALU,          stateALU,          stateALU,
stateALUAccumImm,  stateALUAccumImm,  stateSegOverride,  stateDAA,
stateALU,          stateALU,          stateALU,          stateALU,
stateALUAccumImm,  stateALUAccumImm,  stateSegOverride,  stateDAS,
stateALU,          stateALU,          stateALU,          stateALU,
stateALUAccumImm,  stateALUAccumImm,  stateSegOverride,  stateAAA,
stateALU,          stateALU,          stateALU,          stateALU,
stateALUAccumImm,  stateALUAccumImm,  stateSegOverride,  stateAAS,
stateIncDecRW,     stateIncDecRW,     stateIncDecRW,     stateIncDecRW,
stateIncDecRW,     stateIncDecRW,     stateIncDecRW,     stateIncDecRW,
stateIncDecRW,     stateIncDecRW,     stateIncDecRW,     stateIncDecRW,
stateIncDecRW,     stateIncDecRW,     stateIncDecRW,     stateIncDecRW,
statePushRW,       statePushRW,       statePushRW,       statePushRW,
statePushRW,       statePushRW,       statePushRW,       statePushRW,
statePopRW,        statePopRW,        statePopRW,        statePopRW,
statePopRW,        statePopRW,        statePopRW,        statePopRW,
stateJCond,        stateJCond,        stateJCond,        stateJCond,
stateJCond,        stateJCond,        stateJCond,        stateJCond,
stateJCond,        stateJCond,        stateJCond,        stateJCond,
stateJCond,        stateJCond,        stateJCond,        stateJCond,
stateJCond,        stateJCond,        stateJCond,        stateJCond,
stateJCond,        stateJCond,        stateJCond,        stateJCond,
stateJCond,        stateJCond,        stateJCond,        stateJCond,
stateJCond,        stateJCond,        stateJCond,        stateJCond,
stateALURMImm,     stateALURMImm,     stateALURMImm,     stateALURMImm,
stateTestRMReg,    stateTestRMReg,    stateXchgRMReg,    stateXchgRMReg,
stateMovRMReg,     stateMovRMReg,     stateMovRegRM,     stateMovRegRM,
stateMovRMWSegReg, stateLEA,          stateMovSegRegRMW, statePopMW,
stateXchgAxRW,     stateXchgAxRW,     stateXchgAxRW,     stateXchgAxRW,
stateXchgAxRW,     stateXchgAxRW,     stateXchgAxRW,     stateXchgAxRW,
stateCBW,          stateCWD,          stateCallCP,       stateWait,
statePushF,        statePopF,         stateSAHF,         stateLAHF,
stateMovAccumInd,  stateMovAccumInd,  stateMovIndAccum,  stateMovIndAccum,
stateMovS,         stateMovS,         stateCmpS,         stateCmpS,
stateTestAccumImm, stateTestAccumImm, stateStoS,         stateStoS,
stateLodS,         stateLodS,         stateScaS,         stateScaS,
stateMovRegImm,    stateMovRegImm,    stateMovRegImm,    stateMovRegImm,
stateMovRegImm,    stateMovRegImm,    stateMovRegImm,    stateMovRegImm,
stateMovRegImm,    stateMovRegImm,    stateMovRegImm,    stateMovRegImm,
stateMovRegImm,    stateMovRegImm,    stateMovRegImm,    stateMovRegImm,
stateInvalid,      stateInvalid,      stateRet,          stateRet,
stateLoadFar,      stateLoadFar,      stateMovRMImm,     stateMovRMImm,
stateInvalid,      stateInvalid,      stateRet,          stateRet,
stateInt3,         stateInt,          stateIntO,         stateIRet,
stateShift,        stateShift,        stateShift,        stateShift,
stateAAM,          stateAAD,          stateSALC,         stateXlatB,
stateEscape,       stateEscape,       stateEscape,       stateEscape,
stateEscape,       stateEscape,       stateEscape,       stateEscape,
stateLoop,         stateLoop,         stateLoop,         stateLoop,
stateInOut,        stateInOut,        stateInOut,        stateInOut,
stateCallCW,       stateJmpCW,        stateJmpCP,        stateJmpCB,
stateInOut,        stateInOut,        stateInOut,        stateInOut,
stateLock,         stateInvalid,      stateRep,          stateRep,
stateHlt,          stateCmC,          stateMath,         stateMath,
stateLoadC,        stateLoadC,        stateLoadI,        stateLoadI,
stateLoadD,        stateLoadD,        stateMisc,         stateMisc};
                        _state = stateForOpcode[_opcode];
                    }
                    break;
                case stateEndInstruction:
                    _segmentOverride = -1;
                    _rep = 0;
                    _usePortSpace = false;
                    _newInstruction = true;
                    _newIP = _ip;
                    _afterCheckInt = stateBegin;
                    _state = stateCheckInt;
                    break;

                case stateBeginModRM: fetch(stateDecodeModRM, false); break;
                case stateDecodeModRM:
                    _modRM = _data;
                    if ((_modRM & 0xc0) == 0xc0) {
                        _useMemory = false;
                        _address = _modRM & 7;
                        _state = _afterEA;
                        break;
                    }
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

                case stateEAIO:
                    if (_useMemory)
                        initIO(_afterEAIO, _ioType, _wordSize);
                    else {
                        if (!_wordSize)
                            if (_ioType == ioRead)
                                _data = _byteRegisters[_address];
                            else
                                _byteRegisters[_address] = _data;
                        else
                            if (_ioType == ioRead)
                                _data = _wordRegisters[_address];
                            else
                                _wordRegisters[_address] = _data;
                        _state = _afterEAIO;
                    }
                    break;

                case statePush:
                    sp() -= 2;
                case statePush2:
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

                case stateALUAccumImm:
                    fetch(stateALUAccumImm2, _wordSize);
                    break;
                case stateALUAccumImm2:
                    _destination = getAccum();
                    _source = _data;
                    _aluOperation = (_opcode >> 3) & 7;
                    doALUOperation();
                    if (_aluOperation != 7)
                        setAccum();
                    end(4);
                    break;

                case statePushSegReg:
                    push(_segmentRegisters[_opcode >> 3]);
                    break;

                case statePopSegReg: pop(statePopSegReg2); break;
                case statePopSegReg2:
                    _segmentRegisters[_opcode >> 3] = _data;
                    end(4);
                    break;

                case stateSegOverride:
                    _segmentOverride = (_opcode >> 3) & 3;
                    _state = stateBegin;
                    _wait = 2;
                    break;

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
                case statePushRW:
                    sp() -= 2;
                    _data = rw();
                    _afterIO = stateEndInstruction;
                    _state = statePush2;
                    _wait += 7;
                    break;
                case statePopRW: pop(statePopRW2); break;
                case statePopRW2: rw() = _data; end(4); break;

                case stateInvalid: end(0); break;

                case stateJCond: fetch(stateJCond2, false); break;
                case stateJCond2:
                    {
                        bool jump;
                        switch (_opcode & 0x0e) {
                            case 0x00: jump =  of(); break;
                            case 0x02: jump =  cf(); break;
                            case 0x04: jump =  zf(); break;
                            case 0x06: jump =  cf() || zf(); break;
                            case 0x08: jump =  sf(); break;
                            case 0x0a: jump =  pf(); break;
                            case 0x0c: jump = (sf() != of()); break;
                            case 0x0e: jump = (sf() != of()) || zf(); break;
                        }
                        if ((_opcode & 1) != 0)
                            jump = !jump;
                        if (jump)
                            jumpShort();
                        end(jump ? 16 : 4);
                    }
                    break;

                case stateALURMImm: readEA(stateALURMImm2); break;
                case stateALURMImm2:
                    _destination = _data;
                    fetch(stateALURMImm3, _opcode == 0x81);
                    break;
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
                        writeEA(_data, _wait);
                    else
                        _state = stateEndInstruction;
                    break;

                case stateTestRMReg: readEA(stateTestRMReg2); break;
                case stateTestRMReg2:
                    test(_data, getReg());
                    end(_useMemory ? 5 : 3);
                    break;
                case stateXchgRMReg: readEA(stateXchgRMReg2); break;
                case stateXchgRMReg2:
                    _source = getReg();
                    setReg(_data);
                    writeEA(_source, _useMemory ? 9 : 4);
                    break;
                case stateMovRMReg: loadEA(stateMovRMReg2); break;
                case stateMovRMReg2:
                    writeEA(getReg(), _useMemory ? 5 : 2);
                    break;
                case stateMovRegRM: readEA(stateMovRegRM2); break;
                case stateMovRegRM2:
                    setReg(_data);
                    end(_useMemory ? 4 : 2);
                    break;
                case stateMovRMWSegReg:
                    _wordSize = true;
                    loadEA(stateMovRMWSegReg2);
                    break;
                case stateMovRMWSegReg2:
                    writeEA(_segmentRegisters[modRMReg() & 3],
                        _useMemory ? 5 : 2);
                    break;
                case stateLEA: loadEA(stateLEA2); break;
                case stateLEA2: setReg(_address); end(2); break;
                case stateMovSegRegRMW:
                    _wordSize = true;
                    readEA(stateMovSegRegRMW2);
                    break;
                case stateMovSegRegRMW2:
                    _segmentRegisters[modRMReg() & 3] = _data;
                    end(_useMemory ? 4 : 2);
                    break;
                case statePopMW:
                    _afterIO = statePopMW2;
                    loadEA(statePop);
                    break;
                case statePopMW2: writeEA(_data, _useMemory ? 9 : 12); break;
                case stateXchgAxRW:
                    _data = rw();
                    rw() = ax();
                    ax() = _data;
                    end(3);
                    break;
                case stateCBW: ax() = signExtend(al()); end(2); break;
                case stateCWD:
                    dx() = ((ax() & 0x8000) == 0 ? 0x0000 : 0xffff);
                    end(5);
                    break;
                case stateCallCP: fetch(stateCallCP2, true); break;
                case stateCallCP2:
                    _savedIP = _data;
                    fetch(stateCallCP3, true);
                    break;
                case stateCallCP3:
                    _savedCS = _data;
                    _wait = 2;
                    push(cs(), stateCallCP4);
                    break;
                case stateCallCP4: push(_ip, stateJmpCP4); break;
                case stateWait: end(4); break;
                case statePushF: push((_flags & 0x0fd7) | 0xf000); break;
                case statePopF: pop(statePopF2); break;
                case statePopF2: _flags = _data | 2; end(4); break;
                case stateSAHF:
                    _flags = (_flags & 0xff02) | ah();
                    end(4);
                    break;
                case stateLAHF: ah() = _flags & 0xd7; end(4); break;

                case stateMovAccumInd: fetch(stateMovAccumInd2, true); break;
                case stateMovAccumInd2:
                    _address = _data;
                    initIO(stateMovAccumInd3, ioRead, _wordSize);
                    break;
                case stateMovAccumInd3: setAccum(); end(6); break;
                case stateMovIndAccum: fetch(stateMovIndAccum2, true); break;
                case stateMovIndAccum2:
                    _address = _data;
                    _data = getAccum();
                    _wait = 6;
                    initIO(stateEndInstruction, ioWrite, _wordSize);
                    break;

                case stateMovS:
                    _wait = (_rep != 0 ? 9 : 10);
                    if (repCheck())
                        lodS(stateMovS2);
                    break;
                case stateMovS2:
                    _afterRep = stateMovS;
                    stoS(stateRepAction);
                    break;
                case stateRepAction:
                    _state = stateEndInstruction;
                    if (_rep != 0 && cx() != 0) {
                        --cx();
                        _afterCheckInt = _afterRep;
                        if (cx() == 0)
                            _afterCheckInt = stateEndInstruction;
                        if ((_afterRep == stateCmpS || _afterRep == stateScaS)
                            && (zf() == (_rep == 1)))
                            _afterCheckInt = stateEndInstruction;
                        _state = stateCheckInt;
                    }
                    break;
                case stateCmpS:
                    _wait = 14;
                    if (repCheck())
                        lodS(stateCmpS2);
                    break;
                case stateCmpS2:
                    _destination = _data;
                    lodDIS(stateCmpS3);
                    break;
                case stateCmpS3:
                    _source = _data;
                    sub();
                    _afterRep = stateCmpS;
                    _state = stateRepAction;
                    break;

                case stateTestAccumImm:
                    fetch(stateTestAccumImm2, _wordSize);
                    break;
                case stateTestAccumImm2:
                    test(getAccum(), _data);
                    end(4);
                    break;

                case stateStoS:
                    _wait = (_rep != 0 ? 6 : 7);
                    if (repCheck()) {
                        _data = getAccum();
                        _afterRep = stateStoS;
                        stoS(stateRepAction);
                    }
                    break;
                case stateLodS:
                    _wait = (_rep != 0 ? 9 : 8);
                    if (repCheck())
                        lodS(stateLodS2);
                    break;
                case stateLodS2:
                    setAccum();
                    _afterRep = stateLodS;
                    _state = stateRepAction;
                    break;
                case stateScaS:
                    _wait = 11;
                    if (repCheck())
                        lodDIS(stateScaS2);
                    break;
                case stateScaS2:
                    _destination = getAccum();
                    _source = _data;
                    sub();
                    _afterRep = stateScaS;
                    _state = stateRepAction;
                    break;

                case stateMovRegImm:
                    _wordSize = ((_opcode & 8) != 0);
                    fetch(stateMovRegImm2, _wordSize);
                    break;
                case stateMovRegImm2:
                    if (_wordSize)
                        rw() = _data;
                    else
                        rb() = _data;
                    end(4);
                    break;

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
                case stateLoadFar2:
                    setReg(_data);
                    _afterIO = stateLoadFar3;
                    _state = stateLoadFar3;
                    break;
                case stateLoadFar3:
                    _segmentRegisters[!_wordSize ? 0 : 3] = _data;
                    end(8);
                    break;

                case stateMovRMImm: loadEA(stateMovRMImm2); break;
                case stateMovRMImm2: fetch(stateMovRMImm3, _wordSize); break;
                case stateMovRMImm3: writeEA(_data, _useMemory ? 6 : 4); break;

                case stateCheckInt:
                    _state = _afterCheckInt;
                    if (_nmiRequested) {
                        _nmiRequested = false;
                        interrupt(2);
                    }
                    if (intf() && _interruptRequested) {
                        initIO(stateHardwareInt, ioInterruptAcknowledge,
                            false);
                        _interruptRequested = false;
                        _wait = 1;
                    }
                    break;
                case stateHardwareInt:
                    _wait = 1;
                    _state = stateHardwareInt2;
                    break;
                case stateHardwareInt2:
                    initIO(stateIntAction, ioInterruptAcknowledge, false);
                    break;
                case stateInt3:
                    interrupt(3);
                    _wait = 1;
                    break;
                case stateInt:
                    fetch(stateIntAction, false);
                    break;
                case stateIntAction:
                    _source = _data;
                    push(_flags & 0x0fd7, stateIntAction2);
                    break;
                case stateIntAction2:
                    setIF(false);
                    setTF(false);
                    push(cs(), stateIntAction3);
                    break;
                case stateIntAction3: push(_ip, stateIntAction4); break;
                case stateIntAction4:
                    _address = _source << 2;
                    _segment = 1;
                    cs() = 0;
                    initIO(stateIntAction5, ioRead, true);
                    break;
                case stateIntAction5:
                    _savedIP = _data;
                    _address++;
                    initIO(stateIntAction6, ioRead, true);
                    break;
                case stateIntAction6:
                    cs() = _data;
                    setIP(_savedIP);
                    _wait = 13;
                    _state = stateEndInstruction;
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
                case stateIRet4:
                    _flags = _data | 2;
                    cs() = _savedCS;
                    setIP(_savedIP);
                    end(20);
                    break;

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
                                case 0xe0:
                                    if (zf())
                                        jump = false;
                                    _wait = 6;
                                    break;
                                case 0xe1: if (!zf()) jump = false; break;
                            }
                        }
                        else {
                            _wait = 6;
                            jump = (cx() == 0);
                        }
                        if (jump) {
                            jumpShort();
                            _wait = _opcode == 0xe0 ? 19 :
                                _opcode == 0xe2 ? 17 : 18;
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
                    _usePortSpace = true;
                    _ioType = ((_opcode & 2) == 0 ? ioRead : ioWrite);
                    _segment = 7;
                    _address = _data;
                    if (_ioType == ioWrite)
                        _data = getAccum();
                    initIO(stateInOut3, _ioType, _wordSize);
                    break;
                case stateInOut3:
                    if (_ioType == ioRead)
                        setAccum();
                    end(4);
                    break;

                case stateCallCW: fetch(stateCallCW2, true); break;
                case stateCallCW2:
                    _savedIP = _data + _ip;
                    _wait = 3;
                    _state = stateCallCW3;
                    break;
                case stateCallCW3: push(_ip, stateJmpCW3); break;

                case stateJmpCW: fetch(stateJmpCW2, true); break;
                case stateJmpCW2:
                    _savedIP = _data + _ip;
                    _wait = 9;
                    _state = stateJmpCW3;
                    break;
                case stateJmpCW3: setIP(_savedIP); end(6); break;

                case stateJmpCP: fetch(stateJmpCP2, true); break;
                case stateJmpCP2:
                    _savedIP = _data;
                    fetch(stateJmpCP3, true);
                    break;
                case stateJmpCP3:
                    _savedCS = _data;
                    _wait = 9;
                    _state = stateJmpCP4;
                    break;
                case stateJmpCP4: cs() = _savedCS; _state = stateJmpCW3; break;

                case stateJmpCB: fetch(stateJmpCB2, false); break;
                case stateJmpCB2: jumpShort(); end(15); break;

                case stateLock: end(2); break;
                case stateRep:
                    _rep = (_opcode == 0xf2 ? 1 : 2);
                    _wait = 9;
                    _state = stateBegin;
                    break;
                case stateHlt:
                    _wait = 1;
                    _state = stateHalted;
                    break;
                case stateHalted:
                    _wait = 1;
                    _state = stateCheckInt;
                    _afterCheckInt = stateHalted;
                    break;
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
                                        ah() -= _destination;
                                    if ((_destination & 0x80) != 0)
                                        ah() -= _source;
                                    setCF(ah() ==
                                        ((al() & 0x80) == 0 ? 0 : 0xff));
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
                                        dx() -= _destination;
                                    if ((_destination & 0x8000) != 0)
                                        dx() -= _source;
                                    _data |= dx();
                                    setCF(dx() ==
                                        ((ax() & 0x8000) == 0 ? 0 : 0xffff));
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
                                if (modRMReg() == 6) {
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
                                if (modRMReg() == 6) {
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
                        case 7:
                            push(_data, stateEndInstruction);
                            _wait += (_useMemory ? 2 : 1);
                            break;
                    }
                    break;
            }
        } while (true);
    }

    void setReady(Tick tick, bool ready)
    {
        runTo(tick);
        _ready = ready;
    }

    class Connector : public ConnectorBase<Connector>
    {
    public:
        Connector(Intel8088CPU* cpu) : ConnectorBase<Connector>(cpu) { }
        static String typeName() { return "Intel8088CPU.Connector"; }
        static auto protocolDirection()
        {
            return ProtocolDirection(CPU8088Protocol(), true);
        }
    protected:
        void connect(::Connector* other)
        {
            static_cast<Intel8088CPU*>(component())->_bus =
                static_cast<ISA8BitBus*>(other->component());
        }
    };
    class NMIConnector : public InputConnector<bool>
    {
    public:
        NMIConnector(Intel8088CPU* cpu) : InputConnector(cpu) { }
        void setData(Tick t, bool v)
        {
            if (v)
                static_cast<Intel8088CPU*>(component())->_nmiRequested = true;
        }
    };
    class IRQConnector : public InputConnector<bool>
    {
    public:
        IRQConnector(Intel8088CPU* cpu) : InputConnector(cpu) { }
        void setData(Tick t, bool v)
        {
            if (v)
                static_cast<Intel8088CPU*>(component())->_interruptRequested = true;
        }
    };

private:
    class PersistQueueType : public NamedNullary<::Type, PersistQueueType>
    {
    public:
        static String name() { return "PrefetchQueue"; }
        class Body : public NamedNullary<::Type, PersistQueueType>::Body
        {
        public:
            String serialize(void* p, int width, int used, int indent,
                int delta) const
            {
                auto cpu = static_cast<Intel8088CPU*>(p);

                String s = "{ ";
                bool needComma = false;
                for (int i = 0; i < cpu->_prefetched; ++i) {
                    if (needComma)
                        s += ", ";
                    needComma = true;
                    s += hex(cpu->_prefetchQueue[
                        (i + cpu->_prefetchOffset) & 3], 2);
                }
                return s + " }";
            }
            void deserialize(const Value& value, void* p) const
            {
                auto cpu = static_cast<Intel8088CPU*>(p);
                auto prefetch = value.value<List<Value>>();
                int prefetched = 0;
                for (auto i : prefetch) {
                      cpu->_prefetchQueue[prefetched] = i.value<int>();
                      ++prefetched;
                }
                cpu->_prefetched = prefetched;
                cpu->_prefetchOffset = 0;
            }
            Value defaultValue() const
            {
                return Value(this->type(), List<Value>());
            }
            Value value(void* p) const
            {
                auto cpu = static_cast<Intel8088CPU*>(p);
                List<Value> v;
                for (int i = 0; i < cpu->_prefetched; ++i) {
                    v.add(Value(IntegerType(), static_cast<int>(
                        cpu->_prefetchQueue[(i + cpu->_prefetchOffset) & 3])));
                }
                return Value(this->type(), v);
            }
        };
    };

    enum IOType
    {
        ioNone,
        ioRead,
        ioWrite,
        ioInstructionFetch,
        ioInterruptAcknowledge
    };
    enum State
    {
        stateWaitingForBIU,
        stateBegin, stateDecodeOpcode,
        stateEndInstruction,
        stateBeginModRM, stateDecodeModRM,
        stateEAOffset,
        stateEARegisters,
        stateEAByte,
        stateEAWord,
        stateEASetSegment,
        stateEAIO,
        statePush, statePush2,
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
        stateCheckInt, stateHardwareInt, stateHardwareInt2,
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
        stateLock, stateRep, stateHlt, stateHalted, stateCmC,
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
        bool dividendNegative = false;
        if (modRMReg() == 7) {
            if ((_destination & 0x80000000) != 0) {
                _destination =
                    static_cast<UInt32>(-static_cast<SInt32>(_destination));
                negative = !negative;
                dividendNegative = true;
            }
            if ((_source & 0x8000) != 0) {
                _source = (static_cast<UInt32>(-static_cast<SInt32>(_source)))
                    & 0xffff;
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
        if (negative)
            _data = static_cast<UInt32>(-static_cast<SInt32>(_data));
        if (dividendNegative)
            _remainder = static_cast<UInt32>(-static_cast<SInt32>(_remainder));
    }
    void jumpShort() { setIP(_ip + signExtend(_data)); }
    void interrupt(UInt8 number)
    {
        _data = number;
        _state = stateIntAction;
    }
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
    bool repCheck()
    {
        if (_rep != 0 && cx() == 0) {
            _state = stateRepAction;
            return false;
        }
        return true;
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
    void loadEA(State state) { _afterEA = state; _state = stateBeginModRM; }
    void readEA(State state)
    {
        _afterEAIO = state;
        _ioType = ioRead;
        loadEA(stateEAIO);
    }
    void fetch(State state, bool wordSize)
    {
        initIO(state, ioInstructionFetch, wordSize);
    }
    void writeEA(UInt16 data, int wait)
    {
        _data = data;
        _wait = wait;
        _afterEAIO = stateEndInstruction;
        _ioType = ioWrite;
        _state = stateEAIO;
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
    template<class U> class Register
    {
    public:
        void init(String name, U* data)
        {
            _name = name;
            _data = data;
            _read = false;
            _written = false;
        }
        const U& operator=(U value)
        {
            *_data = value;
            _writtenValue = value;
            _written = true;
            return *_data;
        }
        operator U()
        {
            if (!_read && !_written) {
                _read = true;
                _readValue = *_data;
            }
            return *_data;
        }
        const U& operator+=(U value)
        {
            return operator=(operator U() + value);
        }
        const U& operator-=(U value)
        {
            return operator=(operator U() - value);
        }
        const U& operator%=(U value)
        {
            return operator=(operator U() % value);
        }
        const U& operator&=(U value)
        {
            return operator=(operator U() & value);
        }
        const U& operator^=(U value)
        {
            return operator=(operator U() ^ value);
        }
        U operator-() const { return -operator U(); }
        void operator++() { operator=(operator U() + 1); }
        void operator--() { operator=(operator U() - 1); }
        String text()
        {
            String text;
            if (_read) {
                text = hex(_readValue, sizeof(U)==1 ? 2 : 4, false) + "<-" +
                    _name + "  ";
                _read = false;
            }
            if (_written) {
                text += _name + "<-" +
                    hex(_writtenValue, sizeof(U)==1 ? 2 : 4, false) + "  ";
                _written = false;
            }
            return text;
        }
    protected:
        String _name;
        U* _data;
        bool _read;
        bool _written;
        U _readValue;
        U _writtenValue;
    };
    class FlagsRegister : public Register<UInt16>
    {
    public:
        UInt32 operator=(UInt32 value)
        {
            Register<UInt16>::operator=(value);
            return Register<UInt16>::operator UInt16();
        }
        String text()
        {
            String text;
            if (this->_read) {
                text = flags(this->_readValue) + "<-" + this->_name + "  ";
                this->_read = false;
            }
            if (this->_written) {
                text += this->_name + "<-" + flags(this->_writtenValue) + "  ";
                this->_written = false;
            }
            return text;
        }
    private:
        String flags(UInt16 value) const
        {
            return String(value & 0x800 ? "O" : "p") +
                String(value & 0x400 ? "D" : "d") +
                String(value & 0x200 ? "I" : "i") +
                String(value & 0x100 ? "T" : "t") +
                String(value & 0x80 ? "S" : "s") +
                String(value & 0x40 ? "Z" : "z") +
                String(value & 0x10 ? "A" : "a") +
                String(value & 4 ? "P" : "p") +
                String(value & 1 ? "C" : "c");
        }
    };

    Register<UInt16>& rw() { return _wordRegisters[_opcode & 7]; }
    Register<UInt16>& ax() { return _wordRegisters[0]; }
    Register<UInt16>& cx() { return _wordRegisters[1]; }
    Register<UInt16>& dx() { return _wordRegisters[2]; }
    Register<UInt16>& bx() { return _wordRegisters[3]; }
    Register<UInt16>& sp() { return _wordRegisters[4]; }
    Register<UInt16>& bp() { return _wordRegisters[5]; }
    Register<UInt16>& si() { return _wordRegisters[6]; }
    Register<UInt16>& di() { return _wordRegisters[7]; }
    Register<UInt8>& rb() { return _byteRegisters[_opcode & 7]; }
    Register<UInt8>& al() { return _byteRegisters[0]; }
    Register<UInt8>& cl() { return _byteRegisters[1]; }
    Register<UInt8>& ah() { return _byteRegisters[4]; }
    Register<UInt16>& cs() { return _segmentRegisters[1]; }
    UInt16& csQuiet() { return _segmentRegisterData[1]; }
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
            ((_data & (!_wordSize ? 0xff : 0xffff)) == 0 ? 0x40 : 0);
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
    Register<UInt16>& modRMRW() { return _wordRegisters[modRMReg()]; }
    Register<UInt8>& modRMRB() { return _byteRegisters[modRMReg()]; }
    UInt16 getReg()
    {
        if (!_wordSize)
            return static_cast<UInt8>(modRMRB());
        return modRMRW();
    }
    UInt16 getAccum()
    {
        if (!_wordSize)
            return static_cast<UInt8>(al());
        return ax();
    }
    void setAccum() { if (!_wordSize) al() = _data; else ax() = _data; }
    void setReg(UInt16 value)
    {
        if (!_wordSize)
            modRMRB() = static_cast<UInt8>(value);
        else
            modRMRW() = value;
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
    void setIP(UInt16 value)
    {
        _ip = value;
        _abandonFetch = true;
        _prefetched = 0;
        _prefetchAddress = _ip;
    }
    UInt32 physicalAddress(UInt16 segment, UInt16 offset)
    {
        return ((_segmentRegisterData[segment] << 4) + offset) & 0xfffff;
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

    Register<UInt16> _wordRegisters[8];
    Register<UInt8> _byteRegisters[8];
    Register<UInt16> _segmentRegisters[8];
    UInt16 _registerData[8];      /* AX CX DX BX SP BP SI DI */
    UInt16 _segmentRegisterData[8];
    UInt16 _flagsData;
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
    FlagsRegister _flags;
    UInt16 _ip;
    UInt8 _prefetchQueue[4];
    UInt8 _prefetchOffset;
    UInt8 _prefetched;
    int _segment;
    int _segmentOverride;
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
    State _afterEAIO;
    State _afterRep;
    State _afterCheckInt;
    bool _sourceIsRM;
    UInt16 _savedCS;
    UInt16 _savedIP;
    int _rep;
    bool _usePortSpace;
    bool _halted;
    bool _newInstruction;
    UInt16 _newIP;
    ISA8BitBus* _bus;
    int _cycle;
    int _stopAtCycle;
    UInt32 _busAddress;
    UInt8 _busData;
    bool _nmiRequested;
    bool _interruptRequested;
    bool _ready;
    Tick _interruptTick;
    Tick _readyChangeTick;
    Array<bool> _visited;

    Disassembler _disassembler;

    Connector _connector;
    IRQConnector _irqConnector;
    NMIConnector _nmiConnector;
    Intel8259PIC* _pic;
};
