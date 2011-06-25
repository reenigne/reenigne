static const int c = 52;
static const int s = 22;
static const int b = 21;

int state = 0;
// States:
//   0 = Block start
//   1 = Mixing
//   2 = Block finish
//   3 = Frame update
int mixingSample = 0;
int block = 0;
int voice = 0;

class InstructionByte
{
private:
    Instruction* _instruction;
    int _byte;
};

class InstructionCycle
{
private:
    Instruction* _instruction;
    int _cycle;
};

class Instruction
{
private:
    String _text;
    int _bytes;
    Byte _data[5];
    int _ios;
    int _position;
    int _cycle;
};

Instruction getNextInstruction()
{
    do {
        Instruction i;
        switch (state) {
            case 0:
                i = getNextInstructionMixing();
                if (i.valid())
                    return i;

                state = 1;
                break;
            case 1:
                i = getNextInstruction


}

int main()
{
}
