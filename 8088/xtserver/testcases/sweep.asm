org 0
cpu 8086

  mov cx,65535

  in al,0x61
  or al,3
  out 0x61,al

  mov al,0xb6
  out 0x43,al

loopTop:
  mov ax,cx
  out 0x42,al
  mov al,ah
  out 0x42,al
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  loop loopTop

  int 0x67

