org 0x100
cpu 8086

  xor ax,ax
  mov ds,ax
  mov byte[0x485],16   ; Request 25 line mode
  ret
