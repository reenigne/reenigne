class Instruction
{
};

class MovInstruction : public Instruction
{
};

class LabelInstruction : public Instruction
{
};

class InstructionChain
{
};

void mov(Operand destination, Operand source)
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
