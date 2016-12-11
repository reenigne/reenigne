#include <stdio.h>

typedef unsigned char Byte;
typedef unsigned short int Word;

Word registers[12];
Byte* byteRegisters[8];
Word ip = 0x100;
Byte* ram;
bool useMemory;
Word address;
Word flags = 2;

Word ax() { return registers[0]; }
Word cx() { return registers[1]; }
Word dx() { return registers[2]; }
Word bx() { return registers[3]; }
Word sp() { return registers[4]; }
Word bp() { return registers[5]; }
Word si() { return registers[6]; }
Word di() { return registers[7]; }
Byte al() { return *byteRegisters[0]; }
Byte cl() { return *byteRegisters[1]; }
Byte dl() { return *byteRegisters[2]; }
Byte bl() { return *byteRegisters[3]; }
Byte ah() { return *byteRegisters[4]; }
Byte ch() { return *byteRegisters[5]; }
Byte dh() { return *byteRegisters[6]; }
Byte bh() { return *byteRegisters[7]; }
void setAX(Word value) { registers[0] = value; }
void setCX(Word value) { registers[1] = value; }
void setDX(Word value) { registers[2] = value; }
void setBX(Word value) { registers[3] = value; }
void setSP(Word value) { registers[4] = value; }
void setBP(Word value) { registers[5] = value; }
void setSI(Word value) { registers[6] = value; }
void setDI(Word value) { registers[7] = value; }
void setAL(Byte value) { *byteRegisters[0] = value; }
void setCL(Byte value) { *byteRegisters[1] = value; }
void setDL(Byte value) { *byteRegisters[2] = value; }
void setBL(Byte value) { *byteRegisters[3] = value; }
void setAH(Byte value) { *byteRegisters[4] = value; }
void setCH(Byte value) { *byteRegisters[5] = value; }
void setDH(Byte value) { *byteRegisters[6] = value; }
void setBH(Byte value) { *byteRegisters[7] = value; }
Word es() { return registers[8]; }
Word cs() { return registers[9]; }
Word ss() { return registers[10]; }
Word ds() { return registers[11]; }
void setES(Word value) { registers[8] = value; }
void setCS(Word value) { registers[9] = value; }
void setSS(Word value) { registers[10] = value; }
void setDS(Word value) { registers[11] = value; }

Word ea()
{
    Byte modRM = ram[ip];
    ++ip;
    if ((modRM & 0xc0) == 0xc0) {
        useMemory = false;
        return modRM & 7;
    }
    useMemory = true;
    switch (modRM & 7) {
        case 0: return bx() + si();
        case 1: return bx() + di();
        case 2: return bp() + si();
        case 3: return bp() + di();
        case 4: return        si();
        case 5: return        di();
        case 6: return bp();
        case 7: return bx();
    }
}

void readEA()
{
    switch (modRM & 0xc0) {
        case 0x00:
            if ((modRM & 7) == 6)
                fetch(stateEAOffset, true);
            else
                _state = stateEASetSegment;
            break;
        case 0x40: fetch(stateEAByte, false); break;
        case 0x80: fetch(stateEAWord, true); break;
    }

}

void error(const char* operation, const char* filename)
{
    fprintf(stderr, "Error %s file %s: %s\n", operation, filename,
        sys_errlist[errno]);
    exit(1);
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        printf("Usage: %s <program name>\n",argv[0]);
        exit(0);
    }
    const char* filename = argv[1];
    FILE* fp = fopen(filename, "rb");
    if (fp == 0)
        error("opening");
    ram = (Byte*)malloc(0x10000);
    memset(ram, 0, 0x10000);
    if (ram == 0) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    if (fseek(fp, 0, SEEK_END) != 0)
        error("seeking", filename);
    int length = ftell(fp);
    if (length == -1)
        error("telling", filename);
    if (fseek(fp, 0, SEEK_SET) != 0)
        error("seeking", filename);
    if (length > 0x10000 - 0x100) {
        fprintf(stderr, "%s is too long to be a .com file\n");
        exit(1);
    }
    if (fread(&ram[0x100], length, 1, fp) != 1)
        error("reading", filename);
    fclose(fp);

    Word segment = 0x1000;
    setAX(0x0000);
    setCX(0x00FF);
    setDX(segment);
    setBX(0x0000);
    setSP(0xFFFE);
    setBP(0x091C);
    setSI(0x0100);
    setDI(0xFFFE);
    setES(segment);
    setCS(segment);
    setSS(segment);
    setDS(segment);

    Byte* byteData = &registers[0];
    if (registers[2] == 0) {
        // Little-endian
        byteRegisters[0] = &byteData[0];
        byteRegisters[1] = &byteData[2];
        byteRegisters[2] = &byteData[4];
        byteRegisters[3] = &byteData[6];
        byteRegisters[4] = &byteData[1];
        byteRegisters[5] = &byteData[3];
        byteRegisters[6] = &byteData[5];
        byteRegisters[7] = &byteData[7];
    }
    else {
        // Big-endian
        byteRegisters[0] = &byteData[1];
        byteRegisters[1] = &byteData[3];
        byteRegisters[2] = &byteData[5];
        byteRegisters[3] = &byteData[7];
        byteRegisters[4] = &byteData[0];
        byteRegisters[5] = &byteData[2];
        byteRegisters[6] = &byteData[4];
        byteRegisters[7] = &byteData[6];
    }

    for (long i = 0; i < 1000000000; ++i) {
        Byte opcode = ram[ip];
        bool wordSize = ((opcode & 1) != 0);
        bool sourceIsRM = ((_opcode & 2) != 0);

        ++ip;
        switch (opcode) {
            case 0x00:
            case 0x01:
            case 0x02:
            case 0x03:
            case 0x08:
            case 0x09:
            case 0x0a:
            case 0x0b:
            case 0x10:
            case 0x11:
            case 0x12:
            case 0x13:
            case 0x18:
            case 0x19:
            case 0x1a:
            case 0x1b:
            case 0x20:
            case 0x21:
            case 0x22:
            case 0x23:
            case 0x28:
            case 0x29:
            case 0x2a:
            case 0x2b:
            case 0x30:
            case 0x31:
            case 0x32:
            case 0x33:
            case 0x38:
            case 0x39:
            case 0x3a:
            case 0x3b:
                readEA();
                if (!sourceIsRM) {
                    destination = data;
                    source = getReg();
                }
                else {
                    destination = getReg();
                    source = data;
                }
                aluOperation = (opcode >> 3) & 7;
                doALUOperation();
                if (aluOperation != 7) {
                    if (!sourceIsRM)
                        writeEA(data, 0);
                    else
                        setReg(data);
                }
                break;
        }
    }
}
