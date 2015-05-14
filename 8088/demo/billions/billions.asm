%include "../../defaults_bin.asm"

  cli

  ; Copy data
  mov ax,cs
  mov ds,ax
  mov ax,0xb800
  mov es,ax
  mov cx,8000
  mov si,data
  mov di,160
  cld
  rep movsw

;  mov cx,102
;  xor di,di
;  mov si,bursts
;  xor bx,bx
;burstLoop:
;  times 4 movsw
;  sub si,8
;  inc bx
;  cmp bx,6
;  jb noNextBurst
;  add si,8
;  xor bx,bx
;noNextBurst:
;  cmp si,bursts+16*8
;  jb noResetBurst
;  mov si,bursts
;noResetBurst:
;  add di,160-8
;  loop burstLoop

  mov cx,102
  xor di,di
  mov si,bursts2
  xor bx,bx
burstLoop:
  times 4 movsw
  sub si,8
  inc bx
  cmp bx,1
  jb noNextBurst
  add si,8
  xor bx,bx
noNextBurst:
  cmp si,bursts2+96*8
  jb noResetBurst
  mov si,bursts2
noResetBurst:
  add di,160-10
  loop burstLoop


unrolledCode equ data + 160*102

  ; Set up interrupt masks.
  mov al,0xfd  ; Enable IRQ 1 (keyboard)
  out 0x21,al  ; Leave disabled 0 (timer), 2 (EGA/VGA/slave 8259) 3 (COM2/COM4), 4 (COM1/COM3), 5 (hard drive, LPT2), 6 (floppy disk) and 7 (LPT1)

  xor ax,ax
  mov es,ax
  mov word[es:9*4],interrupt9
  mov [es:9*4+2],cs

  jmp doUnroll

interrupt9:
  mov ax,cs
  mov ds,ax

  in al,0x60
  mov ah,al

  in al,0x61
  or al,0x80
  out 0x61,al
  and al,0x7f
  out 0x61,al

  mov al,0x20
  out 0x20,al

  add sp,6
  cmp ah,0x4b
  jne notLeft
  dec byte[refreshPhase]
  cmp byte[refreshPhase],0xff
  jne notLeft
  mov byte[refreshPhase],19*4-1
notLeft:
  cmp ah,0x4d
  jne notRight
  inc byte[refreshPhase]
  cmp byte[refreshPhase],19*4
  jne notRight
  mov byte[refreshPhase],0
notRight:
  cmp ah,0x48
  jne notUp
  dec word[crtcPhase]
  cmp word[refreshPhase],0xffff
  jne notUp
  mov word[refreshPhase],76*4-1
notUp:
  cmp ah,0x50
  jne notDown
  inc word[crtcPhase]
  cmp word[crtcPhase],76*4
  jne notDown
  mov word[crtcPhase],0
notDown:

doUnroll:
  print "Refresh: "
  mov al,[refreshPhase]
  mov ah,0
  printHex
  print ", CRTC: "
  mov ax,[crtcPhase]
  printHex
  printCharacter 10

  mov ax,cs
  mov es,ax
  mov si,unrollData1
  mov di,unrolledCode
  mov cx,unrollData2-unrollData1
  rep movsb

  mov cl,[refreshPhase]
  mov ch,0
  shr cx,1
  shr cx,1
  mov al,0x90
  rep stosb

  mov si,unrollData2
  mov cx,unrollData3-unrollData2
  rep movsb

  mov cx,[crtcPhase]
  shr cx,1
  shr cx,1
  mov al,0x90
  rep stosb

  mov si,unrollData3
  mov cx,unrollDataEnd-unrollData3
  rep movsb

  jmp unrolledCode


unrollData1:
  lockstep2
  mov ax,cs
  mov ds,ax

  mov al,[refreshPhase]
  and al,3
  mov bx,mulTimingTable
  xlatb
  mov cl,0
  mul cl

unrollData2:

  mov al,TIMER1 | LSB | MODE2 | BINARY
  out 0x43,al
  mov al,19
  out 0x41,al  ; Timer 1 rate

; Scanlines 0-199 = image display at one CRTC frame per scanline
; Scanline 200 = set CRTC for vsync      0
; Scanline 224 = start of vsync pulse   24
; Scanline 261 = set CRTC for normal


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

  ;   0x0f Horizontal Sync Width                        0c
  mov ax,0x0c03
  out dx,ax

  ;   0x7f Vertical Total                               01
  mov ax,0x0104
  out dx,ax

  ;   0x1f Vertical Total Adjust                        00
  mov ax,0x0005
  out dx,ax

  ;   0x7f Vertical Displayed                           02
  mov ax,0x0206
  out dx,ax

  ;   0x7f Vertical Sync Position                       18
  mov ax,0x1807
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

  mov al,[crtcPhase]
  and al,3
  mov bx,mulTimingTable
  xlatb
  mov cl,0
  mul cl

unrollData3:

;  captureScreen

  mov di,0x3fff
  sti

frameLoop:
%rep 200
    mov ax,0x0401
    out dx,ax
    mov ax,0x0b00
    out dx,ax
    mov ax,0x5001
    out dx,ax
    mov ax,0x6500
    out dx,ax
    stosb
    dec di
    times 38 nop
%endrep
  mov ax,0x0401
  out dx,ax
  mov ax,0x0b00
  out dx,ax
  mov ax,0x5001
  out dx,ax
  mov ax,0x6500
  out dx,ax
  stosb
  dec di
  times 33 nop

  mov dl,0xda
  waitForVerticalSync
  waitForNoVerticalSync
  waitForDisplayEnable
  mov dl,0xd8

  jmp frameLoop

unrollDataEnd:


;  ; Set the CGA back to a normal mode so we don't risk breaking anything
;  mov ax,3
;  int 0x10
;
;  ; Relinquish control
;  int 0x67

bursts:
  dw 0x8055, 0x8055, 0x8055, 0x8055
  dw 0x8055, 0x8055, 0x8055, 0x8013
  dw 0x8055, 0x8013, 0x8055, 0x8013
  dw 0x8055, 0x8013, 0x8013, 0x8013
  dw 0x8013, 0x8013, 0x8013, 0x8013
  dw 0x8013, 0x8013, 0x8013, 0x0855
  dw 0x8013, 0x0855, 0x8013, 0x0855
  dw 0x8013, 0x0855, 0x0855, 0x0855
  dw 0x0855, 0x0855, 0x0855, 0x0855
  dw 0x0855, 0x0855, 0x0855, 0x0813
  dw 0x0855, 0x0813, 0x0855, 0x0813
  dw 0x0855, 0x0813, 0x0813, 0x0813
  dw 0x0813, 0x0813, 0x0813, 0x0813
  dw 0x0813, 0x0813, 0x0813, 0x8055
  dw 0x0813, 0x8055, 0x0813, 0x8055
  dw 0x0813, 0x8055, 0x8055, 0x8055

bursts2:
  dw 0x11b1, 0x11b1, 0x11b1, 0x11b1
  dw 0x11b1, 0x11b1, 0x11b1, 0x31b1
  dw 0x11b1, 0x31b1, 0x11b1, 0x31b1
  dw 0x11b1, 0x31b1, 0x31b1, 0x31b1
  dw 0x31b1, 0x31b1, 0x31b1, 0x31b1
  dw 0x31b1, 0x31b1, 0x31b1, 0x33b1
  dw 0x31b1, 0x33b1, 0x31b1, 0x33b1
  dw 0x31b1, 0x33b1, 0x33b1, 0x33b1
  dw 0x33b1, 0x33b1, 0x33b1, 0x33b1
  dw 0x33b1, 0x33b1, 0x33b1, 0x23b1
  dw 0x33b1, 0x23b1, 0x33b1, 0x23b1
  dw 0x33b1, 0x23b1, 0x23b1, 0x23b1
  dw 0x23b1, 0x23b1, 0x23b1, 0x23b1
  dw 0x23b1, 0x23b1, 0x23b1, 0x22b1
  dw 0x23b1, 0x22b1, 0x23b1, 0x22b1
  dw 0x23b1, 0x22b1, 0x22b1, 0x22b1
  dw 0x22b1, 0x22b1, 0x22b1, 0x22b1
  dw 0x22b1, 0x22b1, 0x22b1, 0x62b1
  dw 0x22b1, 0x62b1, 0x22b1, 0x62b1
  dw 0x22b1, 0x62b1, 0x62b1, 0x62b1
  dw 0x62b1, 0x62b1, 0x62b1, 0x62b1
  dw 0x62b1, 0x62b1, 0x62b1, 0x66b1
  dw 0x62b1, 0x66b1, 0x62b1, 0x66b1
  dw 0x62b1, 0x66b1, 0x66b1, 0x66b1
  dw 0x66b1, 0x66b1, 0x66b1, 0x66b1
  dw 0x66b1, 0x66b1, 0x66b1, 0x46b1
  dw 0x66b1, 0x46b1, 0x66b1, 0x46b1
  dw 0x66b1, 0x46b1, 0x46b1, 0x46b1
  dw 0x46b1, 0x46b1, 0x46b1, 0x46b1
  dw 0x46b1, 0x46b1, 0x46b1, 0x44b1
  dw 0x46b1, 0x44b1, 0x46b1, 0x44b1
  dw 0x46b1, 0x44b1, 0x44b1, 0x44b1
  dw 0x44b1, 0x44b1, 0x44b1, 0x44b1
  dw 0x44b1, 0x44b1, 0x44b1, 0x54b1
  dw 0x44b1, 0x54b1, 0x44b1, 0x54b1
  dw 0x44b1, 0x54b1, 0x54b1, 0x54b1
  dw 0x54b1, 0x54b1, 0x54b1, 0x54b1
  dw 0x54b1, 0x54b1, 0x54b1, 0x55b1
  dw 0x54b1, 0x55b1, 0x54b1, 0x55b1
  dw 0x54b1, 0x55b1, 0x55b1, 0x55b1
  dw 0x55b1, 0x55b1, 0x55b1, 0x55b1
  dw 0x55b1, 0x55b1, 0x55b1, 0x15b1
  dw 0x55b1, 0x15b1, 0x55b1, 0x15b1
  dw 0x55b1, 0x15b1, 0x15b1, 0x15b1
  dw 0x15b1, 0x15b1, 0x15b1, 0x15b1
  dw 0x15b1, 0x15b1, 0x15b1, 0x11b1
  dw 0x15b1, 0x11b1, 0x15b1, 0x11b1
  dw 0x15b1, 0x11b1, 0x11b1, 0x11b1

  dw 0x11b1, 0x11b1, 0x11b1, 0x11b1
  dw 0x11b1, 0x11b1, 0x11b1, 0x13b1
  dw 0x11b1, 0x13b1, 0x11b1, 0x13b1
  dw 0x11b1, 0x13b1, 0x13b1, 0x13b1
  dw 0x13b1, 0x13b1, 0x13b1, 0x13b1
  dw 0x13b1, 0x13b1, 0x13b1, 0x33b1
  dw 0x13b1, 0x33b1, 0x13b1, 0x33b1
  dw 0x13b1, 0x33b1, 0x33b1, 0x33b1
  dw 0x33b1, 0x33b1, 0x33b1, 0x33b1
  dw 0x33b1, 0x33b1, 0x33b1, 0x32b1
  dw 0x33b1, 0x32b1, 0x33b1, 0x32b1
  dw 0x33b1, 0x32b1, 0x32b1, 0x32b1
  dw 0x32b1, 0x32b1, 0x32b1, 0x32b1
  dw 0x32b1, 0x32b1, 0x32b1, 0x22b1
  dw 0x32b1, 0x22b1, 0x32b1, 0x22b1
  dw 0x32b1, 0x22b1, 0x22b1, 0x22b1
  dw 0x22b1, 0x22b1, 0x22b1, 0x22b1
  dw 0x22b1, 0x22b1, 0x22b1, 0x26b1
  dw 0x22b1, 0x26b1, 0x22b1, 0x26b1
  dw 0x22b1, 0x26b1, 0x26b1, 0x26b1
  dw 0x26b1, 0x26b1, 0x26b1, 0x26b1
  dw 0x26b1, 0x26b1, 0x26b1, 0x66b1
  dw 0x26b1, 0x66b1, 0x26b1, 0x66b1
  dw 0x26b1, 0x66b1, 0x66b1, 0x66b1
  dw 0x66b1, 0x66b1, 0x66b1, 0x66b1
  dw 0x66b1, 0x66b1, 0x66b1, 0x64b1
  dw 0x66b1, 0x64b1, 0x66b1, 0x64b1
  dw 0x66b1, 0x64b1, 0x64b1, 0x64b1
  dw 0x64b1, 0x64b1, 0x64b1, 0x64b1
  dw 0x64b1, 0x64b1, 0x64b1, 0x44b1
  dw 0x64b1, 0x44b1, 0x64b1, 0x44b1
  dw 0x64b1, 0x44b1, 0x44b1, 0x44b1
  dw 0x44b1, 0x44b1, 0x44b1, 0x44b1
  dw 0x44b1, 0x44b1, 0x44b1, 0x45b1
  dw 0x44b1, 0x45b1, 0x44b1, 0x45b1
  dw 0x44b1, 0x45b1, 0x45b1, 0x45b1
  dw 0x45b1, 0x45b1, 0x45b1, 0x45b1
  dw 0x45b1, 0x45b1, 0x45b1, 0x55b1
  dw 0x45b1, 0x55b1, 0x45b1, 0x55b1
  dw 0x45b1, 0x55b1, 0x55b1, 0x55b1
  dw 0x55b1, 0x55b1, 0x55b1, 0x55b1
  dw 0x55b1, 0x55b1, 0x55b1, 0x51b1
  dw 0x55b1, 0x51b1, 0x55b1, 0x51b1
  dw 0x55b1, 0x51b1, 0x51b1, 0x51b1
  dw 0x51b1, 0x51b1, 0x51b1, 0x51b1
  dw 0x51b1, 0x51b1, 0x51b1, 0x11b1
  dw 0x51b1, 0x11b1, 0x51b1, 0x11b1
  dw 0x51b1, 0x11b1, 0x11b1, 0x11b1

mulTimingTable:
  db 0x01, 0x03, 0x07, 0x0f

refreshPhase: db 0
crtcPhase: dw 0xf8

data:
