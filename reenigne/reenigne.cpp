#include "alfe/main.h"
#include "alfe/config_file.h"
//#include "alfe/statement.h"

class StateVectorAllocator
{
public:
	StateVectorAllocator() : _nextAddress(0) { }
	int allocate(int bytes)
	{
		int r = _nextAddress;
		_nextAddress += bytes;
		return r;
	}
private:
	int _nextAddress;
};

class StateVector
{
public:
    StateVector() { }
	StateVector(StateVectorAllocator allocator)
	{
		_data.allocate(allocator.allocate(0));
	}
	StateVector(const StateVector& other) { _data = other._data.copy();	}
	Byte* data(int address) { return &_data[address]; }
private:
	Array<Byte> _data;
};

class ByteSource
{
public:
    ByteSource(Byte* ram, Word ip) : _ram(ram), _ip(ip) { }
    Byte get()
    {
        Byte b = _ram[_ip];
        ++_ip;
        return b;
    }
private:
    Byte* _ram;
    Word _ip;
};

class InstructionPattern;

class Instruction
{
public:
    Instruction(InstructionPattern* pattern, Array<int> parameters)
      : _pattern(pattern), _parameters(parameters)
    {

    }
    String disassemble()
    {

    }
    //StatementSequence decompile()
    //{

    //}

private:
    InstructionPattern* _pattern;
    Array<int> _parameters;
};

class InterpretationContext
{
public:
    void addType(Type type, TycoIdentifier identifier = TycoIdentifier())
    {
        if (!identifier.valid())
            identifier = TycoIdentifier(type.toString());
        _types.add(identifier, type);
    }
private:
    HashTable<TycoIdentifier, Type> _types;
    HashTable<Identifier, Value> _values;
};

class InstructionPattern
{
public:
    InstructionPattern() { }
    InstructionPattern(InterpretationContext context, String parameters,
        String assembly, String binary, String effect)
    {
        CharacterSource s(parameters);
        TycoSpecifier tycoSpecifier = TycoSpecifier::parse(&s);
        if (!tycoSpecifier.valid())
            throw Exception("Expected TycoSpecifier");

        Identifier objectIdentifier = Identifier::parse(&s);
        if (objectIdentifier.isOperator()) {
            objectIdentifier.span().throwError("Cannot create an object "
                "with operator name");
        }
        String objectName = objectIdentifier.name();
        if (has(objectIdentifier)) {
            objectIdentifier.span().throwError(objectName +
                " already exists");
        }

    }
    virtual Instruction decode(Byte* bytes, int count)
    {
        return Instruction(this, Array<int>());
    }
private:
    struct FieldData
    {
        int bitOffset;
        int bitCount;
        
        UInt32 decodeField(Byte* bytes)
        {
            bytes += bitOffset >> 3;
            int firstBit = bitOffset & 7;
            UInt32 r = 0;
            int b = bitCount + firstBit;
            int s = 0;
            while (b > 0) {
                r += (*bytes) << s;
                ++bytes;
                s += 8;
                b -= 8;
            }
            return (r >> firstBit) & ((1 << bitCount) - 1);
        }
        void encodeField(UInt32 value, Byte* bytes)
        {
            bytes += bitOffset >> 3;
            int firstBit = bitOffset & 7;
            assert(value < (1U << bitCount));
            value <<= firstBit;
            UInt32 mask = ((1 << bitCount) - 1) << firstBit;
            while (mask != 0) {
                *bytes = ((*bytes) & ~mask) | (value & mask);
                ++bytes;
                mask >>= 8;
                value >>= 8;
            }
        }
    };
    Array<FieldData> _fieldData;
    UInt32 _opcode;
    UInt32 _mask;
};

class InvalidInstructionPattern : public InstructionPattern
{
public:
    Instruction decode(Byte* bytes, int count)
    {
        String message = "Unknown opcode ";
        for (int i = 0; i < count; ++i)
            message += hex(bytes[i], 2);
        throw Exception(message);
    }
};

class InstructionDatabase
{
public:
    InstructionDatabase()
    {
        _patterns.append(new InvalidInstructionPattern());
        _decoderTree.allocate(0x100);
        for (int i = 0; i < 0x100; ++i)
            _decoderTree[i] = -1;
    }
    void add(InstructionPattern* pattern)
    {
        
    }
    ~InstructionDatabase()
    {
        for (auto p : _patterns)
            if (p != 0)
                delete p;
    }
    Instruction decode(ByteSource s)
    {
        AppendableArray<Byte> bytes;
        int o = 0;
        do {
            int b = s.get();
            bytes.append(b);
            o = _decoderTree[b + o];
            if (o < 0)
                return _patterns[-1-o]->decode(&bytes[0], bytes.count());
        } while (true);
    }
private:
    Array<int> _decoderTree;
    AppendableArray<InstructionPattern*> _patterns;
};

class Decompiler
{
public:
    Decompiler(ConfigFile* configFile) : _configFile(configFile)
    {
        _context.addType(ByteType());
        _context.addType(WordType());
        _instructionDatabase.add(new InstructionPattern(
            "GeneralWordRegister* destination, Word source",
            "mov ${*destination}, $source",
            "byte(0xb0 + $destination).word(source)",
            "*destination = source;"
        ));

        StateVectorAllocator allocator;
        _registers = allocator.allocate(8*2 + 4*2 + 4);
        _ram = allocator.allocate(640*1024);
        _v = StateVector(allocator);
    }
    Byte* ram() { return _v.data(_ram); }
    void setAX(Word v) { registers()[0] = v; }
    void setCX(Word v) { registers()[1] = v; }
    void setDX(Word v) { registers()[2] = v; }
    void setBX(Word v) { registers()[3] = v; }
    void setSP(Word v) { registers()[4] = v; }
    void setBP(Word v) { registers()[5] = v; }
    void setSI(Word v) { registers()[6] = v; }
    void setDI(Word v) { registers()[7] = v; }
    void setES(Word v) { registers()[8] = v; }
    void setCS(Word v) { registers()[9] = v; }
    void setSS(Word v) { registers()[10] = v; }
    void setDS(Word v) { registers()[11] = v; }
    void setIP(Word v) { registers()[12] = v; }
    void setFlags(Word v) { registers()[13] = v; }
    Word cs() { return registers()[9]; }
    Word ip() { return registers()[12]; }

    void dynamic()
    {
        do {
            ByteSource s(ram() + (cs() << 4), ip());
            Instruction i = _instructionDatabase.decode(s);

        } while (true);
    }
private:
    Word* registers() { return reinterpret_cast<Word*>(_v.data(_registers)); }

    StateVector _v;
    int _registers;
    int _ram;
    ConfigFile* _configFile;
    InstructionDatabase _instructionDatabase;
    InterpretationContext _context;
};

class DOSLoader
{
public:
    DOSLoader(ConfigFile* configFile) : _configFile(configFile) { }
    void load(File file, Decompiler* decompiler)
    {
        int loadAddress = _configFile->get<int>("loadAddress");
        int imageSegment = loadAddress >> 4;

        _ram = decompiler->ram();

        file.readIntoArray(&_program);
        int length = _program.count();
        Word cs;
        Word ds;
        if (length >= 2 && readWord(0) == 0x5a4d) {  // .exe file?
            if (length < 0x21) {
                throw Exception(file.path() +
                    " is too short to be an .exe file\n");
            }
            Word bytesInLastBlock = readWord(2);
            int exeLength = ((readWord(4) - (bytesInLastBlock == 0 ? 0 : 1))
                << 9) + bytesInLastBlock;
            int headerParagraphs = readWord(8);
            int headerLength = headerParagraphs << 4;
            if (exeLength > length || headerLength > length ||
                headerLength > exeLength)
                throw Exception(file.path() + " is corrupt\n");
            int relocationCount = readWord(6);
            for (int i = 0; i < exeLength - headerLength; ++i)
                _ram[i + loadAddress] = _program[i + headerLength];

            int relocationData = readWord(0x18);
            for (int i = 0; i < relocationCount; ++i) {
                Word offset = readWord(relocationData);
                Word segment = readWord(relocationData + 2) + imageSegment;
                writeWord(offset + (segment << 4),
                    readWord(offset + headerLength) + imageSegment);
                relocationData += 4;
            }
            decompiler->setSS(readWord(0xe) + imageSegment);
            decompiler->setSP(readWord(0x10));
            decompiler->setIP(readWord(0x14));
            cs = readWord(0x16) + imageSegment;
            ds = cs - 0x10;
        }
        else {
            if (length > 0xff00) {
                throw Exception(file.path() +
                    " is too long to be a .com file\n");
            }
            for (int i = 0; i < length; ++i)
                _ram[i + loadAddress + 0x100] = _program[i];
            decompiler->setSS(imageSegment);
            decompiler->setSP(0xFFFE);
            decompiler->setIP(0x100);
            cs = imageSegment;
            ds = cs;
        }
        decompiler->setAX(0x0000);
        decompiler->setCX(0x00FF);
        decompiler->setDX(cs);
        decompiler->setBX(0x0000);
        decompiler->setBP(0x091C);
        decompiler->setSI(0x0100);
        decompiler->setDI(0xFFFE);
        decompiler->setES(cs);
        decompiler->setCS(cs);
        decompiler->setDS(ds);
        decompiler->setFlags(2);
    }
private:
    ConfigFile* _configFile;
    Array<Byte> _program;
    Byte* _ram;

    Byte readByte(int offset) { return _program[offset]; }
    Word readWord(int offset)
    {
        Word r = readByte(offset);
        return r + (readByte(offset + 1) << 8);
    }
    void writeWord(int offset, Word word)
    {
        _ram[offset] = static_cast<Byte>(word);
        _ram[offset + 1] = word >> 8;
    }
};

class Program : public ProgramBase
{
public:
    void run()
    {
        ConfigFile configFile;
        configFile.addOption("inputProgram", StringType());
        configFile.addDefaultOption("loadAddress", 0x10000);
        configFile.addDefaultOption("targetArchitecture",
            String("x86.Generic8086"));

        String configPath = "default.config";
        if (_arguments.count() >= 2)
            configPath = _arguments[1];
        File configFilePath(configPath, true);
        configFile.load(configFilePath);

        Decompiler decompiler(&configFile);

        String inputProgram = configFile.get<String>("inputProgram");
        File inputProgramFile(inputProgram, configFilePath.parent());

        DOSLoader loader(&configFile);
        loader.load(inputProgramFile, &decompiler);

        decompiler.dynamic();
    }
private:


};