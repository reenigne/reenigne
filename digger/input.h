/* Digger Remastered
   Copyright (c) Andrew Jenner 1998-2004 */

void detectjoy(void);
bool teststart(void);
void readdir(int n);
Sint4 getdir(int n);
void checkkeyb(void);
void flushkeybuf(void);
void findkey(int kn);
void clearfire(int n);

extern bool firepflag,fire2pflag,escape;
extern Sint3 keypressed;
extern Sint4 akeypressed;

extern int keycodes[17][5];
extern bool krdf[17];
extern bool pausef;
