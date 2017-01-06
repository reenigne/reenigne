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
  cmp al,0x10
  jne searchLoop
foundEnd:
  dec si
  mov byte[si],0

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
  mov [vramSize],ax
  mov ax,0x4200          ; seek to start
  int 0x21
  jc error
  mov cx,[vramSize]
  mov ah,0x3f
  mov dx,vramStart
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
  mov ax,6
  int 0x10
  mov dx,0x3d8
  mov al,0x1a
  out dx,al
  mov ax,0xb800
  mov es,ax
  xor di,di
  mov si,vramStart
  mov cx,[vramSize]
  cli
  rep movsb

  mov ah,0x00
  int 0x16

  mov ax,3
  int 0x10

  int 0x20

vramSize: dw 0

vramFileName: db "vram.dat",0
errorMessage: db "File error$"

stackLow:
  times 128 dw 0
stackHigh:

vramStart:
