org 0
cpu 8086

  mov cx,65535

  in al,0x61
  or al,3
  out 0x61,al

  mov al,TIMER2 | BOTH | MODE3 | BINARY
  out 0x43,al

loopTop:
  mov dx,59
  mov ax,0
  mov bx,0
  sub bx,cx
  cmp bx,0
  je .skipDiv
  idiv bx
.skipDiv:
  out 0x42,al
  mov al,ah
  out 0x42,al

  push cx
  mov cx,50
.loop2:
  loop .loop2
  pop cx

  loop loopTop

  in al,0x61
  and al,0xfc
  out 0x61,al

  int 0x67


