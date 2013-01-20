/* Digger Remastered
   Copyright (c) Andrew Jenner 1998-2004 */

#include "def.h"
#include "main.h"
#include "sound.h"
#include "hardware.h"
#include "record.h"
#include "digger.h"
#ifdef _WINDOWS
#include "win_dig.h"
#endif

bool escape=false,firepflag=false,aleftpressed=false,arightpressed=false,
     auppressed=false,adownpressed=false,start=false,af1pressed=false;
bool fire2pflag=false,aleft2pressed=false,aright2pressed=false,
     aup2pressed=false,adown2pressed=false,af12pressed=false;

Sint4 akeypressed;

Sint4 dynamicdir=-1,dynamicdir2=-1,staticdir=-1,staticdir2=-1,joyx=0,joyy=0;

bool joybut1=false,joybut2=false;

Sint4 keydir=0,keydir2=0,jleftthresh=0,jupthresh=0,jrightthresh=0,
      jdownthresh=0,joyanax=0,joyanay=0;

bool joyflag=false,pausef=false;

bool krdf[17]={false,false,false,false,false,false,false,false,false,false,
               false,false,false,false,false,false,false};


#ifdef ARM

#include "C:kernel.h"
#define rightpressed (_kernel_osbyte(129,keycodes[0][0],255)&0xff==0xff)
#define uppressed (_kernel_osbyte(129,keycodes[1][0],255)&0xff==0xff)
#define leftpressed (_kernel_osbyte(129,keycodes[2][0],255)&0xff==0xff)
#define downpressed (_kernel_osbyte(129,keycodes[3][0],255)&0xff==0xff)
#define f1pressed (_kernel_osbyte(129,keycodes[4][0],255)&0xff==0xff)
#define right2pressed (_kernel_osbyte(129,keycodes[5][0],255)&0xff==0xff)
#define up2pressed (_kernel_osbyte(129,keycodes[6][0],255)&0xff==0xff)
#define left2pressed (_kernel_osbyte(129,keycodes[7][0],255)&0xff==0xff)
#define down2pressed (_kernel_osbyte(129,keycodes[8][0],255)&0xff==0xff)
#define f12pressed (_kernel_osbyte(129,keycodes[9][0],255)&0xff==0xff)

/* Default key codes for ARM */

int keycodes[17][5]={{134,0,-2,-2,-2},   /* 1 Right */
                     {198,0,-2,-2,-2},   /* 1 Up */
                     {230,0,-2,-2,-2},   /* 1 Left */
                     {214,0,-2,-2,-2},   /* 1 Down */
                     {142,0,-2,-2,-2},   /* 1 Fire */
                     {134,0,-2,-2,-2},   /* 2 Right */
                     {198,0,-2,-2,-2},   /* 2 Up */
                     {230,0,-2,-2,-2},   /* 2 Left */
                     {214,0,-2,-2,-2},   /* 2 Down */
                     {142,0,-2,-2,-2},   /* 2 Fire */
                     {20,-2,-2,-2,-2},   /* Cheat */
                     {43,-2,-2,-2,-2},   /* Accelerate */
                     {45,-2,-2,-2,-2},   /* Brake */
                     {137,-2,-2,-2,-2},  /* Music */
                     {139,-2,-2,-2,-2},  /* Sound */
                     {140,-2,-2,-2,-2},  /* Exit */
                     {32,-2,-2,-2,-2}};  /* Pause */

#define ASCIIF8 138

/* This function exclusively used in keyboard redefinition */
void findkey(int kn)
{
  int k=0,i,j;
  bool f=false;
  do {
    for (i=130;i<256 && !f;i++)
      if (_kernel_osbyte(129,i,255)&0xff==0xff)
        f=true;
    gretrace();
    if (kbhit())
      k=getkey();
  } while (k==0 && !f);
  j=i-1;
  if (k==0) k=-2;
  if (k>='a' && k<='z')
    k-='a'-'A';
  for (i=0;i<5;i++)
    keycodes[kn][i]=-2;
  if (kn>9)
    i=0;
  else {
    i=2;
    keycodes[kn][0]=j;
    keycodes[kn][1]=0;
  }
  keycodes[kn][i++]=k;
  if (k>='A' && k<='Z') {
    keycodes[kn][i++]=k-('A'-'a'); /* lower case */
    keycodes[kn][i++]=k-'@'; /* ctrl code */
  }
  krdf[kn]=true;
}

#else

#ifdef _WINDOWS

#define rightpressed  (GetAsyncKeyState(keycodes[0][0]) & 0x8000)
#define uppressed     (GetAsyncKeyState(keycodes[1][0]) & 0x8000)
#define leftpressed   (GetAsyncKeyState(keycodes[2][0]) & 0x8000)
#define downpressed   (GetAsyncKeyState(keycodes[3][0]) & 0x8000)
#define f1pressed     (GetAsyncKeyState(keycodes[4][0]))
#define right2pressed (GetAsyncKeyState(keycodes[5][0]) & 0x8000)
#define up2pressed    (GetAsyncKeyState(keycodes[6][0]) & 0x8000)
#define left2pressed  (GetAsyncKeyState(keycodes[7][0]) & 0x8000)
#define down2pressed  (GetAsyncKeyState(keycodes[8][0]) & 0x8000)
#define f12pressed    (GetAsyncKeyState(keycodes[9][0]))

int keycodes[17][5]={{VK_RIGHT,VK_RIGHT+0x80,0x14d,-2,-2}, /* 1 Right */
                     {VK_UP,VK_UP+0x80,-2,-2},             /* 1 Up */
                     {VK_LEFT,VK_LEFT+0x80,0x14b,-2,-2},   /* 1 Left */
                     {VK_DOWN,VK_DOWN+0x80,0x150,-2,-2},   /* 1 Down */
                     {VK_F1,VK_F1+0x80,0x13b,-2,-2},       /* 1 Fire */
                     {'S','S'+0x80,83,115,19},             /* 2 Right */
                     {'W','W'+0x80,87,119,23},             /* 2 Up */
                     {'A','A'+0x80,65,97,1},               /* 2 Left */
                     {'Z','Z'+0x80,90,122,26},             /* 2 Down */
                     {15,143,9,-2,-2},                     /* 2 Fire */
                     {'T',-2,-2,-2,-2},                    /* Cheat */
                     {VK_ADD,-2,-2,-2,-2},                 /* Accelerate */
                     {VK_SUBTRACT,-2,-2,-2,-2},            /* Brake */
                     {VK_F7,-2,-2,-2,-2},                  /* Music */
                     {VK_F9,-2,-2,-2,-2},                  /* Sound */
                     {VK_F10,-2,-2,-2,-2},                 /* Exit */
                     {VK_SPACE,-2,-2,-2,-2}};              /* Pause */

#define ASCIIF8 VK_F8

#else

bool leftpressed=false,rightpressed=false,uppressed=false,downpressed=false,
     f1pressed=false,left2pressed=false,right2pressed=false,up2pressed=false,
     down2pressed=false,f12pressed=false;

/* Default key codes */

int keycodes[17][5]={{0x4d,0xcd,0x14d,-2,-2}, /* 1 Right */
                     {0x48,0xc8,0x148,-2,-2}, /* 1 Up */
                     {0x4b,0xcb,0x14b,-2,-2}, /* 1 Left */
                     {0x50,0xd0,0x150,-2,-2}, /* 1 Down */
                     {0x3b,0xbb,0x13b,-2,-2}, /* 1 Fire */
                     {31,159,83,115,19},      /* 2 Right */
                     {17,145,87,119,23},      /* 2 Up */
                     {30,158,65,97,1},        /* 2 Left */
                     {44,172,90,122,26},      /* 2 Down */
                     {15,143,9,-2,-2},        /* 2 Fire */
                     {20,-2,-2,-2,-2},        /* Cheat */
                     {43,-2,-2,-2,-2},        /* Accelerate */
                     {45,-2,-2,-2,-2},        /* Brake */
                     {321,-2,-2,-2,-2},       /* Music */
                     {323,-2,-2,-2,-2},       /* Sound */
                     {324,-2,-2,-2,-2},       /* Exit */
                     {32,-2,-2,-2,-2}};       /* Pause */

#define ASCIIF8 322

#endif

Uint4 scancode;

int pki;

#ifndef _WINDOWS
bool *flagp[10]={
  &rightpressed,&uppressed,&leftpressed,&downpressed,&f1pressed,
  &right2pressed,&up2pressed,&left2pressed,&down2pressed,&f12pressed};

/* We need to know when keys are released so we know when to stop.
   This routine is only called on platforms where keyboard makes and breaks
   cause interrupts (this being the handler). On platforms where makes and
   breaks set and release flags, these "variables" are actually macros linking
   to these flags (they are each only read once).
*/
void processkey(Uint4 key)
{
  for (pki=0;pki<10;pki++) {
    if (key==keycodes[pki][0]) /* Make */
      *flagp[pki]=true;
    if (key==keycodes[pki][1]) /* Break */
      *flagp[pki]=false;
  }
  scancode=key;
}
#endif

/* This function exclusively used in keyboard redefinition */
void findkey(int kn)
{
  int k=0,i;
  scancode=0;
  do {
    if (kbhit())
      k=getkey();
#ifdef _WINDOWS
    do_windows_events();
#endif
  } while (k==0 && (scancode==0 || scancode&0x80));
  if (kbhit())
    k=getkey();
  if (k==0)
    k=-2;
  if (k>='a' && k<='z')
    k-='a'-'A';
  for (i=0;i<5;i++)
    keycodes[kn][i]=-2;
  if (kn>9)
    i=0;
  else {
    i=2;
    keycodes[kn][0]=scancode&0x7f;
    keycodes[kn][1]=scancode|0x80;
  }
  keycodes[kn][i++]=k;
  if (k>='A' && k<='Z') {
    keycodes[kn][i++]=k-('A'-'a'); /* lower case */
    keycodes[kn][i]=k-'@'; /* ctrl code */
  }
  krdf[kn]=true;
}

#endif


void readjoy(void);

/* The standard ASCII keyboard is also checked so that very short keypresses
   are not overlooked. The functions kbhit() (returns bool denoting whether or
   not there is a key in the buffer) and getkey() (wait until a key is in the
   buffer, then return it) are used. These functions are emulated on platforms
   which only provide an inkey() function (return the key in the buffer, unless
   there is none, in which case return -1. It is done this way around for
   historical reasons, there is no fundamental reason why it shouldn't be the
   other way around. */
void checkkeyb(void)
{
  int i,j,k=0;
  bool *aflagp[10]={&arightpressed,&auppressed,&aleftpressed,&adownpressed,
                    &af1pressed,&aright2pressed,&aup2pressed,&aleft2pressed,
                    &adown2pressed,&af12pressed};
#ifdef _WINDOWS
//  readjoy();
//  if (joybut1)
//    firepflag=true;
//  else
//    firepflag=false;
#endif
  if (leftpressed)
    aleftpressed=true;
  if (rightpressed)
    arightpressed=true;
  if (uppressed)
    auppressed=true;
  if (downpressed)
    adownpressed=true;
  if (f1pressed)
    af1pressed=true;
  if (left2pressed)
    aleft2pressed=true;
  if (right2pressed)
    aright2pressed=true;
  if (up2pressed)
    aup2pressed=true;
  if (down2pressed)
    adown2pressed=true;
  if (f12pressed)
    af12pressed=true;

  while (kbhit()) {
    akeypressed=getkey();
    for (i=0;i<10;i++)
      for (j=2;j<5;j++)
        if (akeypressed==keycodes[i][j])
          *aflagp[i]=true;
    for (i=10;i<17;i++)
      for (j=0;j<5;j++)
        if (akeypressed==keycodes[i][j])
          k=i;
    switch (k) {
      case 10: /* Cheat! */
        if (playing) {      /* Why was this "!gauntlet"? */
          playing=false;
          drfvalid=false;
        }
        break;
      case 11: /* Increase speed */
        if (ftime>10000l)
          ftime-=10000l;
        break;
      case 12: /* Decrease speed */
        ftime+=10000l;
        break;
      case 13: /* Toggle music */
        musicflag=!musicflag;
        break;
      case 14: /* Toggle sound */
        soundflag=!soundflag;
        break;
      case 15: /* Exit */
        escape=true;
        break;
      case 16: /* Pause */
        pausef=true;
    }
    if (akeypressed==ASCIIF8) /* Save DRF */
      savedrf=true;
    if (akeypressed!=27 && akeypressed!='n' && akeypressed!='N')
      start=true;                                /* Change number of players */
  }
}

/* Joystick not yet implemented. It will be, though, using gethrt on platform
   DOSPC. */
void readjoy(void)
{
#ifdef _WINDOWS
  JOYINFO ji;
  if (joyGetPos(0,&ji)==JOYERR_NOERROR)
  {
    joyx=(Sint4) (ji.wXpos / 655);
    joyy=(Sint4) (ji.wYpos / 655);
    joybut1=ji.wButtons & JOY_BUTTON1;
    joybut2=ji.wButtons & JOY_BUTTON2;
  }
#endif
}

void detectjoy(void)
{
#ifdef _WINDOWS
  init_joystick();
#else
  joyflag=false;
#endif
  staticdir=dynamicdir=DIR_NONE;
}

/* Contrary to some beliefs, you don't need a separate OS call to flush the
   keyboard buffer. */
void flushkeybuf(void)
{
  while (kbhit())
    getkey();
  aleftpressed=arightpressed=auppressed=adownpressed=af1pressed=false;
  aleft2pressed=aright2pressed=aup2pressed=adown2pressed=af12pressed=false;
}

void clearfire(int n)
{
  if (n==0)
    af1pressed=false;
  else
    af12pressed=false;
}

bool oupressed=false,odpressed=false,olpressed=false,orpressed=false;
bool ou2pressed=false,od2pressed=false,ol2pressed=false,or2pressed=false;

void readdir(int n)
{
  Sint4 j;
  bool u=false,d=false,l=false,r=false;
  bool u2=false,d2=false,l2=false,r2=false;

#ifdef _WINDOWS
  firepflag=false;
#endif
  if (n==0) {
    if (auppressed || uppressed) { u=true; auppressed=false; }
    if (adownpressed || downpressed) { d=true; adownpressed=false; }
    if (aleftpressed || leftpressed) { l=true; aleftpressed=false; }
    if (arightpressed || rightpressed) { r=true; arightpressed=false; }
    if (f1pressed || af1pressed) {
      firepflag=true;
      af1pressed=false;
    }
#ifndef _WINDOWS
    else
      firepflag=false;
#endif
    if (u && !oupressed)
      staticdir=dynamicdir=DIR_UP;
    if (d && !odpressed)
      staticdir=dynamicdir=DIR_DOWN;
    if (l && !olpressed)
      staticdir=dynamicdir=DIR_LEFT;
    if (r && !orpressed)
      staticdir=dynamicdir=DIR_RIGHT;
    if ((oupressed && !u && dynamicdir==DIR_UP) ||
        (odpressed && !d && dynamicdir==DIR_DOWN) ||
        (olpressed && !l && dynamicdir==DIR_LEFT) ||
        (orpressed && !r && dynamicdir==DIR_RIGHT)) {
      dynamicdir=DIR_NONE;
      if (u) dynamicdir=staticdir=2;
      if (d) dynamicdir=staticdir=6;
      if (l) dynamicdir=staticdir=4;
      if (r) dynamicdir=staticdir=0;
    }
    oupressed=u;
    odpressed=d;
    olpressed=l;
    orpressed=r;
    keydir=staticdir;
    if (dynamicdir!=DIR_NONE)
      keydir=dynamicdir;
    staticdir=DIR_NONE;
  }
  else {
    if (aup2pressed || up2pressed) { u2=true; aup2pressed=false; }
    if (adown2pressed || down2pressed) { d2=true; adown2pressed=false; }
    if (aleft2pressed || left2pressed) { l2=true; aleft2pressed=false; }
    if (aright2pressed || right2pressed) { r2=true; aright2pressed=false; }
    if (f12pressed || af12pressed) {
      fire2pflag=true;
      af12pressed=false;
    }
    else
      fire2pflag=false;
    if (u2 && !ou2pressed)
      staticdir2=dynamicdir2=DIR_UP;
    if (d2 && !od2pressed)
      staticdir2=dynamicdir2=DIR_DOWN;
    if (l2 && !ol2pressed)
      staticdir2=dynamicdir2=DIR_LEFT;
    if (r2 && !or2pressed)
      staticdir2=dynamicdir2=DIR_RIGHT;
    if ((ou2pressed && !u2 && dynamicdir2==DIR_UP) ||
        (od2pressed && !d2 && dynamicdir2==DIR_DOWN) ||
        (ol2pressed && !l2 && dynamicdir2==DIR_LEFT) ||
        (or2pressed && !r2 && dynamicdir2==DIR_RIGHT)) {
      dynamicdir2=DIR_NONE;
      if (u2) dynamicdir2=staticdir2=2;
      if (d2) dynamicdir2=staticdir2=6;
      if (l2) dynamicdir2=staticdir2=4;
      if (r2) dynamicdir2=staticdir2=0;
    }
    ou2pressed=u2;
    od2pressed=d2;
    ol2pressed=l2;
    or2pressed=r2;
    keydir2=staticdir2;
    if (dynamicdir2!=DIR_NONE)
      keydir2=dynamicdir2;
    staticdir2=DIR_NONE;
  }
#ifdef _WINDOWS
  readjoy();
  if (joybut1)
    firepflag=true;
#else
  if (joyflag) {
    incpenalty();
    incpenalty();
    joyanay=0;
    joyanax=0;
    for (j=0;j<4;j++) {
      readjoy();
      joyanax+=joyx;
      joyanay+=joyy;
    }
    joyx=joyanax>>2;
    joyy=joyanay>>2;
    if (joybut1)
      firepflag=true;
    else
      firepflag=false;
  }
#endif
}

/* Calibrate joystick while waiting at title screen. This works more
   effectively if the user waggles the joystick in the title screen. */
bool teststart(void)
{
  Sint4 j;
  bool startf=false;
  if (joyflag) {
    readjoy();
    if (joybut1)
      startf=true;
  }
  if (start) {
    start=false;
    startf=true;
#ifndef _WINDOWS
    joyflag=false;
#endif
  }
  if (!startf)
    return false;
  if (joyflag) {
    joyanay=0;
    joyanax=0;
    for (j=0;j<50;j++) {
      readjoy();
      joyanax+=joyx;
      joyanay+=joyy;
    }
    joyx=joyanax/50;
    joyy=joyanay/50;
    jleftthresh=joyx-35;
    if (jleftthresh<0)
      jleftthresh=0;
    jleftthresh+=10;
    jupthresh=joyy-35;
    if (jupthresh<0)
      jupthresh=0;
    jupthresh+=10;
    jrightthresh=joyx+35;
    if (jrightthresh>255)
      jrightthresh=255;
    jrightthresh-=10;
    jdownthresh=joyy+35;
    if (jdownthresh>255)
      jdownthresh=255;
    jdownthresh-=10;
    joyanax=joyx;
    joyanay=joyy;
  }
  return true;
}

/* Why the joystick reading is split between readdir and getdir like this is a
   mystery to me. */
Sint4 getdir(int n)
{
  Sint4 dir=((n==0) ? keydir : keydir2);
  if (joyflag) {
#ifdef _WINDOWS
  if (abs(joyx-50) > abs(joyy-50)) {
    if (joyx<jleftthresh)
      dir=DIR_LEFT;
    if (joyx>jrightthresh)
      dir=DIR_RIGHT;
  }
  else {
    if (joyy<jupthresh)
      dir=DIR_UP;
    if (joyy>jdownthresh)
      dir=DIR_DOWN;
  }
#else
    dir=DIR_NONE;
    if (joyx<jleftthresh)
      dir=DIR_LEFT;
    if (joyx>jrightthresh)
      dir=DIR_RIGHT;
    if (joyx>=jleftthresh && joyx<=jrightthresh) {
      if (joyy<jupthresh)
        dir=DIR_UP;
      if (joyy>jdownthresh)
        dir=DIR_DOWN;
    }
#endif
  }
  if (n==0) {
    if (playing)
      playgetdir(&dir,&firepflag);
    recputdir(dir,firepflag);
  }
  else {
    if (playing)
      playgetdir(&dir,&fire2pflag);
    recputdir(dir,fire2pflag);
  }
  return dir;
}
