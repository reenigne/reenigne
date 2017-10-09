  %include "../defaults_bin.asm"

  xor cx,cx
  mov dl,0xa
  mov ds,cx
  xor si,si
  sendFile
  complete
