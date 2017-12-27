  %include "defaults_bin.asm"

  mov ax,0xaa
  db 0xd1,0xf0
  outputHex
  outputCharacter ' '

;  mov al,0xaa
;  db 0xd0,0xf0
;  outputHex
;  outputCharacter ' '

  complete
