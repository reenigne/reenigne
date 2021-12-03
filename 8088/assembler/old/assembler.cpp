#include "alfe/main.h"

class Register;

class RegisterFile
{
public:
    void addRegister(Register* r)
    {
        _registers.append(r);
    }
private:
    AppendableArray<Register*> _registers;
};

class Symbol
{
public:
    Symbol() : _cookie(_nextCookie) { ++_nextCookie; }
    bool operator==(Symbol other) { return _cookie == other._cookie; }
    bool operator!=(Symbol other) { return _cookie != other._cookie; }
    UInt32 hash(int t) { return Hash(typeid(Symbol)).mixin(_cookie); }
private:
    static int _nextCookie;
    int _cookie;
};

int Symbol::_nextCookie = 0;

Symbol zero;
Symbol unknown;

class Register
{
public:
    Register(RegisterFile* file, String name, int binaryEncoding, int width)
      : _file(file), _name(name), _binaryEncoding(binaryEncoding),
        _width(width)
    {
        file->addRegister(this);
    }
    Word value() const { return _value; }
    bool isConstant() const { return _symbol == zero; }
    virtual void assign(T value, Symbol symbol)
    {
        _value = value;
        _symbol = symbol;
        for (auto& i : _partOf)
            i->partAssigned(value, symbol, this);
    }
    bool hasValue(Word v) const { return isConstant() && value() == v; }
    void clear() { assign(0, unknown); }
    void makePartOf(Register* larger)
    {
        _partOf.append(larger);
    }
    virtual void partAssigned(Word value, Symbol symbol, Register* part) { }
    int width() const { return _width; }
    int binaryEncoding() const { return _binaryEncoding; }
    String toString() const { return _name; }
protected:
    RegisterFile* _file;
    String _name;
    int _width;
    int _binaryEncoding;
    Word _value;
    Symbol _symbol;
    AppendableArray<Register*> _partOf;
};

class CompoundRegister : public Register
{
public:
    CompoundRegister(RegisterFile* file, String name, int binaryEncoding,
        Register* lowPart, Register* highPart)
      : Register(file, name, binaryEncoding,
            lowPart->_width + highPart->_width),
        _lowPart(lowPart), _highPart(highPart)
    {
        lowPart->makePartOf(this);
        highPart->makePartOf(this);
    }
    void assign(Word value, Symbol symbol)
    {
        Register::assign(value, symbol);
        int shift = 8 * lowPart->_width;
        // Assign direcly instead of going through assign to avoid sprious
        // partAssigned() calls.
        _lowPart->_value = value & ((1 << shift) - 1);
        _lowPart->_symbol = symbol;
        _highPart->_value = value >> shift;
        _highPart->_symbol = (symbol == zero ? zero : unknown);
    }
    void partAssigned(Word value, Symbol symbol, Register* part)
    {
        int shift = 8 * lowPart->_width;
        if (part == _lowPart) {
            _value = (_highPart->_value << shift) | value;
            if (symbol != _symbol) {
                if (_highPart->_symbol == zero && symbol == zero)
                    _symbol = zero;
                else
                    _symbol = unknown;
            }
        }
        else {
            _value = _lowPart->_value | (value << shift);
            if (_lowPart->_symbol == zero && symbol == zero)
                _symbol = zero;
            else
                _symbol = unknown;
        }
    }
private:
    Register<T2>* _lowPart;
    Register<T2>* _highPart;
};

class RegisterClass
{
public:
    RegisterClass() { }
    template<class... T> RegisterClass(const Register& r, T... rest)
      : RegisterClass(rest)
    {
        _registers.add(&r);
    }
    template<class... T> RegisterClass(const RegisterClass& c, T... rest)
      : RegisterClass(rest)
    {
        for (auto r : c._registers)
            _registers.add(r);
    }
    bool contains(const Register& r) { return contains(&r); }
    bool contains(const Register* r) { return _registers.has(r); }
private:
    Set<const Register*> _registers;
};

RegisterFile registerFile;
Register al(&registerFile, "al", 0, 1);
Register cl(&registerFile, "cl", 1, 1);
Register dl(&registerFile, "dl", 2, 1);
Register bl(&registerFile, "bl", 3, 1);
Register ah(&registerFile, "ah", 4, 1);
Register ch(&registerFile, "ch", 5, 1);
Register dh(&registerFile, "dh", 6, 1);
Register bh(&registerFile, "bh", 7, 1);
CompoundRegister ax(&registerFile, "ax", 0, &al, &ah);
CompoundRegister cx(&registerFile, "cx", 1, &cl, &dh);
CompoundRegister dx(&registerFile, "dx", 2, &dl, &dh);
CompoundRegister bx(&registerFile, "bx", 3, &bl, &bh);
Register sp(&registerFile, "sp", 4, 2);
Register bp(&registerFile, "bp", 5, 2);
Register si(&registerFile, "si", 6, 2);
Register di(&registerFile, "di", 7, 2);
Register es(&registerFile, "es", 0, 2);
Register cs(&registerFile, "cs", 1, 2);
Register ss(&registerFile, "ss", 2, 2);
Register ds(&registerFile, "ds", 3, 2);
Register ip(&registerFile, "ip", 0, 2);
Register flags(&registerFile, "flags", 0, 2);
RegisterClass generalWordRegisters(ax, cx, dx, bx, sp, bp, si, di);
RegisterClass generalByteRegisters(al, cl, dl, bl, ah, ch, dh, bh);
RegisterClass generalRegisters(generalWordRegisters, generalByteRegisters);
RegisterClass indexRegisters(si, di);
RegisterClass segmentRegisters(es, cs, ss, ds);

class RegisterExpression
{
public:
    RegisterExpression(const Register* r, Word offset)
      : _base(0), _index(0), _offset(offset)
    {
        addRegister(r);
    }
    RegisterExpression(const RegisterExpression& re, Word offset)
    {
        *this = re;
        _offset += offset;
    }
    RegisterExpression(const RegisterExpression& re, Register* r)
    {
        *this = re;
        addRegister(r);
    }
    RegisterExpression(const RegisterExpression& re1, RegisterExpression& re2)
    {
        *this = re1;
        if (re2._base != 0)
            addRegister(re2._base);
        if (re2._index != 0)
            addRegister(re2._index);
        _offset += re2._offset;
    }
    int length() const
    {
        if (_offset == 0 && (_base != &bp || _index != 0))
            return 1;
        if (_offset >= -128 && _offset < 128 && (_base != 0 || _index != 0))
            return 2;
        return 3;
    }
    void assemble(int reg, Byte* p) const
    {
        Byte modrm;
        if (_base == 0) {
            if (_index == 0)
                modrm = 6;
            else {
                if (_index == &si)
                    modrm = 4;
                else
                    modrm = 5;
            }
        }
        else {
            if (_base == &bx) {
                if (_index == 0)
                    modrm = 7;
                else {
                    if (_index == &si)
                        modrm = 0;
                    else
                        modrm = 1;
                }
            }
            else {
                if (_index == 0)
                    modrm = 6;
                else {
                    if (_index == &si)
                        modrm = 2;
                    else
                        modrm = 3;
                }
            }
        }
        *p = modrm + (reg << 3);
        int l = length();
        if (l > 1) {
            p[1] = offset;
            if (l > 2)
                p[2] = offset >> 8;
        }
    }
    bool defaultSS() const { return _base == &bp; }
private:
    void addRegister(Register* r)
    {
        if (r == &bx || r == &bp) {
            if (_base != 0)
                throw Exception("Too many base registers in expression.");
            _base = r;
            return;
        }
        if (indexRegisters.contains(r)) {
            if (_index != 0)
                throw Exception("Too many index registers in expression.");
            _index = r;
            return;
        }
        throw Exception("Bad register in expression");
    }
    Word _offset;
    const Register* _base;
    const Register* _index;
};

RegisterExpression operator+(const Register& r, int offset)
{
    return RegisterExpression(&r, offset);
}

RegisterExpression operator+(int offset, const Register& r)
{
    return RegisterExpression(&r, offset);
}

RegisterExpression operator-(const Register& r, int offset)
{
    return RegisterExpression(&r, -offset);
}

RegisterExpression operator+(const RegisterExpression& re, int offset)
{
    return RegisterExpression(re, offset);
}

RegisterExpression operator+(int offset, const RegisterExpression& re)
{
    return RegisterExpression(re, offset);
}

RegisterExpression operator-(const RegisterExpression& re, int offset)
{
    return RegisterExpression(re, -offset);
}

RegisterExpression operator+(const Register& r, const RegisterExpression& re)
{
    return RegisterExpression(re, &r);
}

RegisterExpression operator+(const RegisterExpression& re, const Register& r)
{
    return RegisterExpression(re, &r);
}

RegisterExpression operator+(const RegisterExpression& re1,
    const RegisterExpression& re2)
{
    return RegisterExpression(re1, re2);
}

class Operand : public Handle
{
public:
    Operand(Register* r) : Handle(create<RegisterBody>(r)) { }
    Operand(Register& r) : Handle(create<RegisterBody>(&r)) { }
    Operand(int size, int segment, Word offset, const Register* base,
        const Register* index)
      : Handle(create<MemoryBody>(size, segment, offset, base, index) { }
    Operand(int value, int size = 2)
      : Handle(create<ImmediateBody>(c, size)) { }
    int size() const { return body()->size(); }
    int length() const { return body()->length(); }
    void assemble(int opcode, int reg, Byte* p) const
    {
        body()->assemble(opcode, reg, p);
    }
    bool isRegister() const { return body()->isRegister(); }
    Register* reg() const { return body()->reg(); }
    int segment() const { return body()->segment(); }
    bool isConstant() const { return body()->isConstant(); }
    bool hasSize() const { return body()->size() != 0; }
protected:
    Operand(Handle h) : Handle(h) { }
    class Body : public Handle::Body
    {
    public:
        virtual int size() = 0;
        virtual int length() { return 2; }
        virtual void assemble(int opcode, int reg, Byte* p) = 0;
        virtual bool isRegister() { return true; }
        virtual Register* reg() { return 0; }
        virtual int segment() { return -1; }
        virtual bool isConstant() { return false; }
        virtual int value() { return 0; }
    };
    class RegisterBody : public Body
    {
    public:
        RegisterBody(Register* r) : _register(r) { }
        int size() const { return _register->width(); }
        void assemble(int opcode, int reg, Byte* p) const
        {
            *p = opcode;
            if (!generalWordRegisters.contains(_register) &&
                !generalByteRegisters.contains(_register))
                throw Exception("Not a general register.");
            p[1] = _register->binaryEncoding() + 0xc0 + (reg << 3);
        }
        Register* reg() const { return _register; }
    private:
        Register* _register;
    };
    class MemoryBody : public Body
    {
    public:
        MemoryBody(int size, int segment, RegisterExpression e)
          : _size(size), _segment(segment), _e(e) { }
        int size() const { return _size; }
        int length() const
        {
            return (_segment != -1 ? 1 : 0) + 1 + _e.length();
        }
        void assemble(int opcode, int reg, Byte* p) const
        {
            if (_segment != -1) {
                *p = 0x26 + (s << 3);
                ++p;
            }
            *p = opcode;
            _e.assemble(reg, p+1);
        }
        int segment() const { return _segment; }
        bool isRegister() const { return false; }
    private:
        int _size;
        int _segment;
        RegisterExpression _e;
    };
    class ImmediateBody : public body
    {
    public:
        ImmediateBody(int value, int size)
          : _value(value), _size(size) { }
        int size() const { return _size; }
        int length() const { return _size; }
        void assemble(int opcode, int reg, Byte* p) const
        {
            *p = opcode + reg;
            p[1] = _value;
            if (_size > 1)
                p[2] = _value >> 8;
        }
        bool isRegister() const { return false; }
        bool isConstant() const { return true; }
        int value() const { return _value; }
    private:
        int _size;
        int _value;
    };
    const Body* body() const { return as<Body>(); }
};

Operand word(Word offset, int segment = -1)
{
    return Operand(2, segment, RegisterExpression(offset, 0, 0));
}

Operand byte(Word offset, int segment = -1)
{
    return Operand(1, segment, RegisterExpression(offset, 0, 0));
}

Operand memory(Word offset, int segment = -1)
{
    return Operand(0, segment, RegisterExpression(offset, 0, 0));
}

Operand word(const Register& r, int segment = -1)
{
    return Operand(2, segment, RegisterExpression(0, &r, 0));
}

Operand byte(const Register& r, int segment = -1)
{
    return Operand(1, segment, RegisterExpression(0, &r, 0));
}

Operand memory(const Register& r, int segment = -1)
{
    return Operand(0, segment, RegisterExpression(0, &r, 0));
}

Operand word(const RegisterExpression& r, int segment = -1)
{
    return Operand(2, segment, r);
}

Operand byte(const RegisterExpression& r, int segment = -1)
{
    return Operand(1, segment, r);
}

Operand memory(const RegisterExpression& r, int segment = -1)
{
    return Operand(0, segment, r);
}


class Instruction : public LinkedListMember<Instruction>
{
public:
    virtual Instruction* expand() = 0;
    virtual int length() const = 0;
    virtual void assemble(Byte* p) const = 0;
};

class OneByteInstruction : public Instruction
{
public:
    int length() const { return 1; }
};

class AADInstruction : public Instruction
{
public:
    AADInstruction(int n = 10) : _n(n) { }
    int length() const { return 2; }
    void assemble(Byte* p) const
    {
        *p = 0xd5;
        p[1] = _n;
    }
private:
    int _n;
};

class AAMInstruction : public Instruction
{
public:
    AAMInstruction(int n = 10) : _n(n) { }
    int length() const { return 2; }
    void assemble(Byte* p) const
    {
        *p = 0xd4;
        p[1] = _n;
    }
private:
    int _n;
};

class AluInstruction : public Instruction
{
public:
    AluInstruction(const Operand& destination, const Operand& source)
      : _destination(destination), _source(source)
    {
        if (_source.isConstant())
    }
    int length() const
    {
        if (_source.isConstant()) {
            if (_destination.isRegister() &&
                _destination.reg()->binaryEncoding() == 0)
                return 1 + (_destination.wordSize() ? 2 : 1);
            return _destination.length() + (_destination.wordSize() ? 2 : 1);
        }
        if (_source.isRegister())
            return _destination.length();
        return _source.length();
    }
    void assemble(Byte* p) const
    {
        int op = operation();
        if (_source.isConstant()) {
            int v = _source.value();
            if (_destination.isRegister() &&
                _destination.reg()->binaryEncoding() == 0) {
                _destination.assemble(4, op << 3,
                *p = 4 + (op << 3) + (_destination.wordSize() ? 1 : 0);
                p[1] = v;
                if (_destination.wordSize())
                    p[2] = v >> 8;
                return;
            }
            if (!_destination.wordSize()) {
                _destination.assemble(0x80, op, p);
                p += _destination.length();
                *p = v;
                return;
            }
            if (v >= -128 && v < 128) {
                _destination.assemble(0x83, op, p);
                p += _destination.length();
                *p = v;
                return;
            }
            _destination.assemble(0x81, op, p);
            p += _destination.length();
            *p = v;
            p[1] = v >> 8;
            return;
        }
        if (_source.isRegister()) {
            _destination.assemble(op*8 + (_source.wordSize() ? 1 : 0),
                _source.reg()->binaryEncoding(), p);
            return;
        }
        _source.assemble(2 + op*8 + (_destination.wordSize() ? 1 : 0),
            _destination.reg()->binaryEncoding(), p);
        return;
    }
protected:
    virtual int operation() const = 0;
private:
    Operand _destination;
    Operand _source;
};

class ADDInstruction : public AluInstruction
{
private:
    int operation() const { return 0; }
};

class ORInstruction : public AluInstruction
{
private:
    int operation() const { return 1; }
};

class ADCInstruction : public AluInstruction
{
private:
    int operation() const { return 2; }
};

class SBBInstruction : public AluInstruction
{
private:
    int operation() const { return 3; }
};

class ANDInstruction : public AluInstruction
{
private:
    int operation() const { return 4; }
};

class SUBInstruction : public AluInstruction
{
private:
    int operation() const { return 5; }
};

class XORInstruction : public AluInstruction
{
private:
    int operation() const { return 6; }
};

class CMPInstruction : public AluInstruction
{
private:
    int operation() const { return 7; }
};

class CALLInstruction : public Instruction
{
public:
    CALLInstruction(const Operand& address) : _address(address) { }

private:
    Operand _address;
};

9A cp    CALL cp    E8 cv    CALL cv    FF /2    CALL rmw   FF /3    CALL mp
F6 /2    NOT  rmb   F7 /2    NOT  rmw
F6 /3    NEG  rmb   F7 /3    NEG  rmw
F6 /4    MUL  rmb   F7 /4    MUL  rmw
F6 /5    IMUL rmb   F7 /5    IMUL rmw
F6 /6    DIV  rmb   F7 /6    DIV  rmw
F6 /7    IDIV rmb   F7 /7    IDIV rmw
D8+i /r  ESC i,r,rm
EC       IN AL,DX   E4 ib    IN AL,ib   ED       IN AX,DX   E5 ib    IN AX,ib
EE       OUT DX,AL  EF       OUT DX,AX  E6 ib    OUT ib,AL  E7 ib    OUT ib,AX
CC       INT 3      CD ib    INT ib
77 cb    JA cb      73 cb    JAE cb     72 cb    JB cb      76 cb    JBE cb     74 cb    JE cb      7F cb    JG cb      7D cb    JGE cb     7C cb    JL cb      7E cb    JLE cb     75 cb    JNE cb     71 cb    JNO cb     79 cb    JNS cb     70 cb    JO cb      7A cb    JP cb      78 cb    JS cb      7B cb    JNP cb
EB cb    JMP cb     EA cp    JMP cp     E9 cv    JMP cv     FF /5    JMP mp     FF /4    JMP rmv
E3 cb    JCXZ cb
C4 /r    LES rw,m   C5 /r    LDS rw,m
8D /r    LEA rw,m
E2 cb    LOOP cb
E1 cb    LOOPE cb
E0 cb    LOOPNE cb
B0+r ib  MOV rb,ib  A0 iw    MOV AL,xb  B8+r iw  MOV rw,iw  A1 iv    MOV AX,xw  8A /r    MOV rb,rmb C6 /0 ib MOV rmb,ib C6 /r ib MOV rmb,ib 88 /r    MOV rmb,rb C7 /0 iw MOV rmw,iw C7 /r iw MOV rmw,iw 89 /r    MOV rmw,rw 8B /r    MOV rw,rmw 8E /r    MOV segreg,rmw A2 iv    MOV xb,AL A3 iw    MOV xw,AX 8C /r    MOV rmw,segreg
58+r     POP rw     8F /0    POP mw     07+8*r   POP segreg
50+r     PUSH rw    FF /6    PUSH rmw   06+8*r   PUSH segreg
D0 /2    RCL rmb,1  D2 /2    RCL rmb,CL D1 /2    RCL rmv,1  D3 /2    RCL rmv,CL
D0 /3    RCR rmb,1  D2 /3    RCR rmb,CL D1 /3    RCR rmv,1  D3 /3    RCR rmv,CL
D0 /0    ROL rmb,1  D2 /0    ROL rmb,CL D1 /0    ROL rmv,1  D3 /0    ROL rmv,CL
D0 /1    ROR rmb,1  D2 /1    ROR rmb,CL D1 /1    ROR rmv,1  D3 /1    ROR rmv,CL
D0 /7    SAR rmb,1  D2 /7    SAR rmb,CL D1 /7    SAR rmv,1  D3 /7    SAR rmv,CL
D0 /4    SHL rmb,1  D2 /4    SHL rmb,CL D1 /4    SHL rmv,1  D3 /4    SHL rmv,CL
D0 /5    SHR rmb,1  D2 /5    SHR rmb,CL D1 /5    SHR rmv,1  D3 /5    SHR rmv,CL
C3       RET        C2 iw    RET iw
CB       RETF       CA iw    RETF iw
A8 ib    TEST AL,ib A9 iv    TEST AX,iv F6 /0 ib TEST rmb,ibF6 /1 ib TEST rmb,ib84 /r    TEST rmb,rbF7 /0 iv TEST rmv,ivF7 /1 iv TEST rmv,iv85 /r    TEST rmv,rv

class CBWInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0x98; }
};

class CMCInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0xf5; }
};

class SALCInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0xd6; }
};

class LAHFInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0x9f; }
};

class CLCInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0xf8; }
};

class STCInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0xf9; }
};

class CLDInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0xfc; }
};

class STDInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0xfd; }
};

class CLIInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0xfa; }
};

class STIInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0xfb; }
};

class SAHFInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0x9e; }
};

class DAAInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0x27; }
};

class DASInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0x2f; }
};

class AAAInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0x37; }
};

class AASInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0x3f; }
};

class CWDInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0x99; }
};

class WAITInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0x9b; }
};

class XLATBInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0xd7; }
};

class CMPSBInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0xa6; }
};

class CMPSWInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0xa7; }
};

class LODSBInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0xac; }
};

class LODSWInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0xad; }
};

class MOVSBInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0xa4; }
};

class MOVSWInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0xa5; }
};

class SCASBInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0xae; }
};

class SCASWInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0xaf; }
};

class STOSBInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0xaa; }
};

class STOSWInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0xab; }
};

class HLTInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0xf4; }
};

class LOCKInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0xf0; }
};

class POPFInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0x9d; }
};

class PUSHFInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0x9c; }
};

class INTOInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0xce; }
};

class IRETInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0xcf; }
};

class REPInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0xf3; }
};

class REPNEInstruction : public OneByteInstruction
{
public:
    void assemble(Byte* p) { *p = 0xf2; }
};


class IncDecInstruction : public Instruction
{
public:
    IncDecInstruction(const Operand& o) : _operand(o) { }
    int length() const
    {
        if (_operand.isRegister() &&
            generalWordRegisters.contains(_operand.reg()))
            return 1;
        return (_operand.segment() == -1 ? 0 : 1) + 1 + _operand.length();
    }
    void assemble(Byte* p)
    {
        Register* r = _operand.reg();
        if (generalWordRegisters.contains(r)) {
            *p = 0x40 + r->binaryEncoding() + (isDec ? 8 : 0);
            return;
        }
        int s = _operand.segment();
        if (s != -1) {
            *p = 0x26 + (s << 3);
            ++p;
        }
        *p = 0xfe + (_operand.wordSize() ? 1 : 0);
        _operand.assemble(isDec() ? 1 : 0, p);
    }
protected:
    virtual bool isDec() const { return false; }
private:
    Operand _operand;
};

class INCInstruction : public IncDecInstruction
{
public:
    INCInstruction(const Operand& o) : IncDecInstruction(o) { }
};

class DECInstruction : public IncDecInstruction
{
public:
    DECInstruction(const Operand& o) : IncDecInstruction(o) { }
private:
    bool isDec() const { return true; }
};

class XCHGInstruction : public Instruction
{
public:
    XCHGInstruction(const Operand& o1, const Operand& o2) : _o1(o1), _o2(o2)
    {
        if (_o1.wordSize() != _o2.wordSize())
            throw Exception("Operands to XCHG must be the same size.");
        Register* r1 = _o1.reg();
        Register* r2 = _o2.reg();
        if (!_o1.isRegister() && !_o2.isRegister())
            throw Exception("Memory<->Memory XCHG not yet implemented.");
        if (_o1.isRegister() && !generalRegisters.contains(r1))
            throw Exception("XCHG with register " + r1->toString() + " not yet implemented.");
        if (_o2.isRegister() && !generalRegisters.contains(r2))
            throw Exception("XCHG with register " + r2->toString() + " not yet implemented.");
        // Normalize to:
        //   ax,reg
        //   reg,reg
        //   reg,mem
        if (_o2.isRegister() && !_o1.isRegister())
            swap(_o1, _o2);
        if (_o2.reg() == &ax && _o1.isRegister())
            swap(_o1, _o2);
    }
    int length() const
    {
        if (_o1.reg() == &ax && _o2.isRegister())
            return 1;
        return _o2.length();
    }
    void assemble(Byte* p)
    {
        Register* r1 = _o1.reg()
        if (r1 == &ax && _o2.isRegister()) {
            *p = 0x90 + r1->binaryEncoding();
            return;
        }
        _o2.assemble(0x86 + (_operand.wordSize() ? 1 : 0),
            r1->binaryEncoding(), p);
    }
private:
    Operand _o1;
    Operand _o2;
};

class NOPInstruction : public XCHGInstruction
{
public:
    NOPInstruction() : XCHGInstruction(ax, ax) { }
};

class MOVInstruction : public Instruction
{
public:
    MOVInstruction(Operand destination, Operand source)
      : _destination(destination), _source(source)
    { }
private:
    Operand _destination;
    Operand _source;

};

class LabelInstruction : public Instruction
{
};

class InstructionChain
{
public:
    void add(Instruction* instruction) { _instructions.add(instruction); }
private:
    OwningLinkedList<Instruction> _instructions;
};

class PortWriteByteInstruction : public Instruction
{
public:
    PortWriteByteInstruction(Word port, Byte value)
      : _port(port), _value(value) { }
    Instruction* expand2()
    {
        if (port >= 0x100)
            insertBefore(new SetInstruction(&dx, _port));
        if (value.size() == 1) {
            mov(al, value);
            if (dx.value() == port
        }
        else
            mov(ax, value);
    }
private:
    Word _port;
    Byte _value;
};

class SetInstruction : public Instruction
{
public:
    SetInstruction(Register* destination, Word value, Symbol symbol = zero)
      : _destination(destination), _value(value), _symbol(symbol) { }
    Instruction* expand()
    {
        if (dynamic_cast<CPURegister*>(_destination) != 0)
            return this;

        if (_symbol == zero) {
            if (_destination->symbol() == zero) {
                auto pr = dynamic_cast<PortRegister*>(_destionation);
                if (pr != 0) {
                    auto ipr = dynamic_cast<IndexedPortRegister*>(pr);
                    if (ipr != 0) {
                        insertBefore(new SetInstruction(ipr->indexRegister(),
                            ipr->indexValue());
                    }
                    insertBefore(new PortWriteByteInstruction(
                        pr->registerPort(), value);
                    insertBefore(new SetNote(_destination, _value, _symbol));
                    Instruction* r = previous();
                    remove();
                    return r;
                }
                else {
                    if (!_destination->hasValue(_value)) {

                    }
                }
            }
            else {

            }
        }
        else {

        }

    }
private:
    Register* _destination;
    Word _value;
    Symbol _symbol;
};

static const Word port_CGA_status = 0x3da;
static const Byte mask_CGA_notDisplayEnable = 1;
static const Byte mask_CGA_verticalSync = 8;

class CGAWaitForDisplayEnableInstruction : public Instruction
{
public:
    Instruction* expand()
    {
        insertBefore(new SetInstruction(&dx, port_CGA_status));
        LabelInstruction wait = new LabelInstruction();
        insertBefore(wait);
        insertBefore(new INInstruction(&al, &dx));
        insertBefore(new TESTInstruction(&al, mask_CGA_notDisplayEnable));
        insertBefore(new JNZInstruction(wait));
        Instruction* r = previous();
        remove();
        return r;
    }
};

class CGAWaitForDisplayDisableInstruction : public Instruction
{
public:
    Instruction* expand()
    {
        insertBefore(new SetInstruction(&dx, port_CGA_status));
        LabelInstruction wait = new LabelInstruction();
        insertBefore(wait);
        insertBefore(new INInstruction(&al, &dx));
        insertBefore(new TESTInstruction(&al, mask_CGA_notDisplayEnable));
        insertBefore(new JZInstruction(wait));
        Instruction* r = previous();
        remove();
        return r;
    }
};

class CGAWaitForVerticalSyncInstruction : public Instruction
{
public:
    Instruction* expand()
    {
        insertBefore(new SetInstruction(&dx, port_CGA_status));
        LabelInstruction wait = new LabelInstruction();
        insertBefore(wait);
        insertBefore(new INInstruction(&al, &dx));
        insertBefore(new TESTInstruction(&al, mask_CGA_verticalSync));
        insertBefore(new JZInstruction(wait));
        Instruction* r = previous();
        remove();
        return r;
    }
};

class CGAWaitForNoVerticalSyncInstruction : public Instruction
{
public:
    Instruction* expand()
    {
        insertBefore(new SetInstruction(&dx, port_CGA_status));
        LabelInstruction wait = new LabelInstruction();
        insertBefore(wait);
        insertBefore(new INInstruction(&al, &dx));
        insertBefore(new TESTInstruction(&al, mask_CGA_verticalSync));
        insertBefore(new JNZInstruction(wait));
        Instruction* r = previous();
        remove();
        return r;
    }
};


class Program : public ProgramBase
{
public:
    void run()
    {

    }
};
