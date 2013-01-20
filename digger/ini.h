/* Digger Remastered
   Copyright (c) Andrew Jenner 1998-2004 */

void WriteINIString(char *section,char *key,char *value,char *filename);
void GetINIString(char *section,char *key,char *def,char *dest,int destsize,
                  char *filename);
Sint5 GetINIInt(char *section,char *key,Sint5 def,char *filename);
void WriteINIInt(char *section,char *key,Sint5 value,char *filename);
bool GetINIBool(char *section,char *key,bool def,char *filename);
void WriteINIBool(char *section,char *key,bool value,char *filename);
