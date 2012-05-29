%include "../defaults.asm"
  %undef writeHex
  %undef writeString
  %undef writeCharacter

  ; Don't want any stray interrupts interfering with the serial port accesses.
  cli

  ; Blank the screen to avoid burn-in
  initCGA 0

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
  ; int 0x60 == output AX as a 4-digit hex number
  ; int 0x61 == output CX bytes from DS:SI
  ; int 0x62 == output AL as a character
  ; int 0x67 == finish
  xor ax,ax
  mov ds,ax
  setInterrupt 0x60, writeHex
  setInterrupt 0x61, writeString
  setInterrupt 0x62, writeCharacter
  setInterrupt 0x67, complete


  initSerial


  ; Push the cleanup address for the program to retf back to.
  mov bx,cs
  push bx
  mov ax,complete
  push ax

  ; Find the next segment after the end of the kernel. This is where we'll
  ; load our program.
  mov ax,(kernelEnd + 15) >> 4
  add ax,bx
  mov ds,ax

  xor di,di

tryLoad:
  ; Push the address
  push ds
  push di

  mov al,1
  out dx,al   ; Activate DTR
  inc dx      ; 5
  ; Read a 3-byte count and then a number of bytes into memory, starting at
  ; DS:DI
  receiveByte
  mov cl,al
  receiveByte
  mov ch,al
  receiveByte
  mov bl,al
  mov bh,0

  mov si,bx
  push cx
  xor ah,ah
pagesLoop:
  cmp si,0
  je noFullPages
  xor cx,cx
  call loadBytes
  dec si
  jmp pagesLoop
noFullPages:
  pop cx
  test cx,cx
  jz loadProgramDone
  call loadBytes
loadProgramDone:
  ; Check that the checksum matches
  receiveByte
  mov bx,ax

  dec dx      ; 4
  mov al,0
  out dx,al   ; Deactivate DTR

  mov ax,cs
  mov ds,ax

  cmp bh,bl
  je checksumOk
  pop di
  pop ds
  jmp tryLoad
checksumOk:
  retf

; Load CX bytes from keyboard to DS:DI (or a full 64Kb if CX == 0)
loadBytes:
  receiveByte
  add ah,al
  mov [di],al
  add di,1
  jnc noOverflow
  mov bx,ds
  add bh,0x10
  mov ds,bx
noOverflow:
  loop loadBytes
  ret


writeNybble:
  cmp al,10
  jge alphabetic
  add al,'0'
  jmp gotCharacter
alphabetic:
  add al,'A' - 10
gotCharacter:
  sendByte
  ret


writeHex:
  push bx
  push cx
  push dx
  mov dx,0x3f8 + 5
  mov cl,4
  mov bx,ax
  mov al,bh
  shr al,cl
  call writeNybble
  mov al,bh
  and al,0xf
  call writeNybble
  mov al,bl
  shr al,cl
  call writeNybble
  mov al,bl
  and al,0xf
  call writeNybble
  pop dx
  pop cx
  pop bx
  iret


writeString:
  push ax
  push dx
  mov dx,0x3f8 + 5
writeStringLoop:
  lodsb
  sendByte
  loop writeStringLoop
  pop dx
  pop ax
  iret


writeCharacter:
  push dx
  mov dx,0x3f8 + 5
  sendByte
  pop dx
  iret


complete:
  mov al,26
  int 0x62  ; Write a ^Z character to tell the "run" program to finish
  jmp 0  ; Restart the kernel


kernelEnd:
