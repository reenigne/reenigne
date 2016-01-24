  %include "../../defaults_bin.asm"

  ; Set graphics mode
  mov ax,6
  int 0x10
  mov dx,0x3d8
  mov al,0x1a
  out dx,al

  ; Copy image data
  mov ax,0xb800
  mov es,ax
  mov ax,cs
  mov ds,ax
  xor di,di
  mov si,imageData
  cld
  mov cx,4000 + 0x1000
  rep movsw

  cli
  hlt
imageData:
