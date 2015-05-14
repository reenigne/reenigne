  %include "../../../defaults_bin.asm"

  initCGA 8
  mov ax,0xb800
  mov es,ax
  cld
  mov dx,0x3da
  mov bx,0x0f00

frameLoop:
  waitForVerticalSync

  dec dx
  mov al,15
  out dx,ax

  xor di,di
  mov ax,bx
  mov cx,40*25
  rep stosw

  inc bl

  mov al,0
  out dx,ax
  inc dx
  jmp frameLoop

