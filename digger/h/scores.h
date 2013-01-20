/* Digger Remastered
   Copyright (c) Andrew Jenner 1998-2004 */

void loadscores(void);
void showtable(void);
void zeroscores(void);
void writecurscore(int col);
void drawscores(void);
void initscores(void);
void endofgame(void);
void scorekill(int n);
void scorekill2(void);
void scoreemerald(int n);
void scoreoctave(int n);
void scoregold(int n);
void scorebonus(int n);
void scoreeatm(int n,int msc);
void addscore(int n,Sint4 score);

#ifdef INTDRF
Sint5 getscore0(void);
#endif

extern Uint4 bonusscore;
extern Sint5 scoret;

extern char scoreinit[11][4];
