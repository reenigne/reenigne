cpu 8086
org 0


  mov ah,0  ; Subfunction 0 = Reset Disk System
  mov dl,0  ; Drive 0 (A:)
  int 0x13
  int 0x60  ; Output AX
  mov al,10
  int 0x62  ; New line

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


  ; Find the next segment after the end of the kernel. This is where we'll
  ; load our program.
  mov ax,(programEnd + 15) >> 4
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
  jmp tryLoad
checksumOk:

  ; Now write


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




  mov ax,cs
  mov es,ax
  mov ds,ax


  ; Get drive parameters
  mov ah,8
  mov dl,0x80
  int 0x13
  pushf
  int 0x60      ; AX = 0000  no error
  mov ax,bx
  int 0x60      ; BX = 0000
  mov ax,cx
  int 0x60      ; CX = 3051  17 sectors per track (1..17), 305 cylinders (0..304)
  mov ax,dx
  int 0x60      ; DX = 0301  4 heads (0..3), 1 drive. Total = 17*305*4*512 = 10618880 bytes = 10370Kb = 10.12Mb (?)
  pop ax
  int 0x60      ; Flags = F246  CF=0 PF=1 AF=0 ZF=1 SF=0 TF=0 IF=1 DF=0 OF=0   Success
  mov al,10
  int 0x62

  mov al,cl
  and al,0x3f
  mov byte[sectors],al

  inc dh
  mov byte[heads],dh

  mov al,ch
  mov ah,cl
  mov cl,6
  shr ah,cl
  inc ax
  mov word[cylinders],ax
  int 0x60


  mov word[cylinder],0
cylinderLoop:

  mov byte[head],0
headLoop:

  mov byte[sector],1
sectorLoop:

  mov bx,buffer

  mov cx,10
retryLoop:
  push cx

  mov dh,byte[head]
  mov ax,word[cylinder]
  mov ch,al
  mov cl,6
  shl ah,cl
  mov cl,ah
  or cl,byte[sector]

  mov ax,word[cylinder]
  int 0x60
  mov al,byte[sector]
  mov ah,dh
  int 0x60
  mov al,10
  int 0x62

  mov al,1  ; Number of sectors to read
  mov dl,0x80  ; Drive number
  mov ah,2
  int 0x13
  pushf
  int 0x60      ; AX = 1000
  pop ax
  push ax
  int 0x60      ; Flags = F217  CF=1 PF=1 AF=1 ZF=0 SF=0 TF=0 IF=1 DF=0 OF=0   Failure
  mov al,10
  int 0x62
  popf
  jnc output

  pop cx
  loop retryLoop
  jmp nextSector

output:
  pop cx
  mov si,buffer
  mov cx,0x200
outputLoop:
  lodsb
  int 0x62
  loop outputLoop

nextSector:
  inc byte[sector]
  mov al,byte[sector]
  cmp al,byte[sectors]
  jle sectorLoop

  inc byte[head]
  mov al,byte[head]
  cmp al,byte[heads]
  jl headLoop

  inc word[cylinder]
  mov ax,word[cylinder]
  mov ax,word[cylinder]
  cmp ax,word[cylinders]
  jge finished
  jmp cylinderLoop
finished:
  retf

cylinders:
  dw 0
sectors:
  db 0
heads:
  db 0
cylinder:
  dw 0
sector:
  db 0
head:
  db 0

buffer:
