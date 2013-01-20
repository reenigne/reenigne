/* Digger Remastered
   Copyright (c) Andrew Jenner 1998-2004 */

#include <stdlib.h>
#include "def.h"
#include "monster.h"
#include "main.h"
#include "sprite.h"
#include "digger.h"
#include "drawing.h"
#include "bags.h"
#include "sound.h"
#include "scores.h"
#include "record.h"

struct monster
{
  Sint4 x,y,h,v,xr,yr,dir,hdir,t,hnt,death,bag,dtime,stime,chase;
  bool flag,nob,alive;
} mondat[6];

Sint4 nextmonster=0,totalmonsters=0,maxmononscr=0,nextmontime=0,mongaptime=0;
Sint4 chase=0;

bool unbonusflag=false;

void createmonster(void);
void monai(Sint4 mon);
void mondie(Sint4 mon);
bool fieldclear(Sint4 dir,Sint4 x,Sint4 y);
void squashmonster(Sint4 mon,Sint4 death,Sint4 bag);
Sint4 nmononscr(void);

void initmonsters(void)
{
  Sint4 i;
  for (i=0;i<MONSTERS;i++)
    mondat[i].flag=false;
  nextmonster=0;
  mongaptime=45-(levof10()<<1);
  totalmonsters=levof10()+5;
  switch (levof10()) {
    case 1:
      maxmononscr=3;
      break;
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
      maxmononscr=4;
      break;
    case 8:
    case 9:
    case 10:
      maxmononscr=5;
  }
  nextmontime=10;
  unbonusflag=true;
}

void erasemonsters(void)
{
  Sint4 i;
  for (i=0;i<MONSTERS;i++)
    if (mondat[i].flag)
      erasespr(i+FIRSTMONSTER);
}

void domonsters(void)
{
  Sint4 i;
  if (nextmontime>0)
    nextmontime--;
  else {
    if (nextmonster<totalmonsters && nmononscr()<maxmononscr && isalive() &&
        !bonusmode)
      createmonster();
    if (unbonusflag && nextmonster==totalmonsters && nextmontime==0)
      if (isalive()) {
        unbonusflag=false;
        createbonus();
      }
  }
  for (i=0;i<MONSTERS;i++)
    if (mondat[i].flag) {
      if (mondat[i].hnt>10-levof10()) {
        if (mondat[i].nob) {
          mondat[i].nob=false;
          mondat[i].hnt=0;
        }
      }
      if (mondat[i].alive)
        if (mondat[i].t==0) {
          monai(i);
          if (randno(15-levof10())==0) /* Need to split for determinism */
            if (mondat[i].nob && mondat[i].alive)
              monai(i);
        }
        else
          mondat[i].t--;
      else
        mondie(i);
    }
}

void createmonster(void)
{
  Sint4 i;
  for (i=0;i<MONSTERS;i++)
    if (!mondat[i].flag) {
      mondat[i].flag=true;
      mondat[i].alive=true;
      mondat[i].t=0;
      mondat[i].nob=true;
      mondat[i].hnt=0;
      mondat[i].h=14;
      mondat[i].v=0;
      mondat[i].x=292;
      mondat[i].y=18;
      mondat[i].xr=0;
      mondat[i].yr=0;
      mondat[i].dir=DIR_LEFT;
      mondat[i].hdir=DIR_LEFT;
      mondat[i].chase=chase+curplayer;
      chase=(chase+1)%diggers;
      nextmonster++;
      nextmontime=mongaptime;
      mondat[i].stime=5;
      movedrawspr(i+FIRSTMONSTER,mondat[i].x,mondat[i].y);
      break;
    }
}

bool mongotgold=false;

void mongold(void)
{
  mongotgold=true;
}

void monai(Sint4 mon)
{
  Sint4 monox,monoy,dir,mdirp1,mdirp2,mdirp3,mdirp4,t;
  int clcoll[SPRITES],clfirst[TYPES],i,m,dig;
  bool push,bagf;
  monox=mondat[mon].x;
  monoy=mondat[mon].y;
  if (mondat[mon].xr==0 && mondat[mon].yr==0) {

    /* If we are here the monster needs to know which way to turn next. */

    /* Turn hobbin back into nobbin if it's had its time */

    if (mondat[mon].hnt>30+(levof10()<<1))
      if (!mondat[mon].nob) {
        mondat[mon].hnt=0;
        mondat[mon].nob=true;
      }

    /* Set up monster direction properties to chase Digger */

    dig=mondat[mon].chase;
    if (!digalive(dig))
      dig=(diggers-1)-dig;

    if (abs(diggery(dig)-mondat[mon].y)>abs(diggerx(dig)-mondat[mon].x)) {
      if (diggery(dig)<mondat[mon].y) { mdirp1=DIR_UP;    mdirp4=DIR_DOWN; }
                                 else { mdirp1=DIR_DOWN;  mdirp4=DIR_UP; }
      if (diggerx(dig)<mondat[mon].x) { mdirp2=DIR_LEFT;  mdirp3=DIR_RIGHT; }
                                 else { mdirp2=DIR_RIGHT; mdirp3=DIR_LEFT; }
    }
    else {
      if (diggerx(dig)<mondat[mon].x) { mdirp1=DIR_LEFT;  mdirp4=DIR_RIGHT; }
                                 else { mdirp1=DIR_RIGHT; mdirp4=DIR_LEFT; }
      if (diggery(dig)<mondat[mon].y) { mdirp2=DIR_UP;    mdirp3=DIR_DOWN; }
                                 else { mdirp2=DIR_DOWN;  mdirp3=DIR_UP; }
    }

    /* In bonus mode, run away from Digger */

    if (bonusmode) {
      t=mdirp1; mdirp1=mdirp4; mdirp4=t;
      t=mdirp2; mdirp2=mdirp3; mdirp3=t;
    }

    /* Adjust priorities so that monsters don't reverse direction unless they
       really have to */

    dir=reversedir(mondat[mon].dir);
    if (dir==mdirp1) {
      mdirp1=mdirp2;
      mdirp2=mdirp3;
      mdirp3=mdirp4;
      mdirp4=dir;
    }
    if (dir==mdirp2) {
      mdirp2=mdirp3;
      mdirp3=mdirp4;
      mdirp4=dir;
    }
    if (dir==mdirp3) {
      mdirp3=mdirp4;
      mdirp4=dir;
    }

    /* Introduce a random element on levels <6 : occasionally swap p1 and p3 */

    if (randno(levof10()+5)==1) /* Need to split for determinism */
      if (levof10()<6) {
        t=mdirp1;
        mdirp1=mdirp3;
        mdirp3=t;
      }

    /* Check field and find direction */

    if (fieldclear(mdirp1,mondat[mon].h,mondat[mon].v))
      dir=mdirp1;
    else
      if (fieldclear(mdirp2,mondat[mon].h,mondat[mon].v))
        dir=mdirp2;
      else
        if (fieldclear(mdirp3,mondat[mon].h,mondat[mon].v))
          dir=mdirp3;
        else
          if (fieldclear(mdirp4,mondat[mon].h,mondat[mon].v))
            dir=mdirp4;

    /* Hobbins don't care about the field: they go where they want. */

    if (!mondat[mon].nob)
      dir=mdirp1;

    /* Monsters take a time penalty for changing direction */

    if (mondat[mon].dir!=dir)
      mondat[mon].t++;

    /* Save the new direction */

    mondat[mon].dir=dir;
  }

  /* If monster is about to go off edge of screen, stop it. */

  if ((mondat[mon].x==292 && mondat[mon].dir==DIR_RIGHT) ||
      (mondat[mon].x==12 && mondat[mon].dir==DIR_LEFT) ||
      (mondat[mon].y==180 && mondat[mon].dir==DIR_DOWN) ||
      (mondat[mon].y==18 && mondat[mon].dir==DIR_UP))
    mondat[mon].dir=DIR_NONE;

  /* Change hdir for hobbin */

  if (mondat[mon].dir==DIR_LEFT || mondat[mon].dir==DIR_RIGHT)
    mondat[mon].hdir=mondat[mon].dir;

  /* Hobbins dig */

  if (!mondat[mon].nob)
    eatfield(mondat[mon].x,mondat[mon].y,mondat[mon].dir);

  /* (Draw new tunnels) and move monster */

  switch (mondat[mon].dir) {
    case DIR_RIGHT:
      if (!mondat[mon].nob)
        drawrightblob(mondat[mon].x,mondat[mon].y);
      mondat[mon].x+=4;
      break;
    case DIR_UP:
      if (!mondat[mon].nob)
        drawtopblob(mondat[mon].x,mondat[mon].y);
      mondat[mon].y-=3;
      break;
    case DIR_LEFT:
      if (!mondat[mon].nob)
        drawleftblob(mondat[mon].x,mondat[mon].y);
      mondat[mon].x-=4;
      break;
    case DIR_DOWN:
      if (!mondat[mon].nob)
        drawbottomblob(mondat[mon].x,mondat[mon].y);
      mondat[mon].y+=3;
      break;
  }

  /* Hobbins can eat emeralds */

  if (!mondat[mon].nob)
    hitemerald((mondat[mon].x-12)/20,(mondat[mon].y-18)/18,
               (mondat[mon].x-12)%20,(mondat[mon].y-18)%18,
               mondat[mon].dir);

  /* If Digger's gone, don't bother */

  if (!isalive()) {
    mondat[mon].x=monox;
    mondat[mon].y=monoy;
  }

  /* If monster's just started, don't move yet */

  if (mondat[mon].stime!=0) {
    mondat[mon].stime--;
    mondat[mon].x=monox;
    mondat[mon].y=monoy;
  }

  /* Increase time counter for hobbin */

  if (!mondat[mon].nob && mondat[mon].hnt<100)
    mondat[mon].hnt++;

  /* Draw monster */

  push=true;
  drawmon(mon,mondat[mon].nob,mondat[mon].hdir,mondat[mon].x,mondat[mon].y);
  for (i=0;i<TYPES;i++)
    clfirst[i]=first[i];
  for (i=0;i<SPRITES;i++)
    clcoll[i]=coll[i];
  incpenalty();

  /* Collision with another monster */

  if (clfirst[2]!=-1) {
    mondat[mon].t++; /* Time penalty */
    /* Ensure both aren't moving in the same dir. */
    i=clfirst[2];
    do {
      m=i-FIRSTMONSTER;
      if (mondat[mon].dir==mondat[m].dir && mondat[m].stime==0 &&
          mondat[mon].stime==0)
        mondat[m].dir=reversedir(mondat[m].dir);
      /* The kludge here is to preserve playback for a bug in previous
         versions. */
      if (!kludge)
        incpenalty();
      else
        if (!(m&1))
          incpenalty();
      i=clcoll[i];
    } while (i!=-1);
    if (kludge)
      if (clfirst[0]!=-1)
        incpenalty();
  }

  /* Check for collision with bag */

  i=clfirst[1];
  bagf=false;
  while (i!=-1) {
    if (bagexist(i-FIRSTBAG)) {
      bagf=true;
      break;
    }
    i=clcoll[i];
  }

  if (bagf) {
    mondat[mon].t++; /* Time penalty */
    mongotgold=false;
    if (mondat[mon].dir==DIR_RIGHT || mondat[mon].dir==DIR_LEFT) {
      push=pushbags(mondat[mon].dir,clfirst,clcoll);      /* Horizontal push */
      mondat[mon].t++; /* Time penalty */
    }
    else
      if (!pushudbags(clfirst,clcoll)) /* Vertical push */
        push=false;
    if (mongotgold) /* No time penalty if monster eats gold */
      mondat[mon].t=0;
    if (!mondat[mon].nob && mondat[mon].hnt>1)
      removebags(clfirst,clcoll); /* Hobbins eat bags */
  }

  /* Increase hobbin cross counter */

  if (mondat[mon].nob && clfirst[2]!=-1 && isalive())
    mondat[mon].hnt++;

  /* See if bags push monster back */

  if (!push) {
    mondat[mon].x=monox;
    mondat[mon].y=monoy;
    drawmon(mon,mondat[mon].nob,mondat[mon].hdir,mondat[mon].x,mondat[mon].y);
    incpenalty();
    if (mondat[mon].nob) /* The other way to create hobbin: stuck on h-bag */
      mondat[mon].hnt++;
    if ((mondat[mon].dir==DIR_UP || mondat[mon].dir==DIR_DOWN) &&
        mondat[mon].nob)
      mondat[mon].dir=reversedir(mondat[mon].dir); /* If vertical, give up */
  }

  /* Collision with Digger */

  if (clfirst[4]!=-1 && isalive())
    if (bonusmode) {
      killmon(mon);
      i=clfirst[4];
      while (i!=-1) {
        if (digalive(i-FIRSTDIGGER+curplayer))
          sceatm(i-FIRSTDIGGER+curplayer);
        i=clcoll[i];
      }
      soundeatm(); /* Collision in bonus mode */
    }
    else {
      i=clfirst[4];
      while (i!=-1) {
        if (digalive(i-FIRSTDIGGER+curplayer))
          killdigger(i-FIRSTDIGGER+curplayer,3,0); /* Kill Digger */
        i=clcoll[i];
      }
    }

  /* Update co-ordinates */

  mondat[mon].h=(mondat[mon].x-12)/20;
  mondat[mon].v=(mondat[mon].y-18)/18;
  mondat[mon].xr=(mondat[mon].x-12)%20;
  mondat[mon].yr=(mondat[mon].y-18)%18;
}

void mondie(Sint4 mon)
{
  switch (mondat[mon].death) {
    case 1:
      if (bagy(mondat[mon].bag)+6>mondat[mon].y)
        mondat[mon].y=bagy(mondat[mon].bag);
      drawmondie(mon,mondat[mon].nob,mondat[mon].hdir,mondat[mon].x,
                 mondat[mon].y);
      incpenalty();
      if (getbagdir(mondat[mon].bag)==-1) {
        mondat[mon].dtime=1;
        mondat[mon].death=4;
      }
      break;
    case 4:
      if (mondat[mon].dtime!=0)
        mondat[mon].dtime--;
      else {
        killmon(mon);
        if (diggers==2)
          scorekill2();
        else
          scorekill(curplayer);
      }
  }
}

bool fieldclear(Sint4 dir,Sint4 x,Sint4 y)
{
  switch (dir) {
    case DIR_RIGHT:
      if (x<14)
        if ((getfield(x+1,y)&0x2000)==0)
          if ((getfield(x+1,y)&1)==0 || (getfield(x,y)&0x10)==0)
            return true;
      break;
    case DIR_UP:
      if (y>0)
        if ((getfield(x,y-1)&0x2000)==0)
          if ((getfield(x,y-1)&0x800)==0 || (getfield(x,y)&0x40)==0)
            return true;
      break;
    case DIR_LEFT:
      if (x>0)
        if ((getfield(x-1,y)&0x2000)==0)
          if ((getfield(x-1,y)&0x10)==0 || (getfield(x,y)&1)==0)
            return true;
      break;
    case DIR_DOWN:
      if (y<9)
        if ((getfield(x,y+1)&0x2000)==0)
          if ((getfield(x,y+1)&0x40)==0 || (getfield(x,y)&0x800)==0)
            return true;
  }
  return false;
}

void checkmonscared(Sint4 h)
{
  Sint4 m;
  for (m=0;m<MONSTERS;m++)
    if (h==mondat[m].h && mondat[m].dir==DIR_UP)
      mondat[m].dir=DIR_DOWN;
}

void killmon(Sint4 mon)
{
  if (mondat[mon].flag) {
    mondat[mon].flag=mondat[mon].alive=false;
    erasespr(mon+FIRSTMONSTER);
    if (bonusmode)
      totalmonsters++;
  }
}

void squashmonsters(Sint4 bag,int *clfirst,int *clcoll)
{
  int next=clfirst[2],m;
  while (next!=-1) {
    m=next-FIRSTMONSTER;
    if (mondat[m].y>=bagy(bag))
      squashmonster(m,1,bag);
    next=clcoll[next];
  }
}

Sint4 killmonsters(int *clfirst,int *clcoll)
{
  int next=clfirst[2],m,n=0;
  while (next!=-1) {
    m=next-FIRSTMONSTER;
    killmon(m);
    n++;
    next=clcoll[next];
  }
  return n;
}

void squashmonster(Sint4 mon,Sint4 death,Sint4 bag)
{
  mondat[mon].alive=false;
  mondat[mon].death=death;
  mondat[mon].bag=bag;
}

Sint4 monleft(void)
{
  return nmononscr()+totalmonsters-nextmonster;
}

Sint4 nmononscr(void)
{
  Sint4 i,n=0;
  for (i=0;i<MONSTERS;i++)
    if (mondat[i].flag)
      n++;
  return n;
}

void incmont(Sint4 n)
{
  Sint4 m;
  if (n>MONSTERS)
    n=MONSTERS;
  for (m=1;m<n;m++)
    mondat[m].t++;
}

Sint4 getfield(Sint4 x,Sint4 y)
{
  return field[y*15+x];
}
