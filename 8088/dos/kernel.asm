org 0

  ; Don't want any stray interrupts interfering with the serial port accesses.
  cli

  ; Blank the screen
  mov dx,0x3d8
  mov al,0
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
  mov ax,0x1f0a
  out dx,ax

  ;   0x1f Cursor End                                   07
  mov ax,0x1f0b
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
  mov word [0x180], writeHex
  mov [0x182], cs
  mov word [0x184], writeString
  mov [0x186], cs
  mov word [0x188], writeCharacter
  mov [0x18a], cs
  mov word [0x19c], complete
  mov [0x19e], cs

; Receive a byte over serial and put it in AL. DX == port base address + 5
%macro receiveByte 0
    ; Wait until a byte is available
  %%waitForData:
    in al,dx
    test al,1
    jz %%waitForData
    ; Read the data byte
    sub dx,5
    in al,dx
    add dx,5
%endmacro


; Send byte in AL over serial. DX == port base address + 5
%macro sendByte 0
    mov ah,al
  %%waitForSpace:
    in al,dx
    test al,0x20
    jz %%waitForSpace
    inc dx
  %%waitForDSR:
    in al,dx
    test al,0x20
    jz %%waitForDSR
    ; Write the data byte
    sub dx,6
    mov al,ah
    out dx,al
    add dx,5
%endmacro


  mov dx,0x3f8  ; COM1 (0x3f8 == COM1, 0x2f8 == COM2, 0x3e8 == COM3, 0x2e8 == COM4)

  ; dx + 0 == Transmit/Receive Buffer   (bit 7 of LCR == 0)  Baud Rate Divisor LSB (bit 7 of LCR == 1)
  ; dx + 1 == Interrupt Enable Register (bit 7 of LCR == 0)  Baud Rate Divisor MSB (bit 7 of LCR == 1)
  ; dx + 2 == Interrupt Identification Register IIR (read)   16550 FIFO Control Register (write)
  ; dx + 3 == Line Control Register LCR
  ; dx + 4 == Modem Control Register MCR
  ; dx + 5 == Line Status Register LSR
  ; dx + 6 == Modem Status Register MSR
  ; dx + 7 == Scratch Pad Register

  add dx,3    ; 3
  mov al,0x80
  out dx,al   ; Set LCR bit 7 to 1 to allow us to set baud rate

  dec dx      ; 2
  dec dx      ; 1
  mov al,0x00
  out dx,al   ; Set baud rate divisor high = 0x00

  dec dx      ; 0
  mov al,0x01
  out dx,al   ; Set baud rate divisor low  = 0x01 = 115200 baud

  add dx,3    ; 3
  ; Line Control Register LCR                                03
  ;      1 Word length -5 low bit                             1
  ;      2 Word length -5 high bit                            2
  ;      4 1.5/2 stop bits                                    0
  ;      8 parity                                             0
  ;   0x10 even parity                                        0
  ;   0x20 parity enabled                                     0
  ;   0x40 force spacing break state                          0
  ;   0x80 allow changing baud rate                           0
  mov al,0x03
  out dx,al

  dec dx      ; 2
  dec dx      ; 1
  ; Interrupt Enable Register                                00
  ;      1 Enable data available interrupt and 16550 timeout  0
  ;      2 Enable THRE interrupt                              0
  ;      4 Enable lines status interrupt                      0
  ;      8 Enable modem status change interrupt               0
  mov al,0x00
  out dx,al

  add dx,3    ; 4
  ; Modem Control Register                                   00
  ;      1 Activate DTR                                       0
  ;      2 Activate RTS                                       0
  ;      4 OUT1                                               0
  ;      8 OUT2                                               0
  ;   0x10 Loop back test                                     0
  out dx,al


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
