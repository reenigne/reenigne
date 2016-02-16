org 0x100
cpu 8086

  cli
  mov ax,cs
  mov ds,ax
  mov ss,ax
  mov sp,stackHigh-2
  sti

  mov ax,startData
  add ax,15
  mov cl,4
  shr ax,cl
  mov bx,cs
  add bx,ax
  mov [startSegment],bx

  ; Load image file

  mov ax,0x3d00
  mov dx,imageFileName
  int 0x21               ; open file
  jc error
  mov bx,ax
  mov ax,0x4202
  xor cx,cx
  xor dx,dx
  int 0x21               ; seek to end
  jnc noError
error:
  mov ah,9
  mov dx,errorMessage
  int 0x21
  int 0x20
noError:
  shr dx,1
  rcr ax,1
  shr dx,1
  rcr ax,1
  shr dx,1
  rcr ax,1
  shr dx,1
  rcr ax,1
  mov di,ax
  mov si,[startSegment]
  add ax,si
  mov [endSegment],ax
  mov ds,ax
  cmp ax,0xa000
  jae error
  mov ax,0x4200
  xor dx,dx
  int 0x21               ; seek to start
  jc error
loadLoop:
  cmp di,0x800
  jae fullBlock
  cmp di,0
  je loadDone
  mov ah,0x3f
  mov cx,di
  shl cx,1
  shl cx,1
  shl cx,1
  shl cx,1
  mov ds,si
  xor dx,dx
  int 0x21
  jc error
loadedABlock:
  shr cx,1
  shr cx,1
  shr cx,1
  shr cx,1
  sub di,cx
  add si,cx
  jmp loadLoop
fullBlock:
  mov ah,0x3f
  mov cx,0x8000
  mov ds,si
  xor dx,dx
  int 0x21
  jc error
  jmp loadedABlock
loadDone:
  mov ah,0x3e
  int 0x21
  jc error

  mov ax,0x0e  ;640x200x16 EGA
  int 0x10

  mov ax,0xa000
  mov es,ax
  mov bx,[cs:startSegment]
  mov ds,bx
  cld

  mov dx,0x3c4
  mov ax,0x0102
planeLoop:
  out dx,ax

  mov cx,40*200
  xor si,si
  xor di,di
  rep movsw
;  rep stosw

  add bx,(80*200)/16
  mov ds,bx

  shl ah,1
  cmp ah,8
  jle planeLoop

  mov ah,0
  int 0x16
  mov ax,3
  int 0x10
  int 0x20

startSegment: dw 0
endSegment: dw 0

imageFileName: db "image.dat",0
errorMessage: db "File error$"

stackLow:
  times 128 dw 0
stackHigh:

startData:
