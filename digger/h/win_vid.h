/* Digger Remastered
   Copyright (c) Andrew Jenner 1998-2004 */

#ifdef DIRECTX
#include <ddraw.h>

HRESULT restore_surface();
void init_directdraw();
HRESULT ChangeCoopLevel();
HRESULT release_directdraw_objects();
void toggle_screen_mode();
void attach_clipper();
void release_clipper();
bool create_sprite_cache();
extern LPDIRECTDRAWSURFACE4 g_pDDSPrimary;
extern int use_direct_draw;
#endif

void blit_rect_to_window(int x, int y, int w, int h);
HRESULT blit_to_window();
HRESULT init_surfaces();
HRESULT fatal_error(HRESULT hRet, LPCTSTR szError);
bool create_back_buffer(HDC window_dc);
void destroy_back_buffer();
void blit_to_window_dc(HWND wnd);
void init_graphics();
void clear_sprite_cache();
void display_title_bitmap(int i);
void load_title_bitmaps();

extern BOOL g_bActive;
extern BOOL g_bReady;
extern BOOL g_bWindowed;
extern RECT g_rcWindow;
extern RECT g_rcViewport;
extern RECT g_rcScreen;
HRESULT init_surfaces();
extern HBITMAP title_bitmap[2];
extern bool use_async_screen_updates;
extern int cur_intensity;
extern int cur_palette;
extern bool palettized_desktop;
extern HDC back_dc;
extern HBITMAP back_bitmap;
#ifndef WIN32
char __huge* back_bitmap_bits;
#else
extern unsigned char *back_bitmap_bits;
#endif
extern HGDIOBJ old_bitmap;
extern bool use_640x480_fullscreen;

extern Uint3 *ascii2vga[];
extern Uint3 *ascii2cga[];
#ifdef WIN32
extern Uint3 *vgatable[];
extern Uint3 *cgatable[];
#else
extern Uint3 __huge *vgatable[];
extern Uint3 __huge *cgatable[];
#endif
extern void outtext(char *p,Sint4 x,Sint4 y,Sint4 c);
extern bool synchvid;
extern Uint3 video_mode;
#define VIDEO_MODE_VGA_16 0
#define VIDEO_MODE_CGA 1
#define VIDEO_MODE_VGA_256 2

#ifdef RUNTIMEDYNAMICLINK
extern HINSTANCE hDirectDrawInstance;
extern HRESULT (WINAPI *lpDirectDrawCreate)(GUID FAR *lpGUID, LPDIRECTDRAW FAR *lplpDD, IUnknown FAR *pUnkOuter );
#endif
