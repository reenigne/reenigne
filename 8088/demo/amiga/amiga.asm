%include "../../defaults_bin.asm"

  cli

  ; Copy data
  mov ax,cs
  mov ds,ax
  mov ax,0xb800
  mov es,ax
  mov cx,0x2000
  mov si,data
  mov di,0
  cld
  rep movsw



  lockstep2
  mov ax,cs
  mov ds,ax

  mov al,[refreshPhase]
  and al,3
  mov bx,mulTimingTable
  xlatb
  mov cl,0
  mul cl

  mov al,TIMER1 | LSB | MODE2 | BINARY
  out 0x43,al
  mov al,19
  out 0x41,al  ; Timer 1 rate

; Scanlines 0-199 = image display at one CRTC frame per scanline
; Scanline 200 = set CRTC for vsync      0
; Scanline 224 = start of vsync pulse   24
; Scanline 261 = set CRTC for normal


  ; Mode                                                0a
  ;      1 +HRES                                         0
  ;      2 +GRPH                                         2
  ;      4 +BW                                           0
  ;      8 +VIDEO ENABLE                                 8
  ;   0x10 +1BPP                                         0
  ;   0x20 +ENABLE BLINK                                 0
  mov dx,0x3d8
  mov al,0x0a
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

  ;   0xff Horizontal Total                             38
  mov ax,0x3800
  out dx,ax

  ;   0xff Horizontal Displayed                         0c
  mov ax,0x0c01
  out dx,ax

  ;   0xff Horizontal Sync Position                     2d
  mov ax,0x2d02
  out dx,ax

  ;   0x0f Horizontal Sync Width                        0a
  mov ax,0x0a03
  out dx,ax

  ;   0x7f Vertical Total                               7f
  mov ax,0x7f04
  out dx,ax

  ;   0x1f Vertical Total Adjust                        06
  mov ax,0x0006
  out dx,ax

  ;   0x7f Vertical Displayed                           28
  mov ax,0x2806
  out dx,ax

  ;   0x7f Vertical Sync Position                       70
  mov ax,0x7007
  out dx,ax

  ;   0x03 Interlace Mode                               02
  mov ax,0x0208
  out dx,ax

  ;   0x1f Max Scan Line Address                        01
  mov ax,0x0109
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
  jmp frameLoop



;  ; Set the CGA back to a normal mode so we don't risk breaking anything
;  mov ax,3
;  int 0x10
;
;  ; Relinquish control
;  int 0x67

unrollData19:
  out dx,al
  xchg ax,bx
  out dx,al
  xchg ax,cx
  out dx,al
  xchg ax,di
  out dx,al
  xchg ax,bp
  out dx,al
  xchg ax,sp
  out dx,al
  xchg ax,bx
  out dx,al
  xchg ax,cx
  out dx,al
  xchg ax,di
  out dx,al
  xchg ax,bp
  out dx,al
  xchg ax,sp
  out dx,al
  xchg ax,bx
  out dx,al
  xchg ax,cx
  out dx,al
  xchg ax,di
  out dx,al
  xchg ax,bp
  out dx,al
  xchg ax,sp
  out dx,al
  xchg ax,bx
  out dx,al
  xchg ax,cx
  lodsb
  out 0x42,al
  mov al,1
unrollData19End:

unrollData76:
  out dx,al
  xchg ax,bx
  out dx,al
  xchg ax,cx
  out dx,al
  xchg ax,di
  out dx,al
  xchg ax,bp
  out dx,al
  xchg ax,sp
  out dx,al
  xchg ax,bx
  out dx,al
  xchg ax,cx
  out dx,al
  xchg ax,di
  out dx,al
  xchg ax,bp
  out dx,al
  xchg ax,sp
  out dx,al
  xchg ax,bx
  out dx,al
  xchg ax,cx
  out dx,al
  xchg ax,di
  out dx,al
  xchg ax,bp
  out dx,al
  xchg ax,sp
  out dx,al
  xchg ax,bx
  out dx,al
  xchg ax,cx
  lodsb
  out 0x42,al
  mov al,1
  nop
  nop
  nop
unrollData76End:
