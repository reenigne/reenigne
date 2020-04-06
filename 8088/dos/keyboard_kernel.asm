  %include "../defaults_bin.asm"

  jmp codeStart

  db '20171011-keyb',0

codeStart:
  mov ax,cs
  mov ds,ax

  call findIP
findIP:
  pop bx
  sub bx,findIP

  sub di,0x500
  cmp di,kernelEnd
  je lengthOk1
  or byte[flags + bx],1
lengthOk1:


  mov ah,0
  mov cx,checkSum
  mov si,bx
checksumLoop:
  lodsb
  add ah,al
  loop checksumLoop
  cmp ah,[checkSum + bx]
  je checksumOk1
  or byte[flags + bx],2
checksumOk1:

  ; Turn interrupts off - the keyboard send routine is cycle-counted.
  cli

  ; Set up the screen so we can debug the keyboard send routine
  initCGA 8

  ; Clear the video memory
  mov ax,0xb800
  mov es,ax
  mov cx,40*25
  mov ax,0x0700
  xor di,di
  cld
  rep stosw

  ; Set up the timer interrupt
  mov al,TIMER0 | BOTH | MODE3 | BINARY
  out 0x43,al
  mov al,0     ; rate = 13125000/11/2^16 ~= 18.2Hz
  out 0x40,al
  out 0x40,al

  ; Set up interrupt masks.
  mov al,0xbc  ; Enable IRQs 0 (timer), 1 (keyboard) and 6 (floppy disk).
  out 0x21,al  ; Leave disabled 2 (EGA/VGA/slave 8259) 3 (COM2/COM4), 4 (COM1/COM3), 5 (hard drive, LPT2) and 7 (LPT1)

  ; Set up interrupt table
  xor ax,ax
  mov es,ax
  mov ax,0xf000
  mov ds,ax
  mov cx,8
  mov si,0xfef3
  mov di,0x20
interruptSetupLoop:
  movsw
  inc di
  inc di
  loop interruptSetupLoop
  mov word[0x02*4], 0xf85f       ; nmi_int
  mov word[0x05*4], 0xff54       ; print_screen
  mov word[0x18*4 + 2], 0xf600   ; segment for BASIC

  ; Find end of memory. Memory is always added in 16Kb units. We can't use
  ; the BIOS measurement since it won't have been initialized.
  mov ax,0x9c00
findRAM:
  mov ds,ax
  mov [0],ax
  cmp [0],ax
  je foundRAM
  sub ax,0x400
  jmp findRAM
foundRAM:
  sub ax,0xc00
  ; Move the stack right at the end of main RAM.
  mov ss,ax
  xor sp,sp

  ; Enable NMI
  in al,0x61
  or al,0x30
  out 0x61,al
  and al,0xcf
  out 0x61,al
  mov al,0x80
  out 0xa0,al

  mov di,0x50 ;Target segment (TODO: make this 0060:0000 as FreeDOS does?)
  mov bx,cs
  call main
main:
  pop si
  sub si,main ; Offset of our start within CS
  jnz checkDestinationClear
  cmp bx,di
  jz noRelocationNeeded
checkDestinationClear:
  ; Check that (start of our code) >= (end of destination code)
  ; Normalize our address
  mov cl,4
  mov ax,si
  shr ax,cl
  add ax,bx
  and si,0x0f ; Our start address in normalized xxxx:000x form is now in AX:SI
  ; compute end of destination as a physical address
  mov dx,di
  add dx,(kernelEnd + 15) >> 4  ; end of destination segment
  cmp ax,dx
  jge doMove
  ; We can't relocate to the final destination directly, since our code
  ; overlaps that space. We need to move to a point that is higher than both
  ; the end of our code and the end of the final destination.
  mov di,ax
  add di,(kernelEnd + 15) >> 4  ; end of current code
  cmp di,dx
  jge doMove
  ; We are overlapping the start of the final destination - relocate to after
  ; the end of the final destination.
  mov di,dx
doMove:
  push di  ; Push return segment
  ; Move kernelEnd bytes from AX:SI to DI:0
  mov cx,kernelEnd
  mov ds,ax
  mov es,di
  xor di,di
  push di  ; Push return offset
  rep movsb
  add di,0x500
  retf

noRelocationNeeded:
  ; Set up some interrupts
  ; int 0x60 == capture screen
  ; int 0x61 == start audio recording
  ; int 0x62 == stop audio recording
  ; int 0x63 == output AX as a 4-digit hex number
  ; int 0x64 == output CX bytes from DS:SI
  ; int 0x65 == output AL as a character
  ; int 0x66 == send file
  ; int 0x67 == finish
  ; int 0x68 == load data from keyboard port at ES:DI. On completion, ES:DI points to end of loaded data.
  ; int 0x69 == stop screen output
  ; int 0x6a == start screen output
  ; int 0x6b == stop keyboard port output
  ; int 0x6c == start keyboard port output
  xor ax,ax
  mov ds,ax
  setInterrupt 0x60, captureScreenRoutine
  setInterrupt 0x61, startAudioRoutine
  setInterrupt 0x62, stopAudioRoutine
  setInterrupt 0x63, outputHexRoutine
  setInterrupt 0x64, outputStringRoutine
  setInterrupt 0x65, outputCharacterRoutine
  setInterrupt 0x66, sendFileRoutine
  setInterrupt 0x67, completeRoutine
  setInterrupt 0x68, loadDataRoutine
  setInterrupt 0x69, stopScreenRoutine
  setInterrupt 0x6a, resumeScreenRoutine
  setInterrupt 0x6b, stopKeyboardRoutine
  setInterrupt 0x6c, resumeKeyboardRoutine

  ; Reset video variables
  mov ax,cs
  mov ds,ax
  xor ax,ax
  mov [column],al
  mov [startAddress],ax

  test byte[flags],1
  jz lengthOk

  mov si,lengthWrong
  mov cx,lengthWrongEnd - lengthWrong
  outputString

lengthOk:
  test byte[flags],2
  jz checksumOk

  mov si,checksumWrong
  mov cx,checksumWrongEnd - checksumWrong
  outputString

checksumOk:
  ; Push the cleanup address for the program to retf back to.
  mov bx,cs
  push bx
  mov ax,completeRoutine
  push ax

  ; Find the next segment after the end of the kernel. This is where we'll
  ; load our program.
  mov ax,(kernelEnd + 15) >> 4
  add ax,bx
  mov ds,ax
  mov es,ax

  ; Push the address
  push ds
  xor di,di
  push di

  ; Set up the 8259 PIC to read the IRR lines
  mov al,0x0a  ; OCW3 - no bit 5 action, no poll command issued, act on bit 0,
  out 0x20,al  ;  read Interrupt Request Register

  ; The BIOS leaves the keyboard with an unacknowledged byte - acknowledge it
  in al,0x61
  or al,0x80
  out 0x61,al
  and al,0x7f
  out 0x61,al

  mov al,5
  call sendChar

  loadData

  mov ah,0
  xor cx,cx
  call sendLoop

  retf


; Load data to ES:DI
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


; Load CX bytes from keyboard to DS:DI (or a full 64Kb if CX == 0)
loadBytes:
  call keyboardRead
  add dl,bl
  mov [di],bl
  add di,1
  jnc noOverflow
  mov bx,ds
  add bh,0x10
  mov ds,bx
noOverflow:
  test di,0x000f
  jnz noPrint
noPrint:
  loop loadBytes
  ret


; Send a single byte packet, contents AL
sendChar:
  mov bx,cs
  mov ds,bx
  mov si,writeBuffer
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
  disconnect
  jmp 0  ; Restart the kernel


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
  mov di,writeBuffer
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
  mov si,writeBuffer

  cmp word[cs:screenCounter],0
  jne .noScreen
  call printLoop
.noScreen:
  cmp word[cs:keyboardCounter],0
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
  cmp word[cs:screenCounter],0
  jne .noScreen
  call printLoop
.noScreen:
  cmp word[cs:keyboardCounter],0
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
  cmp word[cs:screenCounter],0
  jne .noScreen
  push ax
  call printChar
  pop ax
.noScreen:
  cmp word[cs:keyboardCounter],0
  jne .noKeyboard
  push ds
  push si
  push di
  push bx
  push cx
  push dx
  call sendChar
  pop dx
  pop cx
  pop bx
  pop di
  pop si
  pop ds
.noKeyboard:
  iret


printChar:
  push bx
  push cx
  push dx
  push es
  push di
  mov dx,0xb800
  mov es,dx
  mov dx,0x3d4
  mov cx,[cs:startAddress]
  cmp al,10
  je .newLine
  mov di,cx
  add di,cx
  mov bl,[cs:column]
  xor bh,bh
  mov [es:bx+di+24*40*2],al
  inc bx
  inc bx
  cmp bx,80
  jne .done
.newLine:
  add cx,40
  and cx,0x1fff
  mov [cs:startAddress],cx

  ; Scroll screen
  mov ah,ch
  mov al,0x0c
  out dx,ax
  mov ah,cl
  inc ax
  out dx,ax
  ; Clear the newly scrolled area
  mov di,cx
  add di,cx
  add di,24*40*2
  mov cx,40
  mov ax,0x0700
  rep stosw
  mov cx,[cs:startAddress]

  xor bx,bx
.done:
  mov [cs:column],bl

  ; Move cursor
  shr bx,1
  add bx,cx
  add bx,24*40
  and bx,0x1fff
  mov ah,bh
  mov al,0x0e
  out dx,ax
  mov ah,bl
  inc ax
  out dx,ax

  pop di
  pop es
  pop dx
  pop cx
  pop bx
  ret


stopScreenRoutine:
  inc word[cs:screenCounter]
  iret

resumeScreenRoutine:
  dec word[cs:screenCounter]
  iret

stopKeyboardRoutine:
  inc word[cs:keyboardCounter]
  iret

resumeKeyboardRoutine:
  dec word[cs:keyboardCounter]
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
  push ax
  mov al,1
  call sendChar2
  pop ax
  iret

startAudioRoutine:
  push ax
  mov al,2
  call sendChar2
  pop ax
  iret

stopAudioRoutine:
  push ax
  mov al,3
  call sendChar2
  pop ax
  iret

sendFileRoutine:
  cld
  push ax
  push di
  push ds
  push si
  push es
  mov ax,cs
  mov ds,ax
  mov es,ax
  mov di,writeBuffer
  mov al,4
  stosb
  mov ax,cx
  stosw
  mov al,dl
  stosb
  pop es
  mov si,writeBuffer
  push cx
  push dx
  mov cx,4
  mov ah,0
  call sendLoop
  pop dx
  pop cx
  pop si
  pop ds
  mov ah,dl
  call sendLoop
  pop di
  pop ax
  iret


column:
  db 0
startAddress:
  dw 0
lengthWrong:
  db 'Length incorrect',10
lengthWrongEnd:
checksumWrong:
  db 'Checksum incorrect',10
checksumWrongEnd:
okMessage:
  db 'OK',10
okMessageEnd:
screenCounter:
  dw 0
keyboardCounter:
  dw 0

flags:
  db 0

checkSum:
  db 0     ; This will be overwritten by quickboot

kernelEnd:
