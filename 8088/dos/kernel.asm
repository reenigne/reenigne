%include "../defaults_bin.asm"

  jmp codeStart

  db '20131208-com1-115200f',0

codeStart:
  sub di,0x500
  cmp di,kernelEnd
  jne lengthOk1
  or byte[cs:flags],1
lengthOk1:

  call findSP
findSP:
  pop si
  sub si,findSP

  mov ah,0
  mov cx,kernelEnd
checksumLoop:
  lodsb
  add ah,al
  loop checksumLoop
  cmp ah,[cS:checkSum]
  je checksumOk1
  or byte[cs:flags],2
checksumOk1:


  ; Don't want any stray interrupts interfering with the serial port accesses.
  cli

  ; Set 40-column text mode
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

  ; Disable NMI
  xor al,al
  out 0xa0,al

  ; Move the stack right at the end of main RAM.
  mov ax,0x9000
  mov ss,ax
  xor sp,sp

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
  retf

noRelocationNeeded:
  ; Set up some interrupts
  ; int 0x60 == capture screen
  ; int 0x61 == start audio recording
  ; int 0x62 == stop audio recording
  ; int 0x63 == print AX as a 4-digit hex number
  ; int 0x64 == print CX bytes from DS:SI
  ; int 0x65 == print AL as a character
  ; int 0x66 == send file
  ; int 0x67 == finish
  ; int 0x68 == load data from serial port at ES:DI. On completion, ES:DI points to end of loaded data.
  ; int 0x69 == stop screen output
  ; int 0x6a == start screen output
  xor ax,ax
  mov ds,ax
  setInterrupt 0x60, captureScreenRoutine
  setInterrupt 0x61, startAudioRoutine
  setInterrupt 0x62, stopAudioRoutine
  setInterrupt 0x63, printHexRoutine
  setInterrupt 0x64, printStringRoutine
  setInterrupt 0x65, printCharacterRoutine
  setInterrupt 0x66, sendFileRoutine
  setInterrupt 0x67, completeRoutine
  setInterrupt 0x68, loadSerialDataRoutine
  setInterrupt 0x69, stopScreenRoutine
  setInterrupt 0x6a, resumeScreenRoutine

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
  printString

lengthOk:
  test byte[flags],2
  jz checksumOk

  mov si,checksumWrong
  mov cx,checksumWrongEnd - checksumWrong
  printString
checksumOk:

  ; Print the boot message
  mov si,bootMessage
  mov cx,bootMessageEnd - bootMessage
  printString

  ; Push the cleanup address for the program to retf back to.
  mov bx,cs
  push bx
  mov ax,completeRoutine
  push ax

  ; Find the next segment after the end of the kernel. This is where we'll
  ; load our program.
  mov ax,(kernelEnd + 15) >> 4
  add ax,bx
  mov es,ax
  xor di,di

  ; Push the address so we can just retf to it after the load
  push es
  push di

  loadSerialData

  ; Print the OK message
  mov si,okMessage
  mov cx,okMessageEnd - okMessage
  printString

  retf


loadSerialDataRoutine:
  initSerial

  inc dx      ; 5
  mov al,'R'
  sendByte
  dec dx

packetLoop:
  ; Activate DTR
  mov al,1
  out dx,al
  inc dx      ; 5

  ; Receive packet size in bytes
  receiveByte

  mov cl,al
  mov ch,0

  ; Push a copy to check when we're done and adjust DI for retries
  push cx

;  call printCharNoSerial

  ; Init checksum
  mov ah,0

  ; Receive CX bytes and store them at ES:DI
  jcxz noBytes
byteLoop:
  receiveByte
  add ah,al
  stosb
  loop byteLoop
noBytes:

  ; Receive checksum
  receiveByte
  sub ah,al

  ; Deactivate DTR
  dec dx      ; 4
  mov al,0
  out dx,al

  cmp ah,0
  jne checkSumFailed

  ; Send success byte
  inc dx      ; 5
  mov al,'K'
  sendByte
  dec dx      ; 4


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
  inc dx      ; 5
  mov al,'F'
  sendByte
  dec dx      ; 4

  pop cx
  sub di,cx
  jmp packetLoop

transferComplete:
  iret


completeRoutine:
  disconnect
  jmp 0  ; Restart the kernel


printNybble:
  cmp al,10
  jge printAlphabetic
  add al,'0'
  jmp printGotCharacter
printAlphabetic:
  add al,'A' - 10
printGotCharacter:
  jmp printChar


printHexRoutine:
  push bx
  push cx
  mov bx,ax
  mov al,bh
  mov cl,4
  shr al,cl
  call printNybble
  mov al,bh
  and al,0xf
  call printNybble
  mov al,bl
  shr al,cl
  call printNybble
  mov al,bl
  and al,0xf
  call printNybble
  pop cx
  pop bx
  iret


printStringRoutine:
  lodsb
  call printChar
  loop printStringRoutine
  iret


printCharacterRoutine:
  call printChar
  iret


printCharNoScreen:
  push dx
  mov dx,0x3f8 + 5
  sendByte
  pop dx
  ret


printCharNoSerial:
  push dx
  jmp printCharCommon


printChar:
  push dx

  ; Output the character over serial as well
  mov dx,0x3f8 + 5
  sendByte

printCharCommon:
  cmp word[cs:screenCounter],0
  jne .complete

  push bx
  push cx
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
  pop cx
  pop bx
.complete:
  pop dx
  ret

captureScreenRoutine:
  push ax
  mov al,1
  call printCharNoScreen
  pop ax
  iret

startAudioRoutine:
  push ax
  mov al,2
  call printCharNoScreen
  pop ax
  iret

stopAudioRoutine:
  push ax
  mov al,3
  call printCharNoScreen
  pop ax
  iret

printCharEscaped:
  cmp al,0x5
  ja .checkForComplete
.escapeNeeded:
  push ax
  mov al,0
  call printCharNoScreen
  pop ax
.noEscapeNeeded:
  call printCharNoScreen
  ret
.checkForComplete:
  cmp al,0x1a
  je .escapeNeeded
  jmp .noEscapeNeeded


sendFileRoutine:
  cld
  mov al,4
  call printCharNoScreen
  mov al,cl
  call printCharEscaped
  mov al,ch
  call printCharEscaped
  mov al,dl
  call printCharEscaped
.loopTop:
  cmp cx,0
  jne .doByte
  cmp dl,0
  je .done
.doByte:
  cmp si,0xffff
  jne .normalized
  mov si,0x000f
  mov ax,ds
  add ax,0xfff
  mov ds,ax
.normalized:
  lodsb
  call printCharEscaped
  dec cx
  cmp cx,0xffff
  jne .loopTop
  dec dl
  jmp .loopTop
.done:
  iret


stopScreenRoutine:
  inc word[cs:screenCounter]
  iret


resumeScreenRoutine:
  dec word[cs:screenCounter]
  iret


column:
  db 0
startAddress:
  dw 0
bootMessage:
  db 'XT Serial Kernel',10
bootMessageEnd:
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

flags:
  db 0

checkSum:
  db 0     ; This will be overwritten by quickboot

kernelEnd:
