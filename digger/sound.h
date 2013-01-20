/* Digger Remastered
   Copyright (c) Andrew Jenner 1998-2004 */

void initsound(void);
void soundstop(void);
void music(Sint4 tune);
void musicoff(void);
void soundlevdone(void);
void sound1up(void);
void soundpause(void);
void soundpauseoff(void);
void setsoundt2(void);
void sett2val(Sint4 t2v);
void startint8(void);
void stopint8(void);
void soundbonus(void);
void soundbonusoff(void);
void soundfire(int n);
void soundexplode(int n);
void soundfireoff(int n);
void soundem(void);
void soundemerald(int emn);
void soundeatm(void);
void soundddie(void);
void soundwobble(void);
void soundwobbleoff(void);
void soundfall(void);
void soundfalloff(void);
void soundbreak(void);
void soundgold(void);

void soundint(void);

/*
void soundoff(void);
void timer2(Uint4 t2v);
*/

extern bool soundflag,musicflag;
extern Sint4 volume,timerrate;
extern Uint4 timercount;

extern void (*setupsound)(void);
extern void (*killsound)(void);
extern void (*fillbuffer)(void);
extern void (*initint8)(void);
extern void (*restoreint8)(void);
extern void (*soundoff)(void);
extern void (*setspkrt2)(void);
extern void (*settimer0)(Uint4 t0v);
extern void (*timer0)(Uint4 t0v);
extern void (*settimer2)(Uint4 t2v);
extern void (*timer2)(Uint4 t2v);
extern void (*soundkillglob)(void);

