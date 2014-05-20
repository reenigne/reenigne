%include "../defaults_bin.asm"

  in al,0x61
  or al,3
  out 0x61,al

  printCharacter 'B'

  mov al,TIMER2 | BOTH | MODE5 | BINARY
  out 0x43,al
  mov al,0x01  ; Count = 0x0001 so we'll stop almost immediately
  out 0x42,al
  mov al,0x01  ; Count = 0x0001 so we'll stop almost immediately
  out 0x42,al

  mov cx,100

top1:
;  mov al,TIMER2 | LATCH | MODE0 | BINARY
;  out 0x43,al
  in al,0x42
  mov ah,al
  in al,0x42
  xchg ah,al
  printHex
  printCharacter 13
  loop top1

  mov al,TIMER2 | LSB | MODE2 | BINARY
  out 0x43,al
  mov al,18
  out 0x42,al  ; Timer 1 rate
;  xor al,al
;  out 0x42,al

  printCharacter 13

  mov cx,100

top:
;  mov al,TIMER2 | LATCH | MODE2 | BINARY
;  out 0x43,al
  in al,0x42
  mov ah,al
  in al,0x42
  xchg ah,al
  printHex
  printCharacter 13
  loop top
  complete
