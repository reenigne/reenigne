/* Digger Remastered
   Copyright (c) Andrew Jenner 1998-2004 */

#include <windows.h>
#ifndef WIN32
#include <memory.h>
#endif
#define DIGGER_WS_WINDOWED WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_BORDER
#define DIGGER_WS_FULLSCREEN 0
#define SAVE TRUE
#define OPEN FALSE


#ifdef _MSVC
#ifdef LIBC               /* linking with static library LIBC.LIB */
#define strupr _strupr
#define strnicmp _strnicmp
#define stricmp _stricmp
#define itoa _itoa
#endif
#endif

#ifndef WIN32
#define LPCTSTR LPCSTR
#define HRESULT int
#endif

extern HWND hWnd;
extern SIZE window_size;
extern HINSTANCE g_hInstance;
extern bool shutting_down;

void do_windows_events(void);
void windows_finish(void);
void refresh_menu_items(void);
void show_game_menu(void);
void show_main_menu(void);
void remove_menu(void);

void pause_windows_sound_playback(void);
void resume_windows_sound_playback(void);
void init_joystick();

int do_dialog_box(HINSTANCE hInstance, LPCTSTR lpTemplate, HWND hWndParent,
                  DLGPROC lpDialogFunc);
bool get_open_save_filename(bool save, char* title, char* filter, char* defext, char filename[]);
HRESULT fatal_error(HRESULT hRet, LPCTSTR szError);
void load_level_file(char* fn);
void restore_original_level_data();

extern bool use_performance_counter;
#ifdef WIN32
extern _int64 performance_frequency;
#endif
extern void windows_init_sound();

#ifdef RUNTIMEDYNAMICLINK
void init_direct_x();
void release_direct_x();
extern bool check_for_direct_x;
#endif

extern HINSTANCE g_hInstance;
extern BOOL g_bActive;
extern bool reset_main_menu_screen;
extern char drf_filename[FILENAME_BUFFER_SIZE];
