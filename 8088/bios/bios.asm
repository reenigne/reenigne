cpu 8086
org 0

  mov ax,0x40
  push ax
  jmp 0xf000:0xe518
