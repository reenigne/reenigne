  list p=12f508
#include "P12F508.INC"

  __config _MCLRE_OFF & _CP_OFF & _WDT_OFF & _IntRC_OSC

  MOVWF OSCCAL
  MOVLW 80h                  ; wake up on pin change disabled (80h) | weak pull-ups enabled (00h) | timer 0 clock source on instruction cycle (00h) | timer 0 source
  OPTION
  CLRF 0x1f

; Pulses GPIO at frequency 1MHz/4096 = 244.140625Hz = 16,000,000Hz / 65,536
;    990,000Hz : 66198   +662  +0x296
;  1,010,000Hz : 64887   -649  -0x289

loop
  GOTO $+1          ; 2*255
  GOTO $+1          ; 2*255
  GOTO $+1          ; 2*255
  GOTO $+1          ; 2*255
  GOTO $+1          ; 2*255
  GOTO $+1          ; 2*255
  NOP               ; 1*255
  DECFSZ 0x1f       ; 1*255 + 2
  GOTO loop         ; 2*255
  GOTO $+1          ; 2
  GOTO $+1          ; 2
  GOTO $+1          ; 2
  GOTO $+1          ; 2
  GOTO $+1          ; 2
  BSF GPIO, 0       ; 1
  BCF GPIO, 0       ; 1
  GOTO loop         ; 2

  end

