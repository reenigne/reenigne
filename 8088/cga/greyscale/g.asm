  %include "../../../defaults_bin.asm"

  initCGA 9, 0, 2

  mov ax,0xb800
  mov es,ax
  xor di,di
  mov ax,cs
  mov ds,ax
  mov si,imageData
  mov cx,80*100
  cld
  rep movsw
  captureScreen
  hlt
imageData:

