  %include "defaults_bin.asm"


  xor ax,ax
  mov es,ax
  mov word[es:0],overflow
  mov word[es:2],cs

  mov dx,1
  xor ax,ax
  mov cx,2
  div cx     ; Does not overflow
  outputHex


  mov dx,1
  xor ax,ax
  mov cx,-2
  idiv cx     ; Overflows
  outputHex

  mov dx,1
  xor ax,ax
  mov cx,2
  idiv cx     ; Overflows
  outputHex

  mov dx,2
  xor ax,ax
  mov cx,0
  idiv cx
  outputHex

overflow:
  outputCharacter 'E'

  complete
