  %include "../defaults_bin.asm"

  mov ax,0x40
  push ax
  jmp 0xf000:0xe518
