  %include "../../defaults_bin.asm"

  mov ax,4
  int 0x10
  mov ax,0xb800
  mov es,ax
  xor di,di
  cld
  xor ax,ax
  mov cx,0x2000
  rep stosw

  mov dx,0x3d4
  mov ax,0x1000
  out dx,ax
  mov ax,0x1004
  out dx,ax

  mov al,0

loopTop:
  mov dx,0x3d9
  out dx,al

  push ax
  mov ah,0
  int 0x16
  pop ax
  inc ax
  jmp loopTop

