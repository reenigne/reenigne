  %include "../defaults_bin.asm"

  mov cx,0x100
loopTop:
  mov al,cl
  out 0x43,al
  mov al,cl
  xor al,0x55
  out 0xe0,al
  in al,0x43
  mov ah,al
  mov al,0x99
  out 0x43,al

  in al,0x61
  or al,0x80
  out 0x61,al
  and al,0x7f
  out 0x61,al

  mov al,ah
  outputHex
  mov al,0x20
  outputCharacter
  loop loopTop
  complete
