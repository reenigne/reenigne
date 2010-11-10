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
catchUp           EQU 10h
recvData          EQU 11h
recvSync          EQU 12h
lowPrnt           EQU 13h
lowChld           EQU 14h
bits              EQU 15h
after0            EQU 16h
after1            EQU 17h
after2            EQU 18h
after3            EQU 19h
count             EQU 1ah
temp              EQU 1bh


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
  MOVLW low#v(b)
  CALL prime
  BTFSC GPIO, bit#v(b)
  RETLW 0
  DECF INDF, F
  BCF bits, bit#v(b)
  BTFSS GPIO, bit#v(b)  ; wait for prime complete
  GOTO $-1
  RETLW 0


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
  delay1
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
  BSF catchUp, 0
  MOVF recvSync, W
  MOVWF PCL

  CLRF catchUp
recvSync#v(b)
  BTFSS GPIO, bit#v(b)
  GOTO $-2
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

delay4
  RETLW 0

setupFinal
  CLRF more
  delay2
  GOTO setupF0

prime
  ANDWF bits, W
  TRIS GPIO
  MOVWF temp       ; 1        ; 0
  delay1           ; 1        ; 1
  MOVLW 0x0a       ; 1        ; 2
  MOVWF count      ; 1        ; 3
  MOVF bits, W     ; 1        ; 4
  TRIS GPIO        ; 1        ; 5

  DECFSZ count, F  ; 1*9 + 2  ; 6       0-6 = 6 cycles low
  GOTO $-1         ; 2*9

  MOVF temp, W     ; 1        ; 35
  TRIS GPIO        ; 1        ; 36
  delay2           ; 2        ; 37      6-37 = 31 cycles high
  delay1           ; 1        ; 39
  INCF FSR, F      ; 1        ; 40
  MOVF bits, W     ; 1        ; 41
  TRIS GPIO        ; 1        ; 42
                              ; 43      37-43 = 6 cycles low
  RETLW 0



; --- high page (actually starts somewhere in sendData, can be anywhere after prime label) ---

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
  TRIS GPIO         ; send M "more data to come" (low) to parent
  CLRF lengthLow + 0
  CLRF lengthLow + 1
  MOVF bits, W
  TRIS GPIO         ; send + "clear more" (high) to parent
  BTFSC catchUp, 0
  CALL delay4
  COMF lowChld, W
  IORWF bits, W
  TRIS GPIO         ; send R "data request" (high) to child
  MOVWF bits
  delay2
  ANDWF lowChld, W
  TRIS GPIO         ; send S "sync falling" (low) to child
  delay1
  MOVF bits, W
  TRIS GPIO         ; send T "sync rising" (high) to child
  ANDWF lowChld, W
  TRIS GPIO         ; send U "sync falling" (low) to child
  MOVF bits, W
  TRIS GPIO         ; send V "sync rising" (high) to child
  CLRF lengthLow + 2
  CLRF lengthLow + 3
  CLRF lengthLow + 4
  CLRF lengthLow + 5
  CLRF lengthLow + 6
  CLRF lengthLow + 7
  MOVF recvData, W
  MOVWF PCL

startup
  MOVWF OSCCAL           ; 1
  MOVLW 80h              ; 1    ; wake up on pin change disabled (80h) | weak pull-ups enabled (00h) | timer 0 clock source on instruction cycle (00h) | timer 0 source
  OPTION                 ; 1

reset
  MOVLW 1                ; 1
  MOVWF childBabsent     ; 1
  MOVWF childCabsent     ; 1
  MOVWF childDabsent     ; 1
  MOVLW childBabsent-1   ; 1
  MOVWF FSR              ; 1
waitForPrime
  COMF GPIO, W           ; 1       1        ; 0
  ANDLW 33h              ; 1       1        ; 1
  BTFSC STATUS, Z        ; 2       1        ; 2
  GOTO waitForPrime      ; 0       2        ; 4
  MOVWF temp             ; 1 1 1 1          ; 4
  BTFSC temp, bit3       ; 2 2 2 1          ; 5
  GOTO found3            ; 0 0 0 2          ; 7
  BTFSC temp, bit2       ; 2 2 1 0          ; 7
  GOTO found2            ; 0 0 2 0          ; 9
  BTFSC temp, bit1       ; 2 1 0 0          ; 9
  GOTO found1            ; 0 2 0 0          ; 11

highPageCode macro b

found#v(b)
  if b == 0
    delay1               ; 1 0 0 0          ; 11
  else if b == 2
    delay2               ; 0 0 2 0          ; 12
  else if b == 3
    CALL delay4          ; 0 0 0 4          ; 12
  endif

  BTFSS GPIO, bit#v(b)   ; 2                ; 12
  GOTO waitForPrime      ; 0                ; 14

  CALL initData#v(b&1)   ; 19               ; 14
  MOVLW recvSync#v(b)    ; 1                ; 33
  MOVWF recvSync         ; 1                ; 34
  MOVLW low#v(b)         ; 1                ; 35
  MOVWF lowPrnt          ; 1                ; 36
  MOVWF bits

  BTFSC GPIO, bit#v(b)                      ; 37
  TRIS GPIO
  GOTO waitForPrime
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

  endm

  unroll highPageCode

foundHelperB
  MOVF bits, W
  TRIS GPIO
  RETLW setupFinal

initData                   ; 2 + 1 + 2
  MOVWF parentAxis         ; 1
  if (length & 1)
    BSF lengthLow, 0
  else
    BCF lengthLow, 0       ; 1
  endif
  if ((length >> 1) & 1)
    BSF lengthMiddle, 0
  else
    BCF lengthMiddle, 0    ; 1
  endif
  if ((length >> 2) & 1)
    BSF lengthHigh, 0
  else
    BCF lengthHigh, 0      ; 1
  endif
  BSF more, 0              ; 1
  CLRF switch              ; 1
  BTFSC GPIO, 2            ; 1
  INCF switch, F           ; 1
  CLRF GPIO                ; 1
  delay2                   ; 2
  RETLW 0                  ; 2

  end

; 77 instructions free,
