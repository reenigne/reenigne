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
public:
    Instruction(String text)
    {
        CharacterSource source(text, "");
        Space::parse(&source);
        Span span;
        static String mov("mov");
        if (Space::parseKeyword(source, mov, &span)) {
            Symbol left = parseLValue(&source);
            if (!Space::parseCharacter(&source, ',', &span))
                throw Exception();
            Symbol right = parseExpression(&source);

/*
   88 /r          MOV rmb,rb             2  5+EA+4
   89 /r          MOV rmw,rw             2  5+EA+8
   8A /r          MOV rb,rmb             2  4+EA+4
   8B /r          MOV rw,rmw             2  4+EA+8
   A0 iw          MOV AL,xb              6+4
   A1 iw          MOV AX,xw              6+8
   A2 iw          MOV xb,AL              6+4
   A3 iw          MOV xw,AX              6+8
   B? ib          MOV rb,ib              4
   B? iw          MOV rw,iw              4
   C6 /0 ib       MOV rmb,ib             4  6+EA+4
   C6 /r ib       MOV rmb,ib             4  6+EA+4
   C7 /0 iw       MOV rmw,iw             4  6+EA+8
   C7 /r iw       MOV rmw,iw             4  6+EA+8
   8C /r          MOV rmw,segreg         2  5+EA+8
   8E /r          MOV segreg,rmw         2  4+EA+8
*/
        }
        static String pop("pop");
/*
   5?             POP rw                 4+8
*/
        static String add("add");
        static String adc("adc");
        static String sub("sub");
        static String cmp("cmp");
/*
   ?? /r          alu rmb,rb             3  8+EA+4+4
   ?? /r          alu rmw,rw             3  8+EA+8+8
   ?? /r          alu rb,rmb             3  5+EA+4
   ?? /r          alu rw,rmw             3  5+EA+8
   ?? ib          alu AL,ib              4
   ?? iw          alu AX,iw              4
   80 /? ib       alu rmb,ib             4  9+EA+4+4
   81 /0 iw       alu rmw,iw             4  9+EA+8+8
   83 /? ib       alu rmw,ib             4  9+EA+8+8
   38 /r          CMP rmb,rb             3  5+EA+4
   39 /r          CMP rmw,rw             3  5+EA+8
   80 /7 ib       CMP rmb,ib             4  6+EA+4
   81 /7 iw       CMP rmw,iw             4  6+EA+8
   83 /7 ib       CMP rmw,ib             4  6+EA+8
*/
        static String out("out");
/*
   E6 ib          OUT ib,AL              6+4
*/
        static String xchg("xchg");
/*
   86 /r          XCHG rmb,rb            4  9+EA+4+4
*/
        static String es("es:");
/*
   ??             segreg:
*/
        static String inc("inc");
/*
   4?             INC rw                 2
*/
        static String je("je");
/*
   7? cb          Jcond cb               4/16
*/
        static String jmp("jmp");
/*
   E9 cw          JMP cw                15
*/

    }
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
