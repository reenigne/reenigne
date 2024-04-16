  %include "../defaults_bin.asm"

  mov ax,6
  int 0x10
  mov ax,0x071a
  mov dx,0x3d8
  out dx,ax
  mov ax,0xb800
  mov es,ax
  xor di,di
  mov ax,0x8888
  mov cx,8192
  rep stosw

  sti
l:
  mov ah,0
  int 0x16

  mov ax,0x0602
  out dx,ax

  mov ah,0
  int 0x16

  mov ax,0x061a
  out dx,ax

  mov ah,0
  int 0x16

  mov ax,0x071a
  out dx,ax
  jmp l



  cli
  hlt
