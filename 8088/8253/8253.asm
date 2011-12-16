cpu 8086
org 0

  cli

  ; Mode control (port 0x43) values

  TIMER0 EQU 0x00
  TIMER1 EQU 0x40
  TIMER2 EQU 0x80

  LATCH  EQU 0x00
  LSB    EQU 0x10
  MSB    EQU 0x20
  BOTH   EQU 0x30 ; LSB first, then MSB

  MODE0  EQU 0x00 ; Interrupt on terminal count: low during countdown then high                            (useful for PWM)
  MODE1  EQU 0x02 ; Programmable one shot      : low from gate rising to end of countdown
  MODE2  EQU 0x04 ; Rate generator             : output low for one cycle out of N                         (useful for timing things)
  MODE3  EQU 0x06 ; Square wave generator      : high for ceil(n/2) and low for floor(n/2)                 (useful for beepy sounds)
  MODE4  EQU 0x08 ; Software triggered strobe  : high during countdown then low for one cycle
  MODE5  EQU 0x0a ; Hardware triggered strobe  : wait for gate rising, then high during countdown, then low for one cycle

  BINARY EQU 0x00
  BCD    EQU 0x01


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

; Experiment 6: One write before, one write between  0xffff   (ff8a when latched and read first)                              ff7f              ff7f
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
  int 0x67


init:
  push ax
  mov ax,bx
  int 0x60
  mov al,'-'
  int 0x62

  pop ax
  int 0x60
  mov al,0x20
  int 0x62

  mov al,TIMER2 | BOTH | MODE0
  out 0x43,al
  mov al,0
  out 0x42,al
  out 0x42,al

  mov cx,200
waitLoop:
  loop waitLoop

  mov al,TIMER2 | LATCH
  out 0x43,al
  in al,0x42
  mov ah,al
  in al,0x42
  xchg ah,al
  mov bx,ax


  ret




