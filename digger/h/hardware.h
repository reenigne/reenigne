/* Digger Remastered
   Copyright (c) Andrew Jenner 1998-2004 */

void olddelay(Sint4 t);
Sint5 getkips(void);
void inittimer(void);
Uint5 gethrt(void);
Sint5 getlrt(void);

void s0initint8(void);
void s0restoreint8(void);
void s0soundoff(void);
void s0setspkrt2(void);
void s0settimer0(Uint4 t0v);
void s0timer0(Uint4 t0v);
void s0settimer2(Uint4 t2v);
void s0timer2(Uint4 t2v);
void s0soundinitglob(void);
void s0soundkillglob(void);
void s1initint8(void);
void s1restoreint8(void);

void initkeyb(void);
void restorekeyb(void);
Sint4 getkey(void);
bool kbhit(void);

void graphicsoff(void);
void gretrace(void);

void cgainit(void);
void cgaclear(void);
void cgapal(Sint4 pal);
void cgainten(Sint4 inten);
void cgaputi(Sint4 x,Sint4 y,Uint3 *p,Sint4 w,Sint4 h);
void cgageti(Sint4 x,Sint4 y,Uint3 *p,Sint4 w,Sint4 h);
void cgaputim(Sint4 x,Sint4 y,Sint4 ch,Sint4 w,Sint4 h);
Sint4 cgagetpix(Sint4 x,Sint4 y);
void cgawrite(Sint4 x,Sint4 y,Sint4 ch,Sint4 c);
void cgatitle(void);

void vgainit(void);
void vgaclear(void);
void vgapal(Sint4 pal);
void vgainten(Sint4 inten);
void vgaputi(Sint4 x,Sint4 y,Uint3 *p,Sint4 w,Sint4 h);
void vgageti(Sint4 x,Sint4 y,Uint3 *p,Sint4 w,Sint4 h);
void vgaputim(Sint4 x,Sint4 y,Sint4 ch,Sint4 w,Sint4 h);
Sint4 vgagetpix(Sint4 x,Sint4 y);
void vgawrite(Sint4 x,Sint4 y,Sint4 ch,Sint4 c);
void vgatitle(void);
