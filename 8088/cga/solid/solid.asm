  %include "../../defaults_bin.asm"

  ; Mode
  ;      1 +HRES
  ;      2 +GRPH
  ;      4 +BW
  ;      8 +VIDEO ENABLE
  ;   0x10 +1BPP
  ;   0x20 +ENABLE BLINK
  mov dx,0x3d8
  mov al,2
  out dx,al

  ; Palette
  ;      1 +OVERSCAN B
  ;      2 +OVERSCAN G
  ;      4 +OVERSCAN R
  ;      8 +OVERSCAN I
  ;   0x10 +BACKGROUND I
  ;   0x20 +COLOR SEL
  inc dx
  mov al,6
  out dx,al

  mov dl,0xd4

  ;   0xff Horizontal Total                             38 71
  mov ax,0x2800
  out dx,ax

  ;   0xff Horizontal Displayed                         28 50
  mov ax,0x2801
  out dx,ax

  ;   0xff Horizontal Sync Position                     2d 5a
  mov ax,0x2d02
  out dx,ax

  ;   0x0f Horizontal Sync Width                              0a
  mov ax,0x0a03
  out dx,ax

  ;   0x7f Vertical Total                                        1f 7f
  mov ax,0x6404
  out dx,ax

  ;   0x1f Vertical Total Adjust                              06
  mov ax,0x0605
  out dx,ax

  ;   0x7f Vertical Displayed                                    19 64
  mov ax,0x6406
  out dx,ax

  ;   0x7f Vertical Sync Position                                1c 70
  mov ax,0x7007
  out dx,ax

  ;   0x03 Interlace Mode                                     02
  mov ax,0x0208
  out dx,ax

  ;   0x1f Max Scan Line Address                                 07 01
  mov ax,0x0109
  out dx,ax

  ; Cursor Start                                              06
  ;   0x1f Cursor Start                                        6
  ;   0x60 Cursor Mode                                         0
  mov ax,0x060a
  out dx,ax

  ;   0x1f Cursor End                                         07
  mov ax,0x070b
  out dx,ax

  ;   0x3f Start Address (H)                                  00
  mov ax,0x000c
  out dx,ax

  ;   0xff Start Address (L)                                  00
  inc ax
  out dx,ax

  ;   0x3f Cursor (H)                                         03  0x3c0 == 40*24 == start of last line
  mov ax,0x030e
  out dx,ax

  ;   0xff Cursor (L)                                         c0
  mov ax,0xc00f
  out dx,ax


  hlt

