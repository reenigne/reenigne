/* Digger Remastered
   Copyright (c) Andrew Jenner 1998-2004 */

#include <string.h>
#include "def.h"
#include "bags.h"
#include "main.h"
#include "sprite.h"
#include "sound.h"
#include "drawing.h"
#include "monster.h"
#include "digger.h"
#include "scores.h"

struct bag {
  Sint4 x,y,h,v,xr,yr,dir,wt,gt,fallh;
  bool wobbling,unfallen,exist;
} bagdat1[BAGS],bagdat2[BAGS],bagdat[BAGS];

Sint4 pushcount=0,goldtime=0;

void updatebag(Sint4 bag);
void baghitground(Sint4 bag);
bool pushbag(Sint4 bag,Sint4 dir);
void removebag(Sint4 bn);
void getgold(Sint4 bag);

void initbags(void)
{
  Sint4 bag,x,y;
  pushcount=0;
  goldtime=150-levof10()*10;
  for (bag=0;bag<BAGS;bag++)
    bagdat[bag].exist=FALSE;
  bag=0;
  for (x=0;x<MWIDTH;x++)
    for (y=0;y<MHEIGHT;y++)
      if (getlevch(x,y,levplan())=='B')
        if (bag<BAGS) {
          bagdat[bag].exist=TRUE;
          bagdat[bag].gt=0;
          bagdat[bag].fallh=0;
          bagdat[bag].dir=DIR_NONE;
          bagdat[bag].wobbling=FALSE;
          bagdat[bag].wt=15;
          bagdat[bag].unfallen=TRUE;
          bagdat[bag].x=x*20+12;
          bagdat[bag].y=y*18+18;
          bagdat[bag].h=x;
          bagdat[bag].v=y;
          bagdat[bag].xr=0;
          bagdat[bag++].yr=0;
        }
  if (curplayer==0)
    memcpy(bagdat1,bagdat,BAGS*sizeof(struct bag));
  else
    memcpy(bagdat2,bagdat,BAGS*sizeof(struct bag));
}

void drawbags(void)
{
  Sint4 bag;
  for (bag=0;bag<BAGS;bag++) {
    if (curplayer==0)
      memcpy(&bagdat[bag],&bagdat1[bag],sizeof(struct bag));
    else
      memcpy(&bagdat[bag],&bagdat2[bag],sizeof(struct bag));
    if (bagdat[bag].exist)
      movedrawspr(bag+FIRSTBAG,bagdat[bag].x,bagdat[bag].y);
  }
}

void cleanupbags(void)
{
  Sint4 bag;
  soundfalloff();
  for (bag=0;bag<BAGS;bag++) {
    if (bagdat[bag].exist && ((bagdat[bag].h==7 && bagdat[bag].v==9) ||
        bagdat[bag].xr!=0 || bagdat[bag].yr!=0 || bagdat[bag].gt!=0 ||
        bagdat[bag].fallh!=0 || bagdat[bag].wobbling)) {
      bagdat[bag].exist=FALSE;
      erasespr(bag+FIRSTBAG);
    }
    if (curplayer==0)
      memcpy(&bagdat1[bag],&bagdat[bag],sizeof(struct bag));
    else
      memcpy(&bagdat2[bag],&bagdat[bag],sizeof(struct bag));
  }
}

void dobags(void)
{
  Sint4 bag;
  bool soundfalloffflag=TRUE,soundwobbleoffflag=TRUE;
  for (bag=0;bag<BAGS;bag++)
    if (bagdat[bag].exist) {
      if (bagdat[bag].gt!=0) {
        if (bagdat[bag].gt==1) {
          soundbreak();
          drawgold(bag,4,bagdat[bag].x,bagdat[bag].y);
          incpenalty();
        }
        if (bagdat[bag].gt==3) {
          drawgold(bag,5,bagdat[bag].x,bagdat[bag].y);
          incpenalty();
        }
        if (bagdat[bag].gt==5) {
          drawgold(bag,6,bagdat[bag].x,bagdat[bag].y);
          incpenalty();
        }
        bagdat[bag].gt++;
        if (bagdat[bag].gt==goldtime)
          removebag(bag);
        else
          if (bagdat[bag].v<MHEIGHT-1 && bagdat[bag].gt<goldtime-10)
            if ((getfield(bagdat[bag].h,bagdat[bag].v+1)&0x2000)==0)
              bagdat[bag].gt=goldtime-10;
      }
      else
        updatebag(bag);
    }
  for (bag=0;bag<BAGS;bag++) {
    if (bagdat[bag].dir==DIR_DOWN && bagdat[bag].exist)
      soundfalloffflag=FALSE;
    if (bagdat[bag].dir!=DIR_DOWN && bagdat[bag].wobbling && bagdat[bag].exist)
      soundwobbleoffflag=FALSE;
  }
  if (soundfalloffflag)
    soundfalloff();
  if (soundwobbleoffflag)
    soundwobbleoff();
}

Sint4 wblanim[4]={2,0,1,0};

void updatebag(Sint4 bag)
{
  Sint4 x,h,xr,y,v,yr,wbl;
  x=bagdat[bag].x;
  h=bagdat[bag].h;
  xr=bagdat[bag].xr;
  y=bagdat[bag].y;
  v=bagdat[bag].v;
  yr=bagdat[bag].yr;
  switch (bagdat[bag].dir) {
    case DIR_NONE:
      if (y<180 && xr==0) {
        if (bagdat[bag].wobbling) {
          if (bagdat[bag].wt==0) {
            bagdat[bag].dir=DIR_DOWN;
            soundfall();
            break;
          }
          bagdat[bag].wt--;
          wbl=bagdat[bag].wt%8;
          if (!(wbl&1)) {
            drawgold(bag,wblanim[wbl>>1],x,y);
            incpenalty();
            soundwobble();
          }
        }
        else
          if ((getfield(h,v+1)&0xfdf)!=0xfdf)
            if (!checkdiggerunderbag(h,v+1))
              bagdat[bag].wobbling=TRUE;
      }
      else {
        bagdat[bag].wt=15;
        bagdat[bag].wobbling=FALSE;
      }
      break;
    case DIR_RIGHT:
    case DIR_LEFT:
      if (xr==0)
        if (y<180 && (getfield(h,v+1)&0xfdf)!=0xfdf) {
          bagdat[bag].dir=DIR_DOWN;
          bagdat[bag].wt=0;
          soundfall();
        }
        else
          baghitground(bag);
      break;
    case DIR_DOWN:
      if (yr==0)
        bagdat[bag].fallh++;
      if (y>=180)
        baghitground(bag);
      else
        if ((getfield(h,v+1)&0xfdf)==0xfdf)
          if (yr==0)
            baghitground(bag);
      checkmonscared(bagdat[bag].h);
  }
  if (bagdat[bag].dir!=DIR_NONE)
    if (bagdat[bag].dir!=DIR_DOWN && pushcount!=0)
      pushcount--;
    else
      pushbag(bag,bagdat[bag].dir);
}

void baghitground(Sint4 bag)
{
  int clfirst[TYPES],clcoll[SPRITES],i;
  if (bagdat[bag].dir==DIR_DOWN && bagdat[bag].fallh>1)
    bagdat[bag].gt=1;
  else
    bagdat[bag].fallh=0;
  bagdat[bag].dir=DIR_NONE;
  bagdat[bag].wt=15;
  bagdat[bag].wobbling=FALSE;
  drawgold(bag,0,bagdat[bag].x,bagdat[bag].y);
  for (i=0;i<TYPES;i++)
    clfirst[i]=first[i];
  for (i=0;i<SPRITES;i++)
    clcoll[i]=coll[i];
  incpenalty();
  i=clfirst[1];
  while (i!=-1) {
    removebag(i-FIRSTBAG);
    i=clcoll[i];
  }
}

bool pushbag(Sint4 bag,Sint4 dir)
{
  Sint4 x,y,h,v,ox,oy;
  int clfirst[TYPES],clcoll[SPRITES],i;
  bool push=TRUE,digf;
  ox=x=bagdat[bag].x;
  oy=y=bagdat[bag].y;
  h=bagdat[bag].h;
  v=bagdat[bag].v;
  if (bagdat[bag].gt!=0) {
    getgold(bag);
    return TRUE;
  }
  if (bagdat[bag].dir==DIR_DOWN && (dir==DIR_RIGHT || dir==DIR_LEFT)) {
    drawgold(bag,3,x,y);
    for (i=0;i<TYPES;i++)
      clfirst[i]=first[i];
    for (i=0;i<SPRITES;i++)
      clcoll[i]=coll[i];
    incpenalty();
    i=clfirst[4];
    while (i!=-1) {
      if (diggery(i-FIRSTDIGGER+curplayer)>=y)
        killdigger(i-FIRSTDIGGER+curplayer,1,bag);
      i=clcoll[i];
    }
    if (clfirst[2]!=-1)
      squashmonsters(bag,clfirst,clcoll);
    return 1;
  }
  if ((x==292 && dir==DIR_RIGHT) || (x==12 && dir==DIR_LEFT) ||
      (y==180 && dir==DIR_DOWN) || (y==18 && dir==DIR_UP))
    push=FALSE;
  if (push) {
    switch (dir) {
      case DIR_RIGHT:
        x+=4;
        break;
      case DIR_LEFT:
        x-=4;
        break;
      case DIR_DOWN:
        if (bagdat[bag].unfallen) {
          bagdat[bag].unfallen=FALSE;
          drawsquareblob(x,y);
          drawtopblob(x,y+21);
        }
        else
          drawfurryblob(x,y);
        eatfield(x,y,dir);
        killemerald(h,v);
        y+=6;
    }
    switch(dir) {
      case DIR_DOWN:
        drawgold(bag,3,x,y);
        for (i=0;i<TYPES;i++)
          clfirst[i]=first[i];
        for (i=0;i<SPRITES;i++)
          clcoll[i]=coll[i];
        incpenalty();
        i=clfirst[4];
        while (i!=-1) {
          if (diggery(i-FIRSTDIGGER+curplayer)>=y)
            killdigger(i-FIRSTDIGGER+curplayer,1,bag);
          i=clcoll[i];
        }
        if (clfirst[2]!=-1)
          squashmonsters(bag,clfirst,clcoll);
        break;
      case DIR_RIGHT:
      case DIR_LEFT:
        bagdat[bag].wt=15;
        bagdat[bag].wobbling=FALSE;
        drawgold(bag,0,x,y);
        for (i=0;i<TYPES;i++)
          clfirst[i]=first[i];
        for (i=0;i<SPRITES;i++)
          clcoll[i]=coll[i];
        incpenalty();
        pushcount=1;
        if (clfirst[1]!=-1)
          if (!pushbags(dir,clfirst,clcoll)) {
            x=ox;
            y=oy;
            drawgold(bag,0,ox,oy);
            incpenalty();
            push=FALSE;
          }
        i=clfirst[4];
        digf=FALSE;
        while (i!=-1) {
          if (digalive(i-FIRSTDIGGER+curplayer))
            digf=TRUE;
          i=clcoll[i];
        }
        if (digf || clfirst[2]!=-1) {
          x=ox;
          y=oy;
          drawgold(bag,0,ox,oy);
          incpenalty();
          push=FALSE;
        }
    }
    if (push)
      bagdat[bag].dir=dir;
    else
      bagdat[bag].dir=reversedir(dir);
    bagdat[bag].x=x;
    bagdat[bag].y=y;
    bagdat[bag].h=(x-12)/20;
    bagdat[bag].v=(y-18)/18;
    bagdat[bag].xr=(x-12)%20;
    bagdat[bag].yr=(y-18)%18;
  }
  return push;
}

bool pushbags(Sint4 dir,int *clfirst,int *clcoll)
{
  bool push=TRUE;
  int next=clfirst[1];
  while (next!=-1) {
    if (!pushbag(next-FIRSTBAG,dir))
      push=FALSE;
    next=clcoll[next];
  }
  return push;
}

bool pushudbags(int *clfirst,int *clcoll)
{
  bool push=TRUE;
  int next=clfirst[1];
  while (next!=-1) {
    if (bagdat[next-FIRSTBAG].gt!=0)
      getgold(next-FIRSTBAG);
    else
      push=FALSE;
    next=clcoll[next];
  }
  return push;
}

void removebag(Sint4 bag)
{
  if (bagdat[bag].exist) {
    bagdat[bag].exist=FALSE;
    erasespr(bag+FIRSTBAG);
  }
}

bool bagexist(int bag)
{
  return bagdat[bag].exist;
}

Sint4 bagy(Sint4 bag)
{
  return bagdat[bag].y;
}

Sint4 getbagdir(Sint4 bag)
{
  if (bagdat[bag].exist)
    return bagdat[bag].dir;
  return -1;
}

void removebags(int *clfirst,int *clcoll)
{
  int next=clfirst[1];
  while (next!=-1) {
    removebag(next-FIRSTBAG);
    next=clcoll[next];
  }
}

Sint4 getnmovingbags(void)
{
  Sint4 bag,n=0;
  for (bag=0;bag<BAGS;bag++)
    if (bagdat[bag].exist && bagdat[bag].gt<10 &&
        (bagdat[bag].gt!=0 || bagdat[bag].wobbling))
      n++;
  return n;
}

void getgold(Sint4 bag)
{
  bool f=TRUE;
  int i;
  drawgold(bag,6,bagdat[bag].x,bagdat[bag].y);
  incpenalty();
  i=first[4];
  while (i!=-1) {
    if (digalive(i-FIRSTDIGGER+curplayer)) {
      scoregold(i-FIRSTDIGGER+curplayer);
      soundgold();
      digresettime(i-FIRSTDIGGER+curplayer);
      f=FALSE;
    }
    i=coll[i];
  }
  if (f)
    mongold();
  removebag(bag);
}
