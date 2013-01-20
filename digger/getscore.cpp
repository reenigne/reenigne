/* Digger Remastered
   Copyright (c) Andrew Jenner 1998-2004 */

/* GETSCORE.C - retrieve original Digger score table
   Compile with Turbo C 2.01 (http://community.borland.com/museum)
*/

#include <dos.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

void main(int argc,char *argv[])
{
  int dr,r;
  char buffer[512];
  FILE *out;
  if (argc<2) {
    printf("Syntax: GETSCORE <floppy drive letter>\n");
    exit(1);
  }
  dr=toupper(argv[1][0])-'A';
  _DL=dr;
  _BX=(int)buffer;
  _ES=_DS;
  _DH=0;      /* Head 0 */
  _AX=0x201;  /* Interrupt 0x13 function 2 - Read sectors, AL=1 sector */
  _CX=0x2707; /* Track 0x27, sector 7 */
  geninterrupt(0x13);
  r=_AL;
  if (r!=0) {
    printf("Error: Could not read sector (0x%02x)\n",r);
    exit(1);
  }
  out=fopen("digger.sco","wb");
  if (out==NULL) {
    printf("Error: Could not open output file: %s\n",sys_errlist[errno]);
    exit(1);
  }
  fwrite(buffer,512,1,out);
  fclose(out);
}
