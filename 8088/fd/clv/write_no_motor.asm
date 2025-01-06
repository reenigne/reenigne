  org 0
  cpu 8086

; Want to be able to deal with speeds from 180rpm to 330rpm
; =182ms to 333ms = 3.3 to 6.06 timer ticks

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

  mov ax,cs
  mov ds,ax
  mov es,ax
  mov ax,[currentPeriod]
  mov al,0x34
  out 0x43,al
  mov ax,0x1000
  out 0x40,al
  mov al,ah
  out 0x40,al
  sti

  mov ax,1
  int 0x10
  mov dx,0x3d8
  mov al,0x08
  out dx,al

;  mov al,0xb4
;  out 0x43,al
;  mov al,0
;  out 0x42,al
;  out 0x42,al

  in al,0x61
  and al,0xfc
  or al,1
  out 0x61,al

  mov ax,ds
  mov cl,4
  shl ax,cl
  add ax,sectorBuffer
  mov [bufferDMAAddress],ax

sectorReadLoop:
  mov ax,cs
  mov es,ax
  mov ax,0xf000
  mov si,0x8000
  mov ds,ax
  mov di,sectorBuffer
  mov cx,256
  rep movsw
  mov bp,cs
  mov ds,bp

  cmp byte[blipMotor],0
  jne .doWrite
  mov ax,0x0201
  mov bp,80
  jmp .doOperation
.doWrite:
  mov ax,0x0301
  mov bp,80+80*7
.doOperation:
  mov cx,0x0001
  mov dx,0x0000
  mov bx,sectorBuffer
  int 0x13
  pushf

  mov si,sectorBuffer
  mov ax,0xb800
  mov es,ax
  mov di,bp ; 80
  mov cx,256
  rep movsw
;  mov al,0x80
;  out 0x43,al
;  in al,0x42
;  mov ah,al
;  in al,0x42
;  xchg ah,al

  xor byte[blipMotor],1

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
;  sub si,cx              ; The value we're currently counting down from
  sub si,bx              ; The number of cycles we've counted down since the last wrap
  add ax,si              ; Add that to current time value
  adc dx,0

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

  popf
  jnc noError
  mov al,'E'
  stosw

  mov ah,0
  mov dl,0
  int 0x13

  jmp sectorReadLoop
noError:
  mov al,' '
  stosw

  jmp sectorReadLoop

;hexDigit: db "0123456789ABCDEF"
lastTime: dw 0,0
timer: dw 0,0
totalPeriod: db 0
currentPeriod: dw 0x1000     ; ==107 bytes of floppy transfer
dutyCycle: db 0
currentDutyCycle: dw 0x1000 ; 0x0777
blipMotor: db 0
bufferDMAAddress: dw 0

irq0:
  push dx
  push ax
  push ds

  mov ax,cs
  mov ds,ax

  cmp byte[blipMotor],0
  je dmaNotRunning

  mov dx,[bufferDMAAddress]

dmaLoop:
  in al,0x04
  mov ah,al
  in al,0x04
  xchg al,ah
  sub ax,dx
  cmp ax,0x80
  jb dmaNotRunning
  cmp ax,0x180
  ja dmaNotRunning
  cmp ax,0x100
  jb dmaLoop

  mov dx,0x3f2
  mov al,0x0c
  out dx,al
  mov dx,0x3d9
  mov al,1
  out dx,al

dmaLoop2:
  in al,0x04
  mov ah,al
  in al,0x04
  xchg al,ah
  mov dx,[bufferDMAAddress]
  sub ax,dx
  cmp ax,0x80
  jb dmaNotRunning
  cmp ax,0x180
  ja dmaNotRunning
  cmp ax,0x180
  jb dmaLoop2

dmaNotRunning:

  mov dx,0x3f2
  mov al,0x1c
  out dx,al
  mov dx,0x3d9
  mov al,0
  out dx,al



;  mov dx,0x3f2
;motorByte:
;  mov al,0x0c
;  xor al,0x10
;  mov [motorByte+1],al
;   shr al,1
;   mov dx,0x3d9
;   out dx,al
;   mov dx,0x3f2
;;   mov al,0x1c
;   mov al,[motorByte+1]
;  out dx,al
;   mov al,[motorByte+1]
;
;  ; We are now counting down from the value in [dutyCycle+1]
;  ; if [motorByte+1] is 0x1c the motor has been turned on
;;  mov ax,[dutyCycle+1]
;;  mov [currentPeriod],ax
;  test al,0x10
;  jz totalPeriod
;  mov ax,[currentPeriod]
;  mov [totalPeriod + 1],ax
;  mov ax,[currentDutyCycle]
;  mov [dutyCycle + 1],ax
;  jmp totalPeriod  ; Flush prefetch queue (not needed on 8088)
;
;totalPeriod:
;  mov ax,0x0555
;dutyCycle:
;  sub ax,0x0111
;  mov [dutyCycle+1],ax  ; Because we alternate between two periods, the new period we're programming is also the period that has just elapsed
;  add [timer],ax
;  adc word[timer+2],0
;
;  cmp ax,0x100
;  jb noUpdatePIT
;
;  out 0x40,al
;  mov al,ah
;  out 0x40,al
;noUpdatePIT:


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


;fillRandomBuffer:
;  mov ax,0xf000
;  mov ds,ax
;  mov si,0xe000
;  mov di,randomBuffer
;  mov cx,256
;  rep movsw
;  mov ax,cs
;  mov ds,ax
;  rep



hexDigit: db "0123456789ABCDEF"

sectorBuffer: resb 512

