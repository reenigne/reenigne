/* Digger Remastered
   Copyright (c) Andrew Jenner 1998-2004 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "def.h"

#ifndef FLATFILE
#if defined (_WINDOWS) && !defined (WIN32)
#include <malloc.h>
#else
#include <alloc.h>
#endif
#endif

#ifdef _WINDOWS
#include "win_dig.h"
#endif

#include "record.h"
#include "hardware.h"
#include "sound.h"
#include "input.h"
#include "main.h"
#include "scores.h"
#include "sprite.h"

char huge *recb,huge *plb,huge *plp;

bool playing=false,savedrf=false,gotname=false,gotgame=false,drfvalid=true,
     kludge=false;

WCHAR rname[128];

int reccc=0,recrl=0,rlleft=0;
Uint5 recp=0;
char recd,rld;

void mprintf(char *f,...);
void makedir(Sint4 *dir,bool *fire,char d);
char maked(Sint4 dir,bool fire);

#ifdef ARM
#define DEFAULTSN "Digger:Lastgame"
#else
#define DEFAULTSN L"DIGGER.DRF"
#endif

#ifdef INTDRF
FILE *info;
#endif

void openplay(WCHAR *name)
{
  FILE *playf=_wfopen(name,L"rt");
  Sint5 l,i;
  char c,buf[80];
  int x,y,n,origgtime=gtime;
  bool origg=gauntlet;
  Sint4 origstartlev=startlev,orignplayers=nplayers,origdiggers=diggers;
#ifdef INTDRF
  info=_wfopen(L"DRFINFO.TXT",L"wt");
#endif
  if (playf==NULL) {
    escape=true;
    return;
  }
  rlleft=0;
  gauntlet=false;
  startlev=1;
  nplayers=1;
  diggers=1;
  /* The file is in two distint parts. In the first, line breaks are used as
     separators. In the second, they are ignored. This is the first. */

  fgets(buf,80,playf); /* Get id string */
  if (buf[0]!='D' || buf[1]!='R' || buf[2]!='F') {
    fclose(playf);
    escape=true;
    return;
  }
  fgets(buf,80,playf); /* Get version for kludge switches */
  if (atol(buf+7)<=19981125l)
    kludge=true;
  fgets(buf,80,playf); /* Get mode */
  if (*buf=='1') {
    nplayers=1;
    x=1;
  }
  else
    if (*buf=='2') {
      nplayers=2;
      x=1;
    }
    else {
      if (*buf=='M') {
        diggers=buf[1]-'0';
        x=2;
      }
      else
        x=0;
      if (buf[x]=='G') {
        gauntlet=true;
        x++;
        gtime=atoi(buf+x);
        while (buf[x]>='0' && buf[x]<='9')
          x++;
      }
    }
  if (buf[x]=='U') /* Unlimited lives are ignored on playback. */
    x++;
  if (buf[x]=='I')
    startlev=atoi(buf+x+1);
  fgets(buf,80,playf); /* Get bonus score */
  bonusscore=atoi(buf);
  for (n=0;n<8;n++)
    for (y=0;y<10;y++) {
      for (x=0;x<15;x++)
        buf[x]=' ';
      fgets(buf,80,playf); /* Get a line of map */
      for (x=0;x<15;x++)
        leveldat[((n*MHEIGHT) + y)*MWIDTH + x]=buf[x];
    }

  /* This is the second. The line breaks here really are only so that the file
     can be emailed. */

  i=ftell(playf);
  fseek(playf,0,SEEK_END);
  l=ftell(playf)-i;
  fseek(playf,i,SEEK_SET);
  plb=plp=(char huge *)farmalloc(l);
  if (plb==(char huge *)NULL) {
    fclose(playf);
    escape=true;
    return;
  }

  for (i=0;i<l;i++) {
    c=fgetc(playf); /* Get everything that isn't line break into 1 string */
    if (c>=' ')
      *(plp++)=c;
  }
  fclose(playf);
  plp=plb;

  playing=true;
  recinit();
  game();
  gotgame=true;
  playing=false;
  farfree(plb);
  gauntlet=origg;
  gtime=origgtime;
  kludge=false;
  startlev=origstartlev;
  diggers=origdiggers;
  nplayers=orignplayers;
}

void recstart(void)
{
#if defined FLATFILE || defined WIN16
  Uint5 s=MAX_REC_BUFFER;
  do {
    recb=(char huge *)farmalloc(s);
    if (recb==NULL)
      s>>=1;
  } while (recb==(char huge *)NULL && s>1024);
#else
  Uint5 s=farcoreleft();
  if (s>MAX_REC_BUFFER)
    s=MAX_REC_BUFFER;
  recb=(char huge *)farmalloc(s);
#endif
  if (recb==NULL) {
    finish();
#ifdef _WINDOWS
    MessageBox(hWnd, L"Cannot allocate memory for recording buffer.\n",
        L"Error", MB_OK);
#else
    printf("Cannot allocate memory for recording buffer.\n");
#endif
    exit(1);
  }
  recp=0;
}

void mprintf(char *f,...)
{
  va_list ap;
  char buf[80];
  int i,l;
  va_start(ap,f);
  vsprintf(buf,f,ap);
  l=strlen(buf);
  for (i=0;i<l;i++)
    recb[recp+i]=buf[i];
  recp+=l;
  if (recp>MAX_REC_BUFFER-80)
    recp=0;          /* Give up, file is too long */
}

void makedir(Sint4 *dir,bool *fire,char d)
{
  if (d>='A' && d<='Z') {
    *fire=true;
    d-='A'-'a';
  }
  else
    *fire=false;
  switch (d) {
    case 's': *dir=DIR_NONE; break;
    case 'r': *dir=DIR_RIGHT; break;
    case 'u': *dir=DIR_UP; break;
    case 'l': *dir=DIR_LEFT; break;
    case 'd': *dir=DIR_DOWN; break;
  }
}

void playgetdir(Sint4 *dir,bool *fire)
{
  if (rlleft>0) {
    makedir(dir,fire,rld);
    rlleft--;
  }
  else {
    if (*plp=='E' || *plp=='e') {
      escape=true;
      return;
    }
    rld=*(plp++);
    while (*plp>='0' && *plp<='9')
      rlleft=rlleft*10+((*(plp++))-'0');
    makedir(dir,fire,rld);
    if (rlleft>0)
      rlleft--;
  }
}

char maked(Sint4 dir,bool fire)
{
  char d;
  if (dir==DIR_NONE)
    d='s';
  else
    d="ruld"[dir>>1];
  if (fire)
    d+='A'-'a';
  return d;
}

void putrun(void)
{
  if (recrl>1)
    mprintf("%c%i",recd,recrl);
  else
    mprintf("%c",recd);
  reccc++;
  if (recrl>1) {
    reccc++;
    if (recrl>=10) {
      reccc++;
      if (recrl>=100)
        reccc++;
    }
  }
  if (reccc>=60) {
    mprintf("\n");
    reccc=0;
  }
}

void recputdir(Sint4 dir,bool fire)
{
  char d=maked(dir,fire);
  if (recrl==0)
    recd=d;
  if (recd!=d) {
    putrun();
    recd=d;
    recrl=1;
  }
  else {
    if (recrl==999) {
      putrun(); /* This probably won't ever happen. */
      recrl=0;
    }
    recrl++;
  }
}

void recinit(void)
{
  int x,y,l;
  recp=0;
  drfvalid=true;
  mprintf("DRF\n"); /* Required at start of DRF */
  if (kludge)
    mprintf("AJ DOS 19981125\n");
  else
    mprintf(DIGGER_VERSION"\n");
  if (diggers>1) {
    mprintf("M%i",diggers);
    if (gauntlet)
      mprintf("G%i",gtime);
  }
  else
    if (gauntlet)
      mprintf("G%i",gtime);
    else
      mprintf("%i",nplayers);
/*  if (unlimlives)
    mprintf("U"); */
  if (startlev>1)
    mprintf("I%i",startlev);
  mprintf("\n%i\n",bonusscore);
  for (l=0;l<8;l++) {
    for (y=0;y<MHEIGHT;y++) {
      for (x=0;x<MWIDTH;x++)
        mprintf("%c",leveldat[((l*MHEIGHT) + y)*MWIDTH + x]);
      mprintf("\n");
    }
  }
  reccc=recrl=0;
}

void recputrand(Uint5 randv)
{
  mprintf("%08lX\n",randv);
  reccc=recrl=0;
}

/* This is a mess. Sorry. */

void recsavedrf(void)
{
  FILE *recf;
  Uint5 i;
  int j;
  bool gotfile=true;
  WCHAR nambuf[FILENAME_BUFFER_SIZE];
  char init[4];
  if (!drfvalid)
    return;
  if (gotname)
#ifdef _WINDOWS
    wcscpy(nambuf,rname);
#else
    if ((recf=fopen(rname,"wt"))==NULL)
      gotname=false;
    else
      gotfile=true;
#endif
  if (!gotname) {
    if (nplayers==2)
#ifdef _WINDOWS
      wcscpy(nambuf,DEFAULTSN);
#else
      recf=fopen(DEFAULTSN,"wt"); /* Should get a name, really */
#endif
    else {
      for (j=0;j<3;j++) {
        init[j]=scoreinit[0][j];
        if (!((init[j]>='A' && init[j]<='Z') ||
              (init[j]>='a' && init[j]<='z')))
          init[j]='_';
      }
      init[3]=0;
      if (scoret<100000l)
        swprintf(nambuf,FILENAME_BUFFER_SIZE, L"%s%li",init,scoret);
      else
        if (init[2]=='_')
          swprintf(nambuf,FILENAME_BUFFER_SIZE, L"%c%c%li",init[0],init[1],scoret);
        else
          if (init[0]=='_')
            swprintf(nambuf,FILENAME_BUFFER_SIZE, L"%c%c%li",init[1],init[2],scoret);
          else
            swprintf(nambuf,FILENAME_BUFFER_SIZE, L"%c%c%li",init[0],init[2],scoret);
#ifndef ARM
      wcscat(nambuf,L".drf");
#endif
#ifndef _WINDOWS
      recf=fopen(nambuf,"wt");
#endif
    }
#ifdef _WINDOWS
    if (!get_open_save_filename(SAVE,"Save Recorded Game","Digger Record Files (*.drf)\0*.DRF\0All Files (*.*)\0*.*\0","DRF", nambuf))
      return;
    recf=_wfopen(nambuf,L"wt");
#endif

    if (recf==NULL)
      gotfile=false;
    else
      gotfile=true;
  }
  if (!gotfile)
    return;
  for (i=0;i<recp;i++)
    fputc(recb[i],recf);
  fclose(recf);
  gotgame=false;
#ifdef _WINDOWS
  wcscpy(drf_filename,nambuf);
#endif

}

void playskipeol(void)
{
  plp+=3;
}

Uint5 playgetrand(void)
{
  int i;
  Uint5 r=0;
  char p;
  if ((*plp)=='*')
    plp+=4;
  for (i=0;i<8;i++) {
    p=*(plp++);
    if (p>='0' && p<='9')
      r|=(Uint5)(p-'0')<<((7-i)<<2);
    if (p>='A' && p<='F')
      r|=(Uint5)(p-'A'+10)<<((7-i)<<2);
    if (p>='a' && p<='f')
      r|=(Uint5)(p-'a'+10)<<((7-i)<<2);
  }
  return r;
}

void recputinit(char *init)
{
  mprintf("*%c%c%c\n",init[0],init[1],init[2]);
}

void recputeol(void)
{
  if (recrl>0)
    putrun();
  if (reccc>0)
    mprintf("\n");
  mprintf("EOL\n");
}

void recputeog(void)
{
  mprintf("EOG\n");
}

void recname(WCHAR *name)
{
  gotname=true;
  wcscpy(rname,name);
}
