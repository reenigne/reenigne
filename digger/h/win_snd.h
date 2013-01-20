/* Digger Remastered
   Copyright (c) Andrew Jenner 1998-2004 */

#include <mmsystem.h>
#ifdef DIRECTX
#include <dsound.h>
extern LPDIRECTSOUND lpds;           /* address of direct sound object */
extern LPDIRECTSOUNDBUFFER lpdsb;    /* address of secondary direct sound buffer */
extern bool use_direct_sound;
#endif

extern LONG dx_sound_volume;
extern bool wave_device_available;
extern int sound_card_capture_flag;

BOOL create_windows_sound_buffer(Uint4 samprate, Uint4 bufsize);
LONG get_sound_volume();
void set_sound_volume(LONG new_volume);
void change_sound_volume(bool louder);
void pause_windows_sound_playback();
void resume_windows_sound_playback();
DWORD get_sound_freq();
void unprepare_sound_data(WAVEHDR*);
void play_sound_data(WAVEHDR* pwhdr);
void destroy_sound_buffers();
void waveOut_fillbuffer(WAVEHDR* pwhdr);
void capture_sound_card();
void release_sound_card();

extern void change_sound_volume(bool louder);
extern LONG get_sound_volume();
extern void set_sound_volume(LONG);
extern DWORD get_sound_freq();
extern Uint4 sound_rate,sound_length;


#ifdef RUNTIMEDYNAMICLINK
extern HINSTANCE hDirectSoundInstance;
extern HRESULT (WINAPI *lpDirectSoundCreate)(LPGUID, LPDIRECTSOUND *, LPUNKNOWN);
#endif
