  list p=12f508
#include "P12F508.INC"

IIII EQU 3fh
IOII EQU 3eh

data0         EQU 07h
data1         EQU 08h
data2         EQU 09h
data3         EQU 0ah
data4         EQU 0bh
data5         EQU 0ch
data6         EQU 0dh
data7         EQU 0eh

count         EQU 0fh

lengthLow     EQU 07h
lengthMiddle  EQU 08h
lengthHigh    EQU 09h
parentLow     EQU 0ah
switch        EQU 0bh
childBpresent EQU 0ch
childCpresent EQU 0dh
childDpresent EQU 0eh


  __config _MCLRE_OFF & _CP_OFF & _WDT_OFF & _IntRC_OSC

delay1 macro
  NOP
  endm

delay2 macro
  local l
  GOTO l
l
  endm

delay3 macro
  delay2
  delay1
  endm


startup
  MOVWF OSCCAL
  MOVLW 80h                  ; wake up on pin change disabled (80h) | weak pull-ups enabled (00h) | timer 0 clock source on instruction cycle (00h) | timer 0 source
  OPTION
  CLRF GPIO

reset
  ; Mark a new data stream
  TRIS 7
  ; Prime the child
  MOVLW IOII
  TRIS GPIO
  ; Wait for prime to be recognized
  CALL delay14
  ; Clear prime
  MOVLW IIII
  TRIS GPIO
  ; Wait for response to be ready
  CALL delay7
  ; Check for child present
  BTFSC GPIO, 0
  GOTO reset
  ; Wait while "prime pending"
waitForPrimeComplete
  BTFSS GPIO, 0
  GOTO waitForPrimeComplete
  ; Wait for child to set function pointers
  CALL delay14
dataLoop
  delay3
  ; Send "data request"
  MOVLW IOII                 ;  1   1    -6
  TRIS GPIO                  ;  1   1    -5
  CALL delay4
  ; Send "sync"
  MOVLW IIII                 ;  1   0     0
  TRIS GPIO                  ;  1   0     1
  MOVLW IOII                 ;  1   1    -6
  TRIS GPIO                  ;  1   1     3
  ; Wait for child to read
  MOVLW 0x1a
  MOVWF count
readDelay
  DECFSZ count, F
  GOTO readDelay
  delay2
  ; Send "sync"
  MOVLW IIII
  TRIS GPIO
  CALL delay6          ; 6    1        -8
  MOVLW IOII                 ;  1   1    -6
  TRIS GPIO            ; 1    1        -1
  MOVLW IIII           ; 1    0         0
  TRIS GPIO            ; 1    0         1
  MOVLW IOII                 ;  1   1    -6
  TRIS GPIO            ; 1    1         3
  ; Wait for child to start sending data
  CALL delay21
  MOVLW IIII                 ; 2
  TRIS GPIO
  ; Read and output data bits
  MOVLW 0                    ; 1   -1
  BTFSC GPIO, 0              ; 1    0
  MOVLW 1                    ; 1
  TRIS 5                     ; 1
  delay1                     ; 1
  MOVLW 0                    ; 1
  BTFSC GPIO, 0              ; 1
  MOVLW 1                    ; 1
  TRIS 5                     ; 1
  CALL delay4
  MOVLW 0                    ; 1   -1
  BTFSC GPIO, 0              ; 1    0
  MOVLW 1                    ; 1
  TRIS 5                     ; 1
  delay1                     ; 1
  MOVLW 0                    ; 1
  BTFSC GPIO, 0              ; 1
  MOVLW 1                    ; 1
  TRIS 5                     ; 1
  CALL delay4
  MOVLW 0                    ; 1   -1
  BTFSC GPIO, 0              ; 1    0
  MOVLW 1                    ; 1
  TRIS 5                     ; 1
  delay1                     ; 1
  MOVLW 0                    ; 1
  BTFSC GPIO, 0              ; 1
  MOVLW 1                    ; 1
  TRIS 5                     ; 1
  CALL delay4
  MOVLW 0                    ; 1   -1
  BTFSC GPIO, 0              ; 1    0
  MOVLW 1                    ; 1
  TRIS 5                     ; 1
  delay1                     ; 1
  MOVLW 0                    ; 1
  BTFSC GPIO, 0              ; 1
  MOVLW 1                    ; 1
  TRIS 5                     ; 1
  CALL delay8
  ; Check for more data
  BTFSC GPIO, 0
  GOTO reset
  GOTO dataLoop


delay21
  NOP
delay20
  GOTO delay18
delay18
  GOTO delay16
delay16
  GOTO delay14
delay14
  GOTO delay12
delay12
  GOTO delay10
delay10
  GOTO delay8
delay8
  NOP
delay7
  NOP
delay6
  GOTO delay4
delay4
  RETLW 0

  end
