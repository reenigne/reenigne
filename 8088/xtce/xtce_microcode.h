class Disassembler
{
public:
    Disassembler() { reset(); }
    void reset() { _byteCount = 0; }
    String disassemble(Byte byte, bool firstByte)
    {
        String bytes;
        if (firstByte) {
            if (_byteCount != 0)
                bytes = "!a";
            _byteCount = 0;
        }
        _code[_byteCount] = byte;
        ++_byteCount;
        _lastOffset = 0;
        String instruction = disassembleInstruction();
        if (_lastOffset >= _byteCount)
            return bytes;  // We don't have the complete instruction yet
        _byteCount = 0;
        for (int i = 0; i <= _lastOffset; ++i)
            bytes += hex(_code[i], 2, false);
        return bytes.alignLeft(12) + " " + instruction;
    }
private:
    String disassembleInstruction()
    {
        _wordSize = (opcode() & 1) != 0;
        _doubleWord = false;
        _offset = 1;
        if ((opcode() & 0xc4) == 0)
            return alu(op1()) + regMemPair();
        if ((opcode() & 0xc6) == 4)
            return alu(op1()) + accum() + ", " + imm();
        if ((opcode() & 0xe7) == 6)
            return "PUSH " + segreg(op1());
        if ((opcode() & 0xe7) == 7)
            return "POP " + segreg(op1());
        if ((opcode() & 0xe7) == 0x26)
            return segreg(op1() & 3) + ":";
        if ((opcode() & 0xf8) == 0x40)
            return "INC " + rwo();
        if ((opcode() & 0xf8) == 0x48)
            return "DEC " + rwo();
        if ((opcode() & 0xf8) == 0x50)
            return "PUSH " + rwo();
        if ((opcode() & 0xf8) == 0x58)
            return "POP " + rwo();
        if ((opcode() & 0xfc) == 0x80)
            return alu(reg()) + ea() + ", " +
            (opcode() == 0x81 ? iw(true) : sb(true));
        if ((opcode() & 0xfc) == 0x88)
            return "MOV " + regMemPair();
        if ((opcode() & 0xf8) == 0x90)
            if (opcode() == 0x90)
                return "NOP";
            else
                return "XCHG AX, " + rwo();
        if ((opcode() & 0xf8) == 0xb0)
            return "MOV " + rbo() + ", " + ib();
        if ((opcode() & 0xf8) == 0xb8)
            return "MOV " + rwo() + ", " + iw();
        if ((opcode() & 0xfc) == 0xd0) {
            static String shifts[8] = {
                "ROL", "ROR", "RCL", "RCR", "SHL", "SHR", "SHL", "SAR"};
            return shifts[reg()] + " " + ea() + ", " +
                ((op0() & 2) == 0 ? String("1") : byteRegs(1));
        }
        if ((opcode() & 0xf8) == 0xd8) {
            _wordSize = false;
            _doubleWord = true;
            return String("ESC ") + op0() + ", " + reg() + ", " + ea();
        }
        if ((opcode() & 0xf6) == 0xe4)
            return "IN " + accum() + ", " + port();
        if ((opcode() & 0xf6) == 0xe6)
            return "OUT " + port() + ", " + accum();
        if ((opcode() & 0xe0) == 0x60) {
            static String conds[16] = {
                "O", "NO", "B", "AE", "E", "NE", "BE", "A",
                "S", "NS", "P", "NP", "L", "GE", "LE", "G"};
            return "J" + conds[opcode() & 0xf] + " " + cb();
        }
        switch (opcode()) {
            case 0x27: return "DAA";
            case 0x2f: return "DAS";
            case 0x37: return "AAA";
            case 0x3f: return "AAS";
            case 0x84:
            case 0x85: return "TEST " + regMemPair();
            case 0x86:
            case 0x87: return "XCHG " + regMemPair();
            case 0x8c:
                _wordSize = true;
                return "MOV " + ea() + ", " + segreg(reg());
            case 0x8d:
                _doubleWord = true;
                _wordSize = false;
                return "LEA " + rw() + ", " + ea();
            case 0x8e:
                _wordSize = true;
                return "MOV " + segreg(reg()) + ", " + ea();
            case 0x8f: return "POP " + ea();
            case 0x98: return "CBW";
            case 0x99: return "CWD";
            case 0x9a: return "CALL " + cp();
            case 0x9b: return "WAIT";
            case 0x9c: return "PUSHF";
            case 0x9d: return "POPF";
            case 0x9e: return "SAHF";
            case 0x9f: return "LAHF";
            case 0xa0:
            case 0xa1: return "MOV " + accum() + ", " + size() + "[" + iw() + "]";
            case 0xa2:
            case 0xa3: return "MOV " + size() + "[" + iw() + "], " + accum();
            case 0xa4:
            case 0xa5: return "MOVS" + size();
            case 0xa6:
            case 0xa7: return "CMPS" + size();
            case 0xa8:
            case 0xa9: return "TEST " + accum() + ", " + imm();
            case 0xaa:
            case 0xab: return "STOS" + size();
            case 0xac:
            case 0xad: return "LODS" + size();
            case 0xae:
            case 0xaf: return "SCAS" + size();
            case 0xc0:
            case 0xc2: return "RET " + iw();
            case 0xc1:
            case 0xc3: return "RET";
            case 0xc4: _doubleWord = true; return "LES " + rw() + ", " + ea();
            case 0xc5:
                _doubleWord = true;
                _wordSize = false;
                return "LDS " + rw() + ", " + ea();
            case 0xc6:
            case 0xc7: return "MOV " + ea() + ", " + imm(true);
            case 0xc8:
            case 0xca: return "RETF " + iw();
            case 0xc9:
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
            case 0xf0:
            case 0xf1: return "LOCK";
            case 0xf2: return "REPNE ";
            case 0xf3: return "REP ";
            case 0xf4: return "HLT";
            case 0xf5: return "CMC";
            case 0xf6:
            case 0xf7:
                switch (reg()) {
                    case 0:
                    case 1: return "TEST " + ea() + ", " + imm(true);
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
        return "!b";
    }
    UInt8 getByte(int offset)
    {
        _lastOffset = max(_lastOffset, offset);
        return _code[offset];
    }
    UInt16 getWord(int offset)
    {
        return getByte(offset) | (getByte(offset + 1) << 8);
    }
    String regMemPair()
    {
        if ((op0() & 2) == 0)
            return ea() + ", " + r();
        return r() + ", " + ea();
    }
    String r() { return !_wordSize ? rb() : rw(); }
    String rb() { return byteRegs(reg()); }
    String rw() { return wordRegs(reg()); }
    String rbo() { return byteRegs(op0()); }
    String rwo() { return wordRegs(op0()); }
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
            case 1: s = disp() + sb(); _offset = 3; break;
            case 2: s = disp() + "+" + iw(); _offset = 4; break;
            case 3: return !_wordSize ? byteRegs(rm()) : wordRegs(rm());
        }
        return size() + "[" + s + "]";
    }
    String size()
    {
        if (!_doubleWord)
            return (!_wordSize ? "B" : "W");
        else
            return (!_wordSize ? "" : "D");
    }
    String disp()
    {
        static String d[8] = {
            "BX+SI", "BX+DI", "BP+SI", "BP+DI", "SI", "DI", "BP", "BX"};
        if (mod() == 0 && rm() == 6) {
            String s = iw();
            _offset = 4;
            return s;
        }
        return d[rm()];
    }
    String alu(int op)
    {
        static String o[8] = {
            "ADD ", "OR ", "ADC ", "SBB ", "AND ", "SUB ", "XOR ", "CMP "};
        return o[op];
    }
    Byte opcode() { return getByte(0); }
    int op0() { return opcode() & 7; }
    int op1() { return (opcode() >> 3) & 7; }
    Byte modRM() { _offset = 2; return getByte(1); }
    int mod() { return modRM() >> 6; }
    int reg() { return (modRM() >> 3) & 7; }
    int rm() { return modRM() & 7; }
    String imm(bool m = false) { return !_wordSize ? ib(m) : iw(m); }
    String iw(bool m = false)
    {
        if (m)
            ea();
        return hex(getWord(_offset), 4, false);
    }
    String ib(bool m = false)
    {
        if (m)
            ea();
        return hex(getByte(_offset), 2, false);
    }
    String sb(bool m = false)
    {
        if (m)
            ea();
        UInt8 byte = getByte(_offset);
        if ((byte & 0x80) == 0)
            return "+" + hex(byte, 2, false);
        return "-" + hex(-byte, 2, false);
    }
    String accum() { return !_wordSize ? "AL" : "AX"; }
    String segreg(int r)
    {
        static String sr[8] = {"ES", "CS", "SS", "DS", "??", "??", "??", "??"};
        return sr[r];
    }
    String cb()
    {
        return "IP" + sb();
        //hex(_address + static_cast<SInt8>(getByte(_offset)), 4, false);
    }
    String cw()
    {
        return "IP+" + iw();
        //return hex(_address + getWord(_offset), 4, false);
    }
    String cp()
    {
        return hex(getWord(_offset + 2), 4, false) + ":" +
            hex(getWord(_offset), 4, false);
    }
    String port() { return ((op1() & 1) == 0 ? ib() : wordRegs(2)); }

    UInt16 _ip;
    UInt8 _code[6];
    int _byteCount;
    bool _wordSize;
    bool _doubleWord;
    int _offset;
    int _lastOffset;
};

class SnifferDecoder
{
public:
    void reset()
    {
        _cpu_rqgt0 = false;  // Used by 8087 for bus mastering, NYI
        _cpu_ready = true;   // Used for DMA and wait states
        _cpu_test = false;   // Used by 8087 for synchronization, NYI
        _cpu_lock = false;
        _bus_dma = 0;        // NYI
        _dmas = 0;
        _bus_irq = 0xfc;     // NYI
        _int = false;
        _bus_iochrdy = true; // Used for wait states, NYI
        _bus_aen = false;    // Used for DMA
        _bus_tc = false;     // Used for DMA, NYI
        _cga = 0;

        _t = 0;
        _tNext = 0;
        _d = -1;
        _queueLength = 0;
        _lastS = 0;
        _cpu_s = 7;
        _cpu_qs = 0;
        _cpu_next_qs = 0;

        _disassembler.reset();
    }
    String getLine()
    {
        static const char qsc[] = ".IES";
        static const char sc[] = "ARWHCrwp";
        static const char dmasc[] = " h:H";
        String line;
        if (_cpuDataFloating)
            line = String(hex(_cpu_ad >> 8, 3, false)) + "??";
        else
            line = String(hex(_cpu_ad, 5, false));
        line += " " +
            codePoint(qsc[_cpu_qs]) + codePoint(sc[_cpu_s]) +
            (_cpu_rqgt0 ? "G" : ".") + (_cpu_ready ? "." : "z") +
            (_cpu_test ? "T" : ".") + (_cpu_lock ? "L" : ".") +
            "  " + hex(_bus_address, 5, false) + " ";
        if (_isaDataFloating)
            line += "??";
        else
            line += hex(_bus_data, 2, false);
        line += " " + hex(_bus_dma, 2, false) + codePoint(dmasc[_dmas]) +
            " " + hex(_bus_irq, 2, false) + (_int ? "I" : " ") + " " +
            hex(_bus_pit, 1, false) + hex(_cga, 1, false) + " " + (_bus_ior ? "R" : ".") +
            (_bus_iow ? "W" : ".") + (_bus_memr ? "r" : ".") +
            (_bus_memw ? "w" : ".") + (_bus_iochrdy ? "." : "z") +
            (_bus_aen ? "D" : ".") +
            (_bus_tc ? "T" : ".");
        line += "  ";
        if (_cpu_s != 7 && _cpu_s != 3)
            switch (_tNext) {
                case 0:
                case 4:
                    // T1 state occurs after transition out of passive
                    _tNext = 1;
                    break;
                case 1:
                    _tNext = 2;
                    break;
                case 2:
                    _tNext = 3;
                    break;
                case 3:
                    _tNext = 5;
                    break;
            }
        else
            switch (_t) {
                case 4:
                    _d = -1;
                case 0:
                    _tNext = 0;
                    break;
                case 1:
                case 2:
                    _tNext = 6;
                    break;
                case 3:
                case 5:
                    _d = -1;
                    _tNext = 4;
                    break;
            }
        switch (_t) {
            case 0: line += "  "; break;
            case 1: line += "T1"; break;
            case 2: line += "T2"; break;
            case 3: line += "T3"; break;
            case 4: line += "T4"; break;
            case 5: line += "Tw"; break;
            default: line += "!c"; _tNext = 0; break;
        }
        line += " ";
        if (_bus_aen)
            switch (_d) {
                // This is a bit of a hack since we don't have access
                // to the right lines to determine the DMA state
                // properly. This probably breaks for memory-to-memory
                // copies.
                case -1: _d = 0; break;
                case 0: _d = 1; break;
                case 1: _d = 2; break;
                case 2: _d = 3; break;
                case 3:
                case 5:
                    if ((_bus_iow && _bus_memr) || (_bus_ior && _bus_memw))
                        _d = 4;
                    else
                        _d = 5;
                    break;
                case 4:
                    _d = -1;
            }
        switch (_d) {
            case -1: line += "  "; break;
            case 0: line += "S0"; break;
            case 1: line += "S1"; break;
            case 2: line += "S2"; break;
            case 3: line += "S3"; break;
            case 4: line += "S4"; break;
            case 5: line += "SW"; break;
            default: line += "!d"; _t = 0; break;
        }
        line += " ";
        String instruction;
        if (_cpu_qs != 0) {
            if (_cpu_qs == 2)
                _queueLength = 0;
            else {
                Byte b = _queue[0];
                for (int i = 0; i < 3; ++i)
                    _queue[i] = _queue[i + 1];
                --_queueLength;
                if (_queueLength < 0) {
                    line += "!g";
                    _queueLength = 0;
                }
                instruction = _disassembler.disassemble(b, _cpu_qs == 1);
            }
        }
        if (_tNext == 4 || _d == 4) {
            if (_tNext == 4 && _d == 4)
                line += "!e";
            String seg;
            switch (_cpu_ad & 0x30000) {
                case 0x00000: seg = "ES "; break;
                case 0x10000: seg = "SS "; break;
                case 0x20000: seg = "CS "; break;
                case 0x30000: seg = "DS "; break;
            }
            String type = "-";
            if (_lastS == 0)
                line += hex(_bus_data, 2, false) + " <-i           ";
            else {
                if (_lastS == 4) {
                    type = "f";
                    seg = "   ";
                }
                if (_d == 4) {
                    type = "d";
                    seg = "   ";
                }
                line += hex(_bus_data, 2, false) + " ";
                if (_bus_ior || _bus_memr)
                    line += "<-" + type + " ";
                else
                    line += type + "-> ";
                if (_bus_memr || _bus_memw)
                    line += "[" + seg + hex(_bus_address, 5, false) + "]";
                else
                    line += "port[" + hex(_bus_address, 4, false) + "]";
                if (_lastS == 4 && _d != 4) {
                    if (_queueLength >= 4)
                        line += "!f";
                    else {
                        _queue[_queueLength] = _bus_data;
                        ++_queueLength;
                    }
                }
            }
            line += " ";
        }
        else
            line += "                  ";
        if (_cpu_qs != 0)
            line += codePoint(qsc[_cpu_qs]);
        else
            line += " ";
        line += " " + instruction;
        _lastS = _cpu_s;
        _t = _tNext;
        if (_t == 4 || _d == 4) {
            _bus_ior = false;
            _bus_iow = false;
            _bus_memr = false;
            _bus_memw = false;
        }
        // 8086 Family Users Manual page 4-37 clock cycle 12: "remember
        // the queue status lines indicate queue activity that has occurred in
        // the previous clock cycle".
        _cpu_qs = _cpu_next_qs;
        _cpu_next_qs = 0;
        return line;
    }
    void queueOperation(int qs) { _cpu_next_qs = qs; }
    void setStatus(int s) { _cpu_s = s; }
    void setStatusHigh(int segment)
    {
        _cpu_ad &= 0xcffff;
        switch (segment) {
            case 0:  // ES
                break;
            case 1:  // CS or none
                _cpu_ad |= 0x20000;
                break;
            case 2:  // SS
                _cpu_ad |= 0x10000;
                break;
            case 3:  // DS
                _cpu_ad |= 0x30000;
                break;
        }
        setBusFloating();
    }
    void setInterruptFlag(bool intf)
    {
        _cpu_ad = (_cpu_ad & 0xbffff) | (intf ? 0x40000 : 0);
    }
    void setBusOperation(int s)
    {
        switch (s) {
            case 1: _bus_ior = true; break;
            case 2: _bus_iow = true; break;
            case 4:
            case 5: _bus_memr = true; break;
            case 6: _bus_memw = true; break;
        }
    }
    void setData(Byte data)
    {
        _cpu_ad = (_cpu_ad & 0xfff00) | data;
        _bus_data = data;
        _cpuDataFloating = false;
        _isaDataFloating = false;
    }
    void setAddress(UInt32 address)
    {
        _cpu_ad = address;
        _bus_address = address;
        _cpuDataFloating = false;
    }
    void setBusFloating()
    {
        _cpuDataFloating = true;
        _isaDataFloating = true;
    }
    void setPITBits(int bits) { _bus_pit = bits; }
    void setAEN(bool aen) { _bus_aen = aen; }
    void setDMA(UInt8 dma) { _bus_dma = dma; }
    void setReady(bool ready) { _cpu_ready = ready; }
    void setLock(bool lock) { _cpu_lock = lock; }
    void setDMAS(UInt8 dmas) { _dmas = dmas; }
    void setIRQs(UInt8 irq) { _bus_irq = irq; }
    void setINT(bool intrq) { _int = intrq; }
    void setCGA(UInt8 cga) { _cga = cga; }
private:
    Disassembler _disassembler;

    // Internal variables that we use to keep track of what's going on in order
    // to be able to print useful logs.
    int _t;  // 0 = Tidle, 1 = T1, 2 = T2, 3 = T3, 4 = T4, 5 = Tw
    int _tNext;
    int _d;  // -1 = SI, 0 = S0, 1 = S1, 2 = S2, 3 = S3, 4 = S4, 5 = SW
    Byte _queue[4];
    int _queueLength;
    int _lastS;

    // These represent the CPU and ISA bus pins used to create the sniffer
    // logs.
    UInt32 _cpu_ad;
    // A19/S6        O ADDRESS/STATUS: During T1, these are the four most significant address lines for memory operations. During I/O operations, these lines are LOW. During memory and I/O operations, status information is available on these lines during T2, T3, Tw, and T4. S6 is always low.
    // A18/S5        O The status of the interrupt enable flag bit (S5) is updated at the beginning of each clock cycle.
    // A17/S4        O  S4*2+S3 0 = Alternate Data, 1 = Stack, 2 = Code or None, 3 = Data
    // A16/S3        O
    // A15..A8       O ADDRESS BUS: These lines provide address bits 8 through 15 for the entire bus cycle (T1±T4). These lines do not have to be latched by ALE to remain valid. A15±A8 are active HIGH and float to 3-state OFF during interrupt acknowledge and local bus ``hold acknowledge''.
    // AD7..AD0     IO ADDRESS DATA BUS: These lines constitute the time multiplexed memory/IO address (T1) and data (T2, T3, Tw, T4) bus. These lines are active HIGH and float to 3-state OFF during interrupt acknowledge and local bus ``hold acknowledge''.
    UInt8 _cpu_qs;
    UInt8 _cpu_next_qs;
    // QS0           O QUEUE STATUS: provide status to allow external tracking of the internal 8088 instruction queue. The queue status is valid during the CLK cycle after which the queue operation is performed.
    // QS1           0 = No operation, 1 = First Byte of Opcode from Queue, 2 = Empty the Queue, 3 = Subsequent Byte from Queue
    UInt8 _cpu_s;
    // -S0           O STATUS: is active during clock high of T4, T1, and T2, and is returned to the passive state (1,1,1) during T3 or during Tw when READY is HIGH. This status is used by the 8288 bus controller to generate all memory and I/O access control signals. Any change by S2, S1, or S0 during T4 is used to indicate the beginning of a bus cycle, and the return to the passive state in T3 and Tw is used to indicate the end of a bus cycle. These signals float to 3-state OFF during ``hold acknowledge''. During the first clock cycle after RESET becomes active, these signals are active HIGH. After this first clock, they float to 3-state OFF.
    // -S1           0 = Interrupt Acknowledge, 1 = Read I/O Port, 2 = Write I/O Port, 3 = Halt, 4 = Code Access, 5 = Read Memory, 6 = Write Memory, 7 = Passive
    // -S2
    bool _cpu_rqgt0;    // -RQ/-GT0 !87 IO REQUEST/GRANT: pins are used by other local bus masters to force the processor to release the local bus at the end of the processor's current bus cycle. Each pin is bidirectional with RQ/GT0 having higher priority than RQ/GT1. RQ/GT has an internal pull-up resistor, so may be left unconnected.
    bool _cpu_ready;    // READY        I  READY: is the acknowledgement from the addressed memory or I/O device that it will complete the data transfer. The RDY signal from memory or I/O is synchronized by the 8284 clock generator to form READY. This signal is active HIGH. The 8088 READY input is not synchronized. Correct operation is not guaranteed if the set up and hold times are not met.
    bool _cpu_test;     // -TEST        I  TEST: input is examined by the ``wait for test'' instruction. If the TEST input is LOW, execution continues, otherwise the processor waits in an ``idle'' state. This input is synchronized internally during each clock cycle on the leading edge of CLK.
    bool _cpu_lock;     // -LOCK    !87  O LOCK: indicates that other system bus masters are not to gain control of the system bus while LOCK is active (LOW). The LOCK signal is activated by the ``LOCK'' prefix instruction and remains active until the completion of the next instruction. This signal is active LOW, and floats to 3-state off in ``hold acknowledge''.
    UInt32 _bus_address;
    // +A19..+A0      O Address bits: These lines are used to address memory and I/O devices within the system. These lines are generated by either the processor or DMA controller.
    UInt8 _bus_data;
    // +D7..+D0      IO Data bits: These lines provide data bus bits 0 to 7 for the processor, memory, and I/O devices.
    UInt8 _bus_dma;
    // +DRQ0 JP6/1 == U28.19 == U73.9
    // +DRQ1..+DRQ3  I  DMA Request: These lines are asynchronous channel requests used by peripheral devices to gain DMA service. They are prioritized with DRQ3 being the lowest and DRQl being the highest. A request is generated by bringing a DRQ line to an active level (high). A DRQ line must be held high until the corresponding DACK line goes active.
    // -DACK0..-DACK3 O -DMA Acknowledge: These lines are used to acknowledge DMA requests (DRQ1-DRQ3) and to refresh system dynamic memory (DACK0). They are active low.
    UInt8 _dmas;        // JP9/4 HRQ DMA (bit 0), JP4/1 HOLDA (bit 1)
    UInt8 _bus_irq;
    // +IRQ0..+IRQ7  I  Interrupt Request lines: These lines are used to signal the processor that an I/O device requires attention. An Interrupt Request is generated by raising an IRQ line (low to high) and holding it high until it is acknowledged by the processor (interrupt service routine).
    bool _int;          // JP9/1 INT
    UInt8 _cga;         // JP7/2  CGA HCLK (bit 0), JP7/1  CGA LCLK (bit 1)
    UInt8 _bus_pit;     // clock, gate, output
    bool _bus_ior;      // -IOR         O -I/O Read Command: This command line instructs an I/O device to drive its data onto the data bus. It may be driven by the processor or the DMA controller. This signal is active low.
    bool _bus_iow;      // -IOW         O -I/O Write Command: This command line instructs an I/O device to read the data on the data bus. It may be driven by the processor or the DMA controller. This signal is active low.
    bool _bus_memr;     // -MEMR        O Memory Read Command: This command line instructs the memory to drive its data onto the data bus. It may be driven by the processor or the DMA controller. This signal is active low.
    bool _bus_memw;     // -MEMW        O Memory Write Command: This command line instructs the memory to store the data present on the data bus. It may be driven by the processor or the DMA controller. This signal is active low.
    bool _bus_iochrdy;  // +I/O CH RDY I  I/O Channel Ready: This line, normally high (ready), is pulled low (not ready) by a memory or I/O device to lengthen I/O or memory cycles. It allows slower devices to attach to the I/O channel with a minimum of difficulty. Any slow device using this line should drive it low immediately upon detecting a valid address and a read or write command. This line should never be held low longer than 10 clock cycles. Machine cycles (I/O or memory) are extended by an integral number of CLK cycles (210 ns).
    bool _bus_aen;      // +AEN         O Address Enable: This line is used to de-gate the processor and other devices from the I/O channel to allow DMA transfers to take place. When this line is active (high), the DMA controller has control of the address bus, data bus, read command lines (memory and I/O), and the write command lines (memory and I/O).
    bool _bus_tc;       // +T/C         O Terminal Count: This line provides a pulse when the terminal count for any DMA channel is reached. This signal is active high.
    bool _cpuDataFloating;
    bool _isaDataFloating;
};

class PITEmulator
{
public:
    void reset()
    {
        for (int i = 0; i < 3; ++i)
            _counters[i].reset();
    }
    void stubInit()
    {
        for (int i = 0; i < 3; ++i)
            _counters[i].stubInit();
    }
    void write(int address, Byte data)
    {
        if (address == 3) {
            int counter = data >> 6;
            if (counter == 3)
                return;
            _counters[counter].control(data & 0x3f);
        }
        else
            _counters[address].write(data);
    }
    Byte read(int address)
    {
        if (address == 3)
            return 0xff;
        return _counters[address].read();
    }
    void wait()
    {
        for (int i = 0; i < 3; ++i)
            _counters[i].wait();
    }
    void setGate(int counter, bool gate)
    {
        _counters[counter].setGate(gate);
    }
    bool getOutput(int counter) { return _counters[counter]._output; }
    //int getMode(int counter) { return _counters[counter]._control; }
private:
    enum State
    {
        stateWaitingForCount,
        stateCounting,
        stateWaitingForGate,
        stateGateRose,
        stateLoadDelay,
        statePulsing
    };
    struct Counter
    {
        void reset()
        {
            _value = 0;
            _count = 0;
            _firstByte = true;
            _latched = false;
            _output = true;
            _control = 0x30;
            _state = stateWaitingForCount;
        }
        void stubInit()
        {
            _value = 0xffff;
            _count = 0xffff;
            _firstByte = true;
            _latched = false;
            _output = true;
            _control = 0x34;
            _state = stateCounting;
            _gate = true;
        }
        void write(Byte data)
        {
            _writeByte = data;
            _haveWriteByte = true;
        }
        Byte read()
        {
            if (!_latched) {
                // TODO: corrupt countdown in a deterministic but
                // non-trivial way.
                _latch = _count;
            }
            switch (_control & 0x30) {
                case 0x10:
                    _latched = false;
                    return _latch & 0xff;
                case 0x20:
                    _latched = false;
                    return _latch >> 8;
                case 0x30:
                    if (_firstByte) {
                        _firstByte = false;
                        return _latch & 0xff;
                    }
                    _firstByte = true;
                    _latched = false;
                    return _latch >> 8;
            }
            // This should never happen.
            return 0;
        }
        void wait()
        {
            switch (_control & 0x0e) {
                case 0x00:  // Interrupt on Terminal Count
                    if (_state == stateLoadDelay) {
                        _state = stateCounting;
                        _value = _count;
                        break;
                    }
                    if (_gate && _state == stateCounting) {
                        countDown();
                        if (_value == 0)
                            _output = true;
                    }
                    break;
                case 0x02:  // Programmable One-Shot
                    if (_state == stateLoadDelay) {
                        _state = stateWaitingForGate;
                        break;
                    }
                    if (_state == stateGateRose) {
                        _output = false;
                        _value = _count;
                        _state = stateCounting;
                    }
                    countDown();
                    if (_value == 0) {
                        _output = true;
                        _state = stateWaitingForGate;
                    }
                    break;
                case 0x04:
                case 0x0c:  // Rate Generator
                    if (_state == stateLoadDelay) {
                        _state = stateCounting;
                        _value = _count;
                        break;
                    }
                    if (_gate && _state == stateCounting) {
                        countDown();
                        if (_value == 1)
                            _output = false;
                        if (_value == 0) {
                            _output = true;
                            _value = _count;
                        }
                    }
                    break;
                case 0x06:
                case 0x0e:  // Square Wave Rate Generator
                    if (_state == stateLoadDelay) {
                        _state = stateCounting;
                        _value = _count;
                        break;
                    }
                    if (_gate && _state == stateCounting) {
                        if ((_value & 1) != 0) {
                            if (!_output) {
                                countDown();
                                countDown();
                            }
                        }
                        else
                            countDown();
                        countDown();
                        if (_value == 0) {
                            _output = !_output;
                            _value = _count;
                        }
                    }
                    break;
                case 0x08:  // Software Triggered Strobe
                    if (_state == stateLoadDelay) {
                        _state = stateCounting;
                        _value = _count;
                        break;
                    }
                    if (_state == statePulsing) {
                        _output = true;
                        _state = stateWaitingForCount;
                    }
                    if (_gate && _state == stateCounting) {
                        countDown();
                        if (_value == 0) {
                            _output = false;
                            _state = statePulsing;
                        }
                    }
                    break;
                case 0x0a:  // Hardware Triggered Strobe
                    if (_state == stateLoadDelay) {
                        _state = stateWaitingForGate;
                        break;
                    }
                    if (_state == statePulsing) {
                        _output = true;
                        _state = stateWaitingForCount;
                    }
                    if (_state == stateGateRose) {
                        _output = false;
                        _value = _count;
                        _state = stateCounting;
                    }
                    if (_state == stateCounting) {
                        countDown();
                        if (_value == 1)
                            _output = false;
                        if (_value == 0) {
                            _output = true;
                            _state = stateWaitingForGate;
                        }
                    }
                    break;
            }
            if (_haveWriteByte) {
                _haveWriteByte = false;
                switch (_control & 0x30) {
                    case 0x10:
                        load(_writeByte);
                        break;
                    case 0x20:
                        load(_writeByte << 8);
                        break;
                    case 0x30:
                        if (_firstByte) {
                            _lowByte = _writeByte;
                            _firstByte = false;
                        }
                        else {
                            load((_writeByte << 8) + _lowByte);
                            _firstByte = true;
                        }
                        break;
                }
            }
        }
        void countDown()
        {
            if ((_control & 1) == 0) {
                --_value;
                return;
            }
            if ((_value & 0xf) != 0) {
                --_value;
                return;
            }
            if ((_value & 0xf0) != 0) {
                _value -= (0x10 - 9);
                return;
            }
            if ((_value & 0xf00) != 0) {
                _value -= (0x100 - 0x99);
                return;
            }
            _value -= (0x1000 - 0x999);
        }
        void load(Word count)
        {
            _count = count;
            switch (_control & 0x0e) {
                case 0x00:  // Interrupt on Terminal Count
                    if (_state == stateWaitingForCount)
                        _state = stateLoadDelay;
                    _output = false;
                    break;
                case 0x02:  // Programmable One-Shot
                    if (_state != stateCounting)
                        _state = stateLoadDelay;
                    break;
                case 0x04:
                case 0x0c:  // Rate Generator
                    if (_state == stateWaitingForCount)
                        _state = stateLoadDelay;
                    break;
                case 0x06:
                case 0x0e:  // Square Wave Rate Generator
                    if (_state == stateWaitingForCount)
                        _state = stateLoadDelay;
                    break;
                case 0x08:  // Software Triggered Strobe
                    if (_state == stateWaitingForCount)
                        _state = stateLoadDelay;
                    break;
                case 0x0a:  // Hardware Triggered Strobe
                    if (_state != stateCounting)
                        _state = stateLoadDelay;
                    break;
            }
        }
        void control(Byte control)
        {
            int command = control & 0x30;
            if (command == 0) {
                _latch = _value;
                _latched = true;
                return;
            }
            _control = control;
            _firstByte = true;
            _latched = false;
            _state = stateWaitingForCount;
            switch (_control & 0x0e) {
                case 0x00:  // Interrupt on Terminal Count
                    _output = false;
                    break;
                case 0x02:  // Programmable One-Shot
                    _output = true;
                    break;
                case 0x04:
                case 0x0c:  // Rate Generator
                    _output = true;
                    break;
                case 0x06:
                case 0x0e:  // Square Wave Rate Generator
                    _output = true;
                    break;
                case 0x08:  // Software Triggered Strobe
                    _output = true;
                    break;
                case 0x0a:  // Hardware Triggered Strobe
                    _output = true;
                    break;
            }
        }
        void setGate(bool gate)
        {
            if (_gate == gate)
                return;
            switch (_control & 0x0e) {
                case 0x00:  // Interrupt on Terminal Count
                    break;
                case 0x02:  // Programmable One-Shot
                    if (gate)
                        _state = stateGateRose;
                    break;
                case 0x04:
                case 0x0c:  // Rate Generator
                    if (!gate)
                        _output = true;
                    else
                        _value = _count;
                    break;
                case 0x06:
                case 0x0e:  // Square Wave Rate Generator
                    if (!gate)
                        _output = true;
                    else
                        _value = _count;
                    break;
                case 0x08:  // Software Triggered Strobe
                    break;
                case 0x0a:  // Hardware Triggered Strobe
                    if (gate)
                        _state = stateGateRose;
                    break;
            }
            _gate = gate;
        }

        Word _count;
        Word _value;
        Word _latch;
        Byte _control;
        Byte _lowByte;
        bool _gate;
        bool _output;
        bool _firstByte;
        bool _latched;
        State _state;
        Byte _writeByte;
        bool _haveWriteByte;
    };

    Counter _counters[3];
};

class PICEmulator
{
public:
    void reset()
    {
        _interruptPending = false;
        _interrupt = 0;
        _irr = 0;
        _imr = 0;
        _isr = 0;
        _icw1 = 0;
        _icw2 = 0;
        _icw3 = 0;
        _icw4 = 0;
        _ocw3 = 0;
        _lines = 0;
        _specialMaskMode = false;
        _acknowledgedBytes = 0;
        _priority = 0;
        _rotateInAutomaticEOIMode = false;
        _initializationState = initializationStateNone;
    }
    void stubInit()
    {
        _icw1 = 0x13;
        _icw2 = 0x08;
        _icw4 = 0x0f;
        _imr = 0xbc;
    }
    void write(int address, Byte data)
    {
        if (address == 0) {
            if ((data & 0x10) != 0) {
                _icw1 = data;
                if (levelTriggered())
                    _irr = _lines;
                else
                    _irr = 0;
                _initializationState = initializationStateICW2;
                _imr = 0;
                _isr = 0;
                _icw2 = 0;
                _icw3 = 0;
                _icw4 = 0;
                _ocw3 = 0;
                _acknowledgedBytes = 0;
                _priority = 0;
                _rotateInAutomaticEOIMode = false;
                _specialMaskMode = false;
                _interrupt = 0;
                _interruptPending = false;
            }
            else {
                if ((data & 8) == 0) {
                    Byte b = 1 << (data & 7);
                    switch (data & 0xe0) {
                        case 0x00:  // Rotate in automatic EOI mode (clear) (Automatic Rotation)
                            _rotateInAutomaticEOIMode = false;
                            break;
                        case 0x20:  // Non-specific EOI command (End of Interrupt)
                            nonSpecificEOI(false);
                            break;
                        case 0x40:  // No operation
                            break;
                        case 0x60:  // Specific EOI command (End of Interrupt)
                            _isr &= ~b;
                            break;
                        case 0x80:  // Rotate in automatic EOI mode (set) (Automatic Rotation)
                            _rotateInAutomaticEOIMode = true;
                            break;
                        case 0xa0:  // Rotate on non-specific EOI command (Automatic Rotation)
                            nonSpecificEOI(true);
                            break;
                        case 0xc0:  // Set priority command (Specific Rotation)
                            _priority = (data + 1) & 7;
                            break;
                        case 0xe0:  // Rotate on specific EOI command (Specific Rotation)
                            if ((_isr & b) != 0) {
                                _isr &= ~b;
                                _priority = (data + 1) & 7;
                            }
                            break;
                    }
                }
                else {
                    _ocw3 = data;
                    if ((_ocw3 & 0x40) != 0)
                        _specialMaskMode = (_ocw3 & 0x20) != 0;
                }
            }
        }
        else {
            switch (_initializationState) {
                case initializationStateICW2:
                    _icw2 = data;
                    if (cascadeMode())
                        _initializationState = initializationStateICW3;
                    else
                        checkICW4Needed();
                    break;
                case initializationStateICW3:
                    _icw3 = data;
                    checkICW4Needed();
                    break;
                case initializationStateICW4:
                    _icw4 = data;
                    _initializationState = initializationStateNone;
                    break;
                case initializationStateNone:
                    _imr = data;
                    break;
            }
        }
    }
    Byte read(int address)
    {
        if ((_ocw3 & 4) != 0) {  // Poll mode
            acknowledge();
            return (_interruptPending ? 0x80 : 0) + _interrupt;
        }
        if (address == 0) {
            if ((_ocw3 & 1) != 0)
                return _isr;
            return _irr;
        }
        else
            return _imr;
    }
    Byte interruptAcknowledge()
    {
        if (_acknowledgedBytes == 0) {
            acknowledge();
            _acknowledgedBytes = 1;
            if (i86Mode())
                return 0xff;
            else
                return 0xcd;
        }
        if (i86Mode()) {
            _acknowledgedBytes = 0;
            if (autoEOI())
                nonSpecificEOI(_rotateInAutomaticEOIMode);
            _interruptPending = false;
            if (slaveOn(_interrupt))
                return 0xff;  // Filled in by slave PIC
            return _interrupt + (_icw2 & 0xf8);
        }
        if (_acknowledgedBytes == 1) {
            _acknowledgedBytes = 2;
            if (slaveOn(_interrupt))
                return 0xff;  // Filled in by slave PIC
            if ((_icw1 & 4) != 0)  // Call address interval 4
                return (_interrupt << 2) + (_icw1 & 0xe0);
            return (_interrupt << 3) + (_icw1 & 0xc0);
        }
        _acknowledgedBytes = 0;
        if (autoEOI())
            nonSpecificEOI(_rotateInAutomaticEOIMode);
        _interruptPending = false;
        if (slaveOn(_interrupt))
            return 0xff;  // Filled in by slave PIC
        return _icw2;
    }
    void setIRQLine(int line, bool state)
    {
        Byte b = 1 << line;
        if (state) {
            if (levelTriggered() || (_lines & b) == 0)
                _irr |= b;
            _lines |= b;
        }
        else {
            _irr &= ~b;
            _lines &= ~b;
        }
        if (findBestInterrupt() != -1)
            _interruptPending = true;
    }
    bool interruptPending() const { return _interruptPending; }
    UInt8 getIRQLines() { return _lines; }
private:
    bool cascadeMode() { return (_icw1 & 2) == 0; }
    bool levelTriggered() { return (_icw1 & 8) != 0; }
    bool i86Mode() { return (_icw4 & 1) != 0; }
    bool autoEOI() { return (_icw4 & 2) != 0; }
    bool slaveOn(int channel)
    {
        return cascadeMode() && (_icw4 & 0xc0) == 0xc0 &&
            (_icw3 & (1 << channel)) != 0;
    }
    int findBestInterrupt()
    {
        int n = _priority;
        for (int i = 0; i < 8; ++i) {
            Byte b = 1 << n;
            bool s = (_icw4 & 0x10) != 0 && slaveOn(n);
            if ((_isr & b) != 0 && !_specialMaskMode && !s)
                break;
            if ((_irr & b) != 0 && (_imr & b) == 0 && ((_isr & b) == 0 || s))
                return n;
            if ((_isr & b) != 0 && !_specialMaskMode && s)
                break;
            n = (n + 1) & 7;
        }
        return -1;
    }
    void acknowledge()
    {
        int i = findBestInterrupt();
        if (i == -1) {
            _interrupt = 7;
            return;
        }
        Byte b = 1 << i;
        _isr |= b;
        if (!levelTriggered())
            _irr &= ~b;
    }
    void nonSpecificEOI(bool rotatePriority = false)
    {
        int n = _priority;
        for (int i = 0; i < 8; ++i) {
            Byte b = 1 << n;
            n = (n + 1) & 7;
            if ((_isr & b) != 0 && (_imr & b) == 0) {
                _isr &= ~b;
                if (rotatePriority)
                    _priority = n & 7;
                break;
            }
        }
    }
    void checkICW4Needed()
    {
        if ((_icw1 & 1) != 0)
            _initializationState = initializationStateICW4;
        else
            _initializationState = initializationStateNone;
    }

    enum InitializationState
    {
        initializationStateNone,
        initializationStateICW2,
        initializationStateICW3,
        initializationStateICW4
    };
    bool _interruptPending;
    int _interrupt;
    Byte _irr;
    Byte _imr;
    Byte _isr;
    Byte _icw1;
    Byte _icw2;
    Byte _icw3;
    Byte _icw4;
    Byte _ocw3;
    Byte _lines;
    int _acknowledgedBytes;
    int _priority;
    bool _specialMaskMode;
    bool _rotateInAutomaticEOIMode;
    InitializationState _initializationState;
};

class DMACEmulator
{
public:
    void reset()
    {
        for (int i = 0; i < 4; ++i)
            _channels[i].reset();
        _temporaryAddress = 0;
        _temporaryWordCount = 0;
        _status = 0;
        _command = 0;
        _temporary = 0;
        _mask = 0xf;
        _request = 0;
        _high = false;
        _channel = -1;
        _needHighAddress = true;
    }
    void write(int address, Byte data)
    {
        switch (address) {
            case 0x00: case 0x02: case 0x04: case 0x06:
                _channels[(address & 6) >> 1].setAddress(_high, data);
                _high = !_high;
                break;
            case 0x01: case 0x03: case 0x05: case 0x07:
                _channels[(address & 6) >> 1].setCount(_high, data);
                _high = !_high;
                break;
            case 0x08:  // Write Command Register
                _command = data;
                break;
            case 0x09:  // Write Request Register
                setRequest(data & 3, (data & 4) != 0);
                break;
            case 0x0a:  // Write Single Mask Register Bit
                {
                    Byte b = 1 << (data & 3);
                    if ((data & 4) != 0)
                        _mask |= b;
                    else
                        _mask &= ~b;
                }
                break;
            case 0x0b:  // Write Mode Register
                _channels[data & 3]._mode = data;
                break;
            case 0x0c:  // Clear Byte Pointer Flip/Flop
                _high = false;
                break;
            case 0x0d:  // Master Clear
                reset();
                break;
            case 0x0e:  // Clear Mask Register
                _mask = 0;
                break;
            case 0x0f:  // Write All Mask Register Bits
                _mask = data;
                break;
        }
    }
    Byte read(int address)
    {
        switch (address) {
            case 0x00: case 0x02: case 0x04: case 0x06:
                _high = !_high;
                return _channels[(address & 6) >> 1].getAddress(!_high);
            case 0x01: case 0x03: case 0x05: case 0x07:
                _high = !_high;
                return _channels[(address & 6) >> 1].getCount(!_high);
            case 0x08:  // Read Status Register
                return _status;
                break;
            case 0x0d:  // Read Temporary Register
                return _temporary;
                break;
            default:  // Illegal
                return 0xff;
        }
    }
    void setDMARequestLine(int line, bool state)
    {
        setRequest(line, state != dreqSenseActiveLow());
    }
    bool getHoldRequestLine()
    {
        if (_channel != -1)
            return true;
        if (disabled())
            return false;
        for (int i = 0; i < 4; ++i) {
            int channel = i;
            if (rotatingPriority())
                channel = (channel + _priorityChannel) & 3;
            if ((_request & (1 << channel)) != 0) {
                _channel = channel;
                _priorityChannel = (channel + 1) & 3;
                return true;
            }
        }
        return false;
    }
    void dmaCompleted() { _channel = -1; }
    Byte dmaRead()
    {
        if (memoryToMemory() && _channel == 1)
            return _temporary;
        return 0xff;
    }
    void dmaWrite(Byte data)
    {
        if (memoryToMemory() && _channel == 0)
            _temporary = data;
    }
    Word address()
    {
        Word address = _channels[_channel]._currentAddress;
        _channels[_channel].incrementAddress();
        return address;
    }
    int channel() { return _channel; }
private:
    struct Channel
    {
        void setAddress(bool high, Byte data)
        {
            if (high) {
                _baseAddress = (_baseAddress & 0xff00) + data;
                _currentAddress = (_currentAddress & 0xff00) + data;
            }
            else {
                _baseAddress = (_baseAddress & 0xff) + (data << 8);
                _currentAddress = (_currentAddress & 0xff) + (data << 8);
            }
        }
        void setCount(bool high, Byte data)
        {
            if (high) {
                _baseWordCount = (_baseWordCount & 0xff00) + data;
                _currentWordCount = (_currentWordCount & 0xff00) + data;
            }
            else {
                _baseWordCount = (_baseWordCount & 0xff) + (data << 8);
                _currentWordCount = (_currentWordCount & 0xff) + (data << 8);
            }
        }
        Byte getAddress(bool high)
        {
            if (high)
                return _currentAddress >> 8;
            else
                return _currentAddress & 0xff;
        }
        Byte getCount(bool high)
        {
            if (high)
                return _currentWordCount >> 8;
            else
                return _currentWordCount & 0xff;
        }
        void reset()
        {
            _baseAddress = 0;
            _baseWordCount = 0;
            _currentAddress = 0;
            _currentWordCount = 0;
            _mode = 0;
        }
        void incrementAddress()
        {
            if (!addressDecrement())
                ++_currentAddress;
            else
                --_currentAddress;
            --_currentWordCount;
        }
        bool write() { return (_mode & 0x0c) == 4; }
        bool read() { return (_mode & 0x0c) == 8; }
        bool verify() { return (_mode & 0x0c) == 0; }
        bool autoinitialization() { return (_mode & 0x10) != 0; }
        bool addressDecrement() { return (_mode & 0x20) != 0; }
        bool demand() { return (_mode & 0xc0) == 0x00; }
        bool single() { return (_mode & 0xc0) == 0x40; }
        bool block() { return (_mode & 0xc0) == 0x80; }
        bool cascade() { return (_mode & 0xc0) == 0xc0; }

        Word _baseAddress;
        Word _baseWordCount;
        Word _currentAddress;
        Word _currentWordCount;
        Byte _mode;  // Only 6 bits used
    };

    bool memoryToMemory() { return (_command & 1) != 0; }
    bool channel0AddressHold() { return (_command & 2) != 0; }
    bool disabled() { return (_command & 4) != 0; }
    bool compressedTiming() { return (_command & 8) != 0; }
    bool rotatingPriority() { return (_command & 0x10) != 0; }
    bool extendedWriteSelection() { return (_command & 0x20) != 0; }
    bool dreqSenseActiveLow() { return (_command & 0x40) != 0; }
    bool dackSenseActiveHigh() { return (_command & 0x80) != 0; }
    void setRequest(int line, bool active)
    {
        Byte b = 1 << line;
        Byte s = 0x10 << line;
        if (active) {
            _request |= b;
            _status |= s;
        }
        else {
            _request &= ~b;
            _status &= ~s;
        }
    }

    Channel _channels[4];
    Word _temporaryAddress;
    Word _temporaryWordCount;
    Byte _status;
    Byte _command;
    Byte _temporary;
    Byte _mask;  // Only 4 bits used
    Byte _request;  // Only 4 bits used
    bool _high;
    int _channel;
    int _priorityChannel;
    bool _needHighAddress;
};

// Most of the complexity of the PPI is in the strobed and bidirectional bus
// modes, which aren't actually used in the PC/XT. These modes are implemented
// here for future reference, but are untested.
class PPIEmulator
{
public:
    void reset()
    {
        _mode = 0x99;  // XT normal operation: mode 0, A and C input, B output
        //_mode = 0x9b; // Default: all mode 0, all inputs
        _a = 0;
        _b = 0;
        _c = 0;
        _aLines = 0xff;
        _bLines = 0xff;
        _cLines = 0xff;
    }
    void write(int address, Byte data)
    {
        switch (address) {
            case 0:
                _a = data;
                if (aStrobedOutput())
                    _c &= 0x77;  // Clear -OFB and INTR
                break;
            case 1:
                _b = data;
                if (bStrobedOutput())
                    _c &= 0xfc;  // Clear -OFB and INTR
                break;
            case 2: _c = data; break;
            case 3:
                if ((data & 0x80) != 0) {  // Mode set
                    _mode = data;
                    _a = 0;
                    _b = 0;
                    _c = 0;
                    break;
                }
                if ((data & 1) == 0)  // Port C bit reset
                    _c &= ~(1 << ((data & 0xe) >> 1));
                else
                    _c |= 1 << ((data & 0xe) >> 1);
        }
    }
    Byte read(int address)
    {
        switch (address) {
            case 0:
                if (aStrobedInput())
                    _c &= 0xd7;  // Clear IBF and INTR
                if (aMode() == 0 && aInput())
                    return _aLines;
                return _a;
            case 1:
                if (bMode() != 0)
                    _c &= 0xfc;  // Clear IBF and INTR
                if (bMode() == 0 && bInput())
                    return _bLines;
                return _b;
            case 2:
                {
                    Byte c = _c;
                    if (aMode() == 0) {
                        if (cUpperInput())
                            c = (c & 0x0f) + (_cLines & 0xf0);
                    }
                    else {
                        if (aMode() == 0x20 && cUpperInput()) {
                            if (aInput())
                                c = (c & 0x3f) + (_cLines & 0xc0);
                            else
                                c = (c & 0xcf) + (_cLines & 0x30);
                        }
                    }
                    if (bMode() == 0 && cLowerInput())
                        c = (c & 0xf0) + (_cLines & 0x0f);
                    return c;
                }
        }
        return _mode;
    }
    void setA(int line, bool state)
    {
        if (aStrobedInput() && aStrobe())
            _b = _bLines;
        _aLines = (_aLines & ~(1 << line)) | (state ? (1 << line) : 0);
    }
    void setB(int line, bool state)
    {
        if (bStrobedInput() && bStrobe())
            _b = _bLines;
        _bLines = (_bLines & ~(1 << line)) | (state ? (1 << line) : 0);
    }
    void setC(int line, bool state)
    {
        if (aStrobedInput() && line == 4 && (!state || aStrobe())) {
            _a = _aLines;
            _c |= 0x20;  // Set IBF
            if (aInputInterruptEnable() && state)
                _c |= 8;  // Set INTR on rising edge
        }
        if (aStrobedOutput() && line == 6 && (!state || aAcknowledge())) {
            _c |= 0x80;  // Set -OBF
            if (aOutputInterruptEnable() && state)
                _c |= 8;  // Set INTR on rising edge
        }
        if (bStrobedInput() && line == 2 && (!state || bStrobe())) {
            _b = _bLines;
            _c |= 2;  // Set IBF
            if (bInterruptEnable() && state)
                _c |= 1;  // Set INTR on rising edge
        }
        if (bStrobedOutput() && line == 2 && (!state || bStrobe())) {
            _c |= 2;  // Set -OBF
            if (bInterruptEnable() && state)
                _c |= 1;  // Set INTR on rising edge
        }
        _cLines = (_cLines & ~(1 << line)) | (state ? (1 << line) : 0);
    }
    bool getA(int line)
    {
        Byte m = 1 << line;
        if (aMode() == 0) {
            if (aInput())
                return (_aLines & m) != 0;
            return (_a & _aLines & m) != 0;
        }
        return (_a & m) != 0;
    }
    bool getB(int line)
    {
        Byte m = 1 << line;
        if (bMode() == 0) {
            if (bInput())
                return (_bLines & m) != 0;
            return (_b & _bLines & m) != 0;
        }
        return (_b & m) != 0;
    }
    bool getC(int line)
    {
        // 0 bit means output enabled, so _c bit low drives output low
        // 1 bit means tristate from PPI so return _cLine bit.
        static const Byte m[] = {
            0x00, 0x0f, 0x00, 0x0f, 0x04, 0x0c, 0x04, 0x0c,  // A mode 0
            0xf0, 0xff, 0xf0, 0xff, 0xf4, 0xfc, 0xf4, 0xfc,
            0x00, 0x0f, 0x00, 0x0f, 0x04, 0x0c, 0x04, 0x0c,
            0xf0, 0xff, 0xf0, 0xff, 0xf4, 0xfc, 0xf4, 0xfc,

            0x40, 0x47, 0x40, 0x47, 0x44, 0x44, 0x44, 0x44,  // A mode 1 output
            0x70, 0x77, 0x70, 0x77, 0x74, 0x74, 0x74, 0x74,
            0x10, 0x17, 0x10, 0x17, 0x14, 0x14, 0x14, 0x14,  // A mode 1 input
            0xd0, 0xd7, 0xd0, 0xd7, 0xd4, 0xd4, 0xd4, 0xd4,

            0x50, 0x57, 0x50, 0x57, 0x54, 0x54, 0x54, 0x54,  // A mode 2
            0x50, 0x57, 0x50, 0x57, 0x54, 0x54, 0x54, 0x54,
            0x50, 0x57, 0x50, 0x57, 0x54, 0x54, 0x54, 0x54,
            0x50, 0x57, 0x50, 0x57, 0x54, 0x54, 0x54, 0x54,

            0x50, 0x57, 0x50, 0x57, 0x54, 0x54, 0x54, 0x54,  // A mode 2
            0x50, 0x57, 0x50, 0x57, 0x54, 0x54, 0x54, 0x54,
            0x50, 0x57, 0x50, 0x57, 0x54, 0x54, 0x54, 0x54,
            0x50, 0x57, 0x50, 0x57, 0x54, 0x54, 0x54, 0x54,
        };
        return (_cLines & (_c | m[_mode & 0x7f]) & (1 << line)) != 0;
    }
private:
    Byte aMode() { return _mode & 0x60; }
    Byte bMode() { return _mode & 4; }
    bool aInput() { return (_mode & 0x10) != 0; }
    bool cUpperInput() { return (_mode & 8) != 0; }
    bool bInput() { return (_mode & 2) != 0; }
    bool cLowerInput() { return (_mode & 1) != 0; }
    bool aStrobe() { return (_cLines & 0x10) == 0; }
    bool bStrobe() { return (_cLines & 4) == 0; }
    bool aAcknowledge() { return (_cLines & 0x40) == 0; }
    bool bAcknowledge() { return (_cLines & 4) == 0; }
    bool aStrobedInput()
    {
        return (aMode() == 0x20 && aInput()) || aMode() == 0x40;
    }
    bool aStrobedOutput()
    {
        return (aMode() == 0x20 && !aInput()) || aMode() == 0x40;
    }
    bool bStrobedInput() { return bMode() != 0 && bInput(); }
    bool bStrobedOutput() { return bMode() != 0 && !bInput(); }
    bool aInputInterruptEnable() { return (_c & 0x10) != 0; }
    bool aOutputInterruptEnable() { return (_c & 0x40) != 0; }
    bool bInterruptEnable() { return (_c & 4) != 0; }

    Byte _a;
    Byte _b;
    Byte _c;
    Byte _aLines;
    Byte _bLines;
    Byte _cLines;
    Byte _mode;
};

class BusEmulator
{
public:
    BusEmulator() : _ram(0xa0000), _rom(0x8000)
    {
        File("Q:\\external\\8088\\roms\\ibm5160\\1501512.u18", true).
            openRead().read(&_rom[0], 0x8000);
        _pit.setGate(0, true);
        _pit.setGate(1, true);
        _pit.setGate(2, true);
    }
    Byte* ram() { return &_ram[0]; }
    void reset()
    {
        _dmac.reset();
        _pic.reset();
        _pit.reset();
        _ppi.reset();
        _pitPhase = 2;
        _lastCounter0Output = false;
        _lastCounter1Output = true;
        _counter2Output = false;
        _counter2Gate = false;
        _speakerMask = false;
        _speakerOutput = false;
        _dmaState = sIdle;
        _passiveOrHalt = true;
        _lock = false;
        _previousLock = false;
        _previousPassiveOrHalt = true;
        _lastNonDMAReady = true;
        _cgaPhase = 0;
    }
    void stubInit()
    {
        _pic.stubInit();
        _pit.stubInit();
        _pitPhase = 2;
        _lastCounter0Output = true;
    }
    void startAccess(DWord address, int type)
    {
        _address = address;
        _type = type;
        _cycle = 1;
    }
    void wait()
    {
        _cgaPhase = (_cgaPhase + 3) & 0x0f;
        ++_pitPhase;
        if (_pitPhase == 4) {
            _pitPhase = 0;
            _pit.wait();
            bool counter0Output = _pit.getOutput(0);
            if (_lastCounter0Output != counter0Output)
                _pic.setIRQLine(0, counter0Output);
            _lastCounter0Output = counter0Output;
            bool counter1Output = _pit.getOutput(1);
            if (counter1Output && !_lastCounter1Output && !dack0()) {
                _dmaRequests |= 1;
                _dmac.setDMARequestLine(0, true);
            }
            _lastCounter1Output = counter1Output;
            bool counter2Output = _pit.getOutput(2);
            if (_counter2Output != counter2Output) {
                _counter2Output = counter2Output;
                setSpeakerOutput();
                _ppi.setC(5, counter2Output);
                updatePPI();
            }
        }
        if (_speakerCycle != 0) {
            --_speakerCycle;
            if (_speakerCycle == 0) {
                _speakerOutput = _nextSpeakerOutput;
                _ppi.setC(4, _speakerOutput);
                updatePPI();
            }
        }

        // Set to false to implement 5160s without the U90 fix and 5150s
        // without the U101 fix as described in
        // http://www.vcfed.org/forum/showthread.php?29211-Purpose-of-U90-in-XT-second-revision-board
        bool hasDMACFix = true;

        if (_type != 2 || (_address & 0x3e0) != 0x000 || !hasDMACFix)
            _lastNonDMAReady = nonDMAReady();
        //if (_previousLock && !_lock)
        //    _previousLock = false;
        //_previousLock = _lock;
        switch (_dmaState) {
            case sIdle:
                if (_dmac.getHoldRequestLine())
                    _dmaState = sDREQ;
                break;
            case sDREQ:
                _dmaState = sHRQ; //(_passiveOrHalt && !_previousLock) ? sHRQ : sHoldWait;
                break;
            case sHRQ:
                //_dmaState = _lastNonDMAReady ? sAEN : sPreAEN;
                if ((_passiveOrHalt || _previousPassiveOrHalt) && !_previousLock && _lastNonDMAReady)
                    _dmaState = sAEN;
                break;
            //case sHoldWait:
            //    if (_passiveOrHalt && !_previousLock)
            //        _dmaState = _lastNonDMAReady ? sAEN : sPreAEN;
            //    break;
            //case sPreAEN:
            //    if (_lastNonDMAReady)
            //        _dmaState = sAEN;
            //    break;
            case sAEN: _dmaState = s0; break;
            case s0:
                if ((_dmaRequests & 1) != 0) {
                    _dmaRequests &= 0xfe;
                    _dmac.setDMARequestLine(0, false);
                }
                _dmaState = s1; break;
            case s1: _dmaState = s2; break;
            case s2: _dmaState = s3; break;
            case s3: _dmaState = s4; break;
            case s4: _dmaState = sDelayedT1; _dmac.dmaCompleted(); break;
            case sDelayedT1: _dmaState = sDelayedT2; break;
            case sDelayedT2: _dmaState = sDelayedT3; break;
            case sDelayedT3: _dmaState = sIdle; break;
        }
        _previousLock = _lock;
        _previousPassiveOrHalt = _passiveOrHalt;

        _lastNonDMAReady = nonDMAReady();
        ++_cycle;
    }
    bool ready()
    {
        if (_dmaState == s1 || _dmaState == s2 || _dmaState == s3 ||
            _dmaState == sWait || _dmaState == s4 || _dmaState == sDelayedT1 ||
            _dmaState == sDelayedT2 /*|| _dmaState == sDelayedT3*/)
            return false;
        return nonDMAReady();
    }
    void write(Byte data)
    {
        if (_type == 2) {
            switch (_address & 0x3e0) {
                case 0x00:
                    _dmac.write(_address & 0x0f, data);
                    break;
                case 0x20:
                    _pic.write(_address & 1, data);
                    break;
                case 0x40:
                    _pit.write(_address & 3, data);
                    break;
                case 0x60:
                    _ppi.write(_address & 3, data);
                    updatePPI();
                    break;
                case 0x80:
                    _dmaPages[_address & 3] = data;
                    break;
                case 0xa0:
                    _nmiEnabled = (data & 0x80) != 0;
                    break;
            }
        }
        else
            if (_address < 0xa0000)
                _ram[_address] = data;
    }
    Byte read()
    {
        if (_type == 0) // Interrupt acknowledge
            return _pic.interruptAcknowledge();
        if (_type == 1) { // Read port
            switch (_address & 0x3e0) {
                case 0x00: return _dmac.read(_address & 0x0f);
                case 0x20: return _pic.read(_address & 1);
                case 0x40: return _pit.read(_address & 3);
                case 0x60:
                    {
                        Byte b = _ppi.read(_address & 3);
                        updatePPI();
                        return b;
                    }

            }
            return 0xff;
        }
        if (_address >= 0xf8000)
            return _rom[_address - 0xf8000];
        if (_address >= 0xa0000)
            return 0xff;
        return _ram[_address];
    }
    bool interruptPending() { return _pic.interruptPending(); }
    int pitBits()
    {
        return (_pitPhase == 1 || _pitPhase == 2 ? 1 : 0) +
            (_counter2Gate ? 2 : 0) + (_pit.getOutput(2) ? 4 : 0);
    }
    void setPassiveOrHalt(bool v) { _passiveOrHalt = v; }
    bool getAEN()
    {
        return _dmaState == sAEN || _dmaState == s0 || _dmaState == s1 ||
            _dmaState == s2 || _dmaState == s3 || _dmaState == sWait ||
            _dmaState == s4;
    }
    UInt8 getDMA()
    {
        return _dmaRequests | (dack0() ? 0x10 : 0);
    }
    String snifferExtra()
    {
        return ""; //hex(_pit.getMode(1), 4, false) + " ";
    }
    int getBusOperation()
    {
        switch (_dmaState) {
            case s2: return 5;  // memr
            case s3: return 2;  // iow
        }
        return 0;
    }
    bool getDMAS3() { return _dmaState == s3; }
    DWord getDMAAddress()
    {
        return dmaAddressHigh(_dmac.channel()) + _dmac.address();
    }
    void setLock(bool lock) { _lock = lock; }
    UInt8 getIRQLines() { return _pic.getIRQLines(); }
    UInt8 getDMAS()
    {
        if (_dmaState == sAEN || _dmaState == s0 || _dmaState == s1 ||
            _dmaState == s2 || _dmaState == s3 || _dmaState == sWait)
            return 3;
        if (_dmaState == sHRQ || _dmaState == sHoldWait ||
            _dmaState == sPreAEN)
            return 1;
        return 0;
    }
    UInt8 getCGA()
    {
        return _cgaPhase >> 2;
    }
private:
    bool nonDMAReady()
    {
        if (_type == 1 || _type == 2)  // Read port, write port
            return _cycle < 2 || _cycle > 2;  // System board adds a wait state for onboard IO devices
        return true;
    }
    bool dack0()
    {
        return _dmaState == s1 || _dmaState == s2 || _dmaState == s3 ||
            _dmaState == sWait;
    }
    void setSpeakerOutput()
    {
        bool o = !(_counter2Output && _speakerMask);
        if (_nextSpeakerOutput != o) {
            if (_speakerOutput == o)
                _speakerCycle = 0;
            else
                _speakerCycle = o ? 3 : 2;
            _nextSpeakerOutput = o;
        }
    }
    void updatePPI()
    {
        bool speakerMask = _ppi.getB(1);
        if (speakerMask != _speakerMask) {
            _speakerMask = speakerMask;
            setSpeakerOutput();
        }
        _counter2Gate = _ppi.getB(0);
        _pit.setGate(2, _counter2Gate);
    }
    DWord dmaAddressHigh(int channel)
    {
        static const int pageRegister[4] = {0x83, 0x83, 0x81, 0x82};
        return _dmaPages[pageRegister[channel]] << 16;
    }

    enum DMAState
    {
        sIdle,
        sDREQ,
        sHRQ,
        sHoldWait,
        sPreAEN,
        sAEN,
        s0,
        s1,
        s2,
        s3,
        sWait,
        s4,
        sDelayedT1,
        sDelayedT2,
        sDelayedT3
    };

    Array<Byte> _ram;
    Array<Byte> _rom;
    DWord _address;
    int _type;
    int _cycle;
    DMACEmulator _dmac;
    PICEmulator _pic;
    PITEmulator _pit;
    PPIEmulator _ppi;
    int _pitPhase;
    bool _lastCounter0Output;
    bool _lastCounter1Output;
    bool _counter2Output;
    bool _counter2Gate;
    bool _speakerMask;
    bool _speakerOutput;
    bool _nextSpeakerOutput;
    Word _dmaAddress;
    int _dmaCycles;
    int _dmaType;
    int _speakerCycle;
    Byte _dmaPages[4];
    bool _nmiEnabled;
    bool _passiveOrHalt;
    DMAState _dmaState;
    Byte _dmaRequests;
    bool _lock;
    bool _previousLock;
    bool _previousPassiveOrHalt;
    bool _lastNonDMAReady;
    Byte _cgaPhase;
};

class CPUEmulator
{
    enum {
        groupMemory                     = 1,
        groupInitialEARead              = 2,
        groupMicrocodePointerFromOpcode = 4,
        groupNonPrefix                  = 8,
        groupEffectiveAddress           = 0x10,
        groupAddSubBooleanRotate        = 0x20,
        groupNonFlagSet                 = 0x40,
        groupMNotAccumulator            = 0x80,
        groupNonSegregEA                = 0x100,
        groupNoDirectionBit             = 0x200,
        groupMicrocoded                 = 0x400,
        groupNoWidthInOpcodeBit0        = 0x800,
        groupByteOrWordAccess           = 0x1000,
        groupF1ZZFromPrefix             = 0x2000,
        groupIncDec                     = 0x4000,

        groupLoadRegisterImmediate      = 0x10000,
        groupWidthInOpcodeBit3          = 0x20000,
        groupCMC                        = 0x40000,
        groupHLT                        = 0x80000,
        groupREP                        = 0x100000,
        groupSegmentOverride            = 0x200000,
        groupLOCK                       = 0x400000,
        groupCLI                        = 0x800000,
        groupLoadSegmentRegister        = 0x1000000,
    };
public:
    CPUEmulator() : _consoleLogging(false)
    {
        _registers[24] = 0x100;
        auto byteData = reinterpret_cast<Byte*>(&_registers[24]);
        int bigEndian = *byteData;
        int byteNumbers[8] = {0, 2, 4, 6, 1, 3, 5, 7};
        for (int i = 0 ; i < 8; ++i)
            _byteRegisters[i] = &byteData[byteNumbers[i] ^ bigEndian];
        _registers[21] = 0xffff;
        _registers[23] = 0;

        // Initialize microcode data and put it in a format more suitable for
        // software emulation.

        DWord instructions[512];
        for (int i = 0; i < 512; ++i)
            instructions[i] = 0;
        for (int y = 0; y < 4; ++y) {
            int h = (y < 3 ? 24 : 12);
            for (int half = 0; half < 2; ++half) {
                String s = File(
                    String("..\\..\\..\\..\\Projects\\Emulation\\PC\\8086\\") +
                    (half == 1 ? "l" : "r") + decimal(y) + ".txt", true).
                    contents();
                String s2 = File(
                    String("..\\..\\..\\..\\Projects\\Emulation\\PC\\8086\\") +
                    (half == 1 ? "l" : "r") + decimal(y) + "a.txt", true).
                    contents();
                for (int yy = 0; yy < h; ++yy) {
                    int ib = y * 24 + yy;
                    for (int xx = 0; xx < 64; ++xx) {
                        int b = (s[yy * 66 + (63 - xx)] == '0' ? 1 : 0);
                        static const bool use8086 = false;
                        if (use8086)
                            b = (s2[yy * 66 + (63 - xx)] == '0' ? 1 : 0);
                        instructions[xx * 8 + half * 4 + yy % 4] |=
                            b << (20 - (ib >> 2));
                    }
                }
            }
        }
        for (int i = 0; i < 512; ++i) {
            int d = instructions[i];
            int s = ((d >> 13) & 1) + ((d >> 10) & 6) + ((d >> 11) & 0x18);
            int dd = ((d >> 20) & 1) + ((d >> 18) & 2) + ((d >> 16) & 4) +
                ((d >> 14) & 8) + ((d >> 12) & 0x10);
            int typ = (d >> 7) & 7;
            if ((typ & 4) == 0)
                typ >>= 1;
            int f = (d >> 10) & 1;
            _microcode[i * 4] = dd;
            _microcode[i * 4 + 1] = s;
            _microcode[i * 4 + 2] = (f << 3) + typ;
            _microcode[i * 4 + 3] = d & 0xff;
        }

        int stage1[128];
        for (int x = 0; x < 128; ++x)
            stage1[x] = 0;
        for (int g = 0; g < 9; ++g) {
            int n = 16;
            if (g == 0 || g == 8)
                n = 8;
            int xx[9] = { 0, 8, 24, 40, 56, 72, 88, 104, 120 };
            int xp = xx[g];
            for (int h = 0; h < 2; ++h) {
                String s = File(
                    String("..\\..\\..\\..\\Projects\\Emulation\\PC\\8086\\") +
                    decimal(g) + (h == 0 ? "t" : "b") + ".txt", true).
                    contents();
                for (int y = 0; y < 11; ++y) {
                    for (int x = 0; x < n; ++x) {
                        int b = (s[y * (n + 2) + x] == '0' ? 1 : 0);
                        if (b != 0)
                            stage1[127 - (x + xp)] |= 1 << (y * 2 + (h ^ (y <= 2 ? 1 : 0)));
                    }
                }
            }
        }
        for (int i = 0; i < 2048; ++i) {
            static const int ba[] = { 7, 2, 1, 0, 5, 6, 8, 9, 10, 3, 4 };
            for (int j = 0; j < 128; ++j) {
                int s1 = stage1[j];
                if (s1 == 0)
                    continue;
                bool found = true;
                for (int b = 0; b < 11; ++b) {
                    int x = (s1 >> (ba[b] * 2)) & 3;
                    int ib = (i >> (10 - b)) & 1;
                    if (!(x == 0 || (x == 1 && ib == 1) || (x == 2 && ib == 0))) {
                        found = false;
                        break;
                    }
                }
                if (found) {
                    _microcodeIndex[i] = j;
                    break;
                }
            }
        }

        String translationString = File("..\\..\\..\\..\\Projects\\Emulation\\PC\\8086\\translation.txt").contents();
        int tsp = 0;
        char c = translationString[0];
        for (int i = 0; i < 33; ++i) {
            int mask = 0;
            int bits = 0;
            int output = 0;
            for (int j = 0; j < 8; ++j) {
                if (c != '?')
                    mask |= 128 >> j;
                if (c == '1')
                    bits |= 128 >> j;
                ++tsp;
                c = translationString[tsp];
            }
            for (int j = 0; j < 14; ++j) {
                while (c != '0' && c != '1') {
                    ++tsp;
                    c = translationString[tsp];
                }
                if (c == '1')
                    output |= 8192 >> j;
                ++tsp;
                c = translationString[tsp];
            }
            while (c != 10 && c != 13) {
                ++tsp;
                c = translationString[tsp];
            }
            while (c == 10 || c == 13) {
                ++tsp;
                c = translationString[tsp];
            }
            for (int j = 0; j < 256; ++j) {
                if ((j & mask) == bits)
                    _translation[j] = output;
            }
        }

        int groupInput[38 * 18];
        int groupOutput[38 * 15];
        String groupString = File(
            String("..\\..\\..\\..\\Projects\\Emulation\\PC\\8086\\group.txt")).
            contents();
        for (int x = 0; x < 38; ++x) {
            for (int y = 0; y < 15; ++y) {
                groupOutput[y * 38 + x] =
                    (groupString[y * 40 + x] == '0' ? 0 : 1);
            }
            for (int y = 0; y < 18; ++y) {
                int c = groupString[((y / 2) + 15) * 40 + x];
                if ((y & 1) == 0)
                    groupInput[y * 38 + x] = ((c == '*' || c == '0') ? 1 : 0);
                else
                    groupInput[y * 38 + x] = ((c == '*' || c == '1') ? 1 : 0);
            }
        }
        static const int groupYY[18] = { 1, 0,  3, 2,  4, 6,  5, 7,  11, 10,  12, 13,  8, 9,  15, 14,  16, 17 };
        for (int x = 0; x < 34; ++x) {
            if (x == 11)
                continue;
            for (int i = 0; i < 0x101; ++i) {
                bool found = true;
                for (int j = 0; j < 9; ++j) {
                    int y0 = groupInput[groupYY[j*2] * 38 + x];
                    int y1 = groupInput[groupYY[j*2 + 1] * 38 + x];
                    int b = (i >> j) & 1;
                    if ((y0 == 1 && b == 1) || (y1 == 1 && b == 0)) {
                        found = false;
                        break;
                    }
                }
                if (found) {
                    int g = 0;
                    for (int j = 0; j < 15; ++j)
                        g |= groupOutput[j * 38 + x] << j;
                    if (x == 10)
                        g |= groupLoadRegisterImmediate;
                    if (x == 12)
                        g |= groupWidthInOpcodeBit3;
                    if (x == 13)
                        g |= groupCMC;
                    if (x == 14)
                        g |= groupHLT;
                    if (x == 31)
                        g |= groupREP;
                    if (x == 32)
                        g |= groupSegmentOverride;
                    if (x == 33)
                        g |= groupLOCK;
                    if (i == 0xfa)
                        g |= groupCLI;
                    if (i == 0x8e || (i & 0xe7) == 0x07)
                        g |= groupLoadSegmentRegister;
                    _groups[i] = g;
                }
            }
        }

        _microcodePointer = 0;
        _microcodeReturn = 0;
    }
    Byte* getRAM() { return _bus.ram(); }
    Word* getRegisters() { return &_registers[24]; }
    Word* getSegmentRegisters() { return &_registers[0]; }
    void stubInit() { _bus.stubInit(); }
    void setExtents(int logStartCycle, int logEndCycle, int executeEndCycle,
        int stopIP, int stopSeg, int timeIP1, int timeSeg1)
    {
        _logStartCycle = logStartCycle + 4;
        _logEndCycle = logEndCycle;
        _executeEndCycle = executeEndCycle;
        _stopIP = stopIP;
        _stopSeg = stopSeg;
        _timeIP1 = timeIP1;
        _timeSeg1 = timeSeg1;
    }
    void setInitialIP(int v) { ip() = v; }
    int cycle() const { return _cycle - 11; }
    String log() const { return _log; }
    void reset()
    {
        _bus.reset();

        for (int i = 0; i < 0x20; ++i)
            _registers[i] = 0;
        _registers[1] = 0xffff;  // RC (CS)
        _registers[21] = 0xffff; // ONES
        _registers[15] = 2; // FLAGS

        _cycle = 0;
        _microcodePointer = 0x1800;
        _busState = tIdle;
        _ioType = ioPassive;
        _snifferDecoder.reset();
        _prefetching = true;
        _log = "";
        ip() = 0;
        _nmiRequested = false;
        _queueBytes = 0;
        _queue = 0;
        _segmentOverride = -1;
        _f1 = false;
        _repne = false;
        _lock = false;
        _loaderState = 0;
        _lastMicrocodePointer = -1;
        _prefetchCompleting = false;
        _dequeueing = false;
        _ioCancelling = 0;
        _ioRequested = false;
        _ioFirstIdle = false;
        _prefetchDelayed = false;
        _queueFlushing = false;
        _queueWasFilled = false;
        _lastIOType = ioPassive;
        _ioSecondIdle = false;
        _interruptPending = false;
    }
    void run()
    {
        do {
            simulateCycle();
        } while ((getRealIP() != _stopIP + 2 || cs() != _stopSeg) &&
            _cycle < _executeEndCycle);
    }
    void setConsoleLogging() { _consoleLogging = true; }
private:
    Word getRealIP() { return ip() - _queueBytes; }
    enum IOType
    {
        ioInterruptAcknowledge = 0,
        ioReadPort = 1,
        ioWritePort = 2,
        ioHalt = 3,
        ioPrefetch = 4,
        ioReadMemory = 5,
        ioWriteMemory = 6,
        ioPassive = 7
    };
    Byte queueRead()
    {
        Byte byte = _queue & 0xff;
        _dequeueing = true;
        _snifferDecoder.queueOperation(3);
        return byte;
    }
    int modRMReg() { return (_modRM >> 3) & 7; }
    int modRMReg2() { return _modRM & 7; }
    Word& rw(int r) { return _registers[24 + r]; }
    Word& rw() { return rw(_opcode & 7); }
    Word& ax() { return rw(0); }
    Byte& rb(int r) { return *_byteRegisters[r]; }
    Byte& al() { return rb(0); }
    Word& sr(int r) { return _registers[r]; }
    Word& cs() { return sr(1); }
    bool cf() { return lowBit(flags()); }
    void setCF(bool v) { flags() = (flags() & ~1) | (v ? 1 : 0); }
    bool pf() { return (flags() & 4) != 0; }
    bool af() { return (flags() & 0x10) != 0; }
    bool zf() { return (flags() & 0x40) != 0; }
    bool sf() { return (flags() & 0x80) != 0; }
    bool intf() { return (flags() & 0x200) != 0; }
    void setIF(bool v) { flags() = (flags() & ~0x200) | (v ? 0x200 : 0); }
    bool df() { return (flags() & 0x400) != 0; }
    void setDF(bool v) { flags() = (flags() & ~0x400) | (v ? 0x400 : 0); }
    bool of() { return (flags() & 0x800) != 0; }
    void setOF(bool v) { flags() = (flags() & ~0x800) | (v ? 0x800 : 0); }
    Word& ip() { return _registers[4]; }
    Word& ind() { return _registers[5]; }
    Word& opr() { return _registers[6]; }
    Word& tmpa() { return _registers[12]; }
    Word& tmpb() { return _registers[13]; }
    Word& flags() { return _registers[15]; }
    Word& modRMRW() { return rw(modRMReg()); }
    Byte& modRMRB() { return rb(modRMReg()); }
    Word& modRMRW2() { return rw(modRMReg2()); }
    Byte& modRMRB2() { return rb(modRMReg2()); }
    Word getMemOrReg(bool mem)
    {
        if (mem) {
            if (_useMemory)
                return opr();
            if (!_wordSize)
                return modRMRB2();
            return modRMRW2();
        }
        if ((_group & groupNonSegregEA) == 0)
            return sr(modRMReg());
        if (!_wordSize) {
            int n = modRMReg();
            Word r = rw(n & 3);
            if ((n & 4) != 0)
                r = (r >> 8) + (r << 8);
            return r;
        }
        return modRMRW();
    }
    void setMemOrReg(bool mem, Word v)
    {
        if (mem) {
            if (_useMemory)
                opr() = v;
            else {
                if (!_wordSize)
                    modRMRB2() = static_cast<Byte>(v);
                else
                    modRMRW2() = v;
            }
        }
        else {
            if ((_group & groupNonSegregEA) == 0)
                sr(modRMReg()) = v;
            else {
                if (!_wordSize)
                    modRMRB() = static_cast<Byte>(v);
                else
                    modRMRW() = v;
            }
        }
    }
    void startInstruction()
    {
        if ((_group & groupNonPrefix) != 0) {
            _segmentOverride = -1;
            _f1 = false;
            _repne = false;
            if (_lock) {
                _lock = false;
                _bus.setLock(false);
            }
        }
        _opcode = _nextMicrocodePointer >> 4;
        _group = _nextGroup;
    }
    void startMicrocodeInstruction()
    {
        _loaderState = 2;
        startInstruction();
        _microcodePointer = _nextMicrocodePointer;
        _wordSize = true;
        if ((_group & groupNoWidthInOpcodeBit0) == 0 && !lowBit(_opcode))
            _wordSize = false;
        if ((_group & groupByteOrWordAccess) == 0)
            _wordSize = false;  // Just for XLAT
        _carry = cf();  // Just for SALC
        _overflow = of(); // Not sure if the other flags work the same
        _parity = pf() ? 0x40 : 0;
        _sign = sf();
        _zero = zf();
        _auxiliary = af();
        _alu = 0;
        _mIsM = ((_group & groupNoDirectionBit) != 0 || (_opcode & 2) == 0);
        _rni = false;
        _nx = false;
        _skipRNI = false;
        _state = stateRunning;

        if ((_group & groupEffectiveAddress) != 0) {
            // EALOAD and EADONE finish with RTN
            _modRM = _nextModRM;
            if ((_group & groupMicrocodePointerFromOpcode) == 0) {
                _microcodePointer = ((_modRM << 1) & 0x70) | 0xf00 |
                    ((_opcode & 1) << 12) | ((_opcode & 8) << 4);
            }
            _useMemory = ((_modRM & 0xc0) != 0xc0);
            if (_useMemory) {
                int t = _translation[2 + ((_modRM & 7) << 3) + (_modRM & 0xc0)];
                _segment = (lowBit(t) ? 2 : 3);
                _microcodeReturn = _microcodePointer;
                _microcodePointer = t >> 1;
                _state = stateSingleCycleWait;
            }
        }
    }
    void startNonMicrocodeInstruction()
    {
        _loaderState = 0;
        startInstruction();
        if ((_group & groupLOCK) != 0) {
            _lock = true;
            _bus.setLock(true);
            return;
        }
        if ((_group & groupREP) != 0) {
            _f1 = true;
            _repne = !lowBit(_opcode);
            return;
        }
        if ((_group & groupHLT) != 0)
            _prefetching = false;
        if ((_group & groupCMC) != 0) {
            flags() ^= 1;
            return;
        }
        if ((_group & groupNonFlagSet) == 0) {
            switch (_opcode & 0x06) {
                case 0: // CLCSTC
                    setCF(_opcode & 1);
                    break;
                case 2: // CLISTI
                    setIF(_opcode & 1);
                    break;
                case 4: // CLDSTD
                    setDF(_opcode & 1);
                    break;
            }
            return;
        }
        if ((_group & groupSegmentOverride) != 0) {
            _segmentOverride = (_opcode >> 3) & 3;
            return;
        }
    }
    Word doRotate(Word v, Word a, bool carry)
    {
        _carry = carry;
        _overflow = topBit(v ^ a);
        return v;
    }
    Word doShift(Word v, Word a, bool carry, bool auxiliary)
    {
        _auxiliary = auxiliary;
        doPZS(v);
        return doRotate(v, a, carry);
    }
    Word doALU()
    {
        Word t;
        bool oldAF;
        DWord a = _registers[_aluInput + 12], v;
        switch (_alu) {
            case 0x00: // ADD
            case 0x02: // ADC
                return add(a, tmpb(), cf() && _alu != 0);
            case 0x01: // OR
                return bitwise(a | tmpb());
            case 0x03: // SBB
            case 0x05: // SUBT
            case 0x07: // CMP
                return sub(a, tmpb(), cf() && _alu == 3);
            case 0x04: // AND
                return bitwise(a & tmpb());
            case 0x06: // XOR
                return bitwise(a ^ tmpb());
            case 0x08: // ROL
                return doRotate((a << 1) | (topBit(a) ? 1 : 0), a, topBit(a));
            case 0x09: // ROR  
                return doRotate((a >> 1) | topBit(lowBit(a)), a, lowBit(a));
            case 0x0a: // LRCY
                return doRotate((a << 1) | (_carry ? 1 : 0), a, topBit(a));
            case 0x0b: // RRCY
                return doRotate((a >> 1) | topBit(_carry), a, lowBit(a));
            case 0x0c: // SHL  
                return doShift(a << 1, a, topBit(a), (a & 8) != 0);
            case 0x0d: // SHR
                return doShift(a >> 1, a, lowBit(a), (a & 0x20) != 0);
            case 0x0e: // SETMO
                return doShift(0xffff, a, false, false);
            case 0x0f: // SAR
                return doShift((a >> 1) | topBit(topBit(a)), a, lowBit(a), (a & 0x20) != 0);
            case 0x10: // PASS
                return doShift(a, a, false, false);
            case 0x14: // DAA
                oldAF = _auxiliary;
                t = a;
                if (oldAF || (a & 0x0f) > 9) {
                    t = a + 6;
                    _overflow = topBit(t & (t ^ a));
                    _auxiliary = true;
                }
                if (_carry || a > (oldAF ? 0x9fU : 0x99U)) {
                    v = t + 0x60;
                    _overflow = topBit(v & (v ^ t));
                    _carry = true;
                }
                else
                    v = t;
                doPZS(v);
                break;
            case 0x15: // DAS
                oldAF = _auxiliary;
                t = a;
                if (oldAF || (a & 0x0f) > 9) {
                    t = a - 6;
                    _overflow = topBit(a & (t ^ a));
                    _auxiliary = true;
                }
                if (_carry || a > (oldAF ? 0x9fU : 0x99U)) {
                    v = t - 0x60;
                    _overflow = topBit(t & (v ^ t));
                    _carry = true;
                }
                else
                    v = t;
                doPZS(v);
                break;
            case 0x16: // AAA
                _carry = (_auxiliary || (a & 0xf) > 9);
                _auxiliary = _carry;
                v = a + (_carry ? 6 : 0);
                _overflow = topBit(v & (v ^ a));
                doPZS(v);
                return v & 0x0f;
            case 0x17: // AAS
                _carry = (_auxiliary || (a & 0xf) > 9);
                _auxiliary = _carry;
                v = a - (_carry ? 6 : 0);
                _overflow = topBit(a & (v ^ a));
                doPZS(v);
                return v & 0x0f;
            case 0x18: // INC
                v = a + 1;
                doPZS(v);
                _overflow = topBit((v ^ a) & (v ^ 1));
                _auxiliary = (((v ^ a ^ 1) & 0x10) != 0);
                break;
            case 0x19: // DEC
                v = a - 1;
                doPZS(v);
                _overflow = topBit((a ^ 1) & (v ^ a));
                _auxiliary = (((v ^ a ^ 1) & 0x10) != 0);
                break;
            case 0x1a: // COM1
                return ~a;
            case 0x1b: // NEG
                return sub(0, a, false);
            case 0x1c: // INC2
                return a + 2; // flags never updated
            case 0x1d: // DEC2
                return a - 2; // flags never updated
            default:
                return 0;
        }
        return v;
    }
    enum MicrocodeState
    {
        stateRunning,
        stateWaitingForQueueData,
        stateWaitingForQueueIdle,
        stateWaitingUntilFirstByteReadCanStart,
        stateWaitingUntilFirstByteRead,
        stateWaitingUntilSecondByteRead,
        stateWaitingUntilFirstByteWriteCanStart,
        stateWaitingUntilFirstByteWritten,
        stateWaitingUntilSecondByteWritten,
        stateWaitingUntilFirstByteIRQAcknowledgeCanStart,
        stateWaitingUntilFirstByteIRQAcknowledgeRead,
        stateWaitingUntilSecondByteIRQAcknowledgeRead,
        stateSingleCycleWait,
    };
    DWord readSource()
    {
        DWord v;
        switch (_source) {
            case 7:  // Q
                if (_queueBytes == 0) {
                    _state = stateWaitingForQueueData;
                    return 0;
                }
                return queueRead();
            case 8:  // A (AL)
            case 9:  // C (CL)? - not used
            case 10: // E (DL)? - not used
            case 11: // L (BL)? - not used
                return rb(_source & 3);
            case 16: // X (AH)
            case 17: // B (CH)? - not used
                return rb((_source & 3) + 4);
            case 18: // M
                if ((_group & groupMNotAccumulator) == 0)
                    return (_wordSize ? ax() : al());
                if ((_group & groupEffectiveAddress) == 0)
                    return rw();
                return getMemOrReg(_mIsM);
            case 19: // R
                if ((_group & groupEffectiveAddress) == 0)
                    return sr((_opcode >> 3) & 7);
                return getMemOrReg(!_mIsM);
            case 20: // SIGMA
                v = doALU();
                if (_updateFlags) {
                    flags() = (flags() & 0xf702) | (_overflow ? 0x800 : 0)
                        | (_sign ? 0x80 : 0) | (_zero ? 0x40 : 0)
                        | (_auxiliary ? 0x10 : 0) | _parity
                        | (_carry ? 1 : 0);
                }
                return v;
            case 22: // CR
                return _microcodePointer & 0xf;
        }
        return _registers[_source];
    }
    void writeDestination(DWord v)
    {
        switch (_destination) {
            case 8:  // A (AL)
            case 9:  // C (CL)? - not used
            case 10: // E (DL)? - not used
            case 11: // L (BL)? - not used
                rb(_destination & 3) = v;
                break;
            case 15: // F
                flags() = (v & 0xfd5) | 2;
                break;
            case 16: // X (AH)
            case 17: // B (CH)? - not used
                rb((_destination & 3) + 4) = v;
                break;
            case 18: // M
                if (_alu == 7)
                    break;
                if ((_group & groupMNotAccumulator) == 0) {
                    if (!_wordSize)
                        al() = static_cast<Byte>(v);
                    else
                        ax() = v;
                    break;
                }
                if ((_group & groupEffectiveAddress) == 0) {
                    if ((_group & groupWidthInOpcodeBit3) != 0 &&
                        (_opcode & 8) != 0)
                        rb(_opcode & 7) = v;
                    else
                        rw() = v;
                }
                else {
                    setMemOrReg(_mIsM, v);
                    _skipRNI = (_mIsM && _useMemory);
                }
                break;
            case 19: // R
                if ((_group & groupEffectiveAddress) == 0)
                    sr((_opcode >> 3) & 7) = v;
                else
                    setMemOrReg(!_mIsM, v);
                break;
            case 20: // tmpaL
                tmpa() = (tmpa() & 0xff00) | (v & 0xff);
                break;
            case 21: // tmpbL - sign extend to tmpb
                tmpb() = ((v & 0x80) != 0 ? 0xff00 : 0) | (v & 0xff);
                break;
            case 22: // tmpaH
                tmpa() = (tmpa() & 0xff) | (v << 8);
                break;
            case 23: // tmpbH
                tmpb() = (tmpb() & 0xff) | (v << 8);
                break;
            default:
                _registers[_destination] = v;
        }
    }
    void busAccessDone()
    {
        if ((_operands & 0x10) != 0)
            _rni = true;
        switch (_operands & 3) {
            case 0: // Increment IND by 2
                ind() += 2;
                break;
            case 1: // Adjust IND according to word size and DF
                ind() += _wordSize ? (df() ? -2 : 2) : (df() ? -1 : 1);
                break;
            case 2: // Decrement IND by 2
                ind() -= 2;
                break;
        }
        _state = stateRunning;
    }
    int busSegment()
    {
        int r = (_operands >> 2) & 3;
        switch (r) {
            case 0: // ES
            case 2: // SS
                return r;
            case 1: // segment 0
                return 23;
        }
        return _segmentOverride != -1 ? _segmentOverride : _segment;
    }
    IOType busType()
    {
        int t = (_operands >> 5) & 3;
        if (t == 1)
            return ioInterruptAcknowledge;
        if (t == 0)
            return (_group & groupMemory) != 0 ? ioReadMemory : ioReadPort;
        return (_group & groupMemory) != 0 ? ioWriteMemory : ioWritePort;
    }
    void busStart(MicrocodeState state, Word offset)
    {
        _ioSegment = busSegment();
        _ioAddress = physicalAddress(_ioSegment, offset);
        _ioType = busType();
        _state = state;
        _ioDone = false;
    }
    void busStartWrite()
    {
        _ioWriteData = opr() & 0xff;
        busStart(stateWaitingUntilFirstByteWritten, ind());
        _ioWriteDone = false;
    }
    void doSecondHalf()
    {
        switch (_type) {
            case 0: // short jump
                if (!condition(_operands >> 4))
                    break;
                _microcodePointer =
                    (_microcodePointer & 0x1ff0) + (_operands & 0xf);
                _state = stateSingleCycleWait;
                break;
            case 1: // precondition ALU
                _alu = _operands >> 3;
                _nx = lowBit(_operands);
                // This is surprisingly complicated, and may not be the same
                // logic the actual CPU uses. But it should give the same
                // results.
                if (_mIsM && _useMemory && _alu != 7 &&
                    (_group & groupEffectiveAddress) != 0)
                    _nx = false;
                _aluInput = (_operands >> 1) & 3;
                if (_alu == 0x11) { // XI
                    _alu = ((((_opcode & 0x80) != 0 ? _modRM : _opcode) >> 3) & 7) |
                        ((_opcode >> 3) & 8) |
                        ((_group & groupAddSubBooleanRotate) != 0 ? 0 : 0x10);
                }
                break;
            case 4:
                switch ((_operands >> 3) & 0x0f) {
                    case 0: // MAXC
                        _counter = _wordSize ? 15 : 7;
                        _state = stateSingleCycleWait; // HACK: this makes the queuefiller "MUL AL" timing come out right, not sure if it's correct.
                        break;
                    case 1: // FLUSH
                        _queueBytes = 0;
                        _snifferDecoder.queueOperation(2);
                        _queueFlushing = true;
                        break;
                    case 2: // CF1
                        _f1 = !_f1;
                        break;
                    case 3: // CITF
                        setIF(false);
                        flags() &= ~0x100;
                        break;
                    case 4: // RCY
                        setCF(false);
                        break;
                    case 6: // CCOF
                        setCF(false);
                        setOF(false);
                        break;
                    case 7: // SCOF
                        setCF(true);
                        setOF(true);
                        break;
                    case 8: // WAIT
                        // Don't know what this does!
                        break;
                }
                switch (_operands & 7) {
                    case 0: // RNI
                        if (!_skipRNI)
                            _rni = true;
                        break;
                    case 1: // WB,NX
                        if (!_mIsM || !_useMemory || _alu == 7)
                            _nx = true;
                        break;
                    case 2: // CORR
                        //if (_ioType != ioPassive || _busState == t4 || _ioFirstIdle || _busState == tIdle) {
                            _state = stateWaitingForQueueIdle;
                        //    return;
                        //}
                        //ip() -= _queueBytes;
                        //_queueBytes = 0; // so that realIP() is correct
                        break;
                    case 3: // SUSP
                        _prefetching = false;
                        if (_busState == t4 || _busState == tIdle)
                            _ioType = ioPassive;
                        break;
                    case 4: // RTN
                        _microcodePointer = _microcodeReturn;
                        _state = stateSingleCycleWait;
                        break;
                    case 5: // NX
                        _nx = true;
                        break;
                }
                break;
            case 6:
                switch ((_operands >> 5) & 3) {
                    case 0: // R
                        _state = stateWaitingUntilFirstByteReadCanStart;
                        break;
                    case 1: // IRQ
                        _state = stateWaitingUntilFirstByteIRQAcknowledgeCanStart;
                        break;
                    case 2: // W
                        _state = stateWaitingUntilFirstByteWriteCanStart;
                        break;
                }
                _ioRequested = true;
                _ioCancelling = 2;
                if (_ioType != ioPassive || _ioSecondIdle) {
                    if ((_busState == t2t3tWaitNotLast || _busState == t3tWaitLast || _busState == t4 || _busState == tIdle)) {
                        _ioCancelling = ((_busState == t3tWaitLast || (_busState == tIdle && _ioSecondIdle && _lastIOType == ioPrefetch && _queueBytes == 3)) ? 3 : 2);
                        if (_busState == t4 || _busState == tIdle)
                            _ioType = ioPassive;
                    }
                }
                break;
            case 5: // long jump or call
            case 7:
                if (!condition(_operands >> 4))
                    break;
                if (_type == 7)
                    _microcodeReturn = _microcodePointer;
                _microcodePointer = _translation[
                    ((_type & 2) << 6) +
                        ((_operands << 3) & 0x78) +
                        ((_group & groupInitialEARead) == 0 ? 4 : 0) +
                        ((_modRM & 0xc0) == 0 ? 1 : 0)] >> 1;
                _state = stateSingleCycleWait;
                break;
        }
    }
    void executeMicrocode()
    {
        Byte* m;
        DWord v;
        switch (_state) {
            case stateRunning:
                _lastMicrocodePointer = _microcodePointer;
                m = &_microcode[
                    ((_microcodeIndex[_microcodePointer >> 2] << 2) +
                        (_microcodePointer & 3)) << 2];
                _microcodePointer = (_microcodePointer & 0xfff0) |
                    ((_microcodePointer + 1) & 0xf);
                _destination = m[0];
                _source = m[1];
                _type = m[2] & 7;
                _updateFlags = ((m[2] & 8) != 0);
                _operands = m[3];
                v = readSource();
                if (_state == stateWaitingForQueueData)
                    break;
                writeDestination(v);
                doSecondHalf();
                break;
            case stateWaitingForQueueData:
                if (_queueBytes == 0)
                    break;
                _state = stateRunning;
                writeDestination(readSource());
                doSecondHalf();
                break;
            case stateWaitingForQueueIdle:
                if (_ioType != ioPassive || _busState == t4 || _ioFirstIdle)
                    break;
                ip() -= _queueBytes;
                _queueBytes = 0; // so that realIP() is correct
                _state = stateRunning;
                break;
            case stateWaitingUntilFirstByteReadCanStart:
                if (_ioCancelling != 0) {
                    --_ioCancelling;
                    if (_ioCancelling != 0)
                        break;
                }
                if (_ioType != ioPassive)
                    break;
                busStart(stateWaitingUntilFirstByteRead, ind());
                break;
            case stateWaitingUntilFirstByteRead:
                if (!_wordSize) {
                    _ioRequested = false;
                    if (!_ioDone)
                        break;
                }
                else {
                    if (_ioType != ioPassive)
                        break;
                }
                _ioDone = false;
                opr() = _ioReadData;
                if (!_wordSize) {
                    opr() |= 0xff00;
                    busAccessDone();
                    break;
                }
                busStart(stateWaitingUntilSecondByteRead, ind() + 1);
                break;
            case stateWaitingUntilSecondByteRead:
                _ioRequested = false;
                if (!_ioDone)
                    break;
                opr() |= _ioReadData << 8;
                busAccessDone();
                break;
            case stateWaitingUntilFirstByteWriteCanStart:
                if (_ioCancelling != 0) {
                    --_ioCancelling;
                    if (_ioCancelling != 0)
                        break;
                }
                if (_ioType != ioPassive)
                    break;
                busStartWrite();
                break;
            case stateWaitingUntilFirstByteWritten:
                if (!_wordSize) {
                    _ioRequested = false;
                    if (!_ioWriteDone)
                        break;
                }
                else {
                    if (_busState != t4)
                        break;
                }
                _ioWriteDone = false;
                if (!_wordSize) {
                    busAccessDone();
                    break;
                }
                _ioWriteData = opr() >> 8;
                busStart(stateWaitingUntilSecondByteWritten, ind() + 1);
                _ioWriteDone = false;
                break;
            case stateWaitingUntilSecondByteWritten:
                _ioRequested = false;
                if (!_ioWriteDone)
                    break;
                busAccessDone();
                break;
            case stateWaitingUntilFirstByteIRQAcknowledgeCanStart:
                if (_ioCancelling != 0) {
                    --_ioCancelling;
                    if (_ioCancelling != 0)
                        break;
                }
                if (_ioType != ioPassive)
                    break;
                busStart(stateWaitingUntilFirstByteIRQAcknowledgeRead, ind());  // No address placed on bus during IRQ Acknowledge - it's left to float
                break;
            case stateWaitingUntilFirstByteIRQAcknowledgeRead:
                if (!_wordSize) {
                    _ioRequested = false;
                    if (!_ioDone)
                        break;
                }
                else {
                    if (_ioType != ioPassive)
                        break;
                }
                _ioDone = false;
                opr() = _ioReadData;
                if (!_wordSize) {
                    opr() |= 0xff00;
                    busAccessDone();
                    break;
                }
                busStart(stateWaitingUntilSecondByteRead, ind() + 1);
                break;
            case stateWaitingUntilSecondByteIRQAcknowledgeRead:
                _ioRequested = false;
                if (!_ioDone)
                    break;
                opr() |= _ioReadData << 8;
                busAccessDone();
                break;
            case stateSingleCycleWait:
                _state = stateRunning;
                break;
        }
    }
    void setNextMicrocode(int nextState, int nextMicrocode)
    {
        _nextMicrocodePointer = nextMicrocode;
        _loaderState = nextState | 1;
        _nextGroup = _groups[nextMicrocode >> 4];
    }
    void readOpcode(int nextState)
    {
        if ((flags() & 0x100) != 0) {
            setNextMicrocode(nextState, 0x1000);
            return;
        }
        if (_nmiRequested) {
            _nmiRequested = false;
            setNextMicrocode(nextState, 0x1001);
            return;
        }
        if (interruptPending()) {
            setNextMicrocode(nextState, 0x1002);
            return;
        }
        if (_queueBytes != 0) {
            setNextMicrocode(nextState, queueRead() << 4);
            _snifferDecoder.queueOperation(1);
            return;
        }
        _loaderState = nextState & 2;
    }
    void simulateCycle()
    {
        if (_dequeueing /* && (_busState != t4 || (_ioType != ioPrefetch && _ioType != ioPassive))*/) {
            _dequeueing = false;
            _queue >>= 8;
            --_queueBytes;
        }
        if ((_loaderState & 2) != 0)
            executeMicrocode();
        switch (_loaderState) {
            case 0:
                readOpcode(0);
                break;
            case 1:
            case 3:
                if ((_nextGroup & groupMicrocoded) == 0)  // 1BL
                    startNonMicrocodeInstruction();
                else {
                    if ((_nextGroup & groupEffectiveAddress) == 0)  // SC
                        startMicrocodeInstruction();
                    else {
                        _loaderState = 1;
                        if (_queueBytes != 0) {  // SC
                            _nextModRM = queueRead();
                            startMicrocodeInstruction();
                        }
                    }
                }
                break;
            case 2:
                if (_rni)
                    readOpcode(0);
                else {
                    if (_nx)
                        readOpcode(2);
                }
                break;
        }

        BusState nextState = _busState;
        bool write = _ioType == ioWriteMemory || _ioType == ioWritePort;
        bool ready = true;
        _ioSecondIdle = _ioFirstIdle;
        _ioFirstIdle = false;
        int newQueueBytes = _queueBytes;
        switch (_busState) {
            case t1:
                _snifferDecoder.setAddress(_ioAddress);
                _bus.startAccess(_ioAddress, (int)_ioType);
                if (write)
                    _snifferDecoder.setData(_ioWriteData);
                nextState = t2t3tWaitNotLast;
                break;
            case t2t3tWaitNotLast:
                _snifferDecoder.setStatusHigh(_ioSegment);
                _snifferDecoder.setBusOperation((int)_ioType);
                ready = _bus.ready();
                if (!ready)
                    break;
                nextState = t3tWaitLast;
                if (_ioType == ioInterruptAcknowledge ||
                    _ioType == ioReadPort || _ioType == ioReadMemory ||
                    _ioType == ioPrefetch) {
                    _ioReadData = _bus.read();
                }
                else
                    _ioWriteDone = true;
                if (_ioType != ioPrefetch)
                    _ioDone = true;
                break;
            case t3tWaitLast:
                if (_ioType == ioInterruptAcknowledge ||
                    _ioType == ioReadPort || _ioType == ioReadMemory ||
                    _ioType == ioPrefetch)
                    _snifferDecoder.setData(_ioReadData);
                nextState = t4;
                if (_ioType == ioPrefetch) {
                    _prefetchCompleting = true;
                    ++newQueueBytes;
                }
                if (write)
                    _bus.write(_ioWriteData);
                _bus.setPassiveOrHalt(true);
                _snifferDecoder.setStatus((int)ioPassive);
                _lastIOType = _ioType;
                _ioType = ioPassive;
                break;
            case t4:
                if (_prefetchCompleting) {
                    _prefetchCompleting = false;
                    _queue |= (_ioReadData << (_queueBytes << 3));
                    ++_queueBytes;
                    ++ip();
                    if (_queueBytes == 4)
                        _queueWasFilled = true;
                }
                nextState = tIdle;
                _ioFirstIdle = true;
                break;
        }
        if (nextState == tIdle && _ioType != ioPassive) {
            nextState = t1;
            if (_ioType == ioPrefetch)
                _ioAddress = physicalAddress(_ioSegment, ip());

            _bus.setPassiveOrHalt(_ioType == ioHalt);
            _snifferDecoder.setStatus((int)_ioType);
        }

        if (_ioType == ioPassive && _prefetching && !_ioRequested && _busState != t4 && !(newQueueBytes == 3 && _ioSecondIdle && _lastIOType == ioPrefetch) &&
            (_queueBytes < 3 || (_queueBytes == 3 && !_prefetchCompleting) || (_queueWasFilled && _dequeueing))) {
            if (_queueWasFilled && !_prefetchDelayed)
                _prefetchDelayed = true;
            else {
                _prefetchDelayed = false;
                _ioType = ioPrefetch;
                _ioSegment = 1;
            }
        }
        if (!_ioFirstIdle && _queueWasFilled)
            _queueWasFilled = false;
        if (_queueFlushing) {
            _queueFlushing = false;
            _prefetching = true;
        }
        if (_cycle < _logEndCycle) {
            _snifferDecoder.setAEN(_bus.getAEN());
            _snifferDecoder.setDMA(_bus.getDMA());
            _snifferDecoder.setPITBits(_bus.pitBits());
            _snifferDecoder.setBusOperation(_bus.getBusOperation());
            _snifferDecoder.setInterruptFlag(intf());
            if (_bus.getDMAS3())
                _snifferDecoder.setAddress(_bus.getDMAAddress());
            _snifferDecoder.setReady(ready);
            _snifferDecoder.setLock(_lock);
            _snifferDecoder.setDMAS(_bus.getDMAS());
            _snifferDecoder.setIRQs(_bus.getIRQLines());
            _snifferDecoder.setINT(_bus.interruptPending());
            _snifferDecoder.setCGA(_bus.getCGA());
            String l = _bus.snifferExtra() + _snifferDecoder.getLine();
            l = pad(l, 103) + microcodeString() + "\n";
            if (_cycle >= _logStartCycle) {
                if (_consoleLogging)
                    console.write(l);
                else
                    _log += l;
            }
        }
        _busState = nextState;
        ++_cycle;
        _interruptPending = _bus.interruptPending();
        _bus.wait();
    }
    String pad(String s, int n) { return s + (n - s.length()) * String(" "); }
    String microcodeString()
    {
        if (_lastMicrocodePointer == -1)
            return "";
        static const char* regNames[] = {
            "RA",  // ES
            "RC",  // CS
            "RS",  // SS - presumably, to fit pattern. Only used in RESET
            "RD",  // DS
            "PC",
            "IND",
            "OPR",
            "no dest",  // as dest only - source is Q
            "A",   // AL
            "C",   // CL? - not used
            "E",   // DL? - not used
            "L",   // BL? - not used
            "tmpa",
            "tmpb",
            "tmpc",
            "F",   // flags register
            "X",   // AH
            "B",   // CH? - not used
            "M",
            "R",   // source specified by modrm and direction, destination specified by r field of modrm
            "tmpaL",    // as dest only - source is SIGNA
            "tmpbL",    // as dest only - source is ONES
            "tmpaH",    // as dest only - source is CR
            "tmpbH",    // as dest only - source is ZERO
            "XA",  // AX
            "BC",  // CX
            "DE",  // DX
            "HL",  // BX
            "SP",  // SP
            "MP",  // BP
            "IJ",  // SI
            "IK",  // DI
        };
        static const char* condNames[] = {
            "F1ZZ",
            "MOD1", // jump if short offset in effective address
            "L8  ", // jump if short immediate (skip 2nd byte from Q)
            "Z   ", // jump if zero (used in IMULCOF/MULCOF)
            "NCZ ",
            "TEST", // jump if overflow flag is set
            "OF  ", // jump if -TEST pin not asserted
            "CY  ",
            "UNC ",
            "NF1 ",
            "NZ  ", // jump if not zero (used in JCXZ and LOOP)
            "X0  ", // jump if bit 3 of opcode is 1
            "NCY ",
            "F1  ",
            "INT ", // jump if interrupt is pending
            "XC  ",  // jump if condition based on low 4 bits of opcode
        };
        static const char* destNames[] = {
            "FARCALL ",
            "NEARCALL",
            "RELJMP  ",
            "EAOFFSET",
            "EAFINISH",
            "FARCALL2",
            "INTR    ",
            "INT0    ",
            "RPTI    ",
            "AAEND   ",
            "FARRET  ",
            "RPTS    ",
            "CORX    ", // unsigned multiply tmpc and tmpb, result in tmpa:tmpc
            "CORD    ", // unsigned divide tmpa:tmpc by tmpb, quotient in ~tmpc, remainder in tmpa
            "PREIMUL ", // abs tmpc and tmpb, invert F1 if product negative
            "NEGATE  ", // negate product tmpa:tmpc 
            "IMULCOF ", // clear carry and overflow flags if product of signed multiply fits in low part, otherwise set them
            "MULCOF  ", // clear carry and overflow flags if product of unsigned multiply fits in low part, otherwise set them
            "PREIDIV ", // abs tmpa:tmpc and tmpb, invert F1 if one or the other but not both were negative
            "POSTIDIV", // negate ~tmpc if F1 set
        };
        Byte* m = &_microcode[
            ((_microcodeIndex[_lastMicrocodePointer >> 2] << 2) +
                (_lastMicrocodePointer & 3)) << 2];
        int d = m[0];
        int s = m[1];
        int t = m[2] & 7;
        bool f = ((m[2] & 8) != 0);
        int o = m[3];
        String r;
        if (d == 0 && s == 0 && t == 0 && !f && o == 0) {
            r += "null instruction executed!";
            s = 0x15;
            d = 0x07;
            t = 4;
            o = 0xfe;
        }
        if (s == 0x15 && d == 0x07)  // "ONES  -> Q" used as no-op move
            r = "                ";
        else {
            const char* source = regNames[s];
            switch (s) {
                case 0x07: source = "Q"; break;
                case 0x14: source = "SIGMA"; break;
                case 0x15: source = "ONES"; break;
                case 0x16: source = "CR"; break;  // low 3 bits of microcode address Counting Register + 1? Used as interrupt number at 0x198 (1), 0x199 (2), 0x1a7 (0), 0x1af (4), and 0x1b2 (3)
                case 0x17: source = "ZERO"; break;
            }
            r = pad(String(source), 5) + " -> " + pad(String(regNames[d]), 7);
        }
        r += "   ";
        if ((o & 0x7f) == 0x7f) {
            r += "                  ";
            t = -1;
        }
        else
            r += decimal(t) + "   ";
        switch (t) {  // TYP bits
            case 0:
            case 5:
            case 7:
                r += condNames[(o >> 4) & 0x0f] + String("  ");
                if (t == 5) 
                    r += destNames[o & 0xf];
                else {
                    if (t == 7)
                        r += destNames[10 + (o & 0xf)];
                    else {
                        String s = decimal(o & 0xf);
                        r += pad(s, 4) + "    ";
                    }
                }
                break;
            case 4:
                switch ((o >> 3) & 0x0f) {
                    case 0x00: r += "MAXC "; break;
                    case 0x01: r += "FLUSH"; break;
                    case 0x02: r += "CF1  "; break;
                    case 0x03: r += "CITF "; break;  // clear interrupt and trap flags
                    case 0x04: r += "RCY  "; break;  // reset carry
                    case 0x06: r += "CCOF "; break;  // clear carry and overflow
                    case 0x07: r += "SCOF "; break;  // set carry and overflow
                    case 0x08: r += "WAIT "; break;  // not sure what this does
                    case 0x0f: r += "none "; break;
                }
                r += " ";
                switch (o & 7) {
                    case 0: r += "RNI     "; break;
                    case 1: r += "WB,NX   "; break;
                    case 2: r += "CORR    "; break;
                    case 3: r += "SUSP    "; break;
                    case 4: r += "RTN     "; break;
                    case 5: r += "NX      "; break;
                    case 7: r += "none    "; break;
                }
                break;
            case 1:
                switch ((o >> 3) & 0x1f) {
                    case 0x00: r += "ADD "; break;
                    case 0x02: r += "ADC "; break;
                    case 0x04: r += "AND "; break;
                    case 0x05: r += "SUBT"; break;
                    case 0x0a: r += "LRCY"; break;
                    case 0x0b: r += "RRCY"; break;
                    case 0x10: r += "PASS"; break;
                    case 0x11: r += "XI  "; break;
                    case 0x18: r += "INC "; break;
                    case 0x19: r += "DEC "; break;
                    case 0x1a: r += "COM1"; break;
                    case 0x1b: r += "NEG "; break;
                    case 0x1c: r += "INC2"; break;
                    case 0x1d: r += "DEC2"; break;
                }
                r += "  ";
                switch (o & 7) {
                    case 0: r += "tmpa    "; break;
                    case 1: r += "tmpa, NX"; break;
                    case 2: r += "tmpb    "; break;
                    case 3: r += "tmpb, NX"; break;
                    case 4: r += "tmpc    "; break;
                }
                break;
            case 6:
                switch ((o >> 4) & 7) {
                    case 0: r += "R    "; break;
                    case 2: r += "IRQ  "; break;
                    case 4: r += "w    "; break;
                    case 5: r += "W,RNI"; break;
                }
                r += " ";
                switch ((o >> 2) & 3) {  // Bits 0 and 1 are segment, bits 2 and 3 are IND update
                    case 0: r += "DA,"; break;  // ES
                    case 1: r += "D0,"; break;  // segment 0
                    case 2: r += "DS,"; break;  // SS
                    case 3: r += "DD,"; break;  // DS
                }
                switch (o & 3) {  // Bits 0 and 1 are segment, bits 2 and 3 are IND update
                    case 0: r += "P2"; break;  // Increment IND by 2
                    case 1: r += "BL"; break;  // Adjust IND according to word size and DF
                    case 2: r += "M2"; break;  // Decrement IND by 2
                    case 3: r += "P0"; break;  // Don't adjust IND
                }
                r += "   ";
                break;
        }
        r += " ";
        if (f)
            r += "F";
        else
            r += " ";
        r += "  ";
        for (int i = 0; i < 13; ++i) {
            if ((_lastMicrocodePointer & (1 << (12 - i))) != 0)
                r += "1";
            else
                r += "0";
            if (i == 8)
                r += ".";
        }
        _lastMicrocodePointer = -1;
        return r;
    }
    bool condition(int n)
    {
        switch (n) {
            case 0x00: // F1ZZ
                if ((_group & groupF1ZZFromPrefix) != 0)
                    return _zero == (_f1 && _repne);
                return _zero != lowBit(_opcode);
            case 0x01: // MOD1
                return (_modRM & 0x40) != 0;
            case 0x02: // L8 - not sure if there's a better way to compute this
                if ((_group & groupLoadRegisterImmediate) != 0)
                    return (_opcode & 8) == 0;
                return !lowBit(_opcode) || (_opcode & 6) == 2;
            case 0x03: // Z
                return _zero;
            case 0x04: // NCZ
                --_counter;
                return _counter != -1;
            case 0x05: // TEST - no 8087 emulated yet
                return true;
            case 0x06: // OF
                return of();  // only used in INTO
            case 0x07: // CY
                return _carry;
            case 0x08: // UNC
                return true;
            case 0x09: // NF1
                return !_f1;
            case 0x0a: // NZ
                return !_zero;
            case 0x0b: // X0
                if ((_group & groupMicrocodePointerFromOpcode) == 0)
                    return (_modRM & 8) != 0;
                return (_opcode & 8) != 0;
            case 0x0c: // NCY
                return !_carry;
            case 0x0d: // F1
                return _f1;
            case 0x0e: // INT
                return interruptPending();
        }
        bool jump; // XC
        switch (_opcode & 0x0e) {
            case 0x00: jump = of(); break;  // O
            case 0x02: jump = cf(); break;  // C
            case 0x04: jump = zf(); break;  // Z
            case 0x06: jump = cf() || zf(); break;  // BE
            case 0x08: jump = sf(); break;  // S
            case 0x0a: jump = pf(); break;  // P
            case 0x0c: jump = (sf() != of()); break;  // L
            case 0x0e: jump = (sf() != of()) || zf(); break;  // LE
        }
        if (lowBit(_opcode))
            jump = !jump;
        return jump;
    }
    bool interruptPending()
    {
        return _nmiRequested || (intf() && _interruptPending);
    }
    void doPZS(Word v)
    {
        static Byte table[0x100] = {
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
            4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4 };
        _parity = table[v & 0xff];
        _zero = ((v & (_wordSize ? 0xffff : 0xff)) == 0);
        _sign = topBit(v);
    }
    void doFlags(DWord result, bool of, bool af)
    {
        _carry = ((result & (_wordSize ? 0x10000 : 0x100)) != 0);
        doPZS(result);
        _overflow = of;
        _auxiliary = af;
    }
    Word bitwise(Word data)
    {
        doFlags(data, false, false);
        return data;
    }
    bool lowBit(DWord v) { return (v & 1) != 0; }
    bool topBit(int w) { return (w & (_wordSize ? 0x8000 : 0x80)) != 0; }
    bool topBit(DWord w) { return (w & (_wordSize ? 0x8000 : 0x80)) != 0; }
    Word topBit(bool v) { return v ? (_wordSize ? 0x8000 : 0x80) : 0; }
    Word add(DWord a, DWord b, bool c)
    {
        if (!_wordSize) {
            a &= 0xff;
            b &= 0xff;
        }
        DWord r = a + b + (c ? 1 : 0);
        doFlags(r, topBit((r ^ a) & (r ^ b)), ((a ^ b ^ r) & 0x10) != 0);
        return r;
    }
    Word sub(DWord a, DWord b, bool c)
    {
        if (!_wordSize) {
            a &= 0xff;
            b &= 0xff;
        }
        DWord r = a - (b + (c ? 1 : 0));
        doFlags(r, topBit((a ^ b) & (r ^ a)), ((a ^ b ^ r) & 0x10) != 0);
        return r;
    }
    DWord physicalAddress(int segment, Word offset)
    {
        return ((sr(segment) << 4) + offset) & 0xfffff;
    }

    enum BusState
    {
        t1,
        t2t3tWaitNotLast,
        t3tWaitLast,
        t4,
        tIdle,
    };

    String _log;
    BusEmulator _bus;

    int _stopIP;
    int _stopSeg;
    int _timeIP1;
    int _timeSeg1;

    int _cycle;
    int _logStartCycle;
    int _logEndCycle;
    int _executeEndCycle;
    bool _consoleLogging;

    bool _nmiRequested;  // Not actually set anywhere yet

    BusState _busState;
    bool _prefetching;

    IOType _ioType;
    DWord _ioAddress;
    Byte _ioReadData;
    Byte _ioWriteData;
    int _ioSegment;
    IOType _lastIOType;

    SnifferDecoder _snifferDecoder;

    Word _registers[32];
    DWord _queue;
    int _queueBytes;
    Byte* _byteRegisters[8];
    Byte _microcode[4*512];
    Byte _microcodeIndex[2048];
    Word _translation[256];
    DWord _groups[257];
    DWord _group;
    DWord _nextGroup;
    Word _microcodePointer;
    Word _nextMicrocodePointer;
    Word _microcodeReturn;
    int _counter; // only 4 bits on the CPU
    Byte _alu;
    int _segmentOverride;
    bool _f1;
    bool _repne;
    bool _lock;
    Byte _opcode;
    Byte _modRM;
    bool _carry;
    bool _zero;
    bool _auxiliary;
    bool _sign;
    Byte _parity;
    bool _overflow;
    int _aluInput;
    Byte _nextModRM;
    int _loaderState;
    bool _rni;
    bool _nx;
    MicrocodeState _state;
    int _source;
    int _destination;
    int _type;
    bool _updateFlags;
    Byte _operands;
    bool _mIsM;
    bool _skipRNI;
    bool _useMemory;
    bool _wordSize;
    int _segment;
    int _lastMicrocodePointer;
    bool _prefetchCompleting;
    bool _dequeueing;
    bool _ioDone;
    bool _ioWriteDone;
    int _ioCancelling;
    bool _ioRequested;
    bool _ioFirstIdle;
    bool _ioSecondIdle;
    bool _prefetchDelayed;
    bool _queueFlushing;
    bool _queueWasFilled;
    bool _interruptPending;
};
