/* Digger Remastered
   Copyright (c) Andrew Jenner 1998-2004 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "def.h"

#ifdef _WINDOWS
#include "win_dig.h"
#include "win_snd.h"
#include "win_vid.h"
#endif

#include "hardware.h"
#include "sound.h"
#include "sprite.h"
#include "input.h"
#include "scores.h"
#include "drawing.h"
#include "digger.h"
#include "monster.h"
#include "bags.h"
#include "record.h"
#include "main.h"
#include "newsnd.h"
#include "ini.h"

struct game
{
  Sint4 level;
  bool levdone;
} gamedat[2];

char pldispbuf[14];

Sint4 curplayer=0,nplayers=1,penalty=0,diggers=1,startlev=1;

bool levnotdrawn=false,alldead=false,unlimlives=false,started;

WCHAR levfname[FILENAME_BUFFER_SIZE];
bool levfflag=false;
bool biosflag=false;
Sint5 delaytime=0;
int gtime=0;
bool gauntlet=false,timeout=false,synchvid=false;

void shownplayers(void);
void switchnplayers(void);
void drawscreen(void);
void initchars(void);
void checklevdone(void);
Sint4 levno(void);
void testpause(void);
void calibrate(void);
void parsecmd(int argc,char *argv[]);
void patchcga(void);
void initlevel(void);
void finish(void);
void inir(void);
void redefkeyb(bool allf);
int getalllives(void);

Sint3 leveldat[]=
  "S   B     HHHHS"
  "V  CC  C  V B  "
  "VB CC  C  V    "
  "V  CCB CB V CCC"
  "V  CC  C  V CCC"
  "HH CC  C  V CCC"
  " V    B B V    "
  " HHHH     V    "
  "C   V     V   C"
  "CC  HHHHHHH  CC"
  "SHHHHH  B B  HS"
  " CC  V       V "
  " CC  V CCCCC V "
  "BCCB V CCCCC V "
  "CCCC V       V "
  "CCCC V B  HHHH "
  " CC  V CC V    "
  " BB  VCCCCV CC "
  "C    V CC V CC "
  "CC   HHHHHH    "
  "SHHHHB B BHHHHS"
  "CC  V C C V BB "
  "C   V C C V CC "
  " BB V C C VCCCC"
  "CCCCV C C VCCCC"
  "CCCCHHHHHHH CC "
  " CC  C V C  CC "
  " CC  C V C     "
  "C    C V C    C"
  "CC   C H C   CC"
  "SHBCCCCBCCCCBHS"
  "CV  CCCCCCC  VC"
  "CHHH CCCCC HHHC"
  "C  V  CCC  V  C"
  "   HHH C HHH   "
  "  B  V B V  B  "
  "  C  VCCCV  C  "
  " CCC HHHHH CCC "
  "CCCCC CVC CCCCC"
  "CCCCC CHC CCCCC"
  "SHHHHHHHHHHHHHS"
  "VBCCCCBVCCCCCCV"
  "VCCCCCCV CCBC V"
  "V CCCC VCCBCCCV"
  "VCCCCCCV CCCC V"
  "V CCCC VBCCCCCV"
  "VCCBCCCV CCCC V"
  "V CCBC VCCCCCCV"
  "VCCCCCCVCCCCCCV"
  "HHHHHHHHHHHHHHH"
  "SHHHHHHHHHHHHHS"
  "VCBCCV V VCCBCV"
  "VCCC VBVBV CCCV"
  "VCCCHH V HHCCCV"
  "VCC V CVC V CCV"
  "VCCHH CVC HHCCV"
  "VC V CCVCC V CV"
  "VCHHBCCVCCBHHCV"
  "VCVCCCCVCCCCVCV"
  "HHHHHHHHHHHHHHH"
  "SHCCCCCVCCCCCHS"
  " VCBCBCVCBCBCV "
  "BVCCCCCVCCCCCVB"
  "CHHCCCCVCCCCHHC"
  "CCV CCCVCCC VCC"
  "CCHHHCCVCCHHHCC"
  "CCCCV CVC VCCCC"
  "CCCCHH V HHCCCC"
  "CCCCCV V VCCCCC"
  "CCCCCHHHHHCCCCC"
  "HHHHHHHHHHHHHHS"
  "V CCBCCCCCBCC V"
  "HHHCCCCBCCCCHHH"
  "VBV CCCCCCC VBV"
  "VCHHHCCCCCHHHCV"
  "VCCBV CCC VBCCV"
  "VCCCHHHCHHHCCCV"
  "VCCCC V V CCCCV"
  "VCCCCCV VCCCCCV"
  "HHHHHHHHHHHHHHH";

Sint4 getlevch(Sint4 x,Sint4 y,Sint4 l)
{
  if ((l==3 || l==4) && !levfflag && diggers==2 && y==9 && (x==6 || x==8))
    return 'H';
  return leveldat[((l-1)*MHEIGHT + y)*MWIDTH + x];
}

#ifdef INTDRF
extern FILE *info;
#endif

void game(void)
{
  Sint4 t,c,i;
  bool flashplayer=false;
#ifdef _WINDOWS
  show_game_menu();
#endif
  if (gauntlet) {
    cgtime=gtime*1193181l;
    timeout=false;
  }
  initlives();
  gamedat[0].level=startlev;
  if (nplayers==2)
    gamedat[1].level=startlev;
  alldead=false;
  gclear();
  curplayer=0;
  initlevel();
  curplayer=1;
  initlevel();
  zeroscores();
  bonusvisible=true;
  if (nplayers==2)
    flashplayer=true;
  curplayer=0;
  while (getalllives()!=0 && !escape && !timeout) {
    while (!alldead && !escape && !timeout) {
      initmbspr();

      if (playing)
        randv=playgetrand();
      else
        randv=getlrt();
#ifdef INTDRF
      fprintf(info,"%lu\n",randv);
      frame=0;
#endif
      recputrand(randv);
      if (levnotdrawn) {
        levnotdrawn=false;
        drawscreen();
        if (flashplayer) {
          flashplayer=false;
          strcpy(pldispbuf,"PLAYER ");
          if (curplayer==0)
            strcat(pldispbuf,"1");
          else
            strcat(pldispbuf,"2");
          cleartopline();
          for (t=0;t<15;t++)
            for (c=1;c<=3;c++) {
              outtext(pldispbuf,108,0,c);
              writecurscore(c);
              newframe();
              if (escape)
                return;
            }
          drawscores();
          for (i=0;i<diggers;i++)
            addscore(i,0);
        }
      }
      else
        initchars();
      outtext("        ",108,0,3);
      initscores();
      drawlives();
      music(1);

      flushkeybuf();
      for (i=0;i<diggers;i++)
        readdir(i);
      while (!alldead && !gamedat[curplayer].levdone && !escape && !timeout) {
        penalty=0;
        dodigger();
        domonsters();
        dobags();
        if (penalty>8)
          incmont(penalty-8);
        testpause();
        checklevdone();
      }
      erasediggers();
      musicoff();
      t=20;
      while ((getnmovingbags()!=0 || t!=0) && !escape && !timeout) {
        if (t!=0)
          t--;
        penalty=0;
        dobags();
        dodigger();
        domonsters();
        if (penalty<8)
          t=0;
      }
      soundstop();
      for (i=0;i<diggers;i++)
        killfire(i);
      erasebonus();
      cleanupbags();
      savefield();
      erasemonsters();
      recputeol();
      if (playing)
        playskipeol();
      if (escape)
        recputeog();
      if (gamedat[curplayer].levdone)
        soundlevdone();
      if (countem()==0 || gamedat[curplayer].levdone) {
#ifdef INTDRF
        fprintf(info,"%i\n",frame);
#endif
        for (i=curplayer;i<diggers+curplayer;i++)
          if (getlives(i)>0 && !digalive(i))
            declife(i);
        drawlives();
        gamedat[curplayer].level++;
        if (gamedat[curplayer].level>1000)
          gamedat[curplayer].level=1000;
        initlevel();
      }
      else
        if (alldead) {
#ifdef INTDRF
          fprintf(info,"%i\n",frame);
#endif
          for (i=curplayer;i<curplayer+diggers;i++)
            if (getlives(i)>0)
              declife(i);
          drawlives();
        }
      if ((alldead && getalllives()==0 && !gauntlet && !escape) || timeout)
        endofgame();
    }
    alldead=false;
    if (nplayers==2 && getlives(1-curplayer)!=0) {
      curplayer=1-curplayer;
      flashplayer=levnotdrawn=true;
    }
  }
#ifdef INTDRF
  fprintf(info,"-1\n%lu\n%i",getscore0(),gamedat[0].level);
#endif
}

void maininit(void)
{
  calibrate();
  ginit();
  gpal(0);
  setretr(true);
  initkeyb();
  detectjoy();
  inir();
  initsound();
  recstart();
}

#ifndef _WINDOWS
int main(int argc,char *argv[])
{
  maininit();
  parsecmd(argc,argv);
  return mainprog();
}
#endif

#ifdef _WINDOWS
  Sint4 frame;
#endif

int mainprog(void)
{
  Sint4 t,x;
#ifndef _WINDOWS
  Sint4 frame;
#endif
  loadscores();
  escape=false;
  do {
#ifdef _WINDOWS
    show_main_menu();
#endif
    soundstop();
    creatembspr();
    detectjoy();
    gclear();
    gtitle();
    outtext("D I G G E R",100,0,3);
    shownplayers();
    showtable();
    started=false;
    frame=0;
    newframe();
    teststart();
#ifdef _WINDOWS
    reset_main_menu_screen=false;
#endif
    while (!started) {
      started=teststart();
      if ((akeypressed==27 || akeypressed=='n' || akeypressed=='N') &&
          !gauntlet && diggers==1) {
        switchnplayers();
        shownplayers();
        akeypressed=0;
      }
      if (frame==0)
        for (t=54;t<174;t+=12)
          outtext("            ",164,t,0);
      if (frame==50) {
        movedrawspr(FIRSTMONSTER,292,63);
        x=292;
      }
      if (frame>50 && frame<=77) {
        x-=4;
        drawmon(0,1,DIR_LEFT,x,63);
      }
      if (frame>77)
        drawmon(0,1,DIR_RIGHT,184,63);
      if (frame==83)
        outtext("NOBBIN",216,64,2);
      if (frame==90) {
        movedrawspr(FIRSTMONSTER+1,292,82);
        drawmon(1,0,DIR_LEFT,292,82);
        x=292;
      }
      if (frame>90 && frame<=117) {
        x-=4;
        drawmon(1,0,DIR_LEFT,x,82);
      }
      if (frame>117)
        drawmon(1,0,DIR_RIGHT,184,82);
      if (frame==123)
        outtext("HOBBIN",216,83,2);
      if (frame==130) {
        movedrawspr(FIRSTDIGGER,292,101);
        drawdigger(0,DIR_LEFT,292,101,1);
        x=292;
      }
      if (frame>130 && frame<=157) {
        x-=4;
        drawdigger(0,DIR_LEFT,x,101,1);
      }
      if (frame>157)
        drawdigger(0,DIR_RIGHT,184,101,1);
      if (frame==163)
        outtext("DIGGER",216,102,2);
      if (frame==178) {
        movedrawspr(FIRSTBAG,184,120);
        drawgold(0,0,184,120);
      }
      if (frame==183)
        outtext("GOLD",216,121,2);
      if (frame==198)
        drawemerald(184,141);
      if (frame==203)
        outtext("EMERALD",216,140,2);
      if (frame==218)
        drawbonus(184,158);
      if (frame==223)
        outtext("BONUS",216,159,2);
      newframe();
      frame++;
      if (frame>250)
        frame=0;
#ifdef _WINDOWS
      if (reset_main_menu_screen) {
        escape=false;
        break;
      }
#endif
    }
    if (savedrf) {
      if (gotgame)
        recsavedrf();
      savedrf=false;
      continue;
    }
#ifdef _WINDOWS
    if (reset_main_menu_screen)
      continue;
#endif
    if (escape)
      break;
    recinit();
    game();
    gotgame=true;
    if (gotname)
      recsavedrf();
    savedrf=false;
    escape=false;
  } while (!escape);
  finish();
  return 0;
}

void finish(void)
{
  killsound();
  soundoff();
  soundkillglob();
  restorekeyb();
  graphicsoff();
#ifdef _WINDOWS
  windows_finish();
#endif
}

void shownplayers(void)
{
  if (diggers==2)
    if (gauntlet) {
      outtext("TWO PLAYER",180,25,3);
      outtext("GAUNTLET",192,39,3);
    }
    else {
      outtext("TWO PLAYER",180,25,3);
      outtext("SIMULTANEOUS",170,39,3);
    }
  else
    if (gauntlet) {
      outtext("GAUNTLET",192,25,3);
      outtext("MODE",216,39,3);
    }
    else
      if (nplayers==1) {
        outtext("ONE",220,25,3);
        outtext(" PLAYER ",192,39,3);
      }
      else {
        outtext("TWO",220,25,3);
        outtext(" PLAYERS",184,39,3);
      }
}

int getalllives(void)
{
  int t=0,i;
  for (i=curplayer;i<diggers+curplayer;i++)
    t+=getlives(i);
  return t;
}

void switchnplayers(void)
{
  nplayers=3-nplayers;
}

void initlevel(void)
{
  gamedat[curplayer].levdone=false;
  makefield();
  makeemfield();
  initbags();
  levnotdrawn=true;
}

void drawscreen(void)
{
  creatembspr();
  drawstatics();
  drawbags();
  drawemeralds();
  initdigger();
  initmonsters();
}

void initchars(void)
{
  initmbspr();
  initdigger();
  initmonsters();
}

void checklevdone(void)
{
  if ((countem()==0 || monleft()==0) && isalive())
    gamedat[curplayer].levdone=true;
  else
    gamedat[curplayer].levdone=false;
}

void incpenalty(void)
{
  penalty++;
}

void cleartopline(void)
{
  outtext("                          ",0,0,3);
  outtext(" ",308,0,3);
}

Sint4 levplan(void)
{
  Sint4 l=levno();
  if (l>8)
    l=(l&3)+5; /* Level plan: 12345678, 678, (5678) 247 times, 5 forever */
  return l;
}

Sint4 levof10(void)
{
  if (gamedat[curplayer].level>10)
    return 10;
  return gamedat[curplayer].level;
}

Sint4 levno(void)
{
  return gamedat[curplayer].level;
}

void setdead(bool df)
{
  alldead=df;
}

void testpause(void)
{
  int i;
  if (pausef) {
    pausef=false;
    soundpause();
    sett2val(40);
    setsoundt2();
    cleartopline();
    outtext("PRESS ANY KEY",80,0,1);
    getkey();
    cleartopline();
    drawscores();
    for (i=0;i<diggers;i++)
      addscore(i,0);
    drawlives();
    if (!synchvid)
      curtime=gethrt();
  }
  else
    soundpauseoff();
}

void calibrate(void)
{
  volume=(Sint4)(getkips()/291);
  if (volume==0)
    volume=1;
}

Uint4 sound_device,sound_port,sound_irq,sound_dma,sound_rate,sound_length;

void parsecmd(int argc,char *argv[])
{
  WCHAR *word;
  Sint4 arg,i,j,speedmul;
  bool sf,gs=false,norepf=false;
  FILE *levf;

  for (arg=1;arg<argc;arg++) {
    word=argv[arg];
    if (word[0]=='/' || word[0]=='-') {
      if (word[1]=='L' || word[1]=='l' || word[1]=='R' || word[1]=='r' ||
          word[1]=='P' || word[1]=='p' || word[1]=='S' || word[1]=='s' ||
          word[1]=='E' || word[1]=='e' || word[1]=='G' || word[1]=='g' ||
          word[1]=='A' || word[1]=='a' || word[1]=='I' || word[1]=='i')
        if (word[2]==':')
          i=3;
        else
          i=2;
      if (word[1]=='L' || word[1]=='l') {
        j=0;
        while (word[i]!=0)
          levfname[j++]=word[i++];
        levfname[j]=word[i];
        levfflag=true;
      }
      if (word[1]=='R' || word[1]=='r')
        recname(word+i);
      if (word[1]=='P' || word[1]=='p' || word[1]=='E' || word[1]=='e') {
        openplay(word+i);
        if (escape)
          norepf=true;
      }
      if (word[1]=='E' || word[1]=='e') {
        finish();
        if (escape)
          exit(0);
        exit(1);
      }
      if ((word[1]=='O' || word[1]=='o') && !norepf) {
        arg=0;
        continue;
      }
      if (word[1]=='S' || word[1]=='s') {
        speedmul=0;
        while (word[i]!=0)
          speedmul=10*speedmul+word[i++]-'0';
        ftime=speedmul*2000l;
        gs=true;
      }
      if (word[1]=='I' || word[1]=='i')
        sscanf(word+i,"%u",&startlev);
      if (word[1]=='U' || word[1]=='u')
        unlimlives=true;
#ifndef _WINDOWS
      if (word[1]=='?' || word[1]=='h' || word[1]=='H') {
        finish();
        printf("DIGGER - Copyright (c) 1983 Windmill software\n"
               "Restored 1998 by AJ Software\n"
#ifdef ARM
               "Acorn port by Julian Brown\n"
#endif
               "http://www.digger.org\n"
               "Version: "DIGGER_VERSION"\n\n"

               "Command line syntax:\n"
               "  DIGGER [[/S:]speed] [[/L:]level file] [/C] [/B] [/Q] [/M] "
                                                         "[/P:playback file]\n"
               "         [/E:playback file] [/R:record file] [/O] [/K[A]] "
                                                           "[/G[:time]] [/2]\n"
               "         [/A:device,port,irq,dma,rate,length] [/V] [/U] "
                                                               "[/I:level]\n\n"
               "/C = Use CGA graphics\n"
               "/B = Use BIOS palette functions for CGA (slow!)\n"
               "/Q = Quiet mode (no sound at all)       "
               "/M = No music\n"
               "/R = Record graphics to file\n"
               "/P = Playback and restart program       "
               "/E = Playback and exit program\n"
               "/O = Loop to beginning of command line\n"
               "/K = Redefine keyboard\n"
               "/G = Gauntlet mode\n"
               "/2 = Two player simultaneous mode\n"
               "/A = Use alternate sound device\n"
               "/V = Synchronize timing to vertical retrace\n"
               "/U = Allow unlimited lives\n"
               "/I = Start on a level other than 1\n");
        exit(1);
      }
#endif
      if (word[1]=='Q' || word[1]=='q')
        soundflag=false;
      if (word[1]=='M' || word[1]=='m')
        musicflag=false;
      if (word[1]=='2')
        diggers=2;
#ifndef _WINDOWS
      if (word[1]=='B' || word[1]=='b' || word[1]=='C' || word[1]=='c') {
        ginit=cgainit;
        gpal=cgapal;
        ginten=cgainten;
        gclear=cgaclear;
        ggetpix=cgagetpix;
        gputi=cgaputi;
        ggeti=cgageti;
        gputim=cgaputim;
        gwrite=cgawrite;
        gtitle=cgatitle;
        if (word[1]=='B' || word[1]=='b')
          biosflag=true;
        ginit();
        gpal(0);
      }
      if (word[1]=='K' || word[1]=='k')
        if (word[2]=='A' || word[2]=='a')
          redefkeyb(true);
        else
          redefkeyb(false);
      if (word[1]=='A' || word[1]=='a') {
        sscanf(word+i,"%u,%x,%u,%u,%u,%u",&sound_device,&sound_port,&sound_irq,
               &sound_dma,&sound_rate,&sound_length);
        killsound();
        if (sound_device>0) {
          volume=1;
          setupsound=s1setupsound;
          killsound=s1killsound;
          fillbuffer=s1fillbuffer;
          initint8=s1initint8;
          restoreint8=s1restoreint8;
          if (sound_device==1) {
            soundoff=s1soundoff;
            setspkrt2=s1setspkrt2;
            settimer0=s1settimer0;
            timer0=s1timer0;
            settimer2=s1settimer2;
            timer2=s1timer2;
            getsample=getsample1;
          }
          else {
            soundoff=s2soundoff;
            setspkrt2=s2setspkrt2;
            settimer0=s2settimer0;
            timer0=s2timer0;
            settimer2=s2settimer2;
            timer2=s2timer2;
            getsample=getsample2;
          }
          soundinitglob(sound_port,sound_irq,sound_dma,sound_length,sound_rate);
        }
        initsound();
      }
      if (word[1]=='V' || word[1]=='v')
        synchvid=true;
#endif
      if (word[1]=='G' || word[1]=='g') {
        gtime=0;
        while (word[i]!=0)
          gtime=10*gtime+word[i++]-'0';
        if (gtime>3599)
          gtime=3599;
        if (gtime==0)
          gtime=120;
        gauntlet=true;
      }
    }
    else {
      i=strlen(word);
      if (i<1)
        continue;
      sf=true;
      if (!gs)
        for (j=0;j<i;j++)
          if (word[j]<'0' || word[j]>'9') {
            sf=false;
            break;
          }
      if (sf) {
        speedmul=0;
        j=0;
        while (word[j]!=0)
          speedmul=10*speedmul+word[j++]-'0';
        gs=true;
        ftime=speedmul*2000l;
      }
      else {
        j=0;
        while (word[j]!=0) {
          levfname[j]=word[j];
          j++;
        }
        levfname[j]=word[j];
        levfflag=true;
      }
    }
  }

  if (levfflag) {
    levf=fopen(levfname,"rb");
    if (levf==NULL) {
      strcat(levfname,".DLF");
      levf=fopen(levfname,"rb");
    }
    if (levf==NULL)
      levfflag=false;
    else {
      fread(&bonusscore,2,1,levf);
      fread(leveldat,1200,1,levf);
      fclose(levf);
    }
  }
}

Sint5 randv;

Sint4 randno(Sint4 n)
{
  randv=randv*0x15a4e35l+1;
  return (Sint4)((randv&0x7fffffffl)%n);
}

char *keynames[17]={"Right","Up","Left","Down","Fire",
                    "Right","Up","Left","Down","Fire",
                    "Cheat","Accel","Brake","Music","Sound","Exit","Pause"};

#ifndef _WINDOWS
int dx_sound_volume;
bool g_bWindowed,use_640x480_fullscreen,use_async_screen_updates;
#endif

void inir(void)
{
  char kbuf[80],vbuf[80];
  int i,j,p;
  bool cgaflag;

  for (i=0;i<17;i++) {
    sprintf(kbuf,"%s%c",keynames[i],(i>=5 && i<10) ? '2' : 0);
    sprintf(vbuf,"%i/%i/%i/%i/%i",keycodes[i][0],keycodes[i][1],
            keycodes[i][2],keycodes[i][3],keycodes[i][4]);
    GetINIString(INI_KEY_SETTINGS,kbuf,vbuf,vbuf,80,ININAME);
    krdf[i]=true;
    p=0;
    for (j=0;j<5;j++) {
      keycodes[i][j]=atoi(vbuf+p);
      while (vbuf[p]!='/' && vbuf[p]!=0)
        p++;
      if (vbuf[p]==0)
        break;
      p++;
    }
  }
  gtime=(int)GetINIInt(INI_GAME_SETTINGS,"GauntletTime",120,ININAME);
  ftime=GetINIInt(INI_GAME_SETTINGS,"Speed",80000l,ININAME);
  gauntlet=GetINIBool(INI_GAME_SETTINGS,"GauntletMode",false,ININAME);
  GetINIString(INI_GAME_SETTINGS,"Players","1",vbuf,80,ININAME);
  _strupr(vbuf);
  if (vbuf[0]=='2' && vbuf[1]=='S') {
    diggers=2;
    nplayers=1;
  }
  else {
    diggers=1;
    nplayers=atoi(vbuf);
    if (nplayers<1 || nplayers>2)
      nplayers=1;
  }
  soundflag=GetINIBool(INI_SOUND_SETTINGS,"SoundOn",true,ININAME);
  musicflag=GetINIBool(INI_SOUND_SETTINGS,"MusicOn",true,ININAME);
  sound_device=(int)GetINIInt(INI_SOUND_SETTINGS,"Device",DEF_SND_DEV,ININAME);
  sound_port=(int)GetINIInt(INI_SOUND_SETTINGS,"Port",544,ININAME);
  sound_irq=(int)GetINIInt(INI_SOUND_SETTINGS,"Irq",5,ININAME);
  sound_dma=(int)GetINIInt(INI_SOUND_SETTINGS,"DMA",1,ININAME);
  sound_rate=(int)GetINIInt(INI_SOUND_SETTINGS,"Rate",22050,ININAME);
  sound_length=(int)GetINIInt(INI_SOUND_SETTINGS,"BufferSize",DEFAULT_BUFFER,
                              ININAME);
  if (sound_device>0) {
    volume=1;
    setupsound=s1setupsound;
    killsound=s1killsound;
    fillbuffer=s1fillbuffer;
    initint8=s1initint8;
    restoreint8=s1restoreint8;
    if (sound_device==1) {
      soundoff=s1soundoff;
      setspkrt2=s1setspkrt2;
      settimer0=s1settimer0;
      timer0=s1timer0;
      settimer2=s1settimer2;
      timer2=s1timer2;
      getsample=getsample1;
    }
    else {
      soundoff=s2soundoff;
      setspkrt2=s2setspkrt2;
      settimer0=s2settimer0;
      timer0=s2timer0;
      settimer2=s2settimer2;
      timer2=s2timer2;
      getsample=getsample2;
    }
    soundinitglob(sound_port,sound_irq,sound_dma,sound_length,sound_rate);
  }
#ifdef _WINDOWS
  dx_sound_volume=(int)GetINIInt(INI_SOUND_SETTINGS,"SoundVolume",100,ININAME);
  set_sound_volume(dx_sound_volume);
#else
  dx_sound_volume=(int)GetINIInt(INI_SOUND_SETTINGS,"SoundVolume",0,ININAME);
#endif
#ifndef DIRECTX
  g_bWindowed=true;
#else
  g_bWindowed=!GetINIBool(INI_GRAPHICS_SETTINGS,"FullScreen",false,ININAME);
#endif
  use_640x480_fullscreen=GetINIBool(INI_GRAPHICS_SETTINGS,"640x480",false,
                                    ININAME);
#ifdef DIRECTX
  if (!g_bWindowed)
    ChangeCoopLevel();
#endif
  use_async_screen_updates=GetINIBool(INI_GRAPHICS_SETTINGS,"Async",true,
                                      ININAME);
  synchvid=GetINIBool(INI_GRAPHICS_SETTINGS,"Synch",false,ININAME);
  cgaflag=GetINIBool(INI_GRAPHICS_SETTINGS,"CGA",false,ININAME);
  biosflag=GetINIBool(INI_GRAPHICS_SETTINGS,"BIOSPalette",false,ININAME);
  if (cgaflag || biosflag) {
    ginit=cgainit;
    gpal=cgapal;
    ginten=cgainten;
    gclear=cgaclear;
    ggetpix=cgagetpix;
    gputi=cgaputi;
    ggeti=cgageti;
    gputim=cgaputim;
    gwrite=cgawrite;
    gtitle=cgatitle;
    ginit();
    gpal(0);
  }
  unlimlives=GetINIBool(INI_GAME_SETTINGS,"UnlimitedLives",false,ININAME);
  startlev=(int)GetINIInt(INI_GAME_SETTINGS,"StartLevel",1,ININAME);
#ifdef _WINDOWS
  GetINIString(INI_GAME_SETTINGS,"LevelFile","",levfname,FILENAME_BUFFER_SIZE,ININAME);
  levfflag=(levfname[0]!='\0'); /* true; */
#endif
}

void redefkeyb(bool allf)
{
  int i,j,k,l,z,y=0;
  bool f;
  char kbuf[80],vbuf[80];
  if (diggers==2) {
    outtext("PLAYER 1:",0,y,3);
    y+=12;
  }

  outtext("PRESS NEW KEY FOR",0,y,3);
  y+=12;

/* Step one: redefine keys that are always redefined. */

  for (i=0;i<5;i++) {
    outtext(keynames[i],0,y,2); /* Red first */
    findkey(i);
    outtext(keynames[i],0,y,1); /* Green once got */
    y+=12;
    for (j=0;j<i;j++) { /* Note: only check keys just pressed (I hate it when
                           this is done wrong, and it often is.) */
      if (keycodes[i][0]==keycodes[j][0] && keycodes[i][0]!=0) {
        i--;
        y-=12;
        break;
      }
      for (k=2;k<5;k++)
        for (l=2;l<5;l++)
          if (keycodes[i][k]==keycodes[j][l] && keycodes[i][k]!=-2) {
            j=i;
            k=5;
            i--;
            y-=12;
            break; /* Try again if this key already used */
          }
    }
  }

  if (diggers==2) {
    outtext("PLAYER 2:",0,y,3);
    y+=12;
    for (i=5;i<10;i++) {
      outtext(keynames[i],0,y,2); /* Red first */
      findkey(i);
      outtext(keynames[i],0,y,1); /* Green once got */
      y+=12;
      for (j=0;j<i;j++) { /* Note: only check keys just pressed (I hate it when
                             this is done wrong, and it often is.) */
        if (keycodes[i][0]==keycodes[j][0] && keycodes[i][0]!=0) {
          i--;
          y-=12;
          break;
        }
        for (k=2;k<5;k++)
          for (l=2;l<5;l++)
            if (keycodes[i][k]==keycodes[j][l] && keycodes[i][k]!=-2) {
              j=i;
              k=5;
              i--;
              y-=12;
              break; /* Try again if this key already used */
            }
      }
    }
  }

/* Step two: redefine other keys which step one has caused to conflict */

  z=0;
  y-=12;
  for (i=10;i<17;i++) {
    f=false;
    for (j=0;j<10;j++)
      for (k=0;k<5;k++)
        for (l=2;l<5;l++)
          if (keycodes[i][k]==keycodes[j][l] && keycodes[i][k]!=-2)
            f=true;
    for (j=10;j<i;j++)
      for (k=0;k<5;k++)
        for (l=0;l<5;l++)
          if (keycodes[i][k]==keycodes[j][l] && keycodes[i][k]!=-2)
            f=true;
    if (f || (allf && i!=z)) {
      if (i!=z)
        y+=12;
      outtext(keynames[i],0,y,2); /* Red first */
      findkey(i);
      outtext(keynames[i],0,y,1); /* Green once got */
      z=i;
      i--;
    }
  }

/* Step three: save the INI file */

  for (i=0;i<17;i++)
    if (krdf[i]) {
      sprintf(kbuf,"%s%c",keynames[i],(i>=5 && i<10) ? '2' : 0);
      sprintf(vbuf,"%i/%i/%i/%i/%i",keycodes[i][0],keycodes[i][1],
              keycodes[i][2],keycodes[i][3],keycodes[i][4]);
      WriteINIString(INI_KEY_SETTINGS,kbuf,vbuf,ININAME);
    }
}
