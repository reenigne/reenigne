/* Digger Remastered
   Copyright (c) Andrew Jenner 1998-2004 */

int mouseinit(void);
void mouseshow(void);
void mousehide(void);
int mousepos(int *x,int *y);
void mouseput(int x,int y);
void mousebox(int left,int top,int right,int bottom);
void vgaline(int x0,int y0,int x1,int y1,int c,int m);
void cgaline(int x0,int y0,int x1,int y1,int c,int m);

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
void cgawrite(Sint4 x,Sint4 y,Sint4 ch,Sint4 c);

void vgainit(void);
void vgaclear(void);
void vgapal(Sint4 pal);
void vgainten(Sint4 inten);
void vgaputi(Sint4 x,Sint4 y,Uint3 *p,Sint4 w,Sint4 h);
void vgageti(Sint4 x,Sint4 y,Uint3 *p,Sint4 w,Sint4 h);
void vgaputim(Sint4 x,Sint4 y,Sint4 ch,Sint4 w,Sint4 h);
void vgawrite(Sint4 x,Sint4 y,Sint4 ch,Sint4 c);
