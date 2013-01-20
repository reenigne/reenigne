/* Digger Remastered
   Copyright (c) Andrew Jenner 1998-2004 */

#include <stdlib.h>
#include "def.h"
#include "sprite.h"
#include "hardware.h"

bool retrflag=true;

bool sprrdrwf[SPRITES+1];
bool sprrecf[SPRITES+1];
bool sprenf[SPRITES];
Sint4 sprch[SPRITES+1];
Uint3 *sprmov[SPRITES];
Sint4 sprx[SPRITES+1];
Sint4 spry[SPRITES+1];
Sint4 sprwid[SPRITES+1];
Sint4 sprhei[SPRITES+1];
Sint4 sprbwid[SPRITES];
Sint4 sprbhei[SPRITES];
Sint4 sprnch[SPRITES];
Sint4 sprnwid[SPRITES];
Sint4 sprnhei[SPRITES];
Sint4 sprnbwid[SPRITES];
Sint4 sprnbhei[SPRITES];

void clearrdrwf(void);
void clearrecf(void);
void setrdrwflgs(Sint4 n);
bool collide(Sint4 bx,Sint4 si);
bool bcollide(Sint4 bx,Sint4 si);
void putims(void);
void putis(void);
void bcollides(int bx);

void (*ginit)(void)=vgainit;
void (*gclear)(void)=vgaclear;
void (*gpal)(Sint4 pal)=vgapal;
void (*ginten)(Sint4 inten)=vgainten;
void (*gputi)(Sint4 x,Sint4 y,Uint3 *p,Sint4 w,Sint4 h)=vgaputi;
void (*ggeti)(Sint4 x,Sint4 y,Uint3 *p,Sint4 w,Sint4 h)=vgageti;
void (*gputim)(Sint4 x,Sint4 y,Sint4 ch,Sint4 w,Sint4 h)=vgaputim;
Sint4 (*ggetpix)(Sint4 x,Sint4 y)=vgagetpix;
void (*gtitle)(void)=vgatitle;
void (*gwrite)(Sint4 x,Sint4 y,Sint4 ch,Sint4 c)=vgawrite;

void setretr(bool f)
{
#ifdef INTDRF
  retrflag=false;
#else
  retrflag=f;
#endif
}

void createspr(Sint4 n,Sint4 ch,Uint3 *mov,Sint4 wid,Sint4 hei,Sint4 bwid,
               Sint4 bhei)
{
  sprnch[n]=sprch[n]=ch;
  sprmov[n]=mov;
  sprnwid[n]=sprwid[n]=wid;
  sprnhei[n]=sprhei[n]=hei;
  sprnbwid[n]=sprbwid[n]=bwid;
  sprnbhei[n]=sprbhei[n]=bhei;
  sprenf[n]=false;
}

void movedrawspr(Sint4 n,Sint4 x,Sint4 y)
{
  sprx[n]=x&-4;
  spry[n]=y;
  sprch[n]=sprnch[n];
  sprwid[n]=sprnwid[n];
  sprhei[n]=sprnhei[n];
  sprbwid[n]=sprnbwid[n];
  sprbhei[n]=sprnbhei[n];
  clearrdrwf();
  setrdrwflgs(n);
  putis();
  ggeti(sprx[n],spry[n],sprmov[n],sprwid[n],sprhei[n]);
  sprenf[n]=true;
  sprrdrwf[n]=true;
  putims();
}

void erasespr(Sint4 n)
{
  if (!sprenf[n])
    return;
  gputi(sprx[n],spry[n],sprmov[n],sprwid[n],sprhei[n]);
  sprenf[n]=false;
  clearrdrwf();
  setrdrwflgs(n);
  putims();
}

void drawspr(Sint4 n,Sint4 x,Sint4 y)
{
  Sint4 t1,t2,t3,t4;
  x&=-4;
  clearrdrwf();
  setrdrwflgs(n);
  t1=sprx[n];
  t2=spry[n];
  t3=sprwid[n];
  t4=sprhei[n];
  sprx[n]=x;
  spry[n]=y;
  sprwid[n]=sprnwid[n];
  sprhei[n]=sprnhei[n];
  clearrecf();
  setrdrwflgs(n);
  sprhei[n]=t4;
  sprwid[n]=t3;
  spry[n]=t2;
  sprx[n]=t1;
  sprrdrwf[n]=true;
  putis();
  sprenf[n]=true;
  sprx[n]=x;
  spry[n]=y;
  sprch[n]=sprnch[n];
  sprwid[n]=sprnwid[n];
  sprhei[n]=sprnhei[n];
  sprbwid[n]=sprnbwid[n];
  sprbhei[n]=sprnbhei[n];
  ggeti(sprx[n],spry[n],sprmov[n],sprwid[n],sprhei[n]);
  putims();
  bcollides(n);
}

void initspr(Sint4 n,Sint4 ch,Sint4 wid,Sint4 hei,Sint4 bwid,Sint4 bhei)
{
  sprnch[n]=ch;
  sprnwid[n]=wid;
  sprnhei[n]=hei;
  sprnbwid[n]=bwid;
  sprnbhei[n]=bhei;
}

void initmiscspr(Sint4 x,Sint4 y,Sint4 wid,Sint4 hei)
{
  sprx[SPRITES]=x;
  spry[SPRITES]=y;
  sprwid[SPRITES]=wid;
  sprhei[SPRITES]=hei;
  clearrdrwf();
  setrdrwflgs(SPRITES);
  putis();
}

void getis(void)
{
  Sint4 i;
  for (i=0;i<SPRITES;i++)
    if (sprrdrwf[i])
      ggeti(sprx[i],spry[i],sprmov[i],sprwid[i],sprhei[i]);
  putims();
}

void drawmiscspr(Sint4 x,Sint4 y,Sint4 ch,Sint4 wid,Sint4 hei)
{
  sprx[SPRITES]=x&-4;
  spry[SPRITES]=y;
  sprch[SPRITES]=ch;
  sprwid[SPRITES]=wid;
  sprhei[SPRITES]=hei;
  gputim(sprx[SPRITES],spry[SPRITES],sprch[SPRITES],sprwid[SPRITES],
         sprhei[SPRITES]);
}

void clearrdrwf(void)
{
  Sint4 i;
  clearrecf();
  for (i=0;i<SPRITES+1;i++)
    sprrdrwf[i]=false;
}

void clearrecf(void)
{
  Sint4 i;
  for (i=0;i<SPRITES+1;i++)
    sprrecf[i]=false;
}

void setrdrwflgs(Sint4 n)
{
  Sint4 i;
  if (!sprrecf[n]) {
    sprrecf[n]=true;
    for (i=0;i<SPRITES;i++)
      if (sprenf[i] && i!=n) {
        if (collide(i,n)) {
          sprrdrwf[i]=true;
          setrdrwflgs(i);
        }
      }
  }
}

bool collide(Sint4 bx,Sint4 si)
{
  if (sprx[bx]>=sprx[si]) {
    if (sprx[bx]>(sprwid[si]<<2)+sprx[si]-1)
      return false;
  }
  else
    if (sprx[si]>(sprwid[bx]<<2)+sprx[bx]-1)
      return false;
  if (spry[bx]>=spry[si]) {
    if (spry[bx]<=sprhei[si]+spry[si]-1)
      return true;
    return false;
  }
  if (spry[si]<=sprhei[bx]+spry[bx]-1)
    return true;
  return false;
}

bool bcollide(Sint4 bx,Sint4 si)
{
  if (sprx[bx]>=sprx[si]) {
    if (sprx[bx]+sprbwid[bx]>(sprwid[si]<<2)+sprx[si]-sprbwid[si]-1)
      return false;
  }
  else
    if (sprx[si]+sprbwid[si]>(sprwid[bx]<<2)+sprx[bx]-sprbwid[bx]-1)
      return false;
  if (spry[bx]>=spry[si]) {
    if (spry[bx]+sprbhei[bx]<=sprhei[si]+spry[si]-sprbhei[si]-1)
      return true;
    return false;
  }
  if (spry[si]+sprbhei[si]<=sprhei[bx]+spry[bx]-sprbhei[bx]-1)
    return true;
  return false;
}

void putims(void)
{
  int i;
  for (i=0;i<SPRITES;i++)
    if (sprrdrwf[i])
      gputim(sprx[i],spry[i],sprch[i],sprwid[i],sprhei[i]);
}

void putis(void)
{
  int i;
  for (i=0;i<SPRITES;i++)
    if (sprrdrwf[i])
      gputi(sprx[i],spry[i],sprmov[i],sprwid[i],sprhei[i]);
}

int first[TYPES],coll[SPRITES];
int firstt[TYPES]={FIRSTBONUS,FIRSTBAG,FIRSTMONSTER,FIRSTFIREBALL,FIRSTDIGGER};
int lastt[TYPES]={LASTBONUS,LASTBAG,LASTMONSTER,LASTFIREBALL,LASTDIGGER};

void bcollides(int spr)
{
  int spc,next,i;
  for (next=0;next<TYPES;next++)
    first[next]=-1;
  for (next=0;next<SPRITES;next++)
    coll[next]=-1;
  for (i=0;i<TYPES;i++) {
    next=-1;
    for (spc=firstt[i];spc<lastt[i];spc++)
      if (sprenf[spc] && spc!=spr)
        if (bcollide(spr,spc))
          if (next==-1)
            first[i]=next=spc;
          else
            coll[next=(coll[next]=spc)]=-1;
  }
}
