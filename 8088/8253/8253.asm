  %include "../defaults_bin.asm"

  cli



  mov al,0x4d  ; speaker off but gate high
  out 0x61,al

;                                                    mode 2                                                               mode 2 + latch    mode 0 + latch     mode 0

; Experiment 1: No writes                            0xfc73                                                                   fc77              fc75             fc60
  call init
  in al,0x42                                       ;     73 LSB okay
  mov ah,al
  in al,0x42                                       ;   fc   MSB  okay
  xchg ah,al

; Experiment 2: One write before                     0x68ff                                                                   5cff              67ff             5bff
  call init
  mov al,0
  out 0x42,al
  in al,0x42                                       ;     ff Read reset MSB?
  mov ah,al
  in al,0x42                                       ;   68   Read an old LSB???
  xchg ah,al

; Experiment 3: One write between                    0x6e75                                                                   5c75              8477             a15c
  call init
  in al,0x42                                       ;     75 LSB okay
  mov ah,al
  mov al,0
  out 0x42,al
  in al,0x42                                       ;   6e   Also an LSB
  xchg ah,al

; Experiment 4: Two writes before                    0xfc60  What's going on here? Shouldn't it get reset by the count load?  fc5c              ffff             fffe
  call init                                             ;     No, in mode 2 new count is only loaded when value reaches 0
  mov al,0
  out 0x42,al
  mov al,0
  out 0x42,al
  in al,0x42                                       ;     60 LSB
  mov ah,al
  in al,0x42                                       ;   fc   MSB
  xchg ah,al

; Experiment 5: Two writes between                   0xff73                                                                   ff75              ff77             ff5e
  call init
  in al,0x42                                       ;     73 LSB okay
  mov ah,al
  mov al,0
  out 0x42,al
  mov al,0
  out 0x42,al
  in al,0x42                                       ;   ff   reset MSB
  xchg ah,al

; Experiment 6: One write before, one write between  0xffff   (ff8a when latched and read first)                              ff7f              ff7f             ff8a
  call init
  out 0x42,al
  mov al,0
  in al,0x42                                       ;     ff
  mov ah,al
  mov al,0
  out 0x42,al
  in al,0x42                                       ;   ff
  xchg ah,al

; Done
  call init
  complete


init:
  push ax
  mov ax,bx
  printHex
  printCharacter '-'

  pop ax
  printHex
  printCharacter ' '

  writePIT16 2, 0, 0

  mov cx,200
waitLoop:
  loop waitLoop

  readPIT16 2
  mov bx,ax


  ret




