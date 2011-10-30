org 0
cpu 8086

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


  mov ax,6
  int 0x10

  mov al,0x1a
  mov dx,0x3d8
  out dx,al

  mov al,0x0f
  mov dx,0x3d9
  out dx,al

  mov al,0

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

  mov dx,0x3da
frameLoop:
  waitForVerticalSync
  waitforNoVerticalSync

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






  add di,0x2000-80
  mov cx,80
  rep stosb
  sub di,0x2000
  pop cx
  loop lineLoop

  add al,0x11
  test al,0xf
  jnz rowLoop

colourLoop:
  push ax
  mov ah,0
  int 0x16
  pop ax

  inc al
  mov dx,0x3d9
  out dx,al
  loop colourLoop


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

