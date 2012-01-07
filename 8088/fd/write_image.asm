cpu 8086
org 0

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

  ; Set up some interrupts
  ; int 0x63 == print AX as a 4-digit hex number
  ; int 0x64 == print CX bytes from DS:SI
  ; int 0x65 == print AL as a character
  xor ax,ax
  mov ds,ax
  mov word [0x18c], printHex
  mov [0x18e], cs
  mov word [0x190], printString
  mov [0x192], cs
  mov word [0x194], printCharacter
  mov [0x196], cs
  mov word [0x78], driveParameters
  mov [0x7a], cs

  ; Reset video variables
  xor ax,ax
  mov [cs:column],al
  mov [cs:startAddress],ax


  mov ah,0  ; Subfunction 0 = Reset Disk System
  mov dl,0  ; Drive 0 (A:)
  int 0x13
  push ax
  int 0x60  ; Output AX
  mov al,10
  int 0x62  ; New line
  pop ax
  int 0x63
  mov al,10
  int 0x65

; Receive a byte over serial and put it in AL. DX == port base address + 5
%macro receiveByte 0                            ; 14
    ; Wait until a byte is available
  %%waitForData:
    in al,dx                              ; 1 1
    test al,1                             ; 2 0
    jz %%waitForData                      ; 2 0
    ; Read the data byte
    sub dl,5                              ; 3 0
    in al,dx                              ; 1 1
    add dl,5                              ; 3 0
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
  mov al,0x03
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


tryLoad:
  ; Find the next segment after the end of this program. This is where we'll
  ; load our disk image.
  mov ax,0x1000
;  mov ax,cs
;  add ax,(programEnd + 15) >> 4
  mov es,ax

  xor di,di

  mov ax,0xb800
  mov ds,ax

  ; Push the address
  push es
  push di

  mov al,1
  out dx,al   ; Activate DTR
  inc dx      ; 5
  ; Read a 3-byte count and then a number of bytes into memory, starting at
  ; DS:DI
  receiveByte                                       ; 14
  mov cl,al                                         ;  2
  receiveByte                                       ; 14
  mov ch,al                                         ;  2
  receiveByte                                       ; 14
  mov bl,al                                         ;  2
  mov bh,0                                          ;  2

  ; Debug: print number of bytes to load
;  mov ax,bx
;  int 0x63
;  mov ax,cx
;  int 0x63
;  mov al,10
;  int 0x65

  mov si,bx                                         ;  2
  push cx                                           ;  3
  xor ah,ah                                         ;  2
pagesLoop:
  cmp si,0                                          ;  2
  je noFullPages                                    ;  2
  xor cx,cx                                         ;  2
  call loadBytes                                    ; 37

  mov bx,es                                         ;  2
  add bh,0x10                                       ;  2
  mov es,bx                                         ;  2

  dec si                                            ;  1
  jmp pagesLoop                                     ;  2
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
  pop di
  pop es
  jmp tryLoad


  ; Load CX bytes from keyboard to DS:DI (or a full 64Kb if CX == 0)
loadBytes:
  receiveByte                                       ; 14
  add ah,al                                         ;  2
  stosb                                             ;  2
  mov bx,di
  mov [800],bl

;  test di,0x00ff
;  jnz noPrint

;  dec dx      ; 4
;  mov al,0
;  out dx,al   ; Deactivate DTR

;  ; Debug: print load address
;  mov byte[cs:column],0
;  mov ax,ds
;  int 0x63
;  mov ax,di
;  int 0x63

;  mov al,1
;  out dx,al   ; Activate DTR
;  inc dx      ; 5

;noPrint:
  loop loadBytes                                    ;  2
  ret                                               ;  3


checksumOk:
  mov si,okMessage
  mov cx,okMessageEnd - okMessage
  int 0x64


  mov byte[cs:cylinder],0
cylinderLoop:

  mov byte[cs:head],0
headLoop:

  ; TODO: Format track
  ;mov ah,5

  ; Copy a track's worth of data to a buffer at a known location
  pop si
  pop ds
  xor di,di
  mov ax,0x8000  ; We know this won't cross a 64K boundary
  mov es,ax
  mov cx,(512*9)>>1   ; Copy 9*512 byte sectors of data
  rep movsw
  mov ax,ds
  add ax,(512*9)>>4   ; Advance the pointer by increasing the segment
  push ax
  xor si,si
  push si

  ; Write the track
  mov byte[cs:retry],0
retryLoop:

  mov ax,0x8000
  mov es,ax             ; Track buffer segment
  mov bx,0              ; Track buffer offset
  mov ah,3              ; 3 = write disk sectors
  mov al,9              ; write 9 sectors (1 track)
  mov ch,[cs:cylinder]  ; cylinder number
  mov cl,1              ; initial sector number
  mov dh,[cs:head]      ; head number
  mov dl,0              ; drive A:
  int 0x13
  jnc writeOk

  push ax
  mov al,'W'
  int 0x65
  pop ax
  int 0x63
  mov al,10
  int 0x65

  mov ah,0  ; Subfunction 0 = Reset Disk System
  mov dl,0  ; Drive 0 (A:)
  int 0x13

  inc byte[cs:retry]
  cmp byte[cs:retry],10
  jl retryLoop

  mov si,diskFailMessage
  mov cx,diskFailMessageEnd - diskFailMessage
  int 0x64

  jmp tryLoad

writeOk:
  mov al,'.'
  int 0x65


  inc byte[cs:head]
  mov al,[cs:head]
  cmp al,2
  jl headLoop

  inc byte[cs:cylinder]
  mov al,[cs:cylinder]
  cmp al,40
  jge finished
  jmp cylinderLoop
finished:

  ; Jump back into BIOS to boot from the newly written disk
  mov ax,0x40
  push ax
  jmp 0xf000:0xe518


cylinder:
  db 0
head:
  db 0

printNybble:
  cmp al,10
  jge printAlphabetic
  add al,'0'
  jmp printGotCharacter
printAlphabetic:
  add al,'A' - 10
printGotCharacter:
  jmp printChar


printHex:
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
  push dx
  push es
  push di
  mov dx,0xb800
  mov es,dx
  mov dx,0x3d4
  mov cx,[cs:startAddress]
  cmp al,10
  je printNewLine
  mov di,cx
  add di,cx
  mov bl,[cs:column]
  xor bh,bh
  mov [es:bx+di+24*40*2],al
  inc bx
  inc bx
  cmp bx,80
  jne donePrint
printNewLine:
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
retry:
  db 0
okMessage:
  db 'OK',10
okMessageEnd:
failMessage:
  db 'Checksum failure',10
failMessageEnd:
diskFailMessage:
  db 'Disk failure',10
diskFailMessageEnd:

driveParameters:
  db 0xcf
  db 2
  db 37
  db 2
  db 9
  db 0x2a
  db 0xff
  db 0x50
  db 0xf6
  db 25
  db 4


programEnd:
