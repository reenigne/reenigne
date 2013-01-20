/* Digger Remastered
   Copyright (c) Andrew Jenner 1998-2004 */

void dodigger(void);
void erasediggers(void);
void killfire(int n);
void erasebonus(void);
Sint4 countem(void);
void makeemfield(void);
void drawemeralds(void);
void initdigger(void);
void drawexplosion(int n);
void updatefire(int n);
void createbonus(void);
Sint4 reversedir(Sint4 d);
bool hitemerald(Sint4 x,Sint4 y,Sint4 rx,Sint4 ry,Sint4 dir);
void killdigger(int n,Sint4 bp6,Sint4 bp8);
bool checkdiggerunderbag(Sint4 h,Sint4 v);
void killemerald(Sint4 bpa,Sint4 bpc);
void newframe(void);
int diggerx(int n);
int diggery(int n);
void digresettime(int n);
void sceatm(int n);
bool isalive(void);
bool digalive(int n);
int getlives(int pl);
void addlife(int pl);
void initlives(void);
void declife(int pl);

extern bool bonusvisible,digonscr,bonusmode;
extern Uint5 ftime,curtime,cgtime;

#ifdef INTDRF
extern Uint5 frame;
#endif
