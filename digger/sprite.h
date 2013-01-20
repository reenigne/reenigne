/* Digger Remastered
   Copyright (c) Andrew Jenner 1998-2004 */

void setretr(bool f);
void movedrawspr(Sint4 n,Sint4 x,Sint4 y);
void erasespr(Sint4 n);
void createspr(Sint4 n,Sint4 ch,Uint3 *mov,Sint4 wid,Sint4 hei,Sint4 bwid,
               Sint4 bhei);
void initspr(Sint4 n,Sint4 ch,Sint4 wid,Sint4 hei,Sint4 bwid,Sint4 bhei);
void drawspr(Sint4 n,Sint4 x,Sint4 y);
void initmiscspr(Sint4 x,Sint4 y,Sint4 wid,Sint4 hei);
void getis(void);
void drawmiscspr(Sint4 x,Sint4 y,Sint4 ch,Sint4 wid,Sint4 hei);

extern void (*ginit)(void);
extern void (*gclear)(void);
extern void (*gpal)(Sint4 pal);
extern void (*ginten)(Sint4 inten);
extern void (*gputi)(Sint4 x,Sint4 y,Uint3 *p,Sint4 w,Sint4 h);
extern void (*ggeti)(Sint4 x,Sint4 y,Uint3 *p,Sint4 w,Sint4 h);
extern void (*gputim)(Sint4 x,Sint4 y,Sint4 ch,Sint4 w,Sint4 h);
extern Sint4 (*ggetpix)(Sint4 x,Sint4 y);
extern void (*gtitle)(void);
extern void (*gwrite)(Sint4 x,Sint4 y,Sint4 ch,Sint4 c);

extern int first[TYPES],coll[SPRITES];
