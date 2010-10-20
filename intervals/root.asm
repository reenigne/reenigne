  list p=12f508
#include "P12F508.INC"

#define length 0

bit0 EQU 4
bit1 EQU 5
bit2 EQU 0
bit3 EQU 1

highAll EQU 3fh

low0 EQU 2fh
low1 EQU 1fh
low2 EQU 3eh
low3 EQU 3dh

lengthLow         EQU 07h
lengthMiddle      EQU 08h
lengthHigh        EQU 09h
parentAxis        EQU 0ah
switch            EQU 0bh
childBabsent      EQU 0ch
childCabsent      EQU 0dh
childDabsent      EQU 0eh
more              EQU 0fh
recvData          EQU 10h
recvSync          EQU 11h
lowPrnt           EQU 12h
lowChld           EQU 13h
bits              EQU 14h
after0            EQU 15h
after1            EQU 16h
after2            EQU 17h
after3            EQU 18h
count             EQU 19h
temp              EQU 1ah


unroll macro m
  m 0
  m 1
  m 2
  m 3
  endm


delay1 macro
  NOP
  endm

delay2 macro
  GOTO $+1
  endm

delay3 macro
  delay2
  delay1
  endm


recvBit macro i, b
  BTFSC GPIO, bit#v(b)
  INCF lengthLow + i, F
  delay2
  endm

sendBit macro i
  NOP
  NOP
  NOP
  NOP
  endm

  __config _MCLRE_OFF & _CP_OFF & _WDT_OFF & _IntRC_OSC


; --- low page ---

  GOTO startup
foundHelper
  GOTO foundHelperB

lowPageCode macro b

prime#v(b)
  GOTO prime#v(b)B

recvData#v(b)
  delay1
  recvBit 0, b
  recvBit 1, b
  recvBit 2, b
  recvBit 3, b
  recvBit 4, b
  recvBit 5, b
  recvBit 6, b
  BTFSC GPIO, bit#v(b)
  INCF lengthLow + 7, F
  TRIS 5
  MOVF after#v(b), W
  BTFSC GPIO, bit#v(b)
  MOVWF PCL
setup#v(b)
  MOVLW recvData#v(b)
  MOVWF recvData
  MOVLW low#v(b)
  MOVWF lowChld
  ANDWF bits, F
setupF#v(b)
  MOVF bits, W
  TRIS GPIO            ; send W "wait for data request" (low) to child
  MOVF recvSync, W
  MOVWF PCL

recvSync#v(b)
  delay2
  delay2
  delay2
  delay2
  delay2
  delay2
  GOTO sendData

  endm

  unroll lowPageCode


initData0
  MOVLW 0  ; parent axis
  GOTO initData

initData1
  MOVLW 1  ; parent axis
  GOTO initData

delay5
  delay1
delay4
  RETLW 0

prime
  ANDWF bits, W
  TRIS GPIO

  ; delay for 57 cycles
  MOVLW 0x12       ; 1
  MOVWF count      ; 1
  DECFSZ count, F  ; 1*17 + 2
  GOTO $-1         ; 2*17
  delay2           ; 2

  MOVF bits, W
  TRIS GPIO
  INCF FSR, F
  RETLW 0

setupFinal
  CLRF more
  delay2
  GOTO setupF0


; --- high page (actually starts somewhere in sendData, can be anywhere after setupFinal label) ---

sendData
  sendBit 0
  sendBit 1
  sendBit 2
  sendBit 3
  sendBit 4
  sendBit 5
  sendBit 6
  sendBit 7         ; If there's no more data the child bits are guaranteed to be 1 (or their data would follow) so we don't need to send "more" separately
  BTFSS more, 0
  GOTO reset
  NOP
  NOP
  COMF lowChld, W
  IORWF bits, W
  MOVWF bits
  TRIS GPIO         ; clear "more" (high) and send R "data request" (high) to child
  CLRF lengthLow + 0
  CLRF lengthLow + 1
  ANDWF lowChld, W
  TRIS GPIO         ; send S "sync falling" (low) to child
  MOVF bits, W
  TRIS GPIO         ; send T "sync rising" (high) to child
  CLRF lengthLow + 2
  CLRF lengthLow + 3
  CLRF lengthLow + 4
  CLRF lengthLow + 5
  CLRF lengthLow + 6
  CLRF lengthLow + 7
  MOVF recvData, W
  MOVWF PCL

startup
  MOVWF OSCCAL
  MOVLW 80h                  ; wake up on pin change disabled (80h) | weak pull-ups enabled (00h) | timer 0 clock source on instruction cycle (00h) | timer 0 source
  OPTION

reset
  MOVLW 1                ; 1
  MOVWF childBabsent     ; 1
  MOVWF childCabsent     ; 1
  MOVWF childDabsent     ; 1
  MOVLW childBabsent-1   ; 1
  MOVWF FSR              ; 1
waitForPrime
  TRIS 7
  GOTO found0
  NOP
  NOP
  NOP
  NOP
  NOP
  NOP
  NOP
  NOP
  NOP

highPageCode macro b

found#v(b)
  ; These lines need to take 35+9 cycles so we avoid confusing prime with data
  ; (36+9 for data+more, +1 for clock drift, -2 for the BTFSCs)
  CALL initData#v(b&1)   ; 23+19
  MOVLW low#v(b)         ;  1
  MOVWF lowPrnt          ;  1
  MOVLW recvSync#v(b)    ;  1
  MOVWF recvSync         ;  1

  NOP
  NOP
  BCF bits, bit#v(b)
  CALL prime#v((b+1)&3)
  CALL prime#v((b+2)&3)
  CALL prime#v((b+3)&3)
  BSF bits, bit#v(b)
  CALL foundHelper
  MOVWF after#v((b+3)&3)
  BTFSS childDabsent, 0
  MOVLW setup#v((b+3)&3)
  MOVWF after#v((b+2)&3)
  BTFSS childCabsent, 0
  MOVLW setup#v((b+2)&3)
  MOVWF after#v((b+1)&3)
  BTFSS childBabsent, 0
  MOVLW setup#v((b+1)&3)
  MOVWF PCL

prime#v(b)B
  MOVLW low#v(b)
  CALL prime
  BTFSC GPIO, bit#v(b)
  RETLW 0
  DECF INDF, F
  BCF bits, bit#v(b)
  BTFSS GPIO, bit#v(b)  ; wait for prime complete
  GOTO $-1
  RETLW 0

  endm

  unroll highPageCode

foundHelperB
  MOVF bits, W
  TRIS GPIO
  RETLW setupFinal

initData
  MOVWF parentAxis
  MOVLW 1
  if (length & 1)
    MOVWF lengthLow
  else
    CLRF lengthLow
  endif
  if ((length >> 1) & 1)
    MOVWF lengthMiddle
  else
    CLRF lengthMiddle
  endif
  if ((length >> 2) & 1)
    MOVWF lengthHigh
  else
    CLRF lengthHigh
  endif
  MOVWF more
  CLRF switch
  BTFSC GPIO, 2
  INCF switch, F
  CLRF GPIO
  MOVLW highAll
  MOVWF bits
  CALL delay4
  CALL delay5
  RETLW 0

  end
