org 0x100
cpu 8086

demoAPI EQU 0F8h

  ; Determine if loader is present, and abort if not

  ; First, check to see if the API is even present.
  ; If not, don't try to call anything since it will hang the system.
  xor bx,bx
  mov es,bx
  mov di,(demoAPI * 4)+2      ;check to see if our INT is empty
  cmp [word es:di],bx         ;int. vector empty?
  je  exitShort               ;abort if so
  mov ax,0700h                ;check int. vector to see if it's ours
  int demoAPI
  cmp ax,0C0DEh               ;magic cookie received?
  jne exitShort               ;abort if not
  jmp mstart
exitShort:
  jmp exitEffect
mstart:


; Move vsync start up 50 scanlines (25 rows) to scanline 174 = row 37

; Active region:
; Scanline   0 = row   0 scanline 0 of 100-line screen
; Scanline   1 = row   1 scanline 0 of 100-line screen
; ...

; Overscan region:
; Scanline 100 = row   0 scanline 0 of 162-line screen
; Scanline 101 = row   0 scanline 1 of 162-line screen = last visible
; Scanline 102 = row   1 scanline 0 of 162-line screen
; Scanline 103 = row   1 scanline 1 of 162-line screen
; ...
; Scanline 126 = row 126 scanline 0 of 162-line screen
; Scanline 127 = row   0 scanline 0 of 162-line screen

; Common:  0x7100, 0x5001, 0x5a02, 0x0003, 0x0005, 0x0208, 0x060a, 0x070b, 0x030a, 0xc00f
; Active region: 0x6304, 0x6406, 0x7f07, 0x0009, 0x1f0c, 0x400d
; Overscan region: 0x5004, 0x0106, 0x2507, 0x0109, 0x000c, 0x000d


  ; Ready to start; tell API we are waiting, then proceed when
  ; we get the signal.
  mov ah,01                   ;tell loader we are ready to start
  int demoAPI
  mov ah,02                   ;wait for signal to proceed
  stc
waitLoader:
  int demoAPI
  jnc waitLoader              ;if carry not set, don't start yet


  ; Copy data
  mov ax,cs
  mov ds,ax
  mov ax,0xb800
  mov es,ax
  mov cx,8160
  mov si,data
  xor di,di
  cld
  rep movsw
  xor ax,ax
  stosw

  ; Mode                                                09
  ;      1 +HRES                                         1
  ;      2 +GRPH                                         0
  ;      4 +BW                                           0
  ;      8 +VIDEO ENABLE                                 8
  ;   0x10 +1BPP                                         0
  ;   0x20 +ENABLE BLINK                                 0
  mov dx,0x3d8
  mov al,0x09
  out dx,al

  ; Palette                                             00
  ;      1 +OVERSCAN B                                   0
  ;      2 +OVERSCAN G                                   2
  ;      4 +OVERSCAN R                                   4
  ;      8 +OVERSCAN I                                   0
  ;   0x10 +BACKGROUND I                                 0
  ;   0x20 +COLOR SEL                                    0
  inc dx
  mov al,0x00
  out dx,al

  mov dl,0xd4

  ;   0xff Horizontal Total                             71
  mov ax,0x7100
  out dx,ax

  ;   0xff Horizontal Displayed                         50
  mov ax,0x5001
  out dx,ax

  ;   0xff Horizontal Sync Position                     5a
  mov ax,0x5a02
  out dx,ax

  ;   0x0f Horizontal Sync Width                        0d
  mov ax,0x0003
  out dx,ax

  ;   0x7f Vertical Total                               3d
  mov ax,0x6304
  out dx,ax

  ;   0x1f Vertical Total Adjust                        00
  mov ax,0x0005
  out dx,ax

  ;   0x7f Vertical Displayed                           02
  mov ax,0x6406
  out dx,ax

  ;   0x7f Vertical Sync Position                       18
  mov ax,0x7f07
  out dx,ax

  ;   0x03 Interlace Mode                               02
  mov ax,0x0208
  out dx,ax

  ;   0x1f Max Scan Line Address                        00
  mov ax,0x0009
  out dx,ax

  ; Cursor Start                                        06
  ;   0x1f Cursor Start                                  6
  ;   0x60 Cursor Mode                                   0
  mov ax,0x000a
  out dx,ax

  ;   0x1f Cursor End                                   07
  mov ax,0x000b
  out dx,ax

  ;   0x3f Start Address (H)                            00
  mov ax,0x000c
  out dx,ax

  ;   0xff Start Address (L)                            00
  mov ax,0x000d
  out dx,ax

  ;   0x3f Cursor (H)                                   03
  mov ax,0x1f0e
  out dx,ax

  ;   0xff Cursor (L)                                   c0
  mov ax,0xff0f
  out dx,ax


%macro waitForDisplayEnable 0
  %%waitForDisplayEnable
    in al,dx                       ; 1 1 2
    test al,1                      ; 2 0 2
    jnz %%waitForDisplayEnable     ; 2 0 2
%endmacro

%macro waitForDisplayDisable 0
  %%waitForDisplayDisable
    in al,dx                       ; 1 1 2
    test al,1                      ; 2 0 2
    jz %%waitForDisplayDisable     ; 2 0 2
%endmacro

%macro waitForVerticalSync 0
  %%waitForVerticalSync
    in al,dx
    test al,8
    jz %%waitForVerticalSync
%endmacro

%macro waitForNoVerticalSync 0
  %%waitForNoVerticalSync
    in al,dx
    test al,8
    jnz %%waitForNoVerticalSync
%endmacro

  cli


  mov dl,0xda
  mov bx,80     ; Initial
  mov cx,0      ; Frame counter
frameLoop:

  ; Scanline 0
  waitForDisplayEnable
; Active region: 0x6304, 0x6406, 0x7f07, 0x0009, 0x1f0c, 0x400d
  mov dl,0xd4
  mov ax,0x0009
  out dx,ax
  mov ax,0x6304
  out dx,ax
  mov ax,0x6406
  out dx,ax
  mov ax,0x7f07
  out dx,ax
  mov ax,0x1f0c
  out dx,ax
  mov ax,0x400d
  out dx,ax
  mov dl,0xda
  waitForDisplayDisable

  ; Scanlines 1-99
%rep 99
  waitForDisplayEnable
  waitForDisplayDisable
%endrep

  ; Scanline 100
  waitForDisplayEnable
; Overscan region: 0x5004, 0x0106, 0x2507, 0x0109, 0x000c, 0x000d
  mov dl,0xd4
  mov ax,0x0109
  out dx,ax
  mov ax,0x0106
  out dx,ax
  mov ax,0x2507
  out dx,ax
  mov ax,0x5004
  out dx,ax
  mov ax,0x000c
  out dx,ax
  inc ax
  out dx,ax
  mov dl,0xda
  waitForDisplayDisable

  waitForVerticalSync
  waitForNoVerticalSync

  push dx
  mov ah,0
  int demoAPI
  mov ah,2
  int demoAPI
  jnc exitEffect
  pop dx

  jmp frameLoop
exitEffect:
  mov ax,0x4c00
  int 0x21
data:

