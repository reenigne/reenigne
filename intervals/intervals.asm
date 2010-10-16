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
waitDReq          EQU 1ah
delta             EQU 1bh


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
  MOVF bits, W
  BTFSS lengthLow + i, 0
  ANDWF lowPrnt, W
  TRIS GPIO
  endm

  __config _MCLRE_OFF & _CP_OFF & _WDT_OFF & _IntRC_OSC


; --- low page ---

  GOTO startup
foundHelper
  GOTO foundHelperB

lowPageCode macro b

prime#v(b)
  GOTO prime#v(b)B
init#v(b)
  GOTO init#v(b)B
waitDReq#v(b)
  GOTO waitDReq#v(b)B

recvData#v(b)
  recvBit 0, b
  recvBit 1, b
  recvBit 2, b
  recvBit 3, b
  recvBit 4, b
  recvBit 5, b
  recvBit 6, b
  BTFSC GPIO, bit#v(b)
  INCF lengthLow + 7, F
  delay1
  MOVF after#v(b), W
  BTFSC GPIO, bit#v(b)
  MOVWF PCL
setup#v(b)
  MOVLW recvData#v(b)
  MOVWF recvData
  MOVLW low#v(b)
setupF#v(b)
  MOVWF lowChld
  MOVF recvSync, W
  MOVWF PCL

recvSync#v(b)
  BTFSC GPIO, bit#v(b)
  delay2
  delay1
  BTFSS GPIO, bit#v(b)
  delay2
  BTFSC GPIO, bit#v(b)
  delay2
  BTFSC GPIO, bit#v(b)
  GOTO sendData
  GOTO sendData

  endm

  unroll lowPageCode


initData0
  MOVLW 0  ; parent axis
  GOTO initData

initData1
  MOVLW 1  ; parent axis
  GOTO initData

prime
  ANDWF bits, W
  TRIS GPIO

  ; delay for 54 cycles
  MOVLW 0x11       ; 1
  MOVWF count      ; 1
  DECFSZ count, F  ; 1*16 + 2
  GOTO $-1         ; 2*16
  delay2

  MOVF bits, W
  TRIS GPIO
  INCF FSR, F
  RETLW 0

setupFinal
  CLRF more
  GOTO setupF0

initHelper
  MOVWF recvSync
  ADDWF delta, W
  MOVWF waitDReq
  RETLW 0


; --- high page (actually starts somewhere in sendData, can be anywhere after initHelper label) ---

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
  ANDWF lowPrnt, W
  TRIS GPIO         ; send "more" (low)
  COMF lowChld, W
  IORWF bits, W
  MOVWF bits
  TRIS GPIO         ; clear "more" and send "data request" or "sync initial high" to child
  delay2
  CLRF lengthLow + 0
  CLRF lengthLow + 1
  ANDWF lowChld, W
  TRIS GPIO         ; send "sync first falling" to child
  CLRF lengthLow + 2
  MOVF bits, W
  TRIS GPIO         ; send "sync first rising" to child
  ANDWF lowChld, W
  TRIS GPIO         ; send "sync second falling" to child
  MOVF bits, W
  TRIS GPIO         ; send "sync second rising" to child
  CLRF lengthLow + 3
  CLRF lengthLow + 4
  CLRF lengthLow + 5
  CLRF lengthLow + 6
  CLRF lengthLow + 7
  delay1
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
  BTFSS GPIO, bit3   ; 1 2 2 2 2
  GOTO found3        ; 2 0 0 0 0
  BTFSS GPIO, bit2   ; 0 1 2 2 2
  GOTO found2        ; 0 2 0 0 0
  BTFSS GPIO, bit1   ; 0 0 1 2 2
  GOTO found1        ; 0 0 2 0 0
  BTFSC GPIO, bit0   ; 0 0 0 2 1
  GOTO waitForPrime  ; 0 0 0 0 2

highPageCode macro b

  ; We get here 1.75-11.75 cycles after prime goes low
found#v(b)
  ; These lines need to take 35 cycles so we avoid confusing prime with data
  ; (36 for data+more, +1 for clock drift, -2 for the BTFSCs)
  CALL initData#v(b&1)   ; 23
  MOVLW low#v(b)         ;  1
  MOVWF lowPrnt          ;  1
  MOVLW init#v(b)        ;  1
  CALL initHelper        ;  9

  BTFSC GPIO, bit#v(b)
  GOTO waitForPrime
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

init#v(b)B
  MOVLW recvSync#v(b)
  GOTO init

waitDReq#v(b)B
  MOVF recvSync, W
  BTFSS GPIO, bit#v(b)
  GOTO $-1
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

init
  MOVWF recvSync
  MOVF waitDReq, W
  MOVWF PCL

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
  MOVLW (waitDReq0 - init0)
  MOVWF delta
  RETLW 0

  end

; 56 instructions free, 52 in low page





; Data request happens somewhere between 0.25 and 3.25

; 0  BTFSS GPIO, bit#v(b)
; 1  GOTO $-1                    TRIS GPIO
; 2
; 3  BTFSS GPIO, bit#v(b)
; 4
; 5  MOVWF PCL
; 6  BTFSC GPIO, bit#v(b)
; 7  delay2                      TRIS GPIO
; 8
; 9  delay1                      MOVF bits, W
;10  BTFSS GPIO, bit#v(b)        TRIS GPIO
;11  delay2                      ANDWF lowChld, W
;12                              TRIS GPIO
;13  BTFSC GPIO, bit#v(b)        MOVF bits, W
;14                              TRIS GPIO
;15  BTFSC GPIO, bit#v(b)
;16  GOTO sendData
