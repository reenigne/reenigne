#include "alfe/main.h"

int instructions[512];


int s_a = 0;
int s_ones = 1;
int s_sigma = 2;
int s_blank = 3;
int s_x = 4;
int s_q = 5;
int s_zero = 6;
int s_m = 7;
int s_ij = 8;
int s_ind = 9;
int s_ik = 10;
int s_opr = 11;
int s_parm = 12;
int s_xa = 13;
int s_hl = 14;
int s_bc = 15;
int s_pc = 16;

String s_names[] = {
    "A","ONES","SIGMA","","X","Q","ZERO","M",
    "IJ","IND","IK","OPR","(M)","XA","HL","BC",
    "PC"};

int d_tmpal = 0;
int d_tmpb = 1;
int d_a = 2;
int d_blank = 3;
int d_x = 4;
int d_tmpa = 5;
int d_tmpc = 6;
int d_no_dest = 7;
int d_ind = 8;
int d_ij = 9;
int d_ik = 10;
int d_m = 11;
int d_tmp2 = 12;
int d_opr = 13;
int d_bc = 14;

String d_names[] = {
    "tmpaL","tmpb","A","","X","tmpa","tmpc","no dest",
    "IND","IJ","IK","M","tmp2","OPR","BC"};

int t_1 = 0;
int t_blank = 1;
int t_0 = 2;
int t_4 = 3;
int t_7 = 4;
int t_5 = 5;
int t_6 = 6;

String t_names[] = { "1","","0","4","7","5","6" };

int a_xi = 0;
int a_blank = 1;
int a_dec = 2;
int a_x0 = 3;
int a_inc = 4;
int a_ncy = 5;
int a_none = 6;
int a_unc = 7;
int a_com1 = 8;
int a_pass = 9;
int a_rrcy = 10;
int a_maxc = 11;
int a_add = 12;
int a_ncz = 13;
int a_subt = 14;
int a_lrcy = 15;
int a_cy = 16;
int a_rcy = 17;
int a_f1 = 18;
int a_r = 19;
int a_w = 20;
int a_f1zz = 21;
int a_int = 22;
int a_zu = 23;
int a_dec2 = 24;
int a_flush = 25;

String a_names[] = {
    "XI","","DEC","X0","INC","NCY","none","UNC","COM1",
    "PASS","RRCY","MAXC","ADD","NCZ","SUBT","LRCY",
    "CY","RCY","F1","R","F1ZZ","INT","ZU","DEC2","FLUSH" };

int b_tmp2 = 0;
int b_blank = 1;
int b_tmpb = 2;
int b_5 = 3;
int b_7 = 4;
int b_rni = 5;
int b_3 = 6;
int b_tmpc = 7;
int b_tmpa_nx = 8;
int b_tmpb_nx = 9;
int b_none = 10;
int b_8 = 11;
int b_tmpa = 12;
int b_rtn = 13;
int b_13 = 14;
int b_14 = 15;
int b_10 = 16;
int b_1 = 17;
int b_dd_bl = 18;
int b_6 = 19;
int b_da_bl = 20;
int b_0 = 21;
int b_dd_f0 = 22;
int b_12 = 23;
int b_11 = 24;
int b_susp = 25;
int b_corr = 26;

String b_names[] = {
    "tmp2","","tmpb","5","7","RNI","3","tmpc",
    "tmpa, NX","tmpb, NX","8","tmpa","RTN","13","14",
    "10","1","DD,BL","6","DA,BL","12",
    "11","SUSP,CORR" };

int f_f = 0;
int f_blank = 1;

String f_names[] = { "F" ,"" };

auto fieldNames = { s_names, d_names, t_names, a_names, b_names, f_names };
auto fieldBits = { 5, 5, 2, 5, 3, 1 };

struct Instruction
{
    Instruction() { }
    Instruction(std::initializer_list<int> i)
    {
        _data[0] = i.begin()[0];
        _data[1] = i.begin()[1];
        _data[2] = i.begin()[2];
        _data[3] = i.begin()[3];
        _data[4] = i.begin()[4];
        _data[5] = i.begin()[5];
    }
    int _data[6];
};

struct InstructionGroup
{
    InstructionGroup(std::initializer_list<Instruction> instructions)
    {
        _nInstructions = instructions.size();
        _instructions.allocate(_nInstructions);
        for (int i = 0; i < _nInstructions; ++i)
            _instructions[i] = instructions.begin()[i];
    }
    int _nInstructions;
    Array<Instruction> _instructions;
};



InstructionGroup groups[]{
   {{s_a,     d_tmpal,   t_1,     a_xi,    b_tmp2,    f_blank},
    {s_ones,  d_tmpb,    t_blank, a_blank, b_blank,   f_blank},
    {s_sigma, d_a,       t_1,     a_dec,   b_tmpb,    f_f    },
    {s_blank, d_blank,   t_0,     a_x0,    b_5,       f_blank},
    {s_blank, d_blank,   t_1,     a_inc,   b_tmpb,    f_blank},
    {s_x,     d_tmpb,    t_0,     a_ncy,   b_7,       f_blank},
    {s_sigma, d_x,       t_4,     a_none,  b_rni,     f_blank},
    {s_blank, d_blank,   t_4,     a_none,  b_rni,     f_blank}},

   {{s_q,     d_tmpb,    t_blank, a_blank, b_blank,   f_blank},
    {s_zero,  d_tmpa,    t_blank, a_blank, b_blank,   f_blank},
    {s_a,     d_tmpc,    t_7,     a_unc,   b_3,       f_blank},
    {s_blank, d_blank,   t_1,     a_com1,  b_tmpc,    f_blank},
    {s_sigma, d_x,       t_1,     a_pass,  b_tmpa_nx, f_blank},
    {s_sigma, d_a,       t_4,     a_none,  b_rni,     f_f    },
    {s_blank, d_blank,   t_blank, a_blank, b_blank,   f_blank},
    {s_blank, d_blank,   t_blank, a_blank, b_blank,   f_blank}},

   {{s_m,     d_tmpb,    t_1,     a_xi,    b_tmpb_nx, f_blank},
    {s_sigma, d_m,       t_4,     a_none,  b_rni,     f_blank},
    {s_blank, d_blank,   t_blank, a_blank, b_blank,   f_blank},
    {s_zero,  d_tmpa,    t_1,     a_rrcy,  b_tmpc,    f_blank},
    {s_sigma, d_tmpc,    t_4,     a_maxc,  b_none,    f_blank},
    {s_blank, d_blank,   t_0,     a_ncy,   b_8,       f_blank},
    {s_blank, d_blank,   t_1,     a_add,   b_tmpa,    f_blank},
    {s_sigma, d_tmpa,    t_blank, a_blank, b_blank,   f_f    },
    {s_blank, d_blank,   t_1,     a_rrcy,  b_tmpa,    f_blank},
    {s_sigma, d_tmpa,    t_1,     a_rrcy,  b_tmpc,    f_blank},
    {s_sigma, d_tmpc,    t_0,     a_ncz,   b_5,       f_blank},
    {s_blank, d_blank,   t_4,     a_none,  b_rtn,     f_blank}},

   {{s_blank, d_blank,   t_1,     a_subt,  b_tmpa,    f_blank},
    {s_sigma, d_no_dest, t_4,     a_maxc,  b_none,    f_f    },
    {s_blank, d_blank,   t_5,     a_ncy,   b_7,       f_blank},
    {s_blank, d_blank,   t_1,     a_lrcy,  b_tmpc,    f_blank},
    {s_sigma, d_tmpc,    t_1,     a_lrcy,  b_tmpa,    f_blank},
    {s_sigma, d_tmpa,    t_1,     a_subt,  b_tmpa,    f_blank},
    {s_blank, d_blank,   t_0,     a_cy,    b_13,      f_blank},
    {s_sigma, d_no_dest, t_blank, a_blank, b_blank,   f_f    },
    {s_blank, d_blank,   t_0,     a_ncy,   b_14,      f_blank},
    {s_blank, d_blank,   t_0,     a_ncz,   b_3,       f_blank},
    {s_blank, d_blank,   t_1,     a_lrcy,  b_tmpc,    f_blank},
    {s_sigma, d_tmpc,    t_blank, a_blank, b_blank,   f_blank},
    {s_sigma, d_no_dest, t_4,     a_none,  b_rtn,     f_blank},
    {s_blank, d_blank,   t_0,     a_rcy,   b_none,    f_blank},
    {s_sigma, b_tmpa,    t_0,     a_ncz,   b_3,       f_blank},
    {s_blank, d_blank,   t_0,     a_unc,   b_10,      f_blank}},

   {{s_blank, d_blank,   t_7,     a_f1,    b_1,       f_blank},
    {s_ij,    d_ind,     t_6,     a_r,     b_dd_bl,   f_blank},
    {s_ind,   d_ij,      t_0,     a_x0,    b_6,       f_blank},
    {s_ik,    d_ind,     t_6,     a_w,     b_da_bl,   f_blank},
    {s_ind,   d_ik,      t_0,     a_f1,    b_0,       f_blank},
    {s_blank, d_blank,   t_4,     a_none,  b_rni,     f_blank},
    {s_opr,   d_m,       t_0,     a_f1,    b_0,       f_blank},
    {s_blank, d_blank,   t_4,     a_none,  b_rni,     f_blank}},

   {{s_blank, d_blank,   t_7,     a_f1,    b_1,       f_blank},
    {s_parm,  d_tmp2,    t_0,     a_x0,    b_5,       f_blank},
    {s_ij,    d_ind,     t_6,     a_r,     b_dd_bl,   f_blank},
    {s_ind,   d_ij,      t_blank, a_blank, b_blank,   f_blank},
    {s_opr,   d_tmpa,    t_blank, a_blank, b_blank,   f_blank},
    {s_ik,    d_ind,     t_6,     a_r,     b_da_bl,   f_blank},
    {s_opr,   d_tmpb,    t_1,     a_subt,  b_tmpa,    f_blank},
    {s_sigma, d_no_dest, t_blank, a_blank, b_blank,   f_f    },
    {s_ind,   d_ik,      t_blank, a_blank, b_blank,   f_blank},
    {s_blank, d_blank,   t_0,     a_f1zz,  b_0,       f_blank},
    {s_blank, d_blank,   t_4,     a_none,  b_rni,     f_blank},
    {s_blank, d_blank,   t_blank, a_blank, b_blank,   f_blank}},

   {{s_ik,    d_ind,     t_7,     a_f1,    b_1,       f_blank},
    {s_parm,  d_opr,     t_6,     a_w,     b_da_bl,   f_blank},
    {s_ind,   d_ik,      t_0,     a_f1,    b_0,       f_blank},
    {s_blank, d_blank,   t_4,     a_none,  b_rni,     f_blank}},

   {{s_zero,  d_tmpa,    t_blank, a_blank, b_blank,   f_blank},
    {s_xa,    d_tmpal,   t_blank, a_blank, b_blank,   f_blank},
    {s_hl,    d_tmpb,    t_1,     a_add,   b_tmpa,    f_blank},
    {s_sigma, d_ind,     t_6,     a_r,     b_dd_f0,   f_blank},
    {s_opr,   d_a,       t_4,     a_none,  b_rni,     f_blank},
    {s_blank, d_blank,   t_blank, a_blank, b_blank,   f_blank},
    {s_bc,    d_tmpb,    t_0,     a_int,   b_12,      f_blank},
    {s_blank, d_blank,   t_1,     a_pass,  b_tmpb,    f_blank},
    {s_sigma, d_no_dest, t_1,     a_dec,   b_tmpb,    f_blank},
    {s_blank, d_blank,   t_0,     a_zu,    b_11,      f_blank},
    {s_sigma, d_bc,      t_4,     a_none,  b_rtn,     f_blank},
    {s_blank, d_blank,   t_4,     a_none,  b_rni,     f_blank},
    {s_blank, d_blank,   t_4,     a_none,  b_susp,    f_blank},
    {s_blank, d_blank,   t_4,     a_none,  b_corr,    f_blank},
    {s_pc,    d_tmpb,    t_1,     a_dec2,  b_tmpb,    f_blank},
    {s_sigma, d_blank,   t_4,     a_flush, b_rni,     f_blank}}
};

class Program : public ProgramBase
{
public:
    void run()
    {
        for (int i = 0; i < 512; ++i)
            instructions[i] = 0;
        for (int y = 0; y < 4; ++y) {
            int h = (y < 3 ? 24 : 12);
            for (int half = 0; half < 2; ++half) {
                String s = File(
                    String("..\\..\\..\\Projects\\Emulation\\PC\\8086\\") +
                    (half == 1 ? "l" : "r") + decimal(y) + ".txt", true).
                    contents();
                String s2 = File(
                    String("..\\..\\..\\Projects\\Emulation\\PC\\8086\\") +
                    (half == 1 ? "l" : "r") + decimal(y) + "a.txt", true).
                    contents();
                for (int yy = 0; yy < h; ++yy) {
                    int ib = y * 24 + yy;
                    for (int xx = 0; xx < 64; ++xx) {
                        int b = (s[yy * 66 + (63 - xx)] == '0' ? 1 : 0);
                        int b2 = (s2[yy * 66 + (63 - xx)] == '0' ? 1 : 0);
                        //if (b != 0) {   // 8088
                        if (b2 != 0) {  // 8086
                            instructions[xx * 8 + half * 4 + yy % 4] |=
                                1 << (20 - (ib >> 2));
                        }
                        //if (b != b2) {
                        //    printf("Quad %i column %i row %i side %i  old %i new %i\n", y, xx, yy, half, b, b2);
                        //}
                    }
                }
            }
        }

        int stage1[128];
        for (int x = 0; x < 128; ++x)
            stage1[x] = 0;
        for (int g = 0; g < 9; ++g) {
            int n = 16;
            if (g == 0 || g == 8)
                n = 8;
            int xx[9] = { 0, 8, 24, 40, 56, 72, 88, 104, 120 };
            int xp = xx[g];
            for (int h = 0; h < 2; ++h) {
                String s = File(
                    String("..\\..\\..\\Projects\\Emulation\\PC\\8086\\") +
                    decimal(g) + (h == 0 ? "t" : "b") + ".txt", true).
                    contents();
                for (int y = 0; y < 11; ++y) {
                    for (int x = 0; x < n; ++x) {
                        int b = (s[y * (n + 2) + x] == '0' ? 1 : 0);
                        if (b != 0)
                            stage1[127 - (x + xp)] |= 1 << (y * 2 + (h ^ (y <= 2 ? 1 : 0)));
                    }
                }
            }
        }

        int groupInput[38 * 18];
        int groupOutput[38 * 15];
        String groupString = File(
            String("..\\..\\..\\Projects\\Emulation\\PC\\8086\\group.txt")).
            contents();
        for (int x = 0; x < 38; ++x) {
            for (int y = 0; y < 15; ++y) {
                groupOutput[y * 38 + x] =
                    (groupString[y * 40 + x] == '0' ? 1 : 0);
            }
            for (int y = 0; y < 18; ++y) {
                int c = groupString[((y/2) + 15) * 40 + x];
                if ((y & 1) == 0)
                    groupInput[y * 38 + x] = ((c == '*' || c == '0') ? 1 : 0);
                else
                    groupInput[y * 38 + x] = ((c == '*' || c == '1') ? 1 : 0);
            }
        }
        //for (int y0 = 0; y0 < 18; ++y0) {
        //    for (int y1 = 0; y1 < 18; ++y1) {
        //        bool compatible = true;
        //        for (int x = 0; x < 38; ++x) {
        //            if (groupInput[y0 * 38 + x] == 1 && groupInput[y1 * 38 + x] == 1) {
        //                compatible = false;
        //                break;
        //            }
        //        }
        //        if (compatible) {
        //            printf("%i and %i are compatible\n", y0, y1);
        //        }
        //    }
        //}
        //int groupYY[18] = { 1, 0,  3, 2,  4, 6,  5, 7,  11, 10,  12, 13,  8, 9,  15, 14,  16, 17 };
        //for (int y = 0; y < 18; y += 2) {
        //    for (int x = 0; x < 38; ++x) {
        //        int y0 = groupInput[groupYY[y] * 38 + x];
        //        int y1 = groupInput[groupYY[y + 1] * 38 + x];
        //        static const char c[] = "?01*";
        //        printf("%c", c[y0 + y1 * 2]);
        //    }
        //    printf("\n");
        //}


        //// S field in group 3 is always either s_blank or s_sigma - look for this pattern
        //for (int p = 0; p < 512 - 16; ++p) {
        //    for (int b = 0; b < 21; ++b) {
        //        int pp = 0;
        //        int cc = 0;
        //        for (int i = 0; i < 16; ++i) {
        //            int pi = groups[3]._instructions[i]._data[0] == s_blank ? 0 : 1;
        //            int ci = (instructions[p + i] & (1 << b)) == 0 ? 0 : 1;
        //            pp |= (pi << i);
        //            cc |= (ci << i);
        //        }
        //        cc ^= pp;
        //        int n = 0;
        //        for (int i = 0; i < 16; ++i)
        //            if ((cc & (1 << i)) != 0)
        //                ++n;
        //        if (n == 0 || n == 16) { //pp == cc || pp == (cc ^ 0xffff)) {
        //            printf("Instruction %03x bit %i\n", p, b);
        //        }
        //        if (n == 1 || n == 15) { //pp == cc || pp == (cc ^ 0xffff)) {
        //            printf("instruction %03x bit %i\n", p, b);
        //        }
        //    }
        //}

        //// Try to find group 2 (INCR, DECR, CORX)
        //for (int p = 0; p < 512 - 12; ++p) {
        //    bool found = true;
        //    for (int y = 0; y < 12; ++y) {
        //        int s = groups[2]._instructions[y]._data[0];
        //        if (s != s_blank && s != s_sigma)
        //            continue;
        //        int b = (instructions[p + y] & (1 << 7)) == 0 ? 0 : 1;
        //        if (!((b == 0 && s == s_blank) || (b == 1 && s == s_sigma)))
        //            found = false;
        //    }
        //    if (found) {
        //        printf("Group 2 at %03x\n", p);
        //    }
        //}

        //// Try to find group 7 (XLAT, RPTS)
        //int bestCount = 0;
        //int bestP = 0;
        //for (int p = 0; p < 512 - 10; ++p) {
        //    int count = 0;
        //    for (int y = 0; y < 10; ++y) {
        //        int s = groups[7]._instructions[y + 6]._data[0];
        //        if (s != s_blank && s != s_sigma)
        //            continue;
        //        int b = (instructions[p + y] & (1 << 7)) == 0 ? 0 : 1;
        //        if ((b == 0 && s == s_blank) || (b == 1 && s == s_sigma))
        //            ++count;
        //    }
        //    if (count >= bestCount) {
        //        bestCount = count;
        //        bestP = p;
        //        printf("Group 7 at %03x count %i\n", bestP, bestCount);
        //    }
        //}


        for (int i = 0; i < 512; ++i) {
            printf("%03x ", i);
            int d = instructions[i];
            for (int b = 0; b < 21; ++b) {
                if ((d & (1 << (20 - b))) != 0)
                    printf("%c", 'A' + b);
                else
                    printf(" ");
            }
            printf("       ");
            int s = ((d >> 13) & 1) + ((d >> 10) & 6) + ((d >> 11) & 0x18);
            int dd = ((d >> 20) & 1) + ((d >> 18) & 2) + ((d >> 16) & 4) + ((d >> 14) & 8) + ((d >> 12) & 0x10);
            int typ = (d >> 7) & 7;
            if ((typ & 4) == 0)
                typ >>= 1;

            if (d == 0) {
                printf("                                         ");
                goto afterDisassembly;
            }
            static const char* regNames[] = {
                "RA",  // ES
                "RC",  // CS
                "RS",  // SS - presumably, to fit pattern. Only used in RESET
                "RD",  // DS
                "PC",
                "IND",
                "OPR",
                "no dest",  // as dest only - source is Q
                "A",   // AL
                "C",   // CL? - not used
                "E",   // DL? - not used
                "L",   // BL? - not used
                "tmpa",
                "tmpb",
                "tmpc",
                "F",   // flags register
                "X",   // AH
                "B",   // CH? - not used
                "M",
                "R",   // source specified by modrm and direction, destination specified by r field of modrm
                "tmpaL",    // as dest only - source is SIGNA
                "tmpbL",    // as dest only - source is ONES
                "tmpaH",    // as dest only - source is CR
                "tmpbH",    // as dest only - source is ZERO
                "XA",  // AX
                "BC",  // CX
                "DE",  // DX
                "HL",  // BX
                "SP",  // SP
                "MP",  // BP
                "IJ",  // SI
                "IK",  // DI
            };
            if (s == 0x15 && dd == 0x07)  // "ONES  -> Q" used as no-op move
                printf("                ");
            else {
                const char* source = regNames[s];
                const char* dest = regNames[dd];

                switch (s) {
                    case 0x07: source = "Q"; break;
                    case 0x14: source = "SIGMA"; break;
                    case 0x15: source = "ONES"; break;
                    case 0x16: source = "CR"; break;  // low 3 bits of microcode address Counting Register + 1? Used as interrupt number at 0x198 (1), 0x199 (2), 0x1a7 (0), 0x1af (4), and 0x1b2 (3)
                    case 0x17: source = "ZERO"; break;
                }
                printf("%-5s -> %-7s", source, regNames[dd]);
            }
            printf("   ");

            if (typ == 4 && (d & 0x7f) == 0x7f) {
                printf("                  ");
                goto afterSecondHalf;
            }
            printf("%i   ", typ);
            switch (typ) {  // TYP bits
                case 0:
                case 5:
                case 7:
                    switch ((d >> 4) & 0xf) {
                        case 0x00: printf("F1ZZ"); break;
                        case 0x01: printf("MOD1"); break; // jump if short offset in effective address
                        case 0x02: printf("L8  "); break; // jump if short immediate (skip 2nd byte from Q)
                        case 0x03: printf("Z   "); break; // jump if zero (used in IMULCOF/MULCOF)
                        case 0x04: printf("NCZ "); break;
                        case 0x05: printf("TEST"); break; // jump if -TEST pin not asserted
                        case 0x06: printf("OF  "); break; // jump if overflow flag is set
                        case 0x07: printf("CY  "); break;
                        case 0x08: printf("UNC "); break;
                        case 0x09: printf("NF1 "); break;
                        case 0x0a: printf("NZ  "); break; // jump if not zero (used in JCXZ and LOOP)
                        case 0x0b: printf("X0  "); break; // jump if bit 3 of opcode is 1
                        case 0x0c: printf("NCY "); break;
                        case 0x0d: printf("F1  "); break;
                        case 0x0e: printf("INT "); break; // jump if interrupt is pending
                        case 0x0f: printf("XC  "); break;  // jump if condition based on low 4 bits of opcode
                    }
                    printf("  ");
                    if (typ == 5) {
                        switch (d & 0xf) {
                            case 0: printf("FARCALL "); break;
                            case 1: printf("NEARCALL"); break;
                            case 2: printf("RELJMP  "); break;
                            case 3: printf("EAOFFSET"); break;
                            case 4: printf("EAFINISH"); break;
                            case 5: printf("FARCALL2"); break;
                            case 6: printf("INTR    "); break;
                            case 7: printf("INT0    "); break;
                            case 8: printf("RPTI    "); break;
                            case 9: printf("AAEND   "); break;
                        }
                    }
                    else {
                        if (typ == 7) {
                            switch (d & 0xf) {
                                case 0: printf("FARRET  "); break;
                                case 1: printf("RPTS    "); break;
                                case 2: printf("CORX    "); break; // unsigned multiply tmpc and tmpb, result in tmpa:tmpc
                                case 3: printf("CORD    "); break; // unsigned divide tmpa:tmpc by tmpb, quotient in ~tmpc, remainder in tmpa
                                case 4: printf("PREIMUL "); break; // abs tmpc and tmpb, invert F1 if product negative
                                case 5: printf("NEGATE  "); break; // negate product tmpa:tmpc 
                                case 6: printf("IMULCOF "); break; // clear carry and overflow flags if product of signed multiply fits in low part, otherwise set them
                                case 7: printf("MULCOF  "); break; // clear carry and overflow flags if product of unsigned multiply fits in low part, otherwise set them
                                case 8: printf("PREIDIV "); break; // abs tmpa:tmpc and tmpb, invert F1 if one or the other but not both were negative
                                case 9: printf("POSTIDIV"); break; // negate ~tmpc if F1 set
                            }
                        }
                        else
                            printf("%4i    ", d & 0xf);
                    }
                    break;
                case 4:
                    switch ((d >> 3) & 0x0f) {  // od14 od15 od16 od17   d3 d4 d5 d6
                        case 0x00: printf("MAXC "); break;
                        case 0x01: printf("FLUSH"); break;
                        case 0x02: printf("CF1  "); break;
                        case 0x03: printf("CITF "); break;  // clear interrupt and trap flags
                        case 0x04: printf("RCY  "); break;  // reset carry
                        case 0x06: printf("CCOF "); break;  // clear carry and overflow
                        case 0x07: printf("SCOF "); break;  // set carry and overflow
                        case 0x08: printf("WAIT "); break;  // not sure what this does
                        case 0x0f: printf("none "); break;
                    }
                    printf(" ");
                    switch (d & 7) {  // od18 od19 od20  d0 d1 d2
                        case 0: printf("RNI     "); break;
                        case 1: printf("WB,NX   "); break;  // possible write back to EA
                        case 2: printf("CORR    "); break;
                        case 3: printf("SUSP    "); break;
                        case 4: printf("RTN     "); break;
                        case 5: printf("NX      "); break;
                        case 7: printf("none    "); break;
                    }
                    break;
                case 1:
                    {
                        switch ((d >> 3) & 0x1f) {  // od13 od14 od15 od16 od17   d3 d4 d5 d6 d7
                            case 0x00: printf("ADD "); break;
                            case 0x01: printf("OR  "); break; // not used in microcode
                            case 0x02: printf("ADC "); break;
                            case 0x03: printf("SBB "); break; // not used in microcode
                            case 0x04: printf("AND "); break; // 0x09e
                            case 0x05: printf("SUBT"); break;
                            case 0x06: printf("XOR "); break; // not used in microcode
                            case 0x07: printf("CMP "); break; // not used in microcode
                            case 0x08: printf("ROL "); break; // not used in microcode
                            case 0x09: printf("ROR "); break; // not used in microcode
                            case 0x0a: printf("LRCY"); break;
                            case 0x0b: printf("RRCY"); break;
                            case 0x0c: printf("SHL "); break; // not used in microcode
                            case 0x0d: printf("SHR "); break; // not used in microcode
                            case 0x0e: printf("SETMO"); break; // not used in microcode
                            case 0x0f: printf("SAR "); break; // not used in microcode
                            case 0x10: printf("PASS"); break;
                            case 0x11: printf("XI  "); break;
                            case 0x14: printf("DAA "); break; // not used in microcode
                            case 0x15: printf("DAS "); break; // not used in microcode
                            case 0x16: printf("AAA "); break; // not used in microcode
                            case 0x17: printf("AAS "); break; // not used in microcode
                            case 0x18: printf("INC "); break;
                            case 0x19: printf("DEC "); break;
                            case 0x1a: printf("COM1"); break;
                            case 0x1b: printf("NEG "); break;
                            case 0x1c: printf("INC2"); break;
                            case 0x1d: printf("DEC2"); break;
                        }
                    }
                    printf("  ");
                    switch (d & 7) {
                        case 0x00: printf("tmpa    "); break;
                        case 0x01: printf("tmpa, NX"); break;
                        case 0x02: printf("tmpb    "); break;
                        case 0x03: printf("tmpb, NX"); break;
                        case 0x04: printf("tmpc    "); break;
                        case 0x05: printf("tmpc, NX"); break;  // Not in patent, filling in the pattern
                    }
                    break;
                case 6:
                    switch ((d >> 4) & 7) {
                        case 0x00: printf("R    "); break;
                        case 0x02: printf("IRQ  "); break;
                        case 0x04: printf("w    "); break;
                        case 0x05: printf("W,RNI"); break;
                    }
                    printf(" ");
                    switch ((d >> 2) & 3) {
                        case 0x00: printf("DA,"); break;  // ES
                        case 0x01: printf("D0,"); break;  // segment 0
                        case 0x02: printf("DS,"); break;  // SS
                        case 0x03: printf("DD,"); break;  // DS
                    }
                    switch (d & 3) {
                        case 0x00: printf("P2"); break;  // Increment IND by 2
                        case 0x01: printf("BL"); break;  // Adjust IND according to word size and DF
                        case 0x02: printf("M2"); break;  // Decrement IND by 2
                        case 0x03: printf("P0"); break;  // Don't adjust IND
                    }
                    printf("   ");
                    break;
            }
            afterSecondHalf:
            printf(" ");
            if (((d >> 10) & 1) != 0)
                printf("F");
            else
                printf(" ");
            printf("  ");
            afterDisassembly:
            if (i % 4 == 0 && stage1[i >> 2] != 0) {
                int ba[] = {7, 2, 1, 0, 5, 6, 8, 9, 10, 3, 4};

                int s1 = stage1[i >> 2];
                for (int b = 0; b < 11; ++b) {
                    int x = (s1 >> (ba[b] * 2)) & 3;
                    printf("%c", "?10*"[x]);
                    if (b == 8)
                        printf(".");
                }
                //int group = '0' + ((s1 >> (3 * 2)) & 1)*2 + ((s1 >> (4 * 2)) & 1);
                //printf(" %c%c", group, "?so*"[(s1 >> (7 * 2)) & 3]);
            }
            else
                printf("            ");
            printf("  ");
            //bool jump = false;
            //switch ((d >> 12) & 0x1f) {
            //    case 0x1c:
            //        if ((d & 0x800) != 0)
            //            printf("CY  ");
            //        else
            //            printf("    ");
            //        jump = true;
            //        break;
            //    case 0x01: printf("ADD "); break;
            //    case 0x15:
            //        if ((d & 0x20000) != 0)
            //            printf("RRCY");
            //        else
            //            printf("LRCY");
            //        break;
            //        break;
            //    case 0x00: printf("MAXC"); break;
            //    case 0x06: printf("NCY "); jump = true; break;
            //    case 0x04: printf("NCZ "); jump = true; break;
            //    case 0x02: printf("UNC "); jump = true; break;
            //    case 0x08: printf("RCY "); break;
            //    case 0x09: printf("SUBT"); break;
            //    case 0x03: printf("XI  "); break;
            //    default: printf("%4x", (d >> 12) & 0x1f); break;
            //}
            //printf(" ");
            //else {
            //    switch ((d >> 18) & 7) {
            //    case 7: printf("   ");
            //    }
            //}
            if (i % 4 == 0) {
                const char* s[] = {
                    "MOV rm<->r",
                    "LEA",
                    "alu rm<->r",
                    "alu rm,i",
                    "",
                    "MOV rm,i",
                    "alu A,i",
                    "MOV r,i",
                    " INC/DEC rm",
                    " PUSH rm",
                    "PUSH rw",
                    "PUSH sr",
                    "PUSHF",
                    "POP rw",
                    "POP sr",
                    "POPF",
                    "POP rmw",
                    "",
                    "",
                    " NOT rm",
                    " NEG rm",
                    "CBW",
                    "CWD",
                    "",
                    "MOV A,[i]",
                    "MOV [i],A",
                    " CALL FAR rm",
                    "",
                    "CALL cd",
                    " CALL rm",
                    "",
                    "CALL cw",
                    "",
                    "XCHG AX,rw",
                    "rot rm,1",
                    "rot rm,CL",
                    "",
                    "TEST rm,r",
                    " TEST rm,i",
                    "TEST A,i",
                    "SALC",
                    "XCHG rm,r",
                    "",
                    "IN A,ib",
                    "OUT ib,A",
                    "IN A,DX",
                    "OUT DX,A",
                    "RET",
                    "RETF",
                    "",
                    "IRET",
                    "RET/RETF iw",
                    "JMP cw/JMP cb",
                    "",
                    " JMP rm",
                    " JMP FAR rm",
                    "JMP cd",
                    "",
                    "Jcond cb",
                    "MOV rmw<->sr",
                    "LES",
                    "LDS",
                    "WAIT",
                    "",
                    "SAHF",
                    "LAHF",
                    "ESC",
                    "XLAT",
                    "",
                    "",
                    "",
                    "STOS",
                    "CMPS/SCAS",
                    "",
                    "",
                    "MOVS/LODS",
                    "",
                    "JCXZ",
                    "LOOPNE/LOOPE",
                    "",
                    "LOOP",
                    "DAA/DAS",
                    "AAA/AAS",
                    "",
                    " iMUL rmb",
                    "",
                    " iMUL rmw",
                    "",
                    " iDIV rmb",
                    "",
                    " iDIV rmw",
                    "",
                    "AAD",
                    "AAM",
                    "",
                    "INC/DEC",
                    "",
                    "",
                    "",
                    "",
                    "",
                    "",
                    "",
                    "",
                    "",
                    "",
                    "INT ib",
                    "INTO",
                    "INT 3",
                    "",
                    "",
                    "",
                    "",
                    "",
                    "",
                    "",
                    "",
                    "",
                    "",
                    "",
                    "",
                    "",
                    "",
                    "WAIT continued",
                    "STOS continued",
                    "CMPS/SCAS continued",
                    "MOVS/LODS continued",
                    "JCXZ continued"};
                printf("%s", s[i / 4]);
            }
            switch (i) {
                case 0x06b: printf("FARCALL"); break;
                case 0x077: printf("NEARCALL"); break;
                case 0x0c2: printf("FARRET"); break;
                case 0x0d2: printf("RELJMP"); break;
                case 0x1d4: printf("[BX+SI]"); break;
                case 0x1da: printf("[BX+DI]"); break;

                case 0x1db: printf("[BP+SI]"); break;
                case 0x1d7: printf("[BP+DI]"); break;
                case 0x003: printf("[SI]"); break;
                case 0x01f: printf("[DI]"); break;
                case 0x1dc: printf("[iw]"); break;
                case 0x023: printf("[BP]"); break;
                case 0x037: printf("[BX]"); break;

                case 0x1de: printf("[i]"); break;
                case 0x1e1: printf("EALOAD"); break;
                case 0x1e3: printf("EADONE"); break;
                case 0x06c: printf("FARCALL2"); break;

                case 0x19d: printf("INTR"); break;
                case 0x112: printf("RPTS"); break;
                case 0x1a7: printf("INT0"); break;
                case 0x17f: printf("CORX"); break;
                case 0x188: printf("CORD"); break;
                case 0x1c0: printf("PREIMUL"); break;
                case 0x1b6: printf("NEGATE"); break;
                case 0x1cd: printf("IMULCOF"); break;

                case 0x1d2: printf("MULCOF"); break;
                case 0x1b4: printf("PREIDIV"); break;
                case 0x1c4: printf("POSTIDIV"); break;
                case 0x118: printf("RPTI"); break;
                case 0x179: printf("AAEND"); break;

                case 0x198: printf("INT1"); break;
                case 0x199: printf("INT2"); break;
                case 0x19a: printf("IRQ "); break;
                case 0x1e4: printf("RESET"); break;

            }
            printf("\n");
            // printf("%06x\n", instructions[i]);
        }
    }
};