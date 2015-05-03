  %include "../defaults_bin.asm"

  ; Set up drive parameters
  xor ax,ax
  mov ds,ax
  setInterrupt 0x1e, driveParameters


  mov ah,0  ; Subfunction 0 = Reset Disk System
  mov dl,0  ; Drive 0 (A:)
  int 0x13
  outputHex
  outputNewLine

  initCGA 8

tryLoad:
  ; Set load location
  mov bx,0x1000
  mov es,bx
  xor di,di

  ; Push a copy to use when we write the image to disk
  push es
  push di

  stopScreen

loadCylinderLoop:

  mov byte[requestBuffer+7],0
loadHeadLoop:

   push ax
   push dx
   mov dx,0x3d9
   mov al,1
   out dx,al
   pop dx
   pop ax

  ; Output host interrupt request
  push cx
  mov si,requestBuffer
  mov ax,cs
  mov ds,ax
  mov cx,19
  outputString
  pop cx

   push ax
   push dx
   mov dx,0x3d9
   mov al,2
   out dx,al
   pop dx
   pop ax

;   push cx
;   xor cx,cx
;   loop $
;   pop cx

  ; Read data
  xor di,di
  loadData

   push ax
   push dx
   mov dx,0x3d9
   mov al,3
   out dx,al
   pop dx
   pop ax

;   push cx
;   xor cx,cx
;   loop $
;   pop cx

  ; Read result
  mov ax,cs
  mov es,ax
  mov di,resultBuffer

   push ax
   push dx
   mov dx,0x3d9
   mov al,4
   out dx,al
   pop dx
   pop ax

  loadData

   push ax
   push dx
   mov dx,0x3d9
   mov al,5
   out dx,al
   pop dx
   pop ax

;   push cx
;   xor cx,cx
;   loop $
;   pop cx

  add bx,(512*9)/16
  mov es,ax

  inc byte[requestBuffer+7]
  cmp byte[requestBuffer+7],2
  jne loadHeadLoop

  inc byte[requestBuffer+5]
  cmp byte[requestBuffer+5],40
  jne loadCylinderLoop

  mov cx,0
  outputString

  resumeScreen
  outputCharacter 'D'

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
  outputCharacter 'W'
  pop ax
  outputHex
  outputNewLine

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
  outputString

  pop di
  pop es
  jmp tryLoad

writeOk:
  outputCharacter '.'


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

  outputCharacter 'B'

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

requestBuffer:
  db 5, 0x13, 9,2, 1,0, 0,0

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

resultBuffer:
  db 0,0,0

programEnd:
