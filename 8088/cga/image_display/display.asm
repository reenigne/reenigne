  %include "../defaults_com.asm"

  ; Set graphics mode
  mov ax,4
  int 0x10

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

  ; Wait for a keystroke
  mov ah,0
  int 0x16

  ; Back to text mode
  mov ax,3
  int 0x10

  ; Return to OS
  ret

imageData:
