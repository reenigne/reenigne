org 0x100
cpu 8086

  mov si,0x81
searchArgLoop:
  lodsb
  cmp al,0x20
  je searchArgLoop
  dec si
  mov [startOfArg + 1],si

searchLoop:
  lodsb
  cmp al,0x20
  je foundEnd
  cmp al,0x0d
  jne searchLoop
foundEnd:
  dec si
  mov byte[si],0
  cmp si,0x81
  je error

  ; Load data file

  mov ax,0x3d00
startOfArg:
  mov dx,0x81
  int 0x21               ; open file
  jc error
  mov bx,ax
  mov ax,0x4202
  xor cx,cx
  xor dx,dx
  int 0x21               ; seek to end
  jc error
  cmp dx,0
  jne error
  mov [dataSize],ax
  mov ax,0x4200          ; seek to start
  int 0x21
  jc error
  mov cx,[dataSize]
  mov ah,0x3f
  mov dx,dataStart
  int 0x21               ; read file
  jc error
  mov ah,0x3e
  int 0x21               ; close file
  jnc success
error:
  mov ah,9
  mov dx,errorMessage
  int 0x21
  ret

success:
  mov word[dataOffset],dataStart

  xor ax,ax
  mov es,ax
  mov ax,[es: 8*4]
  mov [oldInterrupt8],ax
  mov ax,[es: 8*4 + 2]
  mov [oldInterrupt8 + 2],ax
  cli
  mov word[es: 8*4],interrupt8
  mov word[es: 8*4 + 2],cs
  sti
  mov al,0x34
  out 0x43,al
  mov al,19912 & 0xff
  out 0x40,al
  mov al,19912 >> 8
  out 0x40,al
  mov al,0xb6
  out 0x43,al

  in al,0x61
  or al,3
  out 0x61,al

waitLoop:
;  mov ah,9
;  mov dx,errorMessage + 9
;  int 0x21


  hlt
  mov ax,[dataOffset]
  sub ax,dataStart
  cmp ax,[dataSize]
  jb waitLoop

  in al,0x61
  and al,0xfc
  out 0x61,al
  mov al,0x34
  out 0x43,al
  mov al,0
  out 0x40,al
  out 0x40,al

  cli
  mov ax,[oldInterrupt8]
  mov [es: 8*4],ax
  mov ax,[oldInterrupt8 + 2]
  mov [es: 8*4 + 2],ax
  sti

  mov ax,0x4c00
  int 0x21

interrupt8:
  push ax
  push si
  mov si,[cs:dataOffset]
  mov ax,[cs:si]
  inc si
  inc si
  mov [cs:dataOffset],si
  out 0x42,al
  mov al,ah
  out 0x42,al
  pop si

  mov ax,[cs:time]
  add ax,19912
  mov [cs:time],ax
  jnc noOldInterrupt
  pop ax
  jmp far[cs:oldInterrupt8]

noOldInterrupt:
  mov al,0x20
  out 0x20,al
  pop ax
  iret


time: dw 0
dataOffset: dw 0

oldInterrupt8:
  dw 0, 0

dataSize: dw 0

errorMessage: db "File error$"

dataStart:
