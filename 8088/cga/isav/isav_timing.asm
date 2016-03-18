  %include "../../defaults_bin.asm"

  mov al,0x34
  out 0x43,al
  mov al,0
  out 0x40,al
  out 0x40,al

  ; Mode                                                09
  ;      1 +HRES                                         1
  ;      2 +GRPH                                         0
  ;      4 +BW                                           0
  ;      8 +VIDEO ENABLE                                 8
  ;   0x10 +1BPP                                         0
  ;   0x20 +ENABLE BLINK                                 0
  mov dx,0x3d8
  mov al,0x0a  ; 0x1a
  out dx,al

  ; Palette                                             00
  ;      1 +OVERSCAN B                                   0
  ;      2 +OVERSCAN G                                   2
  ;      4 +OVERSCAN R                                   4
  ;      8 +OVERSCAN I                                   0
  ;   0x10 +BACKGROUND I                                 0
  ;   0x20 +COLOR SEL                                    0
  inc dx
  mov al,0x0f
  out dx,al

  mov dl,0xd4

  ;   0xff Horizontal Total                             71
  mov ax,0x3800
  out dx,ax

  ;   0xff Horizontal Displayed                         50
  mov ax,0x2801
  out dx,ax

  ;   0xff Horizontal Sync Position                     5a
  mov ax,0x2d02
  out dx,ax

  ;   0x0f Horizontal Sync Width                        0d
  mov ax,0x0a03
  out dx,ax

  ;   0x7f Vertical Total                               3d
  mov ax,0x4004
  out dx,ax

  ;   0x1f Vertical Total Adjust                        00
  mov ax,0x0205
  out dx,ax

  ;   0x7f Vertical Displayed                           02
  mov ax,0x3206
  out dx,ax

  ;   0x7f Vertical Sync Position                       18
  mov ax,0x3807
  out dx,ax

  ;   0x03 Interlace Mode                               02   0 = non interlaced, 1 = interlace sync, 3 = interlace sync and video
  mov ax,0x0308
  out dx,ax

  ;   0x1f Max Scan Line Address                        00
  mov ax,0x0309
  out dx,ax

  ; Cursor Start                                        06
  ;   0x1f Cursor Start                                  6
  ;   0x60 Cursor Mode                                   0
  mov ax,0x060a
  out dx,ax

  ;   0x1f Cursor End                                   07
  mov ax,0x080b
  out dx,ax

  ;   0x3f Start Address (H)                            00
  mov ax,0x000c
  out dx,ax

  ;   0xff Start Address (L)                            00
  mov ax,0x000d
  out dx,ax

  ;   0x3f Cursor (H)                                   03
  mov ax,0x030e
  out dx,ax

  ;   0xff Cursor (L)                                   c0
  mov ax,0xc00f
  out dx,ax


  mov cx,25
  mov dx,0x3da
  mov ax,cs
  mov es,ax
  mov ds,ax
  mov di,data
loopTop:
  waitForVerticalSync
  waitForDisplayEnable
  mov al,0x04
  out 0x43,al
  in al,0x40
  mov ah,al
  in al,0x40
  xchg ah,al
  stosw
  loop loopTop

  mov cx,24
  mov si,data
  lodsw
loopTop2:
  mov bx,ax

  lodsw
  sub bx,ax
  push ax

  mov ax,bx
  xor dx,dx
  mov bx,76
  div bx
  push dx
  outputHex
  outputCharacter ' '
  pop dx
  mov ax,dx
  outputHex
  outputCharacter 10

  pop ax
  loop loopTop2
  complete
data:
