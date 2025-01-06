  %include "../../defaults_bin.asm"

; Want to be able to deal with speeds from 180rpm to 330rpm
; =182ms to 333ms = 3.3 to 6.06 timer ticks

  xor ax,ax
  mov es,ax
  mov ds,ax
  mov di,0x20
  cli
  mov ax,irq0
  stosw
  mov ax,cs
  stosw
;  mov di,0x24
  mov ax,irq1
  stosw
  mov ax,cs
  stosw
  xor di,di
  mov ax,divideOverflow
  stosw
  mov ax,cs
  stosw
;  lds si,[0x1e*4]
;  mov word[es:0x1e*4],diskBaseTable
;  mov word[es:0x1e*4+2],cs

  mov al,0x34
  out 0x43,al
  mov ax,[cs:irqPeriod]
  out 0x40,al
  mov al,ah
  out 0x40,al

  sti

;  mov ax,cs
;  mov es,ax
;  mov cx,11
;  mov di,diskBaseTable
;  rep movsb
;  mov ds,ax

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

  mov word[trackLength],6221

  call breakFormat

  call breakWrite


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
  mov ax,[onPeriod]
  outputHex
  outputCharacter ' '
  mov ax,[offPeriod]
  outputHex
  outputCharacter ' '

  call slowMotor

  call breakRead

  call

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
  mov ax,[trackLength]
  mov [maxTrackLength],ax

doneCheck:

  outputCharacter 10




  jmp mainLoop

lastTime: dw 0,0
trackLength: dw 6044
bufferDMAAddress: dw 0
interruptFunction: dw noInterruptFunction
minTrackLength: dw 6044
maxTrackLength: dw 8192
onPeriod: dw 100
offPeriod: dw 100
irqPeriod: dw 0x800


readInterruptFunction:
noInterruptFunction:
  ret


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

  mov ax,[irqPeriod]
  add [timer],ax
  adc word[timer+2],0

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
  add word[onPeriod],0x1000
  jmp doneKey
.not1:
  cmp al,0x10  ; 'Q'
  jne .notQ
  sub word[onPeriod],0x1000
  jmp doneKey
.notQ:
  cmp al,0x03  ; '2'
  jne .not2
  add word[onPeriod],0x0100
  jmp doneKey
.not2:
  cmp al,0x11  ; 'W'
  jne .notW
  sub word[onPeriod],0x0100
  jmp doneKey
.notW:
  cmp al,0x04  ; '3'
  jne .not3
  add word[onPeriod],0x0010
  jmp doneKey
.not3:
  cmp al,0x12  ; 'E'
  jne .notE
  sub word[onPeriod],0x0010
  jmp doneKey
.notE:
  cmp al,0x05  ; '4'
  jne .not4
  add word[onPeriod],0x0001
  jmp doneKey
.not4:
  cmp al,0x13  ; 'R'
  jne .notR
  sub word[onPeriod],0x0001
  jmp doneKey
.notR:
  cmp al,0x1E  ; 'A'
  jne .notA
  add word[offPeriod],0x1000
  jmp doneKey
.notA:
  cmp al,0x2C  ; 'Z'
  jne .notZ
  sub word[offPeriod],0x1000
  jmp doneKey
.notZ:
  cmp al,0x1F  ; 'S'
  jne .notS
  add word[offPeriod],0x0100
  jmp doneKey
.notS:
  cmp al,0x2D  ; 'X'
  jne .notX
  sub word[offPeriod],0x0100
  jmp doneKey
.notX:
  cmp al,0x20  ; 'D'
  jne .notD
  add word[offPeriod],0x0010
  jmp doneKey
.notD:
  cmp al,0x2E  ; 'C'
  jne .notC
  sub word[offPeriod],0x0010
  jmp doneKey
.notC:
  cmp al,0x21  ; 'F'
  jne .notF
  add word[offPeriod],0x0001
  jmp doneKey
.notF:
  cmp al,0x2F  ; 'V'
  jne .notV
  sub word[offPeriod],0x0001
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
  mov al,[onPeriod + 1]
  shr al,cl
  xlatb
  stosw
  mov al,[onPeriod + 1]
  and al,0x0f
  xlatb
  stosw
  mov al,[onPeriod]
  shr al,cl
  xlatb
  stosw
  mov al,[onPeriod]
  and al,0x0f
  xlatb
  stosw

  mov di,160
  mov al,[offPeriod + 1]
  shr al,cl
  xlatb
  stosw
  mov al,[offPeriod + 1]
  and al,0x0f
  xlatb
  stosw
  mov al,[offPeriod]
  shr al,cl
  xlatb
  stosw
  mov al,[offPeriod]
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

  mov word[minTrackLength],6044
  mov word[maxTrackLength],8192

  pop ds
  pop ax
  iret


slowStartTime: dw 0,0

slowMotor:
  mov dx,0x3f2
  mov al,0x1c
  out dx,al
  mov dx,0x3d9
  mov al,1
  out dx,al

  call readTimer
  mov [slowStartTime],ax
  mov [slowStartTime+2],dx
slowLoopOn:
  call readTimer
  sub ax,[slowStartTime]
  sbb dx,[slowStartTime+2]
  shr dx,1
  rcr ax,1
  shr dx,1
  rcr ax,1
  shr dx,1
  rcr ax,1
  shr dx,1
  rcr ax,1
  shr dx,1
  rcr ax,1
  cmp ax,[onPeriod]
  jb slowLoopOn

  mov dx,0x3f2
  mov al,0x0c
  out dx,al
  mov dx,0x3d9
  mov al,2
  out dx,al

  call readTimer
  mov [slowStartTime],ax
  mov [slowStartTime+2],dx
slowLoopOff:
  call readTimer
  sub ax,[slowStartTime]
  sbb dx,[slowStartTime+2]
  shr dx,1
  rcr ax,1
  shr dx,1
  rcr ax,1
  shr dx,1
  rcr ax,1
  shr dx,1
  rcr ax,1
  shr dx,1
  rcr ax,1
  cmp ax,[offPeriod]
  jb slowLoopOff

  mov dx,0x3f2
  mov al,0x1c
  out dx,al
  mov dx,0x3d9
  mov al,4
  out dx,al
  ret


motorOn:
  mov dx,0x3f2
  mov al,0x1c
  out dx,al

  call readTimer
  mov [slowStartTime],ax
  mov [slowStartTime+2],dx
motorOnLoop:
  call readTimer
  sub ax,[slowStartTime]
  sbb dx,[slowStartTime+2]
  cmp dx,37
  jb motorOnLoop
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
commandC:           db 0     ; cylinder
commandH:           db 0     ; head
commandR:           db 1     ; sector number
commandN:           db 6     ; sector size = 128 bytes * 2^N, or DTL when N==0
commandEOT:         db 8     ; end of track (last sector number)
commandGPL:         db 0x50  ; 0x5d  ; gap length
commandDTL:         db 0     ; data length in bytes for N==0

commandSC:          db 1     ; sectors per track (format command only)
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


standardCommand:
  or al,[commandCode]
standardCommand2:
  mov dx,0x03f4
  mov word[operationCallback],standardCallback
  call write765
  mov al,[commandUnit]
  call write765
  mov al,[commandC]
  call write765
  mov al,[commandH]
  call write765
  mov al,[commandR]
  call write765
  mov al,[commandN]
  call write765
  mov al,[commandEOT]
  call write765
  mov al,[commandGPL]
  call write765
  mov al,[commandDTL]
  call write765
;  outputString ".",10
  ret

commandReadData:
;  outputString "Command Read Data: Writing"
  mov al,0x06
  jmp standardCommand

;commandReadDeletedData:
;  outputString "Command Read Deleted Data: Writing"
;  mov al,0x0c
;  jmp standardCommand

commandWriteData:
;  outputString "Command Write Data: Writing"
  mov al,[commandCode]
  and al,0xc0
  or al,0x05
  jmp standardCommand2

;commandWriteDeletedData:
;  outputString "Command Write Deleted Data: Writing"
;  mov al,[commandCode]
;  and al,0xc0
;  or al,0x09
;  jmp standardCommand2
;
;commandReadTrack:
;  outputString "Command Read Track: Writing"
;  mov al,[commandCode]
;  and al,0x60
;  or al,0x02
;  jmp standardCommand2
;
;commandReadID:
;  outputString "Command Read ID: Writing"
;  mov dx,0x03f4
;  mov al,[commandCode]
;  and al,0x40
;  or al,0x0a
;  call write765
;  mov al,[commandUnit]
;  call write765
;  outputString ".",10
;  ret

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

;commandScanEqual:
;  outputString "Command Scan Equal: Writing"
;  mov al,0x11
;  jmp standardCommand
;
;commandScanLowOrEqual:
;  outputString "Command Scan Low Or Equal: Writing"
;  mov al,0x19
;  jmp standardCommand
;
;commandScanHighOrEqual:
;  outputString "Command Scan High Or Equal: Writing"
;  mov al,0x1d
;  jmp standardCommand
;
;commandRecalibrate:
;  outputString "Command Recalibrate: Writing"
;  mov word[operationCallback],seekCallback
;  mov dx,0x03f4
;  mov al,0x07
;  call write765
;  mov al,[commandUnit]
;  and al,0x03
;  call write765
;  outputString ".",10
;  ret
;
;commandSenseInterruptStatus:
;  outputString "Command Sense Interrupt Status: Writing"
;  mov dx,0x03f4
;  mov al,0x08
;  call write765
;  outputString ". "
;  call getResults
;  outputString "Result: "
;  call printST0
;  outputString ", PCN="
;  mov bl,[resultPCN]
;  call printByte2
;  outputString ".",10
;  ret
;
;commandSpecify:
;  outputString "Command Specify: Writing"
;  mov dx,0x03f4
;  mov al,0x03
;  call write765
;  mov al,[specify1]
;  call write765
;  mov al,[specify2]
;  call write765
;  outputString ".",10
;  ret
;
;commandSenseDriveStatus:
;  outputString "Command Sense Drive Status: Writing"
;  mov dx,0x03f4
;  mov al,0x04
;  call write765
;  outputString ". "
;  call getResults
;  outputString "Result: "
;  call printST3
;  outputNewLine
;  ret
;
;commandSeek:
;  outputString "Command Seek: Writing"
;  mov word[operationCallback],seekCallback
;  mov dx,0x03f4
;  mov al,0x0f
;  call write765
;  mov al,[commandUnit]
;  call write765
;  mov al,[commandNCN]
;  call write765
;  outputString ".",10
;  ret
;
;commandVersion:
;  outputString "Command Version: Writing"
;  mov dx,0x03f4
;  mov al,0x10
;  call write765
;  outputString ". "
;  call getResults
;  outputString "Result: "
;  cmp byte[resultST0],0x80
;  jne .not_v765a
;  outputString "765A/A-2",10
;  ret
;.not_v765a:
;  cmp byte[resultST0],0x90
;  je .v765b
;  outputString "Unknown 765 version",10
;  jmp [cleanup]
;.v765b:
;  outputString "765B",10
;  ret



startDMA:
  mov al,0
  out 0x0c,al  ; Clear first/last flip-flop
  mov al,[dmaMode]  ; Channel 2, autoinit disabled, upwards, single
  out 0x0b,al  ; Set DMA mode
;  mov ax,[dmaOffset]
  mov ax,[bufferDMAAddress]
;  sub ax,4
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
  mov ax,cs
  mov es,ax
  mov di,sectorBuffer
  mov si,formatData0
  movsw
  movsw

   call slowMotor

;  outputString "Performing break-format",10
  mov byte[dmaMode],0x4a
  call startDMA

;  mov es,[dmaPointer+2]
;  mov di,[dmaPointer]
;  mov ax,0x0000  ; H C
;  stosw
;  mov ax,0x0601  ; N R
;  stosw

  ; Start the format
  mov byte[commandN],6
  mov byte[commandSC],1
  call commandFormatTrack

  ; Wait until the FDC retreives the format data
  mov di,.loop
  mov ax,

  mov di,4
  add di,[bufferDMAAddress]
.loop:
  in al,0x04
  mov ah,al
  in al,0x04
  xchg al,ah
  cmp ax,di
  jb .loop



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


breakWrite:
   mov cx,[trackLength]
   mov ax,0xf000
   mov ds,ax
   mov si,0x8000
   mov ax,cs
   mov es,ax
   mov di,sectorBuffer
   shr cx,1
   inc cx
   rep movsw
   mov ds,ax

   call slowMotor

  mov byte[dmaMode],0x4a
  call startDMA
  ; Start the write
  call commandWriteData

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

  mov ah,0
  mov dl,0
  int 0x13

  mov dx,0x3f2
  mov al,0x1c
  out dx,al

  ret


breakRead:
  mov ax,cs
  mov es,ax
  mov di,sectorBuffer
  mov cx,[trackLength]
  shr cx,1
  inc cx
  xor ax,ax
  rep stosw

   call slowMotor

  mov byte[dmaMode],0x46
  call startDMA
  ; Start the write
  call commandWriteData

;  mov word[interruptFunction], readInterruptFunction
;  mov ax,0x0201
;  mov cx,0x0001
;  mov dx,0x0000
;  mov bx,sectorBuffer
;  int 0x13
;  jnc noErrorR
;  mov ax,0xb800
;  mov es,ax
;  mov di,0x16
;  mov al,'E'
;  mov ah,7
;  stosw
;  outputCharacter 'R'
;
;  call outputResult
;
;  mov ah,0
;  mov dl,0
;  int 0x13
;
;  jmp doneRead
;noErrorR:
;  mov ax,0xb800
;  mov es,ax
;  mov di,0x16
;  mov al,' '
;  mov ah,7
;  stosw
;  outputCharacter 'r'
;doneRead:
;  mov word[interruptFunction], noInterruptFunction


printST0:
  outputString "Unit="
  mov bl,[resultST0]
  mov al,bl
  and al,3
  add al,'0'
  outputCharacter

  outputString ", Head="
  test bl,4
  jnz .head1
  outputCharacter '0'
  jmp .headDone
.head1
  outputCharacter '1'
.headDone:

  test bl,8
  jz .no_nr
  outputString ", Not Ready"
.no_nr:

  test bl,0x10
  jz .no_ec
  outputString ", Equipment Check"
.no_ec:

  test bl,0x20
  jz .no_se
  outputString ", Seek End"
.no_se:

  test bl,0x40
  jnz .bitsx1
  test bl,0x80
  jnz .bits10
  outputString ", Normal Termination"
  ret
.bits10:
  outputString ", Invalid Command"
  ret
.bitsx1:
  test bl,0x80
  jnz .bits11
  outputString ", Abnormal Termination"
  ret
.bits11:
  outputString ", Ready Changed"
  ret


printST1:
  mov bl,[resultST1]
  test bl,1
  jz .no_ma
  outputString ", Missing Address Mark"
.no_ma:

  test bl,2
  jz .no_nw
  outputString ", Not Writable"
.no_nw:

  test bl,4
  jz .no_nd
  outputString ", No Data"
.no_nd:

  test bl,0x10
  jz .no_or
  outputString ", Overrun"
.no_or:

  test bl,0x20
  jz .no_de
  outputString ", Data Error"
.no_de:

  test bl,0x80
  jz .no_en
  outputString ", End of Cylinder"
.no_en:
  ret


printST2:
  mov bl,[resultST2]
  test bl,1
  jz .no_md
  outputString ", Missing Address Mark in Data Field"
.no_md:

  test bl,2
  jz .no_bc
  outputString ", Bad Cylinder"
.no_bc:

  test bl,4
  jz .no_sn
  outputString ", Scan Not Satisfied"
.no_sn:

  test bl,8
  jz .no_sh
  outputString ", Scan Equal Hit"
.no_sh:

  test bl,0x10
  jz .no_wc
  outputString ", Wrong Cylinder"
.no_wc:

  test bl,0x20
  jz .no_dd
  outputString ", Data Error in Data Field"
.no_dd:

  test bl,0x40
  jz .no_cm
  outputString ", Control Mark"
.no_cm:
  ret


printST3:
  outputString "Unit="
  mov bl,[resultST3]
  mov al,bl
  and al,3
  add al,'0'
  outputCharacter

  outputString ", Head="
  test bl,4
  jnz .head1
  outputCharacter '0'
  jmp .headDone
.head1
  outputCharacter '1'
.headDone:

  test bl,8
  jz .no_ts
  outputString ", Two-Side"
.no_ts:

  test bl,0x10
  jz .no_t0
  outputString ", Track 0"
.no_t0:

  test bl,0x20
  jz .no_ry
  outputString ", Ready"
.no_ry:

  test bl,0x40
  jz .no_wp
  outputString ", Write Protected"
.no_wp

  test bl,0x80
  jz .no_ft
  outputString ", Fault"
.no_ft:
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


getResults:
  mov ax,cs
  mov es,ax
  mov dx,0x03f4
  outputString "Read"
  mov di,results
  mov cx,7
.getResult:
  call delay12us
  mov bx,[time]
.wait:
  in al,dx
  and al,0xc0
  cmp al,0xc0
  je .do
  cmp al,0x80   ; The IBM BIOS checks bit 4 here instead to see if the read is complete. Not sure if this matters.
  je .done
  mov ax,[time]
  sub ax,bx
  cmp ax,2
  jb .wait
  outputString "Timeout reading byte from 765",10
  jmp [cleanup]
.do:
  inc dx
  in al,dx
  dec dx
  stosb
  push cx
  call printByte
  pop cx
  loop .getResult
.done:
  outputString ".",10
  ret


standardCallback:
  push bx
  push dx
  call getResults
  outputString "Result: "
  call printST0
  call printST1
  call printST2
  outputString ", C="
  mov bl,[resultC]
  call printByte2
  outputString ", H="
  mov bl,[resultH]
  call printByte2
  outputString ", R="
  mov bl,[resultR]
  call printByte2
  outputString ", N="
  mov bl,[resultN]
  call printByte2
  outputString ".",10
  pop dx
  pop bx
  ret


; On entry: location for timeOut in di
; time until timeout (in 54ms ticks) in al
setTimeout:
  cmp byte[timeOutExpired],0
  je .noRestore
  mov si,[timeOutLocation]
  mov bl,[timeOutRestore]
  mov [si],bl
  mov byte[timeOutExpired],0
.noRestore:
  mov [timeOutLocation],di
  mov [timeOutRemaining],al
  mov al,[di]
  mov [timeOutRestore],al
  ret

timeOutExpired: db 0
timeOutLocation: dw 0
timeOutRestore: db 0
timeOutRemaining: db 0


interrupt8:
  push ds
  mov ds,[cs:savedCS]
  inc word[timer+2]
  dec byte[timeOutRemining]
  jnz .noTimeout
  push di
  mov di,[timeOutLocation]
  mov byte[di],0xc3  ; 'ret'
  mov byte[timeOutExpired],1
  pop di
.noTimeout:
  pop ds
  jmp far [cs:oldInterrupt8]

savedCS: dw 0
oldInterrupt8: dw 0,0
timer: dw 0


; Interrupt E hanadler
interruptE:
  push ds
  mov ds,[cs:savedCS]

  mov al,0x00
  out 0x43,al
  in al,0x40
  mov bl,al
  in al,0x40
  mov bh,al
  mov dx,[timer]
  sti
  cmp dx,[timer]
  je .noCorrection

  ; If the timer wrapped after we latched it, the read value must be quite small (it happened in the time between the cli and the out instruction taking effect)
  ; Pick a value for timerWrapCheck that is definitely larger than the maximum possible such read value but smaller than the maximum period programmed into the PIT (minus the
  ; largest time from the timer wrapping to the latch taking effect). 20 PIT cycles should work fine - the actual value shouldn't be much over 5 even on the slowest machine, and a
  ; period of 40 (~30kHz) is not really usable on the slowest machines.
timerWrapCheck equ 20

  ; timer changed, figure out if PIT reloaded before or after we latched it
  cmp bx,timerWrapCheck
  jb noCorrection  ; We wrapped after the latch

  ; wrapped before latch - use the new period and timer value
  mov si,[irqPeriod]
  mov ax,[timer]
  mov dx,[timer+2]
noCorrection:
  sub si,bx              ; The number of cycles we've counted down since the last wrap
  add ax,si              ; Add that to current time value
  adc dx,0
  ret

  pop ds
;  push ax
;  push si
;  push cx
;  outputString "Interrupt received",10
;  mov al,0x20
;  out 0x20,al
;  call word[cs:operationCallback]
;  pop cx
;  pop si
;  pop ax
;  call word[cs:completionCallback]
;  iret
  jmp far [cs:oldInterruptE]

oldInterruptE: dw 0,0
interruptETime: dw 0,0


readTimer:
  sti
  cli
  mov al,0x00
  out 0x43,al
  in al,0x40
  mov bl,al
  in al,0x40
  mov bh,al
  mov dx,[timer]
  sti
  cmp dx,[timer]
  je .noCorrection

  ; If the timer wrapped after we latched it, the read value must be quite small (it happened in the time between the cli and the out instruction taking effect)
  ; Pick a value for timerWrapCheck that is definitely larger than the maximum possible such read value but smaller than the maximum period programmed into the PIT (minus the
  ; largest time from the timer wrapping to the latch taking effect). 20 PIT cycles should work fine - the actual value shouldn't be much over 5 even on the slowest machine, and a
  ; period of 40 (~30kHz) is not really usable on the slowest machines.
timerWrapCheck equ 20

  ; timer changed, figure out if PIT reloaded before or after we latched it
  cmp bx,timerWrapCheck
  jb .noCorrection  ; We wrapped after the latch

  ; wrapped before latch - use the new period and timer value

  mov si,[irqPeriod]
  mov ax,[timer]
  mov dx,[timer+2]
.noCorrection:
  neg bx
  xchg ax,bx
  ret


;readTimerFromInterrupt:
;  mov al,0x00
;  out 0x43,al
;  in al,0x40
;  mov bl,al
;  in al,0x40
;  mov bh,al
;  mov cx,[irqPeriod]
;  sub cx,bx
;  mov ax,[timer]
;  mov dx,[timer+2]
;  add ax,cx
;  adc dx,0
;  ret








;diskBaseTable:
;specify1:           db 0xcf  ; SRT3 SRT2 SRT1 SRT0 HUT3 HUT2 HUT1 HUT0  step rate = 4ms  head unload time = 240ms
;specify2:           db 0x02  ; HLT6 HLT5 HLT4 HLT3 HLT2 HLT1 HLT0   ND  head load time = 4ms  dma = yes
;motorTimeout:       db 37    ; time in 18.2Hz ticks to wait before motor timeout (2 seconds)
;sectorSize:         db 2     ; same as commandN
;lastSector:         db 8     ; same as commandEOT
;gapLength:          db 0x2a  ; same as commandGPL
;dataLength:         db 0xff  ; same as commandDTL
;gapLengthFormat:    db 0x50  ; same as commandGPL for format
;fillByte:           db 0xf6  ; fill byte for format
;headSettleTime:     db 25    ; milliseconds
;motorStartTime:     db 4     ; units of 0.125 seconds




hexDigit: db "0123456789ABCDEF"

nop

formatData0:
;  db 0,0,1,2, 0,0,2,2, 0,0,3,2, 0,0,4,2, 0,0,5,2, 0,0,6,2, 0,0,7,2, 0,0,8,2, 0,0,9,2
  db 0,0,1,6  ; cylinder 0, head 0, sector 1, 8192 bytes
;  db 0,0,1,5  ; cylinder 0, head 0, sector 1, 4096 bytes


;section .bss

sectorBuffer: resb 8192
;logBuffer:
