/* Digger Remastered
   Copyright (c) Andrew Jenner 1998-2004 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "def.h"
#include "scores.h"
#include "main.h"
#include "drawing.h"
#include "hardware.h"
#include "sound.h"
#include "sprite.h"
#include "input.h"
#include "digger.h"
#include "record.h"

#ifdef _WINDOWS
#include "win_dig.h"
#endif

struct scdat
{
  Sint5 score,nextbs;
} scdat[DIGGERS];

char highbuf[10];

Sint5 scorehigh[12]={0,0,0,0,0,0,0,0,0,0,0,0};

char scoreinit[11][4];

Sint5 scoret=0;

char hsbuf[36];

char scorebuf[512];

Uint4 bonusscore=20000;

bool gotinitflag=FALSE;

void readscores(void);
void writescores(void);
void savescores(void);
void getinitials(void);
void flashywait(Sint4 n);
Sint4 getinitial(Sint4 x,Sint4 y);
void shufflehigh(void);
void writenum(Sint5 n,Sint4 x,Sint4 y,Sint4 w,Sint4 c);
void numtostring(char *p,Sint5 n);

#ifdef ARM

#define SFNAME "Digger:Scores"

#else

#define SFNAME "DIGGER.SCO"

#endif

#ifdef INTDRF
Sint5 getscore0(void)
{
  return scdat[0].score;
}
#endif

void readscores(void)
{
  FILE *in;
  scorebuf[0]=0;
  if (!levfflag) {
    if ((in=fopen(SFNAME,"rb"))!=NULL) {
      fread(scorebuf,512,1,in);
      fclose(in);
    }
  }
  else
    if ((in=fopen(levfname,"rb"))!=NULL) {
      fseek(in,1202,0);
      fread(scorebuf,512,1,in);
      fclose(in);
    }
}

void writescores(void)
{
  FILE *out;
  if (!levfflag) {
    if ((out=fopen(SFNAME,"wb"))!=NULL) {
      fwrite(scorebuf,512,1,out);
      fclose(out);
    }
  }
  else
    if ((out=fopen(levfname,"r+b"))!=NULL) {
      fseek(out,1202,0);
      fwrite(scorebuf,512,1,out);
      fclose(out);
    }
}

void initscores(void)
{
  int i;
  for (i=0;i<diggers;i++)
    addscore(i,0);
}

void loadscores(void)
{
  Sint4 p=0,i,x;
  readscores();
  if (gauntlet)
    p=111;
  if (diggers==2)
    p+=222;
  if (scorebuf[p++]!='s')
    for (i=0;i<11;i++) {
      scorehigh[i+1]=0;
      strcpy(scoreinit[i],"...");
    }
  else
    for (i=1;i<11;i++) {
      for (x=0;x<3;x++)
        scoreinit[i][x]=scorebuf[p++];
      p+=2;
      for (x=0;x<6;x++)
        highbuf[x]=scorebuf[p++];
      scorehigh[i+1]=atol(highbuf);
    }
}

void zeroscores(void)
{
  scdat[0].score=scdat[1].score=0;
  scdat[0].nextbs=scdat[1].nextbs=bonusscore;
  scoret=0;
}

void writecurscore(int col)
{
  if (curplayer==0)
    writenum(scdat[0].score,0,0,6,col);
  else
    if (scdat[1].score<100000L)
      writenum(scdat[1].score,236,0,6,col);
    else
      writenum(scdat[1].score,248,0,6,col);
}

void drawscores(void)
{
  writenum(scdat[0].score,0,0,6,3);
  if (nplayers==2 || diggers==2)
    if (scdat[1].score<100000L)
      writenum(scdat[1].score,236,0,6,3);
    else
      writenum(scdat[1].score,248,0,6,3);
}

void addscore(int n,Sint4 score)
{
  scdat[n].score+=score;
  if (scdat[n].score>999999L)
    scdat[n].score=0;
  if (n==0)
    writenum(scdat[n].score,0,0,6,1);
  else
    if (scdat[n].score<100000L)
      writenum(scdat[n].score,236,0,6,1);
    else
      writenum(scdat[n].score,248,0,6,1);
  if (scdat[n].score>=scdat[n].nextbs+n &&   /* +n to reproduce original bug */
      scdat[n].score<1000000L) {
    if (getlives(n)<5 || unlimlives) {
      if (gauntlet)
        cgtime+=17897715L; /* 15 second time bonus instead of the life */
      else
        addlife(n);
      drawlives();
    }
    scdat[n].nextbs+=bonusscore;
  }
  incpenalty();
  incpenalty();
  incpenalty();
}

void endofgame(void)
{
  Sint4 i;
  bool initflag=FALSE;
  for (i=0;i<diggers;i++)
    addscore(i,0);
  if (playing || !drfvalid)
    return;
  if (gauntlet) {
    cleartopline();
    outtext("TIME UP",120,0,3);
    for (i=0;i<50 && !escape;i++)
      newframe();
    outtext("       ",120,0,3);
  }
  for (i=curplayer;i<curplayer+diggers;i++) {
    scoret=scdat[i].score;
    if (scoret>scorehigh[11]) {
      gclear();
      drawscores();
      strcpy(pldispbuf,"PLAYER ");
      if (i==0)
        strcat(pldispbuf,"1");
      else
        strcat(pldispbuf,"2");
      outtext(pldispbuf,108,0,2);
      outtext(" NEW HIGH SCORE ",64,40,2);
      getinitials();
      shufflehigh();
      savescores();
      initflag=TRUE;
    }
  }
  if (!initflag && !gauntlet) {
    cleartopline();
    outtext("GAME OVER",104,0,3);
    for (i=0;i<50 && !escape;i++)
      newframe();
    outtext("         ",104,0,3);
    setretr(TRUE);
  }
}

void showtable(void)
{
  Sint4 i,col;
  outtext("HIGH SCORES",16,25,3);
  col=2;
  for (i=1;i<11;i++) {
    strcpy(hsbuf,"");
    strcat(hsbuf,scoreinit[i]);
    strcat(hsbuf," ");
    numtostring(highbuf,scorehigh[i+1]);
    strcat(hsbuf,highbuf);
    outtext(hsbuf,16,31+13*i,col);
    col=1;
  }
}

void savescores(void)
{
  Sint4 i,p=0,j;
  if (gauntlet)
    p=111;
  if (diggers==2)
    p+=222;
  strcpy(scorebuf+p,"s");
  for (i=1;i<11;i++) {
    strcpy(hsbuf,"");
    strcat(hsbuf,scoreinit[i]);
    strcat(hsbuf," ");
    numtostring(highbuf,scorehigh[i+1]);
    strcat(hsbuf,highbuf);
    for (j=0;j<11;j++)
      scorebuf[p+j+i*11-10]=hsbuf[j];
  }
  writescores();
}

void getinitials(void)
{
  Sint4 k,i;
#ifdef _WINDOWS
  pause_windows_sound_playback();
#endif
  newframe();
  outtext("ENTER YOUR",100,70,3);
  outtext(" INITIALS",100,90,3);
  outtext("_ _ _",128,130,3);
  strcpy(scoreinit[0],"...");
  killsound();
  for (i=0;i<3;i++) {
    k=0;
    while (k==0) {
      k=getinitial(i*24+128,130);
      if (k==8 || k==127) {
        if (i>0)
          i--;
        k=0;
      }
    }
    if (k!=0) {
      gwrite(i*24+128,130,k,3);
      scoreinit[0][i]= static_cast<char>(k);
    }
  }
  for (i=0;i<20;i++)
#ifdef _WINDOWS
    flashywait(2);
#else
    flashywait(15);
#endif
  setupsound();
  gclear();
  gpal(0);
  ginten(0);
  setretr(TRUE);
  recputinit(scoreinit[0]);
#ifdef _WINDOWS
  resume_windows_sound_playback();
#endif
}

void flashywait(Sint4 n)
{
  Sint4 i,gt,cx,p=0;
  Sint3 gap=19;
  setretr(FALSE);
  for (i=0;i<(n<<1);i++)
    for (cx=0;cx<volume;cx++) {
      gpal(p=1-p);
#ifdef _WINDOWS
      for (gt=0;gt<gap;gt++)
        do_windows_events();
#else
      for (gt=0;gt<gap;gt++);
#endif
    }
}

Sint4 getinitial(Sint4 x,Sint4 y)
{
  Sint4 i;
  gwrite(x,y,'_',3);
  do {

#ifdef _WINDOWS
    do_windows_events();
#endif

    for (i=0;i<40;i++) {
      if (kbhit())
        return getkey();
#ifdef _WINDOWS
      flashywait(5);
#else
      flashywait(15);
#endif
    }
    for (i=0;i<40;i++) {
      if (kbhit()) {
        gwrite(x,y,'_',3);
        return getkey();
      }
#ifdef _WINDOWS
      flashywait(5);
#else
      flashywait(15);
#endif
    }
  } while (1);
}

void shufflehigh(void)
{
  Sint4 i,j;
  for (j=10;j>1;j--)
    if (scoret<scorehigh[j])
      break;
  for (i=10;i>j;i--) {
    scorehigh[i+1]=scorehigh[i];
    strcpy(scoreinit[i],scoreinit[i-1]);
  }
  scorehigh[j+1]=scoret;
  strcpy(scoreinit[j],scoreinit[0]);
}

void scorekill(int n)
{
  addscore(n,250);
}

void scorekill2(void)
{
  addscore(0,125);
  addscore(1,125);
}

void scoreemerald(int n)
{
  addscore(n,25);
}

void scoreoctave(int n)
{
  addscore(n,250);
}

void scoregold(int n)
{
  addscore(n,500);
}

void scorebonus(int n)
{
  addscore(n,1000);
}

void scoreeatm(int n,int msc)
{
  addscore(n,msc*200);
}

void writenum(Sint5 n,Sint4 x,Sint4 y,Sint4 w,Sint4 c)
{
  Sint4 d,xp=(w-1)*12+x;
  n%=1000000L;
  while (w>0) {
    d=(Sint4)(n%10);
    if (w>1 || d>0)
      gwrite(xp,y,d+'0',c);
    n/=10;
    w--;
    xp-=12;
  }
}

void numtostring(char *p,Sint5 n)
{
  int x;
  for (x=0;x<7;x++) {
    p[6-x]=(Sint3)(n%10L)+'0';
    n/=10L;
    if (n==0L) {
      x++;
      break;
    }
  }
  for (;x<7;x++)
    p[6-x]=' ';
  p[7]=0;
}
