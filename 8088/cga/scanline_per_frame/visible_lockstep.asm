  %include "../../defaults_bin.asm"

;  captureScreen
;  captureScreen
;  captureScreen

  mov ax,0xb800
  mov es,ax
  xor ax,ax
  xor di,di
  stosw

  mov dx,0x03d8
  mov al,0
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
  mov ax,0x1d07
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

%rep 64
  in al,dx        ; Each iteration of this is 21 cycles = 63 hdots
  stosb
%endrep

  safeRefreshOn
  sti

  mov si,data
  mov ax,cs
  mov ds,ax
%rep 64
  lodsb
  push si
  and al,0x0f
  add al,'0'
  outputCharacter
  pop si
%endrep

; +HRES

  ; c  1100  <
  ; d  1101  =

;<<<<<<<=
;========
;========
;========

;11111110 00000000 00000000 00000000

; -HRES

;================
;================
;==<<<<<<<<<<<<<<
;<<==============

; Want a loop that looks like this:
; top:
;   delay N cycles
;   in al,dx
;   test al,1
;   jnz top
; such that the total length of the loop is 64*N - 16 or 64*N + 16 hdots for some integer N
;   64*N - 16 => cycles = (64*N - 16)/3 => N must be 1 more than a multiple of 3
;   64*N + 16 => cycles = (64*N + 16)/3 => N must be 2 more than a multiple of 3
;   hdots is 48 + N*192 or 144 + N*192
;   cycles is 16 + N*64 or 48 + N*64
; cycles = 16, 48, 80, 112, 144, ...   16 + 32*N



;  complete

  cli
  hlt

data:
