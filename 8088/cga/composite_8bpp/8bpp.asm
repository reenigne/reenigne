  org 0x100
  cpu 8086


  mov ax,6
  int 0x10
  mov ax,0xb800
  mov es,ax

  mov cx,0x40
  mov al,0
  xor di,di
loopTop:
  push cx
  mov cx,40
  rep stosb
  inc ax

  mov cx,40
  rep stosb
  inc ax

  add di,0x2000-80

  mov cx,40
  rep stosb
  inc ax

  mov cx,40
  rep stosb
  inc ax

  add di,-0x2000
  pop cx
  loop loopTop

  mov ah,0
  int 0x16

  mov ax,3
  int 0x10

  ret
