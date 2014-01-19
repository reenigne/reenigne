org 0
cpu 8086

  mov ax,6
  int 0x10
  mov dx,0x3d9
  mov al,15
  out dx,al

top:

  mov ax,0xb800
  mov es,ax
  xor di,di
  mov ax,0xaaaa
  mov cx,0x2000
  rep stosw

  mov ah,0x00
  int 0x16

  xor di,di
  mov ax,0xcccc
  mov cx,0x2000
  rep stosw

  mov ah,0x00
  int 0x16

  xor di,di
  mov ax,0xf0f0
  mov cx,0x2000
  rep stosw

  mov ah,0x00
  int 0x16

  xor di,di
  mov ax,0xff00
  mov cx,0x2000
  rep stosw

  mov ah,0x00
  int 0x16

  jmp top

