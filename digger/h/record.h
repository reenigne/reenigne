/* Digger Remastered
   Copyright (c) Andrew Jenner 1998-2004 */

void openplay(WCHAR *name);
void recstart(void);
void recname(WCHAR *name);
void playgetdir(Sint4 *dir,bool *fire);
void recinit(void);
void recputrand(Uint5 randv);
Uint5 playgetrand(void);
void recputinit(char *init);
void recputeol(void);
void recputeog(void);
void playskipeol(void);
void recputdir(Sint4 dir,bool fire);
void recsavedrf(void);

extern bool playing,savedrf,gotname,gotgame,drfvalid,kludge;
