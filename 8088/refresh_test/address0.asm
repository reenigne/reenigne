  %include "../defaults_com.asm"

main:

  mov cx,400
loopTop2:

  in al,0x00
  mov ah,al
  in al,0x00
  xchg ah,al
  printHex
  printCharacter 32

  loop loopTop2

  print "Test complete",10

  mov ax,0x4c00
  int 0x21
