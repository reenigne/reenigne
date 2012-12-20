%include "../defaults_bin.asm"

  jmp codeStart

  db '20121219b-com1-115200',0

codeStart:
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
  mov al,0x36  ; Timer 0, write both bytes, mode 3 (square wave), binary mode
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
  ; int 0x63 == print AX as a 4-digit hex number
  ; int 0x64 == print CX bytes from DS:SI
  ; int 0x65 == print AL as a character
  ; int 0x67 == finish
  ; int 0x68 == load data from serial port at ES:DI. On completion, ES:DI points to end of loaded data.
  xor ax,ax
  mov ds,ax
  setInterrupt 0x63, printHexRoutine
  setInterrupt 0x64, printStringRoutine
  setInterrupt 0x65, printCharacterRoutine
  setInterrupt 0x67, completeRoutine
  setInterrupt 0x68, loadSerialDataRoutine

  ; Reset video variables
  mov ax,cs
  mov ds,ax
  xor ax,ax
  mov [column],al
  mov [startAddress],ax

  ; Print the boot message
;  mov si,bootMessage
;  mov cx,bootMessageEnd - bootMessage
;  printString

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


printChar:
  push bx
  push cx
  push dx
  push es
  push di

  ; Output the character over serial as well
  mov dx,0x3f8 + 5
  sendByte

  mov dx,0xb800
  mov es,dx
  mov dx,0x3d4
  mov cx,[cs:startAddress]
  cmp al,10
  je doPrintNewLine
  mov di,cx
  add di,cx
  mov bl,[cs:column]
  xor bh,bh
  mov [es:bx+di+24*40*2],al
  inc bx
  inc bx
  cmp bx,80
  jne donePrint
doPrintNewLine:
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
donePrint:
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


column:
  db 0
startAddress:
  dw 0
;bootMessage:
;  db 'XT Serial Kernel',10
;bootMessageEnd:
okMessage:
  db 'OK',10
okMessageEnd:


kernelEnd:
