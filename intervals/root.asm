  list p=12f508
#include "P12F508.INC"

allHigh EQU 3fh
childLow EQU 3eh

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
  MOVLW childLow
  TRIS GPIO
  ; Wait for prime to be recognized
  CALL delay14
  ; Clear prime
  MOVLW allHigh
  TRIS GPIO
  ; Wait for response to be ready
  CALL delay7
  ; Check for child present
  BTFSC GPIO, 0
  GOTO reset
  ; Wait while "prime pending"
waitForPrimeComplete
  BTFSC GPIO, 0
  GOTO waitForPrimeComplete
  ; Wait for child to set function pointers
  CALL delay21
dataLoop
  CALL delay4
  ; Send "data request"
  MOVLW allHigh              ;  1   1    -6
  TRIS GPIO                  ;  1   1    -5
  delay2
  ; Send "sync"
  MOVLW childLow             ;  1   0     0
  TRIS GPIO                  ;  1   0     1
  MOVLW allHigh              ;  1   1    -6
  TRIS GPIO                  ;  1   1     3
  ; Wait for child to read
  MOVLW 0x11                 ; 1
  MOVWF count                ; 1
readDelay
  DECFSZ count, F            ; 1*16 + 2
  GOTO readDelay             ; 2*16
  delay2                     ; 2
  ; Send "sync"
  MOVLW childLow             ; 1    0         0
  TRIS GPIO                  ; 1    0         1
  MOVLW allHigh              ; 1    1        -6
  TRIS GPIO                  ; 1    1         3
  ; Wait for child to start sending data
  CALL delay8
  ; Read and output data bits
  MOVLW 0                    ; 1   -1
  BTFSC GPIO, 0              ; 1    0
  MOVLW 1                    ; 1
  TRIS 5                     ; 1
  MOVLW 0                    ; 1
  BTFSC GPIO, 0              ; 1
  MOVLW 1                    ; 1
  TRIS 5                     ; 1
  MOVLW 0                    ; 1   -1
  BTFSC GPIO, 0              ; 1    0
  MOVLW 1                    ; 1
  TRIS 5                     ; 1
  MOVLW 0                    ; 1
  BTFSC GPIO, 0              ; 1
  MOVLW 1                    ; 1
  TRIS 5                     ; 1
  MOVLW 0                    ; 1   -1
  BTFSC GPIO, 0              ; 1    0
  MOVLW 1                    ; 1
  TRIS 5                     ; 1
  MOVLW 0                    ; 1
  BTFSC GPIO, 0              ; 1
  MOVLW 1                    ; 1
  TRIS 5                     ; 1
  MOVLW 0                    ; 1   -1
  BTFSC GPIO, 0              ; 1    0
  MOVLW 1                    ; 1
  TRIS 5                     ; 1
  MOVLW 0                    ; 1
  BTFSC GPIO, 0              ; 1
  MOVLW 1                    ; 1
  TRIS 5                     ; 1
  delay1
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
