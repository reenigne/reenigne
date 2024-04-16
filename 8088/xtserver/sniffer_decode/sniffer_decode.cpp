#include "alfe/main.h"

class Disassembler
{
public:
    Disassembler() : _byteCount(0) { }
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
            case 0xa1: return "MOV " + accum() + ", " + size() + "[" + iw() +
                "]";
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

class Program : public ProgramBase
{
    void run()
    {
        Disassembler disassembler;

        String data = File(_arguments[1], true).contents();
        auto q = &data[0];
        String o;
        int lengths[48];
        for (int i = 0; i < 48; ++i) {
            lengths[i] = q[0] | (q[1] << 8) | (q[2] << 16) | (q[3] << 24);
            q += 4;
        }

        int length = lengths[0];
        for (int i = 0; i < 16; ++i) {
            int l = lengths[i];
            if (l != length || l == -1) {
                length = max(length, l);
                if (l == -1)
                    o += String("Warning: missing sniffer item ") + decimal(i) + "\n";
                else
                    o += String("Warning: sniffer packets have inconsistent lengths.\n");
            }
        }
        bool fastSampling = (lengths[16] != -1);
        if (!fastSampling) {
            for (int i = 17; i < 48; ++i)
                if (lengths[i] != -1)
                    o += String("Warning: got some but not all fast-sampling sniffer packets.\n");
        }
        else {
            for (int i = 16; i < 48; ++i)
                if (lengths[i] != length) {
                    length = max(length, lengths[i]);
                    o += String("Warning: sniffer packets have inconsistent lengths.\n");
                }
        }
        if (length == -1) {
            o += String("Warning: sniffer activated but no data received.\n");
            return;
        }

        int t = 0;  // 0 = Tidle, 1 = T1, 2 = T2, 3 = T3, 4 = T4, 5 = Tw
        int tNext = 0;
        int d = -1;  // -1 = SI, 0 = S0, 1 = S1, 2 = S2, 3 = S3, 4 = S4, 5 = SW

        Byte queue[4];
        int queueLength = 0;

        int lastS = 0;
        bool lastIOW = false;
        bool lastIOR = false;
        bool lastMEMW = false;
        bool lastMEMR = false;

        for (int th = 0; th < length; ++th)
            for (int tl = 0; tl < (fastSampling ? 3 : 1); ++tl) {
                Byte p[16];
                for (int i = 0; i < 16; ++i) {
                    p[i] = q[th + (i + tl*16)*2048];
                    if ((p[i] & 0x80) != 0)
                        o += String("Warning: sniffer packet corrupted.\n");
                    if (i < 8) {
                        if ((p[i] & 0x40) != 0)
                            o += String("Warning: sniffer port C packet corrupted.\n");
                    }
                    else
                        p[i] = ((p[i] & 0x40) >> 6) | ((p[i] & 0x3f) << 2);
                }
                UInt32 cpu =
                      ((p[0x4] & 0x08) != 0 ? 0x80000 : 0)   // 35 A19/S6        O ADDRESS/STATUS: During T1, these are the four most significant address lines for memory operations. During I/O operations, these lines are LOW. During memory and I/O operations, status information is available on these lines during T2, T3, Tw, and T4. S6 is always low.
                    | ((p[0x7] & 0x10) != 0 ? 0x40000 : 0)   // 36 A18/S5        O The status of the interrupt enable flag bit (S5) is updated at the beginning of each clock cycle.
                    | ((p[0x6] & 0x10) != 0 ? 0x20000 : 0)   // 37 A17/S4        O  S4*2+S3 0 = Alternate Data, 1 = Stack, 2 = Code or None, 3 = Data
                    | ((p[0x5] & 0x10) != 0 ? 0x10000 : 0)   // 38 A16/S3        O
                    | ((p[0x4] & 0x10) != 0 ? 0x08000 : 0)   // 39 A15           O ADDRESS BUS: These lines provide address bits 8 through 15 for the entire bus cycle (T1ñT4). These lines do not have to be latched by ALE to remain valid. A15ñA8 are active HIGH and float to 3-state OFF during interrupt acknowledge and local bus ``hold acknowledge''.
                    | ((p[0x2] & 0x10) != 0 ? 0x04000 : 0)   //  2 A14
                    | ((p[0x1] & 0x10) != 0 ? 0x02000 : 0)   //  3 A13
                    | ((p[0x0] & 0x10) != 0 ? 0x01000 : 0)   //  4 A12
                    | ((p[0x3] & 0x08) != 0 ? 0x00800 : 0)   //  5 A11
                    | ((p[0x2] & 0x08) != 0 ? 0x00400 : 0)   //  6 A10
                    | ((p[0x1] & 0x08) != 0 ? 0x00200 : 0)   //  7 A9
                    | ((p[0x0] & 0x08) != 0 ? 0x00100 : 0)   //  8 A8
                    | ((p[0xb] & 0x04) != 0 ? 0x00080 : 0)   //  9 AD7          IO ADDRESS DATA BUS: These lines constitute the time multiplexed memory/IO address (T1) and data (T2, T3, Tw, T4) bus. These lines are active HIGH and float to 3-state OFF during interrupt acknowledge and local bus ``hold acknowledge''.
                    | ((p[0xa] & 0x04) != 0 ? 0x00040 : 0)   // 10 AD6
                    | ((p[0x9] & 0x04) != 0 ? 0x00020 : 0)   // 11 AD5
                    | ((p[0x8] & 0x04) != 0 ? 0x00010 : 0)   // 12 AD4
                    | ((p[0xb] & 0x08) != 0 ? 0x00008 : 0)   // 13 AD3
                    | ((p[0xa] & 0x08) != 0 ? 0x00004 : 0)   // 14 AD2
                    | ((p[0x9] & 0x08) != 0 ? 0x00002 : 0)   // 15 AD1
                    | ((p[0x8] & 0x08) != 0 ? 0x00001 : 0);  // 16 AD0
                UInt8 qs =
                      ((p[0xe] & 0x08) != 0 ? 1 : 0)         // 25 QS0           O QUEUE STATUS: provide status to allow external tracking of the internal 8088 instruction queue. The queue status is valid during the CLK cycle after which the queue operation is performed.
                    | ((p[0xf] & 0x08) != 0 ? 2 : 0);        // 24 QS1           0 = No operation, 1 = First Byte of Opcode from Queue, 2 = Empty the Queue, 3 = Subsequent Byte from Queue
                char qsc[] = ".IES";

                UInt8 s =
                      ((p[0xd] & 0x08) != 0 ? 1 : 0)         // 26 -S0           O STATUS: is active during clock high of T4, T1, and T2, and is returned to the passive state (1,1,1) during T3 or during Tw when READY is HIGH. This status is used by the 8288 bus controller to generate all memory and I/O access control signals. Any change by S2, S1, or S0 during T4 is used to indicate the beginning of a bus cycle, and the return to the passive state in T3 and Tw is used to indicate the end of a bus cycle. These signals float to 3-state OFF during ``hold acknowledge''. During the first clock cycle after RESET becomes active, these signals are active HIGH. After this first clock, they float to 3-state OFF.
                    | ((p[0xc] & 0x08) != 0 ? 2 : 0)         // 27 -S1           0 = Interrupt Acknowledge, 1 = Read I/O Port, 2 = Write I/O Port, 3 = Halt, 4 = Code Access, 5 = Read Memory, 6 = Write Memory, 7 = Passive
                    | ((p[0xf] & 0x04) != 0 ? 4 : 0);        // 28 -S2
                char sc[] = "ARWHCrwp";

                //bool lock      = ((p[0xe] & 0x04) == 0);     // 29 -LOCK    !87  O LOCK: indicates that other system bus masters are not to gain control of the system bus while LOCK is active (LOW). The LOCK signal is activated by the ``LOCK'' prefix instruction and remains active until the completion of the next instruction. This signal is active LOW, and floats to 3-state off in ``hold acknowledge''.
                bool lock      = ((p[0x3] & 0x10) == 0);    // JP5/1 - not on FPU
                // Note: rqgt0 is FPU's -RQ/GT0 which is CPU's -RQ/GT1
                bool rqgt0     = ((p[0xc] & 0x04) == 0);     // 31 -RQ/-GT0 !87 IO REQUEST/GRANT: pins are used by other local bus masters to force the processor to release the local bus at the end of the processor's current bus cycle. Each pin is bidirectional with RQ/GT0 having higher priority than RQ/GT1. RQ/GT has an internal pull-up resistor, so may be left unconnected.
                //bool rqgt1     = ((p[0xd] & 0x04) == 0);     // 30 -RQ/-GT1     IO REQUEST/GRANT: pins are used by other local bus masters to force the processor to release the local bus at the end of the processor's current bus cycle. Each pin is bidirectional with RQ/GT0 having higher priority than RQ/GT1. RQ/GT has an internal pull-up resistor, so may be left unconnected.
                bool ready     = ((p[0xd] & 0x10) != 0);     // 22 READY        I  READY: is the acknowledgement from the addressed memory or I/O device that it will complete the data transfer. The RDY signal from memory or I/O is synchronized by the 8284 clock generator to form READY. This signal is active HIGH. The 8088 READY input is not synchronized. Correct operation is not guaranteed if the set up and hold times are not met.
                bool test      = ((p[0xc] & 0x10) == 0);     // 23 -TEST        I  TEST: input is examined by the ``wait for test'' instruction. If the TEST input is LOW, execution continues, otherwise the processor waits in an ``idle'' state. This input is synchronized internally during each clock cycle on the leading edge of CLK.
                //bool rd        = ((p[0x7] & 0x08) == 0);     // 32 -RD      !87  O READ: Read strobe indicates that the processor is performing a memory or I/O read cycle, depending on the state of the IO/M pin or S2. This signal is used to read devices which reside on the 8088 local bus. RD is active LOW during T2, T3 and Tw of any read cycle, and is guaranteed to remain HIGH in T2 until the 8088 local bus has floated. This signal floats to 3-state OFF in ``hold acknowledge''.
                //bool intr      = ((p[0xa] & 0x10) != 0);     // 18 INTR     !87 I  INTERRUPT REQUEST: is a level triggered input which is sampled during the last clock cycle of each instruction to determine if the processor should enter into an interrupt acknowledge operation. A subroutine is vectored to via an interrupt vector lookup table located in system memory. It can be internally masked by software resetting the interrupt enable bit. INTR is internally synchronized. This signal is active HIGH.
                bool cpu_clk   = ((p[0x9] & 0x10) != 0);     // 19 CLK          I  CLOCK: provides the basic timing for the processor and bus controller. It is asymmetric with a 33% duty cycle to provide optimized internal timing.
                //bool nmi       = ((p[0xb] & 0x10) != 0);     // 17 NMI      !87 I  NON-MASKABLE INTERRUPT: is an edge triggered input which causes a type 2 interrupt. A subroutine is vectored to via an interrupt vector lookup table located in system memory. NMI is not maskable internally by software. A transition from a LOW to HIGH initiates the interrupt at the end of the current instruction. This input is internally synchronized.

                UInt32 address =
                      ((p[0x4] & 0x02) != 0 ? 0x80000 : 0)   // A12 +A19         O Address bits: These lines are used to address memory and I/O devices within the system. These lines are generated by either the processor or DMA controller.
                    | ((p[0x5] & 0x02) != 0 ? 0x40000 : 0)   // A13 +A18
                    | ((p[0x6] & 0x02) != 0 ? 0x20000 : 0)   // A14 +A17
                    | ((p[0x7] & 0x02) != 0 ? 0x10000 : 0)   // A15 +A16
                    | ((p[0x4] & 0x01) != 0 ? 0x08000 : 0)   // A16 +A15
                    | ((p[0x5] & 0x01) != 0 ? 0x04000 : 0)   // A17 +A14
                    | ((p[0x6] & 0x01) != 0 ? 0x02000 : 0)   // A18 +A13
                    | ((p[0x7] & 0x01) != 0 ? 0x01000 : 0)   // A19 +A12
                    | ((p[0xc] & 0x20) != 0 ? 0x00800 : 0)   // A20 +A11
                    | ((p[0xd] & 0x20) != 0 ? 0x00400 : 0)   // A21 +A10
                    | ((p[0xe] & 0x20) != 0 ? 0x00200 : 0)   // A22 +A9
                    | ((p[0xf] & 0x20) != 0 ? 0x00100 : 0)   // A23 +A8
                    | ((p[0xc] & 0x40) != 0 ? 0x00080 : 0)   // A24 +A7
                    | ((p[0xd] & 0x40) != 0 ? 0x00040 : 0)   // A25 +A6
                    | ((p[0xe] & 0x40) != 0 ? 0x00020 : 0)   // A26 +A5
                    | ((p[0xf] & 0x40) != 0 ? 0x00010 : 0)   // A27 +A4
                    | ((p[0xc] & 0x80) != 0 ? 0x00008 : 0)   // A28 +A3
                    | ((p[0xd] & 0x80) != 0 ? 0x00004 : 0)   // A29 +A2
                    | ((p[0xe] & 0x80) != 0 ? 0x00002 : 0)   // A30 +A1
                    | ((p[0xf] & 0x80) != 0 ? 0x00001 : 0);  // A31 +A0
                UInt8 data =
                      ((p[0x6] & 0x20) != 0 ? 0x80 : 0)      // A2  +D7         IO Data bits: These lines provide data bus bits 0 to 7 for the processor, memory, and I/O devices.
                    | ((p[0x7] & 0x20) != 0 ? 0x40 : 0)      // A3  +D6
                    | ((p[0xc] & 0x01) != 0 ? 0x20 : 0)      // A4  +D5
                    | ((p[0xd] & 0x01) != 0 ? 0x10 : 0)      // A5  +D4
                    | ((p[0xe] & 0x01) != 0 ? 0x08 : 0)      // A6  +D3
                    | ((p[0xf] & 0x01) != 0 ? 0x04 : 0)      // A7  +D2
                    | ((p[0x4] & 0x04) != 0 ? 0x02 : 0)      // A8  +D1
                    | ((p[0x5] & 0x04) != 0 ? 0x01 : 0);     // A9  +D0
                UInt8 dma =
                      ((p[0xf] & 0x10) != 0 ? 0x01 : 0)      //     +DRQ0 JP6/1 == U28.19 == U73.9
                    | ((p[0x0] & 0x01) != 0 ? 0x02 : 0)      // B18 +DRQ1       I  DMA Request: These lines are asynchronous channel requests used by peripheral devices to gain DMA service. They are prioritized with DRQ3 being the lowest and DRQl being the highest. A request is generated by bringing a DRQ line to an active level (high). A DRQ line must be held high until the corresponding DACK line goes active.
                    | ((p[0x8] & 0x01) != 0 ? 0x04 : 0)      // B6  +DRQ2
                    | ((p[0x2] & 0x01) != 0 ? 0x08 : 0)      // B16 +DRQ3
                    | ((p[0xb] & 0x20) == 0 ? 0x10 : 0)      // B19 -DACK0       O -DMA Acknowledge: These lines are used to acknowledge DMA requests (DRQ1-DRQ3) and to refresh system dynamic memory (DACK0). They are active low.
                    | ((p[0x1] & 0x01) == 0 ? 0x20 : 0)      // B17 -DACK1
                    | ((p[0x8] & 0x40) == 0 ? 0x40 : 0)      // B26 -DACK2
                    | ((p[0x3] & 0x01) == 0 ? 0x80 : 0);     // B15 -DACK3
                UInt8 dmas =
                      ((p[0x1] & 0x20) != 0 ? 0x01 : 0)      // JP9/4 HRQ DMA
                    | ((p[0x3] & 0x04) != 0 ? 0x02 : 0);     // JP4/1 HOLDA
                char dmasc[] = " h:H";
                UInt8 irq =
                      ((p[0x3] & 0x20) != 0 ? 0x01 : 0)      // JP9/2  +IRQ0
                    | ((p[0x2] & 0x20) != 0 ? 0x02 : 0)      // JP9/3  +IRQ1
                    | ((p[0xa] & 0x01) != 0 ? 0x04 : 0)      // B4  +IRQ2       I  Interrupt Request lines: These lines are used to signal the processor that an I/O device requires attention. An Interrupt Request is generated by raising an IRQ line (low to high) and holding it high until it is acknowledged by the processor (interrupt service routine).
                    | ((p[0x9] & 0x40) != 0 ? 0x08 : 0)      // B25 +IRQ3
                    | ((p[0xa] & 0x40) != 0 ? 0x10 : 0)      // B24 +IRQ4
                    | ((p[0xb] & 0x40) != 0 ? 0x20 : 0)      // B23 +IRQ5
                    | ((p[0x8] & 0x20) != 0 ? 0x40 : 0)      // B22 +IRQ6
                    | ((p[0x9] & 0x20) != 0 ? 0x80 : 0);     // B21 +IRQ7
                bool irqs =
                      ((p[0x4] & 0x20) != 0 ? 0x01 : 0);     // JP9/1 INT
                UInt8 cga =
                      ((p[0x8] & 0x80) != 0 ? 0x01 : 0)      // JP7/2  CGA HCLK
                    | ((p[0x9] & 0x80) != 0 ? 0x02 : 0);     // JP7/1  CGA LCLK
                UInt8 pit =
                      ((p[0x0] & 0x04) != 0 ? 0x001 : 0)     // JP4/4 clock
                    | ((p[0x1] & 0x04) != 0 ? 0x002 : 0)     // JP4/3 gate
                    | ((p[0x2] & 0x04) != 0 ? 0x004 : 0);    // JP4/2 output

                bool ior       = ((p[0x0] & 0x02) == 0);     // B14 -IOR         O -I/O Read Command: This command line instructs an I/O device to drive its data onto the data bus. It may be driven by the processor or the DMA controller. This signal is active low.
                bool iow       = ((p[0x1] & 0x02) == 0);     // B13 -IOW         O -I/O Write Command: This command line instructs an I/O device to read the data on the data bus. It may be driven by the processor or the DMA controller. This signal is active low.
                bool memr      = ((p[0x2] & 0x02) == 0);     // B12 -MEMR        O Memory Read Command: This command line instructs the memory to drive its data onto the data bus. It may be driven by the processor or the DMA controller. This signal is active low.
                bool memw      = ((p[0x3] & 0x02) == 0);     // B11 -MEMW        O Memory Write Command: This command line instructs the memory to store the data present on the data bus. It may be driven by the processor or the DMA controller. This signal is active low.

                //bool bus_reset = ((p[0x0] & 0x20) != 0);     // B2  +RESET DRV   O This line is used to reset or initialize system logic upon power-up or during a low line voltage outage. This signal is synchronized to the falling edge of clock and is active high.
                //bool iochchk   = ((p[0x5] & 0x20) == 0);     // A1  -I/O CH CK  I  -I/O Channel Check: This line provides the processor with parity (error) information on memory or devices in the I/O channel. When this signal is active low, a parity error is indicated.
                bool iochrdy   = ((p[0x6] & 0x04) != 0);     // A10 +I/O CH RDY I  I/O Channel Ready: This line, normally high (ready), is pulled low (not ready) by a memory or I/O device to lengthen I/O or memory cycles. It allows slower devices to attach to the I/O channel with a minimum of difficulty. Any slow device using this line should drive it low immediately upon detecting a valid address and a read or write command. This line should never be held low longer than 10 clock cycles. Machine cycles (I/O or memory) are extended by an integral number of CLK cycles (210 ns).
                bool aen       = ((p[0x7] & 0x04) != 0);     // A11 +AEN         O Address Enable: This line is used to de-gate the processor and other devices from the I/O channel to allow DMA transfers to take place. When this line is active (high), the DMA controller has control of the address bus, data bus, read command lines (memory and I/O), and the write command lines (memory and I/O).
                //bool bus_clk   = ((p[0xa] & 0x20) != 0);     // B20 CLOCK        O System clock: It is a divide-by-three of the oscillator and has a period of 210 ns (4.77 MHz). The clock has a 33% duty cycle.
                bool bus_ale   = ((p[0xa] & 0x80) != 0);     // B28 +ALE         O Address Latch Enable: This line is provided by the 8288 Bus Controller and is used on the system board to latch valid addresses from the processor. It is available to the I/O channel as an indicator of a valid processor address (when used with AEN). Processor addresses are latched with the failing edge of ALE.
                bool tc        = ((p[0xb] & 0x80) != 0);     // B27 +T/C         O Terminal Count: This line provides a pulse when the terminal count for any DMA channel is reached. This signal is active high.

                //UInt16 jumpers = 0
                //    | ((p[0x9] & 0x01) != 0 ? 0x00400 : 0)    // JP3/1  -DACK0BRD - redundant, same as ISA bus -DACK0
                //    | ((p[0x6] & 0x08) == 0 ? 0x02000 : 0)    // 33 MN/-MX     I    MINIMUM/MAXIMUM: indicates what mode the processor is to operate in. The two modes are discussed in the following sections. (zero for maximum mode on PC/XT)
                //    | ((p[0x5] & 0x08) == 0 ? 0x04000 : 0)    // 34 -SS0        O   Pin 34 is always high in the maximum mode.
                //    | ((p[0xb] & 0x01) == 0 ? 0x08000 : 0)    // Serial - should always be 1
                //    | ((p[0x8] & 0x10) != 0 ? 0x10000 : 0)    // GND - should always be 0
                //    | ((p[0xe] & 0x10) != 0 ? 0x20000 : 0);   // RESET - should always be 0

                o += String(hex(cpu, 5, false)) + " " +
                    codePoint(qsc[qs]) + codePoint(sc[s]) +
                    (rqgt0 ? "G" : ".") + (ready ? "." : "z") +
                    (test ? "T" : ".") + (lock ? "L" : ".") +
                    (fastSampling ? (cpu_clk ? "C" : ".") : "") +
                    "  " + hex(address, 5, false) + " " + hex(data, 2, false) +
                    " " + hex(dma, 2, false) + codePoint(dmasc[dmas]) + " " + hex(irq, 2, false) + (irqs ? "I" : " ") + " " +
                    hex(pit, 1, false) + hex(cga, 1, false) + " " + (ior ? "R" : ".") +
                    (iow ? "W" : ".") + (memr ? "r" : ".") +
                    (memw ? "w" : ".") + (iochrdy ? "." : "z") +
                    (aen ? "D" : ".") +
                    (fastSampling ? (bus_ale ? "a" : ".") : "") +
                    (tc ? "T" : ".");

                if (tl > 0) {
                    o += "\n";
                    continue;
                }

                o += "  ";
                if (s != 7 && s != 3)
                    switch (tNext) {
                        case 0:
                        case 4:
                            // T1 state occurs after transition out of passive
                            tNext = 1;
                            break;
                        case 1:
                            tNext = 2;
                            break;
                        case 2:
                            tNext = 3;
                            break;
                        case 3:
                            tNext = 5;
                            break;
                    }
                else
                    switch (t) {
                        case 4:
                            d = -1;
                        case 0:
                            tNext = 0;
                            break;
                        case 1:
                        case 2:
                            tNext = 6;
                            break;
                        case 3:
                        case 5:
                            d = -1;
                            tNext = 4;
                            break;
                    }
                switch (t) {
                    case 0: o += "  "; break;
                    case 1: o += "T1"; break;
                    case 2: o += "T2"; break;
                    case 3: o += "T3"; break;
                    case 4: o += "T4"; break;
                    case 5: o += "Tw"; break;
                    default: o += "!c"; tNext = 0; break;
                }
                o += " ";
                if (dmas == 3) {
                    switch (d) {
                        // This is a bit of a hack since we don't have access
                        // to the right lines to determine the DMA state
                        // properly. This probably breaks for memory-to-memory
                        // copies.
                        case -1: d = -2; break;
                        case -2: d = 0; break;
                        case 0: d = 1; break;
                        case 1: d = 2; break;
                        case 2: d = 3; break;
                        case 3:
                        case 5:
                            if ((lastIOW && lastMEMR) || (lastIOR && lastMEMW))
                                d = 4;
                            else
                                d = 5;
                            break;
                        case 4:
                            d = -1;
                    }
                }
                else
                    d = (d == 3 ? 4 : -1);
                switch (d) {
                    case -1: o += "  "; break;
                    case -2: o += "SA"; break;
                    case 0: o += "S0"; break;
                    case 1: o += "S1"; break;
                    case 2: o += "S2"; break;
                    case 3: o += "S3"; break;
                    case 4: o += "S4"; break;
                    case 5: o += "SW"; break;
                    default: o += "!d"; t = 0; break;
                }
                o += " ";
                String instruction;
                if (qs != 0) {
                    if (qs == 2)
                        queueLength = 0;
                    else {
                        Byte b = queue[0];
                        for (int i = 0; i < 3; ++i)
                            queue[i] = queue[i + 1];
                        --queueLength;
                        if (queueLength < 0) {
                            o += "!g";
                            queueLength = 0;
                        }
                        instruction = disassembler.disassemble(b, qs == 1);
                    }
                }
                if (tNext == 4 || d == 3) {
                    if (t == 4 && d == 4)
                        o += "!e";
                    String seg;
                    switch (cpu & 0x30000) {
                        case 0x00000: seg = "ES "; break;
                        case 0x10000: seg = "SS "; break;
                        case 0x20000: seg = "CS "; break;
                        case 0x30000: seg = "DS "; break;
                    }
                    String type = "-";
                    if (lastS == 0)
                        o += hex(data, 2, false) + " <-i           ";
                    else {
                        if (lastS == 4) {
                            type = "f";
                            seg = "   ";
                        }
                        if (d == 3) {
                            type = "d";
                            seg = "   ";
                        }
                        o += hex(data, 2, false) + " ";
                        if (ior || memr)
                            o += "<-" + type + " ";
                        else
                            o += type + "-> ";
                        if (memr || memw) {
                            if (d == 3) 
                                o += "[   " + hex(address, 5, false) + "]";
                            else
                                o += "[" + seg + hex(address, 5, false) + "]";
                        }
                        else
                            o += "port[" + hex(address, 4, false) + "]";
                        if (lastS == 4 && d != 3) {
                            if (queueLength >= 4)
                                o += "!f";
                            else {
                                queue[queueLength] = data;
                                ++queueLength;
                            }
                        }
                    }
                    o += " ";
                }
                else
                    o += "                  ";
                if (qs != 0)
                    o += codePoint(qsc[qs]);
                else
                    o += " ";
                o += " " + instruction + "\n";
                lastS = s;
                lastIOW = iow;
                lastIOR = ior;
                lastMEMW = memw;
                lastMEMR = memr;
                t = tNext;
            }
        File(_arguments[2], true).save(o);
    }
};
