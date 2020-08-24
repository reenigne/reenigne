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
                for (int yy = 0; yy < h; ++yy) {
                    int ib = y * 24 + yy;
                    for (int xx = 0; xx < 64; ++xx) {
                        int b = (s[yy * 66 + (63 - xx)] == '0' ? 1 : 0);
                        if (b != 0) {
                            instructions[xx * 8 + half * 4 + yy % 4] |=
                                1 << (ib >> 2);
                        }
                    }
                }
            }
        }

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
                if ((d & (1 << b)) != 0)
                    printf("%c", 'A' + b);
                else
                    printf(" ");
            }
            if (d == 0) {
                printf("\n");
                continue;
            }
            printf("       ");
            int s = (d >> 5) & 0x1f;
            int dd = d & 0x1f;
            switch (s) {
                case 0x01: printf("X    "); break;
                case 0x02: printf("A    "); break;
                case 0x03: printf("XA   "); break;
                case 0x08: printf("PC   "); break;
                case 0x09: printf("SIGMA"); break;
                case 0x0c: printf("IND  "); break;
                case 0x0d:
                    if (dd == 0x07)
                        printf("     ");
                    else
                        printf("ONES ");
                    break;
                case 0x11: printf("M    "); break;
                case 0x17: printf("HL   "); break;
                case 0x18: printf("OPR  "); break;
                case 0x1b: printf("IJ   "); break;
                case 0x1c: printf("Q    "); break;
                case 0x1d: printf("ZERO "); break;
                case 0x1f: printf("IK   "); break;
                default: printf("[%3x]", s); break;
            }
            if (s != 0x0d || dd != 7)
                printf(" -> ");
            else
                printf("    ");
            switch (dd) {
                case 0x05: printf("IND    "); break;
                case 0x06: printf("OPR    "); break;
                case 0x07:
                    if (s == 0xd)
                        printf("       ");
                    else
                        printf("no dest");
                    break;
                case 0x08: printf("A      "); break;
                case 0x0c: printf("tmpa   "); break;
                case 0x0d: printf("tmpb   "); break;
                case 0x0e: printf("tmpc   "); break;
                case 0x10: printf("X      "); break;
                case 0x12: printf("M      "); break;
                case 0x14: printf("tmpaL  "); break;
                case 0x19: printf("M-19   "); break;
                case 0x1e: printf("IJ     "); break;
                case 0x1f: printf("IK     "); break;
                default: printf("[%2x]   ", dd); break;
            }
            printf("   ");

            int typ = (d >> 11) & 7;
            switch (typ) {  // TYP bits
                case 0:
                case 4:
                case 5:
                case 7:
                    if (typ == 5)
                        printf("5   ");
                    else {
                        if (typ == 7)
                            printf("7   ");
                        else
                            printf("0   ");
                    }

                    switch ((d >> 13) & 0x0f) {
                        case 0x00: printf("F1ZZ"); break;
                        case 0x01: printf("UNC "); break;
                        case 0x02: printf("NCZ "); break;
                        case 0x03: printf("NCY "); break;
                        case 0x05: printf("F1-5"); break;
                        case 0x09: printf("F1-9"); break;
                        case 0x0b: printf("F1-B"); break;
                        case 0x0d: printf("X0  "); break;
                        case 0x0e: printf("CY  "); break;
                        default: printf("[%2x]", (d >> 13) & 0x0f); break;
                    }
                    printf("  ");
                    printf("%4i    ", ((d >> 14) & 8) + ((d >> 16) & 4) + ((d >> 18) & 2) + ((d >> 20) & 1));
                    break;
                case 1:
                    if (((d >> 14) & 0x0f) == 0x0f && ((d >> 18) & 7) == 0x07) {
                        printf("                  ");
                        break;
                    }
                    printf("4   ");
                    switch ((d >> 14) & 0x0f) {
                        case 0x00: printf("MAXC "); break;
                        case 0x02: printf("RCY  "); break;
                        case 0x08: printf("FLUSH"); break;
                        case 0x0f: printf("none "); break;
                        default: printf("[%2x] ", (d >> 14) & 0x0f); break;
                    }
                    printf(" ");
                    switch ((d >> 18) & 7) {
                        case 0x00: printf("RNI     "); break;
                        case 0x01: printf("RTN     "); break;
                        case 0x07: printf("none    "); break;
                        default:  printf("[%2x]    ", (d >> 18) & 0x07); break;
                    }
                    break;
                case 2:
                case 6:
                    printf("1   ");
                    switch ((d >> 13) & 0x1f) {
                        case 0x00: printf("ADD "); break;
                        case 0x01: printf("PASS"); break;
                        case 0x03: printf("INC "); break;
                        case 0x0a: printf("LRCY"); break;
                        case 0x0b: printf("COM1"); break;
                        case 0x11: printf("XI  "); break;
                        case 0x13: printf("DEC "); break;
                        case 0x14: printf("SUBT"); break;
                        case 0x17: printf("DEC2"); break;
                        case 0x1a: printf("RRCY"); break;
                        default: printf("[%2x]", (d >> 13) & 0x1f); break;
                    }
                    printf("  ");
                    switch ((d >> 18) & 7) {
                        case 0x00: printf("tmpa    "); break;
                        case 0x01: printf("tmpc    "); break;
                        case 0x02: printf("tmpb    "); break;
                        case 0x03: printf("tmpb?   "); break;
                        case 0x04: printf("tmpa, NX"); break;
                        case 0x05: printf("tmpc, NX"); break;  // Not in patent, filling in the pattern
                        case 0x06: printf("tmpb, NX"); break;
                        case 0x07: printf("tmpb?,NX"); break;  // Not in patent, filling in the pattern
                        default: printf("[%2x]    ", (d >> 18) & 7); break;
                    }
                    break;
                case 3:
                    printf("6   ");
                    switch ((d >> 14) & 0x07) {
                        case 0x00: printf("R   "); break;
                        case 0x01: printf("w   "); break;
                        default: printf("[%2x]", (d >> 14) & 0x07); break;
                    }
                    printf("  ");
                    switch ((d >> 17) & 0x0f) {
                        case 0x07: printf("DD,P0   "); break;
                        case 0x08: printf("DA,BL   "); break;
                        case 0x0b: printf("DD,BL   "); break;
                        default:  printf("[%2x]    ", (d >> 18) & 0x07); break;
                    }
                    break;
            }
            printf(" ");
            if (((d >> 10) & 1) != 0)
                printf("F");
            else
                printf(" ");
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
            switch (i) {
                case 0x10c: printf("XLAT: (0xd7)"); break;
                case 0x112: printf("RPTS"); break;
                case 0x11c: printf("STS: (0xaa, 0xab)"); break;
                case 0x120: printf("CMPS/SRCHS: (0xa6, 0xa7, 0xae, 0xaf)"); break;
                case 0x12c: printf("MOVS/LDS: (0xa4, 0xa5, 0xac, 0xad)"); break;
                case 0x148: printf("AAA/AAS: (0x37, 0x3f)"); break;
                case 0x174: printf("AAM: (0xd4)"); break;
                case 0x17c: printf("INCR/DECR (0x40-0x4f)"); break;
                case 0x17f: printf("CORX"); break;
                case 0x188: printf("CORD"); break;


            }
            printf("\n");
            // printf("%06x\n", instructions[i]);
        }
    }
};