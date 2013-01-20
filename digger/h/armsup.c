/* Digger Remastered
   Copyright (c) Andrew Jenner 1998-2004 */

#include <math.h>
#include <ctype.h>
#include "def.h"

int inkey(void);
void vgaputim2(int a,int ch,int w,int h);
void cgaputim2(int a,int ch,int w,int h);

int convertfunction(int in)
{
  return (int) ((log(1193181.0/(440.0*(float)in))/log(2.0))*0x1000)+0x4000;
}

void vgaputim(Sint4 x,Sint4 y,Sint4 ch,Sint4 w,Sint4 h)
{
  vgaputim2(y*640+x,ch,w,h);
}

void cgaputim(Sint4 x,Sint4 y,Sint4 ch,Sint4 w,Sint4 h)
{
  cgaputim2(y*640+x,ch,w,h);
}

int lastk=0;

bool kbhit(void)
{
  int k;
  if (lastk!=0)
    return TRUE;
  k=inkey();
  if (k==255)
    return FALSE;
  lastk=k;
  return TRUE;
}

int getkey(void)
{
  int k;
  if (lastk!=0) {
    k=lastk;
    lastk=0;
    return k;
  }
  do
    k=inkey();
  while (k==255);
  return k;
}

void strupr(char* in)
{
  for (; *in=toupper(*in); in++);
}
