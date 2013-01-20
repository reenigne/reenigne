/* Digger Remastered
   Copyright (c) Andrew Jenner 1998-2004 */

#include <stdlib.h>
#include <stdio.h>
#include "def.h"
#include "device.h"
#include "hardware.h"
#include "digger.h"
#include "sound.h"
#include "main.h"

/* The function which empties the circular buffer should get samples from
   buffer[firsts] and then do firsts=(firsts+1)&(size-1); This function is
   responsible for incrementing first samprate times per second (on average)
   (if it's a little out, the sound will simply run too fast or too slow). It
   must not take more than (last-firsts-1)&(size-1) samples at once, or the
   sound will break up.

   If DMA is used, doubling the buffer so the data is always continguous, and
   giving half of the buffer at once to the DMA driver may be a good idea. */

samp *buffer=NULL;
Uint4 firsts,last,size;           /* data available to output device */

int rate;
Uint4 t0rate,t2rate,t2new,t0v=0,t2v=0;
Sint4 i8pulse=0;
bool t2f=FALSE,t2sw,i8flag=FALSE;
samp lut[257];
Uint4 pwlut[51];

extern Sint4 spkrmode,pulsewidth;

samp getsample1(void);

samp (*getsample)(void);

/* Initialise circular buffer and PC speaker emulator

   bufsize = buffer size in samples
   samprate = play rate in Hz

   samprate is directly proportional to the sound quality. This should be the
   highest value the hardware can support without slowing down the program too
   much. Ensure 0x1234<samprate<=0x1234dd and that samprate is a factor of
   0x1234dd (or you won't get the rate you want). For example, a value of
   44100 equates to about 44192Hz (a .2% difference - negligable unless you're
   trying to harmonize with a computer running at a different rate, or another
   musical instrument...)

   The lag time is bufsize/samprate seconds. This should be the smallest value
   which does not make the sound break up. There may also be DMA considerations
   to take into account. bufsize should also be a power of 2.
*/

void soundinitglob(int port,int irq,int dma,Uint4 bufsize,Uint4 samprate)
{
  int i;
  if (samprate<5000)
    samprate=5000;
  setsounddevice(port,irq,dma,samprate,bufsize);
#ifndef _WINDOWS
  buffer=malloc((bufsize<<1)*sizeof(samp));
  if (buffer==NULL) {
    finish();
    printf("Cannot allocate memory for sample buffer\n");
    exit(1);
  }
#endif
  rate=(int)(0x1234ddul/(Uint5)samprate);
  firsts=0;
  last=1;
  size=bufsize<<1;
  t2sw=FALSE;     /* As it should be left */
  for (i=0;i<=rate;i++)
    lut[i]=(samp)(MIN_SAMP+(i*(MAX_SAMP-MIN_SAMP))/rate);
  for (i=1;i<=50;i++)
    pwlut[i]=(16+i*18)>>2; /* Counted timer ticks in original */
}

void s1setupsound(void)
{
  inittimer();
  curtime=0;
  startint8();
#ifndef _WINDOWS
  buffer[firsts]=getsample();
#endif
  fillbuffer();
  initsounddevice();
}

void s1killsound(void)
{
  setsoundt2();
  timer2(40);
  stopint8();
  killsounddevice();
#ifdef _WINDOWS
  if (buffer!=NULL)
    free(buffer);
#endif
}

/* This function is called regularly by the Digger engine to keep the circular
   buffer filled. */

#ifndef _WINDOWS
void s1fillbuffer(void)
{
  while (firsts!=last) {
    buffer[last]=getsample();
    last=(last+1)&(size-1);
  }
}
#endif

/* WARNING: Read only code ahead. Unless you're seriously into how the PC
   speaker and Digger's original low-level sound routines work, you shouldn't
   try to mess with, or even understand, the following. I don't understand most
   of it myself, and I wrote it. */

void s1settimer2(Uint4 t2)
{
  if (t2==40)
    t2=rate;   /* Otherwise aliasing would cause noise artifacts */
  t2>>=1;
  t2v=t2new=t2;
}

void s1soundoff(void)
{
  t2sw=FALSE;
}

void s1setspkrt2(void)
{
  t2sw=true;
}

void s1settimer0(Uint4 t0)
{
  t0v=t0rate=t0;
}

void s1timer0(Uint4 t0)
{
  t0rate=t0;
}

void s1timer2(Uint4 t2)
{
  if (t2==40)
    t2=rate;    /* Otherwise aliasing would cause noise artifacts */
  t2>>=1;
  t2new=t2rate=t2;
  t2v=t2rate;
}

bool addcarry(Uint4 *dest,Uint4 add)
{
  *dest+=add;
  if (*dest<add)
    return TRUE;
  return FALSE;
}

bool subcarry(Uint4 *dest,Uint4 sub)
{
  *dest-=sub;
  if (*dest>=(Uint4)(-sub))
    return TRUE;
  return FALSE;
}

/* This function is the workhorse.
   It emulates the functionality of:
    * the 8253 Programmable Interval Timer
    * the PC speaker hardware
    * the IRQ0 timer interrupt which Digger reprograms
   It averages the speaker values over the entire time interval to get the
   sample.
   Despite its complexity, it runs pretty fast, since most of the time, it
   doesn't actually do very much, and when it does stuff, it uses look-up
   tables.
   There are probably fencepost errors but I challenge anyone to detect these
   audibly.
   It would be easier to just calculate each bit separately and add them up,
   but there are 1,193,181 bits to add up per second, so you'd need a fast PC.
   This may be a little more complicated, but its much faster.
*/

samp getsample1(void)
{
  bool f=FALSE,t2sw0;
  Uint4 spkrt2,noi8,complicate=0,not2;

  if (subcarry(&t2v,rate)) {
    not2=t2v+rate; /* Amount of time that went by before change */
    if (t2f) {
      spkrt2=-t2v; /* MIN_SAMPs at beginning */
      t2rate=t2new;
      if (t2rate==(rate>>1))
        t2v=t2rate;
    }
    else                  /* MIN_SAMPs at end */
      spkrt2=t2v+rate;
    t2v+=t2rate;
    if (t2rate==(rate>>1))
      t2v=t2rate;
    else
      t2f=!t2f;
    complicate|=1;
  }

  if (subcarry(&t0v,rate)) { /* Effectively using mode 2 here */
    i8flag=true;
    noi8=t0v+rate; /* Amount of time that went by before interrupt */
    t0v+=t0rate;
    complicate|=2;
  }

  t2sw0=t2sw;

  if (i8flag && i8pulse<=0) {
    f=true;
    if (spkrmode!=0)
      if (spkrmode!=1)
        t2sw=!t2sw;
      else {
        i8pulse=pwlut[pulsewidth];
        t2sw=true;
        f=FALSE;
      }
  }

  if (i8pulse>0) {
    complicate|=4;
    i8pulse-=rate;
    if (i8pulse<=0) {
      complicate|=8;
      t2sw=FALSE;
      i8flag=true;
      f=TRUE;
    }
  }

  if (f) {
    if (addcarry(&timercount,timerrate)) {
      soundint(); /* Update music and sound effects 72.8 Hz */
      timercount-=0x4000;
    }
    i8flag=FALSE;
  }

  if (!(complicate&1) && t2f)
    return MIN_SAMP;

  /* 12 unique cases, no break statements!
     No more than about 6 of these lines are executed on any single call. */

  switch (complicate) {
    case 2: /* Int8 happened */
      if (t2sw!=t2sw0) {
        if (t2sw) /* <==> !t2sw0 */
          return lut[rate-noi8];
        return lut[noi8];
      }
      /* Fall through */
    case 0: /* Nothing happened! */
      if (!t2sw)
        return MIN_SAMP;
      /* Fall through */
    case 4: /* Int8 is pulsing => t2sw */
      return MAX_SAMP;
    case 1: /* The t2 wave changed */
      if (!t2sw)
        return MIN_SAMP;
      /* Fall through */
    case 5: /* The t2 wave changed and Int8 is pulsing => t2sw */
      return lut[spkrt2];
    case 3: /* Int8 happened and t2 wave changed */
      if (!t2sw0 && !t2sw)
        return MIN_SAMP;    /* both parts are off */
      if (t2sw0 && t2sw)
        return lut[spkrt2]; /* both parts are on */
      if (not2<noi8)  /* t2 happened first */
        if (t2sw0) /* "on" part is before i8 */
          if (t2f)
            return lut[spkrt2]; /* MIN_SAMPs at end */
          else
            return lut[spkrt2-(rate-noi8)]; /* MIN_SAMPs at beginning */
        else      /* "on" part is after i8 => constant */
          if (t2f)
            return MIN_SAMP; /* MIN_SAMPs at end */
          else
            return lut[rate-noi8]; /* MIN_SAMPs at beginning */
      else /* i8 happened first */
        if (t2sw0) /* "on" part is before i8 => constant */
          if (t2f)
            return MIN_SAMP; /* MIN_SAMPs at beginning */
          else
            return lut[noi8]; /* MIN_SAMPs at end */
        else       /* "on" part is after i8 */
          if (t2f)
            return lut[spkrt2]; /* MIN_SAMPs at beginning */
          else
            return lut[spkrt2-noi8]; /* MIN_SAMPs at end */
    case 6: /* The Int8 pulse started */
      if (t2sw0)
        return MAX_SAMP;
      return lut[rate-noi8];
    case 7: /* The Int8 pulse started and the t2 wave changed */
      if (t2sw0)
        return lut[spkrt2];
      if (not2<noi8)  /* t2 happened first */
        if (t2f)
          return MIN_SAMP; /* MIN_SAMPs at end */
        else
          return lut[rate-noi8]; /* MIN_SAMPs at beginning */
      else /* i8 happened first */
        if (t2f)
          return lut[spkrt2]; /* MIN_SAMPs at beginning */
        else
          return lut[spkrt2-noi8]; /* MIN_SAMPs at end */
    case 12: /* The Int8 pulse stopped */
      if (t2sw)
        return MAX_SAMP;
      return lut[i8pulse+rate];
    case 13: /* The Int8 pulse stopped and the t2 wave changed */
      if (t2sw)
        return lut[spkrt2];
      if (not2<i8pulse+rate) /* t2 happened first */
        if (t2f)
          return lut[spkrt2+i8pulse]; /* MIN_SAMPs at beginning */
        else
          return lut[spkrt2];         /* MIN_SAMPs at end */
      else /* i8pulse ended first */
        if (t2f)
          return MIN_SAMP; /* MIN_SAMPs at beginning */
        else
          return lut[i8pulse+rate];
    case 14: /* The Int8 pulse started and stopped in the same sample */
      if (t2sw0)
        if (t2sw)
          return MAX_SAMP;
        else
          return lut[noi8+i8pulse+rate];
      else
        if (t2sw)
          return lut[rate-noi8];
        else
          return lut[i8pulse+rate];
    case 15: /* Everything happened at once */
      if (not2<noi8) /* First subcase: t2 happens before pulse */
        if (t2f) /* MIN_SAMPs at beginning */
          if (t2sw0)
            if (t2sw)
              return lut[spkrt2];
            else
              return lut[spkrt2+noi8+i8pulse];
          else
            if (t2sw)
              return lut[rate-noi8];
            else
              return lut[i8pulse+rate];
        else /* MIN_SAMPs at end */
          if (t2sw0) /* No need to test t2sw */
            return lut[spkrt2];
          else
            return MIN_SAMP;
      else
        if (not2<rate+noi8+i8pulse) /* Subcase 2: t2 happens during pulse */
          if (t2f) /* MIN_SAMPs at beginning */
            if (t2sw) /* No need to test t2sw0 */
              return lut[spkrt2];
            else
              return lut[spkrt2+noi8+i8pulse];
          else /* MIN_SAMPs at end */
            if (t2sw0) /* No need to test t2sw */
              return lut[spkrt2];
            else
              return lut[spkrt2-noi8];
        else /* Third subcase: t2 happens after pulse */
          if (t2f) /* MIN_SAMPs at beginning */
            if (t2sw) /* No need to test t2sw0 */
              return lut[spkrt2];
            else
              return MIN_SAMP;
          else /* MIN_SAMPs at end */
            if (t2sw0)
              if (t2sw)
                return lut[spkrt2];
              else
                return lut[noi8+i8pulse+rate];
            else
              if (t2sw)
                return lut[spkrt2-noi8];
              else
                return lut[i8pulse+rate];
  }
  return MIN_SAMP; /* This should never happen */
}

#if 0

/* Code for test beep. The sum of a 1050Hz tone and a 780Hz tone (if played at
   44,100Hz.) Radio waves modulated at these frequencies have been detected
   eminating from a neutron star. So this sound really is the music of the
   spheres. See http://www.np.washington.edu/AV/altvw92.html for more
   information.

   It doesn't work. There is a bug in the soundblaster routines which I didn't
   notice from listening to the Digger soundtrack (I thought it didn't sound
   very good for other reasons.) I suspect there isn't enough time to perform
   the copy from the secondary buffer into the primary one.
*/

samp sine[512]={
64,64,65,66,67,67,68,69,70,70,71,72,73,74,74,75,76,77,77,78,79,80,80,81,82,83,
83,84,85,85,86,87,88,88,89,90,90,91,92,93,93,94,95,95,96,97,97,98,99,99,100,
100,101,102,102,103,103,104,105,105,106,106,107,107,108,109,109,110,110,111,
111,112,112,113,113,114,114,115,115,115,116,116,117,117,118,118,118,119,119,
119,120,120,120,121,121,121,122,122,122,123,123,123,123,124,124,124,124,124,
125,125,125,125,125,125,126,126,126,126,126,126,126,126,126,126,126,126,126,
126,127,126,126,126,126,126,126,126,126,126,126,126,126,126,126,125,125,125,
125,125,125,124,124,124,124,124,123,123,123,123,122,122,122,121,121,121,120,
120,120,119,119,119,118,118,118,117,117,116,116,115,115,115,114,114,113,113,
112,112,111,111,110,110,109,109,108,107,107,106,106,105,105,104,103,103,102,
102,101,100,100,99,99,98,97,97,96,95,95,94,93,93,92,91,90,90,89,88,88,87,86,85,
85,84,83,83,82,81,80,80,79,78,77,77,76,75,74,74,73,72,71,70,70,69,68,67,67,66,
65,64,64,64,63,62,61,61,60,59,58,58,57,56,55,54,54,53,52,51,51,50,49,48,48,47,
46,45,45,44,43,43,42,41,40,40,39,38,38,37,36,35,35,34,33,33,32,31,31,30,29,29,
28,28,27,26,26,25,25,24,23,23,22,22,21,21,20,19,19,18,18,17,17,16,16,15,15,14,
14,13,13,13,12,12,11,11,10,10,10,9,9,9,8,8,8,7,7,7,6,6,6,5,5,5,5,4,4,4,4,4,3,3,
3,3,3,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,
4,4,4,4,4,5,5,5,5,6,6,6,7,7,7,8,8,8,9,9,9,10,10,10,11,11,12,12,13,13,13,14,14,
15,15,16,16,17,17,18,18,19,19,20,21,21,22,22,23,23,24,25,25,26,26,27,28,28,29,
29,30,31,31,32,33,33,34,35,35,36,37,38,38,39,40,40,41,42,43,43,44,45,45,46,47,
48,48,49,50,51,51,52,53,54,54,55,56,57,58,58,59,60,61,61,62,63,64};

long int x1=0,x2=0;

samp getsample1(void)
{
  x1+=210; if (x1>=8820) x1-=8820;
  x2+=156; if (x2>=8820) x2-=8820;
  return sine[(Sint4)((x1<<7)/2205L)]+sine[(Sint4)((x2<<7)/2205L)];
/*  x1+=441;
  if (x1>=44100L) x1-=44100L;
  return sine[(Sint4)((x1<<9)/44100L)]; */
}
#endif
