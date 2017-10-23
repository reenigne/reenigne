class Register
{
public:
private:

};

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

private:
    LinkedList<Instruction> _instructions;
};

class Operand
{
};

InstructionChain currentInstructionChain;

void emit_mov(Operand destination, Operand source)
{
    currentInstructionChain.add(MovInstruction(destination, source));
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
