  %include "../../defaults_bin.asm"

  mov ax,0xb800
  mov es,ax
  mov ax,0xffff
  mov cx,4096
  xor di,di
  cld
  rep stosw
  mov cx,4096
  mov ax,0xff00
  rep stosw

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
  mov al,0x00
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
  mov ax,0x0104
  out dx,ax

  ;   0x1f Vertical Total Adjust                        00
  mov ax,0x0005
  out dx,ax

  ;   0x7f Vertical Displayed                           02
  mov ax,0x0106
  out dx,ax

  ;   0x7f Vertical Sync Position                       18
  mov ax,0x2e07                                                      ; 38
  out dx,ax

  ;   0x03 Interlace Mode                               02   0 = non interlaced, 1 = interlace sync, 3 = interlace sync and video
  mov ax,0x0008
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

  mov dl,0xda
  cli

  mov al,0x34
  out 0x43,al
  mov al,0
  out 0x40,al
  out 0x40,al

  mov al,76*2 - 1
  out 0x40,al
  mov al,0
  out 0x40,al

  xor ax,ax
  mov ds,ax
  mov ax,[0x20]
  mov [cs:originalInterrupt8],ax
  mov ax,[0x22]
  mov [cs:originalInterrupt8+2],ax
  mov word[0x20],int8_oe0
  mov [0x22],cs

  in al,0x21
  mov [cs:originalIMR],al
  mov al,0xfe
  out 0x21,al

  sti
setupLoop:
  hlt
  jmp setupLoop

doneSetup:
  ret


originalInterrupt8:
  dw 0, 0
originalIMR:
  db 0


  ; Step 0 - don't do anything (we've just completed wait for CRTC stabilization)
int8_oe0:
  mov word[0x20],int8_oe1
  mov al,0x20
  out 0x20,al
  iret


  ; Step 1, wait until display is enabled, then change interrupts
int8_oe1:
  in al,dx
  test al,1
  jnz .noInterruptChange  ; jump if -DISPEN, finish if +DISPEN

  mov word[0x20],int8_oe2

.noInterruptChange:
  mov al,0x20
  out 0x20,al
  iret


  ; Step 2, wait until display is disabled - then we'll be just before the start of the active area
int8_oe2:
  in al,dx
  test al,1
  jz .noInterruptChange   ; jump if not -DISPEN, finish if -DISPEN

  mov word[0x20],int8_oe3
  mov cx,2

.noInterruptChange:
  mov al,0x20
  out 0x20,al
  iret


  ; Step 3 - this interrupt occurs right at the start of the active area.
  ; The pattern of scanlines on the screen is +-+-- As the interrupt runs every other scanline, the pattern of scanlines in terms of what is seen from the interrupt is ++---.
int8_oe3:
  mov dl,0xd4
  mov ax,0x0308  ; Set interlace mode to ISAV
  out dx,ax
  mov dl,0xda

  loop .noInterruptChange
  mov word[0x20],int8_oe4
.noInterruptChange:

  mov al,76*2
  out 0x40,al
  mov al,0
  out 0x40,al

  mov al,0x20
  out 0x20,al
  iret


  ; Step 4
int8_oe4:
  in al,dx
  test al,1
  jnz .noInterruptChange  ; jump if -DISPEN, finish if +DISPEN

  mov word[0x20],int8_oe5

.noInterruptChange:
  mov al,0x20
  out 0x20,al
  iret


  ; Step 5
int8_oe5:
  in al,dx
  test al,1
  jz .noInterruptChange   ; jump if not -DISPEN, finish if -DISPEN (i.e. scanline 4)

  mov word[0x20],int8_oe6

.noInterruptChange:
  mov al,0x20
  out 0x20,al
  iret


  ; Step 6. This occurs on scanline 1. The next interrupt will be on scanline 3.
int8_oe6:
  mov word[0x20],int8_oe7
  mov al,0x20
  out 0x20,al
  iret


  ; Step 7. This occurs on scanline 3. The next interrupt will be on scanline 0.
int8_oe7:
  mov word[0x20],int8_oe8
  mov al,0x20
  out 0x20,al
  iret


  ; Step 8 - scanline 0, next interrupt on scanline 2
int8_oe8:
  mov al,(20*76) & 0xff
  out 0x40,al
  mov al,(20*76) >> 8
  out 0x40,al

  mov word[0x20],int8_oe9

  mov dl,0xd4

  mov al,0x20
  out 0x20,al
  iret


  ; Step 9 - initial short (odd) field
int8_oe9:
  mov ax,0x0309
  out dx,ax
  mov ax,0x0106
  out dx,ax
  mov ax,0x0304    ; 0404
  out dx,ax
  mov ax,0x0305
  out dx,ax

  mov al,(242*76) & 0xff
  out 0x40,al
  mov al,(242*76) >> 8
  out 0x40,al

  mov word[0x20],int8_isav0

  mov al,0x20
  out 0x20,al
  iret


  ; Final 0 - scanline 0
int8_isav0:
  mov ax,0x2806    ; 3206
  out dx,ax
  mov ax,0x3b04
  out dx,ax
  mov ax,0x0205
  out dx,ax

  mov dl,0xd9
  mov al,0x31
  out dx,al
  mov dl,0xd4

  mov al,(20*76) & 0xff
  out 0x40,al
  mov al,(20*76) >> 8
  out 0x40,al

  mov word[0x20],int8_isav1

  mov al,0x20
  out 0x20,al
  iret


  ; Final 1 - scanline 242 (202)
int8_isav1:
  mov ax,0x0106
  out dx,ax
  mov ax,0x0304
  out dx,ax
  mov ax,0x0305
  out dx,ax

  mov dl,0xd9
  mov al,0x00
  out dx,al
  mov dl,0xd4

  mov al,(242*76) & 0xff
  out 0x40,al
  mov al,(242*76) >> 8
  out 0x40,al

  mov word[0x20],int8_isav0

  mov al,0x20
  out 0x20,al
  iret

;      Normal field, even
;        240 scanlines normal 0x3B         200 0x31
;          2 scanlines extra  0x02           2 0x02
;          0 scanlines CRTC-created          0
;      Short field, odd
;         16 scanlines normal 0x03          56 0x0d
;          3 scanlines extra  0x03           3 0x03
;          1 scanline  CRTC-created          1

