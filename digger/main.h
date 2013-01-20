/* Digger Remastered
   Copyright (c) Andrew Jenner 1998-2004 */

Sint4 getlevch(Sint4 bp6,Sint4 bp8,Sint4 bpa);
void incpenalty(void);
Sint4 levplan(void);
Sint4 levof10(void);
void setdead(bool df);
void cleartopline(void);
void finish(void);
Sint4 randno(Sint4 n);
void game(void);
void maininit(void);
int mainprog(void);

extern Sint4 nplayers,diggers,curplayer,startlev;
extern bool levfflag;
extern char levfname[];
extern char pldispbuf[];
extern Sint5 randv;
extern Sint3 leveldat[];
extern int gtime;
extern bool gauntlet,timeout,synchvid,unlimlives;
