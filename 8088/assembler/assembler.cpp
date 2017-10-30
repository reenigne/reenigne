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
    Word value() { return _value; }
    bool isConstant() { return _symbol == zero; }
    virtual void assign(T value, Symbol symbol)
    {
        _value = value;
        _symbol = symbol;
        for (auto& i : _partOf)
            i->partAssigned(value, symbol, this);
    }
    bool hasValue(Word v) { return isConstant() && value() == v; }
    void clear() { assign(0, unknown); }
    void makePartOf(Register* larger)
    {
        _partOf.append(larger);
    }
    virtual void partAssigned(Word value, Symbol symbol, Register* part) { }
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
    int _binaryEncoding;
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

class Instruction : public LinkedListMember<Instruction>
{
};

class MovInstruction : public Instruction
{
public:
    MovInstruction(Operand destination, Operand source)
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

class Operand
{
};

void emit_mov(InstructionChain chain, Operand destination, Operand source)
{
    chain.add(MovInstruction(destination, source));
}

void out(Operand port, Operand value)
{
    if (port >= 0x100)
        mov(dx, port);
    if (value.size() == 1) {
        mov(al, value);
        if (dx.value() == port
    }
    else
        mov(ax, value);


}

class SetInstruction
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
