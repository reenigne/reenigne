org 0
cpu 8086

  mov ax,6
  int 0x10

  mov al,0x1a
  mov dx,0x3d8
  out dx,al

;  mov al,0x0f
  mov al,1
  mov dx,0x3d9
  out dx,al

  mov al,0

  mov ax,0xb800
  mov es,ax
screenLoop:
  mov di,0
  cld

rowLoop:

  mov cx,6
lineLoop:
  push cx
  mov cx,80
  rep stosb
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

