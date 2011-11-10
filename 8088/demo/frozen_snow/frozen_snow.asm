org 0
cpu 8086

  ; Disable interrupts and turn off refresh
  cli
  mov al,0x60  ; Timer 1, write LSB, mode 0, binary
  out 0x43,al
  mov al,0x01  ; Count = 0x0001 so we'll stop almost immediately
  out 0x41,al

  ; Set "stosb" destination to be CGA memory
  mov ax,0xb800
  mov es,ax
  mov ds,ax
  xor di,di

  ; Set argument for MUL
  mov cl,1

  ; Ensure "stosb" won't take us out of video memory
  cld

  ; Go into CGA lockstep. The delays were determined by trial and error.
  jmp $+2      ; Clear prefetch queue
  stosb        ; From 16 down to 3 possible CGA/CPU relative phases.
  mov al,0x01
  mul cl
  stosb        ; Down to 2 possible CGA/CPU relative phases.
  mov al,0x7f
  mul cl
  stosb        ; Down to 1 possible CGA/CPU relative phase: lockstep achieved.

  mov dx,0x03d8
  mov al,0x0a
  out dx,al

  ; Set up CRTC for 1 character by 2 scanline "frame". This gives us 2 lchars
  ; per frame.
  mov dl,0xd4
  ;   0xff Horizontal Total
  mov ax,0x0000
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

  times 512 nop
  nop

  ; To get the CRTC into lockstep with the CGA and CPU, we need to figure out
  ; which of the two possible CRTC states we're in and switch states if we're
  ; in the wrong one by waiting for an odd number of lchars more in one code
  ; path than in the other. To keep CGA and CPU in lockstep, we also need both
  ; code paths to take the same time mod 3 lchars, so we wait 3 lchars more on
  ; one code path than on the other.
  mov dl,0xda
  in al,dx
  jmp $+2
  test al,1
  jz shortPath
  times 2 nop
  jmp $+2
shortPath:

  ; Mode                                                09
  ;      1 +HRES                                         1
  ;      2 +GRPH                                         0
  ;      4 +BW                                           0
  ;      8 +VIDEO ENABLE                                 8
  ;   0x10 +1BPP                                         0
  ;   0x20 +ENABLE BLINK                                 0
  mov dl,0xd8
  mov al,0x09
  out dx,al

  ; Palette                                             00
  ;      1 +OVERSCAN B                                   0
  ;      2 +OVERSCAN G                                   0
  ;      4 +OVERSCAN R                                   0
  ;      8 +OVERSCAN I                                   0
  ;   0x10 +BACKGROUND I                                 0
  ;   0x20 +COLOR SEL                                    0
  inc dx
  mov al,0
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

  ; Cursor Start                                        20
  ;   0x1f Cursor Start                                 00
  ;   0x60 Cursor Mode                                  20
  mov ax,0x200a
  out dx,ax

  ;   0x1f Cursor End                                   00
  mov ax,0x000b
  out dx,ax

  ;   0x3f Start Address (H)                            00
  inc ax
  out dx,ax

  ;   0xff Start Address (L)                            00
  inc ax
  out dx,ax

  ;   0x3f Cursor (H)                                   00
  inc ax
  out dx,ax

  ;   0xff Cursor (L)                                   00
  inc ax
  out dx,ax

  mov ax,cs
  mov ds,ax

  push es

  ; Fill visible video memory with spaces
  mov cx,80*25
  mov ax,0x0720
  xor di,di
  rep stosw

  push di

  mov si,snowRoutine
  mov cx,snowRoutineEnd - snowRoutine
  rep movsb

  mov cl,1
  mov al,0x3f
  mov dx,0x03d

  retf

snowRoutine:
  %rep 261
    times 24 nop
    mul cl
  %endrep


;  28.5 per line

;  17.5 on last line


  mov al,0x07
  times 6 nop
;  times 7456 nop
  mul cl
  times 9
  nop
  mov
  jmp snowRoutine
snowRoutineEnd:
