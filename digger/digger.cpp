/* Digger Remastered
   Copyright (c) Andrew Jenner 1998-2004 */

#include "def.h"
#include "sprite.h"
#include "input.h"
#include "hardware.h"
#include "digger.h"
#include "drawing.h"
#include "main.h"
#include "sound.h"
#include "monster.h"
#include "scores.h"
#include "bags.h"

#ifdef _WINDOWS
#include "win_dig.h"
#endif

struct digger
{
  Sint4 x,y,h,v,rx,ry,mdir,dir,bagtime,rechargetime,fx,fy,fdir,expsn,
        deathstage,deathbag,deathani,deathtime,emocttime,emn,msc,lives,ivt;
  bool notfiring,alive,firepressed,dead,levdone,invin;
} digdat[DIGGERS];

Sint4 startbonustimeleft=0,bonustimeleft;

Sint4 emmask=0;

Sint3 emfield[MSIZE];

bool bonusvisible=FALSE,bonusmode=FALSE,digvisible;

void updatedigger(int n);
void diggerdie(int n);
void initbonusmode(void);
void endbonusmode(void);
bool getfirepflag(int n);
void drawdig(int n,int d,int x,int y,bool f);

void initdigger(void)
{
  int dig;
  for (dig=curplayer;dig<diggers+curplayer;dig++) {
    if (digdat[dig].lives==0)
      continue;
    digdat[dig].v=9;
    digdat[dig].mdir=4;
    digdat[dig].h=(diggers==1) ? 7 : (8-dig*2);
    digdat[dig].x=digdat[dig].h*20+12;
    digdat[dig].dir=(dig==0) ? DIR_RIGHT : DIR_LEFT;
    digdat[dig].rx=0;
    digdat[dig].ry=0;
    digdat[dig].bagtime=0;
    digdat[dig].alive=TRUE;
    digdat[dig].dead=FALSE; /* alive !=> !dead but dead => !alive */
    digdat[dig].invin=FALSE;
    digdat[dig].ivt=0;
    digdat[dig].deathstage=1;
    digdat[dig].y=digdat[dig].v*18+18;
    movedrawspr(dig+FIRSTDIGGER-curplayer,digdat[dig].x,digdat[dig].y);
    digdat[dig].notfiring=TRUE;
    digdat[dig].emocttime=0;
    digdat[dig].firepressed=FALSE;
    digdat[dig].expsn=0;
    digdat[dig].rechargetime=0;
    digdat[dig].emn=0;
    digdat[dig].msc=1;
  }
  digvisible=TRUE;
  bonusvisible=bonusmode=FALSE;
}

Uint5 curtime,ftime;

#ifdef INTDRF
Uint5 frame;
#endif

void newframe(void)
{

#ifndef ARM

  Uint5 t;
  if (synchvid) {
    for (;curtime<ftime;curtime+=17094) { /* 17094 = ticks in a refresh */
#ifdef _WINDOWS
      do_windows_events();
#endif
      fillbuffer();
      gretrace();
      checkkeyb();
    }
    curtime-=ftime;
    fillbuffer();
  }
  else {
    do {
#ifdef _WINDOWS
      do_windows_events();
#endif
      fillbuffer();             /* Idle time */
      t=gethrt();
      checkkeyb();
    } while (curtime+ftime>t && t>curtime);
    curtime=t;
  }

#else

  for (;curtime<ftime;curtime+=15000) {
    fillbuffer();
    gretrace();
    soundint();
    checkkeyb();
  }
  curtime-=ftime;

#endif

#ifdef INTDRF
  frame++;
#endif

}

Uint5 cgtime;

void drawdig(int n,int d,int x,int y,bool f)
{
  drawdigger(n-curplayer,d,x,y,f);
  if (digdat[n].invin) {
    digdat[n].ivt--;
    if (digdat[n].ivt==0)
      digdat[n].invin=FALSE;
    else
      if (digdat[n].ivt%10<5)
        erasespr(FIRSTDIGGER+n-curplayer);
  }
}

void dodigger(void)
{
  int n;
  newframe();
  if (gauntlet) {
    drawlives();
    if (cgtime<ftime)
      timeout=TRUE;
    cgtime-=ftime;
  }
  for (n=curplayer;n<diggers+curplayer;n++) {
    if (digdat[n].expsn!=0)
      drawexplosion(n);
    else
      updatefire(n);
    if (digvisible)
      if (digdat[n].alive)
        if (digdat[n].bagtime!=0) {
          drawdig(n,digdat[n].mdir,digdat[n].x,digdat[n].y,
                  digdat[n].notfiring && digdat[n].rechargetime==0);
          incpenalty();
          digdat[n].bagtime--;
        }
        else
          updatedigger(n);
      else
        diggerdie(n);
    if (digdat[n].emocttime>0)
      digdat[n].emocttime--;
  }
  if (bonusmode && isalive()) {
    if (bonustimeleft!=0) {
      bonustimeleft--;
      if (startbonustimeleft!=0 || bonustimeleft<20) {
        startbonustimeleft--;
        if (bonustimeleft&1) {
          ginten(0);
          soundbonus();
        }
        else {
          ginten(1);
          soundbonus();
        }
        if (startbonustimeleft==0) {
          music(0);
          soundbonusoff();
          ginten(1);
        }
      }
    }
    else {
      endbonusmode();
      soundbonusoff();
      music(1);
    }
  }
  if (bonusmode && !isalive()) {
    endbonusmode();
    soundbonusoff();
    music(1);
  }
}

void updatefire(int n)
{
  Sint4 pix;
  int clfirst[TYPES],clcoll[SPRITES],i;
  bool clflag;
  if (digdat[n].notfiring) {
    if (digdat[n].rechargetime!=0)
      digdat[n].rechargetime--;
    else
      if (getfirepflag(n-curplayer))
        if (digdat[n].alive) {
          digdat[n].rechargetime=levof10()*3+60;
          digdat[n].notfiring=FALSE;
          switch (digdat[n].dir) {
            case DIR_RIGHT:
              digdat[n].fx=digdat[n].x+8;
              digdat[n].fy=digdat[n].y+4;
              break;
            case DIR_UP:
              digdat[n].fx=digdat[n].x+4;
              digdat[n].fy=digdat[n].y;
              break;
            case DIR_LEFT:
              digdat[n].fx=digdat[n].x;
              digdat[n].fy=digdat[n].y+4;
              break;
            case DIR_DOWN:
              digdat[n].fx=digdat[n].x+4;
              digdat[n].fy=digdat[n].y+8;
          }
          digdat[n].fdir=digdat[n].dir;
          movedrawspr(FIRSTFIREBALL+n-curplayer,digdat[n].fx,digdat[n].fy);
          soundfire(n);
        }
  }
  else {
    switch (digdat[n].fdir) {
      case DIR_RIGHT:
        digdat[n].fx+=8;
        pix=ggetpix(digdat[n].fx,digdat[n].fy+4)|
            ggetpix(digdat[n].fx+4,digdat[n].fy+4);
        break;
      case DIR_UP:
        digdat[n].fy-=7;
        pix=0;
        for (i=0;i<7;i++)
          pix|=ggetpix(digdat[n].fx+4,digdat[n].fy+i);
        pix&=0xc0;
        break;
      case DIR_LEFT:
        digdat[n].fx-=8;
        pix=ggetpix(digdat[n].fx,digdat[n].fy+4)|
            ggetpix(digdat[n].fx+4,digdat[n].fy+4);
        break;
      case DIR_DOWN:
        digdat[n].fy+=7;
        pix=0;
        for (i=0;i<7;i++)
          pix|=ggetpix(digdat[n].fx,digdat[n].fy+i);
        pix&=0x3;
        break;
    }
    drawfire(n-curplayer,digdat[n].fx,digdat[n].fy,0);
    for (i=0;i<TYPES;i++)
      clfirst[i]=first[i];
    for (i=0;i<SPRITES;i++)
      clcoll[i]=coll[i];
    incpenalty();
    i=clfirst[2];
    while (i!=-1) {
      killmon(i-FIRSTMONSTER);
      scorekill(n);
      digdat[n].expsn=1;
      i=clcoll[i];
    }
    i=clfirst[4];
    while (i!=-1) {
      if (i-FIRSTDIGGER+curplayer!=n && !digdat[i-FIRSTDIGGER+curplayer].invin
          && digdat[i-FIRSTDIGGER+curplayer].alive) {
        killdigger(i-FIRSTDIGGER+curplayer,3,0);
        digdat[n].expsn=1;
      }
      i=clcoll[i];
    }
    if (clfirst[0]!=-1 || clfirst[1]!=-1 || clfirst[2]!=-1 || clfirst[3]!=-1 ||
        clfirst[4]!=-1)
      clflag=TRUE;
    else
      clflag=FALSE;
    if (clfirst[0]!=-1 || clfirst[1]!=-1 || clfirst[3]!=-1) {
      digdat[n].expsn=1;
      i=clfirst[3];
      while (i!=-1) {
        if (digdat[i-FIRSTFIREBALL+curplayer].expsn==0)
          digdat[i-FIRSTFIREBALL+curplayer].expsn=1;
        i=clcoll[i];
      }
    }
    switch (digdat[n].fdir) {
      case DIR_RIGHT:
        if (digdat[n].fx>296)
          digdat[n].expsn=1;
        else
          if (pix!=0 && !clflag) {
            digdat[n].expsn=1;
            digdat[n].fx-=8;
            drawfire(n-curplayer,digdat[n].fx,digdat[n].fy,0);
          }
        break;
      case DIR_UP:
        if (digdat[n].fy<15)
          digdat[n].expsn=1;
        else
          if (pix!=0 && !clflag) {
            digdat[n].expsn=1;
            digdat[n].fy+=7;
            drawfire(n-curplayer,digdat[n].fx,digdat[n].fy,0);
          }
        break;
      case DIR_LEFT:
        if (digdat[n].fx<16)
          digdat[n].expsn=1;
        else
          if (pix!=0 && !clflag) {
            digdat[n].expsn=1;
            digdat[n].fx+=8;
            drawfire(n-curplayer,digdat[n].fx,digdat[n].fy,0);
          }
        break;
      case DIR_DOWN:
        if (digdat[n].fy>183)
          digdat[n].expsn=1;
        else
          if (pix!=0 && !clflag) {
            digdat[n].expsn=1;
            digdat[n].fy-=7;
            drawfire(n-curplayer,digdat[n].fx,digdat[n].fy,0);
          }
    }
  }
}

void erasediggers(void)
{
  int i;
  for (i=0;i<diggers;i++)
    erasespr(FIRSTDIGGER+i);
  digvisible=FALSE;
}

void drawexplosion(int n)
{
  switch (digdat[n].expsn) {
    case 1:
      soundexplode(n);
    case 2:
    case 3:
      drawfire(n-curplayer,digdat[n].fx,digdat[n].fy,digdat[n].expsn);
      incpenalty();
      digdat[n].expsn++;
      break;
    default:
      killfire(n);
      digdat[n].expsn=0;
  }
}

void killfire(int n)
{
  if (!digdat[n].notfiring) {
    digdat[n].notfiring=TRUE;
    erasespr(FIRSTFIREBALL+n-curplayer);
    soundfireoff(n);
  }
}

void updatedigger(int n)
{
  Sint4 dir,ddir,diggerox,diggeroy,nmon;
  bool push=TRUE,bagf;
  int clfirst[TYPES],clcoll[SPRITES],i;
  readdir(n-curplayer);
  dir=getdir(n-curplayer);
  if (dir==DIR_RIGHT || dir==DIR_UP || dir==DIR_LEFT || dir==DIR_DOWN)
    ddir=dir;
  else
    ddir=DIR_NONE;
  if (digdat[n].rx==0 && (ddir==DIR_UP || ddir==DIR_DOWN))
    digdat[n].dir=digdat[n].mdir=ddir;
  if (digdat[n].ry==0 && (ddir==DIR_RIGHT || ddir==DIR_LEFT))
    digdat[n].dir=digdat[n].mdir=ddir;
  if (dir==DIR_NONE)
    digdat[n].mdir=DIR_NONE;
  else
    digdat[n].mdir=digdat[n].dir;
  if ((digdat[n].x==292 && digdat[n].mdir==DIR_RIGHT) ||
      (digdat[n].x==12 && digdat[n].mdir==DIR_LEFT) ||
      (digdat[n].y==180 && digdat[n].mdir==DIR_DOWN) ||
      (digdat[n].y==18 && digdat[n].mdir==DIR_UP))
    digdat[n].mdir=DIR_NONE;
  diggerox=digdat[n].x;
  diggeroy=digdat[n].y;
  if (digdat[n].mdir!=DIR_NONE)
    eatfield(diggerox,diggeroy,digdat[n].mdir);
  switch (digdat[n].mdir) {
    case DIR_RIGHT:
      drawrightblob(digdat[n].x,digdat[n].y);
      digdat[n].x+=4;
      break;
    case DIR_UP:
      drawtopblob(digdat[n].x,digdat[n].y);
      digdat[n].y-=3;
      break;
    case DIR_LEFT:
      drawleftblob(digdat[n].x,digdat[n].y);
      digdat[n].x-=4;
      break;
    case DIR_DOWN:
      drawbottomblob(digdat[n].x,digdat[n].y);
      digdat[n].y+=3;
      break;
  }
  if (hitemerald((digdat[n].x-12)/20,(digdat[n].y-18)/18,
                 (digdat[n].x-12)%20,(digdat[n].y-18)%18,
                 digdat[n].mdir)) {
    if (digdat[n].emocttime==0)
      digdat[n].emn=0;
    scoreemerald(n);
    soundem();
    soundemerald(digdat[n].emn);

    digdat[n].emn++;
    if (digdat[n].emn==8) {
      digdat[n].emn=0;
      scoreoctave(n);
    }
    digdat[n].emocttime=9;
  }
  drawdig(n,digdat[n].dir,digdat[n].x,digdat[n].y,
          digdat[n].notfiring && digdat[n].rechargetime==0);
  for (i=0;i<TYPES;i++)
    clfirst[i]=first[i];
  for (i=0;i<SPRITES;i++)
    clcoll[i]=coll[i];
  incpenalty();

  i=clfirst[1];
  bagf=FALSE;
  while (i!=-1) {
    if (bagexist(i-FIRSTBAG)) {
      bagf=TRUE;
      break;
    }
    i=clcoll[i];
  }

  if (bagf) {
    if (digdat[n].mdir==DIR_RIGHT || digdat[n].mdir==DIR_LEFT) {
      push=pushbags(digdat[n].mdir,clfirst,clcoll);
      digdat[n].bagtime++;
    }
    else
      if (!pushudbags(clfirst,clcoll))
        push=FALSE;
    if (!push) { /* Strange, push not completely defined */
      digdat[n].x=diggerox;
      digdat[n].y=diggeroy;
      drawdig(n,digdat[n].mdir,digdat[n].x,digdat[n].y,
              digdat[n].notfiring && digdat[n].rechargetime==0);
      incpenalty();
      digdat[n].dir=reversedir(digdat[n].mdir);
    }
  }
  if (clfirst[2]!=-1 && bonusmode && digdat[n].alive)
    for (nmon=killmonsters(clfirst,clcoll);nmon!=0;nmon--) {
      soundeatm();
      sceatm(n);
    }
  if (clfirst[0]!=-1) {
    scorebonus(n);
    initbonusmode();
  }
  digdat[n].h=(digdat[n].x-12)/20;
  digdat[n].rx=(digdat[n].x-12)%20;
  digdat[n].v=(digdat[n].y-18)/18;
  digdat[n].ry=(digdat[n].y-18)%18;
}

void sceatm(int n)
{
  scoreeatm(n,digdat[n].msc);
  digdat[n].msc<<=1;
}

Sint4 deatharc[7]={3,5,6,6,5,3,0};

void diggerdie(int n)
{
  int clfirst[TYPES],clcoll[SPRITES],i;
  bool alldead;
  switch (digdat[n].deathstage) {
    case 1:
      if (bagy(digdat[n].deathbag)+6>digdat[n].y)
        digdat[n].y=bagy(digdat[n].deathbag)+6;
      drawdigger(n-curplayer,15,digdat[n].x,digdat[n].y,FALSE);
      incpenalty();
      if (getbagdir(digdat[n].deathbag)+1==0) {
        soundddie();
        digdat[n].deathtime=5;
        digdat[n].deathstage=2;
        digdat[n].deathani=0;
        digdat[n].y-=6;
      }
      break;
    case 2:
      if (digdat[n].deathtime!=0) {
        digdat[n].deathtime--;
        break;
      }
      if (digdat[n].deathani==0)
        music(2);
      drawdigger(n-curplayer,14-digdat[n].deathani,digdat[n].x,digdat[n].y,
                 FALSE);
      for (i=0;i<TYPES;i++)
        clfirst[i]=first[i];
      for (i=0;i<SPRITES;i++)
        clcoll[i]=coll[i];
      incpenalty();
      if (digdat[n].deathani==0 && clfirst[2]!=-1)
        killmonsters(clfirst,clcoll);
      if (digdat[n].deathani<4) {
        digdat[n].deathani++;
        digdat[n].deathtime=2;
      }
      else {
        digdat[n].deathstage=4;
        if (musicflag || diggers>1)
          digdat[n].deathtime=60;
        else
          digdat[n].deathtime=10;
      }
      break;
    case 3:
      digdat[n].deathstage=5;
      digdat[n].deathani=0;
      digdat[n].deathtime=0;
      break;
    case 5:
      if (digdat[n].deathani>=0 && digdat[n].deathani<=6) {
        drawdigger(n-curplayer,15,digdat[n].x,
                   digdat[n].y-deatharc[digdat[n].deathani],FALSE);
        if (digdat[n].deathani==6 && !isalive())
          musicoff();
        incpenalty();
        digdat[n].deathani++;
        if (digdat[n].deathani==1)
          soundddie();
        if (digdat[n].deathani==7) {
          digdat[n].deathtime=5;
          digdat[n].deathani=0;
          digdat[n].deathstage=2;
        }
      }
      break;
    case 4:
      if (digdat[n].deathtime!=0)
        digdat[n].deathtime--;
      else {
        digdat[n].dead=TRUE;
        alldead=TRUE;
        for (i=0;i<diggers;i++)
          if (!digdat[i].dead) {
            alldead=FALSE;
            break;
          }
        if (alldead)
          setdead(TRUE);
        else
          if (isalive() && digdat[n].lives>0) {
            if (!gauntlet)
              digdat[n].lives--;
            drawlives();
            if (digdat[n].lives>0) {
              digdat[n].v=9;
              digdat[n].mdir=4;
              digdat[n].h=(diggers==1) ? 7 : (8-n*2);
              digdat[n].x=digdat[n].h*20+12;
              digdat[n].dir=(n==0) ? DIR_RIGHT : DIR_LEFT;
              digdat[n].rx=0;
              digdat[n].ry=0;
              digdat[n].bagtime=0;
              digdat[n].alive=TRUE;
              digdat[n].dead=FALSE;
              digdat[n].invin=TRUE;
              digdat[n].ivt=50;
              digdat[n].deathstage=1;
              digdat[n].y=digdat[n].v*18+18;
              erasespr(n+FIRSTDIGGER-curplayer);
              movedrawspr(n+FIRSTDIGGER-curplayer,digdat[n].x,digdat[n].y);
              digdat[n].notfiring=TRUE;
              digdat[n].emocttime=0;
              digdat[n].firepressed=FALSE;
              digdat[n].expsn=0;
              digdat[n].rechargetime=0;
              digdat[n].emn=0;
              digdat[n].msc=1;
            }
            clearfire(n);
            if (bonusmode)
              music(0);
            else
              music(1);
          }
      }
  }
}

void createbonus(void)
{
  bonusvisible=TRUE;
  drawbonus(292,18);
}

void initbonusmode(void)
{
  int i;
  bonusmode=TRUE;
  erasebonus();
  ginten(1);
  bonustimeleft=250-levof10()*20;
  startbonustimeleft=20;
  for (i=0;i<diggers;i++)
    digdat[i].msc=1;
}

void endbonusmode(void)
{
  bonusmode=FALSE;
  ginten(0);
}

void erasebonus(void)
{
  if (bonusvisible) {
    bonusvisible=FALSE;
    erasespr(FIRSTBONUS);
  }
  ginten(0);
}

Sint4 reversedir(Sint4 dir)
{
  switch (dir) {
    case DIR_RIGHT: return DIR_LEFT;
    case DIR_LEFT: return DIR_RIGHT;
    case DIR_UP: return DIR_DOWN;
    case DIR_DOWN: return DIR_UP;
  }
  return dir;
}

bool checkdiggerunderbag(Sint4 h,Sint4 v)
{
  int n;
  for (n=curplayer;n<diggers+curplayer;n++)
    if (digdat[n].alive)
      if (digdat[n].mdir==DIR_UP || digdat[n].mdir==DIR_DOWN)
        if ((digdat[n].x-12)/20==h)
          if ((digdat[n].y-18)/18==v || (digdat[n].y-18)/18+1==v)
            return TRUE;
  return FALSE;
}

void killdigger(int n,Sint4 stage,Sint4 bag)
{
  if (digdat[n].invin)
    return;
  if (digdat[n].deathstage<2 || digdat[n].deathstage>4) {
    digdat[n].alive=FALSE;
    digdat[n].deathstage=stage;
    digdat[n].deathbag=bag;
  }
}

void makeemfield(void)
{
  Sint4 x,y;
  emmask=1<<curplayer;
  for (x=0;x<MWIDTH;x++)
    for (y=0;y<MHEIGHT;y++)
      if (getlevch(x,y,levplan())=='C')
        emfield[y*MWIDTH+x]|=emmask;
      else
        emfield[y*MWIDTH+x]&=~emmask;
}

void drawemeralds(void)
{
  Sint4 x,y;
  emmask=1<<curplayer;
  for (x=0;x<MWIDTH;x++)
    for (y=0;y<MHEIGHT;y++)
      if (emfield[y*MWIDTH+x]&emmask)
        drawemerald(x*20+12,y*18+21);
}

Sint4 embox[8]={8,12,12,9,16,12,6,9};

bool hitemerald(Sint4 x,Sint4 y,Sint4 rx,Sint4 ry,Sint4 dir)
{
  bool hit=FALSE;
  Sint4 r;
  if (dir!=DIR_RIGHT && dir!=DIR_UP && dir!=DIR_LEFT && dir!=DIR_DOWN)
    return hit;
  if (dir==DIR_RIGHT && rx!=0)
    x++;
  if (dir==DIR_DOWN && ry!=0)
    y++;
  if (dir==DIR_RIGHT || dir==DIR_LEFT)
    r=rx;
  else
    r=ry;
  if (emfield[y*MWIDTH+x]&emmask) {
    if (r==embox[dir]) {
      drawemerald(x*20+12,y*18+21);
      incpenalty();
    }
    if (r==embox[dir+1]) {
      eraseemerald(x*20+12,y*18+21);
      incpenalty();
      hit=TRUE;
      emfield[y*MWIDTH+x]&=~emmask;
    }
  }
  return hit;
}

Sint4 countem(void)
{
  Sint4 x,y,n=0;
  for (x=0;x<MWIDTH;x++)
    for (y=0;y<MHEIGHT;y++)
      if (emfield[y*MWIDTH+x]&emmask)
        n++;
  return n;
}

void killemerald(Sint4 x,Sint4 y)
{
  if (emfield[(y+1)*MWIDTH+x]&emmask) {
    emfield[(y+1)*MWIDTH+x]&=~emmask;
    eraseemerald(x*20+12,(y+1)*18+21);
  }
}

bool getfirepflag(int n)
{
  return n==0 ? firepflag : fire2pflag;
}

int diggerx(int n)
{
  return digdat[n].x;
}

int diggery(int n)
{
  return digdat[n].y;
}

bool digalive(int n)
{
  return digdat[n].alive;
}

void digresettime(int n)
{
  digdat[n].bagtime=0;
}

bool isalive(void)
{
  int i;
  for (i=curplayer;i<diggers+curplayer;i++)
    if (digdat[i].alive)
      return TRUE;
  return FALSE;
}

int getlives(int pl)
{
  return digdat[pl].lives;
}

void addlife(int pl)
{
  digdat[pl].lives++;
  sound1up();
}

void initlives(void)
{
  int i;
  for (i=0;i<diggers+nplayers-1;i++)
    digdat[i].lives=3;
}

void declife(int pl)
{
  if (!gauntlet)
    digdat[pl].lives--;
}
