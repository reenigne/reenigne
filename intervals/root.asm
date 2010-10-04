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
childBpresent     EQU 0ch
childCpresent     EQU 0dh
childDpresent     EQU 0eh
more              EQU 0fh
recvData          EQU 10h
recvUSync         EQU 11h
recvVSync         EQU 12h
setup             EQU 13h
lowPrnt           EQU 14h
lowChld           EQU 15h
bits              EQU 16h
after0            EQU 17h
after1            EQU 18h
after2            EQU 19h
after3            EQU 1ah
count             EQU 1bh
waitDReq          EQU 1ch
delta1            EQU 1dh
delta2            EQU 1eh


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
  MOVLW 0
  BTFSC GPIO, bit#v(b)
  MOVLW 1
  TRIS 5
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
reset                ; Don't move this label - it's also used to set "more"
  GOTO resetB
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
  CALL delay5
  recvBit 0, b
  recvBit 1, b
  recvBit 2, b
  recvBit 3, b
  recvBit 4, b
  recvBit 5, b
  recvBit 6, b
  recvBit 7, b
  MOVF after#v(b), W
  BTFSC GPIO, bit#v(b)
  MOVWF setup
  GOTO sendVSync

setup#v(b)
  BSF bits, bit#v(b)
  MOVLW recvData#v(b)
  MOVWF recvData
  MOVLW low#v(b)
  MOVWF lowChld
  MOVF recvUSync, W
  MOVWF PCL

recvUSync#v(b)
  BTFSC GPIO, bit#v(b)
  delay2
  BTFSC GPIO, bit#v(b)
  GOTO sendData
  GOTO sendData

recvVSync#v(b)
  BTFSC GPIO, bit#v(b)
  delay2
  BTFSC GPIO, bit#v(b)
  GOTO sendUSync
  GOTO sendUSync

  endm

  unroll lowPageCode


initData0
  MOVLW 0  ; parent axis
  GOTO initData

initData1
  MOVLW 1  ; parent axis
  GOTO initData

delay5     ; used
  delay1
  RETLW 0

prime
  ANDWF bits, W
  TRIS GPIO

  ; delay for 46 + 6 cycles (can't use a subroutine - already have prime#v(b) and found#v(b) on the stack)
  MOVLW 0x11       ; 1
  MOVWF count      ; 1
  DECFSZ count, F  ; 1*16 + 2
  GOTO $-1         ; 2*16

  MOVF bits, W
  TRIS GPIO
  INCF FSR, F
  RETLW 0

setupFinal
  delay2
  MOVLW reset
  MOVWF recvVSync  ; Check that low bit is 1 so that "more" is set correctly
  MOVWF more
  MOVF recvUSync, W
  MOVWF PCL

initHelper
  MOVWF recvUSync
  ADDWF delta1, W
  MOVWF recvVSync
  ADDWF delta2, W
  MOVWF waitDReq
  RETLW 0


; --- high page (actually starts somewhere in initHelper) ---

sendData
  sendBit 0
  sendBit 1
  sendBit 2
  sendBit 3
  sendBit 4
  sendBit 5
  sendBit 6
  sendBit 7
  sendBit 8
  delay2
  MOVF bits, W
  TRIS GPIO
  delay1
  MOVF recvVSync, W
  MOVWF PCL

sendVSync
  MOVF bits, W
  ANDWF lowChld, W
  TRIS GPIO
  MOVF bits, W
  TRIS GPIO
  MOVF setup, W
  MOVWF PCL

sendUSync
  MOVF bits, W
  TRIS GPIO
  delay3
  ANDWF lowChld, W
  TRIS GPIO
  MOVF bits, W
  TRIS GPIO
  MOVF recvData, W
  MOVWF PCL

startup
  MOVWF OSCCAL
  MOVLW 80h                  ; wake up on pin change disabled (80h) | weak pull-ups enabled (00h) | timer 0 clock source on instruction cycle (00h) | timer 0 source
  OPTION
  CLRF GPIO
  MOVLW highAll
  MOVWF bits
  MOVLW (recvVSync0 - init0)
  MOVWF delta1
  MOVLW (waitDReq0 - recvVSync0)
  MOVWF delta2

resetB
  CLRF childBpresent     ; 1
  CLRF childCpresent     ; 1
  CLRF childDpresent     ; 1
  MOVLW childBpresent-1  ; 1
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

highPageCode macro b
  local waitForPrimeComplete

  ; We get here 1.75-11.75 cycles after prime goes low
found#v(b)
  ; These lines need to take 35 cycles so we avoid confusing prime with data
  ; (36 for data+more, +1 for clock drift, -2 for the BTFSCs)
  CALL initData#v(b&1)   ; 23
  MOVLW low#v(b)         ;  1
  MOVWF lowPrnt          ;  1
  MOVLW init#v(b)        ;  1
  CALL initHelper        ;  9

  NOP
  NOP
  BCF bits, bit#v(b)
  CALL prime#v((b+1)&3)
  CALL prime#v((b+2)&3)
  CALL prime#v((b+3)&3)
  BSF bits, bit#v(b)
  CALL foundHelper
  MOVWF after#v((b+3)&3)
  BTFSC childDpresent, 0
  MOVLW setup#v((b+3)&3)
  MOVWF after#v((b+2)&3)
  BTFSC childCpresent, 0
  MOVLW setup#v((b+2)&3)
  MOVWF after#v((b+1)&3)
  BTFSC childBpresent, 0
  MOVLW setup#v((b+1)&3)
  MOVWF PCL

init#v(b)B
  MOVLW recvUSync#v(b)
  GOTO init

waitDReq#v(b)B
  MOVF recvUSync, W
waitDReq#v(b)C
  BTFSS GPIO, bit#v(b)
  GOTO waitDReq#v(b)C
  MOVWF PCL

prime#v(b)B
  MOVLW low#v(b)
  CALL prime
  BTFSC GPIO, bit#v(b)
  RETLW 0
  INCF INDF, F
  BCF bits, bit#v(b)
waitForPrimeComplete
  BTFSS GPIO, bit#v(b)
  GOTO waitForPrimeComplete
  RETLW 0

  endm

  unroll highPageCode

init
  MOVWF recvUSync
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
  CLRF switch
  BTFSC GPIO, 2
  INCF switch, F
  CLRF more
  delay2
  CALL delay5
  RETLW 0

  end
