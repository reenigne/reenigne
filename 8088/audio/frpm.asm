  %include "../defaults_bin.asm"

; Fast-repeating PWM modes 2 and 3

  in al,0x61
  or al,3
  out 0x61,al

  mov dx,0x3da
  waitForVerticalSync
  waitForNoVerticalSync

  mov al,TIMER2 | LSB | MODE2 | BINARY
  out 0x43,al
  mov al,1                                     ; 0
  out 0x42,al

  waitForVerticalSync
  waitForNoVerticalSync

  mov al,TIMER2 | LSB | MODE2 | BINARY
  out 0x43,al
  mov al,2                                     ; 10
  out 0x42,al

  waitForVerticalSync
  waitForNoVerticalSync

  mov al,TIMER2 | LSB | MODE2 | BINARY
  out 0x43,al
  mov al,3                                     ; 100
  out 0x42,al

  waitForVerticalSync
  waitForNoVerticalSync

  mov al,TIMER2 | LSB | MODE2 | BINARY
  out 0x43,al
  mov al,4                                     ; 1000
  out 0x42,al

  waitForVerticalSync
  waitForNoVerticalSync

  mov al,TIMER2 | LSB | MODE3 | BINARY
  out 0x43,al
  mov al,1                                     ; 0
  out 0x42,al

  waitForVerticalSync
  waitForNoVerticalSync

  mov al,TIMER2 | LSB | MODE3 | BINARY
  out 0x43,al
  mov al,2                                     ; 10
  out 0x42,al

  waitForVerticalSync
  waitForNoVerticalSync

  mov al,TIMER2 | LSB | MODE3 | BINARY
  out 0x43,al
  mov al,3                                     ; 1
  out 0x42,al

  waitForVerticalSync
  waitForNoVerticalSync

  mov al,TIMER2 | LSB | MODE3 | BINARY
  out 0x43,al
  mov al,4                                     ; 1100
  out 0x42,al

  waitForVerticalSync
  waitForNoVerticalSync

  mov al,TIMER2 | LSB | MODE3 | BINARY
  out 0x43,al
  mov al,5                                     ; 11000
  out 0x42,al

  waitForVerticalSync
  waitForNoVerticalSync

  complete


; 1 voice: 0 1 perfect
; 2 voices: 00 10 11 perfect
; 3 voices: 000000 100100 101010 111111 distorted
; 4 voices; 000000000000 100010001000 100100100100 101010101010 111111111111

