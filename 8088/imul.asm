  %include "defaults_bin.asm"

  mov al,0xf
  mov bl,0xf
  imul bl
  jo foo
  outputCharacter 'N'
foo:
  outputCharacter '$'
  complete
