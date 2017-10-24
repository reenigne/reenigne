#include "alfe/main.h"

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

template<class T> class Register
{
public:
    virtual T value() const = 0;
    virtual bool isConstant() const = 0;
    virtual void assign(T value, Symbol symbol) = 0;
    bool hasValue(T v) { return isConstant() && value() == v; }
    void clear() { assign(0, unknown); }
};

template<class T> class SimpleRegister : public Register<T>
{
public:
    Register(String name, int width, int binaryEncoding)
      : _name(name), _width(width), _binayrEncoding(binaryEncoding) { }
    T value() const { return _value; }
    bool isConstant() const { return _symbol == zero; }
    void assign(T value, Symbol symbol) { _value = value; _symbol = symbol; }
private:
    RegisterFile* _file;
    String _name;
    int _width;
    int _binaryEncoding;

    T _value;
    Symbol _symbol;
};

template<class T, class T2> class CompoundRegister : public Register<T>
{
public:
    CompoundRegister(Register* lowPart, Register* highPart,
        int binaryEncoding)
      : _lowPart(lowPart), _highPart(highPart),
        _binaryEncoding(binaryEncoding)
    { }
    T value() const
    {
        return (_highPart->value() << (8 * sizeof(T2))) | (_lowPart->value());
    }
    bool isConstant() const
    {
        return _lowPart->isConstant() && _highPart->isConstant();
    }
    void assign(T value, Symbol symbol)
    {

    }
private:
    Register<T2>* _lowPart;
    Register<T2>* _highPart;
    int _binaryEncoding;
};

RegisterFile registerFile;
SimpleRegister<Byte> al(&registerFile, "al", 0);
SimpleRegister<Byte> cl(&registerFile, "cl", 1);
SimpleRegister<Byte> dl(&registerFile, "dl", 2);
SimpleRegister<Byte> bl(&registerFile, "bl", 3);
SimpleRegister<Byte> ah(&registerFile, "ah", 4);
SimpleRegister<Byte> ch(&registerFile, "ch", 5);
SimpleRegister<Byte> dh(&registerFile, "dh", 6);
SimpleRegister<Byte> bh(&registerFile, "bh", 7);
SimpleRegister<Word> es(&registerFile, "es", 0);
SimpleRegister<Word> cs(&registerFile, "cs", 1);
SimpleRegister<Word> ss(&registerFile, "ss", 2);
SimpleRegister<Word> ds(&registerFile, "ds", 3);
CompoundRegister<Word, Byte> ax(&registerFile, &al, &ah, 0);
CompoundRegister<Word, Byte> cx(&registerFile, &cl, &dh, 1);
CompoundRegister<Word, Byte> dx(&registerFile, &dl, &dh, 2);
CompoundRegister<Word, Byte> bx(&registerFile, &bl, &bh, 3);
SimpleRegister<Word> sp(&registerFile, "sp", 4);
SimpleRegister<Word> bp(&registerFile, "bp", 5);
SimpleRegister<Word> si(&registerFile, "si", 6);
SimpleRegister<Word> di(&registerFile, "di", 7);
SimpleRegister<Word> ip(&registerFile, "ip", 0);
SimpleRegister<Word> flags(&registerFile, "flags", 0);

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

class Program : public ProgramBase
{
public:
    void run()
    {
    }
};
