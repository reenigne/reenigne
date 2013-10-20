  %include "../defaults_bin.asm"

  in al,0x61
  or al,1   ; Speaker off but gate high
  out 0x61,al

  mov cx,200
  mov al,TIMER2 | BOTH | MODE3 | BINARY
  out 0x43,al
  mov al,0xff
  out 0x42,al
  out 0x42,al

  mov al,TIMER2 | MSB | MODE3 | BINARY
  out 0x43,al
  mov al,0x00
  out 0x42,al

looptop:
  mov al,TIMER2 | LATCH
  out 0x43,al
  in al,0x42
  mov ah,al
  in al,0x42
  xchg ah,al

  printHex

  in al,0x62
  test al,0x20
  mov al,'*'
  jnz gotBit
  mov al,' '
gotBit:
  printCharacter

  printNewLine

  loop looptop

  in al,0x61
  and al,0xfc
  out 0x61,al

  complete

