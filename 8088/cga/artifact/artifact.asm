  %include "../defaults_bin.asm"

  ; Set graphics mode
  mov ax,6
  int 0x10

  mov al,0x1a
  mov dx,0x3d8
  out dx,al

  mov al,0x0f
  mov dx,0x3d9
  out dx,al


  ; Draw pattern in graphics memory
  mov ax,0xb800
  mov es,ax
screenLoop:
  mov di,0
  cld

rowLoop:

  mov cx,200
lineLoop:

  push cx
  mov al,0
  mov cx,16
barLoop:
  push cx
  mov cx,5
  rep stosb
  pop cx
  add al,0x11
  loop barLoop

  add di,0x1fb0
  cmp di,0x4000
  jl oddLine
  sub di,0x3fb0
oddLine:

  pop cx
  loop lineLoop


;  ; Set up CRTC registers
;  mov dx,0x3d4
;  mov ax,0x1401 ; 1: horizontal displayed = 20
;  out dx,ax
;  mov ax,0x0005 ; 5: Vertical total adjust = 8
;  out dx,ax


  mov dx,0x3da
frameLoop:
;  mov dl,0xd9
;  mov al,0xf
;  out dx,al
;  mov dl,0xd4
;  mov ax,0x2d02
;  out dx,ax
;  mov dl,0xda
  waitForVerticalSync
  waitforNoVerticalSync


;  waitForDisplayEnable
;  waitForDisplayDisable
;  waitForDisplayEnable
;  mov dl,0xd4                                         ; 2 0 2  2
;  mov ax,0x0009 ; 9: lines per row = 1                ; 3 0 3  5
;  out dx,ax
;  mov dl,0xda
;  waitForDisplayDisable
;
;  waitForDisplayEnable
;  mov dl,0xd4                                         ; 2 0 2  2
;  mov ax,0x1300 ; 0: horizontal total = 20            ; 3 0 3  5
;  out dx,ax                                           ; 1 2 3  8
;  mov ax,0x1902 ; 2: horizontal sync position = 25    ; 3 0 3 11
;  out dx,ax                                           ; 1 2 3 14
;  times 13 nop                                        ; 1 0 1 27
;  mov ax,0x2400 ; 0: horizontal total = 37            ; 3 0 3 30
;  out dx,ax                                           ; 1 2 3 33
;  mov dl,0xda
;  waitForDisplayDisable
;
;  waitForDisplayEnable
;  mov dl,0xd4                                         ; 2 0 2  2
;  mov ax,0x3800 ; 0: horizontal total = 57            ; 3 0 3  5
;  out dx,ax                                           ; 1 2 3  8
;  mov ax,0x2d02 ; 2: horizontal sync position = 45    ; 3 0 3 11
;  out dx,ax                                           ; 1 2 3 14
;  mov dl,0xda
;  waitForDisplayDisable
;
;  waitForDisplayEnable
;  mov dl,0xd4                                         ; 2 0 2  2
;  mov ax,0x0109 ; 9: lines per row = 2                ; 3 0 3  5
;  out dx,ax
;  mov dl,0xda
;  waitForDisplayDisable



;  mov dl,0xd4
;  mov ax,0x3700
;  out dx,ax
;  mov ax,0x2e02
;  out dx,ax
;  mov dl,0xda
;  waitForDisplayDisable
;  waitForDisplayEnable
;  mov dl,0xd4
;  mov ax,0x3800
;  out dx,ax
;  mov dl,0xda
;  waitForDisplayDisable
;  waitForDisplayEnable
;  waitForDisplayDisable



  mov cx,16
  mov bl,0
rowLoop2:
  push cx

  mov cx,12
lineLoop2:
  waitForDisplayEnable
  waitForDisplayDisable
  loop lineLoop2
  inc bl
  mov al,bl
  dec dx
  out dx,al
  inc dx
  pop cx
  loop rowLoop2
  jmp frameLoop


;  ; Set up the 8259 PIC to read the IRR lines
;  mov al,0x0a  ; OCW3 - no bit 5 action, no poll command issued, act on bit 0,
;  out 0x20,al  ;  read Interrupt Request Register
;keyboardRead:
;  ; Loop until the IRR bit 1 (IRQ 1) is high
;  in al,0x20
;  and al,2
;  jz keyboardRead
;  ; Read the keyboard byte and store it
;  in al,0x60
;;  mov bl,al
;  ; Acknowledge the previous byte
;  in al,0x61
;  or al,0x80
;  out 0x61,al
;  and al,0x7f
;  out 0x61,al

  jmp screenLoop

