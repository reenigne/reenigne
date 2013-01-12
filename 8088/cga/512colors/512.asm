  %include "../../defaults_bin.asm"

  initCGA 9, 6, 2

  ; Copy data
  mov ax,cs
  mov ds,ax
  mov ax,0xb800
  mov es,ax
  mov cx,8000
  mov si,data
  xor di,di
  cld
  rep movsw


  mov ax,0
loopTop:
  push ax

  mov ah,al

  and al,15
  mov dx,0x3d9
  out dx,al

  shr ah,1
  shr ah,1
  shr ah,1
  shr ah,1
  mov al,3
  mov dl,0xd4
  out dx,ax

  mov ah,0
  int 0x16

  pop ax
  inc ax
  jmp loopTop

data:
