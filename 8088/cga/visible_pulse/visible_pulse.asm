  cpu 8086
  org 0

  ; Mode                                                 8
  ;      1 +HRES                                         0
  ;      2 +GRPH                                         0
  ;      4 +BW                                           0
  ;      8 +VIDEO ENABLE                                 8
  ;   0x10 +1BPP                                         0
  ;   0x20 +ENABLE BLINK                                 0
  mov dx,0x3d8
  mov al,8
  out dx,al

  ; Palette                                             06
  ;      1 +OVERSCAN B                                   0
  ;      2 +OVERSCAN G                                   2
  ;      4 +OVERSCAN R                                   4
  ;      8 +OVERSCAN I                                   0
  ;   0x10 +BACKGROUND I                                 0
  ;   0x20 +COLOR SEL                                    0
  mov dx,0x3d9
  mov al,6
  out dx,al

  mov dx,0x3d4

  ;   0xff Horizontal Total                             1b
  mov ax,0x1b00
  out dx,ax

  ;   0xff Horizontal Displayed                         0b
  mov ax,0x0b01
  out dx,ax

  ;   0xff Horizontal Sync Position                     2d
  mov ax,0x2d02
  out dx,ax

  ;   0x0f Horizontal Sync Width                        0a
  mov ax,0x0a03
  out dx,ax

  ;   0x7f Vertical Total                               1f
  mov ax,0x1f04
  out dx,ax

  ;   0x1f Vertical Total Adjust                        06
  mov ax,0x0605
  out dx,ax

  ;   0x7f Vertical Displayed                           19
  mov ax,0x1906
  out dx,ax

  ;   0x7f Vertical Sync Position                       1c
  mov ax,0x1c07
  out dx,ax

  ;   0x03 Interlace Mode                               02
  mov ax,0x0208
  out dx,ax

  ;   0x1f Max Scan Line Address                        07
  mov ax,0x0709
  out dx,ax

  ; Cursor Start                                        06
  ;   0x1f Cursor Start                                  6
  ;   0x60 Cursor Mode                                   0
  mov ax,0x060a
  out dx,ax

  ;   0x1f Cursor End                                   07
  mov ax,0x070b
  out dx,ax

  ;   0x3f Start Address (H)                            00
  mov ax,0x000c
  out dx,ax

  ;   0xff Start Address (L)                            00
  mov ax,0x000d
  out dx,ax

  ;   0x3f Cursor (H)                                   03  0x3c0 == 40*24 == start of last line
  mov ax,0x030e
  out dx,ax

  ;   0xff Cursor (L)                                   c0
  mov ax,0xc00f
  out dx,ax




low-res timings with +HRES (1 left blank assumption):

40 hchars visible (black)
 5 hchars overscan
 2 hchars blank
 8 hchars sync
 7 hchars hchars overscan

2 left blanks assumption:

40 hchars visible (black)
 5 hchars overscan
 4 hchars blank
 6 hchars sync
 7 hchars hchars overscan


