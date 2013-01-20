/* Digger Remastered
   Copyright (c) Andrew Jenner 1998-2004 */

#include "device.h"

void soundinitglob(int port,int irq,int dma,Uint4 bufsize,Uint4 samprate);
void s1setupsound(void);
void s1killsound(void);
void s1fillbuffer(void);
void s1settimer2(Uint4 t2);
void s1soundoff(void);
void s1setspkrt2(void);
void s1settimer0(Uint4 t0);
void s1timer0(Uint4 t0);
void s1timer2(Uint4 t2);

extern samp (*getsample)(void);
samp getsample1(void);

void s2settimer2(Uint4 t2);
void s2soundoff(void);
void s2setspkrt2(void);
void s2settimer0(Uint4 t0);
void s2timer0(Uint4 t0);
void s2timer2(Uint4 t2);

samp getsample2(void);

void inits2(void);
void finishs2(void);

bool addcarry(Uint4 *dest,Uint4 add);
