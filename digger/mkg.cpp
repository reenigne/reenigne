/* Digger Remastered
   Copyright (c) Andrew Jenner 1998-2004 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "def.h"

FILE *in,*out;

unsigned char buf[1024];

void run(char *data,int l,bool ef)
{
  int i;
  fprintf(out,"  ");
  for (i=0;i<l;i++) {
    fprintf(out,"0x%02x",data[i]&0xff);
    if (i==l-1 && ef)
      fprintf(out,"};\n");
    else
      fprintf(out,",");
  }
  fprintf(out,"\n");
}

#define nspr 107

char *gnames[]={
  "emerald","sbag","rbag","lbag","fbag","gold1","gold2","gold3","nobbin3",
  "nobbin2","nobbin1","nobbind","rhobbin1","rhobbin2","rhobbin3","rhobbind",
  "lhobbin1","lhobbin2","lhobbin3","lhobbind","bonus","rdigger3","rxdigger3",
  "rdigger2","rxdigger2","rdigger1","rxdigger1","ldigger3","lxdigger3",
  "ldigger2","lxdigger2","ldigger1","lxdigger1","udigger3","uxdigger3",
  "udigger2","uxdigger2","udigger1","uxdigger1","ddigger3","dxdigger3",
  "ddigger2","dxdigger2","ddigger1","dxdigger1","rbigger3","rxbigger3",
  "rbigger2","rxbigger2","rbigger1","rxbigger1","lbigger3","lxbigger3",
  "lbigger2","lxbigger2","lbigger1","lxbigger1","ubigger3","uxbigger3",
  "ubigger2","uxbigger2","ubigger1","uxbigger1","dbigger3","dxbigger3",
  "dbigger2","dxbigger2","dbigger1","dxbigger1","grave5","grave4","grave3",
  "grave2","grave1","diggerd","biggerd","liferight","lifeleft","elife",
  "leftlife","rightblob","leftblob","topblob","bottomblob","furryblob",
  "squareblob","edigger","back1","back2","back3","back4","back5","back6",
  "back7","back8","fire1","fire2","fire3","exp1","exp2","exp3","fire1b",
  "fire2b","fire3b","exp1b","exp2b","exp3b"};

int heights[]={
  10,15,15,15,15,15,15,15,15,
  15,15,15,15,15,15,15,
  15,15,15,14,15,15,15,
  15,15,15,15,15,15,
  15,15,15,15,15,15,
  15,15,15,15,15,15,
  15,15,15,15,15,15,
  15,15,15,15,15,15,
  15,15,15,15,15,15,
  15,15,15,15,15,15,
  15,15,15,15,15,15,15,
  15,15,15,15,12,12,12,
  12,18,18,6,6,8,
  6,15,4,4,4,4,4,4,
  4,4,8,8,8,8,8,8,8,
  8,8,8,8,8};

int widths[]={
  4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,
  4,4,4,4,4,4,
  4,4,4,4,4,4,
  4,4,4,4,4,4,
  4,4,4,4,4,4,
  4,4,4,4,4,4,
  4,4,4,4,4,4,
  4,4,4,4,4,4,
  4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,
  4,2,2,6,6,6,
  6,4,5,5,5,5,5,5,
  5,5,2,2,2,2,2,2,2,
  2,2,2,2,2};

FILE *fileopen(char *name,char *mode)
{
  FILE *f;
  f=fopen(name,mode);
  if (f==NULL) {
    printf("Error: Cannot open file %s with mode %s\n",name,mode);
    exit(1);
  }
  return f;
}

void mkgv(void)
{
  int i,x,y,b,p;
  char cbuf[24]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

  in=fileopen("vga.spr","rb");

#ifdef ARM

  fprintf(out,"Uint3 vgazero480[]={\n");
  for (y=0;y<30;y++)
    run(cbuf,16,y==29);

  for (i=0;i<nspr;i++) {
    fread(buf,1024,1,in);
    if (i<80 || i>=95) { /* Normal data */
      fprintf(out,"Uint3 vga%s[]={\n",gnames[i]);
      for (y=0;y<(heights[i]<<1);y++) {
        for (x=0;x<(widths[i]<<2);x++) {
          cbuf[x]=0;
          for (b=0;b<2;b++)
            if (buf[(y<<5)+(x<<1)+b]!=0xff)
              cbuf[x]|=buf[(y<<5)+(x<<1)+b]<<(b<<2);
        }
        run(cbuf,widths[i]<<2,y==(heights[i]<<1)-1);
      }
    }
    if (i>=87 && i<95) { /* Background data */
      fprintf(out,"Uint3 vga%s[]={\n",gnames[i]);
      for (y=0;y<8;y++) {
        for (x=0;x<16;x++) {
          cbuf[x]=0;
          for (b=0;b<2;b++)
            cbuf[x]|=buf[(y<<5)+(x<<1)+b]<<(b<<2);
        }
        for (x=0;x<4;x++) {
          cbuf[16+x]=0;
          for (b=0;b<2;b++)
            cbuf[16+x]|=buf[(y<<5)+(x<<1)+512+b]<<(b<<2);
        }
        run(cbuf,widths[i]<<2,y==7);
      }
    }
    if (i==80 || i==81) { /* Tall masks */
      fprintf(out,"Uint3 vga%smask[]={\n",gnames[i]);
      for (y=0;y<32;y++) {
        for (x=0;x<(widths[i]<<2);x++) {
          cbuf[x]=0;
          for (b=0;b<2;b++)
            if (buf[(y<<5)+(x<<1)+b]&15)
              cbuf[x]|=15<<(b<<2);
        }
        run(cbuf,widths[i]<<2,FALSE);
      }
      for (y=0;y<4;y++) {
        for (x=0;x<(widths[i]<<2);x++) {
          cbuf[x]=0;
          for (b=0;b<2;b++)
            if (buf[(y<<5)+(x<<1)+b+16]&15)
              cbuf[x]|=15<<(b<<2);
        }
        run(cbuf,widths[i]<<2,y==3);
      }
    }
    if (i>=82 && i<86) { /* Wide masks */
      fprintf(out,"Uint3 vga%smask[]={\n",gnames[i]);
      for (y=0;y<(heights[i]<<1);y++) {
        for (x=0;x<16;x++) {
          cbuf[x]=0;
          for (b=0;b<2;b++)
            if (buf[(y<<5)+(x<<1)+b]&15)
              cbuf[x]|=15<<(b<<2);
        }
        for (x=0;x<8;x++) {
          cbuf[x+16]=0;
          for (b=0;b<2;b++)
            if (buf[(y<<5)+(x<<1)+512+b]&15)
              cbuf[x+16]|=15<<(b<<2);
        }
        run(cbuf,widths[i]<<2,y==(heights[i]<<1)-1);
      }
    }
    if (i<80 || i>=95 || i==86) { /* Normal masks */
      fprintf(out,"Uint3 vga%smask[]={\n",gnames[i]);
      for (y=0;y<(heights[i]<<1);y++) {
        for (x=0;x<(widths[i]<<2);x++) {
          cbuf[x]=0;
          for (b=0;b<2;b++)
            if (buf[(y<<5)+(x<<1)+b]&128)
              cbuf[x]|=15<<(b<<2);
        }
        run(cbuf,widths[i]<<2,y==(heights[i]<<1)-1);
      }
    }
  }

#else

  fprintf(out,"Uint3 vgazero480[]={\n");
  for (y=0;y<120;y++)
    run(cbuf,4,y==119);

  for (i=0;i<nspr;i++) {
    fread(buf,1024,1,in);
    if (i<80 || i>=95) { /* Normal data */
      fprintf(out,"Uint3 vga%s[]={\n",gnames[i]);
      for (p=8;p>0;p>>=1)
        for (y=0;y<(heights[i]<<1);y++) {
          for (x=0;x<widths[i];x++) {
            cbuf[x]=0;
            for (b=0;b<8;b++)
              if ((buf[(y<<5)+(x<<3)+b]&p) && buf[(y<<5)+(x<<3)+b]!=0xff)
                cbuf[x]|=128>>b;
          }
          run(cbuf,widths[i],y==(heights[i]<<1)-1 && p==1);
        }
    }
    if (i>=87 && i<95) { /* Background data */
      fprintf(out,"Uint3 vga%s[]={\n",gnames[i]);
      for (p=8;p>0;p>>=1)
        for (y=0;y<8;y++) {
          for (x=0;x<4;x++) {
            cbuf[x]=0;
            for (b=0;b<8;b++)
              if (buf[(y<<5)+(x<<3)+b]&p)
                cbuf[x]|=128>>b;
          }
          cbuf[4]=0;
          for (b=0;b<8;b++)
            if (buf[(y<<5)+b+512]&p)
              cbuf[4]|=128>>b;
          run(cbuf,widths[i],y==7 && p==1);
        }
    }
    if (i==80 || i==81) { /* Tall masks */
      fprintf(out,"Uint3 vga%smask[]={\n",gnames[i]);
      for (y=0;y<32;y++) {
        for (x=0;x<widths[i];x++) {
          cbuf[x]=0;
          for (b=0;b<8;b++)
            if (buf[(y<<5)+(x<<3)+b]&15)
              cbuf[x]|=128>>b;
        }
        run(cbuf,widths[i],FALSE);
      }
      for (y=0;y<4;y++) {
        for (x=0;x<widths[i];x++) {
          cbuf[x]=0;
          for (b=0;b<8;b++)
            if (buf[(y<<5)+(x<<3)+b+16]&15)
              cbuf[x]|=128>>b;
        }
        run(cbuf,widths[i],y==3);
      }
    }
    if (i>=82 && i<86) { /* Wide masks */
      fprintf(out,"Uint3 vga%smask[]={\n",gnames[i]);
      for (y=0;y<(heights[i]<<1);y++) {
        for (x=0;x<4;x++) {
          cbuf[x]=0;
          for (b=0;b<8;b++)
            if (buf[(y<<5)+(x<<3)+b]&15)
              cbuf[x]|=128>>b;
        }
        for (x=0;x<2;x++) {
          cbuf[x+4]=0;
          for (b=0;b<8;b++)
            if (buf[((y+16)<<5)+(x<<3)+b]&15)
              cbuf[x+4]|=128>>b;
        }
        run(cbuf,widths[i],y==(heights[i]<<1)-1);
      }
    }
    if (i<80 || i>=95 || i==86) { /* Normal masks */
      fprintf(out,"Uint3 vga%smask[]={\n",gnames[i]);
      for (y=0;y<(heights[i]<<1);y++) {
        for (x=0;x<widths[i];x++) {
          cbuf[x]=0;
          for (b=0;b<8;b++)
            if (buf[(y<<5)+(x<<3)+b]&128)
              cbuf[x]|=128>>b;
        }
        run(cbuf,widths[i],y==(heights[i]<<1)-1);
      }
    }
  }

#endif

  fclose(in);

  fprintf(out,"Uint3 near *vgatable[]={\n"
"  vgazero480,   vgaediggermask,    /* 0 */\n"
"  vgardigger1,  vgardigger1mask,\n"
"  vgardigger2,  vgardigger2mask,\n"
"  vgardigger3,  vgardigger3mask,\n"
"  vgarxdigger1, vgarxdigger1mask,\n"
"  vgarxdigger2, vgarxdigger2mask,  /* 5 */\n"
"  vgarxdigger3, vgarxdigger3mask,\n"
"  vgaudigger1,  vgaudigger1mask,\n"
"  vgaudigger2,  vgaudigger2mask,\n"
"  vgaudigger3,  vgaudigger3mask,\n"
"  vgauxdigger1, vgauxdigger1mask,  /* 10 */\n"
"  vgauxdigger2, vgauxdigger2mask,\n"
"  vgauxdigger3, vgauxdigger3mask,\n"
"  vgaldigger1,  vgaldigger1mask,\n"
"  vgaldigger2,  vgaldigger2mask,\n"
"  vgaldigger3,  vgaldigger3mask,   /* 15 */\n"
"  vgalxdigger1, vgalxdigger1mask,\n"
"  vgalxdigger2, vgalxdigger2mask,\n"
"  vgalxdigger3, vgalxdigger3mask,\n"
"  vgaddigger1,  vgaddigger1mask,\n"
"  vgaddigger2,  vgaddigger2mask,   /* 20 */\n"
"  vgaddigger3,  vgaddigger3mask,\n"
"  vgadxdigger1, vgadxdigger1mask,\n"
"  vgadxdigger2, vgadxdigger2mask,\n"
"  vgadxdigger3, vgadxdigger3mask,\n"
"  vgadiggerd,   vgadiggerdmask,    /* 25 */\n"
"  vgagrave1,    vgagrave1mask,\n"
"  vgagrave2,    vgagrave2mask,\n"
"  vgagrave3,    vgagrave3mask,\n"
"  vgagrave4,    vgagrave4mask,\n"
"  vgagrave5,    vgagrave5mask,     /* 30 */\n"
"\n"
"  vgazero480,   vgaediggermask,\n"
"  vgarbigger1,  vgarbigger1mask,\n"
"  vgarbigger2,  vgarbigger2mask,\n"
"  vgarbigger3,  vgarbigger3mask,\n"
"  vgarxbigger1, vgarxbigger1mask,  /* 35 */\n"
"  vgarxbigger2, vgarxbigger2mask,\n"
"  vgarxbigger3, vgarxbigger3mask,\n"
"  vgaubigger1,  vgaubigger1mask,\n"
"  vgaubigger2,  vgaubigger2mask,\n"
"  vgaubigger3,  vgaubigger3mask,   /* 40 */\n"
"  vgauxbigger1, vgauxbigger1mask,\n"
"  vgauxbigger2, vgauxbigger2mask,\n"
"  vgauxbigger3, vgauxbigger3mask,\n"
"  vgalbigger1,  vgalbigger1mask,\n"
"  vgalbigger2,  vgalbigger2mask,   /* 45 */\n"
"  vgalbigger3,  vgalbigger3mask,\n"
"  vgalxbigger1, vgalxbigger1mask,\n"
"  vgalxbigger2, vgalxbigger2mask,\n"
"  vgalxbigger3, vgalxbigger3mask,\n"
"  vgadbigger1,  vgadbigger1mask,   /* 50 */\n"
"  vgadbigger2,  vgadbigger2mask,\n"
"  vgadbigger3,  vgadbigger3mask,\n"
"  vgadxbigger1, vgadxbigger1mask,\n"
"  vgadxbigger2, vgadxbigger2mask,\n"
"  vgadxbigger3, vgadxbigger3mask,  /* 55 */\n"
"  vgabiggerd,   vgabiggerdmask,\n"
"  vgagrave1,    vgagrave1mask,\n"
"  vgagrave2,    vgagrave2mask,\n"
"  vgagrave3,    vgagrave3mask,\n"
"  vgagrave4,    vgagrave4mask,     /* 60 */\n"
"  vgagrave5,    vgagrave5mask,\n"
"\n"
"  vgasbag,      vgasbagmask,\n"
"  vgarbag,      vgarbagmask,\n"
"  vgalbag,      vgalbagmask,\n"
"  vgafbag,      vgafbagmask,       /* 65 */\n"
"  vgagold1,     vgagold1mask,\n"
"  vgagold2,     vgagold2mask,\n"
"  vgagold3,     vgagold3mask,\n"
"\n"
"  vganobbin1,   vganobbin1mask,\n"
"  vganobbin2,   vganobbin2mask,    /* 70 */\n"
"  vganobbin3,   vganobbin3mask,\n"
"  vganobbind,   vganobbindmask,\n"
"  vgarhobbin1,  vgarhobbin1mask,\n"
"  vgarhobbin2,  vgarhobbin2mask,\n"
"  vgarhobbin3,  vgarhobbin3mask,   /* 75 */\n"
"  vgarhobbind,  vgarhobbindmask,\n"
"  vgalhobbin1,  vgalhobbin1mask,\n"
"  vgalhobbin2,  vgalhobbin2mask,\n"
"  vgalhobbin3,  vgalhobbin3mask,\n"
"  vgalhobbind,  vgalhobbindmask,   /* 80 */\n"
"\n"
"  vgabonus,     vgaediggermask,\n"
"\n"
"  vgafire1,     vgafire1mask,\n"
"  vgafire2,     vgafire2mask,\n"
"  vgafire3,     vgafire3mask,\n"
"  vgaexp1,      vgaexp1mask,       /* 85 */\n"
"  vgaexp2,      vgaexp2mask,\n"
"  vgaexp3,      vgaexp3mask,\n"
"\n"
"  vgafire1,     vgafire1mask,\n"
"  vgafire2,     vgafire2mask,\n"
"  vgafire3,     vgafire3mask,      /* 90 */\n"
"  vgaexp1,      vgaexp1mask,\n"
"  vgaexp2,      vgaexp2mask,\n"
"  vgaexp3,      vgaexp3mask,\n"
"\n"
"  vgaback1,     vgazero480,\n"
"  vgaback2,     vgazero480,        /* 95 */\n"
"  vgaback3,     vgazero480,\n"
"  vgaback4,     vgazero480,\n"
"  vgaback5,     vgazero480,\n"
"  vgaback6,     vgazero480,\n"
"  vgaback7,     vgazero480,        /* 100 */\n"
"  vgaback8,     vgazero480,\n"
"\n"
"  vgazero480,   vgarightblobmask,\n"
"  vgazero480,   vgatopblobmask,\n"
"  vgazero480,   vgaleftblobmask,\n"
"  vgazero480,   vgabottomblobmask, /* 105 */\n"
"  vgazero480,   vgasquareblobmask,\n"
"  vgazero480,   vgafurryblobmask,\n"
"\n"
"  vgaemerald,   vgaemeraldmask,\n"
"  vgazero480,   vgaemeraldmask,\n"
"\n"
"  vgaliferight, vgaliferightmask,  /* 110 */\n"
"  vgalifeleft,  vgalifeleftmask,\n"
"  vgazero480,   vgaelifemask,\n"
"  vgaleftlife,  vgaleftlifemask,\n"
"\n"
"  vgafire1b,    vgafire1bmask,\n"
"  vgafire2b,    vgafire2bmask,     /* 115 */\n"
"  vgafire3b,    vgafire3bmask,\n"
"  vgaexp1b,     vgaexp1bmask,\n"
"  vgaexp2b,     vgaexp2bmask,\n"
"  vgaexp3b,     vgaexp3bmask};\n");
}

int ccol[16]={0,0,1,0,2,0,3,0};

void mkgc(void)
{
  int i,x,y,b;
  char cbuf[6]={0,0,0,0,0,0};

  in=fileopen("cga.spr","rb");

  fprintf(out,"Uint3 cgazero60[]={\n");
  for (y=0;y<15;y++)
    run(cbuf,4,y==14);

  for (i=0;i<nspr;i++) {
    fread(buf,1024,1,in);
    if (i<80 || i>=87) {
      fprintf(out,"Uint3 cga%s[]={\n",gnames[i]);
      for (y=0;y<heights[i];y++) {
        for (x=0;x<widths[i];x++) {
          cbuf[x]=0;
          for (b=0;b<4;b++)
            cbuf[x]|=ccol[buf[(y<<5)+(x<<2)+b]&7]<<(6-(b<<1));
        }
        run(cbuf,widths[i],y==heights[i]-1);
      }
    }
    if (i<87 || i>=95) {
      fprintf(out,"Uint3 cga%smask[]={\n",gnames[i]);
      for (y=0;y<heights[i];y++) {
        for (x=0;x<widths[i];x++) {
          cbuf[x]=0;
          for (b=0;b<4;b++)
            if ((buf[(y<<5)+(x<<2)+b]&7)==7)
              cbuf[x]|=0xc0>>(b<<1);
        }
        run(cbuf,widths[i],y==heights[i]-1);
      }
    }
  }

  fclose(in);

  fprintf(out,"Uint3 near *cgatable[]={\n"
"  cgazero60,    cgaediggermask,    /* 0 */\n"
"  cgardigger1,  cgardigger1mask,\n"
"  cgardigger2,  cgardigger2mask,\n"
"  cgardigger3,  cgardigger3mask,\n"
"  cgarxdigger1, cgarxdigger1mask,\n"
"  cgarxdigger2, cgarxdigger2mask,  /* 5 */\n"
"  cgarxdigger3, cgarxdigger3mask,\n"
"  cgaudigger1,  cgaudigger1mask,\n"
"  cgaudigger2,  cgaudigger2mask,\n"
"  cgaudigger3,  cgaudigger3mask,\n"
"  cgauxdigger1, cgauxdigger1mask,  /* 10 */\n"
"  cgauxdigger2, cgauxdigger2mask,\n"
"  cgauxdigger3, cgauxdigger3mask,\n"
"  cgaldigger1,  cgaldigger1mask,\n"
"  cgaldigger2,  cgaldigger2mask,\n"
"  cgaldigger3,  cgaldigger3mask,   /* 15 */\n"
"  cgalxdigger1, cgalxdigger1mask,\n"
"  cgalxdigger2, cgalxdigger2mask,\n"
"  cgalxdigger3, cgalxdigger3mask,\n"
"  cgaddigger1,  cgaddigger1mask,\n"
"  cgaddigger2,  cgaddigger2mask,   /* 20 */\n"
"  cgaddigger3,  cgaddigger3mask,\n"
"  cgadxdigger1, cgadxdigger1mask,\n"
"  cgadxdigger2, cgadxdigger2mask,\n"
"  cgadxdigger3, cgadxdigger3mask,\n"
"  cgadiggerd,   cgadiggerdmask,    /* 25 */\n"
"  cgagrave1,    cgagrave1mask,\n"
"  cgagrave2,    cgagrave2mask,\n"
"  cgagrave3,    cgagrave3mask,\n"
"  cgagrave4,    cgagrave4mask,\n"
"  cgagrave5,    cgagrave5mask,     /* 30 */\n"
"\n"
"  cgazero60,    cgaediggermask,\n"
"  cgarbigger1,  cgarbigger1mask,\n"
"  cgarbigger2,  cgarbigger2mask,\n"
"  cgarbigger3,  cgarbigger3mask,\n"
"  cgarxbigger1, cgarxbigger1mask,  /* 35 */\n"
"  cgarxbigger2, cgarxbigger2mask,\n"
"  cgarxbigger3, cgarxbigger3mask,\n"
"  cgaubigger1,  cgaubigger1mask,\n"
"  cgaubigger2,  cgaubigger2mask,\n"
"  cgaubigger3,  cgaubigger3mask,   /* 40 */\n"
"  cgauxbigger1, cgauxbigger1mask,\n"
"  cgauxbigger2, cgauxbigger2mask,\n"
"  cgauxbigger3, cgauxbigger3mask,\n"
"  cgalbigger1,  cgalbigger1mask,\n"
"  cgalbigger2,  cgalbigger2mask,   /* 45 */\n"
"  cgalbigger3,  cgalbigger3mask,\n"
"  cgalxbigger1, cgalxbigger1mask,\n"
"  cgalxbigger2, cgalxbigger2mask,\n"
"  cgalxbigger3, cgalxbigger3mask,\n"
"  cgadbigger1,  cgadbigger1mask,   /* 50 */\n"
"  cgadbigger2,  cgadbigger2mask,\n"
"  cgadbigger3,  cgadbigger3mask,\n"
"  cgadxbigger1, cgadxbigger1mask,\n"
"  cgadxbigger2, cgadxbigger2mask,\n"
"  cgadxbigger3, cgadxbigger3mask,  /* 55 */\n"
"  cgabiggerd,   cgabiggerdmask,\n"
"  cgagrave1,    cgagrave1mask,\n"
"  cgagrave2,    cgagrave2mask,\n"
"  cgagrave3,    cgagrave3mask,\n"
"  cgagrave4,    cgagrave4mask,     /* 60 */\n"
"  cgagrave5,    cgagrave5mask,\n"
"\n"
"  cgasbag,      cgasbagmask,\n"
"  cgarbag,      cgarbagmask,\n"
"  cgalbag,      cgalbagmask,\n"
"  cgafbag,      cgafbagmask,       /* 65 */\n"
"  cgagold1,     cgagold1mask,\n"
"  cgagold2,     cgagold2mask,\n"
"  cgagold3,     cgagold3mask,\n"
"\n"
"  cganobbin1,   cganobbin1mask,\n"
"  cganobbin2,   cganobbin2mask,    /* 70 */\n"
"  cganobbin3,   cganobbin3mask,\n"
"  cganobbind,   cganobbindmask,\n"
"  cgarhobbin1,  cgarhobbin1mask,\n"
"  cgarhobbin2,  cgarhobbin2mask,\n"
"  cgarhobbin3,  cgarhobbin3mask,   /* 75 */\n"
"  cgarhobbind,  cgarhobbindmask,\n"
"  cgalhobbin1,  cgalhobbin1mask,\n"
"  cgalhobbin2,  cgalhobbin2mask,\n"
"  cgalhobbin3,  cgalhobbin3mask,\n"
"  cgalhobbind,  cgalhobbindmask,   /* 80 */\n"
"\n"
"  cgabonus,     cgaediggermask,\n"
"\n"
"  cgafire1,     cgafire1mask,\n"
"  cgafire2,     cgafire2mask,\n"
"  cgafire3,     cgafire3mask,\n"
"  cgaexp1,      cgaexp1mask,       /* 85 */\n"
"  cgaexp2,      cgaexp2mask,\n"
"  cgaexp3,      cgaexp3mask,\n"
"\n"
"  cgafire1,     cgafire1mask,\n"
"  cgafire2,     cgafire2mask,\n"
"  cgafire3,     cgafire3mask,      /* 90 */\n"
"  cgaexp1,      cgaexp1mask,\n"
"  cgaexp2,      cgaexp2mask,\n"
"  cgaexp3,      cgaexp3mask,\n"
"\n"
"  cgaback1,     cgazero60,\n"
"  cgaback2,     cgazero60,         /* 95 */\n"
"  cgaback3,     cgazero60,\n"
"  cgaback4,     cgazero60,\n"
"  cgaback5,     cgazero60,\n"
"  cgaback6,     cgazero60,\n"
"  cgaback7,     cgazero60,         /* 100 */\n"
"  cgaback8,     cgazero60,\n"
"\n"
"  cgazero60,    cgarightblobmask,\n"
"  cgazero60,    cgatopblobmask,\n"
"  cgazero60,    cgaleftblobmask,\n"
"  cgazero60,    cgabottomblobmask, /* 105 */\n"
"  cgazero60,    cgasquareblobmask,\n"
"  cgazero60,    cgafurryblobmask,\n"
"\n"
"  cgaemerald,   cgaemeraldmask,\n"
"  cgazero60,    cgaemeraldmask,\n"
"\n"
"  cgaliferight, cgaliferightmask,  /* 110 */\n"
"  cgalifeleft,  cgalifeleftmask,\n"
"  cgazero60,    cgaelifemask,\n"
"  cgaleftlife,  cgaleftlifemask,\n"
"\n"
"  cgafire1b,    cgafire1bmask,\n"
"  cgafire2b,    cgafire2bmask,     /* 115 */\n"
"  cgafire3b,    cgafire3bmask,\n"
"  cgaexp1b,     cgaexp1bmask,\n"
"  cgaexp2b,     cgaexp2bmask,\n"
"  cgaexp3b,     cgaexp3bmask};\n");
}

/* As new glyphs are added enter them into clist in the order in which they
   appear in the file. */

char clist[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.:_ ";

char *symbols[]={
  "symspace","symexclam","symdquote","symhash","symdollar","sympercent",
  "symamp","symcquote","symoparen","symcparen","symaster","symplus","symcomma",
  "symdash","symdot","symslash","num0","num1","num2","num3","num4","num5",
  "num6","num7","num8","num9","symcolon","symsemi","symless","symequal",
  "symgreater","symquestion","symat","letA","letB","letC","letD","letE","letF",
  "letG","letH","letI","letJ","letK","letL","letM","letN","letO","letP","letQ",
  "letR","letS","letT","letU","letV","letW","letX","letY","letZ","symobracket",
  "symbackslash","symcbracket","symhat","symline","symoquote","leta","letb",
  "letc","letd","lete","letf","letg","leth","leti","letj","letk","letl","letm",
  "letn","leto","letp","letq","letr","lets","lett","letu","letv","letw","letx",
  "lety","letz","symocurly","symbar","symccurly","symtilde"};

void mkga(void)
{
#ifdef _WINDOWS
  char cbuf[12];
  int i,x,y,b,p,l;
  fprintf(out,
    "char copyright[]=\"Portions Copyright(c) 1983 Windmill Software Inc.\";\n\n");

  in=fileopen("cgatext.spr","rb");

  for (i=0;clist[i]!=0;i++) {
    fread(buf,1024,1,in);
    fprintf(out,"Uint3 cga%s[]={\n",symbols[clist[i]-32]);
    for (y=0;y<12;y++) {
      for (x=0;x<12;x++) {
        if (x&0x1)
          cbuf[x>>1]|=buf[(y<<5)+x];
        else
          cbuf[x>>1]=buf[(y<<5)+x]<<4;

        if (x==11)
          run(cbuf,6,y==11);

      }
    }
  }
  fclose(in);

  in=fileopen("vgatext.spr","rb");
  for (i=0;clist[i]!=0;i++) {
    fread(buf,1024,1,in);
    fprintf(out,"Uint3 vga%s[]={\n",symbols[clist[i]-32]);
    for (y=0;y<24;y++) {
      for (x=0;x<24;x++) {
        if (x&0x1)
          cbuf[x>>1]|=buf[(y<<5)+x];
        else
          cbuf[x>>1]=buf[(y<<5)+x]<<4;

        if (x==23)
          run(cbuf,12,y==23 && x==23);
      }
    }
  }
#else
  char cbuf[3];
  int i,x,y,b,p,l;
  fprintf(out,
    "char copyright[]=\"Portions Copyright(c) 1983 Windmill Software Inc.\";\n\n"
    "Sint textoffdat[16]={\n"
    "   72,   0, -72,-72,\n"
    "  144,   0,-288,144,\n"
    "  144,-216, 144,-72,\n"
    "  144,-216, -72,144};\n\n");

  in=fileopen("cgatext.spr","rb");

  for (i=0;clist[i]!=0;i++) {
    fread(buf,1024,1,in);
    fprintf(out,"Uint3 cga%s[]={\n",symbols[clist[i]-32]);
    for (y=0;y<12;y++) {
      for (x=0;x<3;x++) {
        cbuf[x]=0;
        for (b=0;b<4;b++)
          if (buf[(y<<5)+(x<<2)+b]!=0)
            cbuf[x]|=0xc0>>(b<<1);
      }
      run(cbuf,3,y==11);
    }
  }
  fclose(in);
  in=fileopen("vgatext.spr","rb");
  for (i=0;clist[i]!=0;i++) {
    fread(buf,1024,1,in);
    fprintf(out,"Uint3 vga%s[]={\n",symbols[clist[i]-32]);
    for (p=8;p>0;p>>=1)
      for (y=0;y<24;y++) {
        for (x=0;x<3;x++) {
          cbuf[x]=0;
          for (b=0;b<8;b++)
            if ((buf[(y<<5)+(x<<3)+b]&p)!=0)
              cbuf[x]|=128>>b;
        }
        run(cbuf,3,y==23 && p==1);
      }
  }
#endif

  fclose(in);
  fprintf(out,"Uint3 near *ascii2cga[0x5f]={\n");
  l=0;
  p=32;
  for (i=32;p<127;i++) {
    for (x=0;clist[x]!=0;x++)
      if (toupper(clist[x])==toupper(i)) {
        l+=strlen(symbols[clist[x]-32])+5;
        break;
      }
    if (clist[x]==0)
      l+=3;
    if (l>70 || i==127) {
      fprintf(out,"  ");
      l=0;
      for (b=p;b<i;b++) {
        for (x=0;clist[x]!=0;x++)
          if (toupper(clist[x])==toupper(b)) {
            fprintf(out,"cga%s",symbols[clist[x]-32]);
            l+=strlen(symbols[clist[x]-32])+5;
            if (b<126)
              fprintf(out,",");
            break;
          }
        if (clist[x]==0) {
          fprintf(out,"0");
          l+=3;
          if (b<126)
            fprintf(out,",");
          else {
            fprintf(out,"};");
            l++;
          }
        }
      }
      for (b=0;b<71-l;b++)
        fprintf(out," ");
      fprintf(out,"/* ");
      for (b=p;b<i;b++)
        fprintf(out,"%c",b);
      fprintf(out," */\n");
      l=0;
      p=i;
      i--;
    }
  }

  fprintf(out,"Uint3 near *ascii2vga[0x5f]={\n");
  l=0;
  p=32;
  for (i=32;p<127;i++) {
    for (x=0;clist[x]!=0;x++)
      if (toupper(clist[x])==toupper(i)) {
        l+=strlen(symbols[clist[x]-32])+5;
        break;
      }
    if (clist[x]==0)
      l+=3;
    if (l>70 || i==127) {
      fprintf(out,"  ");
      l=0;
      for (b=p;b<i;b++) {
        for (x=0;clist[x]!=0;x++)
          if (toupper(clist[x])==toupper(b)) {
            fprintf(out,"vga%s",symbols[clist[x]-32]);
            l+=strlen(symbols[clist[x]-32])+5;
            if (b<126)
              fprintf(out,",");
            break;
          }
        if (clist[x]==0) {
          fprintf(out,"0");
          l+=3;
          if (b<126)
            fprintf(out,",");
          else {
            fprintf(out,"};");
            l++;
          }
        }
      }
      for (b=0;b<71-l;b++)
        fprintf(out," ");
      fprintf(out,"/* ");
      for (b=p;b<i;b++)
        fprintf(out,"%c",b);
      fprintf(out," */\n");
      l=0;
      p=i;
      i--;
    }
  }
}

int main(int argc,char *argv[])
{
  int n=-1;
  if (argc<=1) {
    fprintf(stderr,"Syntax: MKG <name>\nWhere <name> is vgagrafx.c, cgagrafx.c or alpha.c\n");
    return 1;
  }
  if (stricmp(argv[1],"vgagrafx.c")==0)
    n=0;
  if (stricmp(argv[1],"cgagrafx.c")==0)
    n=1;
  if (stricmp(argv[1],"alpha.c")==0)
    n=2;
  out=fileopen(argv[1],"wt");
  fprintf(out,"#include \"def.h\"\n\n");
  if (n==0)
    mkgv();
  else
    if (n==1)
      mkgc();
    else
      if (n==2)
        mkga();
      else {
        fprintf(stderr,"MKG: Don't know how to make %s\n",argv[1]);
        return 1;
      }
  fclose(out);
  return 0;
}
