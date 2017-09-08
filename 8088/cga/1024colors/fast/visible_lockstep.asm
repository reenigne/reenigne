  %include "../../../defaults_bin.asm"

;  captureScreen
;  captureScreen
;  captureScreen

  mov ax,0xb800
  mov es,ax
  xor ax,ax
  xor di,di
  stosw

  mov dx,0x03d8
  mov al,9
  out dx,al

  inc dx
  mov al,0x0f
  out dx,al

  ; Set up CRTC for 1 character by 2 scanline "frame". This gives us 2 lchars
  ; per frame.
  mov dx,0x3d4
  ;   0xff Horizontal Total
  mov ax,0x0100
  out dx,ax
  ;   0xff Horizontal Displayed                         28
  mov ax,0x0101
  out dx,ax
  ;   0xff Horizontal Sync Position                     2d
  mov ax,0x2d02
  out dx,ax
  ;   0x0f Horizontal Sync Width                        0a
  mov ax,0x0a03
  out dx,ax
  ;   0x7f Vertical Total                               7f
  mov ax,0x0104
  out dx,ax
  ;   0x1f Vertical Total Adjust                        06
  mov ax,0x0005
  out dx,ax
  ;   0x7f Vertical Displayed                           64
  mov ax,0x0106
  out dx,ax
  ;   0x7f Vertical Sync Position                       70
  mov ax,0x0007
  out dx,ax
  ;   0x03 Interlace Mode                               02
  mov ax,0x0208
  out dx,ax
  ;   0x1f Max Scan Line Address                        01
  mov ax,0x0009
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

  safeRefreshOff
  mov dx,0x3da
  mov di,data
  mov ax,cs
  mov es,ax

%rep 32
  in al,dx
  stosb
%endrep

  safeRefreshOn
  sti

  mov si,data
  mov ax,cs
  mov ds,ax
%rep 32
  lodsb
  push si
  and al,0x0f
  add al,'0'
  outputCharacter
  pop si
%endrep

  ; c  1100  <
  ; d  1101  =

;<<<<<<<=
;========
;========
;========

;11111110 00000000 00000000 00000000


  complete

  hlt
  cli

data:
