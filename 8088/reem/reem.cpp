#include "alfe/main.h"
#include "alfe/config_file.h"
#include "alfe/user.h"
#include "alfe/bitmap.h"
#include "alfe/statement.h"

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

Word registers[12];
Byte* byteRegisters[8];
Word ip = 0x100;
Byte* ram;
Byte* initialized;
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

Word cs() { return registers[9]; }
void error(const char* operation)
{
    fprintf(stderr, "Error %s file %s: %s\n", operation, filename,
        strerror(errno));
    exit(1);
}
void runtimeError(const char* message)
{
    fprintf(stderr, "%s\nCS:IP = %04x:%04x\n", message, cs(), ip);
    exit(1);
}
void* alloc(size_t bytes)
{
    void* r = malloc(bytes);
    if (r == 0) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    return r;
}
int getDescriptor()
{
    for (int i = 0; i < fileDescriptorCount; ++i)
        if (fileDescriptors[i] == -1)
            return i;
    int newCount = fileDescriptorCount << 1;
    int* newDescriptors = (int*)alloc(newCount*sizeof(int));
    for (int i = 0; i < fileDescriptorCount; ++i)
        newDescriptors[i] = fileDescriptors[i];
    free(fileDescriptors);
    int oldCount = fileDescriptorCount;
    fileDescriptorCount = newCount;
    fileDescriptors = newDescriptors;
    return oldCount;
}
void divideOverflow() { runtimeError("Divide overflow"); }
DWord physicalAddress(Word offset, int seg, bool write)
{
    ++ios;
    if (ios == 0)
        runtimeError("Cycle counter overflowed.");
    if (seg == -1) {
        seg = segment;
        if (segmentOverride != -1)
            seg = segmentOverride;
    }
    Word segmentAddress = registers[8 + seg];
    DWord a = ((segmentAddress << 4) + offset) & 0xfffff;
    bool bad = false;
    if (write) {
        if (running) {
            if (a >= 0x500 && a < ((DWord)loadSegment << 4) - 0x100)
                 bad = true;
            if (a >= 0xa0000 && a < 0xb8000)
                bad = true;
            if (a >= 0xc0000)
                bad = true;
        }
        initialized[a >> 3] |= 1 << (a & 7);
    }
    if ((initialized[a >> 3] & (1 << (a & 7))) == 0 || bad) {
        fprintf(stderr, "Accessing invalid address %04x:%04x.\n",
            segmentAddress, offset);
        runtimeError("");
    }
    return a;
}
char* initString(Word offset, int seg, bool write, int buffer,
    int bytes = 0x10000)
{
    for (int i = 0; i < bytes; ++i) {
        char p;
        if (write) {
            p = pathBuffers[buffer][i];
            ram[physicalAddress(offset + i, seg, true)] = p;
        }
        else {
            p = ram[physicalAddress(offset + i, seg, false)];
            pathBuffers[buffer][i] = p;
        }
        if (p == 0 && bytes == 0x10000)
            break;
    }
    if (!write)
        pathBuffers[buffer][0xffff] = 0;
    return pathBuffers[buffer];
}
Byte readByte(Word offset, int seg = -1)
{
    return ram[physicalAddress(offset, seg, false)];
}
Word readWord(Word offset, int seg = -1)
{
    Word r = readByte(offset, seg);
    return r + (readByte(offset + 1, seg) << 8);
}
Word read(Word offset, int seg = -1)
{
    return wordSize ? readWord(offset, seg) : readByte(offset, seg);
}
void writeByte(Byte value, Word offset, int seg = -1)
{
    ram[physicalAddress(offset, seg, true)] = value;
}
void writeWord(Word value, Word offset, int seg = -1)
{
    writeByte((Byte)value, offset, seg);
    writeByte((Byte)(value >> 8), offset + 1, seg);
}
void write(Word value, Word offset, int seg = -1)
{
    if (wordSize)
        writeWord(value, offset, seg);
    else
        writeByte((Byte)value, offset, seg);
}
Byte fetchByte() { Byte b = readByte(ip, 1); ++ip; return b; }
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
        4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4};
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
void setAccum() { if (!wordSize) setAL(data); else setAX(data);  }
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
        case 4: segment = 3; address =        si(); break;
        case 5: segment = 3; address =        di(); break;
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
        if (physicalAddress >= reRegions[i*2] &&
            physicalAddress < reRegions[i*2 + 1])
            return true;
    }
    return false;
}

class Decompiler
{
public:
    void decompileFunction(DWord physicalAddress)
    {
        do {
            Byte opcode = _ram[physicalAddress];
            switch (opcode) {
                case 0x50: case 0x51: case 0x52: case 0x53:
                case 0x54: case 0x55: case 0x56: case 0x57:  // PUSH rw
                    push(rw(opcode & 0xf));
                    break;
                default:
                    fprintf(stderr, "Unknown opcode %02x at %05x\n", opcode,
                        physicalAddress);
                    runtimeError("");
                    break;
            }
        } while (true);
    }

private:
    Expression rw(int n)
    {

    }
    void push(Expression e)
    {
    }

    Byte* _ram;
};

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
            fprintf(stderr, "Function at %05x called via both %04x:%04x and "
                "%04x:%04x", physicalAddress, oldSegOff >> 16,
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
    decompileFunction(physicalAddress);
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


DWORD cgaColours[4] = {0, 0x55ffff, 0xff55ff, 0xffffff};

class CGABitmapWindow : public BitmapWindow
{
public:
    void update()
    {
        if (!_bitmap.valid())
            _bitmap = Bitmap<DWORD>(Vector(320, 200));
        int address = 0xb8000;
        Byte* outputRow = _bitmap.data();
        for (int y = 0; y < 200; ++y) {
            DWORD* output = reinterpret_cast<DWORD*>(outputRow);
            for (int x = 0; x < 80; ++x) {
                Byte b = ram[address];
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
private:
    Bitmap<DWORD> _bitmap;
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
private:
    CGABitmapWindow _bitmap;
};

void main1(Array<String> arguments, Program* program);

class Program : public WindowProgram<CGAWindow>
{
public:
    void run()
    {
        createWindow();
        main1(_arguments, this);
    }
    void update()
    {
        _window.update();
        pumpMessages();
    }
};

void main1(Array<String> arguments, Program* program)
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
    List<Value> reRegionsValue = configFile.get<List<Value>>("reRegions");
    regions = reRegionsValue.count();
    reRegions = (DWord*)alloc(regions*2*sizeof(DWord));
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
    ram = (Byte*)alloc(0x100000);
    initialized = (Byte*)alloc(0x20000);
    pathBuffers[0] = (char*)alloc(0x10000);
    pathBuffers[1] = (char*)alloc(0x10000);
    memset(ram, 0, 0x100000);
    memset(initialized, 0, 0x20000);
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
    if (fread(&ram[loadOffset], length, 1, fp) != 1)
        error("reading");
    fclose(fp);
    for (int i = 0; i < length; ++i) {
        setES(loadSegment + (i >> 4));
        physicalAddress(i & 15, 0, true);
    }
    for (int i = 0; i < 4; ++i)
        registers[8 + i] = loadSegment - 0x10;
    if (length >= 2 && readWord(0x100) == 0x5a4d) {  // .exe file?
        if (length < 0x21) {
            fprintf(stderr, "%s is too short to be an .exe file\n", filename);
            exit(1);
        }
        Word bytesInLastBlock = readWord(0x102);
        int exeLength = ((readWord(0x104) - (bytesInLastBlock == 0 ? 0 : 1))
            << 9) + bytesInLastBlock;
        int headerParagraphs = readWord(0x108);
        int headerLength = headerParagraphs << 4;
        if (exeLength > length || headerLength > length ||
            headerLength > exeLength) {
            fprintf(stderr, "%s is corrupt\n", filename);
            exit(1);
        }
        int relocationCount = readWord(0x106);
        Word imageSegment = loadSegment + headerParagraphs;
        int relocationData = readWord(0x118);
        for (int i = 0; i < relocationCount; ++i) {
            int offset = readWord(relocationData + 0x100);
            setCS(readWord(relocationData + 0x102) + imageSegment);
            writeWord(readWord(offset, 1) + imageSegment, offset, 1);
            relocationData += 4;
        }
        loadSegment = imageSegment;  // Prevent further access to header
        Word ss = readWord(0x10e) + loadSegment;  // SS
        setSS(ss);
        setSP(readWord(0x110));
        stackLow = ((exeLength - headerLength + 15) >> 4) + loadSegment;
        if (stackLow < ss)
            stackLow = 0;
        else
            stackLow = (stackLow - (int)ss) << 4;
        ip = readWord(0x114);
        setCS(readWord(0x116) + loadSegment);  // CS
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
    int interrupts[5] = {0, 4, 5, 6, 0x1c};
    for (int i = 0; i < 5; ++i) {
        writeWord(0, interrupts[i]*4);
        writeWord(0, interrupts[i]*4 + 2);
    }
    writeWord(0x600, 0x70);
    writeByte(0xcf, 0x600);
    writeWord(0x3d4, 0x463);
    int envSegment = loadSegment - 0x1c;
    setDS(envSegment);
    const char* env = "PATH=C:\\";
    int envLen = strlen(env);
    for (int i = 0; i < envLen; ++i)
        writeByte(env[i], i);
    writeByte(0, envLen);
    writeByte(0, envLen + 1);
    writeWord(1, envLen + 2);
    int i;
    for (i = 0; filename[i] != 0; ++i)
        writeByte(filename[i], i + 4 + envLen);
    if (i + 4 >= 0xc0) {
        fprintf(stderr, "Program name too long.\n");
        exit(1);
    }
    writeWord(0, i + 3 + envLen);
    setDS(loadSegment - 0x10);
    writeWord(envSegment, 0x2c);
    writeWord(0x9fff, 0x02);
    i = 0x81;
    for (int a = 2; a < arguments.count(); ++a) {
        if (a > 2) {
            writeByte(' ', i);
            ++i;
        }
        NullTerminatedString aa(arguments[a]);
        const char* arg = aa;
        bool quote = strchr(arg, ' ') != 0;
        if (quote) {
            writeByte('\"', i);
            ++i;
        }
        for (; *arg != 0; ++arg) {
            int c = *arg;
            if (c == '\"') {
                writeByte('\\', i);
                ++i;
            }
            writeByte(c, i);
            ++i;
        }
        if (quote) {
            writeByte('\"', i);
            ++i;
        }
    }
    if (i > 0xff) {
        fprintf(stderr, "Arguments too long.\n");
        exit(1);
    }
    writeByte(i - 0x81, 0x80);
    writeByte(13, i);
    // Some testcases copy uninitialized stack data, so mark as initialized
    // any locations that could possibly be stack.
    for (DWord d = (loadSegment << 4) + length;
        d < (DWord)((registers[10] << 4) + sp()); ++d) {
        setES(d >> 4);
        writeByte(0, d & 15, 0);
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
    fileDescriptors = (int*)alloc(6*sizeof(int));
    fileDescriptors[0] = STDIN_FILENO;
    fileDescriptors[1] = STDOUT_FILENO;
    fileDescriptors[2] = STDERR_FILENO;
    fileDescriptors[3] = STDOUT_FILENO;
    fileDescriptors[4] = STDOUT_FILENO;
    fileDescriptors[5] = -1;
    Byte* byteData = (Byte*)&registers[0];
    int bigEndian = (byteData[2] == 0 ? 1 : 0);
    int byteNumbers[8] = {0, 2, 4, 6, 1, 3, 5, 7};
    for (int i = 0 ; i < 8; ++i)
        byteRegisters[i] = &byteData[byteNumbers[i] ^ bigEndian];
    running = true;
    bool prefix = false;
    int lastios = 0;
    for (int i = 0; i < 1000000000; ++i) {
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
        if (!repeating) {
            if (!prefix) {
                segmentOverride = -1;
                rep = 0;
            }
            prefix = false;
            opcode = fetchByte();
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
                                    time_t t = time(0);
                                    struct tm* lt = localtime(&t);
                                    data = (lt->tm_hour*60 + lt->tm_min)*60 +
                                        lt->tm_sec;
                                    setDX(data);
                                    setCX(data >> 16);
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
                                    writeWord(dx(), al()*4, 0);
                                    writeWord(ds(), al()*4 + 2, 0);
                                    setES(t);
                                }
                                break;
                            case 0x2c:
                                {
#ifdef _WIN32
                                    SYSTEMTIME t;
                                    GetLocalTime(&t);
                                    setCH((Byte)t.wHour);
                                    setCL((Byte)t.wMinute);
                                    setDH((Byte)t.wSecond);
                                    setDL(t.wMilliseconds / 1000);
#else
                                    time_t t = time(0);
                                    struct tm* lt = localtime(&t);
                                    struct timeval tv;
                                    struct timezone tz;
                                    gettimeofday(&tv, &tz);
                                    setCH(lt->tm_hour);
                                    setCL(lt->tm_min);
                                    setDH(lt->tm_sec);
                                    setDL(tv.tv_usec / 10000);
#endif
                                }
                                break;
                            case 0x30:
                                setAX(0x1403);
                                break;
                            case 0x35:
                                setES(0);
                                setBX(readWord(al()*4, 0));
                                setES(readWord(al()*4 + 2, 0));
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
                                //printf("Opening file %s\n", dsdx());
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
                                //printf("Reading %i bytes from %i to "
                                //    "%04x:%04x\n", cx(), bx(), ds(), dx());
                                data = read(fileDescriptor, pathBuffers[0],
                                    cx());
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
                                data = write(fileDescriptor, dsdx(false, cx()),
                                    cx());
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
                                if (rename(dsdx(), initString(di(), 0, false,
                                    1)) == 0)
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
                setAL(al() + ah()*data);
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
    runtimeError("Timed out");
}