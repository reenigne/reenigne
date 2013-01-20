/* Digger Remastered
   Copyright (c) Andrew Jenner 1998-2004 */

#include <stdio.h>
#include <string.h>
#include "def.h"
#include "drawing.h"
#include "main.h"
#include "hardware.h"
#include "sprite.h"
#include "digger.h"
#include "sound.h"

Sint4 field1[MSIZE],field2[MSIZE],field[MSIZE];

#ifdef _WINDOWS
Uint3 monbufs[MONSTERS][960],bagbufs[BAGS][960],bonusbufs[BONUSES][960],
      diggerbufs[DIGGERS][960],firebufs[FIREBALLS][256];
#else
Uint3 monbufs[MONSTERS][480],bagbufs[BAGS][480],bonusbufs[BONUSES][480],
      diggerbufs[DIGGERS][480],firebufs[FIREBALLS][128];
#endif

Uint4 bitmasks[12]={0xfffe,0xfffd,0xfffb,0xfff7,0xffef,0xffdf,0xffbf,0xff7f,
                    0xfeff,0xfdff,0xfbff,0xf7ff};

Sint4 monspr[MONSTERS];
Sint4 monspd[MONSTERS];

Sint4 digspr[DIGGERS],digspd[DIGGERS],firespr[FIREBALLS];

void drawlife(Sint4 t,Sint4 x,Sint4 y);
void createdbfspr(void);
void initdbfspr(void);
void drawbackg(Sint4 l);
void drawfield(void);

void outtext(char *p,Sint4 x,Sint4 y,Sint4 c)
{
  Sint4 i;
  for (i=0;p[i];i++) {
    gwrite(x,y,p[i],c);
    x+=12;
  }
}

void makefield(void)
{
  Sint4 c,x,y;
  for (x=0;x<MWIDTH;x++)
    for (y=0;y<MHEIGHT;y++) {
      field[y*MWIDTH+x]=-1;
      c=getlevch(x,y,levplan());
      if (c=='S' || c=='V')
        field[y*MWIDTH+x]&=0xd03f;
      if (c=='S' || c=='H')
        field[y*MWIDTH+x]&=0xdfe0;
      if (curplayer==0)
        field1[y*MWIDTH+x]=field[y*MWIDTH+x];
      else
        field2[y*MWIDTH+x]=field[y*MWIDTH+x];
    }
}

void drawstatics(void)
{
  Sint4 x,y;
  for (x=0;x<MWIDTH;x++)
    for (y=0;y<MHEIGHT;y++)
      if (curplayer==0)
        field[y*MWIDTH+x]=field1[y*MWIDTH+x];
      else
        field[y*MWIDTH+x]=field2[y*MWIDTH+x];
  setretr(true);
  gpal(0);
  ginten(0);
  drawbackg(levplan());
  drawfield();
}

void savefield(void)
{
  Sint4 x,y;
  for (x=0;x<MWIDTH;x++)
    for (y=0;y<MHEIGHT;y++)
      if (curplayer==0)
        field1[y*MWIDTH+x]=field[y*MWIDTH+x];
      else
        field2[y*MWIDTH+x]=field[y*MWIDTH+x];
}

void drawfield(void)
{
  Sint4 x,y,xp,yp;
  for (x=0;x<MWIDTH;x++)
    for (y=0;y<MHEIGHT;y++)
      if ((field[y*MWIDTH+x]&0x2000)==0) {
        xp=x*20+12;
        yp=y*18+18;
        if ((field[y*MWIDTH+x]&0xfc0)!=0xfc0) {
          field[y*MWIDTH+x]&=0xd03f;
          drawbottomblob(xp,yp-15);
          drawbottomblob(xp,yp-12);
          drawbottomblob(xp,yp-9);
          drawbottomblob(xp,yp-6);
          drawbottomblob(xp,yp-3);
          drawtopblob(xp,yp+3);
        }
        if ((field[y*MWIDTH+x]&0x1f)!=0x1f) {
          field[y*MWIDTH+x]&=0xdfe0;
          drawrightblob(xp-16,yp);
          drawrightblob(xp-12,yp);
          drawrightblob(xp-8,yp);
          drawrightblob(xp-4,yp);
          drawleftblob(xp+4,yp);
        }
        if (x<14)
          if ((field[y*MWIDTH+x+1]&0xfdf)!=0xfdf)
            drawrightblob(xp,yp);
        if (y<9)
          if ((field[(y+1)*MWIDTH+x]&0xfdf)!=0xfdf)
            drawbottomblob(xp,yp);
      }
}

void eatfield(Sint4 x,Sint4 y,Sint4 dir)
{
  Sint4 h=(x-12)/20,xr=((x-12)%20)/4,v=(y-18)/18,yr=((y-18)%18)/3;
  incpenalty();
  switch (dir) {
    case DIR_RIGHT:
      h++;
      field[v*MWIDTH+h]&=bitmasks[xr];
      if (field[v*MWIDTH+h]&0x1f)
        break;
      field[v*MWIDTH+h]&=0xdfff;
      break;
    case DIR_UP:
      yr--;
      if (yr<0) {
        yr+=6;
        v--;
      }
      field[v*MWIDTH+h]&=bitmasks[6+yr];
      if (field[v*MWIDTH+h]&0xfc0)
        break;
      field[v*MWIDTH+h]&=0xdfff;
      break;
    case DIR_LEFT:
      xr--;
      if (xr<0) {
        xr+=5;
        h--;
      }
      field[v*MWIDTH+h]&=bitmasks[xr];
      if (field[v*MWIDTH+h]&0x1f)
        break;
      field[v*MWIDTH+h]&=0xdfff;
      break;
    case DIR_DOWN:
      v++;
      field[v*MWIDTH+h]&=bitmasks[6+yr];
      if (field[v*MWIDTH+h]&0xfc0)
        break;
      field[v*MWIDTH+h]&=0xdfff;
  }
}

void creatembspr(void)
{
  Sint4 i;
  for (i=0;i<BAGS;i++)
    createspr(FIRSTBAG+i,62,bagbufs[i],4,15,0,0);
  for (i=0;i<MONSTERS;i++)
    createspr(FIRSTMONSTER+i,71,monbufs[i],4,15,0,0);
  createdbfspr();
  for (i=0;i<MONSTERS;i++) {
    monspr[i]=0;
    monspd[i]=1;
  }
}

void initmbspr(void)
{
  int i;
  for (i=0;i<BAGS;i++)
    initspr(FIRSTBAG+i,62,4,15,0,0);
  for (i=0;i<MONSTERS;i++)
    initspr(FIRSTMONSTER+i,71,4,15,0,0);
  initdbfspr();
}

void drawmon(Sint4 n,bool nobf,Sint4 dir,Sint4 x,Sint4 y)
{
  monspr[n]+=monspd[n];
  if (monspr[n]==2 || monspr[n]==0)
    monspd[n]=-monspd[n];
  if (monspr[n]>2)
    monspr[n]=2;
  if (monspr[n]<0)
    monspr[n]=0;
  if (nobf)
    initspr(FIRSTMONSTER+n,monspr[n]+69,4,15,0,0);
  else
    switch (dir) {
      case DIR_RIGHT:
        initspr(FIRSTMONSTER+n,monspr[n]+73,4,15,0,0);
        break;
      case DIR_LEFT:
        initspr(FIRSTMONSTER+n,monspr[n]+77,4,15,0,0);
    }
  drawspr(FIRSTMONSTER+n,x,y);
}

void drawmondie(Sint4 n,bool nobf,Sint4 dir,Sint4 x,Sint4 y)
{
  if (nobf)
    initspr(FIRSTMONSTER+n,72,4,15,0,0);
  else
    switch(dir) {
      case DIR_RIGHT:
        initspr(FIRSTMONSTER+n,76,4,15,0,0);
        break;
      case DIR_LEFT:
        initspr(FIRSTMONSTER+n,80,4,14,0,0);
    }
  drawspr(FIRSTMONSTER+n,x,y);
}

void drawgold(Sint4 n,Sint4 t,Sint4 x,Sint4 y)
{
  initspr(FIRSTBAG+n,t+62,4,15,0,0);
  drawspr(FIRSTBAG+n,x,y);
}

void drawlife(Sint4 t,Sint4 x,Sint4 y)
{
  drawmiscspr(x,y,t+110,4,12);
}

void drawemerald(Sint4 x,Sint4 y)
{
  initmiscspr(x,y,4,10);
  drawmiscspr(x,y,108,4,10);
  getis();
}

void eraseemerald(Sint4 x,Sint4 y)
{
  initmiscspr(x,y,4,10);
  drawmiscspr(x,y,109,4,10);
  getis();
}

void createdbfspr(void)
{
  int i;
  for (i=0;i<DIGGERS;i++) {
    digspd[i]=1;
    digspr[i]=0;
  }
  for (i=0;i<FIREBALLS;i++)
    firespr[i]=0;
  for (i=FIRSTDIGGER;i<LASTDIGGER;i++)
    createspr(i,0,diggerbufs[i-FIRSTDIGGER],4,15,0,0);
  for (i=FIRSTBONUS;i<LASTBONUS;i++)
    createspr(i,81,bonusbufs[i-FIRSTBONUS],4,15,0,0);
  for (i=FIRSTFIREBALL;i<LASTFIREBALL;i++)
    createspr(i,82,firebufs[i-FIRSTFIREBALL],2,8,0,0);
}

void initdbfspr(void)
{
  int i;
  for (i=0;i<DIGGERS;i++) {
    digspd[i]=1;
    digspr[i]=0;
  }
  for (i=0;i<FIREBALLS;i++)
    firespr[i]=0;
  for (i=FIRSTDIGGER;i<LASTDIGGER;i++)
    initspr(i,0,4,15,0,0);
  for (i=FIRSTBONUS;i<LASTBONUS;i++)
    initspr(i,81,4,15,0,0);
  for (i=FIRSTFIREBALL;i<LASTFIREBALL;i++)
    initspr(i,82,2,8,0,0);
}

void drawrightblob(Sint4 x,Sint4 y)
{
  initmiscspr(x+16,y-1,2,18);
  drawmiscspr(x+16,y-1,102,2,18);
  getis();
}

void drawleftblob(Sint4 x,Sint4 y)
{
  initmiscspr(x-8,y-1,2,18);
  drawmiscspr(x-8,y-1,104,2,18);
  getis();
}

void drawtopblob(Sint4 x,Sint4 y)
{
  initmiscspr(x-4,y-6,6,6);
  drawmiscspr(x-4,y-6,103,6,6);
  getis();
}

void drawbottomblob(Sint4 x,Sint4 y)
{
  initmiscspr(x-4,y+15,6,6);
  drawmiscspr(x-4,y+15,105,6,6);
  getis();
}

void drawfurryblob(Sint4 x,Sint4 y)
{
  initmiscspr(x-4,y+15,6,8);
  drawmiscspr(x-4,y+15,107,6,8);
  getis();
}

void drawsquareblob(Sint4 x,Sint4 y)
{
  initmiscspr(x-4,y+17,6,6);
  drawmiscspr(x-4,y+17,106,6,6);
  getis();
}

void drawbackg(Sint4 l)
{
  Sint4 x,y;
  for (y=14;y<200;y+=4) {
    fillbuffer();
    for (x=0;x<320;x+=20)
      drawmiscspr(x,y,93+l,5,4);
  }
}

void drawfire(int n,Sint4 x,Sint4 y,Sint4 t)
{
  int nn=(n==0) ? 0 : 32;
  if (t==0) {
    firespr[n]++;
    if (firespr[n]>2)
      firespr[n]=0;
    initspr(FIRSTFIREBALL+n,82+firespr[n]+nn,2,8,0,0);
  }
  else
    initspr(FIRSTFIREBALL+n,84+t+nn,2,8,0,0);
  drawspr(FIRSTFIREBALL+n,x,y);
}

void drawbonus(Sint4 x,Sint4 y)
{
  int n=0;
  initspr(FIRSTBONUS+n,81,4,15,0,0);
  movedrawspr(FIRSTBONUS+n,x,y);
}

void drawdigger(int n,Sint4 t,Sint4 x,Sint4 y,bool f)
{
  int nn=(n==0) ? 0 : 31;
  digspr[n]+=digspd[n];
  if (digspr[n]==2 || digspr[n]==0)
    digspd[n]=-digspd[n];
  if (digspr[n]>2)
    digspr[n]=2;
  if (digspr[n]<0)
    digspr[n]=0;
  if (t>=0 && t<=6 && !(t&1)) {
    initspr(FIRSTDIGGER+n,(t+(f ? 0 : 1))*3+digspr[n]+1+nn,4,15,0,0);
    drawspr(FIRSTDIGGER+n,x,y);
    return;
  }
  if (t>=10 && t<=15) {
    initspr(FIRSTDIGGER+n,40+nn-t,4,15,0,0);
    drawspr(FIRSTDIGGER+n,x,y);
    return;
  }
  first[0]=first[1]=first[2]=first[3]=first[4]=-1;
}

void drawlives(void)
{
  Sint4 l,n,g;
  char buf[10];
  if (gauntlet) {
    g=(Sint4)(cgtime/1193181l);
    sprintf(buf,"%3i:%02i",g/60,g%60);
    outtext(buf,124,0,3);
    return;
  }
  n=getlives(0)-1;
  outtext("     ",96,0,2);
  if (n>4) {
    drawlife(0,80,0);
    sprintf(buf,"X%i",n);
    outtext(buf,100,0,2);
  }
  else
    for (l=1;l<5;l++) {
      drawlife(n>0 ? 0 : 2,l*20+60,0);
      n--;
    }
  if (nplayers==2) {
    outtext("     ",164,0,2);
    n=getlives(1)-1;
    if (n>4) {
      sprintf(buf,"%iX",n);
      outtext(buf,220-strlen(buf)*12,0,2);
      drawlife(1,224,0);
    }
    else
      for (l=1;l<5;l++) {
        drawlife(n>0 ? 1 : 2,244-l*20,0);
        n--;
      }
  }
  if (diggers==2) {
    outtext("     ",164,0,1);
    n=getlives(1)-1;
    if (n>4) {
      sprintf(buf,"%iX",n);
      outtext(buf,220-strlen(buf)*12,0,1);
      drawlife(3,224,0);
    }
    else
      for (l=1;l<5;l++) {
        drawlife(n>0 ? 3 : 2,244-l*20,0);
        n--;
      }
  }
}
