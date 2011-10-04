org 0

  ; Turn interrupts off - the keyboard send routine is cycle-counted.
  cli

  ; Set up the screen so we can debug the keyboard send routine

  ; Mode                                                2c
  ;      1 +HRES                                         0
  ;      2 +GRPH                                         0
  ;      4 +BW                                           4
  ;      8 +VIDEO ENABLE                                 8
  ;   0x10 +1BPP                                         0
  ;   0x20 +ENABLE BLINK                                20
  mov dx,0x3d8
  mov al,0x2c
  out dx,al

  ; Palette                                             00
  ;      1 +OVERSCAN B                                   0
  ;      2 +OVERSCAN G                                   0
  ;      4 +OVERSCAN R                                   0
  ;      8 +OVERSCAN I                                   0
  ;   0x10 +BACKGROUND I                                 0
  ;   0x20 +COLOR SEL                                    0
  mov dx,0x3d9
  mov al,0
  out dx,al

  mov dx,0x3d4

  ;   0xff Horizontal Total                             38
  mov ax,0x3800
  out dx,ax

  ;   0xff Horizontal Displayed                         28
  mov ax,0x2801
  out dx,ax

  ;   0xff Horizontal Sync Position                     2d
  mov ax,0x2d02
  out dx,ax

  ;   0x0f Horizontal Sync Width                        0a
  mov ax,0x0a03
  out dx,ax

  ;   0x7f Vertical Total                               1f
  mov ax,0x1f04
  out dx,ax

  ;   0x1f Vertical Total Adjust                        06
  mov ax,0x0605
  out dx,ax

  ;   0x7f Vertical Displayed                           19
  mov ax,0x1906
  out dx,ax

  ;   0x7f Vertical Sync Position                       1c
  mov ax,0x1c07
  out dx,ax

  ;   0x03 Interlace Mode                               02
  mov ax,0x0208
  out dx,ax

  ;   0x1f Max Scan Line Address                        07
  mov ax,0x0709
  out dx,ax

  ; Cursor Start                                        06
  ;   0x1f Cursor Start                                  6
  ;   0x60 Cursor Mode                                   0
  mov ax,0x060a
  out dx,ax

  ;   0x1f Cursor End                                   07
  mov ax,0x070b
  out dx,ax

  ;   0x3f Start Address (H)                            00
  mov ax,0x000c
  out dx,ax

  ;   0xff Start Address (L)                            00
  mov ax,0x000d
  out dx,ax

  ;   0x3f Cursor (H)                                   03  0x3c0 == 40*24 == start of last line
  mov ax,0x030e
  out dx,ax

  ;   0xff Cursor (L)                                   c0
  mov ax,0xc00f
  out dx,ax

  ; Clear the video memory
  mov ax,0xb800
  mov es,ax
  mov cx,40*25
  mov ax,0x0700
  xor di,di
  cld
  rep stosw

  ; Set up some interrupts
  ; int 0x60 == output AX as a 4-digit hex number
  ; int 0x61 == output CX bytes from DS:SI
  ; int 0x62 == output AL as a character
  ; int 0x63 == print AX as a 4-digit hex number
  ; int 0x64 == print CX bytes from DS:SI
  ; int 0x65 == print AL as a character
  ; int 0x66 == beep (for debugging)
  xor ax,ax
  mov ds,ax
  mov word [0x180], writeHex
  mov [0x182], cs
  mov word [0x184], writeString
  mov [0x186], cs
  mov word [0x188], writeCharacter
  mov [0x18a], cs
  mov word [0x18c], printHex
  mov [0x18e], cs
  mov word [0x190], printString
  mov [0x192], cs
  mov word [0x194], printCharacter
  mov [0x196], cs
  mov word [0x198], beep
  mov [0x19a], cs

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

  ; Beep
  int 0x66
  ; Print a message
  mov ax,cs
  mov ds,ax
  mov si,bootMessage
  mov cx,bootMessageEnd - bootMessage
  int 0x64

  mov di,0x50 ;Target segment (TODO: make this 0060:0000 as FreeDOS does?)
  call main
main:
  pop si
  sub si,main ; Offset of our start within CS
  jnz checkDestinationClear
  mov bx,cs
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

  ; Push the address
  push ds
  push di

  ; Set up the 8259 PIC to read the IRR lines
  mov al,0xfd
  out 0x21,al  ; OCW1 - enable keyboard interrupt
  mov al,0xa   ; OCW3 - no bit 5 action, no poll command issued, act on bit 0,
  out 0x20,al  ;  read Interrupt Request Register

  ; Read the non-keyboard bits of port 061
  mov dx,0x61
  in al,dx
  and al,0x7f
  or al,0x40
  mov ah,al

  ; Reads a 3-byte count and then a number of bytes into memory, starting at DS:DI, then runs the loaded code
  call keyboardRead
  mov cl,bl
  call keyboardRead
  mov ch,bl
  call keyboardRead
  mov bh,0
  mov si,bx
  push cx
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
  retf


; Reads the next keyboard scancode into BL
keyboardRead:
  ; Loop until the IRR bit 1 (IRQ 1) is high
  in al,0x20
  and al,2
  jz keyboardRead
  ; Read the keyboard byte and store it
  in al,0x60
  mov bl,al
  ; Acknowledge the keyboard scancode byte
  mov al,ah
  or al,0x80
  out dx,al
  and al,0x7f
  out dx,al
  ret


; Load CX bytes from keyboard to DS:DI (or a full 64Kb if CX == 0)
loadBytes:
  call keyboardRead
  mov [di],bl
  add di,1
  jnc noOverflow
  mov bx,ds
  add bh,0x10
  mov ds,bx
noOverflow:
  loop loadBytes
  ret


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


writeHex:
  push ds
  push es
  push di
  push si
  push bx
  push cx
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
  call sendLoop
  pop cx
  pop bx
  pop si
  pop di
  pop es
  pop ds
  iret


writeString:
  push di
  push bx
  push dx
  call sendLoop
  pop dx
  pop bx
  pop di
  iret


writeCharacter:
  push ds
  push si
  push bx
  push cx
  mov bx,cs
  mov ds,bx
  mov si,writeBuffer
  mov [si],al
  mov cx,1
  call sendLoop
  pop cx
  pop bx
  pop si
  pop ds
  iret


; Send CX bytes pointed to by DS:SI
sendLoop:
  mov di,cx
  ; Lower clock line to tell the Arduino we want to send data
  mov dx,0x61
  in al,dx
  and al,0xbf
  out dx,al
  ; Wait for 1ms
  mov bx,cx
  mov cx,52500000/(11*17*1000)
waitLoop:
  loop waitLoop
  ; Raise clock line again
  or al,0x40
  out dx,al

  ; Throw away the keystroke that comes from clearInterruptedKeystroke()
  call keyboardRead

  ; Read the number of bytes that we can send
  call keyboardRead
  xor bh,bh
  cmp bx,cx
  jae gotCount
  mov cx,bx      ; Write as many bytes as we can
gotCount:
  sub di,cx

  ; Set up the bh register for sendByte
  in al,dx
  mov bh,al
  rcl bh,1
  rcl bh,1

  mov al,cl
  call sendByte  ; Send the number of bytes we'll be sending
sendByteLoop:
  cmp cx,0
  je doneSend
  lodsb
  call sendByte
  dec cx
  jmp sendByteLoop
doneSend:
  cmp di,0
  jne sendLoop
  ret


sendByte:
  push ax              ; for debugging purposes
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

  rcr bl,1             ; 2 0 8  Each bit takes 40 CPU cycles = 8.38us
  mov al,bh            ; 2 0 8  = 8.87us with DRAM refresh
  rcr al,1             ; 2 0 8  = 142 cycles on the Arduino
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

  pop ax               ; for debugging purposes
  call printChar       ; for debugging purposes
  ret


beep:
  push ax
  push cx
  in al,0x61
  mov ah,al
  or al,3
  out 0x61,al
  mov al,0xb6
  out 0x43,al
  mov cx,1193182 / 440
  mov al,cl
  out 0x42,al
  mov al,ch
  out 0x42,al
  xor cx,cx
beepLoop:
  loop beepLoop
  mov al,ah
  and al,0xfc
  out 0x61,al
  xor cx,cx
quietLoop:
  loop quietLoop
  pop cx
  pop ax
  iret


complete:
  mov al,26
  int 0x62  ; Write a ^Z character to tell the "run" program to finish
  jmp 0  ; Restart the kernel


printNybble:
  cmp al,10
  jge printAlphabetic
  add al,'0'
  jmp printGotCharacter
printAlphabetic:
  add al,'A' - 10
printGotCharacter:
  call printChar
  ret


printHex:
  push bx
  push cx
  mov bx,ax
  mov al,bh
  mov cx,4
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


printString:
  lodsb
  call printChar
  loop printString
  iret


printCharacter:
  call printChar
  iret


printChar:
  push bx
  push cx
  cmp al,10
  je printNewLine
  mov cx,[cs:startAddress]
  push si
  mov si,cx
  add si,cx
  mov bl,[cs:column]
  xor bh,bh
  mov [bx+si+24*40*2],al
  pop si
  inc bx
  inc bx
  cmp bx,160
  jne donePrint
printNewLine:
  add cx,80
  and cx,0x1fff
  mov [cs:startAddress],cx
  push dx
  mov dx,0x3d4
  mov ah,ch
  mov al,0x0c
  out dx,ax
  mov ah,cl
  inc ax
  out dx,ax
  pop dx
  xor bx,bx
donePrint:
  mov [cs:column],bl
  pop cx
  pop bx
  ret


column:
  db 0
startAddress:
  dw 0
bootMessage:
  db 'XT OS Kernel',10
bootMessageEnd:

kernelEnd:
