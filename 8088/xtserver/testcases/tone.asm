  %include "../../defaults_bin.asm"

  mov cx,65535

  in al,0x61
  or al,3
  out 0x61,al

  mov al,TIMER2 | BOTH | MODE3 | BINARY
  out 0x43,al

  xor ax,ax
  out 0x42,al
  out 0x42,al
  cli
  hlt

