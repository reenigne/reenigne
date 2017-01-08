org 0x100
cpu 8086

  cli
  mov ax,cs
  mov ds,ax
  mov ss,ax
  mov sp,stackHigh-2
  sti

  mov si,0x81
searchLoop
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

  ; Load meta file

  mov ax,0x3d00
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
  mov [cgadSize],ax
  mov ax,0x4200          ; seek to start
  int 0x21
  jc error
  mov cx,[cgadSize]
  mov ah,0x3f
  mov dx,cgadStart
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
  cmp word[cgadStart],0x4743    ; 'CG'
  jne error
  cmp word[cgadStart+2],0x4441  ; 'AD'
  jne error
  cmp word[cgadStart+4],0       ; version low
  jne error
  cmp word[cgadStart+6],0       ; version high
  jne error

  mov ax,6
  int 0x10
  mov dx,0x3d8
  mov al,0x1a
  out dx,al
  mov ax,0xb800
  mov es,ax
  xor di,di
  mov si,cgadStart
  mov cx,[cgadSize]
  cli
  rep movsb

  mov ah,0x00
  int 0x16

  mov ax,3
  int 0x10

  int 0x20

cgadSize: dw 0

errorMessage: db "File error$"

stackLow:
  times 128 dw 0
stackHigh:

cgadStart:
