/* Digger Remastered
   Copyright (c) Andrew Jenner 1998-2004 */

void domonsters(void);
void incmont(Sint4 n);
void erasemonsters(void);
void initmonsters(void);
Sint4 monleft(void);
void killmon(Sint4 mon);
Sint4 killmonsters(int *clfirst,int *clcoll);
void checkmonscared(Sint4 h);
void squashmonsters(Sint4 bag,int *clfirst,int *clcoll);
void mongold(void);

Sint4 getfield(Sint4 x,Sint4 y);
