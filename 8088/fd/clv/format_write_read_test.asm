  %include "../../defaults_bin.asm"

; Want to be able to deal with speeds from 180rpm to 330rpm
; =182ms to 333ms = 3.3 to 6.06 timer ticks

  mov al,0x34
  out 0x43,al
  mov al,0
  out 0x40,al
  out 0x40,al
  xor ax,ax
  mov es,ax
  mov ds,ax
  mov di,0x20
  cli
  mov ax,irq0
  stosw
  mov ax,cs
  stosw
  mov ax,irq1
  stosw
  mov ax,cs
  stosw
  xor di,di
  mov ax,divideOverflow
  stosw
  mov ax,cs
  stosw
  lds si,[0x1e*4]
  mov word[es:0x1e*4],diskBaseTable
  mov word[es:0x1e*4+2],cs
  sti

  mov ax,cs
  mov es,ax
  mov cx,11
  mov di,diskBaseTable
  rep movsb
  mov ds,ax

  mov ax,1
  int 0x10


  mov al,0xb4
  out 0x43,al
  mov al,0
  out 0x42,al
  out 0x42,al

  in al,0x61
  and al,0xfc
  or al,1
  out 0x61,al

  mov ax,ds
  mov cl,4
  shl ax,cl
  add ax,sectorBuffer
  mov [bufferDMAAddress],ax
;  outputHex


  mov byte[sectorSize],6
  mov byte[lastSector],1

;  mov ax,0x0201
;  mov cx,0x0001
;  mov dx,0x0000
;  mov bx,sectorBuffer
;  int 0x13
  call motorOn


;  mov word[trackLength],3022
mainLoop:
  mov ax,[minTrackLength]
  add ax,[maxTrackLength]
  shr ax,1
  mov [trackLength],ax

;  add ax,1327  ; 663 ;
;  cmp ax,7193  ; 4096
;  jb trackLengthOk
;  sub ax,2149  ; 1074 ;
;trackLengthOk:
;  mov [trackLength],ax

  mov ax,0xb800
  mov es,ax
  mov ah,7
  mov di,40
  mov al,[trackLength + 1]
  shr al,cl
  xlatb
  stosw
  mov al,[trackLength + 1]
  and al,0x0f
  xlatb
  stosw
  mov al,[trackLength]
  shr al,cl
  xlatb
  stosw
  mov al,[trackLength]
  and al,0x0f
  xlatb
  stosw

  mov ax,[trackLength]
  outputHex
  outputCharacter ' '

  mov ax,cs
  mov es,ax


  mov ax,[bufferDMAAddress]
  out 0x04,al
  mov al,ah
  out 0x04,al

 ; call motorPWM

;  mov dx,0x3d9
;  mov al,1
;  out dx,al

  call breakFormat

;  mov word[interruptFunction], formatInterruptFunction
;  mov ax,cs
;  mov es,ax
;  mov ax,0x0501
;  mov cx,0x0000
;  mov dx,0x0000
;  mov bx,formatData0
;  int 0x13
;  jnc noErrorF
;  mov ax,0xb800
;  mov es,ax
;  mov di,0x12
;  mov al,'E'
;  mov ah,7
;  stosw
;  outputCharacter 'F'
;
;  call outputResult
;
;  outputCharacter 10
;
;  mov ah,0
;  mov dl,0
;  int 0x13
;;  jmp mainLoop
;
;  jmp doneFormat
;noErrorF:
;  mov ax,0xb800
;  mov es,ax
;  mov di,0x12
;  mov al,' '
;  mov ah,7
;  stosw
;  outputCharacter 'f'
;doneFormat:
  call enableMotor

;  call logDMAAddress
;  call dumpLog

;  mov dx,0x3d9
;  mov al,6
;  out dx,al

;  call motorPWM
;  mov dx,0x3d9
;  mov al,1
;  out dx,al
  outputCharacter '.'

   mov ax,0xf000
   mov ds,ax
   mov si,0x8000
   mov ax,cs
   mov es,ax
   mov di,sectorBuffer
   mov cx,[cs:trackLength]
   shr cx,1
   inc cx
   rep movsw
   mov ds,ax

  mov ax,[bufferDMAAddress]
  out 0x04,al
  mov al,ah
  out 0x04,al

  mov word[interruptFunction], writeInterruptFunction
;  mov ax,0xf000
  mov ax,cs
  mov es,ax
  mov ax,0x0301
  mov cx,0x0001
  mov dx,0x0000
  mov bx,sectorBuffer ;0x8000
  int 0x13
  jnc noErrorW
  mov ax,0xb800
  mov es,ax
  mov di,0x14
  mov al,'E'
  mov ah,7
  stosw
  outputCharacter 'W'

  call outputResult

  mov ah,0
  mov dl,0
  int 0x13
;  jmp mainLoop

  jmp doneWrite
noErrorW:
  mov ax,0xb800
  mov es,ax
  mov di,0x14
  mov al,' '
  mov ah,7
  stosw
  outputCharacter 'w'
doneWrite:

  mov dx,0x3d9
  mov al,11
  out dx,al

  call motorOn

;  call dumpLog

;   mov cx,2
;retryLoop:
;   push cx

  mov ax,cs
  mov es,ax
  mov di,sectorBuffer
  mov cx,[trackLength]
  shr cx,1
  inc cx
  xor ax,ax
  rep stosw

  mov word[interruptFunction], readInterruptFunction
  mov ax,0x0201
  mov cx,0x0001
  mov dx,0x0000
  mov bx,sectorBuffer
  int 0x13
  jnc noErrorR
  mov ax,0xb800
  mov es,ax
  mov di,0x16
  mov al,'E'
  mov ah,7
  stosw
  outputCharacter 'R'

  call outputResult

  mov ah,0
  mov dl,0
  int 0x13

  jmp doneRead
noErrorR:
  mov ax,0xb800
  mov es,ax
  mov di,0x16
  mov al,' '
  mov ah,7
  stosw
  outputCharacter 'r'
doneRead:
  mov word[interruptFunction], noInterruptFunction

;  call dumpLog

;   mov ax,0xb800
;   mov es,ax
;   mov di,3*80
;   mov si,sectorBuffer
;   mov cx,[trackLength]
;   shr cx,1
;   rep movsw

  mov ax,0xf000
  mov es,ax
  mov di,0x8000
  mov si,sectorBuffer
  mov cx,[trackLength]
checkLoop:
  mov al,[es:di]
  cmp al,[si]
  jne checkFail
  inc di
  inc si
  loop checkLoop

  mov ax,0xb800
  mov es,ax
  mov di,0x18
  mov al,' '
  mov ah,7
  stosw
  outputCharacter 'c'

  mov ax,[trackLength]
  mov [minTrackLength],ax

  jmp doneCheck
checkFail:
;   mov ah,[si]
;   outputHex

  push di
  mov ax,0xb800
  mov es,ax
  mov di,0x18
  mov al,'C'
  mov ah,7
  stosw
  outputCharacter 'C'
;  pop ax
;  outputHex
;  outputCharacter ' '
;  mov ax,cx
;  outputHex
;  outputCharacter ' '
;  mov ax,[trackLength]
;  outputHex
;  outputCharacter ' '
;  mov ax,[bufferDMAAddress]
;  outputHex
  mov ax,[trackLength]
  mov [maxTrackLength],ax

doneCheck:

  outputCharacter 10

;   pop cx
;   loop retryLoop2



  jmp mainLoop

;retryLoop2: jmp retryLoop

;hexDigit: db "0123456789ABCDEF"
lastTime: dw 0,0
timer: dw 0,0
currentPeriod: dw 0x0888
currentDutyCycle: dw 0x0777
pwmTime: dw 0x1235
trackLength: dw 6044
blipMotor: db 0
bufferDMAAddress: dw 0
interruptFunction: dw noInterruptFunction
;logPointer: dw logBuffer
minTrackLength: dw 6044
maxTrackLength: dw 8192


readInterruptFunction:
noInterruptFunction:
  ret


writeInterruptFunction:
;   call logDMAAddress
;  mov dx,0x3d9
;  mov al,7
;  out dx,al

  mov dx,[bufferDMAAddress]
  in al,0x04
  mov ah,al
  in al,0x04
  xchg al,ah
  sub ax,dx
  cmp ax,0x200
  jb .dmaNotRunningYet
  mov word[interruptFunction],writeInterruptFunction2
;  mov dx,0x3d9
;  mov al,8
;  out dx,al

.dmaNotRunningYet:
  ret

writeInterruptFunction2:
;   call logDMAAddress
;  mov dx,0x3d9
;  mov al,9
;  out dx,al

  mov cx,[bufferDMAAddress]
  add cx,[trackLength]
  add cx,3
  mov dx,0x3f2
.dmaLoop:
  in al,0x04
  mov ah,al
  in al,0x04
  xchg al,ah
  cmp ax,cx
  jb .dmaLoop
  ; If the DMA happened between the two reads we'll terminate the write too early - check again to be sure we're at the correct address
  in al,0x04
  mov ah,al
  in al,0x04
  xchg al,ah
  cmp ax,cx
  jb .dmaLoop
  mov al,0x0c
  out dx,al
  mov word[interruptFunction],noInterruptFunction
;  mov dx,0x3d9
;  mov al,10
;  out dx,al

  ret


;formatInterruptFunction:
;;   call logDMAAddress
;;  mov dx,0x3d9
;;  mov al,2
;;  out dx,al
;
;  mov dx,[bufferDMAAddress]
;  in al,0x04
;  mov ah,al
;  in al,0x04
;  xchg al,ah
;  sub ax,dx
;  cmp ax,4
;  jb .dmaNotRunningYet
;  mov word[interruptFunction],formatInterruptFunction3
;  call readTimerFromInterrupt
;  mov [formatStartedTime],ax
;  mov [formatStartedTime + 2],dx
;
;;  mov dx,0x3d9
;;  mov al,3
;;  out dx,al
;
;.dmaNotRunningYet:
;  ret
;
;
;;formatInterruptFunction2:
;;  mov dx,[bufferDMAAddress]
;;  in al,0x04
;;  mov ah,al
;;  in al,0x04
;;  xchg al,ah
;;  sub ax,dx
;;  cmp ax,4
;;  jb .dmaNotRunningYet
;;  mov word[interruptFunction],formatInterruptFunction3
;;  call readTimer
;;  mov [formatStartedTime],ax
;;  mov [formatStartedTime + 2],dx
;;.dmaNotRunningYet:
;;  ret
;
;
;formatStartedTime: dw 0,0
;
;
;formatInterruptFunction3:
;;   call logDMAAddress
;;  mov dx,0x3d9
;;  mov al,4
;;  out dx,al
;
;  call readTimerFromInterrupt
;  sub ax,[formatStartedTime]
;  sbb dx,[formatStartedTime + 2]
;  cmp dx,0
;  je .formatStillRunning             ; 54ms = 27% of a revolution
;  mov word[interruptFunction],noInterruptFunction
;  mov dx,0x3f2
;  mov al,0x0c
;  out dx,al
;
;;  mov dx,0x3d9
;;  mov al,5
;;  out dx,al
;
;.formatStillRunning:
;  ret



irq0:
  push dx
  push ax
  push ds

  mov ax,cs
  mov ds,ax

  push bx
  push cx
  push si
  call word[interruptFunction]
  pop si
  pop cx
  pop bx

motorByte:
  mov al,0x0c
  xor al,0x10
  mov [motorByte+1],al

;   shr al,1
;   mov dx,0x3d9
;   out dx,al
;   shl al,1

  cmp byte[blipMotor],0
  je doneBlip

  mov dx,0x3f2
  out dx,al

doneBlip:
   mov al,[motorByte+1]

  ; We are now counting down from the value in [dutyCycle+1]
  ; if [motorByte+1] is 0x1c the motor has been turned on
;  mov ax,[dutyCycle+1]
;  mov [currentPeriod],ax
  test al,0x10
  jz totalPeriod
  mov ax,[currentPeriod]
  mov [totalPeriod + 1],ax
  mov ax,[currentDutyCycle]
  mov [dutyCycle + 1],ax
  jmp totalPeriod  ; Flush prefetch queue (not needed on 8088)

totalPeriod:
  mov ax,0x0555
dutyCycle:
  sub ax,0x0111
  mov [dutyCycle+1],ax  ; Because we alternate between two periods, the new period we're programming is also the period that has just elapsed
  add [timer],ax
  adc word[timer+2],0

  cmp ax,0x100
  jb noUpdatePIT

  out 0x40,al
  mov al,ah
  out 0x40,al
noUpdatePIT:


  mov al,0x20
  out 0x20,al
  pop ds
  pop ax
  pop dx
  iret


divideOverflow:
  iret


irq1:
  sti
  push ax
  push ds
  mov ax,cs
  mov ds,ax

  in al,0x60

  cmp al,0x02  ; '1'
  jne .not1
  add word[currentPeriod],0x1000
  jmp doneKey
.not1:
  cmp al,0x10  ; 'Q'
  jne .notQ
  sub word[currentPeriod],0x1000
  jmp doneKey
.notQ:
  cmp al,0x03  ; '2'
  jne .not2
  add word[currentPeriod],0x0100
  jmp doneKey
.not2:
  cmp al,0x11  ; 'W'
  jne .notW
  sub word[currentPeriod],0x0100
  jmp doneKey
.notW:
  cmp al,0x04  ; '3'
  jne .not3
  add word[currentPeriod],0x0010
  jmp doneKey
.not3:
  cmp al,0x12  ; 'E'
  jne .notE
  sub word[currentPeriod],0x0010
  jmp doneKey
.notE:
  cmp al,0x05  ; '4'
  jne .not4
  add word[currentPeriod],0x0001
  jmp doneKey
.not4:
  cmp al,0x13  ; 'R'
  jne .notR
  sub word[currentPeriod],0x0001
  jmp doneKey
.notR:
  cmp al,0x1E  ; 'A'
  jne .notA
  add word[currentDutyCycle],0x1000
  jmp doneKey
.notA:
  cmp al,0x2C  ; 'Z'
  jne .notZ
  sub word[currentDutyCycle],0x1000
  jmp doneKey
.notZ:
  cmp al,0x1F  ; 'S'
  jne .notS
  add word[currentDutyCycle],0x0100
  jmp doneKey
.notS:
  cmp al,0x2D  ; 'X'
  jne .notX
  sub word[currentDutyCycle],0x0100
  jmp doneKey
.notX:
  cmp al,0x20  ; 'D'
  jne .notD
  add word[currentDutyCycle],0x0010
  jmp doneKey
.notD:
  cmp al,0x2E  ; 'C'
  jne .notC
  sub word[currentDutyCycle],0x0010
  jmp doneKey
.notC:
  cmp al,0x21  ; 'F'
  jne .notF
  add word[currentDutyCycle],0x0001
  jmp doneKey
.notF:
  cmp al,0x2F  ; 'V'
  jne .notV
  sub word[currentDutyCycle],0x0001
.notV:
doneKey:

  push es
  push di
  push bx
  push cx
  mov ax,0xb800
  mov es,ax

  mov ah,7
  mov bx,hexDigit
  mov cl,4

  mov di,80
  mov al,[currentPeriod + 1]
;  mov al,[totalPeriod + 2]
  shr al,cl
  xlatb
  stosw
  mov al,[currentPeriod + 1]
;  mov al,[totalPeriod + 2]
  and al,0x0f
  xlatb
  stosw
  mov al,[currentPeriod]
;  mov al,[totalPeriod + 1]
  shr al,cl
  xlatb
  stosw
  mov al,[currentPeriod]
;  mov al,[totalPeriod + 1]
  and al,0x0f
  xlatb
  stosw

  mov di,160
  mov al,[currentDutyCycle + 1]
;  mov al,[dutyCycle + 2]
  shr al,cl
  xlatb
  stosw
  mov al,[currentDutyCycle + 1]
;  mov al,[dutyCycle + 2]
  and al,0x0f
  xlatb
  stosw
  mov al,[currentDutyCycle]
;  mov al,[dutyCycle + 1]
  shr al,cl
  xlatb
  stosw
  mov al,[currentDutyCycle]
;  mov al,[dutyCycle + 1]
  and al,0x0f
  xlatb
  stosw

  pop cx
  pop bx
  pop di
  pop es

  in al,0x61
  or al,0x80
  out 0x61,al
  and al,0x7f
  out 0x61,al

  mov al,0x20
  out 0x20,al
  pop ds
  pop ax
  iret


readTimer:
  cli
  mov al,0x00
  out 0x43,al
  in al,0x40
  mov bl,al
  in al,0x40
  mov bh,al
  mov cx,[dutyCycle + 1]
  mov si,[totalPeriod + 1]
  mov ax,[timer]
  mov dx,[timer+2]
  sti
  cmp ax,[timer]
  je noCorrection

  ; If the timer wrapped after we latched it, the read value must be quite small (it happened in the time between the cli and the out instruction taking effect)
  ; Pick a value for timerWrapCheck that is definitely larger than the maximum possible such read value but smaller than the maximum period programmed into the PIT (minus the
  ; largest time from the timer wrapping to the latch taking effect). 20 PIT cycles should work fine - the actual value shouldn't be much over 5 even on the slowest machine, and a
  ; period of 40 (~30kHz) is not really usable on the slowest machines.
timerWrapCheck equ 20

  ; timer changed, figure out if PIT reloaded before or after we latched it
  cmp bx,timerWrapCheck
  jb noCorrection  ; We wrapped after the latch

  ; wrapped before latch - use the new period and timer value
  mov cx,[dutyCycle + 1]
  mov ax,[timer]
  mov dx,[timer+2]
noCorrection:
  sub si,cx              ; The value we're currently counting down from
  sub si,bx              ; The number of cycles we've counted down since the last wrap
  add ax,si              ; Add that to current time value
  adc dx,0
  ret


readTimerFromInterrupt:
  mov al,0x00
  out 0x43,al
  in al,0x40
  mov bl,al
  in al,0x40
  mov bh,al
  mov cx,[dutyCycle + 1]
  sub cx,bx
  mov ax,[timer]
  mov dx,[timer+2]
  add ax,cx
  adc dx,0
  ret



pwmStartTime:

motorPWM:
  mov byte[blipMotor],1
motorOn:
  call readTimer
  mov [pwmStartTime],ax
  mov [pwmStartTime+2],dx
pwmLoop:
;  mov dx,ax
;  mov ax,0xb800
;  mov es,ax
;
;  mov ah,7
;  mov bx,hexDigit
;  mov cl,4
;
;  mov di,40
;  mov al,dh
;;  mov al,[totalPeriod + 2]
;  shr al,cl
;  xlatb
;  stosw
;  mov al,dh
;;  mov al,[totalPeriod + 2]
;  and al,0x0f
;  xlatb
;  stosw
;  mov al,dl
;;  mov al,[totalPeriod + 1]
;  shr al,cl
;  xlatb
;  stosw
;  mov al,dl
;;  mov al,[totalPeriod + 1]
;  and al,0x0f
;  xlatb
;  stosw
  call readTimer
  sub ax,[pwmStartTime]
  sbb dx,[pwmStartTime+2]

  mov al,ah
  mov ah,dl
  cmp ax,[pwmTime]
  jb pwmLoop
  mov byte[blipMotor],0
enableMotor:
  mov dx,0x3f2
  mov al,0x1c
  out dx,al
  ret


showRPM:
  call readTimer

  mov bx,[lastTime]
  mov cx,[lastTime+2]
  mov [lastTime],ax
  mov [lastTime+2],dx
  sub ax,bx
  sbb dx,cx

  shr dx,1
  rcr ax,1
  shr dx,1
  rcr ax,1
  shr dx,1
  rcr ax,1
  mov cx,ax

  mov bp,0xb800
  mov es,bp
  xor di,di

  mov dx,0x3556  ; 0x3556e1dc = 894886364 ~= 6000*157500000/11/12/8
  mov ax,0xe1dc
  div cx

  mov bx,ax
  xor dx,dx
  mov cx,10000
  div cx
  add al,'0'
  mov ah,7
  stosw

  mov ax,dx
  xor dx,dx
  mov cx,1000
  div cx
  add al,'0'
  mov ah,7
  stosw

  mov ax,dx
  xor dx,dx
  mov cx,100
  div cx
  add al,'0'
  mov ah,7
  stosw

  mov al,'.'
  stosw

  mov ax,dx
  xor dx,dx
  mov cx,10
  div cx
  add al,'0'
  mov ah,7
  stosw

  mov al,dl
  add al,'0'
  stosw

  ret


outputResult:
  xor ax,ax
  mov es,ax
  mov ax,[es:0x441]
;  mov ah,1
;  int 0x13
  outputHex
  outputCharacter '.'
  mov ax,[es:0x443]
  outputHex
  outputCharacter '.'
  mov ax,[es:0x445]
  outputHex
  outputCharacter '.'
  mov ax,[es:0x447]
  outputHex
  outputCharacter '.'
  ret


;dumpLog:
;  mov ax,[logPointer]
;  sub ax,logBuffer
;  outputHex
;  outputCharacter '-'
;;  mov ax,
;  mov si,logBuffer
;.loop:
;  lodsw
;  sub ax,[bufferDMAAddress]
;  outputHex
;  outputCharacter ' '
;  cmp si,[logPointer]
;  jb .loop
;  mov word[logPointer],logBuffer
;  ret
;
;
;logDMAAddress:
;  mov si,[logPointer]
;  cmp si,logBuffer+0x800
;  je .logFull
;  in al,0x04
;  mov ah,al
;  in al,0x04
;  xchg al,ah
;  mov [si],ax
;
;  mov al,0x80
;  out 0x43,al
;  in al,0x42
;  mov ah,al
;  in al,0x42
;  xchg ah,al
;  mov [si+2],ax
;
;  add word[logPointer],4
;.logFull:
;  ret


commandCode:        db 0x40  ; +0x80 for MT, +0x40 for MF, +0x20 for SK
commandUnit:        db 0     ; unit (0..3) +0x04 for side 1
commandN:           db 6     ; sector size = 128 bytes * 2^N, or DTL when N==0
commandSC:          db 1     ; sectors per track (format command only)
commandGPL:         db 0x50  ; 0x5d  ; gap length
commandD:           db 0xf6  ; fill byte (format command only)
dmaMode:            db 0x46  ; DMA write (to RAM) mode (0x46) or read (from RAM) mode (0x4a)
dmaPage:            db 0
dmaOffset:          dw 0     ; dmaSegment:dmaOffset form a far pointer to our
dmaSegment:         dw 0     ; DMA address.
dmaLength:          dw 0     ; 1 = 1 byte, ..., 0xffff = 65535 bytes, 0 = 65536 bytes
dmaPointer:         dw 0,0


mainStatus: db 0

; The NEC D765AC datasheet says: After each byte of data read or written to
; the data register, CPU should way for 12us before reading main status
; register. This is not really necessary on PC/XT, but may be for faster
; machines.
delay12us:
  push ax
  mov al,0x80
  out 0x43,al
  in al,0x42
  mov bl,al
  in al,0x42
  mov bh,al
.loop:
  mov al,0x80
  out 0x43,al
  in al,0x42
  mov ah,al
  in al,0x42
  xchg al,ah

  sub ax,bx
  neg ax
  cmp ax,15
  jb .loop
  pop ax
  ret


; Write a byte to the 765. The byte to write is in AL.
; Assume DX=0x03f4.
write765:
;  call printByte
  call delay12us
  mov ah,al
  mov bx,[timer+2]
.wait:
  in al,dx
  and al,0xc0
  cmp al,0x80
  je .do
  mov cx,[timer+2]
  sub cx,bx
  cmp cx,2
  jb .wait

  outputString 10,"Timeout writing byte to 765"
   mov ax,[timer+2]
   outputHex
   mov ax,bx
   outputHex

  in al,dx
  mov [mainStatus],al
  call printMainStatus
  outputNewLine

   cli
   hlt
;  jmp [cleanup]
.do:
  mov al,ah
  inc dx
  out dx,al
  dec dx
  ret


commandFormatTrack:
;  outputString "Command Format Track: Writing"
  mov dx,0x03f4
  mov al,[commandCode]
  and al,0x40
  or al,0x0d
  call write765
  mov al,[commandUnit]
  call write765
  mov al,[commandN]
  call write765
  mov al,[commandSC]
  call write765
  mov al,[commandGPL]
  call write765
  mov al,[commandD]
  call write765
;  outputString ".",10
  ret


startDMA:
  mov al,0
  out 0x0c,al  ; Clear first/last flip-flop
  mov al,[dmaMode]  ; Channel 2, autoinit disabled, upwards, single
  out 0x0b,al  ; Set DMA mode
;  mov ax,[dmaOffset]
  mov ax,[bufferDMAAddress]
  sub ax,4
  out 0x04,al  ; Output DMA offset low
  mov al,ah
  out 0x04,al  ; Output DMA offset high
;  mov al,[dmaPage]
  mov ax,cs
  mov cl,4
  mov al,ah
  shr al,cl
  out 0x81,al  ; Output DMA page
;  mov ax,[dmaLength]
  mov ax,4
  dec ax
  out 0x05,al  ; Output DMA length low
  mov al,ah
  out 0x05,al  ; Output DMA length high
  mov al,2
  out 0x0a,al  ; Clear DMA mask bit for channel 2
  ret


dmaWait:
;  add di,[dmaOffset]
  add di,[bufferDMAAddress]
  sub di,4
.loop:
  in al,0x04
  mov ah,al
  in al,0x04
  xchg al,ah
  cmp ax,di
  jb .loop
  ret


;resetFDC:
;  mov dx,0x03f2
;  mov al,0x10
;;  mov cl,[driveNumber]
;;  shl al,cl
;  or al,8      ; Motor on, DMA/INT enabled, FDC reset
;  or al,cl
;  out dx,al
;  or al,4      ; Motor on, DMA/INT enabled, FDC not reset
;  out dx,al
;  ; According to the NEC uPD765A datasheet: "If the RDY input is held high
;  ; during reset, the FDC will generate an interrupt within 1.024ms. To
;  ; clear this interrupt, use the Sense Interrupt Status command."
;  ; The IBM controller has RDY tied to +5V, so we should expect this
;  ; interrupt.
;  mov word[operationCallback], seekCallback
;  mov byte[interrupted], 0
;  mov word[completionCallback], synchronousCallback
;  call waitInterrupt
;  test byte[resultST0],0xd8
;  jz .ok
;  outputString "Error resetting 765",10
;;  jmp [cleanup]
;.ok:
;  ret
;
;
;waitInterrupt:
;;  outputString "Waiting",10
;  mov bx,[timer+2]
;  sti
;.loop:
;  cmp byte[interrupted],0
;  jnz synchronousComplete
;  mov cx,[timer+2]
;  sub cx,bx
;  cmp cx,37  ; 2 seconds timeout
;  jb .loop
;  ; We timed out
;  outputString "Timeout waiting for 765 interrupt",10
;;  jmp [cleanup]
;
;synchronousComplete:
;  ret
;
;synchronousCallback:
;;  inc byte[cs:interrupted]
;  ret



; Format a track with a single 8192-byte sector.
; Interrupt the format before it is complete so that the track and sector
; headers are not overwritten by data.
breakFormat:
;  outputString "Performing break-format",10
  mov byte[dmaMode],0x4a
  call startDMA

;  mov es,[dmaPointer+2]
;  mov di,[dmaPointer]
;  mov ax,0x0000  ; H C
;  stosw
;  mov ax,0x0801  ; N R
;  stosw

  ; Start the format
;  mov byte[commandN],6 ;8
;  mov byte[commandSC],1
  call commandFormatTrack

  ; Wait until the FDC retreives the format data
  mov di,4
  call dmaWait

  ; Wait a bit longer so we're sure that we've finished the sector header but
  ; but that we haven't spun all the way around and overwritten the track
  ; header.
  mov bx,[timer+2]
waitLoop:
  mov cx,[timer+2]
  sub cx,bx
  cmp cx,2   ; 54-108ms = 27%-54% of a revolution
  jb waitLoop

;  call resetFDC
  mov ah,0
  mov dl,0
  int 0x13

  ret



printMainStatus:
  mov bl,[mainStatus]

  test bl,1
  jz .no_d0b
  outputString ", FDD 0 Busy"
.no_d0b:

  test bl,2
  jz .no_d1b
  outputString ", FDD 1 Busy"
.no_d1b:

  test bl,4
  jz .no_d2b
  outputString ", FDD 2 Busy"
.no_d2b:

  test bl,8
  jz .no_d3b
  outputString ", FDD 3 Busy"
.no_d3b:

  test bl,0x10
  jz .no_cb
  outputString ", FDC Busy"
.no_cb:

  test bl,0x20
  jz .no_exm
  outputString ", Execution Mode"
.no_exm:

  test bl,0x40
  jz .no_dio
  outputString ", Read"
  jmp .done_dio
.no_dio:
  outputString ", Write"
.done_dio:

  test bl,0x80
  jz .no_rqm
  outputString " Request for Master"
.no_rqm:
  ret




diskBaseTable:
specify1:           db 0xcf  ; SRT3 SRT2 SRT1 SRT0 HUT3 HUT2 HUT1 HUT0  step rate = 4ms  head unload time = 240ms
specify2:           db 0x02  ; HLT6 HLT5 HLT4 HLT3 HLT2 HLT1 HLT0   ND  head load time = 4ms  dma = yes
motorTimeout:       db 37    ; time in 18.2Hz ticks to wait before motor timeout (2 seconds)
sectorSize:         db 2     ; same as commandN
lastSector:         db 8     ; same as commandEOT
gapLength:          db 0x2a  ; same as commandGPL
dataLength:         db 0xff  ; same as commandDTL
gapLengthFormat:    db 0x50  ; same as commandGPL for format
fillByte:           db 0xf6  ; fill byte for format
headSettleTime:     db 25    ; milliseconds
motorStartTime:     db 4     ; units of 0.125 seconds




hexDigit: db "0123456789ABCDEF"

nop

formatData0:
;  db 0,0,1,2, 0,0,2,2, 0,0,3,2, 0,0,4,2, 0,0,5,2, 0,0,6,2, 0,0,7,2, 0,0,8,2, 0,0,9,2
  db 0,0,1,6  ; cylinder 0, head 0, sector 1, 8192 bytes
;  db 0,0,1,5  ; cylinder 0, head 0, sector 1, 4096 bytes


;section .bss

sectorBuffer: resb 8192
;logBuffer:
