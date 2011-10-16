org 0

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

    ; Debug
    int 0x65
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

  ; Print the boot message
  mov ax,cs
  mov ds,ax
  mov si,bootMessage
  mov cx,bootMessageEnd - bootMessage
  int 0x64

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
  xor di,di
  push di


tryLoad:
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

  ; Debug: print number of bytes to load
;  mov ax,bx
;  int 0x63
;  mov ax,cx
;  int 0x63
;  mov al,10
;  int 0x65

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
  mov si,failMessage
  mov cx,failMessageEnd - failMessage
  int 0x64
  jmp tryLoad
checksumOk:
  mov si,okMessage
  mov cx,okMessageEnd - okMessage
  int 0x64
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
;  test di,0x000f
;  jnz noPrint

  ; Debug: print load address
;  mov byte[cs:column],0
;  mov ax,ds
;  int 0x63
;  mov ax,di
;  int 0x63

;noPrint:
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
  pop ax
  pop ax
  pop ax
  jmp 0  ; Restart the kernel


bootMessage:
  db 'XT Serial Kernel',10
bootMessageEnd:
okMessage:
  db 'OK',10
okMessageEnd:
failMessage:
  db 'Checksum failure',10
failMessageEnd:


kernelEnd:
