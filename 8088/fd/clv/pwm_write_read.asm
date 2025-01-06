  org 0
  cpu 8086

; Want to be able to deal with speeds from 180rpm to 330rpm
; =182ms to 333ms = 3.3 to 6.06 timer ticks

  mov al,0x34
  out 0x43,al
  mov al,0
  out 0x40,al
  out 0x40,al
  xor ax,ax
  mov es,ax
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
  sti

  mov ax,1
  int 0x10

  mov ax,cs
  mov ds,ax
  mov es,ax

;  mov al,0xb4
;  out 0x43,al
;  mov al,0
;  out 0x42,al
;  out 0x42,al

  in al,0x61
  and al,0xfc
  or al,1
  out 0x61,al



sectorReadLoop:
  mov byte[allowBlip],0
  mov dx,0x3f2
  mov al,0x1c
  out dx,al

  mov ax,cs
  mov es,ax
  mov ax,0x0501
  mov cx,0x0000
  mov dx,0x0000
  mov bx,formatData0
  int 0x13
  jnc noErrorF
  mov ax,0xb800
  mov es,ax
  mov di,0x12
  mov al,'E'
  mov ah,7
  stosw

  mov ah,0
  mov dl,0
  int 0x13

  jmp doneFormat
noErrorF:
  mov ax,0xb800
  mov es,ax
  mov di,0x12
  mov al,' '
  mov ah,7
  stosw
doneFormat:


  mov byte[allowBlip],1
  call readTimer
  mov [lastTime],ax
  mov [lastTime+2],dx

pwmLoop3:
  call readTimer
  sub ax,[lastTime]
  sbb dx,[lastTime+2]

  mov al,ah
  mov ah,dl
  cmp ax,[pwmTime]
  jb pwmLoop3

  mov byte[allowBlip],0
  mov dx,0x3f2
  mov al,0x1c
  out dx,al



  mov byte[allowBlip],0
  mov dx,0x3f2
  mov al,0x1c
  out dx,al

  mov ax,cs
  mov es,ax
  mov ax,0x0301
  mov cx,0x0001
  mov dx,0x0000
  mov bx,sectorBuffer
  int 0x13
  pushf

  call readTimer
  mov [lastTime],ax
  mov [lastTime+2],dx

  popf
  jnc noError
  mov ax,0xb800
  mov es,ax
  mov di,0x10
  mov al,'E'
  mov ah,7
  stosw

  mov ah,0
  mov dl,0
  int 0x13

  jmp doneWrite
noError:
  mov ax,0xb800
  mov es,ax
  mov di,0x10
  mov al,' '
  mov ah,7
  stosw
doneWrite:


  mov byte[allowBlip],1
  call readTimer
  mov [lastTime],ax
  mov [lastTime+2],dx

pwmLoop2:
  call readTimer
  sub ax,[lastTime]
  sbb dx,[lastTime+2]

  mov al,ah
  mov ah,dl
  cmp ax,[pwmTime]
  jb pwmLoop2

  mov byte[allowBlip],0
  mov dx,0x3f2
  mov al,0x1c
  out dx,al


  mov ax,cs
  mov es,ax
  mov ax,0x0201
  mov cx,0x0001
  mov dx,0x0000
  mov bx,sectorBuffer
  int 0x13
  pushf

  call readTimer
  sub ax,[lastTime]
  sbb dx,[lastTime+2]

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

  popf
  jnc noErrorR
  mov al,'E'
  stosw

  mov ah,0
  mov dl,0
  int 0x13

  jmp doneRead
noErrorR:
  mov al,' '
  stosw
doneRead:

  mov byte[allowBlip],1
  call readTimer
  mov [lastTime],ax
  mov [lastTime+2],dx

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
  sub ax,[lastTime]
  sbb dx,[lastTime+2]

  mov al,ah
  mov ah,dl
  cmp ax,[pwmTime]
  jb pwmLoop


  jmp sectorReadLoop

;hexDigit: db "0123456789ABCDEF"
lastTime: dw 0,0
timer: dw 0,0
currentPeriod: dw 0x0888
currentDutyCycle: dw 0x0777
allowBlip: db 0
pwmTime: dw 0x1235


irq0:
  push dx
  push ax
  push ds

  mov ax,cs
  mov ds,ax

  mov dx,0x3f2
motorByte:
  mov al,0x0c
  xor al,0x10
  mov [motorByte+1],al

  cmp byte[allowBlip],0
  je doneBlip

   out dx,al
   shr al,1
   mov dx,0x3d9
   out dx,al
   shl al,1

doneBlip:

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

  cmp al,0x06  ; '5'
  jne .not5
  add word[pwmTime],0x1000
  jmp doneKey
.not5:
  cmp al,0x14  ; 'T'
  jne .notT
  sub word[pwmTime],0x1000
  jmp doneKey
.notT:
  cmp al,0x07  ; '6'
  jne .not6
  add word[pwmTime],0x0100
  jmp doneKey
.not6:
  cmp al,0x15  ; 'Y'
  jne .notY
  sub word[pwmTime],0x0100
  jmp doneKey
.notY:
  cmp al,0x08  ; '7'
  jne .not7
  add word[pwmTime],0x0010
  jmp doneKey
.not7:
  cmp al,0x16  ; 'U'
  jne .notU
  sub word[pwmTime],0x0010
  jmp doneKey
.notU:
  cmp al,0x09  ; '8'
  jne .not8
  add word[pwmTime],0x0001
  jmp doneKey
.not8:
  cmp al,0x17  ; 'I'
  jne .notI
  sub word[pwmTime],0x0001
.notI:

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

  mov di,90
  mov al,[pwmTime + 1]
;  mov al,[dutyCycle + 2]
  shr al,cl
  xlatb
  stosw
  mov al,[pwmTime + 1]
;  mov al,[dutyCycle + 2]
  and al,0x0f
  xlatb
  stosw
  mov al,[pwmTime]
;  mov al,[dutyCycle + 1]
  shr al,cl
  xlatb
  stosw
  mov al,[pwmTime]
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

formatData0:
  db 0,0,1,2, 0,0,2,2, 0,0,3,2, 0,0,4,2, 0,0,5,2, 0,0,6,2, 0,0,7,2, 0,0,8,2, 0,0,9,2

sectorBuffer:

