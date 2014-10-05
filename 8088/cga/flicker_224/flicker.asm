org 0x100  ; 0 for XT Server, 0x100 for .com
cpu 8086

  cli

  ; Copy data
  mov ax,cs
  mov ds,ax
  mov ax,0xb800
  mov es,ax
  mov cx,8000
  mov si,data
  xor di,di
  cld
  rep movsw


  ; Mode                                                09
  ;      1 +HRES                                         1
  ;      2 +GRPH                                         0
  ;      4 +BW                                           0
  ;      8 +VIDEO ENABLE                                 8
  ;   0x10 +1BPP                                         0
  ;   0x20 +ENABLE BLINK                                 0
  mov dx,0x3d8
  mov al,0x08
  out dx,al

  ; Palette                                             00
  ;      1 +OVERSCAN B                                   0
  ;      2 +OVERSCAN G                                   2
  ;      4 +OVERSCAN R                                   4
  ;      8 +OVERSCAN I                                   0
  ;   0x10 +BACKGROUND I                                 0
  ;   0x20 +COLOR SEL                                    0
  inc dx
  mov al,0
  out dx,al

  mov dl,0xd4

  ;   0xff Horizontal Total                             38
  mov ax,0x3800
  out dx,ax

  ;   0xff Horizontal Displayed                         1c  (28 characters)
  mov ax,0x1c01
  out dx,ax

  ;   0xff Horizontal Sync Position                     27  (0x2d - (40 - 28)/2)
  mov ax,0x2702
  out dx,ax

  ;   0x0f Horizontal Sync Width                        0d
  mov ax,0x0d03
  out dx,ax

  ;   0x7f Vertical Total                               7e
  mov ax,0x7e04
  out dx,ax

  ;   0x1f Vertical Total Adjust                        00
  mov ax,0x0005
  out dx,ax

  ;   0x7f Vertical Displayed                           7f
  mov ax,0x7f06
  out dx,ax

  ;   0x7f Vertical Sync Position                       6a
  mov ax,0x6a07
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

  ;   0x3f Cursor (H)                                   03
  mov ax,0x030e
  out dx,ax

  ;   0xff Cursor (L)                                   c0
  mov ax,0xc00f
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


  ; First CRTC frame (127 total, 127 active)
  ;   Changes:
  ;     Set start position to right one 127*28 = 0xbe8 or (140+127)*28 = 0x1d34
  ;     Set Vertical Total Adjust to 0
  ;     Set Vertical Sync Position to 127
  ;     Set Vertical Displayed to 127
  ; Second CRTC frame (135 total, 13 active, 8 adjust, 67 before vsync)
  ;   Start position = 127*24 = 0xbe8
  ;   Scanlines = 135 (adjust = 8)
  ;   Changes:
  ;     Set start position to right one 140*28 = 0xf50 or 0
  ;     Set Vertical Total Adjust to 8
  ;     Set Vertical Sync Position to 67
  ;     Set Vertical Displayed to 13
  ; Absolute vertical sync position = 224 - (200 - 140)/2 = 194

  ; Scanlines 0..126 = first CRTC frame (127 active)
  ; Scanlines 127..139 = second CRTC frame (13 active, 135 total)
  ; Scanlines 140..194 = bottom border
  ; Scanlines 194..209 = vertical sync from CRTC
  ; Scanlines 210..253 = top border
  ; Scanlines 254..261 = adjust


  mov cx,1200      ; Frame counter
frameLoop:
  mov dl,0xda
  waitForVerticalSync
  waitForNoVerticalSync


  waitForDisplayEnable
  mov dl,0xd4
  mov ax,0x0d0c            ; 0xde4 = 28*127
  out dx,ax
  mov dl,0xda
  waitForDisplayDisable

  waitForDisplayEnable
  mov dl,0xd4
  mov ax,0xe40d
  out dx,ax
  mov dl,0xda
  waitForDisplayDisable

  waitForDisplayEnable
  mov dl,0xd4
  mov ax,0x0005
  out dx,ax
  mov dl,0xda
  waitForDisplayDisable

  waitForDisplayEnable
  mov dl,0xd4
  mov ax,0x7f06
  out dx,ax
  mov dl,0xda
  waitForDisplayDisable

  waitForDisplayEnable
  mov dl,0xd4
  mov ax,0x7f07
  out dx,ax
  mov dl,0xda
  waitForDisplayDisable

  %rep 122
    waitForDisplayEnable
    waitForDisplayDisable
  %endrep


  waitForDisplayEnable
  mov dl,0xd4
  mov ax,0x0f0c            ; 0xf50 = 28*140
  out dx,ax
  mov dl,0xda
  waitForDisplayDisable

  waitForDisplayEnable
  mov dl,0xd4
  mov ax,0x500d
  out dx,ax
  mov dl,0xda
  waitForDisplayDisable

  waitForDisplayEnable
  mov dl,0xd4
  mov ax,0x0805
  out dx,ax
  mov dl,0xda
  waitForDisplayDisable

  waitForDisplayEnable
  mov dl,0xd4
  mov ax,0x0d06
  out dx,ax
  mov dl,0xda
  waitForDisplayDisable

  waitForDisplayEnable
  mov dl,0xd4
  mov ax,0x4307
  out dx,ax
  mov dl,0xda
  waitForDisplayDisable

  waitForVerticalSync
  waitForNoVerticalSync


  waitForDisplayEnable
  mov dl,0xd4
  mov ax,0x1d0c            ; 0x1d34 = 28*(127+140)
  out dx,ax
  mov dl,0xda
  waitForDisplayDisable

  waitForDisplayEnable
  mov dl,0xd4
  mov ax,0x340d
  out dx,ax
  mov dl,0xda
  waitForDisplayDisable

  waitForDisplayEnable
  mov dl,0xd4
  mov ax,0x0005
  out dx,ax
  mov dl,0xda
  waitForDisplayDisable

  waitForDisplayEnable
  mov dl,0xd4
  mov ax,0x7f06
  out dx,ax
  mov dl,0xda
  waitForDisplayDisable

  waitForDisplayEnable
  mov dl,0xd4
  mov ax,0x7f07
  out dx,ax
  mov dl,0xda
  waitForDisplayDisable

  %rep 122
    waitForDisplayEnable
    waitForDisplayDisable
  %endrep


  waitForDisplayEnable
  mov dl,0xd4
  mov ax,0x000c            ; 0x0000
  out dx,ax
  mov dl,0xda
  waitForDisplayDisable

  waitForDisplayEnable
  mov dl,0xd4
  mov ax,0x000d
  out dx,ax
  mov dl,0xda
  waitForDisplayDisable

  waitForDisplayEnable
  mov dl,0xd4
  mov ax,0x0805
  out dx,ax
  mov dl,0xda
  waitForDisplayDisable

  waitForDisplayEnable
  mov dl,0xd4
  mov ax,0x0d06
  out dx,ax
  mov dl,0xda
  waitForDisplayDisable

  waitForDisplayEnable
  mov dl,0xd4
  mov ax,0x4307
  out dx,ax
  mov dl,0xda
  waitForDisplayDisable


  loop frameLoop1

  ; Set the CGA back to a normal mode so we don't risk breaking anything
  mov ax,3
  int 0x10

  ; Relinquish control
  retf

frameLoop1:
  jmp frameLoop

complete:

data:
;  %rep 140
;    dw 0x1100, 0x2200, 0x3300, 0x4400, 0x5500, 0x6600, 0x7700, 0x8800, 0x9900, 0xaa00, 0xbb00, 0xcc00, 0xdd00, 0xee00, 0xff00, 0x0000, 0x1100, 0x2200, 0x3300, 0x4400, 0x5500, 0x6600, 0x7700, 0x8800, 0x9900, 0xaa00, 0xbb00, 0xcc00
;  %endrep
;  times 8*28 dw 0x1100
;  times 8*28 dw 0x2200
;  times 8*28 dw 0x3300
;  times 8*28 dw 0x4400
;  times 8*28 dw 0x5500
;  times 8*28 dw 0x6600
;  times 8*28 dw 0x7700
;  times 8*28 dw 0x8800
;  times 8*28 dw 0x9900
;  times 8*28 dw 0xaa00
;  times 8*28 dw 0xbb00
;  times 8*28 dw 0xcc00
;  times 8*28 dw 0xdd00
;  times 8*28 dw 0xee00
;  times 8*28 dw 0xff00
;  times 8*28 dw 0x0000
;  times 8*28 dw 0x1100
;  times 4*28 dw 0x2200

