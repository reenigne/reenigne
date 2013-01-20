/* Digger Remastered
   Copyright (c) Andrew Jenner 1998-2004 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "def.h"
#ifdef WIN32
#include "win_dig.h"
#endif

#define NEWL "\r\n"

/* Get a line from a buffer. This should be compatible with all 3 text file
   formats: DOS, Unix and Mac. s is the string read from buffer, pointer to
   space after buffer is returned. */
char *sgets(char *buffer,char *s)
{
  int i;
  for (i=0;buffer[i]!=10 && buffer[i]!=13 && buffer[i]!=0;i++)
    s[i]=buffer[i];
  s[i]=0;
  if (buffer[i]==13)
    i++;
  if (buffer[i]==10)
    i++;
  return buffer+i;
}

/* These are re-implementations of the Windows version of INI filing. */

void WriteINIString(char *section,char *key,WCHAR *value,char *filename)
{
  FILE *fp;
  char *buffer,*p,*p0,s1[80],s2[80],s3[80];
  int tl;
  fp=fopen(filename,"rb");
  if (fp==NULL) {
    fp=fopen(filename,"wb");
    if (fp==NULL)
      return;
    fprintf(fp,"[%s]" NEWL,section);
    fprintf(fp,"%s=%s" NEWL NEWL,key,value);
    fclose(fp);
    return;
  }
  fseek(fp,0,2);
  tl=(int)ftell(fp);
  fseek(fp,0,0);
  buffer=(char *)malloc(tl+1);
  if (buffer==NULL) {
    fclose(fp);
    return;
  }
  fread(buffer,tl,1,fp);
  buffer[tl]=0;
  fclose(fp);
  strcpy(s2,"[");
  strcat(s2,section);
  strcat(s2,"]");
  strcpy(s3,key);
  strcat(s3,"=");
  p=buffer;
  do {
    p=sgets(p,s1);
    if (_stricmp(s1,s2)==0) {
      do {
        p0=p;
        p=sgets(p,s1);
        if (_strnicmp(s1,s3,strlen(s3))==0) {
          fp=fopen(filename,"wb");
          if (fp==NULL) {
            free(buffer);
            return;
          }
          fwrite(buffer,p0-buffer,1,fp);
          fprintf(fp,"%s=%s" NEWL,key,value);
          fwrite(p,tl-(p-buffer),1,fp);
          fclose(fp);
          free(buffer);
          return;
        }
      } while (s1[0]!=0);
      fp=fopen(filename,"wb");
      if (fp==NULL) {
        free(buffer);
        return;
      }
      fwrite(buffer,p0-buffer,1,fp);
      fprintf(fp,"%s=%s" NEWL,key,value);
      fwrite(p0,tl-(p0-buffer),1,fp);
      fclose(fp);
      free(buffer);
      return;
    }
  } while (p<buffer+tl);
  fp=fopen(filename,"wb");
  if (fp==NULL) {
    free(buffer);
    return;
  }
  fprintf(fp,"[%s]" NEWL,section);
  fprintf(fp,"%s=%s" NEWL NEWL,key,value);
  fwrite(buffer,tl,1,fp);
  fclose(fp);
  free(buffer);
  return;
}

void GetINIString(char* section,char* key,char* def,char* dest,
                  int destsize,char* filename)
{
  FILE* fp;
  char s1[80],sectstr[80],keystr[80];
  char* src;
  int i;
  strcpy(dest,def);
  fp=fopen(filename,"rb");
  if (fp==NULL)
    return;
  strcpy(sectstr,"[");
  strcat(sectstr,section);
  strcat(sectstr,"]");
  strcpy(keystr,key);
  strcat(keystr,"=");
  do {
    fgets(s1,80,fp);
    sgets(s1,s1);
    if (_stricmp(s1,sectstr)==0) {
      do {
        fgets(s1,80,fp);
        sgets(s1,s1);
        if (_strnicmp(s1,keystr,strlen(keystr))==0) {
          src=s1+strlen(keystr);
          for (i=0;i<destsize-1 && src[i]!=0;++i)
            dest[i]=src[i];
          dest[i]=0;
          fclose(fp);
          return;
        }
      } while (s1[0]!=0 && !feof(fp) && !ferror(fp));
    }
  } while (!feof(fp) && !ferror(fp));
  fclose(fp);
}

Sint5 GetINIInt(char *section,char *key,Sint5 def,char *filename)
{
  char buf[80];
  sprintf(buf,"%li",def);
  GetINIString(section,key,buf,buf,80,filename);
  return atol(buf);
}

void WriteINIInt(char *section,char *key,Sint5 value,
                            char *filename)
{
  char buf[80];
  sprintf(buf,"%li",value);
  WriteINIString(section,key,buf,filename);
}

bool GetINIBool(char *section,char *key,bool def,char *filename)
{
  char buf[80];
  sprintf(buf,"%i",def);
  GetINIString(section,key,buf,buf,80,filename);
  _strupr(buf);
  if (buf[0]=='T')
    return true;
  else
    return atoi(buf);
}

void WriteINIBool(char *section,char *key,bool value,
                             char *filename)
{
  WriteINIString(section,key,value ? "true" : "false",filename);
}
