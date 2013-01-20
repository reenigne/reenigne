/* Digger Remastered
   Copyright (c) Andrew Jenner 1998-2004 */

#include "def.h"
#include "win_dig.h"
#include "hardware.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "win_vid.h"
#include "resource.h"
#ifndef WIN32
#include <malloc.h>
#include <wing.h>
#endif

#ifdef DIRECTX
#define NUM_SPRITES_TO_CACHE 150
bool use_sprite_cache=FALSE;
int use_direct_draw=0;
#endif


HBITMAP title_bitmap[2]={(HBITMAP) NULL, (HBITMAP) NULL};

RGBQUAD vga16_pal1_rgbq[] = {{0,0,0,0},        /* palette1, normal intensity */
                      {128,0,0,0},
                      {0,128,0,0},
                      {128,128,0,0},
                      {0,0,128,0},
                      {128,0,128,0},
                      {0,64,128,0},
                      {128,128,128,0},
                      {64,64,64,0},
                      {255,0,0,0},
                      {0,255,0,0},
                      {255,255,0,0},
                      {0,0,255,0},
                      {255,0,255,0},
                      {0,255,255,0},
                      {255,255,255,0}};

RGBQUAD vga16_pal1i_rgbq[] = {{0,0,0,0},       /* palette1, high intensity */
                      {255,0,0,0},
                      {0,255,0,0},
                      {255,255,0,0},
                      {0,0,255,0},
                      {255,0,255,0},
                      {0,128,255,0},
                      {192,192,192,0},
                      {128,128,128,0},
                      {255,128,128,0},
                      {128,255,128,0},
                      {255,255,128,0},
                      {128,128,255,0},
                      {255,128,255,0},
                      {128,255,255,0},
                      {255,255,255,0}};

RGBQUAD vga16_pal2_rgbq[] = {{0,0,0,0},        /* palette2, normal intensity */
                      {0,128,0,0},
                      {0,0,128,0},
                      {0,64,128,0},
                      {128,0,0,0},
                      {128,128,0,0},
                      {128,0,128,0},
                      {128,128,128,0},
                      {64,64,64,0},
                      {0,255,0,0},
                      {0,0,255,0},
                      {0,255,255,0},
                      {255,0,0,0},
                      {255,255,0,0},
                      {255,0,255,0},
                      {255,255,255,0}};

RGBQUAD vga16_pal2i_rgbq[] = {{0,0,0,0},       /* palette2, high intensity */
                      {0,255,0,0},
                      {0,0,255,0},
                      {0,128,255,0},
                      {255,0,0,0},
                      {255,255,0,0},
                      {255,0,255,0},
                      {192,192,192,0},
                      {128,128,128,0},
                      {128,255,128,0},
                      {128,128,255,0},
                      {128,255,255,0},
                      {255,128,128,0},
                      {255,255,128,0},
                      {255,128,255,0},
                      {255,255,255,0}};

RGBQUAD cga16_pal1_rgbq[] = {{0,0,0,0},        /* palette1, normal intensity */
                      {0,168,0,0},
                      {0,0,168,0},
                      {0,84,168,0},
                      {0,0,128,0},
                      {128,0,128,0},
                      {0,64,128,0},
                      {128,128,128,0},
                      {64,64,64,0},
                      {255,0,0,0},
                      {0,255,0,0},
                      {255,255,0,0},
                      {0,0,255,0},
                      {255,0,255,0},
                      {0,255,255,0},
                      {255,255,255,0}};

RGBQUAD cga16_pal1i_rgbq[] = {{0,0,0,0},       /* palette1, high intensity */
                      {85,255,85,0},
                      {85,85,255,0},
                      {85,255,255,0},
                      {0,0,255,0},
                      {255,0,255,0},
                      {0,128,255,0},
                      {192,192,192,0},
                      {128,128,128,0},
                      {255,128,128,0},
                      {128,255,128,0},
                      {255,255,128,0},
                      {128,128,255,0},
                      {255,128,255,0},
                      {128,255,255,0},
                      {255,255,255,0}};

RGBQUAD cga16_pal2_rgbq[] = {{0,0,0,0},        /* palette2, normal intensity */
                      {0,128,0,0},
                      {128,0,128,0},
                      {160,160,160,0},
                      {160,160,160,0},
                      {128,128,0,0},
                      {128,0,128,0},
                      {128,128,128,0},
                      {64,64,64,0},
                      {0,255,0,0},
                      {0,0,255,0},
                      {0,255,255,0},
                      {255,0,0,0},
                      {255,255,0,0},
                      {255,0,255,0},
                      {255,255,255,0}};

RGBQUAD cga16_pal2i_rgbq[] = {{0,0,0,0},       /* palette2, high intensity */
                      {0,255,0,0},
                      {0,0,255,0},
                      {160,160,160,0},
                      {255,0,0,0},
                      {255,255,0,0},
                      {255,0,255,0},
                      {192,192,192,0},
                      {128,128,128,0},
                      {128,255,128,0},
                      {128,128,255,0},
                      {128,255,255,0},
                      {255,128,128,0},
                      {255,255,128,0},
                      {255,128,255,0},
                      {255,255,255,0}};


RGBQUAD *windowed_palette[4];  /* Used in Windowed mode. These palettes are applied to the 'back_bitmap' */

HPALETTE desktop_palette=(HPALETTE) NULL;                                     /* Used in Windowed mode, but is only if Windows is set to a color resolution which supports palettes.  {ie. 256 colors} ) */
bool palettized_desktop=FALSE;
#ifdef DIRECTX
LPDIRECTDRAWPALETTE    fullscreen_palette[4] = {NULL, NULL, NULL, NULL};      /* Used in Full Screen mode.  These palettes are applied to the DirectDraw primary surface. */
LPDIRECTDRAW4           g_pDD         = NULL;   /* DirectDraw object */
LPDIRECTDRAWSURFACE4    g_pDDSPrimary = NULL;   /* DirectDraw primary surface */
LPDIRECTDRAWSURFACE4    g_pDDSBack    = NULL;   /* DirectDraw back surface */
LPDIRECTDRAWSURFACE4    g_pDDSSpriteCache = NULL;
#endif
RECT                    g_rcWindow;             /* Saves the window size & pos.*/
RECT                    g_rcViewport;           /* Pos. & size to blt from */
RECT                    g_rcScreen;             /* Screen pos. for blt */
BOOL                    g_bActive     = FALSE;
BOOL                    g_bReady      = FALSE;  /* App is ready for updates */
BOOL                    g_bWindowed   = TRUE;   /* App is in windowed mode */
RECT                    rc_draw_area;
RECT                    rc_640x400;

bool use_async_screen_updates;
Uint3 video_mode=VIDEO_MODE_VGA_16;

int cur_intensity;
int cur_palette;

HDC back_dc;
HBITMAP back_bitmap;
#ifndef WIN32
char __huge* back_bitmap_bits;
#else
unsigned char *back_bitmap_bits;
#endif
HGDIOBJ old_bitmap;
bool use_640x480_fullscreen;

#ifdef DIRECTX
bool sprite_cached[NUM_SPRITES_TO_CACHE];   /* which sprites are already chached in video memory? */
#endif


HRESULT blit_to_window(void);
void vgaclear(void);
void destroy_palettes();
void init_palettes();

void init_graphics()
{
  HDC window_dc;

  window_dc = GetDC(hWnd);
#ifdef DIRECTX
  init_directdraw();
#endif
  create_back_buffer(window_dc);
  ReleaseDC(hWnd, window_dc);
  init_surfaces();
}

void graphicsoff(void) {};
void gretrace(void)
{
};



/********************************************************/
/* Functions for displaying the VGA data                */
/********************************************************/
void vgainit(void)
{
  video_mode=VIDEO_MODE_VGA_16;
  windowed_palette[0] = vga16_pal1_rgbq;
  windowed_palette[1] = vga16_pal1i_rgbq;
  windowed_palette[2] = vga16_pal2_rgbq;
  windowed_palette[3] = vga16_pal2i_rgbq;
  destroy_palettes();
  init_palettes();
}

void vgaclear(void)
{
#ifdef WIN32
  /*GdiFlush();*/
  memset(back_bitmap_bits, 0, 256000l);
#else
  long int i;
  for (i=0;i<10;i++)
    farmemset(back_bitmap_bits+ (Uint5) i*25600l, 0, 25600l);
#endif
  blit_to_window();
};

void vgapal(Sint4 pal)
{
#ifdef WIN32
  /*GdiFlush();*/
#endif
  cur_palette=pal;
  if (g_bWindowed)
  {
#ifdef WIN32
    SetDIBColorTable(back_dc,0,16, windowed_palette[cur_palette*2 + cur_intensity]);
#else
    WinGSetDIBColorTable(back_dc,0,16, windowed_palette[cur_palette*2 + cur_intensity]);
#endif
    if (use_async_screen_updates)
      InvalidateRect(hWnd, NULL, FALSE);
    else
      blit_to_window();
  }
#ifdef DIRECTX
  else
  {
    IDirectDrawSurface4_SetPalette(g_pDDSPrimary, fullscreen_palette[cur_palette*2 + cur_intensity]);
    SetDIBColorTable(back_dc,0,16, windowed_palette[cur_palette*2 + cur_intensity]);
  }
#endif
};

void vgainten(Sint4 inten)
{
#ifdef WIN32
  /*GdiFlush();*/
#endif
  cur_intensity=inten;
  if (g_bWindowed)
  {
#ifdef WIN32
    SetDIBColorTable(back_dc,0,16, windowed_palette[cur_palette*2 + cur_intensity]);
#else
    WinGSetDIBColorTable(back_dc,0,16, windowed_palette[cur_palette*2 + cur_intensity]);
#endif
    if (use_async_screen_updates)
      InvalidateRect(hWnd, NULL, FALSE);
    else
      blit_to_window();
  }
#ifdef DIRECTX
  else
  {
    IDirectDrawSurface4_SetPalette(g_pDDSPrimary, fullscreen_palette[cur_palette*2 + cur_intensity]);
    SetDIBColorTable(back_dc,0,16, windowed_palette[cur_palette*2 + cur_intensity]);
  }
#endif
};

Sint4 vgagetpix(Sint4 x,Sint4 y)
{
  Uint4 xi,yi;
  Sint4 rval;

#ifdef WIN32
  /*GdiFlush();*/
#endif
  rval=0;
  if (x>319||y>199)
  {
    return 0xff;
  }
  for (yi=0;yi<2;yi++)
    for (xi=0;xi<8;xi++)
      if (back_bitmap_bits[(Uint5) ((y*2l+yi)*640l + x*2l + xi)])
        rval |= 0x80 >> xi;

  rval &= 0xee;
  return rval;
};

void vgawrite(Sint4 x,Sint4 y,Sint4 ch,Sint4 c)
{
  Uint5 yi;
  Uint5 xi;
  int color;

  ch-=32;
  if (ch<0x5f)
  {
    if (ascii2vga[ch])
    {
      for (yi=0;yi<24;yi++)
      {
        for (xi=0;xi<24;xi++)
        {
          if (xi&0x1)
            color=(ascii2vga[ch][yi*12+(xi>>1)] & 0x0F);
          else
            color=(ascii2vga[ch][yi*12+(xi>>1)] >> 4);
          if (color==10)
            if (c==2)
              color=12;
            else
            {
              if (c==3)
                color=14;
            }
          else
            if (color==12)
              if (c==1)
                color=2;
              else
                if (c==2)
                  color=4;
                else
                  if (c==3)
                    color=6;

          back_bitmap_bits[(Uint5) ((y*2+yi)*640l + x*2l+xi)]=color;
        }
      }
    }
    else    /* draw a space (needed when reloading and displaying high scores when user switches to/from normal/gauntlet mode, etc ) */
      for (yi=0;yi<24;yi++)
#ifdef WIN32
        memset(&back_bitmap_bits[(y*2+yi)*640 + x*2] , 0, 24);
#else
        ;//farmemset(&back_bitmap_bits[(y*2+yi)*640l + x*2l] , 0, 24);
#endif
  }
  blit_rect_to_window(x*2, y*2, 24, 24);
}

void cgawrite(Sint4 x,Sint4 y,Sint4 ch,Sint4 c)
{
  Uint5 yi;
  Uint5 xi;
  int color;

  ch-=32;
  if (ch<0x5f)
  {
    if (ascii2cga[ch])
    {
      for (yi=0;yi<12;yi++)
      {
        for (xi=0;xi<12;xi++)
        {
          if (xi&0x1)
            color=(ascii2cga[ch][yi*6+(xi>>1)] & 0x0F);
          else
            color=(ascii2cga[ch][yi*6+(xi>>1)] >> 4);

          color=color&c;
          farmemset(back_bitmap_bits + ((y*2+yi*2)*640l + x*2+xi*2), color,2);
          farmemset(back_bitmap_bits + ((y*2+yi*2+1)*640l + x*2+xi*2), color,2);
        }
      }
    }
    else    /* draw a space (needed when reloading and displaying high scores when user switches to/from normal/gauntlet mode, etc ) */
      for (yi=0;yi<24;yi++)
#ifdef WIN32
        memset(&back_bitmap_bits[(y*2+yi)*640 + x*2] , 0, 24);
#else
        ;//farmemset(&back_bitmap_bits[(y*2+yi)*640l + x*2l] , 0, 24);
#endif
  }
  blit_rect_to_window(x*2, y*2, 24, 24);
}


void vgatitle(void)
{
  display_title_bitmap(0);
}

void cgatitle(void)
{
  display_title_bitmap(1);
}

void display_title_bitmap(int idx)
{
#ifdef WIN32
  HDC temp_dc;
  HGDIOBJ old_bitmap;
  GdiFlush();
  temp_dc = CreateCompatibleDC(back_dc);
  if (palettized_desktop)
  {
    SelectPalette(temp_dc, desktop_palette, FALSE);
    RealizePalette(temp_dc);
  }
  old_bitmap=SelectObject(temp_dc, title_bitmap[idx]);
  BitBlt(back_dc, 0, 0, 640, 400, temp_dc, 0, 0, SRCCOPY);
  GdiFlush();
  SelectObject(temp_dc, old_bitmap);
  DeleteDC(temp_dc);
#else
  HDC temp_dc;
  int result;
  struct {
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD bmiColors[256];
  } bmi;

  temp_dc=GetDC(NULL);
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = 640;
  bmi.bmiHeader.biHeight = -400;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 8;
  bmi.bmiHeader.biClrUsed = 16;
  bmi.bmiHeader.biClrImportant = 16;
  bmi.bmiHeader.biCompression = BI_RGB;
  memcpy(bmi.bmiColors, windowed_palette[0], sizeof(RGBQUAD) * 16);
  result=GetDIBits(temp_dc, title_bitmap[idx], 0, 400, back_bitmap_bits, &bmi, DIB_PAL_COLORS);
  ReleaseDC(NULL,temp_dc);
#endif
  blit_to_window();

}

/* blit to entire backbuffer to the window */
HRESULT blit_to_window()
{
#ifdef DIRECTX
  HRESULT hresult;

  hresult=DD_OK;

  if (!g_bWindowed && !g_bActive)
    return hresult;

  if (g_bWindowed)
  {
    InvalidateRect(hWnd, NULL, FALSE);
    UpdateWindow(hWnd);
  }
  else
  {
    if (g_pDDSPrimary)
      return IDirectDrawSurface4_Blt(g_pDDSPrimary,  &g_rcScreen, g_pDDSBack, &rc_draw_area, DDBLT_WAIT, NULL);
  }
  return DD_OK;
#else
  InvalidateRect(hWnd,NULL, FALSE);
  UpdateWindow(hWnd);
  return 0;
#endif
}

/* This is ONLY called when the window receives a WM_PAINT message. */
/* Repaint all or a portion of the window.                          */
/* (used in both Windowed and Fullscreen mode)                      */
void blit_to_window_dc(HWND wnd)
{
  HDC hDC;
  PAINTSTRUCT paintStruct;
#ifndef WIN32
  BOOL result;
#endif

  hDC = BeginPaint(hWnd, &paintStruct);
  if (palettized_desktop)
  {
    SelectPalette (hDC, desktop_palette, TRUE);
    RealizePalette (hDC);
  }

#ifdef WIN32
  if (g_bWindowed)
  {
    BitBlt(hDC,0,0,640,400,back_dc, 0,0,SRCCOPY);
  }
  else
    BitBlt(hDC,g_rcScreen.left,g_rcScreen.top,640,400,back_dc, 0,0,SRCCOPY);
  //GdiFlush();
#else
  result=WinGBitBlt(hDC,0,0,640,400,back_dc,0,0);
#endif

  EndPaint(hWnd, &paintStruct);
}

#ifdef DIRECTX
HRESULT restore_surface()
{
  HRESULT hRet;
  clear_sprite_cache();
  hRet=IDirectDrawSurface4_Restore(g_pDDSPrimary);
  if (hRet != DD_OK)
  {
    fatal_error(hRet, "DirectDraw: restore surfaces (primary) failed");
  }
  hRet=IDirectDrawSurface4_Restore(g_pDDSSpriteCache);
  if (hRet != DD_OK)
  {
    fatal_error(hRet, "DirectDraw: restore surfaces (sprite cache) failed");
  }
  else
    if (!g_bWindowed)
      memset(sprite_cached, 0, NUM_SPRITES_TO_CACHE * sizeof(bool));
  return TRUE;
}
#endif

/* Blits the given rectangle to the Window (or marks the given rectangle */
/* as dirty if Async option is on).                                      */
/* In DirectX/Fullscreen mode, there must NOT be a Clipper attached to   */
/* the surface (otherwise BltFast will not work).                        */
void blit_rect_to_window(int x, int y, int w, int h)
{
  RECT rect;

  if (!g_bWindowed && !g_bActive)
    return;

  if (g_bWindowed)
  {
    rect.left=x;
    rect.right=x+w;
    rect.top=y;
    rect.bottom=y+h;
    InvalidateRect(hWnd, &rect, FALSE);
    if (!use_async_screen_updates)
      UpdateWindow(hWnd);
  }
#ifdef DIRECTX
  else
  {
    if (g_pDDSPrimary)
    {
      rect.left=x;
      rect.right=x+w;
      rect.top=y;
      rect.bottom=y+h;
      IDirectDrawSurface4_BltFast(g_pDDSPrimary, g_rcScreen.left+x, g_rcScreen.top+y, g_pDDSBack, &rect, DDBLTFAST_NOCOLORKEY /* | DDBLTFAST_WAIT */);
    }
  }
#endif
}

/* This function creates a back buffer for drawing on.             */
/*  - on Win32 this is a DIBSection                                */
/*  - on Win16 this is a WinGBitmap                                */
bool create_back_buffer(HDC window_dc)
{
#ifdef WIN32
  HANDLE hloc;
  BITMAPINFO *pbmi;
#else
  struct {
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD bmiColors[256];
  } bmi;
#endif

#ifdef WIN32
  back_dc = CreateCompatibleDC(window_dc);
#else
  back_dc = WinGCreateDC();
  if (!back_dc)
    fatal_error(0, "create_back_buffer: WinGCreateDC() FAILED");
#endif
  back_bitmap=0;

#ifdef WIN32
  hloc = LocalAlloc(LMEM_ZEROINIT | LMEM_MOVEABLE, sizeof(BITMAPINFOHEADER) + (sizeof(RGBQUAD) * 16));
  pbmi = (BITMAPINFO*) LocalLock(hloc);
  pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  pbmi->bmiHeader.biWidth = 640;
  pbmi->bmiHeader.biHeight = -400;
  pbmi->bmiHeader.biPlanes = 1;
  pbmi->bmiHeader.biBitCount = 8;
  pbmi->bmiHeader.biClrUsed = 16;
  pbmi->bmiHeader.biClrImportant = 16;
  pbmi->bmiHeader.biCompression = BI_RGB;
  memset(pbmi->bmiColors,0,sizeof(RGBQUAD)*16);
  back_bitmap = CreateDIBSection(window_dc, (BITMAPINFO*) pbmi, DIB_RGB_COLORS, (VOID **) &back_bitmap_bits, (HANDLE) NULL, 0);
  LocalFree(hloc);
#else
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = 640;
  bmi.bmiHeader.biHeight = -400;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 8;
  bmi.bmiHeader.biClrUsed = 16;
  bmi.bmiHeader.biClrImportant = 16;
  bmi.bmiHeader.biCompression = BI_RGB;
  memcpy(bmi.bmiColors, windowed_palette[0], sizeof(RGBQUAD) * 16);
  back_bitmap = WinGCreateBitmap(back_dc, (BITMAPINFO far *) &bmi, (void far* far*) &back_bitmap_bits);
#endif

  if (!back_bitmap)
    return FALSE;
  old_bitmap = SelectObject(back_dc, back_bitmap);

  return TRUE;
}

#ifdef DIRECTX
void create_back_buffer_alias()
{
  /* Now create a DirectDrawSurface which actually refers to the same     */
  /* memory location as the 'back_bitmap_bits'.  This is so that I can    */
  /* use the DirectDraw 'blit' while in full-screen mode or use the       */
  /* standard GDI BitBlt when in windowed mode.  The GDI BitBlt is used   */
  /* while in Windowed mode (even though it is slow) so that I can 'fake' */
  /* the palette functions when Windows is set to High/True Color.        */

  HRESULT          hRet;
  DDSURFACEDESC2   ddsd;

  ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
  ZeroMemory(&ddsd.ddpfPixelFormat, sizeof(DDPIXELFORMAT));
  ddsd.dwSize = sizeof(ddsd);
  ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_LPSURFACE;
/* According to the DX6.1 SDK documentation, the Caps member doesn't have    */
/* to be set, but I get an INVALIDCAPS error on the CreateSurface if I don't */
/* set it.                                                                   */
  ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
  ddsd.dwWidth = 640;
  ddsd.dwHeight= 400;
  ddsd.lPitch  = (LONG) 640;
  ddsd.lpSurface = back_bitmap_bits;
  ddsd.ddpfPixelFormat.dwFlags= DDPF_RGB |DDPF_PALETTEINDEXED8;
  ddsd.ddpfPixelFormat.dwRGBBitCount = (DWORD) 8;
  hRet = IDirectDraw4_CreateSurface(g_pDD, &ddsd, &g_pDDSBack, NULL);
  if (hRet != DD_OK)
    fatal_error(hRet, "create back surface alias FAILED");
}
#endif

void destroy_back_buffer()
{
#ifdef DIRECTX
  release_directdraw_objects();
#endif
  SelectObject(back_dc, old_bitmap);
  DeleteObject(back_bitmap);
  DeleteDC(back_dc);
  DeleteObject(desktop_palette);
}

#ifdef DIRECTX
HRESULT release_directdraw_objects()
{
  if (use_direct_draw==6)
  {
    if (g_pDD != NULL)
    {
      IDirectDraw4_SetCooperativeLevel(g_pDD, hWnd, DDSCL_NORMAL);
      if (g_pDDSBack != NULL)
      {
          IDirectDraw4_Release(g_pDDSBack);
          g_pDDSBack = NULL;
      }
      if (g_pDDSPrimary != NULL)
      {
          IDirectDraw4_Release(g_pDDSPrimary);
          g_pDDSPrimary = NULL;
      }
    }
  }
  return DD_OK;
}
#endif

HRESULT fatal_error(HRESULT hRet, LPCTSTR szError)
{
  windows_finish();
  MessageBox(hWnd, szError, "Digger", MB_OK);
  DestroyWindow(hWnd);
  exit(1);
  return hRet;
}


HRESULT init_surfaces()
{
#ifdef DIRECTX
  HRESULT          hRet;
  DDSURFACEDESC2   ddsd;
  DDBLTFX ddbltfx;
#endif
  int i, palnum;
  HDC hDC;
  LOGPALETTE*      logpalette;
  RGBQUAD *wp[] = { vga16_pal1_rgbq,vga16_pal1i_rgbq,vga16_pal2_rgbq,vga16_pal2i_rgbq };

#ifdef DIRECTX
  if (use_direct_draw==0)
#endif
    g_bWindowed=TRUE;

  if (g_bWindowed)
  {
    /* Are we running in 256 colors (or less)?  */

    hDC=GetDC(hWnd);
    palettized_desktop=GetDeviceCaps(hDC, RASTERCAPS)&RC_PALETTE;
    if (palettized_desktop)
    {
      logpalette=(LOGPALETTE*) malloc(sizeof(LOGPALETTE) + sizeof(PALETTEENTRY)*64);
      if (!logpalette)
        fatal_error(0,"init_surfaces: could not allocate memory for 'logpalette'");
      logpalette->palNumEntries=64;
      logpalette->palVersion=0x300;

      for (palnum=0;palnum<4;palnum++)
        for (i=0;i<16;i++)
        {
          logpalette->palPalEntry[palnum*16+i].peRed=wp[palnum][i].rgbRed;
          logpalette->palPalEntry[palnum*16+i].peBlue=wp[palnum][i].rgbBlue;
          logpalette->palPalEntry[palnum*16+i].peGreen=wp[palnum][i].rgbGreen;
          logpalette->palPalEntry[palnum*16+i].peFlags=(BYTE) NULL; //PC_NOCOLLAPSE;
        }

      desktop_palette=CreatePalette(logpalette);
      free (logpalette);
      SelectPalette(hDC,desktop_palette,TRUE);
      i=RealizePalette(hDC);
    }
    ReleaseDC(hWnd, hDC);

#ifdef DIRECTX
    if (use_direct_draw==6)
    {
      hRet = IDirectDraw4_SetCooperativeLevel(g_pDD, hWnd, DDSCL_NORMAL);
      if (hRet != DD_OK)
        return fatal_error(hRet, "DirectDraw: SetCooperativeLevel FAILED");
    }
#endif
    GetClientRect(hWnd, &g_rcViewport);
    GetClientRect(hWnd, &g_rcScreen);
    ClientToScreen(hWnd, (POINT*)&g_rcScreen.left);
    ClientToScreen(hWnd, (POINT*)&g_rcScreen.right);
  }
#ifdef DIRECTX
  else
  {
    SetWindowLong(hWnd, GWL_STYLE, DIGGER_WS_FULLSCREEN);
    hRet = IDirectDraw4_SetCooperativeLevel(g_pDD, hWnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    if (hRet != DD_OK)
      return fatal_error(hRet, "DirectDraw: SetCooperativeLevel (fullscreen) FAILED");

    SetRect(&g_rcViewport, 0, 0, 640, 400 );
    memcpy(&g_rcScreen, &g_rcViewport, sizeof(RECT) );

    if (!use_640x480_fullscreen)
      hRet = IDirectDraw4_SetDisplayMode(g_pDD, 640, 400, 8, 0, 0);
    if (hRet != DD_OK || use_640x480_fullscreen)
    {
      hRet = IDirectDraw4_SetDisplayMode(g_pDD, 640, 480, 8, 0, 0);
      if (hRet != DD_OK)
      {
        return fatal_error(hRet, "DirectDraw: SetDisplayMode FAILED");
      }
      SetRect(&g_rcViewport, 0, 40, 640, 440 );
      memcpy(&g_rcScreen, &g_rcViewport, sizeof(RECT) );
    }

    /* Create the primary surface */
    ZeroMemory(&ddsd,sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    hRet = IDirectDraw4_CreateSurface(g_pDD, &ddsd, &g_pDDSPrimary, NULL);
    if (hRet != DD_OK)
      return fatal_error(hRet, "DirectDraw: CreateSurface (primary, fullscreen) FAILED");

    vgainten(cur_intensity);

    /* clear the screen */
    ZeroMemory(&ddbltfx,sizeof(DDBLTFX));
    ddbltfx.dwFillColor=0;
    ddbltfx.dwSize=sizeof(DDBLTFX);
    IDirectDrawSurface4_Blt(g_pDDSPrimary, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT , &ddbltfx );

  }
#endif

  rc_640x400.left=0;
  rc_640x400.top=0;
  rc_640x400.right=640;
  rc_640x400.bottom=400;

  rc_draw_area.left=0;
  rc_draw_area.top=0;

  rc_draw_area.right=640;
  rc_draw_area.bottom=400;


#ifdef DIRECTX
  create_sprite_cache();
  if (use_direct_draw>0)
    create_back_buffer_alias();
  return DD_OK;
#else
  return 0;
#endif
}

void destroy_palettes()
{
#ifdef DIRECTX
  fullscreen_palette[0]=NULL;
#endif
}

void init_palettes()
{
#ifdef DIRECTX
  HRESULT          hRet;
  PALETTEENTRY     palentry[256];
#endif
  int i, palnum;
  HDC hDC;
  LOGPALETTE*      logpalette;

#ifdef DIRECTX
  if (use_direct_draw==0)
#endif
    g_bWindowed=TRUE;

  if (g_bWindowed)
  {
    /* Are we running in 256 colors (or less)?  */

    hDC=GetDC(hWnd);
    palettized_desktop=GetDeviceCaps(hDC, RASTERCAPS)&RC_PALETTE;
    if (palettized_desktop)
    {
      logpalette=(LOGPALETTE*) malloc(sizeof(LOGPALETTE) + sizeof(PALETTEENTRY)*64);
      if (!logpalette)
        fatal_error(0,"init_surfaces: could not allocate memory for 'logpalette'");
      logpalette->palNumEntries=64;
      logpalette->palVersion=0x300;

      for (palnum=0;palnum<4;palnum++)
        for (i=0;i<16;i++)
        {
          logpalette->palPalEntry[palnum*16+i].peRed=windowed_palette[palnum][i].rgbRed;
          logpalette->palPalEntry[palnum*16+i].peBlue=windowed_palette[palnum][i].rgbBlue;
          logpalette->palPalEntry[palnum*16+i].peGreen=windowed_palette[palnum][i].rgbGreen;
          logpalette->palPalEntry[palnum*16+i].peFlags=(BYTE) NULL; //PC_NOCOLLAPSE;
        }

      desktop_palette=CreatePalette(logpalette);
      free (logpalette);
      SelectPalette(hDC,desktop_palette,TRUE);
      i=RealizePalette(hDC);
    }
    ReleaseDC(hWnd, hDC);

#ifdef DIRECTX
    if (use_direct_draw==6)
    {
      hRet = IDirectDraw4_SetCooperativeLevel(g_pDD, hWnd, DDSCL_NORMAL);
      if (hRet != DD_OK)
        fatal_error(hRet, "DirectDraw: SetCooperativeLevel FAILED");
    }
#endif
    GetClientRect(hWnd, &g_rcViewport);
    GetClientRect(hWnd, &g_rcScreen);
    ClientToScreen(hWnd, (POINT*)&g_rcScreen.left);
    ClientToScreen(hWnd, (POINT*)&g_rcScreen.right);
  }
#ifdef DIRECTX
  else
  {
    if (fullscreen_palette[0]==NULL)
    {
      for (palnum=0;palnum<4;palnum++)
      {
        for (i=0;i<256;i++)
        {
          if (i<16)
          {
            palentry[i].peRed=windowed_palette[palnum][i].rgbRed;
            palentry[i].peBlue=windowed_palette[palnum][i].rgbBlue;
            palentry[i].peGreen=windowed_palette[palnum][i].rgbGreen;
            palentry[i].peFlags=(BYTE) NULL;
          }
          else
          {
            palentry[i].peRed=0;
            palentry[i].peBlue=0;
            palentry[i].peGreen=0;
            palentry[i].peFlags=(BYTE) NULL;
          }
        }
        IDirectDraw4_CreatePalette(g_pDD, DDPCAPS_8BIT, palentry, &fullscreen_palette[palnum], NULL);
      }
    }

    vgainten(cur_intensity);
  }
#endif
}

#ifdef DIRECTX
bool create_sprite_cache()
{
  HRESULT          hRet;
  DDSURFACEDESC2   ddsd;
  DDCOLORKEY       ddcolorkey;

  use_sprite_cache=FALSE;
  if (use_direct_draw==0)
    return FALSE;

  ZeroMemory(&ddsd,sizeof(ddsd));
  ddsd.dwSize = sizeof(ddsd);
  ddsd.dwFlags        = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT;
  ddsd.dwWidth        = 48;
  ddsd.dwHeight       = 40 * NUM_SPRITES_TO_CACHE; /* plenty of room for up to NUM_SPRITES_TO_CACHE sprites. sure, it's a waste of a lot of space, but so what... */
  ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;

  ddsd.ddpfPixelFormat.dwFlags= DDPF_RGB |DDPF_PALETTEINDEXED8;
  ddsd.ddpfPixelFormat.dwRGBBitCount = (DWORD) 8;

  hRet = IDirectDraw4_CreateSurface(g_pDD, &ddsd, &g_pDDSSpriteCache, NULL);
  if (hRet != DD_OK)
    fatal_error(hRet, "DirectDraw: CreateSurface (sprite_cache) FAILED");
  else
    use_sprite_cache=TRUE;

  memset(sprite_cached, 0, NUM_SPRITES_TO_CACHE * sizeof(bool));

  ddcolorkey.dwColorSpaceLowValue=ddcolorkey.dwColorSpaceHighValue=13;
  IDirectDrawSurface4_SetColorKey(g_pDDSSpriteCache, DDCKEY_SRCBLT, &ddcolorkey);
  return (hRet == DD_OK);
}

/* toggle between windowed and fullscreen mode */
HRESULT ChangeCoopLevel()
{
    HRESULT hRet;

    if (FAILED(hRet = release_directdraw_objects()))
      return fatal_error(hRet, "DirectDraw: release_directdraw_objects FAILED");

    if (g_bWindowed)
    {
      SetWindowLong(hWnd, GWL_STYLE, DIGGER_WS_WINDOWED);
      SetWindowPos(hWnd, HWND_NOTOPMOST, g_rcWindow.left, g_rcWindow.top,
                  (g_rcWindow.right - g_rcWindow.left),
                  (g_rcWindow.bottom - g_rcWindow.top), SWP_SHOWWINDOW );
      InvalidateRect(NULL, NULL, TRUE);
    }
    use_sprite_cache=FALSE;
    hRet = init_surfaces();
    init_palettes();
    return hRet;
}

void init_directdraw()
{
  HRESULT hRet;
  LPDIRECTDRAW    pDD;

  GetWindowRect(hWnd, &g_rcWindow);

#ifdef RUNTIMEDYNAMICLINK
  if (!lpDirectDrawCreate)
  {
    use_direct_draw=0;
    return;
  }
#endif

  hRet = DirectDrawCreate( NULL, &pDD, NULL);
  if (hRet==DD_OK)
  {
#ifdef __cplusplus
    hRet = IDirectDraw_QueryInterface(pDD, IID_IDirectDraw4, (LPVOID *)&g_pDD);
#else
    hRet = IDirectDraw_QueryInterface(pDD, &IID_IDirectDraw4, (LPVOID *)&g_pDD);
#endif
    if (hRet==DD_OK)
      use_direct_draw=6;
  }
}

void attach_clipper()
{
  LPDIRECTDRAWCLIPPER pClipper;
  HRESULT hRet;

  /* add a clipper to the primary surface */
  hRet = IDirectDraw4_CreateClipper(g_pDD, 0, &pClipper, NULL);
  if (hRet != DD_OK)
    fatal_error(hRet, "attach_clipper(): CreateClipper FAILED");
  hRet = IDirectDrawClipper_SetHWnd(pClipper, 0, hWnd);
  if (hRet != DD_OK)
    fatal_error(hRet, "attach_clipper(): SetHWnd FAILED");
  hRet = IDirectDrawSurface_SetClipper(g_pDDSPrimary, pClipper);
  if (hRet != DD_OK)
    fatal_error(hRet, "attach_clipper(): SetClipper FAILED");
  IDirectDrawClipper_Release(pClipper);
  pClipper = NULL;
}

void release_clipper()
{
  HRESULT hRet;
  hRet = IDirectDrawSurface_SetClipper(g_pDDSPrimary, (LPDIRECTDRAWCLIPPER) NULL);
  if (hRet != DD_OK)
    fatal_error(hRet, "release_clipper(): SetClipper FAILED");
}

void clear_sprite_cache()
{
  memset(sprite_cached,0,NUM_SPRITES_TO_CACHE*sizeof(bool));
}
#endif

void vgaputim(Sint4 x,Sint4 y,Sint4 ch,Sint4 w,Sint4 h)
{
/* TO DO: convert the vgagraphics to a more appropriate format,
          rewrite this routine(load the sprites onto the DirectDraw
          Surface beforehand). */

  Uint5 scrn_width;
  Uint5 scrn_height;
  int y_loop_end;
  int y_loop_count;
  int x_loop_end;
  int x_loop_count;
#ifdef WIN32
  unsigned char* cur_src_mask_ptr;
  unsigned char* cur_src_ptr;
  unsigned char* cur_dest_ptr;
  unsigned char* scrn_max_ptr;
#else
  Uint3 __huge* cur_src_mask_ptr;
  Uint3 __huge* cur_src_ptr;
  char __huge* cur_dest_ptr;
  char __huge* scrn_max_ptr;
#endif
  Uint5 i;
  Uint5 plane;
  int color;
  Uint5 dest_next_row_offset;
  Uint5 src_plane_offset;
#ifdef DIRECTX
  RECT rect;
  DDSURFACEDESC2 ddsd;
  DDBLTFX ddbltfx;
  HRESULT hRet;

  if (!g_bWindowed)
    if (IDirectDrawSurface4_IsLost(g_pDDSPrimary)||IDirectDrawSurface4_IsLost(g_pDDSSpriteCache))
      use_sprite_cache=FALSE;

  if ((!use_sprite_cache)||(!sprite_cached[ch]))
  {
#endif

#ifdef WIN32
    /*GdiFlush();*/
#endif
    scrn_width=640;
    scrn_height=400;
    scrn_max_ptr = back_bitmap_bits + (Uint5) scrn_width*scrn_height;

    cur_src_mask_ptr=vgatable[ch*2+1];
    cur_src_ptr=vgatable[ch*2];
#ifdef DIRECTX
    if (use_sprite_cache)
    {
      rect.left = 0;
      rect.top = ch*48;
      rect.right = w*8;
      rect.bottom = rect.top + h*2;
      ZeroMemory(&ddbltfx,sizeof(DDBLTFX));
      ddbltfx.dwFillColor=13;
      ddbltfx.dwSize=sizeof(DDBLTFX);
      hRet=IDirectDrawSurface4_Blt(g_pDDSSpriteCache, &rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT , &ddbltfx );
      if (hRet!=DD_OK)
        fatal_error(hRet, "vgaputim: blt failed");
      ZeroMemory(&ddsd,sizeof(DDSURFACEDESC2));
      ddsd.dwSize=sizeof(DDSURFACEDESC2);
      hRet=IDirectDrawSurface4_Lock(g_pDDSSpriteCache, &rect, &ddsd, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL);
      if (hRet!=DD_OK)
        fatal_error(hRet, "vgaputim: lock failed");
      cur_dest_ptr=(unsigned char *)ddsd.lpSurface;
      dest_next_row_offset = ddsd.lPitch-w*8;
    }
    else
    {
#endif
      cur_dest_ptr=&(back_bitmap_bits[(Uint5) (y*2l * scrn_width + x*2l)]);
      dest_next_row_offset = scrn_width - w*8l;
#ifdef DIRECTX
    }
#endif
    src_plane_offset = w*h*2;

    y_loop_end=h*2;
    x_loop_end=w;

    for (y_loop_count=0;y_loop_count<y_loop_end;y_loop_count++)
    {
      for (x_loop_count=0;x_loop_count<x_loop_end;x_loop_count++)
      {
        for (i=0;i<8;i++)
        {
          if (!((*cur_src_mask_ptr)&(0x80>>i)))
          {
            color=0;
            for (plane=0;plane<4;plane++)
            {
              color|=((((*(cur_src_ptr + (Uint5) (plane*src_plane_offset))) << i) & 0x80 ) >> (4 + plane));
            }
#ifdef DIRECTX
            if (use_sprite_cache||cur_dest_ptr < scrn_max_ptr)
#else
            if (cur_dest_ptr < scrn_max_ptr)
#endif
              *cur_dest_ptr = color;

          }
          cur_dest_ptr++;
        }
        cur_src_ptr++;
        cur_src_mask_ptr++;
      }
      cur_dest_ptr+=dest_next_row_offset;
    }
#ifdef DIRECTX
    if (use_sprite_cache)
    {
      IDirectDrawSurface4_Unlock(g_pDDSSpriteCache, &rect);
      if (hRet!=DD_OK)
        fatal_error(hRet, "vgaputim: unlock failed");
      sprite_cached[ch]=TRUE;
    }
  }

  if (use_sprite_cache)
  {
    rect.left = 0;
    rect.right = w*8;
    rect.top = ch* 48;
    if (y*2 + h*2 < 400)
      rect.bottom = rect.top + h*2;
    else
      rect.bottom = rect.top + h*2 - (y*2 + h*2) + 400;
    hRet=IDirectDrawSurface4_BltFast(g_pDDSBack, x*2, y*2, g_pDDSSpriteCache, &rect, DDBLTFAST_WAIT | DDBLTFAST_SRCCOLORKEY);
    if (hRet!=DD_OK)
      fatal_error(hRet, "vgaputim: blt to back surface failed");
    if (!g_bWindowed)
    {
      hRet=IDirectDrawSurface4_BltFast(g_pDDSPrimary, g_rcScreen.left + x*2, g_rcScreen.top + y*2, g_pDDSSpriteCache, &rect, DDBLTFAST_WAIT | DDBLTFAST_SRCCOLORKEY);
      if (hRet!=DD_OK)
        fatal_error(hRet, "vgaputim: blt to screen failed");
    }
    else
    {
      if (y*2 + h*2 < 400)
        blit_rect_to_window(x*2, y*2, w*8, h*2);
      else
        blit_rect_to_window(x*2, y*2, w*8, h*2 - (y*2 + h*2) + 400);
    }
  }
  else
  {
#endif
    if (y*2 + h*2 < 400)
      blit_rect_to_window(x*2, y*2, w*8, h*2);
    else
      blit_rect_to_window(x*2, y*2, w*8, h*2 - (y*2 + h*2) + 400);
#ifdef DIRECTX
  }
#endif

}

void vgaputi(Sint4 x,Sint4 y,Uint3 *p,Sint4 w,Sint4 h)
{
  int i;
#ifdef WIN32
  /*GdiFlush();*/
#endif
  for (i=0;i<h*2;i++)
    if (i+y*2 < 400)
//#ifdef WIN32
//      memcpy(back_bitmap_bits+(y*2+i)*640 + x*2, p+i*w*8 , w*8);
//#else
      farmemcpy(back_bitmap_bits+(Uint5) ((y*2+i)*640l + x*2), (char far*) (p + (Uint5) (i*w*8l)) , w*8l);
//#endif

  if (y*2 + h*2 < 400)
    blit_rect_to_window(x*2, y*2, w*8, h*2);
  else
    blit_rect_to_window(x*2, y*2, w*8, h*2 - (y*2 + h*2) + 400);
}

void vgageti(Sint4 x,Sint4 y,Uint3 *p,Sint4 w,Sint4 h)
{
  int i;
#ifdef WIN32
  /* GdiFlush(); */
#endif
  for (i=0;i<h*2;i++)
    if (i+y*2 < 400)
//#ifdef WIN32
//      memcpy( p+i*w*8, back_bitmap_bits+ (y*2+i)*640 + x*2 , w*8);
//#else
      farmemcpy( (char far*) (p + (Uint5) i*w*8l), back_bitmap_bits+ (Uint5) ((y*2l+i)*640l + x*2l) , w*8);
//#endif
}

void cgaputim(Sint4 x,Sint4 y,Sint4 ch,Sint4 w,Sint4 h)
{
/* TO DO: convert the cgagraphics to a more appropriate format,
          rewrite this routine(load the sprites onto the DirectDraw
          Surface beforehand). */

  Uint5 scrn_width;
  Uint5 scrn_height;
  int y_loop_end;
  int y_loop_count;
  int x_loop_end;
  int x_loop_count;
#ifdef WIN32
  unsigned char* cur_src_mask_ptr;
  unsigned char* cur_src_ptr;
  unsigned char* cur_dest_ptr;
  unsigned char* scrn_max_ptr;
#else
  Uint3 __huge* cur_src_mask_ptr;
  Uint3 __huge* cur_src_ptr;
  char __huge* cur_dest_ptr;
  char __huge* scrn_max_ptr;
#endif
  Uint5 i;
  int color;
  Uint5 dest_next_row_offset;
#ifdef DIRECTX
  RECT rect;
  DDSURFACEDESC2 ddsd;
  DDBLTFX ddbltfx;
  HRESULT hRet;

  if (!g_bWindowed)
    if (IDirectDrawSurface4_IsLost(g_pDDSPrimary)||IDirectDrawSurface4_IsLost(g_pDDSSpriteCache))
      use_sprite_cache=FALSE;

  if ((!use_sprite_cache)||(!sprite_cached[ch]))
  {
#endif

#ifdef WIN32
    /*GdiFlush();*/
#endif
    scrn_width=640;
    scrn_height=400;
    scrn_max_ptr = back_bitmap_bits + (Uint5) scrn_width*scrn_height;

    cur_src_mask_ptr=cgatable[ch*2+1];
    cur_src_ptr=cgatable[ch*2];
#ifdef DIRECTX
    if (use_sprite_cache)
    {
      rect.left = 0;
      rect.top = ch*48;
      rect.right = w*8;
      rect.bottom = rect.top + h*2;
      ZeroMemory(&ddbltfx,sizeof(DDBLTFX));
      ddbltfx.dwFillColor=13;
      ddbltfx.dwSize=sizeof(DDBLTFX);
      hRet=IDirectDrawSurface4_Blt(g_pDDSSpriteCache, &rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT , &ddbltfx );
      if (hRet!=DD_OK)
        fatal_error(hRet, "cgaputim: blt failed");
      ZeroMemory(&ddsd,sizeof(DDSURFACEDESC2));
      ddsd.dwSize=sizeof(DDSURFACEDESC2);
      hRet=IDirectDrawSurface4_Lock(g_pDDSSpriteCache, &rect, &ddsd, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL);
      if (hRet!=DD_OK)
        fatal_error(hRet, "cgaputim: lock failed");
      cur_dest_ptr=(unsigned char *)ddsd.lpSurface;
      dest_next_row_offset = ddsd.lPitch; //-w*4;
    }
    else
    {
#endif
      cur_dest_ptr=&(back_bitmap_bits[(Uint5) (y*2 * scrn_width + x*2)]);
      dest_next_row_offset = scrn_width; // - w*4l;

#ifdef DIRECTX
    }
#endif

    y_loop_end=h;
    x_loop_end=w;

    for (y_loop_count=0;y_loop_count<y_loop_end;y_loop_count++)
    {
      for (x_loop_count=0;x_loop_count<x_loop_end;x_loop_count++)
      {
        for (i=0;i<4;i++)
        {
          if (!( (*cur_src_mask_ptr)&(0xC0>>(i*2)) ))
          {
            color=((*cur_src_ptr)>>(6-(i*2)))&0x03;
#ifdef DIRECTX
            if (use_sprite_cache||cur_dest_ptr < scrn_max_ptr)
#else
            if (cur_dest_ptr < scrn_max_ptr)
#endif
              *cur_dest_ptr = *(cur_dest_ptr+1) = *(cur_dest_ptr+dest_next_row_offset) = *(cur_dest_ptr+dest_next_row_offset+1) = color;


          }
          cur_dest_ptr+=2;
        }
        cur_src_ptr++;
        cur_src_mask_ptr++;
      }
      cur_dest_ptr+=(dest_next_row_offset*2-w*8);
    }

#ifdef DIRECTX
    if (use_sprite_cache)
    {
      IDirectDrawSurface4_Unlock(g_pDDSSpriteCache, &rect);
      if (hRet!=DD_OK)
        fatal_error(hRet, "cgaputim: unlock failed");
      sprite_cached[ch]=TRUE;
    }
  }

  if (use_sprite_cache)
  {
    rect.left = 0;
    rect.right = w*8;
    rect.top = ch* 48;
    if (y*2 + h*2 < 400)
      rect.bottom = rect.top + h*2;
    else
      rect.bottom = rect.top + h*2 - (y*2 + h*2) + 400;
    hRet=IDirectDrawSurface4_BltFast(g_pDDSBack, x*2, y*2, g_pDDSSpriteCache, &rect, DDBLTFAST_WAIT | DDBLTFAST_SRCCOLORKEY);
    if (hRet!=DD_OK)
      fatal_error(hRet, "cgaputim: blt to back surface failed");
    if (!g_bWindowed)
    {
      hRet=IDirectDrawSurface4_BltFast(g_pDDSPrimary, g_rcScreen.left + x*2, g_rcScreen.top + y*2, g_pDDSSpriteCache, &rect, DDBLTFAST_WAIT | DDBLTFAST_SRCCOLORKEY);
      if (hRet!=DD_OK)
        fatal_error(hRet, "cgaputim: blt to screen failed");
    }
    else
    {
      if (y*2 + h*2 < 400)
        blit_rect_to_window(x*2, y*2, w*8, h*2);
      else
        blit_rect_to_window(x*2, y*2, w*8, h*2 - (y*2 + h*2) + 400);
    }
  }
  else
  {
#endif
    if (y*2 + h*2 < 400)
      blit_rect_to_window(x*2, y*2, w*8, h*2);
    else
      blit_rect_to_window(x*2, y*2, w*8, h*2 - (y*2 + h*2) + 400);
#ifdef DIRECTX
  }
#endif
}

void cgageti(Sint4 x,Sint4 y,Uint3 *p,Sint4 w,Sint4 h)
{
  vgageti(x,y,p,w,h);
}

void cgaputi(Sint4 x,Sint4 y,Uint3 *p,Sint4 w,Sint4 h)
{
  vgaputi(x,y,p,w,h);
}

Sint4 cgagetpix(Sint4 x,Sint4 y)
{
  Uint4 xi,yi;
  Sint4 rval;

#ifdef WIN32
  /*GdiFlush();*/
#endif
  rval=0;
  if (x>319||y>199)
  {
    return 0xff;
  }
  for (yi=0;yi<2;yi++)
    for (xi=0;xi<8;xi++)
      if (back_bitmap_bits[(Uint5) ((y*2l+yi)*640l + x*2l + xi)])
        rval |= 0x80 >> xi;

  rval &= 0xee;
  return rval;
}

/*******************************************************/
/* Functions for displaying the CGA data               */
/*******************************************************/
void cgainit(void)
{
  video_mode=VIDEO_MODE_CGA;
  windowed_palette[0] = cga16_pal1_rgbq;
  windowed_palette[1] = cga16_pal1i_rgbq;
  windowed_palette[2] = cga16_pal2_rgbq;
  windowed_palette[3] = cga16_pal2i_rgbq;
  destroy_palettes();
  init_palettes();
}

void cgaclear(void)
{
  vgaclear();
}

void cgapal(Sint4 pal)
{
  vgapal(pal);
}

void cgainten(Sint4 inten)
{
  vgainten(inten);
}

void load_title_bitmaps()
{
#ifdef WIN16
  HRSRC hrsrc;
  HGLOBAL hglob;
  BITMAPINFO far *title_dib;
  unsigned char far* title_bits;
  int i;
  HDC temp_dc;
  HDC temp2_dc;
  HBITMAP old_bitmap;
  BITMAPINFO far* new_bitmap_info;
  char far* data[1000];
  int r;

  temp_dc = GetDC(NULL);
  temp2_dc = CreateCompatibleDC(temp_dc);
  for (i=0;i<1;i++)
  {
    hrsrc=FindResource(g_hInstance,MAKEINTRESOURCE(i==0 ? /*IDB_TITLEBITMAP*/ IDB_VGATITLEBITMAP : IDB_CGATITLEBITMAP),RT_BITMAP);
    hglob=LoadResource(g_hInstance,hrsrc);
    title_dib=(BITMAPINFO far *) LockResource(hglob);
    new_bitmap_info=(char far*) farmalloc(sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD)*16 + title_dib->bmiHeader.biSizeImage);
    farmemcpy(new_bitmap_info, title_dib, sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD)*16 + title_dib->bmiHeader.biSizeImage);
    title_bits=((unsigned char far*) new_bitmap_info) + new_bitmap_info->bmiHeader.biSize + sizeof(RGBQUAD)*16;
    title_bitmap[i]=CreateDIBitmap(temp_dc, (BITMAPINFOHEADER FAR*) new_bitmap_info, CBM_INIT, title_bits, (BITMAPINFO FAR*) new_bitmap_info, DIB_PAL_COLORS);
    r=GetDIBits(temp_dc, title_bitmap[i], 0, 5, data, new_bitmap_info, DIB_PAL_COLORS);
    //title_bits=((unsigned char far*) title_dib) + title_dib->bmiHeader.biSize + sizeof(RGBQUAD)*16;
    //title_bitmap[i]=CreateDIBitmap(temp_dc, (BITMAPINFOHEADER FAR*) title_dib, CBM_INIT, title_bits, (BITMAPINFO FAR*) title_dib, DIB_PAL_COLORS);
    //title_rle_data[i]=(char far*) farmalloc(title_dib->bmiHeader.biSizeImage);
    //farmemcpy(title_rle_data[i], ((char far*) title_dib) + (sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD)*16), title_dib->bmiHeader.biSizeImage);
    UnlockResource(hglob);
    FreeResource(hglob);
  }
  old_bitmap=SelectObject(temp2_dc,title_bitmap[0]);
  BitBlt(temp_dc,0,0,640,400,temp2_dc, 0, 0, SRCCOPY);
  ReleaseDC(NULL,temp_dc);
  DeleteDC(temp2_dc);
#else
  title_bitmap[0]=LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_TITLEBITMAP));
  title_bitmap[1]=LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_CGATITLEBITMAP));
#endif
}

