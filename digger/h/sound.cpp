/* Digger Remastered
   Copyright (c) Andrew Jenner 1998-2004 */

#include "def.h"

#ifdef _WINDOWS
#include "win_dig.h"
#include "win_snd.h"
#endif

#include "sound.h"
#include "hardware.h"
#include "main.h"
#include "digger.h"
#include "input.h"

Sint4 wavetype=0,musvol=0;
Sint4 spkrmode=0,timerrate=0x7d0;
Uint4 timercount=0,t2val=0,t0val=0;
Sint4 pulsewidth=1;
Sint4 volume=0;

Sint3 timerclock=0;

bool soundflag=true,musicflag=true;

void soundint(void);
void soundlevdoneoff(void);
void soundlevdoneupdate(void);
void soundfallupdate(void);
void soundbreakoff(void);
void soundbreakupdate(void);
void soundwobbleupdate(void);
void soundfireupdate(void);
void soundexplodeoff(int n);
void soundexplodeupdate(void);
void soundbonusupdate(void);
void soundemoff(void);
void soundemupdate(void);
void soundemeraldoff(void);
void soundemeraldupdate(void);
void soundgoldoff(void);
void soundgoldupdate(void);
void soundeatmoff(void);
void soundeatmupdate(void);
void soundddieoff(void);
void soundddieupdate(void);
void sound1upoff(void);
void sound1upupdate(void);
void musicupdate(void);
void sett0(void);
void setsoundmode(void);
void s0setupsound(void);
void s0killsound(void);
void s0fillbuffer(void);

void (*setupsound)(void)=s0setupsound;
void (*killsound)(void)=s0killsound;
void (*fillbuffer)(void)=s0fillbuffer;
void (*initint8)(void)=s0initint8;
void (*restoreint8)(void)=s0restoreint8;
void (*soundoff)(void)=s0soundoff;
void (*setspkrt2)(void)=s0setspkrt2;
void (*settimer0)(Uint4 t0v)=s0settimer0;
void (*timer0)(Uint4 t0v)=s0timer0;
void (*settimer2)(Uint4 t2v)=s0settimer2;
void (*timer2)(Uint4 t2v)=s0timer2;
void (*soundkillglob)(void)=s0soundkillglob;

bool sndflag=false,soundpausedflag=false;

Sint5 randvs;

Sint4 randnos(Sint4 n)
{
  randvs=randvs*0x15a4e35l+1;
  return (Sint4)((randvs&0x7fffffffl)%n);
}

void sett2val(Sint4 t2v)
{
  if (sndflag)
    timer2(t2v);
}

void soundint(void)
{
  timerclock++;
  if (soundflag && !sndflag)
    sndflag=musicflag=true;
  if (!soundflag && sndflag) {
    sndflag=false;
    timer2(40);
    setsoundt2();
    soundoff();
  }
  if (sndflag && !soundpausedflag) {
    t0val=0x7d00;
    t2val=40;
    if (musicflag)
      musicupdate();
#ifdef ARM
    else
      soundoff();
#endif
    soundemeraldupdate();
    soundwobbleupdate();
    soundddieupdate();
    soundbreakupdate();
    soundgoldupdate();
    soundemupdate();
    soundexplodeupdate();
    soundfireupdate();
    soundeatmupdate();
    soundfallupdate();
    sound1upupdate();
    soundbonusupdate();
    if (t0val==0x7d00 || t2val!=40)
      setsoundt2();
    else {
      setsoundmode();
      sett0();
    }
    sett2val(t2val);
  }
}

void soundstop(void)
{
  int i;
  soundfalloff();
  soundwobbleoff();
  for (i=0;i<FIREBALLS;i++)
    soundfireoff(i);
  musicoff();
  soundbonusoff();
  for (i=0;i<FIREBALLS;i++)
    soundexplodeoff(i);
  soundbreakoff();
  soundemoff();
  soundemeraldoff();
  soundgoldoff();
  soundeatmoff();
  soundddieoff();
  sound1upoff();
}


bool soundlevdoneflag=false;
Sint4 nljpointer=0,nljnoteduration=0;

void soundlevdone(void)
{
  Sint4 timer=0;
  soundstop();
  nljpointer=0;
  nljnoteduration=20;
  soundlevdoneflag=soundpausedflag=true;
  while (soundlevdoneflag && !escape) {
    fillbuffer();
#ifdef _WINDOWS
    do_windows_events();
    if (!wave_device_available)
      soundlevdoneflag=false;
#endif
#ifdef ARM
    gretrace();
    soundint();
#else
    if (timerclock==timer)
      continue;
#endif
    soundlevdoneupdate();
    checkkeyb();
    timer=timerclock;
  }
  soundlevdoneoff();
}

void soundlevdoneoff(void)
{
  soundlevdoneflag=soundpausedflag=false;
}

Sint4 newlevjingle[11]={0x8e8,0x712,0x5f2,0x7f0,0x6ac,0x54c,
                        0x712,0x5f2,0x4b8,0x474,0x474};

void soundlevdoneupdate(void)
{
  if (sndflag) {
    if (nljpointer<11)
      t2val=newlevjingle[nljpointer];
    t0val=t2val+35;
    musvol=50;
    setsoundmode();
    sett0();
    sett2val(t2val);
    if (nljnoteduration>0)
      nljnoteduration--;
    else {
      nljnoteduration=20;
      nljpointer++;
      if (nljpointer>10)
        soundlevdoneoff();
    }
  }
  else
    soundlevdoneflag=false;
}


bool soundfallflag=false,soundfallf=false;
Sint4 soundfallvalue,soundfalln=0;

void soundfall(void)
{
  soundfallvalue=1000;
  soundfallflag=true;
}

void soundfalloff(void)
{
  soundfallflag=false;
  soundfalln=0;
}

void soundfallupdate(void)
{
  if (soundfallflag)
    if (soundfalln<1) {
      soundfalln++;
      if (soundfallf)
        t2val=soundfallvalue;
    }
    else {
      soundfalln=0;
      if (soundfallf) {
        soundfallvalue+=50;
        soundfallf=false;
      }
      else
        soundfallf=true;
    }
}


bool soundbreakflag=false;
Sint4 soundbreakduration=0,soundbreakvalue=0;

void soundbreak(void)
{
  soundbreakduration=3;
  if (soundbreakvalue<15000)
    soundbreakvalue=15000;
  soundbreakflag=true;
}

void soundbreakoff(void)
{
  soundbreakflag=false;
}

void soundbreakupdate(void)
{
  if (soundbreakflag)
    if (soundbreakduration!=0) {
      soundbreakduration--;
      t2val=soundbreakvalue;
    }
    else
      soundbreakflag=false;
}


bool soundwobbleflag=false;
Sint4 soundwobblen=0;

void soundwobble(void)
{
  soundwobbleflag=true;
}

void soundwobbleoff(void)
{
  soundwobbleflag=false;
  soundwobblen=0;
}

void soundwobbleupdate(void)
{
  if (soundwobbleflag) {
    soundwobblen++;
    if (soundwobblen>63)
      soundwobblen=0;
    switch (soundwobblen) {
      case 0:
        t2val=0x7d0;
        break;
      case 16:
      case 48:
        t2val=0x9c4;
        break;
      case 32:
        t2val=0xbb8;
        break;
    }
  }
}


bool soundfireflag[FIREBALLS]={false,false},sff[FIREBALLS];
Sint4 soundfirevalue[FIREBALLS],soundfiren[FIREBALLS]={0,0};
int soundfirew=0;

void soundfire(int n)
{
  soundfirevalue[n]=500;
  soundfireflag[n]=true;
}

void soundfireoff(int n)
{
  soundfireflag[n]=false;
  soundfiren[n]=0;
}

void soundfireupdate(void)
{
  int n;
  bool f=false;
  for (n=0;n<FIREBALLS;n++) {
    sff[n]=false;
    if (soundfireflag[n])
      if (soundfiren[n]==1) {
        soundfiren[n]=0;
        soundfirevalue[n]+=soundfirevalue[n]/55;
        sff[n]=true;
        f=true;
        if (soundfirevalue[n]>30000)
          soundfireoff(n);
      }
      else
        soundfiren[n]++;
  }
  if (f) {
    do {
      n=soundfirew++;
      if (soundfirew==FIREBALLS)
        soundfirew=0;
    } while (!sff[n]);
    t2val=soundfirevalue[n]+randnos(soundfirevalue[n]>>3);
  }
}


bool soundexplodeflag[FIREBALLS]={false,false},sef[FIREBALLS];
Sint4 soundexplodevalue[FIREBALLS],soundexplodeduration[FIREBALLS];
int soundexplodew=0;

void soundexplode(int n)
{
  soundexplodevalue[n]=1500;
  soundexplodeduration[n]=10;
  soundexplodeflag[n]=true;
  soundfireoff(n);
}

void soundexplodeoff(int n)
{
  soundexplodeflag[n]=false;
}

void soundexplodeupdate(void)
{
  int n;
  bool f=false;
  for (n=0;n<FIREBALLS;n++) {
    sef[n]=false;
    if (soundexplodeflag[n])
      if (soundexplodeduration[n]!=0) {
        soundexplodevalue[n]=soundexplodevalue[n]-(soundexplodevalue[n]>>3);
        soundexplodeduration[n]--;
        sef[n]=true;
        f=true;
      }
      else
        soundexplodeflag[n]=false;
  }
  if (f) {
    do {
      n=soundexplodew++;
      if (soundexplodew==FIREBALLS)
        soundexplodew=0;
    } while (!sef[n]);
    t2val=soundexplodevalue[n];
  }
}


bool soundbonusflag=false;
Sint4 soundbonusn=0;

void soundbonus(void)
{
  soundbonusflag=true;
}

void soundbonusoff(void)
{
  soundbonusflag=false;
  soundbonusn=0;
}

void soundbonusupdate(void)
{
  if (soundbonusflag) {
    soundbonusn++;
    if (soundbonusn>15)
      soundbonusn=0;
    if (soundbonusn>=0 && soundbonusn<6)
      t2val=0x4ce;
    if (soundbonusn>=8 && soundbonusn<14)
      t2val=0x5e9;
  }
}


bool soundemflag=false;

void soundem(void)
{
  soundemflag=true;
}

void soundemoff(void)
{
  soundemflag=false;
}

void soundemupdate(void)
{
  if (soundemflag) {
    t2val=1000;
    soundemoff();
  }
}


bool soundemeraldflag=false;
Sint4 soundemeraldduration,emerfreq,soundemeraldn;

Sint4 emfreqs[8]={0x8e8,0x7f0,0x712,0x6ac,0x5f2,0x54c,0x4b8,0x474};

void soundemerald(int n)
{
  emerfreq=emfreqs[n];
  soundemeraldduration=7;
  soundemeraldn=0;
  soundemeraldflag=true;
}

void soundemeraldoff(void)
{
  soundemeraldflag=false;
}

void soundemeraldupdate(void)
{
  if (soundemeraldflag)
    if (soundemeraldduration!=0) {
      if (soundemeraldn==0 || soundemeraldn==1)
        t2val=emerfreq;
      soundemeraldn++;
      if (soundemeraldn>7) {
        soundemeraldn=0;
        soundemeraldduration--;
      }
    }
    else
      soundemeraldoff();
}


bool soundgoldflag=false,soundgoldf=false;
Sint4 soundgoldvalue1,soundgoldvalue2,soundgoldduration;

void soundgold(void)
{
  soundgoldvalue1=500;
  soundgoldvalue2=4000;
  soundgoldduration=30;
  soundgoldf=false;
  soundgoldflag=true;
}

void soundgoldoff(void)
{
  soundgoldflag=false;
}

void soundgoldupdate(void)
{
  if (soundgoldflag) {
    if (soundgoldduration!=0)
      soundgoldduration--;
    else
      soundgoldflag=false;
    if (soundgoldf) {
      soundgoldf=false;
      t2val=soundgoldvalue1;
    }
    else {
      soundgoldf=true;
      t2val=soundgoldvalue2;
    }
    soundgoldvalue1+=(soundgoldvalue1>>4);
    soundgoldvalue2-=(soundgoldvalue2>>4);
  }
}



bool soundeatmflag=false;
Sint4 soundeatmvalue,soundeatmduration,soundeatmn;

void soundeatm(void)
{
  soundeatmduration=20;
  soundeatmn=3;
  soundeatmvalue=2000;
  soundeatmflag=true;
}

void soundeatmoff(void)
{
  soundeatmflag=false;
}

void soundeatmupdate(void)
{
  if (soundeatmflag)
    if (soundeatmn!=0) {
      if (soundeatmduration!=0) {
        if ((soundeatmduration%4)==1)
          t2val=soundeatmvalue;
        if ((soundeatmduration%4)==3)
          t2val=soundeatmvalue-(soundeatmvalue>>4);
        soundeatmduration--;
        soundeatmvalue-=(soundeatmvalue>>4);
      }
      else {
        soundeatmduration=20;
        soundeatmn--;
        soundeatmvalue=2000;
      }
    }
    else
      soundeatmflag=false;
}


bool soundddieflag=false;
Sint4 soundddien,soundddievalue;

void soundddie(void)
{
  soundddien=0;
  soundddievalue=20000;
  soundddieflag=true;
}

void soundddieoff(void)
{
  soundddieflag=false;
}

void soundddieupdate(void)
{
  if (soundddieflag) {
    soundddien++;
    if (soundddien==1)
      musicoff();
    if (soundddien>=1 && soundddien<=10)
      soundddievalue=20000-soundddien*1000;
    if (soundddien>10)
      soundddievalue+=500;
    if (soundddievalue>30000)
      soundddieoff();
    t2val=soundddievalue;
  }
}


bool sound1upflag=false;
Sint4 sound1upduration=0;

void sound1up(void)
{
  sound1upduration=96;
  sound1upflag=true;
}

void sound1upoff(void)
{
  sound1upflag=false;
}

void sound1upupdate(void)
{
  if (sound1upflag) {
    if ((sound1upduration/3)%2!=0)
      t2val=(sound1upduration<<2)+600;
    sound1upduration--;
    if (sound1upduration<1)
      sound1upflag=false;
  }
}


bool musicplaying=false;
Sint4 musicp=0,tuneno=0,noteduration=0,notevalue=0,musicmaxvol=0,
      musicattackrate=0,musicsustainlevel=0,musicdecayrate=0,musicnotewidth=0,
      musicreleaserate=0,musicstage=0,musicn=0;

void music(Sint4 tune)
{
  tuneno=tune;
  musicp=0;
  noteduration=0;
  switch (tune) {
    case 0:
      musicmaxvol=50;
      musicattackrate=20;
      musicsustainlevel=20;
      musicdecayrate=10;
      musicreleaserate=4;
      break;
    case 1:
      musicmaxvol=50;
      musicattackrate=50;
      musicsustainlevel=8;
      musicdecayrate=15;
      musicreleaserate=1;
      break;
    case 2:
      musicmaxvol=50;
      musicattackrate=50;
      musicsustainlevel=25;
      musicdecayrate=5;
      musicreleaserate=1;
  }
  musicplaying=true;
  if (tune==2)
    soundddieoff();
}

void musicoff(void)
{
  musicplaying=false;
  musicp=0;
}

Sint4 bonusjingle[321]={
  0x11d1,2,0x11d1,2,0x11d1,4,0x11d1,2,0x11d1,2,0x11d1,4,0x11d1,2,0x11d1,2,
   0xd59,4, 0xbe4,4, 0xa98,4,0x11d1,2,0x11d1,2,0x11d1,4,0x11d1,2,0x11d1,2,
  0x11d1,4, 0xd59,2, 0xa98,2, 0xbe4,4, 0xe24,4,0x11d1,4,0x11d1,2,0x11d1,2,
  0x11d1,4,0x11d1,2,0x11d1,2,0x11d1,4,0x11d1,2,0x11d1,2, 0xd59,4, 0xbe4,4,
   0xa98,4, 0xd59,2, 0xa98,2, 0x8e8,10,0xa00,2, 0xa98,2, 0xbe4,2, 0xd59,4,
   0xa98,4, 0xd59,4,0x11d1,2,0x11d1,2,0x11d1,4,0x11d1,2,0x11d1,2,0x11d1,4,
  0x11d1,2,0x11d1,2, 0xd59,4, 0xbe4,4, 0xa98,4,0x11d1,2,0x11d1,2,0x11d1,4,
  0x11d1,2,0x11d1,2,0x11d1,4, 0xd59,2, 0xa98,2, 0xbe4,4, 0xe24,4,0x11d1,4,
  0x11d1,2,0x11d1,2,0x11d1,4,0x11d1,2,0x11d1,2,0x11d1,4,0x11d1,2,0x11d1,2,
   0xd59,4, 0xbe4,4, 0xa98,4, 0xd59,2, 0xa98,2, 0x8e8,10,0xa00,2, 0xa98,2,
   0xbe4,2, 0xd59,4, 0xa98,4, 0xd59,4, 0xa98,2, 0xa98,2, 0xa98,4, 0xa98,2,
   0xa98,2, 0xa98,4, 0xa98,2, 0xa98,2, 0xa98,4, 0x7f0,4, 0xa98,4, 0x7f0,4,
   0xa98,4, 0x7f0,4, 0xa98,4, 0xbe4,4, 0xd59,4, 0xe24,4, 0xfdf,4, 0xa98,2,
   0xa98,2, 0xa98,4, 0xa98,2, 0xa98,2, 0xa98,4, 0xa98,2, 0xa98,2, 0xa98,4,
   0x7f0,4, 0xa98,4, 0x7f0,4, 0xa98,4, 0x7f0,4, 0x8e8,4, 0x970,4, 0x8e8,4,
   0x970,4, 0x8e8,4, 0xa98,2, 0xa98,2, 0xa98,4, 0xa98,2, 0xa98,2, 0xa98,4,
   0xa98,2, 0xa98,2, 0xa98,4, 0x7f0,4, 0xa98,4, 0x7f0,4, 0xa98,4, 0x7f0,4,
   0xa98,4, 0xbe4,4, 0xd59,4, 0xe24,4, 0xfdf,4, 0xa98,2, 0xa98,2, 0xa98,4,
   0xa98,2, 0xa98,2, 0xa98,4, 0xa98,2, 0xa98,2, 0xa98,4, 0x7f0,4, 0xa98,4,
   0x7f0,4, 0xa98,4, 0x7f0,4, 0x8e8,4, 0x970,4, 0x8e8,4, 0x970,4, 0x8e8,4,
  0x7d64};

Sint4 backgjingle[291]={
   0xfdf,2,0x11d1,2, 0xfdf,2,0x1530,2,0x1ab2,2,0x1530,2,0x1fbf,4, 0xfdf,2,
  0x11d1,2, 0xfdf,2,0x1530,2,0x1ab2,2,0x1530,2,0x1fbf,4, 0xfdf,2, 0xe24,2,
   0xd59,2, 0xe24,2, 0xd59,2, 0xfdf,2, 0xe24,2, 0xfdf,2, 0xe24,2,0x11d1,2,
   0xfdf,2,0x11d1,2, 0xfdf,2,0x1400,2, 0xfdf,4, 0xfdf,2,0x11d1,2, 0xfdf,2,
  0x1530,2,0x1ab2,2,0x1530,2,0x1fbf,4, 0xfdf,2,0x11d1,2, 0xfdf,2,0x1530,2,
  0x1ab2,2,0x1530,2,0x1fbf,4, 0xfdf,2, 0xe24,2, 0xd59,2, 0xe24,2, 0xd59,2,
   0xfdf,2, 0xe24,2, 0xfdf,2, 0xe24,2,0x11d1,2, 0xfdf,2,0x11d1,2, 0xfdf,2,
   0xe24,2, 0xd59,4, 0xa98,2, 0xbe4,2, 0xa98,2, 0xd59,2,0x11d1,2, 0xd59,2,
  0x1530,4, 0xa98,2, 0xbe4,2, 0xa98,2, 0xd59,2,0x11d1,2, 0xd59,2,0x1530,4,
   0xa98,2, 0x970,2, 0x8e8,2, 0x970,2, 0x8e8,2, 0xa98,2, 0x970,2, 0xa98,2,
   0x970,2, 0xbe4,2, 0xa98,2, 0xbe4,2, 0xa98,2, 0xd59,2, 0xa98,4, 0xa98,2,
   0xbe4,2, 0xa98,2, 0xd59,2,0x11d1,2, 0xd59,2,0x1530,4, 0xa98,2, 0xbe4,2,
   0xa98,2, 0xd59,2,0x11d1,2, 0xd59,2,0x1530,4, 0xa98,2, 0x970,2, 0x8e8,2,
   0x970,2, 0x8e8,2, 0xa98,2, 0x970,2, 0xa98,2, 0x970,2, 0xbe4,2, 0xa98,2,
   0xbe4,2, 0xa98,2, 0xd59,2, 0xa98,4, 0x7f0,2, 0x8e8,2, 0xa98,2, 0xd59,2,
  0x11d1,2, 0xd59,2,0x1530,4, 0xa98,2, 0xbe4,2, 0xa98,2, 0xd59,2,0x11d1,2,
   0xd59,2,0x1530,4, 0xa98,2, 0x970,2, 0x8e8,2, 0x970,2, 0x8e8,2, 0xa98,2,
   0x970,2, 0xa98,2, 0x970,2, 0xbe4,2, 0xa98,2, 0xbe4,2, 0xd59,2, 0xbe4,2,
   0xa98,4,0x7d64};

Sint4 dirge[]={
  0x7d00, 2,0x11d1, 6,0x11d1, 4,0x11d1, 2,0x11d1, 6, 0xefb, 4, 0xfdf, 2,
   0xfdf, 4,0x11d1, 2,0x11d1, 4,0x12e0, 2,0x11d1,12,0x7d00,16,0x7d00,16,
  0x7d00,16,0x7d00,16,0x7d00,16,0x7d00,16,0x7d00,16,0x7d00,16,0x7d00,16,
  0x7d00,16,0x7d00,16,0x7d00,16,0x7d64};

void musicupdate(void)
{
  if (!musicplaying)
    return;
  if (noteduration!=0)
    noteduration--;
  else {
    musicstage=musicn=0;
    switch (tuneno) {
      case 0:
        noteduration=bonusjingle[musicp+1]*3;
        musicnotewidth=noteduration-3;
        notevalue=bonusjingle[musicp];
        musicp+=2;
        if (bonusjingle[musicp]==0x7d64)
          musicp=0;
        break;
      case 1:
        noteduration=backgjingle[musicp+1]*6;
        musicnotewidth=12;
        notevalue=backgjingle[musicp];
        musicp+=2;
        if (backgjingle[musicp]==0x7d64)
          musicp=0;
        break;
      case 2:
        noteduration=dirge[musicp+1]*10;
        musicnotewidth=noteduration-10;
        notevalue=dirge[musicp];
        musicp+=2;
        if (dirge[musicp]==0x7d64)
          musicp=0;
        break;
    }
  }
  musicn++;
  wavetype=1;
  t0val=notevalue;
  if (musicn>=musicnotewidth)
    musicstage=2;
  switch(musicstage) {
    case 0:
      if (musvol+musicattackrate>=musicmaxvol) {
        musicstage=1;
        musvol=musicmaxvol;
        break;
      }
      musvol+=musicattackrate;
      break;
    case 1:
      if (musvol-musicdecayrate<=musicsustainlevel) {
        musvol=musicsustainlevel;
        break;
      }
      musvol-=musicdecayrate;
      break;
    case 2:
      if (musvol-musicreleaserate<=1) {
        musvol=1;
        break;
      }
      musvol-=musicreleaserate;
  }
  if (musvol==1)
    t0val=0x7d00;
}


void soundpause(void)
{
  soundpausedflag=true;
#ifdef _WINDOWS
  pause_windows_sound_playback();
#endif
}

void soundpauseoff(void)
{
  soundpausedflag=false;
#ifdef _WINDOWS
  resume_windows_sound_playback();
#endif
}

void sett0(void)
{
  if (sndflag) {
    timer2(t2val);
    if (t0val<1000 && (wavetype==1 || wavetype==2))
      t0val=1000;
    timer0(t0val);
    timerrate=t0val;
    if (musvol<1)
      musvol=1;
    if (musvol>50)
      musvol=50;
    pulsewidth=musvol*volume;
    setsoundmode();
  }
}

bool soundt0flag=false;

void setsoundt2(void)
{
  if (soundt0flag) {
    spkrmode=0;
    soundt0flag=false;
    setspkrt2();
  }
}

void setsoundmode(void)
{
  spkrmode=wavetype;
  if (!soundt0flag && sndflag) {
    soundt0flag=true;
    setspkrt2();
  }
}

bool int8flag=false;

void startint8(void)
{
  if (!int8flag) {
    initint8();
    timerrate=0x4000;
    settimer0(0x4000);
    int8flag=true;
  }
}

void stopint8(void)
{
  settimer0(0);
  if (int8flag) {
    restoreint8();
    int8flag=false;
  }
  sett2val(40);
  setspkrt2();
}

void initsound(void)
{
  settimer2(40);
  setspkrt2();
  settimer0(0);
  wavetype=2;
  t0val=12000;
  musvol=8;
  t2val=40;
  soundt0flag=true;
  sndflag=true;
  spkrmode=0;
  int8flag=false;
  setsoundt2();
  soundstop();
  setupsound();
  timerrate=0x4000;
  settimer0(0x4000);
  randvs=getlrt();
}

void s0killsound(void)
{
  setsoundt2();
  timer2(40);
  stopint8();
}

void s0setupsound(void)
{
  inittimer();
  curtime=0;
  startint8();
}

void s0fillbuffer(void)
{
}
