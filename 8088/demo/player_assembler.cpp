enum Atom
{
    atomRegister,
    atomAdd,
    atomSubtract,
    atomDereference,

    atomByte,
    atomWord,

    atomMov,
    atomPop,
    atomALU,
    atomOut,
    atomXchg,
    atomSegmentOverride,
    atomInc,
    atomJCond,
    atomJmp
};

#include "alfe/symbol.h"

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

Symbol parseRegister(CharacterSource* source)
{
    static String ax("ax");
    static String bx("bx");
    static String cx("cx");
    static String dx("dx");
    static String si("si");
    static String di("di");
    static String sp("sp");
    static String bp("bp");
    static String al("al");
    static String bl("bl");
    static String cl("cl");
    static String dl("dl");
    static String ah("ah");
    static String bh("bh");
    static String ch("ch");
    static String dh("dh");
    static String cs("cs");
    static String ss("ss");
    static String es("es");
    static String ds("ds");
    Span span;
    if (Space::parseKeyword(source, ax, &span))
        return Symbol(atomRegister, Symbol(atomWord), 0);
    if (Space::parseKeyword(source, cx, &span))
        return Symbol(atomRegister, Symbol(atomWord), 1);
    if (Space::parseKeyword(source, dx, &span))
        return Symbol(atomRegister, Symbol(atomWord), 2);
    if (Space::parseKeyword(source, bx, &span))
        return Symbol(atomRegister, Symbol(atomWord), 3);
    if (Space::parseKeyword(source, sp, &span))
        return Symbol(atomRegister, Symbol(atomWord), 4);
    if (Space::parseKeyword(source, bp, &span))
        return Symbol(atomRegister, Symbol(atomWord), 5);
    if (Space::parseKeyword(source, si, &span))
        return Symbol(atomRegister, Symbol(atomWord), 6);
    if (Space::parseKeyword(source, di, &span))
        return Symbol(atomRegister, Symbol(atomWord), 7);

    if (Space::parseKeyword(source, al, &span))
        return Symbol(atomRegister, Symbol(atomWord), 0);
    if (Space::parseKeyword(source, cl, &span))
        return Symbol(atomRegister, Symbol(atomWord), 1);
    if (Space::parseKeyword(source, dl, &span))
        return Symbol(atomRegister, Symbol(atomWord), 2);
    if (Space::parseKeyword(source, bl, &span))
        return Symbol(atomRegister, Symbol(atomWord), 3);
    if (Space::parseKeyword(source, ah, &span))
        return Symbol(atomRegister, Symbol(atomWord), 4);
    if (Space::parseKeyword(source, ch, &span))
        return Symbol(atomRegister, Symbol(atomWord), 5);
    if (Space::parseKeyword(source, dh, &span))
        return Symbol(atomRegister, Symbol(atomWord), 6);
    if (Space::parseKeyword(source, bh, &span))
        return Symbol(atomRegister, Symbol(atomWord), 7);

    if (Space::parseKeyword(source, es, &span))
        return Symbol(atomRegister, Symbol(atomSegmentOverride), 0);
    if (Space::parseKeyword(source, cs, &span))
        return Symbol(atomRegister, Symbol(atomSegmentOverride), 1);
    if (Space::parseKeyword(source, ss, &span))
        return Symbol(atomRegister, Symbol(atomSegmentOverride), 2);
    if (Space::parseKeyword(source, ds, &span))
        return Symbol(atomRegister, Symbol(atomSegmentOverride), 3);
    return Symbol();
}

Symbol parseExpression(CharacterSource* source);

Symbol parseMemory(CharacterSource* source)
{
    CharacterSource s = *source;
    Span span;
    int c = s.get(span);
    int size = 0;
    if (c == 'b' || c == 'B') {
        Space::parse(&s);
        size = 1;
        c = s.get(span);
    }
    else
        if (c == 'w' || c == 'W') {
            Space::parse(&s);
            size = 2;
            c = s.get(span);
        }
    if (c != '[')
        return Symbol();
    Space::parse(&s);
    Symbol expression = parseExpression(&s);
    if (!expression.valid())
        throw Exception();
    c = s.get(span);
    if (c != ']')
        throw Exception();
    Space::parse(&s);
    *source = s;
    return Symbol(atomDereference, size, expression);
}

int parseHexadecimalCharacter(CharacterSource* source)
{
    CharacterSource s = *source;
    Span span;
    int c = s.get(span);
    if (c >= '0' && c <= '9') {
        *source = s;
        return c - '0';
    }
    if (c >= 'A' && c <= 'F') {
        *source = s;
        return c + 10 - 'A';
    }
    if (c >= 'a' && c <= 'f') {
        *source = s;
        return c + 10 - 'a';
    }
    return -1;
}

Symbol parseInteger(CharacterSource* source)
{
    CharacterSource s = *source;
    int n = 0;
    Span span;
    int c = s.get(&span);
    if (c < '0' || c > '9')
        return Symbol();
    if (c == '0') {
        do {
            int d = parseHexadecimalCharacter(source, &span);
            if (d == -1)
                break;
            n = n*16 + d;
        } while (true);
    }
    do {
        n = n*10 + c - '0';
        *source = s;
        c = s.get(&span);
        if (c < '0' || c > '9')
            break;
    } while (true);
    Space::parse(source);
    return Symbol(atomIntegerConstant, n);
}

Symbol parseLValue(CharacterSource* source)
{
    Symbol l = parseRegister(source);
    if (l.valid())
        return l;
    l = parseMemory(source);
    if (l.valid())
        return l;
    throw Exception();
}

Symbol parseExpressionElement(CharacterSource* source)
{
    Symbol e = parseInteger(source);
    if (e.valid())
        return e;
    return parseLValue(source);
}

Symbol parseExpression(CharacterSource* source)
{
    Symbol e = parseExpressionElement(source);
    if (!e.valid())
        return Symbol();
    do {
        Span span;
        if (Space::parseCharacter(source, '+', &span)) {
            Symbol e2 = parseExpressionElement(source);
            if (!e2.valid())
                throw Exception();
            e = Symbol(atomAdd, e, e2);
            continue;
        }
        if (Space::parseCharacter(source, '-', &span)) {
            Symbol e2 = parseExpressionElement(source);
            if (!e2.valid())
                throw Exception();
            e = Symbol(atomSubtract, e, e2);
            continue;
        }
        return e;
    } while (true);
}

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
