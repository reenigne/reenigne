/* Digger Remastered
   Copyright (c) Andrew Jenner 1998-2004 */

void dobags(void);
Sint4 getnmovingbags(void);
void cleanupbags(void);
void initbags(void);
void drawbags(void);
bool pushbags(Sint4 dir,int *clfirst,int *clcoll);
bool pushudbags(int *clfirst,int *clcoll);
Sint4 bagy(Sint4 bag);
Sint4 getbagdir(Sint4 bag);
void removebags(int *clfirst,int *clcoll);
bool bagexist(int bag);

