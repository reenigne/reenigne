/* Digger Remastered
   Copyright (c) Andrew Jenner 1998-2004 */

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include "def.h"
#include "edasm.h"

Uint4 bonusscore=20000;

Sint3 leveldat[8][MHEIGHT][MWIDTH]=
{{"S   B     HHHHS",
  "V  CC  C  V B  ",
  "VB CC  C  V    ",
  "V  CCB CB V CCC",
  "V  CC  C  V CCC",
  "HH CC  C  V CCC",
  " V    B B V    ",
  " HHHH     V    ",
  "C   V     V   C",
  "CC  HHHHHHH  CC"},
 {"SHHHHH  B B  HS",
  " CC  V       V ",
  " CC  V CCCCC V ",
  "BCCB V CCCCC V ",
  "CCCC V       V ",
  "CCCC V B  HHHH ",
  " CC  V CC V    ",
  " BB  VCCCCV CC ",
  "C    V CC V CC ",
  "CC   HHHHHH    "},
 {"SHHHHB B BHHHHS",
  "CC  V C C V BB ",
  "C   V C C V CC ",
  " BB V C C VCCCC",
  "CCCCV C C VCCCC",
  "CCCCHHHHHHH CC ",
  " CC  C V C  CC ",
  " CC  C V C     ",
  "C    C V C    C",
  "CC   C H C   CC"},
 {"SHBCCCCBCCCCBHS",
  "CV  CCCCCCC  VC",
  "CHHH CCCCC HHHC",
  "C  V  CCC  V  C",
  "   HHH C HHH   ",
  "  B  V B V  B  ",
  "  C  VCCCV  C  ",
  " CCC HHHHH CCC ",
  "CCCCC CVC CCCCC",
  "CCCCC CHC CCCCC"},
 {"SHHHHHHHHHHHHHS",
  "VBCCCCBVCCCCCCV",
  "VCCCCCCV CCBC V",
  "V CCCC VCCBCCCV",
  "VCCCCCCV CCCC V",
  "V CCCC VBCCCCCV",
  "VCCBCCCV CCCC V",
  "V CCBC VCCCCCCV",
  "VCCCCCCVCCCCCCV",
  "HHHHHHHHHHHHHHH"},
 {"SHHHHHHHHHHHHHS",
  "VCBCCV V VCCBCV",
  "VCCC VBVBV CCCV",
  "VCCCHH V HHCCCV",
  "VCC V CVC V CCV",
  "VCCHH CVC HHCCV",
  "VC V CCVCC V CV",
  "VCHHBCCVCCBHHCV",
  "VCVCCCCVCCCCVCV",
  "HHHHHHHHHHHHHHH"},
 {"SHCCCCCVCCCCCHS",
  " VCBCBCVCBCBCV ",
  "BVCCCCCVCCCCCVB",
  "CHHCCCCVCCCCHHC",
  "CCV CCCVCCC VCC",
  "CCHHHCCVCCHHHCC",
  "CCCCV CVC VCCCC",
  "CCCCHH V HHCCCC",
  "CCCCCV V VCCCCC",
  "CCCCCHHHHHCCCCC"},
 {"HHHHHHHHHHHHHHS",
  "V CCBCCCCCBCC V",
  "HHHCCCCBCCCCHHH",
  "VBV CCCCCCC VBV",
  "VCHHHCCCCCHHHCV",
  "VCCBV CCC VBCCV",
  "VCCCHHHCHHHCCCV",
  "VCCCC V V CCCCV",
  "VCCCCCV VCCCCCV",
  "HHHHHHHHHHHHHHH"}};

void (*gline)(int x0,int y0,int x1,int y1,int c,int m)=vgaline;
void (*ginit)(void)=vgainit;
void (*gclear)(void)=vgaclear;
void (*gpal)(Sint4 pal)=vgapal;
void (*ginten)(Sint4 inten)=vgainten;
void (*gputi)(Sint4 x,Sint4 y,Uint3 *p,Sint4 w,Sint4 h)=vgaputi;
void (*ggeti)(Sint4 x,Sint4 y,Uint3 *p,Sint4 w,Sint4 h)=vgageti;
void (*gputim)(Sint4 x,Sint4 y,Sint4 ch,Sint4 w,Sint4 h)=vgaputim;
void (*gwrite)(Sint4 x,Sint4 y,Sint4 ch,Sint4 c)=vgawrite;
int ydiv=1;

void drawrightblob(Sint4 x,Sint4 y)
{
  gputim(x+16,y-1,102,2,18);
}

void drawleftblob(Sint4 x,Sint4 y)
{
  gputim(x-8,y-1,104,2,18);
}

void drawtopblob(Sint4 x,Sint4 y)
{
  gputim(x-4,y-6,103,6,6);
}

void drawbottomblob(Sint4 x,Sint4 y)
{
  gputim(x-4,y+15,105,6,6);
}

void drawfieldelement(int x,int y,char c,char cr,char cd)
{
  int f=-1;
  if (c=='S' || c=='V')
    f&=0xd03f;
  if (c=='S' || c=='H')
    f&=0xdfe0;
  if (c=='B')
    gputim(x,y,62,4,15);
  if (c=='C')
    gputim(x,y+3,108,4,10);
  if (f&0x2000)
    return;
  if ((f&0xfc0)!=0xfc0) {
    f&=0xd03f;
    drawbottomblob(x,y-15);
    drawbottomblob(x,y-12);
    drawbottomblob(x,y-9);
    drawbottomblob(x,y-6);
    drawbottomblob(x,y-3);
    drawtopblob(x,y+3);
  }
  if ((f&0x1f)!=0x1f) {
    f&=0xdfe0;
    drawrightblob(x-16,y);
    drawrightblob(x-12,y);
    drawrightblob(x-8,y);
    drawrightblob(x-4,y);
    drawleftblob(x+4,y);
  }
  if (x<292) {
    f=-1;
    if (cr=='S' || cr=='V')
      f&=0xd03f;
    if (cr=='S' || cr=='H')
      f&=0xdfe0;
    if ((f&0xfdf)!=0xfdf)
      drawrightblob(x,y);
  }
  if (y<180) {
    f=-1;
    if (cd=='S' || cd=='V')
      f&=0xd03f;
    if (cd=='S' || cd=='H')
      f&=0xdfe0;
    if ((f&0xfdf)!=0xfdf)
      drawbottomblob(x,y);
  }
}

void writenum(Sint5 n,Sint4 x,Sint4 y,Sint4 w,Sint4 c)
{
  Sint4 d,xp=(w-1)*12+x;
  while (w>0) {
    d=(Sint4)(n%10);
    if (w>=1 || d>0)
      gwrite(xp,y,d+'0',c);
    n/=10;
    w--;
    xp-=12;
  }
}

void processkey(int key)
{
}

bool biosflag=FALSE,retrflag=TRUE;

char fn2[256];

char *parsecmd(int argc,char *argv[])
{
  char *filename="EDITED";
  char *word;
  int arg,i;
  bool f=FALSE;
  for (arg=1;arg<argc;arg++) {
    word=argv[arg];
    if (word[0]=='/' || word[0]=='-') {
      if (word[1]=='L' || word[1]=='l')
        if (word[2]==':')
          i=3;
        else
          i=2;
      if (word[1]=='L' || word[1]=='l')
        filename=word;
      if (word[1]=='?' || word[1]=='h' || word[1]=='H') {
        printf("DIGLEVED - Copyright (c) 1999 AJ Software\n"
               "Version: "DIGGER_VERSION"\n\n"

                "Command line syntax:\n"
               "  DIGLEVED [[/L:]level file] [/C]\n\n"

               "/C = Use CGA graphics\n");
        exit(1);
      }
      if (word[1]=='C' || word[1]=='c') {
        ginit=cgainit;
        gpal=cgapal;
        ginten=cgainten;
        gclear=cgaclear;
        gputi=cgaputi;
        ggeti=cgageti;
        gputim=cgaputim;
        gwrite=cgawrite;
        gline=cgaline;
        ginit();
        gpal(0);
        ydiv=2;
      }
    }
    else
      filename=word;
  }

  for (i=0;filename[i]!=0;i++)
    if (filename[i]=='.')
      f=TRUE;
  if (!f) {
    sprintf(fn2,"%s.DLF",filename);
    return fn2;
  }
  return filename;
}


void main(int argc,char *argv[])
{
  int x,y,l=0,c,b,k,l0=-1,y0,x0,x1,y1;
  FILE *levf;
  char *filename=parsecmd(argc,argv);

  levf=fopen(filename,"rb");
  if (levf!=NULL) {
    fread(&bonusscore,2,1,levf);
    fread(leveldat,1200,1,levf);
    fclose(levf);
  }
  levf=fopen(filename,"wb");
  if (levf==NULL) {
    printf("Cannot open level file %s: %s\n\n",filename,sys_errlist[errno]);
    exit(1);
  }
  fwrite(&bonusscore,2,1,levf);
  fwrite(leveldat,1200,1,levf);
  fclose(levf);

  b=mouseinit();
  if (b==0) {
    printf("You need a mouse and a DOS mouse driver to use this program.\n");
    exit(1);
  }
  ginit();
  mouseshow();
  mousebox(0,0,639,399/ydiv);
  mouseput(320,200/ydiv);

  gpal(0);
  initkeyb();

  do {
    if (l0!=l) {
      mousehide();
      gclear();
      for (y=-2;y<14;y+=4)
        for (x=40;x<232;x+=20)
          gputim(x,y,94+l,5,4);
      drawfieldelement(88,-3,'H','H',0);
      drawfieldelement(120,0,'V',0,0);
      drawfieldelement(152,-4,'S',0,0);
      drawfieldelement(184,-2,'C',0,0);
      drawfieldelement(216,-1,'B',0,0);
      for (y=14;y<200;y+=4)
        for (x=0;x<320;x+=20)
          gputim(x,y,94+l,5,4);
      for (x=0;x<MWIDTH;x++)
        for (y=0;y<MHEIGHT;y++)
          drawfieldelement(x*20+12,y*18+18,leveldat[l][y][x],
                           leveldat[l][y][x+1],leveldat[l][y+1][x]);
      writenum(1+l,12,0,1,2);
      writenum(bonusscore,248,0,5,1);
      mouseshow();
      l0=l;
    }
    k=0;
    do {
      b=mousepos(&x,&y);
      if (kbhit())
        k=getkey();
      y/=(3-ydiv);
      x/=2;
    } while (k==0 && b==0);
    if (k==27)
      break;
    if (y<14)
      if (x<40) {
        mousehide();
        mousebox(0,0,80,127);
        y0=16*(7-l);
        mouseput(x*2,y0);
        do {
          b=mousepos(&x,&y);
          if (y!=y0)
            writenum(8-y/16,12,0,1,2);
        } while (b!=0);
        l=7-y/16;
        mousebox(0,0,639,399/ydiv);
        mouseput(x,14/ydiv);
        mouseshow();
      }
      else
        if (x>232) {
          mousehide();
          mousebox(464,0,639,2621);
          y0=2621-bonusscore/25l;
          mouseput(x*2,y0);
          do {
            b=mousepos(&x,&y);
            if (y!=y0)
              writenum((2621-y)*25l,248,0,5,1);
          } while (b!=0);
          bonusscore=(2621-y)*25l;
          mousebox(0,0,639,399/ydiv);
          mouseput(x,14/ydiv);
          mouseshow();
        }
        else {
          mousebox(80,0,464,28/ydiv);
          do {
            b=mousepos(&x,&y);
          } while (b!=0);
          mousebox(0,0,639,399/ydiv);
          c=" HVSCB"[(x/2-40)/32];
          x0=y0=-1;
        }
    else {
      mousebox(0,28/ydiv,639,399/ydiv);
      do {
        b=mousepos(&x,&y);
        x/=2;
        y/=(3-ydiv);
        y=(y-18)/18;
        if (y<0) y=0;
        if (y>9) y=9;
        x=(x-12)/20;
        if (x<0) x=0;
        if (x>14) x=14;
        if (x!=x0 || y!=y0) {
          x0=x;
          y0=y;
          leveldat[l][y][x]=c;
          x1=x*20+12;
          y1=y*18+18;
          mousehide();
          for (y=((y1-2)&(-4))-2;y<(y1&(-4))+20;y+=4)
            for (x=0;x<40;x+=20)
              gputim(x1-(x1%20)+x,y,94+l,5,4);
          x=(x1-12)/20;
          y=(y1-18)/18;
          for (x1=x-1;x1<=x+1;x1++)
            for (y1=y-1;y1<=y+1;y1++)
              if (x1>=0 && y1>=0 && x1<=14 && y1<=9)
                drawfieldelement(x1*20+12,y1*18+18,leveldat[l][y1][x1],
                                 leveldat[l][y1][x1+1],leveldat[l][y1+1][x1]);
          mouseshow();
        }
      } while (b!=0);
      mousebox(0,0,639,399/ydiv);
    }
  } while (k!=27);
  restorekeyb();
  graphicsoff();

  levf=fopen(filename,"wb");
  if (levf==NULL) {
    printf("Cannot open level file %s: %s\n\n",filename,sys_errlist[errno]);
    exit(1);
  }
  fwrite(&bonusscore,2,1,levf);
  fwrite(leveldat,1200,1,levf);
  fclose(levf);

}
