  %include "../defaults_bin.asm"

  mov dx,0x3a8
  in al,dx
  printCharacter
  complete

