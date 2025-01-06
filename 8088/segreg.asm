  %include "defaults_bin.asm"

  mov ax,0x1234
  mov ds,ax
  mov ax,0x5678
  mov ss,ax
  mov ax,0x9abc
  mov es,ax

  mov ax,0x2345
  db 0x8e,0xf8
  mov ax,0x6789
  db 0x8e,0xf0
  mov ax,0xabcd
  db 0x8e,0xe0

  db 0x8c,0xc0
  outputHex
  outputCharacter ' '
  db 0x8c,0xc8
  outputHex
  outputCharacter ' '
  db 0x8c,0xd0
  outputHex
  outputCharacter ' '
  db 0x8c,0xd8
  outputHex
  outputCharacter ' '
  db 0x8c,0xe0
  outputHex
  outputCharacter ' '
  db 0x8c,0xe8
  outputHex
  outputCharacter ' '
  db 0x8c,0xf0
  outputHex
  outputCharacter ' '
  db 0x8c,0xf8
  outputHex
  outputCharacter ' '


;  mov al,0xaa
;  db 0xd0,0xf0
;  outputHex
;  outputCharacter ' '

  complete
