/* Digger Remastered
   Copyright (c) Andrew Jenner 1998-2004 */

/* Generic sound device header */

typedef unsigned char samp;      /* 8 bit unsigned samples */
#define MIN_SAMP 0x0
#define MAX_SAMP 0xff

extern samp *buffer;
extern Uint4 firsts,last,size;

bool setsounddevice(int base,int irq,int dma,Uint4 samprate,Uint4 bufsize);
bool initsounddevice(void);
void killsounddevice(void);
