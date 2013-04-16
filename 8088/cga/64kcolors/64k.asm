org 0x100   ; 0 for XT Server, 0x100 for .com
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


; Scanline   0 = row  0 of  2-line screen (Address 0 = blank)
; Scanline   1 = row  1 of  2-line screen (Address 160 = pixel line 0)
; Scanline   2 = row  0 of  2-line screen (Address 160 = pixel line 0)
; Scanline   3 = row  1 of  2-line screen (Address 320 = pixel line 1)
; ...
; Scanline 198 = row  0 of  2-line screen (Address 15840 = pixel line 98)
; Scanline 199 = row  1 of  2-line screen (Address 16000 = pixel line 99)
; Scanline 200 = row  0 of 62-line screen (Address 16000 = pixel line 99)
; Scanline 201 = row  1 of 62-line screen (Address 16160 = blank)
; ...
; Scanline 224 = row 24 of 62-line screen - sync start
; ...
; Scanline 261 = row 61 of 62-line screen


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
  mov al,6
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
  mov ax,0x0d03
  out dx,ax

  ;   0x7f Vertical Total                               3f
  mov ax,0x3f04
  out dx,ax

  ;   0x1f Vertical Total Adjust                        06
  mov ax,0x0605
  out dx,ax

  ;   0x7f Vertical Displayed                           28
  mov ax,0x2806
  out dx,ax

  ;   0x7f Vertical Sync Position                       38
  mov ax,0x3807
  out dx,ax

  ;   0x03 Interlace Mode                               02
  mov ax,0x0208
  out dx,ax

  ;   0x1f Max Scan Line Address                        03
  mov ax,0x0309
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


  mov cx,1200      ; Frame counter
frameLoop:
  mov dl,0xda
  waitForVerticalSync
  waitForNoVerticalSync

  mov dl,0xd4
  ;   0x3f Start Address (H)                            00
  mov ax,0x0f0c
  out dx,ax

  ;   0xff Start Address (L)                            00
  mov ax,0xa00d
  out dx,ax

  mov dl,0xda
  waitForVerticalSync
  waitForNoVerticalSync

  mov dl,0xd4
  ;   0x3f Start Address (H)                            00
  mov ax,0x000c
  out dx,ax

  ;   0xff Start Address (L)                            00
  mov ax,0x000d
  out dx,ax

  loop frameLoop

  ; Set the CGA back to a normal mode so we don't risk breaking anything
  mov ax,3
  int 0x10

  ; Relinquish control
  ret

data:

