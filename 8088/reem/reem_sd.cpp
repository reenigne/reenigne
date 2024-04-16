#include "alfe/main.h"
#include "alfe/config_file.h"
#include "alfe/user.h"
#include "alfe/bitmap.h"
#include "alfe/statement.h"
#include "alfe/stack.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#ifdef _WIN32
#define STDIN_FILENO _fileno(stdin)
#define STDOUT_FILENO _fileno(stdout)
#define STDERR_FILENO _fileno(stderr)
#include <direct.h>
#include <io.h>
#define mkdir(x, y) _mkdir(x)
#else
#include <unistd.h>
#include <sys/time.h>
#endif

void* alloc(size_t bytes)
{
    void* r = malloc(bytes);
    if (r == 0) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    return r;
}

template<class T> class EmulatorT;

typedef EmulatorT<void> Emulator;

class Address
{
public:
    Address(Word segment, Word offset) : _offset(offset), _segment(segment) { }
    Word segment() const { return _segment; }
    Word offset() const { return _offset; }
    Address operator+(int delta) const
    {
        return Address(_segment, static_cast<Word>(_offset + delta));
    }
    const Address& operator+=(int delta) { _offset += delta; return *this; }
    operator DWord() const { return ((_segment << 4) + _offset) & 0xfffff; }
    bool operator==(const Address& other) const
    {
        return DWord(*this) == other;
    }
    String toString() const
    {
        return hex(_segment, 4, false) + ":" + hex(_offset, 4, false);
    }
    Address changeSegment(Word segment) const
    {
        return Address(segment, *this - (segment << 4));
    }
    Address operator--() { --_offset; return *this; }
    Address operator++() { ++_offset; return *this; }
private:
    Word _offset;
    Word _segment;
};

template<class T> class MemoryT;

typedef MemoryT<void> Memory;

template<class T> class MemoryT
{
public:
    MemoryT()
    {
        _size = 1024*1024;
        _bytes.allocate(_size);
        _flags.allocate(_size);
        _segments.allocate(_size);
        _predecessors.allocate(_size);
        for (int i = 0; i < _size; ++i) {
            _bytes[i] = 0;
            _flags[i] = 0;
            _segments[i] = (i + 0x10) >> 4;  // Never valid
            _predecessors[i] = 0;
        }
        setBad(0xa0000, 0x18000);
        setBad(0xc0000, 0x40000);
    }
    void setEmulator(Emulator* emulator) { _emulator = emulator; }
    void setLoadSegment(Word loadSegment)
    {
        setBad(0x500, ((DWord)loadSegment << 4) - 0x600);
        // We hacked in an IRET at 0x00600 as an interrupt 0x1c handler
        _flags[0x600] &= ~bad;
    }
    bool isInitialized(Address address)
    {
        return (_flags[address] & initialized) != 0;
    }
    void setInitialized(Address address) { _flags[address] |= initialized; }
    Byte getByte(Address address, bool fetch, bool first,
        bool preStart = false)
    {
        if (!preStart)
            checkAddress(address);
        // TODO: put this back under control of config file option.
        //if ((_flags[address] & initialized) == 0) {
        //    _emulator->runtimeError("Reading uninitialized address " +
        //        address.toString() + ".");
        //}
        if (fetch) {
            if (first) {
                if ((_flags[address] & executed) != 0) {
                    if ((_flags[address] & firstByteOfInstruction) == 0) {
                        _emulator->runtimeError("Execute partway through "
                            "another instruction detected at address " +
                            address.toString() + ".");
                    }
                }
                _flags[address] |= firstByteOfInstruction;
            }
            _flags[address] |= executed;
            if ((_flags[address] & writtenAfterExecution) != 0) {
                _emulator->runtimeError("Execute/modify/execute detected at "
                    "address " + address.toString() + ".");
            }
            if ((_flags[address] & written) != 0) {
                _emulator->runtimeError("Execute of written byte detected at "
                    "address " + address.toString() + ".");
            }
            if ((_flags[address] & read) != 0) {
                _emulator->runtimeError("Execute of read byte detected at "
                    "address " + address.toString() + ".");
            }
        }
        else {
            if (!preStart)
                _flags[address] |= read;
            if ((_flags[address] & executed) != 0) {
                _emulator->runtimeError("Read of executed byte detected at "
                    "address " + address.toString() + ".");
            }
        }
        return getByte(address);
    }
    void writeByte(Address address, Byte value, bool preStart)
    {
        if (!preStart)
            checkAddress(address);
        //else
        //    setSegment(address);
        _bytes[address] = value;
        setInitialized(address);
        if (!preStart)
            _flags[address] |= written;
        if ((_flags[address] & executed) != 0) {
            _emulator->runtimeError("Write of executed byte detected at "
                "address " + address.toString() + ".");
            _flags[address] |= writtenAfterExecution;
        }
    }
    Byte getByte(Address address) { return _bytes[address]; }
    Word getWord(Address address)
    {
        return getByte(address) + (getByte(address + 1) << 8);
    }
    Address cb(Address address)
    {
        return address +
            static_cast<int>(static_cast<SInt8>(_bytes[address + 1]));
    }
    Address cw(Address address)
    {
        return Address(address.segment(), getWord(address + 1));
    }
    Address cp(Address address)
    {
        return Address(getWord(address + 3), getWord(address + 1));
    }
    Byte* getBytes() { return &_bytes[0]; }
    int size() { return _size; }
    bool wasRead(Address address) { return (_flags[address] & read) != 0; }
    bool wasWritten(Address address)
    {
        return (_flags[address] & written) != 0;
    }
    bool firstByte(Address address)
    {
        return (_flags[address] & firstByteOfInstruction) != 0;
    }
    bool executedFirstByte(Address address)
    {
        return ((_flags[address] & executed) != 0) && firstByte(address);
    }
    bool data(Address address)
    {
        return ((_flags[address] & read) != 0) ||
            ((_flags[address] & written) != 0) ||
            (((_flags[address] & executed) != 0 ||
              (_flags[address] & staticallyDisassembled) != 0) &&
             ((_flags[address] & firstByteOfInstruction) == 0)) ||
            ((_flags[address] & initialized) == 0) ||
            ((_flags[address] & bad) != 0);
    }
    Word segment(Address address)
    {
        return _segments[address];
    }
    bool segmentValid(Address address)
    {
        return address == address.changeSegment(_segments[address]);
    }
    bool setSegment(Address address)
    {
        Word segment = address.segment();
        bool returnValue = (_segments[address] == segment);
        bool wasValid = segmentValid(address);
        _segments[address] = segment;
        if (!wasValid)
            return true;
        return returnValue;
    }
    void setStatic(Address address, bool first)
    {
        _flags[address] |= staticallyDisassembled;
        if (first)
            _flags[address] |= firstByteOfInstruction;
    }
    void setBad(Address address) { _flags[address] |= bad; }
    Address addressFromPhysical(DWord physical)
    {
        Address a(physical >> 4, physical & 0xf);
        if (!segmentValid(a))
            return a;
        return a.changeSegment(_segments[a]);
    }

    // Adds an edge where execution passes from "from" to "to".
    void addPredecessor(Address from, Address to)
    {
        Predecessor* p = new Predecessor(from);
        p->_next = _predecessors[to];
        _predecessors[to] = p;
    }
    class Predecessor
    {
    public:
        Predecessor(Address address) : _address(address) { }
        Predecessor* _next;
        Address _address;
    };
    Predecessor* getPredecessor(Address address)
    {
        return _predecessors[address];
    }
    ~MemoryT()
    {
        for (int i = 0; i < _size; ++i) {
            Predecessor* p = _predecessors[i];
            while (p != 0) {
                Predecessor* p1 = p->_next;
                delete p;
                p = p1;
            }
        }
    }
private:
    void checkAddress(Address address)
    {
        Word oldSegment = _segments[address];
        if (!setSegment(address)) {
            // TODO: put this back under control of config file option.
                //_emulator->runtimeError("Byte at " + address.toString() +
            //    " previously accessed at " +
            //    address.changeSegment(oldSegment).toString() + ".");
        }
        if ((_flags[address] & bad) != 0) {
            // TODO: put this back under control of config file option.
            //_emulator->runtimeError("Accessing invalid address " +
            //    address.toString() + ".");
        }
    }
    void setBad(int start, int bytes)
    {
        for (int i = 0; i < bytes; ++i)
            _flags[i + start] |= bad;
    }
    enum Flags {
        initialized = 1,
        staticallyDisassembled = 2,
        executed = 4,
        firstByteOfInstruction = 8,
        writtenAfterExecution = 0x10,
        bad = 0x20,
        read = 0x40,
        written = 0x80,
    };
    int _size;
    Array<Byte> _bytes;
    Array<Byte> _flags;
    Array<Word> _segments;
    Array<Predecessor*> _predecessors;
    Emulator* _emulator;
};

class StaticDisassembler
{
public:
    StaticDisassembler(Memory* memory)
      : _memory(memory), _address(0, 0)
    {
        _size = memory->size();
        _bytes = memory->getBytes();
    }
    void run()
    {
        // Search through for executed instructions that are followed by a
        // non-executed instruction
        for (int i = 0; i < _size; ++i) {
            Address a = _memory->addressFromPhysical(i);
            if (!_memory->executedFirstByte(a))
                continue;
            int size = getInstructionSize(a);
            //Word segment = _memory->segment(a);
            //DWord base = segment << 4;
            //Word offset = i - base;
            if (size == 0) {
                error("Invalid instruction executed at address " +
                    a.toString() + ".");
            }
            Address next = a + size;
            int type = getInstructionType(a);
            if (_memory->executedFirstByte(next)) {
                // Add predecessor links. These won't be needed for statically
                // finding instructions, but will be useful for later
                // disassembly.
                processEndOfBasicBlock(a);
                continue;
            }
            if (type == 5) {  // TODO: do same check for unconditional JMP/CALL destinations?
                // TODO: put this back
                //error("Instruction should have continued at address " +
                //    a.toString() + ". Try running for longer.");
                continue;
            }
            pushTask(type, next);
        }
        // Iteratively mark code and non-code using the task stacks
        do {
            int i;
            for (i = 0; i < 5; ++i) {
                if (!_tasks[i].empty())
                    break;
            }
            if (i == 5)
                break;
            Address a = _tasks[i].pop();
            if (i == 0) {
                // This address is not code
                if (_memory->data(a)) {
                    // We already knew that. Can this ever happen?
                    continue;
                }
                _memory->setBad(a);
                // Look at previous instruction
                Address previous = findPreviousAddress(a);
                int type = getInstructionType(previous);
                int size = getInstructionSize(previous);
                if (type == 5) {
                    // Previous instruction continues to here, so that is bad
                    // too.
                    pushTask(0, previous);
                }
                if (type == 1) {
                    // Check the taken path
                    if (_memory->data(_memory->cb(previous)))
                        pushTask(0, previous);
                }
                // Look at predecessor instructions
                auto predecessor = _memory->getPredecessor(a);
                while (predecessor != 0) {
                    Address p = predecessor->_address;
                    int pType = getInstructionType(p);
                    if (pType == 2 || pType == 3)
                        pushTask(0, p);
                    if (pType == 1) {
                        // Check the untaken path
                        if (_memory->data(p + getInstructionSize(p)))
                            pushTask(0, p);
                    }
                }
                continue;
            }

            do {
                // Do a static disassembly starting at address
                if (_memory->data(a)) {
                    // Address was read, written, uninitialized or
                    // out-of-bounds - don't try to disassemble.
                    break;
                }
                if (_memory->firstByte(a)) {
                    // Already done this one
                    break;
                }
                if (!_memory->setSegment(a)) {
                    // Segment is wrong. Can this ever happen?
                    break;
                }
                _memory->setStatic(a, true);
                int size = getInstructionSize(a);
                if (size == 0) {
                    _memory->setBad(a);
                    pushTask(0, a);
                    // Invalid instruction - don't try to disassemble.
                    break;
                }
                for (int i = 1; i < size; ++i)
                    _memory->setStatic(a + i, false);
                Address next = processEndOfBasicBlock(a);
                if (next == a)
                    break;
                a = next;
            } while (true);
        } while (true);
        // Output resulting assembly
        //bool isCode = false;
        bool executed = false;
        bool read = false;
        bool written = false;
        Word segment = 1;
        for (int i = 0; i < _size; ++i) {
            Address a = _memory->addressFromPhysical(i);
            Word newSegment = a.segment();
            if (newSegment != segment) {
                console.write("; Segment = " + hex(newSegment, 4, false) +
                    "\n");
                segment = newSegment;
            }
            if (_memory->firstByte(a)) {
                bool newExecuted = _memory->executedFirstByte(a);
                if (newExecuted != executed) {
                    if (newExecuted)
                        console.write("; --- Dynamic ---\n");
                    else
                        console.write("; --- Static ---\n");
                    executed = newExecuted;
                }
                String instruction = disassembleInstruction(a);
                Memory::Predecessor* predecessor = _memory->getPredecessor(a);
                if (predecessor != 0) {
                    for (int x = instruction.length(); x < 30; ++x)
                        instruction += " ";
                    instruction += "; we get here from ";
                    while (predecessor != 0) {
                        instruction += a.toString();
                        Memory::Predecessor* next = predecessor->_next;
                        if (next != 0)
                            instruction += ", ";
                        predecessor = next;
                    }
                }
                console.write("  " + instruction + "\n");
                i += getInstructionSize(a) - 1;
            }
            else {
                bool newRead = _memory->wasRead(a);
                bool newWritten = _memory->wasWritten(a);
                if (newRead != read || newWritten != written) {
                    if (newRead) {
                        if (newWritten)
                            console.write("; --- Read and written ---\n");
                        else
                            console.write("; --- Read only ---\n");
                    }
                    else {
                        if (newWritten)
                            console.write("; --- Written only ---\n");
                        else
                            console.write("; --- Neither read nor written ---\n");
                    }
                    read = newRead;
                    written = newWritten;
                }

                Word offset = a.offset();
                int x;
                String instruction = "  db ";
                String ascii = " ; ";
                for (x = 0; x < (offset & 0xf); ++x) {
                    instruction += "      ";
                    ascii += " ";
                }
                bool needComma = false;
                for (; x < 0x10; ++x) {
                    if (_memory->firstByte(a))
                        break;
                    if (_memory->wasRead(a) != read ||
                        _memory->wasWritten(a) != written)
                        break;
                    if (needComma)
                        instruction += ", ";
                    else
                        ascii += "\"";
                    Byte b = _memory->getByte(a);
                    ++a;
                    ++i;
                    instruction += hex(b, 2);
                    needComma = true;
                    if (b >= 0x20 && b < 0x7f)
                        ascii += codePoint(b);
                    else
                        ascii += ".";
                }
                console.write(instruction + ascii + "\"\n");
            }

        }
    }
private:
    Address _address;
    Byte _opcode;
    Byte _modRM;
    bool _wordSize;
    bool _doubleWord;
    int _offset;
    String regMemPair()
    {
        if ((_opcode & 2) == 0)
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
        static String b[8] = {
            "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" };
        return b[r];
    }
    String wordRegs(int r)
    {
        static String w[8] = {
            "ax", "cx", "dx", "bx", "sp", "bp", "si", "di" };
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
            return (!_wordSize ? "b" : "w");
        else
            return (!_wordSize ? "" : "d");
    }
    String disp()
    {
        static String d[8] = {
            "bx+si", "bx+di", "bp+si", "bp+di", "si", "di", "bp", "bx" };
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
            "add ", "or ", "adc ", "sbb ", "and ", "sub ", "xor ", "cmp " };
        return o[op];
    }
    int op0() { return _opcode & 7; }
    int op1() { return (_opcode >> 3) & 7; }
    int mod() { return _modRM >> 6; }
    int reg() { return (_modRM >> 3) & 7; }
    int rm() { return _modRM & 7; }
    String imm(bool m = false) { return !_wordSize ? ib(m) : iw(m); }
    String iw(bool m = false)
    {
        if (m)
            ea();
        return hex(_memory->getWord(_address + _offset), 4, false);
    }
    String ib(bool m = false)
    {
        if (m)
            ea();
        return hex(_memory->getByte(_address + _offset), 2, false);
    }
    String sb(bool m = false)
    {
        if (m)
            ea();
        UInt8 byte = _memory->getByte(_address + _offset);
        if ((byte & 0x80) == 0)
            return "+" + hex(byte, 2, false);
        return "-" + hex(-byte, 2, false);
    }
    String accum() { return !_wordSize ? "al" : "ax"; }
    String segreg(int r)
    {
        static String sr[8] = { "es", "cs", "ss", "ds" };
        return sr[r & 3];
    }
    String cb() { return _memory->cb(_address).toString(); }
    String cw() { return _memory->cw(_address).toString(); }
    String cp() { return _memory->cp(_address).toString(); }
    String port() { return ((op1() & 1) == 0 ? ib() : wordRegs(2)); }
    String disassembleInstruction(Address address)
    {
        _address = address;
        _opcode = _memory->getByte(address);
        _wordSize = (_opcode & 1) != 0;
        _doubleWord = false;
        _offset = 1;
        if ((_opcode & 0xc4) == 0)
            return alu(op1()) + regMemPair();
        if ((_opcode & 0xc6) == 4)
            return alu(op1()) + accum() + ", " + imm();
        if ((_opcode & 0xe7) == 6)
            return "push " + segreg(op1());
        if ((_opcode & 0xe7) == 7)
            return "pop " + segreg(op1());
        if ((_opcode & 0xe7) == 0x26)
            return segreg(op1() & 3) + ":";
        if ((_opcode & 0xf8) == 0x40)
            return "inc " + rwo();
        if ((_opcode & 0xf8) == 0x48)
            return "dec " + rwo();
        if ((_opcode & 0xf8) == 0x50)
            return "push " + rwo();
        if ((_opcode & 0xf8) == 0x58)
            return "pop " + rwo();
        if ((_opcode & 0xfc) == 0x80)
            return alu(reg()) + ea() + ", " +
            (_opcode == 0x81 ? iw(true) : sb(true));
        if ((_opcode & 0xfc) == 0x88)
            return "mov " + regMemPair();
        if ((_opcode & 0xf8) == 0x90)
            if (_opcode == 0x90)
                return "nop";
            else
                return "xchg ax, " + rwo();
        if ((_opcode & 0xf8) == 0xb0)
            return "mov " + rbo() + ", " + ib();
        if ((_opcode & 0xf8) == 0xb8)
            return "mov " + rwo() + ", " + iw();
        if ((_opcode & 0xfc) == 0xd0) {
            static String shifts[8] = {
                "rol", "ror", "rcl", "rcr", "shl", "shr", "shl", "sar" };
            return shifts[reg()] + " " + ea() + ", " +
                ((op0() & 2) == 0 ? String("1") : byteRegs(1));
        }
        if ((_opcode & 0xf8) == 0xd8) {
            _wordSize = false;
            _doubleWord = true;
            return String("esc ") + op0() + ", " + reg() + ", " + ea();
        }
        if ((_opcode & 0xf6) == 0xe4)
            return "in " + accum() + ", " + port();
        if ((_opcode & 0xf6) == 0xe6)
            return "out " + port() + ", " + accum();
        if ((_opcode & 0xe0) == 0x60) {
            static String conds[16] = {
                "o", "no", "b", "ae", "e", "ne", "be", "a",
                "s", "ns", "p", "np", "l", "ge", "le", "g" };
            return "j" + conds[_opcode & 0xf] + " " + cb();
        }
        switch (_opcode) {
            case 0x27: return "daa";
            case 0x2f: return "das";
            case 0x37: return "aaa";
            case 0x3f: return "aas";
            case 0x84:
            case 0x85: return "test " + regMemPair();
            case 0x86:
            case 0x87: return "xchg " + regMemPair();
            case 0x8c:
                _wordSize = true;
                return "mov " + ea() + ", " + segreg(reg());
            case 0x8d:
                _doubleWord = true;
                _wordSize = false;
                return "lea " + rw() + ", " + ea();
            case 0x8e:
                _wordSize = true;
                return "mov " + segreg(reg()) + ", " + ea();
            case 0x8f: return "pop " + ea();
            case 0x98: return "cbw";
            case 0x99: return "cwd";
            case 0x9a: return "call " + cp();
            case 0x9b: return "wait";
            case 0x9c: return "pushf";
            case 0x9d: return "popf";
            case 0x9e: return "sahf";
            case 0x9f: return "lahf";
            case 0xa0:
            case 0xa1:
                return "mov " + accum() + ", " + size() + "[" + iw() + "]";
            case 0xa2:
            case 0xa3: return "mov " + size() + "[" + iw() + "], " + accum();
            case 0xa4:
            case 0xa5: return "movs" + size();
            case 0xa6:
            case 0xa7: return "cmps" + size();
            case 0xa8:
            case 0xa9: return "test " + accum() + ", " + imm();
            case 0xaa:
            case 0xab: return "stos" + size();
            case 0xac:
            case 0xad: return "lods" + size();
            case 0xae:
            case 0xaf: return "scas" + size();
            case 0xc0:
            case 0xc2: return "ret " + iw();
            case 0xc1:
            case 0xc3: return "ret";
            case 0xc4: _doubleWord = true; return "les " + rw() + ", " + ea();
            case 0xc5:
                _doubleWord = true;
                _wordSize = false;
                return "lds " + rw() + ", " + ea();
            case 0xc6:
            case 0xc7: return "mov " + ea() + ", " + imm(true);
            case 0xc8:
            case 0xca: return "retf " + iw();
            case 0xc9:
            case 0xcb: return "retf";
            case 0xcc: return "int 3";
            case 0xcd: return "int " + ib();
            case 0xce: return "into";
            case 0xcf: return "iret";
            case 0xd4: return "aam " + ib();
            case 0xd5: return "aad " + ib();
            case 0xd6: return "salc";
            case 0xd7: return "xlatb";
            case 0xe0: return "loopne " + cb();
            case 0xe1: return "loope " + cb();
            case 0xe2: return "loop " + cb();
            case 0xe3: return "jcxz " + cb();
            case 0xe8: return "call " + cw();
            case 0xe9: return "jmp " + cw();
            case 0xea: return "jmp " + cp();
            case 0xeb: return "jmp " + cb();
            case 0xf0:
            case 0xf1: return "lock";
            case 0xf2: return "repne ";
            case 0xf3: return "rep ";
            case 0xf4: return "hlt";
            case 0xf5: return "cmc";
            case 0xf6:
            case 0xf7:
                switch (reg()) {
                    case 0:
                    case 1: return "test " + ea() + ", " + imm(true);
                    case 2: return "not " + ea();
                    case 3: return "neg " + ea();
                    case 4: return "mul " + ea();
                    case 5: return "imul " + ea();
                    case 6: return "div " + ea();
                    case 7: return "idiv " + ea();
                }
            case 0xf8: return "clc";
            case 0xf9: return "stc";
            case 0xfa: return "cli";
            case 0xfb: return "sti";
            case 0xfc: return "cld";
            case 0xfd: return "std";
            case 0xfe:
            case 0xff:
                switch (reg()) {
                    case 0: return "inc " + ea();
                    case 1: return "dec " + ea();
                    case 2: return "call " + ea();
                    case 3: _doubleWord = true; return "call " + ea();
                    case 4: return "jmp " + ea();
                    case 5: _doubleWord = true; return "jmp " + ea();
                    case 7:
                    case 6: return "push " + ea();
                }
        }
        return "???";  // Should never get here
    }
    void error(String message)
    {
        errorConsole.write(message + "\n");
        exit(1);
    }
    // Returns true if we should not continue disassembly
    Address processEndOfBasicBlock(Address a)
    {
        int type = getInstructionType(a);
        Byte opcode = _memory->getByte(a);
        Address cp = _memory->cp(a);
        Address cb = _memory->cb(a);
        Address cw = _memory->cw(a);
        int size = getInstructionSize(a);
        if (type != 5 && !_memory->executedFirstByte(a + size))
            pushTask(type, a + size);
        switch (type) {
            case 1: // Conditional jump
                _memory->addPredecessor(a, cb);
                if (!_memory->executedFirstByte(cb))
                    pushTask(1, cb);
                // Continue disassembling at next instruction.
                return a + size;
            case 2: // call
            case 3: // jump
                // Push the following instruction.
                switch (opcode) {
                    case 0x9a:  // CALL cd
                    case 0xea:  // JMP cd
                        // Continue at the destination.
                        _memory->addPredecessor(a, cp);
                        return cp;
                    case 0x9b:  // WAIT
                    case 0xcc:  // INT 3
                    case 0xcd:  // INT
                    case 0xce:  // INTO
                    case 0xf4:  // HLT
                    case 0xfe:  // CALL/JMP rmb (illegal)
                    case 0xff:  // CALL/JMP rm
                        // Don't continue.
                        return a;
                    case 0xe8:  // CALL cw
                    case 0xe9:  // JMP cw
                        // Continue at the destination.
                        _memory->addPredecessor(a, cw);
                        return cw;
                    case 0xeb:  // JMP cb
                        _memory->addPredecessor(a, cb);
                        return cb;
                    default:
                        error("Unexpected instruction");
                }
                break;
            case 4: // ret
                return a;
            case 5: // normal instruction
                return a + size;
        }
        return a;
    }
    void pushTask(int type, Address address) { _tasks[type].push(address); }
    int getInstructionSize(Address address)
    {
        static const int s[] = {
            -1,-1,-1,-1, 2, 3, 1, 1,-1,-1,-1,-1, 2, 3, 1, 0,
            -1,-1,-1,-1, 2, 3, 1, 1,-1,-1,-1,-1, 2, 3, 1, 1,
            -1,-1,-1,-1, 2, 3, 1, 1,-1,-1,-1,-1, 2, 3, 1, 1,
            -1,-1,-1,-1, 2, 3, 1, 1,-1,-1,-1,-1, 2, 3, 1, 1,
             1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1,
             1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1,
             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
             2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
            -2,-3, 0,-2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
             1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5, 1, 1, 1, 1, 1,
             3, 3, 3, 3, 1, 1, 1, 1, 2, 3, 1, 1, 1, 1, 1, 1,
             2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
             0, 0, 3, 1,-1,-1,-2,-3, 0, 0, 3, 1, 1, 2, 1, 1,
             2, 2, 2, 2, 2, 2, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,
             2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 5, 2, 1, 1, 1, 1,
             1, 0, 1, 1, 1, 1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1
        };
        static const int ms[] = {
            0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 2, 0,
            0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 2, 0,
            0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 2, 0,
            0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 2, 0,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        };

        Byte opcode = _memory->getByte(address);
        int l = s[opcode];
        if (l < 0) {
            Byte modrm = _memory->getByte(address + 1);
            l = -l + ms[modrm];
            if ((opcode & 0xfc) == 0xe0 && (modrm & 0x38) == 0x30) {
                // SETMO
                return 0;
            }
            if (opcode == 0xff && (modrm & 0x38) == 0x38) {
                // Undocumented PUSH rmw alias
                return 0;
            }
            if (opcode == 0xfe && (modrm & 0x38) >= 0x10) {
                // CALL/JMP/PUSH indirect byte
                return 0;
            }
            if ((opcode == 0x8d || (opcode & 0xfe) == 0x84) &&
                (modrm & 0xc0) == 0xc0) {
                // LEA/LES/LDS reg,reg
                return 0;
            }
            if (opcode == 0x8e && (modrm & 0x38) == 1) {
                // MOV to CS
                return 0;
            }
            if ((opcode & 0xfd) == 0x8c && (modrm & 0x38) >= 0x20) {
                // MOV to/from invalid segreg
                return 0;
            }
            if (opcode == 0xff &&
                ((modrm & 0xf8) == 0xd8 || (modrm & 0xf8) == 0xe8)) {
                // CALL/JMP far indirect register
                return 0;
            }
            if ((opcode & 0xfe) == 0xf6 && (modrm & 0x38) == 8) {
                // TEST alias
                return 0;
            }
            if (opcode == 0x8f && (modrm & 0x38) != 0) {
                // POP rmw alias
                return 0;
            }
        }
        DWord segment = _memory->segment(address);
        DWord base = segment << 4;
        int offset = address - base;
        if ((address + l).offset() < address.offset()) {
            if (_memory->firstByte(address)) {
                error("Instruction wrapped segment at address " +
                    address.toString() + ".");
            }
            else
                l = 0;
        }
        return l;
    }
    int getInstructionType(Address address)
    {
        // 0 = Invalid instruction - don't disassemble
        // 1 = Conditional branch - at least one successor is definitely code.
        // 2 = Call - probably has code afterwards
        // 3 = Unconditional jump - may or may not have code afterwards.
        // 4 = Return - any compiler may put non-code after these.
        // 5 = Normal instruction - successor is definitely code.
        static const int types[] = {
             5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 0,
             5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
             5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
             5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
             5, 5, 5, 5, 0, 5, 5, 5, 5, 5, 5, 5, 0, 5, 5, 5,
             5, 5, 5, 5, 0, 5, 5, 5, 5, 5, 5, 5, 0, 5, 5, 5,
             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
             1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
             5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
             5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 2, 2, 5, 5, 5, 5,
             5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
             5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
             0, 0, 4, 4, 5, 5, 5, 5, 0, 0, 4, 4, 2, 2, 2, 4,
             5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
             1, 1, 1, 1, 5, 5, 5, 5, 2, 3, 3, 3, 5, 5, 5, 5,
             5, 0, 5, 5, 2, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5
        };
        Byte opcode = _memory->getByte(address);
        if (opcode >= 0xfe) {
            static const int types2[] = {5, 5, 2, 2, 3, 3, 5, 5};
            return types2[(_memory->getByte(address + 1) >> 3) & 7];
        }
        return types[opcode];
    }
    Address findPreviousAddress(Address address)
    {
        --address;
        do {
            if (_memory->firstByte(address))
                return address;
        } while (true);
    }

    Stack<Address> _tasks[5];
    int _size;
    Memory* _memory;
    Byte* _bytes;
};

template<class T> class EmulatorT
{
public:
    void setMemory(Memory* memory)
    {
        _memory = memory;
        memory->setEmulator(this);
    }
    void run(Array<String> arguments, Program* program)
    {
        if (arguments.count() < 2) {
            console.write("Usage: " + arguments[0] + " <config file>\n");
            exit(0);
        }

        List<StructuredType::Member> regionTypeMembers;
        regionTypeMembers.add(StructuredType::Member("start", IntegerType()));
        regionTypeMembers.add(StructuredType::Member("end", IntegerType()));
        StructuredType regionType("Region", regionTypeMembers);
        List<StructuredType::Member> functionTypeMembers;
        functionTypeMembers.add(StructuredType::Member("address", IntegerType()));
        functionTypeMembers.add(StructuredType::Member("name", StringType()));
        StructuredType functionType("Function", functionTypeMembers);
        ConfigFile configFile;
        List<Value> emptyList;
        configFile.addOption("commandLine", StringType());
        configFile.addDefaultOption("loadSegment", 0x1000);
        Type regionsType = ArrayType(regionType, IntegerType());
        configFile.addDefaultOption("reRegions", regionsType,
            Value(regionsType, emptyList));
        Type functionsType = ArrayType(functionType, IntegerType());
        configFile.addDefaultOption("functions", functionsType,
            Value(functionsType, emptyList));
        configFile.load(File(arguments[1], true));
        String commandLine = configFile.get<String>("commandLine");
        loadSegment = configFile.get<int>("loadSegment");
        _memory->setLoadSegment(loadSegment);
        List<Value> reRegionsValue = configFile.get<List<Value>>("reRegions");
        regions = reRegionsValue.count();
        reRegions = (DWord*)alloc(regions * 2 * sizeof(DWord));
        int r = 0;
        for (auto i : reRegionsValue) {
            auto stv = i.value<HashTable<Identifier, Value>>();
            reRegions[r] = stv["start"].value<int>();
            reRegions[r + 1] = stv["end"].value<int>();
            r += 2;
        }
        List<Value> functionsValue = configFile.get<List<Value>>("functions");
        for (auto i : functionsValue) {
            auto stv = i.value<HashTable<Identifier, Value>>();
            functionNames.add(stv["address"].value<int>(),
                stv["name"].value<String>());
        }

        int iosToTimerIRQ = 0;
        int iosToFrame = 0;
        NullTerminatedString arg1(commandLine);
        filename = arg1;
    #ifdef _WIN32
        _set_fmode(_O_BINARY);
    #endif
        FILE* fp = fopen(filename, "rb");
        if (fp == 0)
            error("opening");
        pathBuffers[0] = (char*)alloc(0x10000);
        pathBuffers[1] = (char*)alloc(0x10000);
        if (fseek(fp, 0, SEEK_END) != 0)
            error("seeking");
        length = ftell(fp);
        if (length == -1)
            error("telling");
        if (fseek(fp, 0, SEEK_SET) != 0)
            error("seeking");
        int loadOffset = loadSegment << 4;
        if (length > 0x100000 - loadOffset)
            length = 0x100000 - loadOffset;
        if (fread(_memory->getBytes() + loadOffset, length, 1, fp) != 1)
            error("reading");
        fclose(fp);
        for (int i = 0; i < length; ++i)
            _memory->setInitialized(
                Address(loadSegment + ((i & 0xf0000) >> 4), i & 0xffff));
        for (int i = 0; i < 4; ++i)
            registers[8 + i] = loadSegment - 0x10;
        if (length >= 2 && readWord(0x100, -1, true) == 0x5a4d) {  // .exe file?
            if (length < 0x21) {
                fprintf(stderr, "%s is too short to be an .exe file\n", filename);
                exit(1);
            }
            Word bytesInLastBlock = readWord(0x102, -1, true);
            int exeLength = ((readWord(0x104, -1, true) - (bytesInLastBlock == 0 ? 0 : 1))
                << 9) + bytesInLastBlock;
            int headerParagraphs = readWord(0x108, -1, true);
            int headerLength = headerParagraphs << 4;
            if (exeLength > length || headerLength > length ||
                headerLength > exeLength) {
                fprintf(stderr, "%s is corrupt\n", filename);
                exit(1);
            }
            int relocationCount = readWord(0x106, -1, true);
            Word imageSegment = loadSegment + headerParagraphs;
            int relocationData = readWord(0x118, -1, true);
            for (int i = 0; i < relocationCount; ++i) {
                int offset = readWord(relocationData + 0x100, -1, true);
                setCS(readWord(relocationData + 0x102, -1, true) + imageSegment);
                writeWord(readWord(offset, 1, true) + imageSegment, offset, 1,
                    true);
                relocationData += 4;
            }
            loadSegment = imageSegment;  // Prevent further access to header
            Word ss = readWord(0x10e, -1, true) + loadSegment;  // SS
            setSS(ss);
            setSP(readWord(0x110, -1, true));
            stackLow = ((exeLength - headerLength + 15) >> 4) + loadSegment;
            if (stackLow < ss)
                stackLow = 0;
            else
                stackLow = (stackLow - (int)ss) << 4;
            ip = readWord(0x114, -1, true);
            setCS(readWord(0x116, -1, true) + loadSegment);  // CS
        }
        else {
            if (length > 0xff00) {
                fprintf(stderr, "%s is too long to be a .com file\n", filename);
                exit(1);
            }
            setSP(0xFFFE);
            stackLow = length + 0x100;
        }
        setDS(0);
        int interrupts[5] = { 0, 4, 5, 6, 0x1c };
        for (int i = 0; i < 5; ++i) {
            writeWord(0, interrupts[i] * 4);
            writeWord(0, interrupts[i] * 4 + 2);
        }
        writeWord(0x600, 0x70);   // Interrupt 0x1c vector
        writeByte(0xcf, 0x600, -1, true);   // IRET
        writeWord(0x3d4, 0x463);  // BIOS CRTC base port address
        int envSegment = loadSegment - 0x1c;
        setDS(envSegment);
        const char* env = "PATH=C:\\";
        Word envLen = static_cast<Word>(strlen(env));
        for (int i = 0; i < envLen; ++i)
            writeByte(env[i], i, -1, true);
        writeByte(0, envLen, -1, true);
        writeByte(0, envLen + 1, -1, true);
        writeWord(1, envLen + 2, -1, true);
        int i;
        for (i = 0; filename[i] != 0; ++i)
            writeByte(filename[i], i + 4 + envLen, -1, true);
        if (i + 4 >= 0xc0) {
            fprintf(stderr, "Program name too long.\n");
            exit(1);
        }
        writeWord(0, i + 3 + envLen, -1, true);
        setDS(loadSegment - 0x10);
        writeWord(envSegment, 0x2c, -1, true);
        writeWord(0x9fff, 0x02, -1, true);
        i = 0x81;
        for (int a = 2; a < arguments.count(); ++a) {
            if (a > 2) {
                writeByte(' ', i, -1, true);
                ++i;
            }
            NullTerminatedString aa(arguments[a]);
            const char* arg = aa;
            bool quote = strchr(arg, ' ') != 0;
            if (quote) {
                writeByte('\"', i, -1, true);
                ++i;
            }
            for (; *arg != 0; ++arg) {
                int c = *arg;
                if (c == '\"') {
                    writeByte('\\', i, -1, true);
                    ++i;
                }
                writeByte(c, i, -1, true);
                ++i;
            }
            if (quote) {
                writeByte('\"', i, -1, true);
                ++i;
            }
        }
        if (i > 0xff) {
            fprintf(stderr, "Arguments too long.\n");
            exit(1);
        }
        writeByte(i - 0x81, 0x80, -1, true);
        writeByte(13, i, -1, true);
        // Some testcases copy uninitialized stack data, so mark as initialized
        // any locations that could possibly be stack.
        for (DWord d = (loadSegment << 4) + length;
            d < (DWord)((registers[10] << 4) + sp()); ++d) {
            //setES(d >> 4);
            //writeByte(0, d & 15, 0, true);
            setES(registers[10]);
            writeByte(0, d - (registers[10] << 4), 0, true);
        }
        ios = 0;
        setDS(loadSegment - 0x10);
        setES(loadSegment - 0x10);
        setAX(0x0000);
        setCX(0x00FF);
        setDX(segment);
        registers[3] = 0x0000;  // BX
        registers[5] = 0x091C;  // BP
        setSI(0x0100);
        setDI(0xFFFE);
        fileDescriptors = (int*)alloc(6 * sizeof(int));
        fileDescriptors[0] = STDIN_FILENO;
        fileDescriptors[1] = STDOUT_FILENO;
        fileDescriptors[2] = STDERR_FILENO;
        fileDescriptors[3] = STDOUT_FILENO;
        fileDescriptors[4] = STDOUT_FILENO;
        fileDescriptors[5] = -1;
        Byte* byteData = (Byte*)&registers[0];
        int bigEndian = (byteData[2] == 0 ? 1 : 0);
        int byteNumbers[8] = { 0, 2, 4, 6, 1, 3, 5, 7 };
        for (int i = 0; i < 8; ++i)
            byteRegisters[i] = &byteData[byteNumbers[i] ^ bigEndian];
        running = true;
        bool prefix = false;
        int lastios = 0;
        for (int i = 0; i < /*1000000000*/ 50000; ++i) {
            int iosElapsed = ios - lastios;
            lastios = ios;
            iosToTimerIRQ -= iosElapsed;
            iosToFrame -= iosElapsed;
            if (!prefix && intf() && iosToTimerIRQ <= 0) {
                iosToTimerIRQ += 65536;
                push(flags);
                push(cs());
                push(ip);
                setCS(0);
                ip = readWord(0x70, 1);
                setCS(readWord(0x72, 1));
            }
            if (iosToFrame <= 0) {
                iosToFrame += 19912;
                program->update();
            }
            if (i >= 0 /* 48697*/) {
                printf("%09i %04x:%04x ", i, cs(), ip);
                for (int j = 0; j < 12; ++j) {
                    if (j != 9) {
                        printf("%04x",registers[j]);
                        if (j != 11)
                            printf(" ");
                        else
                            printf("\n");
                    }
                }
            }
            if (!repeating) {
                if (!prefix) {
                    segmentOverride = -1;
                    rep = 0;
                }
                prefix = false;
                opcode = fetchByte(true);
            }
            if (rep != 0 && (opcode < 0xa4 || opcode >= 0xb0 || opcode == 0xa8 ||
                opcode == 0xa9))
                runtimeError("REP prefix with non-string instruction");
            wordSize = ((opcode & 1) != 0);
            bool sourceIsRM = ((opcode & 2) != 0);
            int operation = (opcode >> 3) & 7;
            bool jump;
            int fileDescriptor;
            switch (opcode) {
                case 0x00: case 0x01: case 0x02: case 0x03:
                case 0x08: case 0x09: case 0x0a: case 0x0b:
                case 0x10: case 0x11: case 0x12: case 0x13:
                case 0x18: case 0x19: case 0x1a: case 0x1b:
                case 0x20: case 0x21: case 0x22: case 0x23:
                case 0x28: case 0x29: case 0x2a: case 0x2b:
                case 0x30: case 0x31: case 0x32: case 0x33:
                case 0x38: case 0x39: case 0x3a: case 0x3b:  // alu rmv,rmv
                    data = readEA();
                    if (!sourceIsRM) {
                        destination = data;
                        source = getReg();
                    }
                    else {
                        destination = getReg();
                        source = data;
                    }
                    aluOperation = operation;
                    doALUOperation();
                    if (aluOperation != 7) {
                        if (!sourceIsRM)
                            finishWriteEA(data);
                        else
                            setReg(data);
                    }
                    break;
                case 0x04: case 0x05: case 0x0c: case 0x0d:
                case 0x14: case 0x15: case 0x1c: case 0x1d:
                case 0x24: case 0x25: case 0x2c: case 0x2d:
                case 0x34: case 0x35: case 0x3c: case 0x3d:  // alu accum,i
                    destination = getAccum();
                    source = !wordSize ? fetchByte() : fetchWord();
                    aluOperation = operation;
                    doALUOperation();
                    if (aluOperation != 7)
                        setAccum();
                    break;
                case 0x06: case 0x0e: case 0x16: case 0x1e:  // PUSH segreg
                    push(registers[operation + 8]);
                    break;
                case 0x07: case 0x17: case 0x1f:  // POP segreg
                    registers[operation + 8] = pop();
                    break;
                case 0x26: case 0x2e: case 0x36: case 0x3e:  // segment override
                    segmentOverride = operation - 4;
                    prefix = true;
                    break;
                case 0x27: case 0x2f:  // DA
                    if (af() || (al() & 0x0f) > 9) {
                        data = al() + (opcode == 0x27 ? 6 : -6);
                        setAL(data);
                        setAF(true);
                        if ((data & 0x100) != 0)
                            setCF(true);
                    }
                    setCF(cf() || al() > 0x9f);
                    if (cf())
                        setAL(al() + (opcode == 0x27 ? 0x60 : -0x60));
                    wordSize = false;
                    data = al();
                    setPZS();
                    break;
                case 0x37: case 0x3f:  // AA
                    if (af() || (al() & 0xf) > 9) {
                        setAL(al() + (opcode == 0x37 ? 6 : -6));
                        setAH(ah() + (opcode == 0x37 ? 1 : -1));
                        setCA();
                    }
                    else
                        clearCA();
                    setAL(al() & 0x0f);
                    break;
                case 0x40: case 0x41: case 0x42: case 0x43:
                case 0x44: case 0x45: case 0x46: case 0x47:
                case 0x48: case 0x49: case 0x4a: case 0x4b:
                case 0x4c: case 0x4d: case 0x4e: case 0x4f:  // incdec rw
                    destination = rw();
                    wordSize = true;
                    setRW(incdec((opcode & 8) != 0));
                    break;
                case 0x50: case 0x51: case 0x52: case 0x53:
                case 0x54: case 0x55: case 0x56: case 0x57:  // PUSH rw
                    push(rw());
                    break;
                case 0x58: case 0x59: case 0x5a: case 0x5b:
                case 0x5c: case 0x5d: case 0x5e: case 0x5f:  // POP rw
                    setRW(pop());
                    break;
                case 0x70: case 0x71: case 0x72: case 0x73:
                case 0x74: case 0x75: case 0x76: case 0x77:
                case 0x78: case 0x79: case 0x7a: case 0x7b:
                case 0x7c: case 0x7d: case 0x7e: case 0x7f:  // Jcond cb
                    switch (opcode & 0x0e) {
                        case 0x00: jump = of(); break;
                        case 0x02: jump = cf(); break;
                        case 0x04: jump = zf(); break;
                        case 0x06: jump = cf() || zf(); break;
                        case 0x08: jump = sf(); break;
                        case 0x0a: jump = pf(); break;
                        case 0x0c: jump = sf() != of(); break;
                        default:   jump = sf() != of() || zf(); break;
                    }
                    jumpShort(fetchByte(), jump == ((opcode & 1) == 0));
                    break;
                case 0x80: case 0x81: case 0x82: case 0x83:  // alu rmv,iv
                    destination = readEA();
                    data = fetch(opcode == 0x81);
                    if (opcode != 0x83)
                        source = data;
                    else
                        source = signExtend(data);
                    aluOperation = modRMReg();
                    doALUOperation();
                    if (aluOperation != 7)
                        finishWriteEA(data);
                    break;
                case 0x84: case 0x85:  // TEST rmv,rv
                    data = readEA();
                    test(data, getReg());
                    break;
                case 0x86: case 0x87:  // XCHG rmv,rv
                    data = readEA();
                    finishWriteEA(getReg());
                    setReg(data);
                    break;
                case 0x88: case 0x89:  // MOV rmv,rv
                    ea();
                    finishWriteEA(getReg());
                    break;
                case 0x8a: case 0x8b:  // MOV rv,rmv
                    setReg(readEA());
                    break;
                case 0x8c:  // MOV rmw,segreg
                    ea();
                    wordSize = 1;
                    finishWriteEA(registers[modRMReg() + 8]);
                    break;
                case 0x8d:  // LEA
                    address = ea();
                    if (!useMemory)
                        runtimeError("LEA needs a memory address");
                    setReg(address);
                    break;
                case 0x8e:  // MOV segreg,rmw
                    wordSize = 1;
                    data = readEA();
                    registers[modRMReg() + 8] = data;
                    break;
                case 0x8f:  // POP rmw
                    writeEA(pop());
                    break;
                case 0x90: case 0x91: case 0x92: case 0x93:
                case 0x94: case 0x95: case 0x96: case 0x97:  // XCHG AX,rw
                    data = ax();
                    setAX(rw());
                    setRW(data);
                    break;
                case 0x98:  // CBW
                    setAX(signExtend(al()));
                    break;
                case 0x99:  // CWD
                    setDX((ax() & 0x8000) == 0 ? 0x0000 : 0xffff);
                    break;
                case 0x9a:  // CALL cp
                    savedIP = fetchWord();
                    savedCS = fetchWord();
                    farCall();
                    break;
                case 0x9c:  // PUSHF
                    push((flags & 0x0fd7) | 0xf000);
                    break;
                case 0x9d:  // POPF
                    flags = pop() | 2;
                    break;
                case 0x9e:  // SAHF
                    flags = (flags & 0xff02) | ah();
                    break;
                case 0x9f:  // LAHF
                    setAH(flags & 0xd7);
                    break;
                case 0xa0: case 0xa1:  // MOV accum,xv
                    segment = 3;
                    data = read(fetchWord());
                    setAccum();
                    break;
                case 0xa2: case 0xa3:  // MOV xv,accum
                    segment = 3;
                    write(getAccum(), fetchWord());
                    break;
                case 0xa4: case 0xa5:  // MOVSv
                    if (rep == 0 || cx() != 0)
                        stoS(lodS());
                    doRep(false);
                    break;
                case 0xa6: case 0xa7:  // CMPSv
                    if (rep == 0 || cx() != 0) {
                        destination = lodS();
                        source = lodDIS();
                        sub();
                    }
                    doRep(true);
                    break;
                case 0xa8: case 0xa9:  // TEST accum,iv
                    data = fetch(wordSize);
                    test(getAccum(), data);
                    break;
                case 0xaa: case 0xab:  // STOSv
                    if (rep == 0 || cx() != 0)
                        stoS(getAccum());
                    doRep(false);
                    break;
                case 0xac: case 0xad:  // LODSv
                    if (rep == 0 || cx() != 0) {
                        data = lodS();
                        setAccum();
                    }
                    doRep(false);
                    break;
                case 0xae: case 0xaf:  // SCASv
                    if (rep == 0 || cx() != 0) {
                        destination = getAccum();
                        source = lodDIS();
                        sub();
                    }
                    doRep(true);
                    break;
                case 0xb0: case 0xb1: case 0xb2: case 0xb3:
                case 0xb4: case 0xb5: case 0xb6: case 0xb7:
                    setRB(fetchByte());
                    break;
                case 0xb8: case 0xb9: case 0xba: case 0xbb:
                case 0xbc: case 0xbd: case 0xbe: case 0xbf:  // MOV rv,iv
                    setRW(fetchWord());
                    break;
                case 0xc2: case 0xc3: case 0xca: case 0xcb:  // RET
                    recordReturn();
                    savedIP = pop();
                    savedCS = (opcode & 8) == 0 ? cs() : pop();
                    if (!wordSize)
                        setSP(sp() + fetchWord());
                    farJump();
                    break;
                case 0xc4: case 0xc5:  // LES/LDS
                    ea();
                    farLoad();
                    *modRMRW() = savedIP;
                    registers[8 + (!wordSize ? 0 : 3)] = savedCS;
                    break;
                case 0xc6: case 0xc7:  // MOV rmv,iv
                    ea();
                    finishWriteEA(fetch(wordSize));
                    break;
                case 0xcd:
                    data = fetchByte();
                    switch (data) {
                        case 0x10:
                            switch (ah()) {
                                case 0:
                                    break;
                                case 0x0f:
                                    setAX(0x2804);
                                    setBH(0);
                                    break;
                                default:
                                    fprintf(stderr, "Unknown BIOS video call "
                                        "0x%02x", ah());
                                    runtimeError("");
                            }
                            break;
                        case 0x12:
                            setAX(640);
                            break;
                        case 0x16:
                            switch (ah()) {
                                case 1:
                                    setZF(true);
                                    setAX(0);
                                    break;
                                default:
                                    fprintf(stderr, "Unknown BIOS keyboard call "
                                        "0x%02x", ah());
                                    runtimeError("");
                            }
                            break;
                        case 0x1a:
                            switch (ah()) {
                                case 0:
                                    {
                                        //time_t t = time(0);
                                        //struct tm* lt = localtime(&t);
                                        //data = (lt->tm_hour*60 + lt->tm_min)*60 +
                                        //    lt->tm_sec;
                                        //setDX(data);
                                        //setCX(data >> 16);
                                        setDX(0);
                                        setCX(0);  // All decompilations take place at midnight
                                    }
                                    break;
                                default:
                                    fprintf(stderr, "Unknown BIOS clock call "
                                        "0x%02x", ah());
                                    runtimeError("");
                            }
                            break;
                        case 0x21:
                            switch (ah()) {
                                case 0x0e:
                                    setCF(false);
                                    break;
                                case 0x19:
                                    setCF(false);
                                    setAL(0);
                                    break;
                                case 0x25:
                                    {
                                        Word t = es();
                                        setES(0);
                                        writeWord(dx(), al() * 4, 0);
                                        writeWord(ds(), al() * 4 + 2, 0);
                                        setES(t);
                                    }
                                    break;
                                case 0x2c:
                                    {
    //#ifdef _WIN32
    //                                    SYSTEMTIME t;
    //                                    GetLocalTime(&t);
    //                                    setCH((Byte)t.wHour);
    //                                    setCL((Byte)t.wMinute);
    //                                    setDH((Byte)t.wSecond);
    //                                    setDL(t.wMilliseconds / 1000);
    //#else
    //                                    time_t t = time(0);
    //                                    struct tm* lt = localtime(&t);
    //                                    struct timeval tv;
    //                                    struct timezone tz;
    //                                    gettimeofday(&tv, &tz);
    //                                    setCH(lt->tm_hour);
    //                                    setCL(lt->tm_min);
    //                                    setDH(lt->tm_sec);
    //                                    setDL(tv.tv_usec / 10000);
    //#endif
                                        setDX(0);
                                        setCX(0);  // All decompilations take place at midnight
                                    }
                                    break;
                                case 0x30:
                                    setAX(0x1403);
                                    break;
                                case 0x35:
                                    setES(0);
                                    setBX(readWord(al() * 4, 0));
                                    setES(readWord(al() * 4 + 2, 0));
                                    break;
                                case 0x39:
                                    if (mkdir(dsdx(), 0700) == 0)
                                        setCF(false);
                                    else {
                                        setCF(true);
                                        setAX(dosError(errno));
                                    }
                                    break;
                                case 0x3a:
                                    if (rmdir(dsdx()) == 0)
                                        setCF(false);
                                    else {
                                        setCF(true);
                                        setAX(dosError(errno));
                                    }
                                    break;
                                case 0x3b:
                                    if (chdir(dsdx()) == 0)
                                        setCF(false);
                                    else {
                                        setCF(true);
                                        setAX(dosError(errno));
                                    }
                                    break;
                                case 0x3c:
                                    fileDescriptor = creat(dsdx(), 0700);
                                    if (fileDescriptor != -1) {
                                        setCF(false);
                                        int guestDescriptor = getDescriptor();
                                        setAX(guestDescriptor);
                                        fileDescriptors[guestDescriptor] =
                                            fileDescriptor;
                                    }
                                    else {
                                        setCF(true);
                                        setAX(dosError(errno));
                                    }
                                    break;
                                case 0x3d:
                                    printf("Opening file %s\n", dsdx());
                                    fileDescriptor = open(dsdx(), al() & 3, 0700);
                                    if (fileDescriptor != -1) {
                                        setCF(false);
                                        setAX(getDescriptor());
                                        fileDescriptors[ax()] = fileDescriptor;
                                    }
                                    else {
                                        setCF(true);
                                        setAX(dosError(errno));
                                    }
                                    break;
                                case 0x3e:
                                    fileDescriptor = getFileDescriptor();
                                    if (fileDescriptor == -1) {
                                        setCF(true);
                                        setAX(6);  // Invalid handle
                                        break;
                                    }
                                    if (fileDescriptor >= 5 &&
                                        close(fileDescriptor) != 0) {
                                        setCF(true);
                                        setAX(dosError(errno));
                                    }
                                    else {
                                        fileDescriptors[bx()] = -1;
                                        setCF(false);
                                    }
                                    break;
                                case 0x3f:
                                    fileDescriptor = getFileDescriptor();
                                    if (fileDescriptor == -1) {
                                        setCF(true);
                                        setAX(6);  // Invalid handle
                                        break;
                                    }
                                    printf("Reading %i bytes from %i to "
                                        "%04x:%04x\n", cx(), bx(), ds(), dx());
                                    data = ::read(fileDescriptor,
                                        pathBuffers[0], cx());
                                    dsdx(true, cx());
                                    if (data == (DWord)-1) {
                                        setCF(true);
                                        setAX(dosError(errno));
                                    }
                                    else {
                                        setCF(false);
                                        setAX(data);
                                    }
                                    break;
                                case 0x40:
                                    fileDescriptor = getFileDescriptor();
                                    if (fileDescriptor == -1) {
                                        setCF(true);
                                        setAX(6);  // Invalid handle
                                        break;
                                    }
                                    data = ::write(fileDescriptor,
                                        dsdx(false, cx()), cx());
                                    if (data == (DWord)-1) {
                                        setCF(true);
                                        setAX(dosError(errno));
                                    }
                                    else {
                                        setCF(false);
                                        setAX(data);
                                    }
                                    break;
                                case 0x41:
                                    if (unlink(dsdx()) == 0)
                                        setCF(false);
                                    else {
                                        setCF(true);
                                        setAX(dosError(errno));
                                    }
                                    break;
                                case 0x42:
                                    fileDescriptor = getFileDescriptor();
                                    if (fileDescriptor == -1) {
                                        setCF(true);
                                        setAX(6);  // Invalid handle
                                        break;
                                    }
                                    data = lseek(fileDescriptor,
                                        (cx() << 16) + dx(), al());
                                    if (data != (DWord)-1) {
                                        setCF(false);
                                        setDX(data >> 16);
                                        setAX(data);
                                    }
                                    else {
                                        setCF(true);
                                        setAX(dosError(errno));
                                    }
                                    break;
                                case 0x44:
                                    switch (al()) {
                                        case 0:
                                            fileDescriptor = getFileDescriptor();
                                            if (fileDescriptor == -1) {
                                                setCF(true);
                                                setAX(6);  // Invalid handle
                                                break;
                                            }
                                            data = isatty(fileDescriptor);
                                            if (data == 1) {
                                                setDX(0x80);
                                                setCF(false);
                                            }
                                            else {
                                                if (errno == ENOTTY) {
                                                    setDX(0);
                                                    setCF(false);
                                                }
                                                else {
                                                    setAX(dosError(errno));
                                                    setCF(true);
                                                }
                                            }
                                            break;
                                        case 8:
                                            setCF(false);
                                            setAX(bl() < 3 ? 0 : 1);
                                            break;
                                        case 0x0e:
                                            setCF(false);
                                            setAL(0);
                                            break;
                                        default:
                                            fprintf(stderr, "Unknown IOCTL 0x%02x",
                                                al());
                                            runtimeError("");
                                    }
                                    break;
                                case 0x47:
                                    if (getcwd(pathBuffers[0], 64) != 0) {
                                        setCF(false);
                                        initString(si(), 3, true, 0);
                                    }
                                    else {
                                        setCF(true);
                                        setAX(dosError(errno));
                                    }
                                    break;
                                case 0x4a:
                                    break;
                                case 0x4c:
                                    printf("*** Bytes: %i\n", length);
                                    printf("*** Cycles: %i\n", ios);
                                    printf("*** EXIT code %i\n", al());
                                    exit(0);
                                    break;
                                case 0x56:
                                    if (rename(dsdx(), initString(di(), 0,
                                        false, 1)) == 0)
                                        setCF(false);
                                    else {
                                        setCF(true);
                                        setAX(dosError(errno));
                                    }
                                    break;
                                default:
                                    fprintf(stderr, "Unknown DOS call 0x%02x",
                                        ah());
                                    runtimeError("");
                            }
                            break;
                        case 0x33:
                            switch (ax()) {
                                case 0x00:
                                    setAX(-1);
                                    setBX(2);
                                    break;
                                case 0x03:
                                    setCX(320);
                                    setDX(100);
                                    setBX(0);
                                    break;
                                default:
                                    fprintf(stderr, "Unknown mouse call 0x%02x",
                                        ax());
                                    runtimeError("");
                            }
                            break;
                        default:
                            fprintf(stderr, "Unknown interrupt 0x%02x", data);
                            runtimeError("");
                    }
                    break;
                case 0xcf:
                    ip = pop();
                    setCS(pop());
                    flags = pop() | 2;
                    break;
                case 0xd0: case 0xd1: case 0xd2: case 0xd3:  // rot rmv,n
                    data = readEA();
                    if ((opcode & 2) == 0)
                        source = 1;
                    else
                        source = cl();
                    while (source != 0) {
                        destination = data;
                        switch (modRMReg()) {
                            case 0:  // ROL
                                data <<= 1;
                                doCF();
                                data |= (cf() ? 1 : 0);
                                setOFRotate();
                                break;
                            case 1:  // ROR
                                setCF((data & 1) != 0);
                                data >>= 1;
                                if (cf())
                                    data |= (!wordSize ? 0x80 : 0x8000);
                                setOFRotate();
                                break;
                            case 2:  // RCL
                                data = (data << 1) | (cf() ? 1 : 0);
                                doCF();
                                setOFRotate();
                                break;
                            case 3:  // RCR
                                data >>= 1;
                                if (cf())
                                    data |= (!wordSize ? 0x80 : 0x8000);
                                setCF((destination & 1) != 0);
                                setOFRotate();
                                break;
                            case 4:  // SHL
                            case 6:
                                data <<= 1;
                                doCF();
                                setOFRotate();
                                setPZS();
                                break;
                            case 5:  // SHR
                                setCF((data & 1) != 0);
                                data >>= 1;
                                setOFRotate();
                                setAF(true);
                                setPZS();
                                break;
                            case 7:  // SAR
                                setCF((data & 1) != 0);
                                data >>= 1;
                                if (!wordSize)
                                    data |= (destination & 0x80);
                                else
                                    data |= (destination & 0x8000);
                                setOFRotate();
                                setAF(true);
                                setPZS();
                                break;
                        }
                        --source;
                    }
                    finishWriteEA(data);
                    break;
                case 0xd4:  // AAM
                    data = fetchByte();
                    if (data == 0)
                        divideOverflow();
                    setAH(al() / data);
                    setAL(al() % data);
                    wordSize = true;
                    setPZS();
                    break;
                case 0xd5:  // AAD
                    data = fetchByte();
                    setAL(al() + ah() * data);
                    setAH(0);
                    setPZS();
                    break;
                case 0xd6:  // SALC
                    setAL(cf() ? 0xff : 0x00);
                    break;
                case 0xd7:  // XLATB
                    setAL(readByte(bx() + al()));
                    break;
                case 0xe0: case 0xe1: case 0xe2:  // LOOPc cb
                    setCX(cx() - 1);
                    jump = (cx() != 0);
                    switch (opcode) {
                        case 0xe0: if (zf()) jump = false; break;
                        case 0xe1: if (!zf()) jump = false; break;
                    }
                    jumpShort(fetchByte(), jump);
                    break;
                case 0xe3:  // JCXZ cb
                    jumpShort(fetchByte(), cx() == 0);
                    break;
                case 0xe4:  // IN AL,ib
                    setAL(inportb(fetchByte()));
                    break;
                case 0xe5:  // IN AX,ib
                    data = fetchByte();
                    setAL(inportb(data));
                    setAH(inportb(data + 1));
                    break;
                case 0xe6:  // OUT ib,AL
                    outportb(fetchByte(), al());
                    break;
                case 0xe7:  // OUT ib,AX
                    data = fetchByte();
                    outportb(data, al());
                    outportb(data + 1, ah());
                    break;
                case 0xe8:  // CALL cw
                    data = fetchWord();
                    call(ip + data);
                    break;
                case 0xe9:  // JMP cw
                    ip += fetchWord();
                    break;
                case 0xea:  // JMP cp
                    savedIP = fetchWord();
                    savedCS = fetchWord();
                    farJump();
                    break;
                case 0xeb:  // JMP cb
                    jumpShort(fetchByte(), true);
                    break;
                case 0xec:  // IN AL,DX
                    setAL(inportb(dx()));
                    break;
                case 0xed:  // IN AX,DX
                    setAL(inportb(dx()));
                    setAH(inportb(dx() + 1));
                    break;
                case 0xee:  // OUT DX,AX
                    outportb(dx(), al());
                    outportb(dx() + 1, ah());
                    break;
                case 0xef:  // OUT DX,AL
                    outportb(dx(), al());
                    break;
                case 0xf2: case 0xf3:  // REP
                    rep = opcode == 0xf2 ? 1 : 2;
                    prefix = true;
                    break;
                case 0xf5:  // CMC
                    flags ^= 1;
                    break;
                case 0xf6: case 0xf7:  // math rmv
                    data = readEA();
                    switch (modRMReg()) {
                        case 0: case 1:  // TEST rmv,iv
                            test(data, fetch(wordSize));
                            break;
                        case 2:  // NOT iv
                            finishWriteEA(~data);
                            break;
                        case 3:  // NEG iv
                            source = data;
                            destination = 0;
                            sub();
                            finishWriteEA(data);
                            break;
                        case 4: case 5:  // MUL rmv, IMUL rmv
                            source = data;
                            destination = getAccum();
                            data = destination;
                            setSF();
                            setPF();
                            data *= source;
                            setAX(data);
                            if (!wordSize) {
                                if (modRMReg() == 4)
                                    setCF(ah() != 0);
                                else {
                                    if ((source & 0x80) != 0)
                                        setAH(ah() - destination);
                                    if ((destination & 0x80) != 0)
                                        setAH(ah() - source);
                                    setCF(ah() ==
                                        ((al() & 0x80) == 0 ? 0 : 0xff));
                                }
                            }
                            else {
                                setDX(data >> 16);
                                if (modRMReg() == 4) {
                                    data |= dx();
                                    setCF(dx() != 0);
                                }
                                else {
                                    if ((source & 0x8000) != 0)
                                        setDX(dx() - destination);
                                    if ((destination & 0x8000) != 0)
                                        setDX(dx() - source);
                                    data |= dx();
                                    setCF(dx() ==
                                        ((ax() & 0x8000) == 0 ? 0 : 0xffff));
                                }
                            }
                            setZF();
                            setOF(cf());
                            break;
                        case 6: case 7:  // DIV rmv, IDIV rmv
                            source = data;
                            if (source == 0)
                                divideOverflow();
                            if (!wordSize) {
                                destination = ax();
                                if (modRMReg() == 6) {
                                    div();
                                    if (data > 0xff)
                                        divideOverflow();
                                }
                                else {
                                    destination = ax();
                                    if ((destination & 0x8000) != 0)
                                        destination |= 0xffff0000;
                                    source = signExtend(source);
                                    div();
                                    if (data > 0x7f && data < 0xffffff80)
                                        divideOverflow();
                                }
                                setAH((Byte)remain);
                                setAL(data);
                            }
                            else {
                                destination = (dx() << 16) + ax();
                                div();
                                if (modRMReg() == 6) {
                                    if (data > 0xffff)
                                        divideOverflow();
                                }
                                else {
                                    if (data > 0x7fff && data < 0xffff8000)
                                        divideOverflow();
                                }
                                setDX(remain);
                                setAX(data);
                            }
                            break;
                    }
                    break;
                case 0xf8: case 0xf9:  // STC/CLC
                    setCF(wordSize);
                    break;
                case 0xfa: case 0xfb:  // STI/CLI
                    setIF(wordSize);
                    break;
                case 0xfc: case 0xfd:  // STD/CLD
                    setDF(wordSize);
                    break;
                case 0xfe: case 0xff:  // misc
                    ea();
                    if ((!wordSize && modRMReg() >= 2 && modRMReg() <= 6) ||
                        modRMReg() == 7) {
                        fprintf(stderr, "Invalid instruction %02x %02x", opcode,
                            modRM);
                        runtimeError("");
                    }
                    switch (modRMReg()) {
                        case 0: case 1:  // incdec rmv
                            destination = readEA2();
                            finishWriteEA(incdec(modRMReg() != 0));
                            break;
                        case 2:  // CALL rmv
                            data = readEA2();
                            call(data);
                            break;
                        case 3:  // CALL mp
                            farLoad();
                            farCall();
                            break;
                        case 4:  // JMP rmw
                            ip = readEA2();
                            break;
                        case 5:  // JMP mp
                            farLoad();
                            farJump();
                            break;
                        case 6:  // PUSH rmw
                            push(readEA2());
                            break;
                    }
                    break;
                default:
                    fprintf(stderr, "Invalid opcode %02x", opcode);
                    runtimeError("");
                    break;

            }
        }
        //runtimeError("Timed out");
    }
    [[noreturn]] void runtimeError(String message)
    {
        errorConsole.write(message + "\nCS:IP = " +
            Address(cs(), ip).toString() + "\n");
        exit(1);
    }
private:
    Word cs() { return registers[9]; }
    void error(String operation)
    {
        errorConsole.write("Error " + operation + " file " + filename + ": " +
            strerror(errno) + "\n");
        exit(1);
    }
    int getDescriptor()
    {
        for (int i = 0; i < fileDescriptorCount; ++i)
            if (fileDescriptors[i] == -1)
                return i;
        int newCount = fileDescriptorCount << 1;
        int* newDescriptors = (int*)alloc(newCount * sizeof(int));
        for (int i = 0; i < fileDescriptorCount; ++i)
            newDescriptors[i] = fileDescriptors[i];
        free(fileDescriptors);
        int oldCount = fileDescriptorCount;
        fileDescriptorCount = newCount;
        fileDescriptors = newDescriptors;
        return oldCount;
    }
    void divideOverflow() { runtimeError("Divide overflow"); }
    Word segmentAddress(int seg)
    {
        ++ios;
        if (ios == 0)
            runtimeError("Cycle counter overflowed.");
        if (seg == -1) {
            seg = segment;
            if (segmentOverride != -1)
                seg = segmentOverride;
        }
        return registers[8 + seg];
    }
    char* initString(Word offset, int seg, bool write, int buffer,
        int bytes = 0x10000)
    {
        for (int i = 0; i < bytes; ++i) {
            char p;
            if (write) {
                p = pathBuffers[buffer][i];
                _memory->writeByte(Address(segmentAddress(seg), offset), p,
                    false);
            }
            else {
                p = _memory->getByte(Address(segmentAddress(seg), offset + i));
                pathBuffers[buffer][i] = p;
            }
            if (p == 0 && bytes == 0x10000)
                break;
        }
        if (!write)
            pathBuffers[buffer][0xffff] = 0;
        return pathBuffers[buffer];
    }
    Byte readByte(Word offset, int seg = -1, bool fetch = false,
        bool firstByte = false, bool preStart = false)
    {
        return _memory->getByte(Address(segmentAddress(seg), offset), fetch,
            firstByte, preStart);
    }
    Word readWord(Word offset, int seg = -1, bool preStart = false)
    {
        Word r = readByte(offset, seg, false, false, preStart);
        return r + (readByte(offset + 1, seg, false, false, preStart) << 8);
    }
    Word read(Word offset, int seg = -1)
    {
        return wordSize ? readWord(offset, seg) : readByte(offset, seg);
    }
    void writeByte(Byte value, Word offset, int seg = -1,
        bool preStart = false)
    {
        Word segment = segmentAddress(seg);
        if ((((segment << 4) + offset) & 0xfffff) == 0x70 && value == 0)
            console.write("ping");
        _memory->writeByte(Address(segment, offset), value,
            preStart);

        // TODO: put back
        //_memory->writeByte(Address(segmentAddress(seg), offset), value,
        //    preStart);
    }
    void writeWord(Word value, Word offset, int seg = -1,
        bool preStart = false)
    {
        writeByte((Byte)value, offset, seg, preStart);
        writeByte((Byte)(value >> 8), offset + 1, seg, preStart);
    }
    void write(Word value, Word offset, int seg = -1)
    {
        if (wordSize)
            writeWord(value, offset, seg);
        else
            writeByte((Byte)value, offset, seg);
    }
    Byte fetchByte(bool firstByte = false)
    {
        Byte b = readByte(ip, 1, true, firstByte);
        ++ip;
        return b;
    }
    Word fetchWord() { Word w = fetchByte(); w += fetchByte() << 8; return w; }
    Word fetch(bool wordSize)
    {
        if (wordSize)
            return fetchWord();
        return fetchByte();
    }
    Word signExtend(Byte data) { return data + (data < 0x80 ? 0 : 0xff00); }
    int modRMReg() { return (modRM >> 3) & 7; }
    void div()
    {
        bool negative = false;
        bool dividendNegative = false;
        if (modRMReg() == 7) {
            if ((destination & 0x80000000) != 0) {
                destination = (unsigned)-(signed)destination;
                negative = !negative;
                dividendNegative = true;
            }
            if ((source & 0x8000) != 0) {
                source = (unsigned)-(signed)source & 0xffff;
                negative = !negative;
            }
        }
        data = destination / source;
        DWord product = data * source;
        // ISO C++ 2003 does not specify a rounding mode, but the x86 always
        // rounds towards zero.
        if (product > destination) {
            --data;
            product -= source;
        }
        remain = destination - product;
        if (negative)
            data = (unsigned)-(signed)data;
        if (dividendNegative)
            remain = (unsigned)-(signed)remain;
    }
    void jumpShort(Byte data, bool jump) { if (jump) ip += signExtend(data); }
    void setCF(bool cf) { flags = (flags & ~1) | (cf ? 1 : 0); }
    void setAF(bool af) { flags = (flags & ~0x10) | (af ? 0x10 : 0); }
    void clearCA() { setCF(false); setAF(false); }
    void setOF(bool of) { flags = (flags & ~0x800) | (of ? 0x800 : 0); }
    void clearCAO() { clearCA(); setOF(false); }
    void setPF()
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
        flags = (flags & ~4) | table[data & 0xff];
    }
    void setZF(bool zf) { flags = (flags & ~0x40) | (zf ? 0x40 : 0); }
    void setZF() { setZF((data & (!wordSize ? 0xff : 0xffff)) == 0); }
    void setSF()
    {
        flags = (flags & ~0x80) |
            ((data & (!wordSize ? 0x80 : 0x8000)) != 0 ? 0x80 : 0);
    }
    void setPZS() { setPF(); setZF(); setSF(); }
    void bitwise(Word value) { data = value; clearCAO(); setPZS(); }
    void test(Word d, Word s)
    {
        destination = d;
        source = s;
        bitwise(destination & source);
    }
    bool cf() { return (flags & 1) != 0; }
    bool pf() { return (flags & 4) != 0; }
    bool af() { return (flags & 0x10) != 0; }
    bool zf() { return (flags & 0x40) != 0; }
    bool sf() { return (flags & 0x80) != 0; }
    bool intf() { return (flags & 0x200) != 0; }
    void setIF(bool intf) { flags = (flags & ~0x200) | (intf ? 0x200 : 0); }
    void setDF(bool df) { flags = (flags & ~0x400) | (df ? 0x400 : 0); }
    bool df() { return (flags & 0x400) != 0; }
    bool of() { return (flags & 0x800) != 0; }
    Word rw() { return registers[opcode & 7]; }
    Word ax() { return registers[0]; }
    Word cx() { return registers[1]; }
    Word dx() { return registers[2]; }
    Word bx() { return registers[3]; }
    Word sp() { return registers[4]; }
    Word bp() { return registers[5]; }
    Word si() { return registers[6]; }
    Word di() { return registers[7]; }
    Word es() { return registers[8]; }
    Word ss() { return registers[10]; }
    Word ds() { return registers[11]; }
    Byte al() { return *byteRegisters[0]; }
    Byte cl() { return *byteRegisters[1]; }
    Byte dl() { return *byteRegisters[2]; }
    Byte bl() { return *byteRegisters[3]; }
    Byte ah() { return *byteRegisters[4]; }
    Byte ch() { return *byteRegisters[5]; }
    Byte dh() { return *byteRegisters[6]; }
    Byte bh() { return *byteRegisters[7]; }
    void setRW(Word value) { registers[opcode & 7] = value; }
    void setAX(Word value) { registers[0] = value; }
    void setCX(Word value) { registers[1] = value; }
    void setDX(Word value) { registers[2] = value; }
    void setBX(Word value) { registers[3] = value; }
    void setSP(Word value) { registers[4] = value; }
    void setSI(Word value) { registers[6] = value; }
    void setDI(Word value) { registers[7] = value; }
    void setAL(Byte value) { *byteRegisters[0] = value; }
    void setCL(Byte value) { *byteRegisters[1] = value; }
    void setDL(Byte value) { *byteRegisters[2] = value; }
    void setBL(Byte value) { *byteRegisters[3] = value; }
    void setAH(Byte value) { *byteRegisters[4] = value; }
    void setCH(Byte value) { *byteRegisters[5] = value; }
    void setDH(Byte value) { *byteRegisters[6] = value; }
    void setBH(Byte value) { *byteRegisters[7] = value; }
    void setRB(Byte value) { *byteRegisters[opcode & 7] = value; }
    void setES(Word value) { registers[8] = value; }
    void setCS(Word value) { registers[9] = value; }
    void setSS(Word value) { registers[10] = value; }
    void setDS(Word value) { registers[11] = value; }
    int stringIncrement()
    {
        int r = (wordSize ? 2 : 1);
        return !df() ? r : -r;
    }
    Word lodS()
    {
        address = si();
        setSI(si() + stringIncrement());
        segment = 3;
        return read(address);
    }
    void doRep(bool compare)
    {
        if (rep == 1 && !compare)
            runtimeError("REPNE prefix with non-compare string instruction");
        if (rep == 0 || cx() == 0)
            return;
        setCX(cx() - 1);
        repeating = cx() != 0 && (!compare || zf() != (rep == 1));
    }
    Word lodDIS()
    {
        address = di();
        setDI(di() + stringIncrement());
        return read(address, 0);
    }
    void stoS(Word data)
    {
        address = di();
        setDI(di() + stringIncrement());
        write(data, address, 0);
    }
    void push(Word value)
    {
        setSP(sp() - 2);
        if (sp() < stackLow)
            runtimeError("Stack overflow");
        writeWord(value, sp(), 2);
    }
    Word pop() { Word r = readWord(sp(), 2); setSP(sp() + 2); return r; }
    void setCA() { setCF(true); setAF(true); }
    void doAF() { setAF(((data ^ source ^ destination) & 0x10) != 0); }
    void doCF() { setCF((data & (!wordSize ? 0x100 : 0x10000)) != 0); }
    void setCAPZS() { setPZS(); doAF(); doCF(); }
    void setOFAdd()
    {
        Word t = (data ^ source) & (data ^ destination);
        setOF((t & (!wordSize ? 0x80 : 0x8000)) != 0);
    }
    void add() { data = destination + source; setCAPZS(); setOFAdd(); }
    void setOFSub()
    {
        Word t = (destination ^ source) & (data ^ destination);
        setOF((t & (!wordSize ? 0x80 : 0x8000)) != 0);
    }
    void sub() { data = destination - source; setCAPZS(); setOFSub(); }
    void setOFRotate()
    {
        setOF(((data ^ destination) & (!wordSize ? 0x80 : 0x8000)) != 0);
    }
    void doALUOperation()
    {
        switch (aluOperation) {
            case 0: add(); break;
            case 1: bitwise(destination | source); break;
            case 2: source += cf() ? 1 : 0; add(); break;
            case 3: source += cf() ? 1 : 0; sub(); break;
            case 4: test(destination, source); break;
            case 5:
            case 7: sub(); break;
            case 6: bitwise(destination ^ source); break;
        }
    }
    Word* modRMRW() { return &registers[modRMReg()]; }
    Byte* modRMRB() { return byteRegisters[modRMReg()]; }
    Word getReg()
    {
        if (!wordSize)
            return *modRMRB();
        return *modRMRW();
    }
    Word getAccum() { return !wordSize ? al() : ax(); }
    void setAccum() { if (!wordSize) setAL(data); else setAX(data); }
    void setReg(Word value)
    {
        if (!wordSize)
            *modRMRB() = (Byte)value;
        else
            *modRMRW() = value;
    }
    Word ea()
    {
        modRM = fetchByte();
        useMemory = true;
        switch (modRM & 7) {
            case 0: segment = 3; address = bx() + si(); break;
            case 1: segment = 3; address = bx() + di(); break;
            case 2: segment = 2; address = bp() + si(); break;
            case 3: segment = 2; address = bp() + di(); break;
            case 4: segment = 3; address = si(); break;
            case 5: segment = 3; address = di(); break;
            case 6: segment = 2; address = bp();        break;
            case 7: segment = 3; address = bx();        break;
        }
        switch (modRM & 0xc0) {
            case 0x00:
                if ((modRM & 0xc7) == 6) {
                    segment = 3;
                    address = fetchWord();
                }
                break;
            case 0x40: address += signExtend(fetchByte()); break;
            case 0x80: address += fetchWord(); break;
            case 0xc0:
                useMemory = false;
                address = modRM & 7;
        }
        return address;
    }
    Word readEA2()
    {
        if (!useMemory) {
            if (wordSize)
                return registers[address];
            return *byteRegisters[address];
        }
        return read(address);
    }
    Word readEA() { address = ea(); return readEA2(); }
    void finishWriteEA(Word data)
    {
        if (!useMemory) {
            if (wordSize)
                registers[address] = data;
            else
                *byteRegisters[address] = (Byte)data;
        }
        else
            write(data, address);
    }
    void writeEA(Word data) { ea(); finishWriteEA(data); }
    void farLoad()
    {
        if (!useMemory)
            runtimeError("This instruction needs a memory address");
        savedIP = readWord(address);
        savedCS = readWord(address + 2);
    }
    void farJump() { setCS(savedCS); ip = savedIP; }
    HashTable<DWord, DWord> functions;
    AppendableArray<DWord> callStack;
    HashTable<DWord, String> functionNames;
    bool inREArea(DWord physicalAddress)
    {
        for (int i = 0; i < regions; ++i) {
            if (physicalAddress >= reRegions[i * 2] &&
                physicalAddress < reRegions[i * 2 + 1])
                return true;
        }
        return false;
    }

    //class Decompiler
    //{
    //public:
    //    void decompileFunction(DWord physicalAddress)
    //    {
    //        do {
    //            Byte opcode = _ram[physicalAddress];
    //            switch (opcode) {
    //                case 0x50: case 0x51: case 0x52: case 0x53:
    //                case 0x54: case 0x55: case 0x56: case 0x57:  // PUSH rw
    //                    push(rw(opcode & 0xf));
    //                    break;
    //                default:
    //                    fprintf(stderr, "Unknown opcode %02x at %05x\n", opcode,
    //                        physicalAddress);
    //                    runtimeError("");
    //                    break;
    //            }
    //        } while (true);
    //    }
    //
    //private:
    //    Expression rw(int n)
    //    {
    //
    //    }
    //    void push(Expression e)
    //    {
    //    }
    //
    //    Byte* _ram;
    //};

    void recordCall(Word newCS, Word newIP)
    {
        DWord segOff = (newCS << 16) + newIP;
        DWord physicalAddress = (newCS << 4) + newIP;
        if (!inREArea(physicalAddress))
            return;
        callStack.append((cs() << 16) + ip);
        callStack.append(physicalAddress);
        if (functions.hasKey(physicalAddress)) {
            DWord oldSegOff = functions[physicalAddress];
            if (oldSegOff != segOff) {
                fprintf(stderr, "Function at %05x called via both %04x:%04x "
                    "and %04x:%04x", physicalAddress, oldSegOff >> 16,
                    oldSegOff & 0xffff, segOff >> 16, segOff & 0xffff);
                runtimeError("");
            }
            return;
        }
        functions.add(physicalAddress, segOff);
        for (int i = 0; i < callStack.count(); i += 2) {
            DWord caller = callStack[i];
            DWord callee = callStack[i + 1];
            String name;
            if (functionNames.hasKey(callee))
                name = functionNames[callee];
            else
                name = "sub_" + hex(callee, 5, false);
            NullTerminatedString n(name);
            printf(":%04x %s", caller & 0xffff, (const char*)n);
        }
        printf("\n");
        //decompileFunction(physicalAddress);
    }
    void recordReturn()
    {
        if (inREArea((cs() << 4) + ip - 1))
            callStack.unappend(2);
    }
    void farCall()
    {
        recordCall(savedCS, savedIP);
        push(cs());
        push(ip);
        farJump();
    }
    Word incdec(bool decrement)
    {
        source = 1;
        if (!decrement) {
            data = destination + source;
            setOFAdd();
        }
        else {
            data = destination - source;
            setOFSub();
        }
        doAF();
        setPZS();
        return data;
    }
    void call(Word address) { recordCall(cs(), address); push(ip); ip = address; }
    char* dsdx(bool write = false, int bytes = 0x10000)
    {
        return initString(dx(), 3, write, 0, bytes);
    }
    int dosError(int e)
    {
        if (e == ENOENT)
            return 2;
        fprintf(stderr, "%s\n", strerror(e));
        runtimeError("");
        return 0;
    }
    int getFileDescriptor()
    {
        if (bx() >= fileDescriptorCount)
            return -1;
        return fileDescriptors[bx()];
    }

    void outportb(Word port, Word value)
    {
        switch (port) {
            case 0x42:
            case 0x61:
                break;
            default:
                fprintf(stderr, "Unknown output port %04x\n", port);
                runtimeError("");
        }
    }

    Byte inportb(Word port)
    {
        Byte cgaStatus = 0;
        switch (port) {
            case 0x61:
                return 0;
            case 0x3da:
                cgaStatus ^= 8;
                return cgaStatus;
            default:
                fprintf(stderr, "Unknown input port %04x\n", port);
                runtimeError("");
        }
    }

    Word registers[12];
    Byte* byteRegisters[8];
    Word ip = 0x100;
    char* pathBuffers[2];
    int* fileDescriptors;
    int fileDescriptorCount = 6;
    Word loadSegment = 0x0cbe;
    int regions;
    DWord* reRegions;
    bool useMemory;
    Word address;
    Word flags = 2;
    Byte modRM;
    bool wordSize;
    DWord data;
    DWord destination;
    DWord source;
    int segment = 3;
    int rep = 0;
    bool repeating = false;
    Word savedIP;
    Word savedCS;
    int segmentOverride = -1;
    Word remain;
    Byte opcode;
    int aluOperation;
    const char* filename;
    int length;
    int ios;
    bool running = false;
    int stackLow;
    Memory* _memory;
};

DWORD cgaColours[4] = {0, 0x55ffff, 0xff55ff, 0xffffff};

class CGABitmapWindow : public BitmapWindow
{
public:
    void update()
    {
        if (!_bitmap.valid())
            _bitmap = Bitmap<DWORD>(Vector(320, 200));
        int address = 0;
        Byte* outputRow = _bitmap.data();
        for (int y = 0; y < 200; ++y) {
            DWORD* output = reinterpret_cast<DWORD*>(outputRow);
            for (int x = 0; x < 80; ++x) {
                Byte b = _memory->getByte(Address(0xb800, address));
                for (int xx = 6; xx >= 0; xx -= 2) {
                    *output = cgaColours[(b >> xx) & 3];
                    ++output;
                }
                ++address;
            }
            if ((y & 1) == 0)
                address += 0x2000 - 80;
            else
                address -= 0x2000;
            outputRow += _bitmap.stride();
        }
        _bitmap = setNextBitmap(_bitmap);
    }
    void setMemory(Memory* memory) { _memory = memory; }
private:
    Bitmap<DWORD> _bitmap;
    Memory* _memory;
};

class CGAWindow : public RootWindow
{
public:
    CGAWindow()
    {
        add(&_bitmap);
    }
    void create()
    {
        setText("CGA");
        setInnerSize(Vector(320, 200));
        _bitmap.setInnerSize(Vector(320, 200));
        _bitmap.setTopLeft(Vector(0, 0));
        RootWindow::create();
    }
    void update()
    {
        _bitmap.update();
    }
    void setMemory(Memory* memory) { _bitmap.setMemory(memory); }
private:
    CGABitmapWindow _bitmap;
};

class Program : public WindowProgram<CGAWindow>
{
public:
    void run()
    {
        createWindow();
        Memory memory;
        _window.setMemory(&memory);
        Emulator emulator;
        emulator.setMemory(&memory);
        emulator.run(_arguments, this);
        StaticDisassembler disassembler(&memory);
        disassembler.run();
    }
    void update()
    {
        _window.update();
        pumpMessages();
    }
};
