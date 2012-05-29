%include "../defaults.asm"

  ; Set 40-column text mode
  initCGA 8

  ; Set up some interrupts
  ; int 0x63 == print AX as a 4-digit hex number
  ; int 0x64 == print CX bytes from DS:SI
  ; int 0x65 == print AL as a character
  xor ax,ax
  mov ds,ax
  setInterrupt 0x63, printHex
  setInterrupt 0x64, printString
  setInterrupt 0x65, printCharacter
  setInterrupt 0x1e, driveParameters

  ; Reset video variables
  xor ax,ax
  mov [cs:column],al
  mov [cs:startAddress],ax


  mov ah,0  ; Subfunction 0 = Reset Disk System
  mov dl,0  ; Drive 0 (A:)
  int 0x13
  push ax
  writeHex
  writeNewLine
  pop ax
  int 0x63
  mov al,10
  int 0x65


  initSerial

tryLoad:
  ; Set load location
  mov ax,0x1000
  mov es,ax
  xor di,di

  ; Push a copy to use when we write the image to disk
  push es
  push di

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
  mov al,'K'
  int 0x65


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
  mov al,'F'
  int 0x65

  pop cx
  sub di,cx
  jmp packetLoop


transferComplete:
  mov al,'D'
;  int 0x65
  writeCharacter


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
;  int 0x65
  writeCharacter
  pop ax
;  int 0x63
  writeHex
;  mov al,10
;  int 0x65
  writeNewLine

  mov ah,0  ; Subfunction 0 = Reset Disk System
  mov dl,0  ; Drive 0 (A:)
  int 0x13

  inc byte[cs:retry]
  cmp byte[cs:retry],10
  jl retryLoop

  mov si,diskFailMessage
  mov cx,diskFailMessageEnd - diskFailMessage
;  int 0x64
  mov ax,cs
  mov ds,ax
  writeString

  pop di
  pop es
  jmp tryLoad

writeOk:
  mov al,'.'
;  int 0x65
  writeCharacter


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

  mov al,'B'
  writeCharacter

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
