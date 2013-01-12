  %include "../defaults_bin.asm"

  cli
  ; Mode                                                2c  28  2d  29  2a  2e  1a  29
  ;      1 +HRES                                         0   0   1   1   0   0   0   1
  ;      2 +GRPH                                         0   0   0   0   2   2   2   0
  ;      4 +BW                                           4   0   4   0   0   4   0   0
  ;      8 +VIDEO ENABLE                                 8   8   8   8   8   8   8   8
  ;   0x10 +1BPP                                         0   0   0   0   0   0  10   0
  ;   0x20 +ENABLE BLINK                                20  20  20  20  20  20   0  20
  mov dx,0x03d8
  mov al,0x0b
  out dx,al

  inc dx
  mov al,0
  out dx,al

  mov dl,0xd4

  ;   0xff Horizontal Total
  mov ax,0x7200
  out dx,ax

  ;   0xff Horizontal Displayed
  mov ax,0x5001
  out dx,ax

  ;   0xff Horizontal Sync Position
  mov ax,0x5a02
  out dx,ax

  ;   0x0f Horizontal Sync Width
  mov ax,0x0f03
  out dx,ax

  ;   0x7f Vertical Total
  mov ax,0x7f04
  out dx,ax

  ;   0x1f Vertical Total Adjust
  mov ax,0x0505
  out dx,ax

  ;   0x7f Vertical Displayed
  mov ax,0x6406
  out dx,ax

  ;   0x7f Vertical Sync Position
  mov ax,0x7007
  out dx,ax

  ;   0x03 Interlace Mode
  mov ax,0x0208
  out dx,ax

  ;   0x1f Max Scan Line Address
  mov ax,0x0109
  out dx,ax

  ; Cursor Start
  ;   0x1f Cursor Start
  ;   0x60 Cursor Mode
  mov ax,0x200a
  out dx,ax

  ;   0x1f Cursor End
  mov ax,0x000b
  out dx,ax

  ;   0x3f Start Address (H)
  mov ax,0x000c
  out dx,ax

  ;   0xff Start Address (L)
  mov ax,0x000d
  out dx,ax


  mov ax,0xb800
  mov es,ax
  xor di,di
  cld
  mov al,0
  mov cx,0x4000
fillLoop:
  stosb
  inc al
  loop fillLoop

  hlt
