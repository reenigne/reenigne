  %include "../defaults_com.asm"

main:
  xor cx,cx
  mov dl,0xa
  mov ds,cx
  xor si,si
  sendFile

  mov ax,0x4c00
  int 0x21
