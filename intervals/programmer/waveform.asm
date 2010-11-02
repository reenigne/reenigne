  list p=12f508
#include "P12F508.INC"

  __config _MCLRE_OFF & _CP_OFF & _WDT_OFF & _IntRC_OSC

  MOVWF OSCCAL
  MOVLW 80h                  ; wake up on pin change disabled (80h) | weak pull-ups enabled (00h) | timer 0 clock source on instruction cycle (00h) | timer 0 source
  OPTION
  MOVLW 0xfe
  TRIS GPIO

; Pulses GPIO at frequency 1MHz/4096 = 244.140625Hz = 16,000,000Hz / 65,536
;    990,000Hz : 66198   +662  +0x296
;  1,010,000Hz : 64887   -649  -0x289

loop1
  MOVLW 0xff        ; 1             1
  MOVWF 0x1f        ; 1             2
loop
  GOTO $+1          ; 2*255       512         -2  2
  GOTO $+1          ; 2*255      1022         -2  2
  GOTO $+1          ; 2*255      1532         -2  2
  GOTO $+1          ; 2*255      2042         -2  2
  GOTO $+1          ; 2*255      2552         -2  2
  GOTO $+1          ; 2*255      3062         -2  2
  NOP               ; 1*255      3317         -1  1
  DECFSZ 0x1f, F    ; 1*254 + 2  3753
  GOTO loop         ; 2*254      4081         -4
  GOTO $+1          ; 2
  GOTO $+1          ; 2
  GOTO $+1          ; 2
  GOTO $+1          ; 2
  GOTO $+1          ; 2          4091
  NOP               ; 1             2
  BSF GPIO, 0       ; 1             3
  BCF GPIO, 0       ; 1             4
  GOTO loop1        ; 2             6

  end


; offset 7226:  period = 72762 AVR cycles = 219.895Hz  => PIC is running at 900.690 KHz, about 10% too slow
