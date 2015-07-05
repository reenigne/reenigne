;  %include "../../defaults_bin.asm"
  cpu 8086
  org 0x100

  ; Set graphics mode
  mov ax,6
  int 0x10

  mov al,0x1a
  mov dx,0x3d8
  out dx,al

;  mov al,0x0f
;  mov dx,0x3d9
;  out dx,al

  mov ax,0xb800
  mov es,ax
  mov ds,ax
  xor di,di

  mov al,0
  mov cx,16
yLoop2:
  push cx
  mov cx,6
yLoop1:
  push cx
  mov cx,16
xLoop:
  push cx
  mov cx,5
  rep stosb
  pop cx
  inc al
  loop xLoop
  pop cx
  sub al,16
  loop yLoop1
  pop cx
  add al,16
  loop yLoop2


  mov cx,80*100
  mov di,0x2000
  xor si,si
swapLoop:
  lodsb
  rol al,1
  rol al,1
  rol al,1
  rol al,1
  stosb
  loop swapLoop

  cli
  hlt
