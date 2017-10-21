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
