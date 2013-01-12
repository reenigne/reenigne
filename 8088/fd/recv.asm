  %include "../defaults_com.asm"

  initSerial

tryLoad:
  ; Set load location
  mov ax,cs
  add ax,(programEnd + 15)>>4
  mov es,ax
  xor di,di

  ; Push a copy to use when we write the image to disk
  push es
  push di

  ; Load the data
  loadSerialData

  printCharacter 'D'


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
  printCharacter 'W'
  pop ax
  printHex
  printNewLine

  mov ah,0  ; Subfunction 0 = Reset Disk System
  mov dl,0  ; Drive 0 (A:)
  int 0x13

  inc byte[cs:retry]
  cmp byte[cs:retry],10
  jl retryLoop

  mov si,diskFailMessage
  mov cx,diskFailMessageEnd - diskFailMessage
  mov ax,cs
  mov ds,ax
  printString

  pop di
  pop es
  jmp tryLoad

writeOk:
  printCharacter '.'


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

  printCharacter 'B'

  ; Jump back into BIOS to boot from the newly written disk
  mov ax,0x40
  push ax
  jmp 0xf000:0xe518


cylinder:
  db 0
head:
  db 0

retry:
  db 0
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
