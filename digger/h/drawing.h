/* Digger Remastered
   Copyright (c) Andrew Jenner 1998-2004 */

void outtext(char *p,Sint4 x,Sint4 y,Sint4 c);

void creatembspr(void);
void initmbspr(void);
void drawmon(Sint4 n,bool nobf,Sint4 dir,Sint4 x,Sint4 y);
void drawdigger(int n,Sint4 t,Sint4 x,Sint4 y,bool f);
void drawgold(Sint4 n,Sint4 t,Sint4 x,Sint4 y);
void drawemerald(Sint4 x,Sint4 y);
void eraseemerald(Sint4 x,Sint4 y);
void drawbonus(Sint4 x,Sint4 y);
void drawlives(void);
void savefield(void);
void makefield(void);
void drawstatics(void);
void drawfire(int n,Sint4 x,Sint4 y,Sint4 t);
void eatfield(Sint4 x,Sint4 y,Sint4 dir);
void drawrightblob(Sint4 x,Sint4 y);
void drawleftblob(Sint4 x,Sint4 y);
void drawtopblob(Sint4 x,Sint4 y);
void drawbottomblob(Sint4 x,Sint4 y);
void drawmondie(Sint4 n,bool nobf,Sint4 dir,Sint4 x,Sint4 y);
void drawfurryblob(Sint4 x,Sint4 y);
void drawsquareblob(Sint4 x,Sint4 y);

extern Sint4 field[];
extern Sint4 fireheight;
