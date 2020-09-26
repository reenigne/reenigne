  %include "defaults_bin.asm"

  mov bx,0x0102
  mov al,'B'
  cmp bl,bh
  mov cx,9999
  loope l1
  mov al,'A'
l1:
  outputCharacter

  mov bx,0x0102
  mov al,'B'
  cmp bl,bh
  mov cx,9999
  loopne l2
  mov al,'A'
l2:
  outputCharacter


  mov bx,0x0102
  mov al,'B'
  cmp bl,bh
  mov cx,9999
  db 0xf2
  loope l3
  mov al,'A'
l3:
  outputCharacter

  mov bx,0x0102
  mov al,'B'
  cmp bl,bh
  mov cx,9999
  db 0xf2
  loopne l4
  mov al,'A'
l4:
  outputCharacter

  mov bx,0x0102
  mov al,'B'
  cmp bl,bh
  mov cx,9999
  db 0xf3
  loope l5
  mov al,'A'
l5:
  outputCharacter

  mov bx,0x0102
  mov al,'B'
  cmp bl,bh
  mov cx,9999
  db 0xf3
  loopne l6
  mov al,'A'
l6:
  outputCharacter


  mov bx,0x0101
  mov al,'B'
  cmp bl,bh
  mov cx,9999
  loope l11
  mov al,'A'
l11:
  outputCharacter

  mov bx,0x0101
  mov al,'B'
  cmp bl,bh
  mov cx,9999
  loopne l12
  mov al,'A'
l12:
  outputCharacter


  mov bx,0x0101
  mov al,'B'
  cmp bl,bh
  mov cx,9999
  db 0xf2
  loope l13
  mov al,'A'
l13:
  outputCharacter

  mov bx,0x0101
  mov al,'B'
  cmp bl,bh
  mov cx,9999
  db 0xf2
  loopne l14
  mov al,'A'
l14:
  outputCharacter


  mov bx,0x0101
  mov al,'B'
  cmp bl,bh
  mov cx,9999
  db 0xf3
  loope l15
  mov al,'A'
l15:
  outputCharacter

  mov bx,0x0101
  mov al,'B'
  cmp bl,bh
  mov cx,9999
  db 0xf3
  loopne l16
  mov al,'A'
l16:
  outputCharacter



  complete
