%include "../../defaults_bin.asm"

residentPortionEnd equ (tempBuffer + 19)
residentPortionLength equ (residentPortionEnd - residentPortion)

  ; Non-resident portion

  ; Set up stack
  cli
  mov ax,0x30
  mov ss,ax
  mov sp,0x100
  sti

  cld
  mov si,residentPortion
  mov di,0
  mov ax,0xa000 - (15 + residentPortionLength)/16
  mov es,ax
  mov ax,cs
  mov ds,ax
  mov cx,(1 + residentPortionLength)/2
  rep movsw

  mov ax,0
  mov ds,ax
  mov word[0x413],640 - ((residentPortionLength + 1023)/1024)   ; Reduce BIOS RAM count to reduce chances of DOS stomping on our resident portion

  mov dx,0xc000
romScanLoop:
  mov ds,dx
  cmp word[0],0xaa55
  jnz noRom

  push dx
  mov ax,0x40
  mov es,ax
  mov word[es:0x67],3
  mov [es:0x69],ds
  call far [es:0x67]      ; Init ROM
  pop dx

noRom:
  add dx,0x80
  cmp dx,0xf600
  jne romScanLoop


  ; Do this after initializing add-in ROMs, in case of hard drive or VGA

  mov ax,0
  mov ds,ax
  mov ax,0xa000 - (15 + residentPortionLength)/16
  mov es,ax

%macro setResidentInterrupt 2
  mov word [%1*4], (%2 - residentPortion)
  mov [%1*4 + 2], es
%endmacro

  mov ax,[0x10*4]
  mov [es:oldInterrupt10 - residentPortion],ax
  mov ax,[0x10*4 + 2]
  mov [es:2+oldInterrupt10 - residentPortion],ax

  setResidentInterrupt 0x10, int10Routine
  setResidentInterrupt 0x13, int13Routine
  setResidentInterrupt 0x60, captureScreenRoutine
  setResidentInterrupt 0x61, startAudioRoutine
  setResidentInterrupt 0x62, stopAudioRoutine
  setResidentInterrupt 0x63, outputHexRoutine
  setResidentInterrupt 0x64, outputStringRoutine
  setResidentInterrupt 0x65, outputCharacterRoutine
  setResidentInterrupt 0x66, sendFileRoutine
  setResidentInterrupt 0x67, completeRoutine
  setResidentInterrupt 0x68, loadDataRoutine
  setResidentInterrupt 0x69, stopScreenRoutine
  setResidentInterrupt 0x6a, resumeScreenRoutine
  setResidentInterrupt 0x6b, stopKeyboardRoutine
  setResidentInterrupt 0x6c, resumeKeyboardRoutine

  ; Init keyboard buffer
  mov si,0x1e
  mov ax,0x40
  mov ds,ax
  mov [0x1a],si
  mov [0x1c],si
  mov [0x80],si
  add si,32
  mov [0x82],si

  sti

  mov ax,1
  int 0x10

  ; Boot machine
  int 0x19


  ; Resident portion

residentPortion:

  ; This is basically a copy of the int 0x6X routines in keyboard_kernel.asm, except
  ; that these ones use int 0x10 to write to the screen.

loadDataRoutine:
  push ax
  push bx
  push cx
  push dx
  push si
  push ds

sendReady:
  push di
  mov al,'R'
  call sendChar
  pop di

packetLoop:
  ; Receive an 'X' to start the packet
  call keyboardRead
  cmp bl,'X'
  jne sendReady

  ; Receive packet size in bytes
  call keyboardRead

  mov cl,bl
  mov ch,0

  ; Push a copy to check when we're done and adjust DI for retries
  push cx

  ; Init checksum
  mov ah,0

  ; Receive CX bytes and store them at ES:DI
  jcxz noBytes
byteLoop:
  call keyboardRead

  mov al,bl
  add ah,al
  stosb
  loop byteLoop
noBytes:

  ; Receive checksum
  call keyboardRead

  sub ah,bl

  cmp ah,0
  jne checkSumFailed

  push di
  mov al,'K'
  call sendChar
  pop di

  ; Normalize ES:DI
  mov ax,di
  mov cl,4
  shr ax,cl
  mov bx,es
  add bx,ax
  mov es,bx
  and di,0xf

  pop cx
  jcxz transferComplete
  jmp packetLoop

checkSumFailed:
  ; Send fail byte
  push di
  mov al,'F'
  call sendChar
  pop di

  pop cx
  sub di,cx
  jmp packetLoop

transferComplete:
  pop ds
  pop si
  pop dx
  pop cx
  pop bx
  pop ax
  iret


; Reads the next keyboard scancode into BL
keyboardRead:
  mov al,0x0a  ; OCW3 - no bit 5 action, no poll command issued, act on bit 0,
  out 0x20,al  ;  read Interrupt Request Register

.loop:
  ; Loop until the IRR bit 1 (IRQ 1) is high
  in al,0x20
  and al,2
  jz .loop
  ; Read the keyboard byte and store it
  in al,0x60
  mov bl,al
  ; Acknowledge the previous byte
  in al,0x61
  or al,0x80
  out 0x61,al
  and al,0x7f
  out 0x61,al
  ret


; Send a single byte packet, contents AL
sendChar:
  mov bx,cs
  mov ds,bx
  mov si,writeBuffer - residentPortion
  mov [si],al
  mov cx,1
  mov ah,0

; Send AH:CX bytes pointed to by DS:SI
sendLoop:
  pushf
  mov di,cx
.loop:
  ; Lower clock line to tell the Arduino we want to send data
  in al,0x61
  and al,0xbf
  out 0x61,al
  cli
  ; Wait for 0.5ms
  mov bx,cx
  mov cx,52500000/(11*54*2000)
.waitLoop:
  loop .waitLoop
  ; Raise clock line again
  in al,0x61
  or al,0x40
  out 0x61,al

  ; Throw away the keystroke that comes from clearInterruptedKeystroke()
  call keyboardRead

  ; Normalise DS:SI
  mov bx,si
  mov cl,4
  shr bx,cl
  and si,0x0f
  mov cx,ds
  add cx,bx
  mov ds,cx

  ; Read the number of bytes that we can send, but do not ack - we want the data line to stay low
.readCountLoop:
  ; Loop until the IRR bit 1 (IRQ 1) is high
  in al,0x20
  and al,2
  jz .readCountLoop
  ; Read the keyboard byte and store it
  in al,0x60
  mov cl,al
  xor ch,ch

  ; Calculate number of bytes to actually send
  cmp ah,0
  jne .gotCount
  cmp di,cx
  jae .gotCount
  mov cx,di
.gotCount:
  sub di,cx
  sbb ah,0

  ; Set up the bh register for sendByte
  mov dx,0x61
  in al,dx
  mov bh,al
  rcl bh,1
  rcl bh,1

  mov al,cl
  call sendByteRoutine  ; Send the number of bytes we'll be sending
  jcxz .doneData

.sendByteLoop:
  lodsb
  call sendByteRoutine
  loop .sendByteLoop

.doneData:
  ; Finally acknowledge the count byte we received earlier to enable keyboard input again.
  in al,0x61
  or al,0x80
  out 0x61,al
  and al,0x7f
  out 0x61,al
  sti

  cmp di,0
  jne .loop
  cmp ah,0
  jne .loop

  popf
  ret
;.loop2:
;  jmp .loop


sendByteRoutine:
  mov bl,al

  clc
  mov al,bh
  rcr al,1
  rcr al,1
  out dx,al

  stc
  mov al,bh
  rcr al,1
  rcr al,1
  out dx,al

  rcr bl,1             ; 2 0 8  Each bit takes 45.6 CPU cycles = 9.55us
  mov al,bh            ; 2 0 8  = 153 cycles on the Arduino
  rcr al,1             ; 2 0 8
  rcr al,1             ; 2 0 8
  out dx,al            ; 1 1 8

  rcr bl,1             ; 2 0 8
  mov al,bh            ; 2 0 8
  rcr al,1             ; 2 0 8
  rcr al,1             ; 2 0 8
  out dx,al            ; 1 1 8

  rcr bl,1             ; 2 0 8
  mov al,bh            ; 2 0 8
  rcr al,1             ; 2 0 8
  rcr al,1             ; 2 0 8
  out dx,al            ; 1 1 8

  rcr bl,1             ; 2 0 8
  mov al,bh            ; 2 0 8
  rcr al,1             ; 2 0 8
  rcr al,1             ; 2 0 8
  out dx,al            ; 1 1 8

  rcr bl,1             ; 2 0 8
  mov al,bh            ; 2 0 8
  rcr al,1             ; 2 0 8
  rcr al,1             ; 2 0 8
  out dx,al            ; 1 1 8

  rcr bl,1             ; 2 0 8
  mov al,bh            ; 2 0 8
  rcr al,1             ; 2 0 8
  rcr al,1             ; 2 0 8
  out dx,al            ; 1 1 8

  rcr bl,1             ; 2 0 8
  mov al,bh            ; 2 0 8
  rcr al,1             ; 2 0 8
  rcr al,1             ; 2 0 8
  out dx,al            ; 1 1 8

  rcr bl,1             ; 2 0 8
  mov al,bh            ; 2 0 8
  rcr al,1             ; 2 0 8
  rcr al,1             ; 2 0 8
  out dx,al            ; 1 1 8

  stc
  mov al,bh
  rcr al,1
  rcr al,1
  out dx,al
  ret


completeRoutine:
  mov al,26
  call sendChar
  cli
  hlt


writeBuffer:
  db 0, 0, 0, 0

convertNybble:
  cmp al,10
  jge alphabetic
  add al,'0'
  jmp gotCharacter
alphabetic:
  add al,'A' - 10
gotCharacter:
  stosb
  ret

outputHexRoutine:
  sti
  push ds
  push es
  push di
  push si
  push bx
  push cx
  push dx
  mov bx,cs
  mov ds,bx
  mov es,bx
  mov di,writeBuffer - residentPortion
  mov bx,ax
  mov al,bh
  mov cx,4
  shr al,cl
  call convertNybble
  mov al,bh
  and al,0xf
  call convertNybble
  mov al,bl
  shr al,cl
  call convertNybble
  mov al,bl
  and al,0xf
  call convertNybble
  mov si,writeBuffer - residentPortion

  cmp word[cs:screenCounter - residentPortion],0
  jne .noScreen
  call printLoop
.noScreen:
  cmp word[cs:keyboardCounter - residentPortion],0
  jne .noKeyboard
  mov ah,0
  call sendLoop
.noKeyboard:
  pop dx
  pop cx
  pop bx
  pop si
  pop di
  pop es
  pop ds
  iret


printLoop:
  push si
  push cx
.loop:
  lodsb
  call printChar
  loop .loop
  pop cx
  pop si
  ret


outputStringRoutine:
  sti
  cmp word[cs:screenCounter - residentPortion],0
  jne .noScreen
  call printLoop
.noScreen:
  cmp word[cs:keyboardCounter - residentPortion],0
  jne .noKeyboard
  push ds
  push ax
  push di
  push bx
  push dx
  mov ah,0
  call sendLoop
  pop dx
  pop bx
  pop di
  pop ax
  pop ds
.noKeyboard:
  iret


outputCharacterRoutine:
  sti
  cmp word[cs:screenCounter - residentPortion],0
  jne .noScreen
  push ax
  call printChar
  pop ax
.noScreen:
  cmp word[cs:keyboardCounter - residentPortion],0
  jne .noKeyboard
  push ds
  push si
  push bx
  push cx
  call sendChar
  pop cx
  pop bx
  pop si
  pop ds
.noKeyboard:
  iret

printChar:
  push ax
  push bx
  mov ah,0x0e
  mov bx,0x0001
  pushf
  call far [cs:oldInterrupt10 - residentPortion]     ; Note, we can only do this once the BIOS has fully initialized, not on the first INT 13h
  pop bx
  pop ax
  ret

stopScreenRoutine:
  sti
  inc word[cs:screenCounter - residentPortion]
  iret

resumeScreenRoutine:
  sti
  dec word[cs:screenCounter - residentPortion]
  iret

stopKeyboardRoutine:
  sti
  inc word[cs:keyboardCounter - residentPortion]
  iret

resumeKeyboardRoutine:
  sti
  dec word[cs:keyboardCounter - residentPortion]
  iret

sendChar2:
  push bx
  push cx
  push dx
  push si
  push di
  push ds
  call sendChar
  pop ds
  pop di
  pop si
  pop dx
  pop cx
  pop bx
  ret

captureScreenRoutine:
  sti
  push ax
  mov al,1
  call sendChar2
  pop ax
  iret

startAudioRoutine:
  sti
  push ax
  mov al,2
  call sendChar2
  pop ax
  iret

stopAudioRoutine:
  sti
  push ax
  mov al,3
  call sendChar2
  pop ax
  iret

sendFileRoutine:
  sti
  cld
  push ax
  push di
  push ds
  push si
  mov ax,cs
  mov ds,ax
  mov di,writeBuffer - residentPortion
  mov al,4
  stosb
  mov ax,cx
  stosw
  mov al,dl
  stosb
  mov si,writeBuffer - residentPortion
  push cx
  mov cx,4
  mov ah,0
  call sendLoop
  pop cx
  pop si
  pop ds
  mov ah,dl
  call sendLoop
  pop di
  pop ax
  iret


screenCounter:
  dw 0
keyboardCounter:
  dw 0
oldInterrupt10:
  dw 0, 0


int10Routine:
  sti
  cmp ah,0x0e
  jne .noIntercept
  push ax
  push bx
  push cx
  push dx
  push si
  push di
  push ds
  call sendChar
  pop ds
  pop di
  pop si
  pop dx
  pop cx
  pop bx
  pop ax
.noIntercept:
  jmp far [cs:oldInterrupt10 - residentPortion]


int13Routine:
  sti

  push bx
  push cx
  push dx
  push si
  push di
  push bp
  push ds
  push es

  mov bp,0x40
  mov ds,bp

  cmp ah,0
  je .reset
  cmp ah,1
  je .status
  mov byte[0x41],0  ; ok
  cmp ah,2
  je .read
  cmp ah,3
  je .write
  cmp ah,4
  je .verify
  cmp ah,5
  je .format
  mov ax,0x100      ; bad command
  stc
  mov byte[0x41],ah
  jmp .complete

.reset:
  mov byte[0x41],0  ; ok
  jmp .complete

.status:
  mov al,byte[0x41]
  jmp .complete

.read:
  call sendParameters

  ; Receive the data to read
  mov di,bx

  loadData

  jmp .getResult

.write:
.verify:
  call sendParameters

  ; Send the data to write/verify
  mov cx,ax
  mov ax,es
  mov ds,ax
  mov si,bx
  mov ah,0
  call sendLoop

.getResult:
  ; Receive the status information
  mov ax,cs
  mov es,ax
  mov di,tempBuffer - residentPortion

  loadData

  ; Store the status information
  mov ah,[cs:tempBuffer+2 - residentPortion]
  sahf
  mov ax,[cs:tempBuffer - residentPortion]
  mov bp,0x40
  mov ds,bp
  mov [0x41],ah

  ; Zero byte packet to re-enable keyboard
  mov ah,0
  xor cx,cx
  call sendLoop

.complete:
  pop es
  pop ds
  pop bp
  pop di
  pop si
  pop dx
  pop cx
  pop bx

  retf 2  ; Throw away saved flags

.format:
  call sendParameters

  ; Send the formatting data
  mov cl,ch      ; Sector count from sendParameters
  mov ch,0
  shl cx,1
  shl cx,1       ; *4 = number of bytes to send for format
  mov ax,es
  mov ds,ax
  mov si,bx
  mov ah,0
  call sendLoop

  jmp .getResult


sendParameters:
  push es
  push bx
  push ax        ; Save sector count and command
  mov ax,cs
  mov es,ax
  mov di,tempBuffer - residentPortion
  cld

  mov al,0x05
  stosb             ; Host command
  mov al,0x13
  stosb             ; Interrupt number
  pop ax
  stosw             ; Number of sectors, Command
  push ax        ; Save sector count
  mov ax,cx
  stosw             ; Sector number, Track number
  mov ax,dx
  stosw             ; Drive number, Head number

  ; Send the contents of the disk parameter table
  xor ax,ax
  mov ds,ax
  lds si,[0x1e*4]
  push word[si+3]    ; Bytes-per-sector shift and sectors per track...
  mov cx,0x0b
  rep movsb

  ; Do the actual send
  mov ax,cs
  mov ds,ax
  mov si,tempBuffer - residentPortion
  mov ah,0
  mov cx,19
  call sendLoop

  pop cx             ; ...in CL and CH respectively
  pop ax         ; Saved sector count from above
  mov ah,0
  add cl,7
  shl ax,cl      ; Return number of bytes to read/write in AX and sector count in CH
  pop bx
  pop es
  ret

tempBuffer:

