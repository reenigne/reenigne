org 0x100
cpu 8086

  cli
  mov ax,cs
  mov ds,ax
  mov ss,ax
  mov sp,stackHigh-2
  sti

  mov si,0x82
searchLoop
  lodsb
  cmp al,0x20
  je foundEnd
  cmp al,0x0d
  jne searchLoop
foundEnd:
  dec si
  mov byte[si],0
  cmp si,0x82
  je error

  ; Load meta file

  mov ax,0x3d00
  mov dx,0x82
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
  mov [datSize],ax
  mov ax,0x4200          ; seek to start
  int 0x21
  jc error
  mov cx,[datSize]
  mov ah,0x3f
  mov dx,datStart
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
  mov dx,0x3d8
  mov al,9
  out dx,al

  inc dx
  mov al,0
  out dx,al

  mov dl,0xd4
  mov ax,0x7100
  out dx,ax
  mov ax,0x5001
  out dx,ax
  mov ax,0x5a02
  out dx,ax
  mov ax,0x0a03
  out dx,ax
  mov ax,0x7f04
  out dx,ax
  mov ax,0x0605
  out dx,ax
  mov ax,0x6406
  out dx,ax
  mov ax,0x7007
  out dx,ax
  mov ax,0x0208
  out dx,ax
  mov ax,0x0109
  out dx,ax
  mov ax,0x060a
  out dx,ax
  mov ax,0x070b
  out dx,ax
  mov ax,0x000c
  out dx,ax
  inc ax
  out dx,ax
  mov ax,0x030e
  out dx,ax
  mov ax,0xc00f
  out dx,ax

  mov ax,0xb800
  mov es,ax
  xor di,di
  mov si,datStart
  mov cx,0x2000
  rep movsw

  mov ah,0x00
  int 0x16

  mov ax,3
  int 0x10

  mov ax,0x4c00
  int 0x21

datSize: dw 0

errorMessage: db "File error$"

stackLow:
  times 128 dw 0
stackHigh:

datStart:

