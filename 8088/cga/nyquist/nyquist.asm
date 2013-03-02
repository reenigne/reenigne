  %include "../../defaults_bin.asm"

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
  xor di,di
  cld

  mov cx,2
pageLoop:
  push cx
  mov cx,16
  mov al,0
rowLoop:
  push cx
  mov cx,6
lineLoop:
  push cx
  mov cx,16
columnLoop:
  push cx
  mov cx,5
  rep stosb
  pop cx
  inc ax
  loop columnLoop
  sub al,0x10
  pop cx
  loop lineLoop
  add al,0x10
  pop cx
  loop rowLoop
  add di,0x200
  pop cx
  loop pageLoop

  hlt

