org 0
cpu 8086

  mov ax,3
  int 0x10
  mov dx,0x3d8
  mov al,9
  out dx,al
  mov dx,0x3d4
  mov ax,0x3
  out dx,ax

  mov ax,0xb800
  mov es,ax
  xor di,di

  mov cx,25
top:
  mov ax,0x0000
  times 5 stosw
  mov ax,0x1100
  times 5 stosw
  mov ax,0x2200
  times 5 stosw
  mov ax,0x3300
  times 5 stosw
  mov ax,0x4400
  times 5 stosw
  mov ax,0x5500
  times 5 stosw
  mov ax,0x6600
  times 5 stosw
  mov ax,0x7700
  times 5 stosw
  mov ax,0x8800
  times 5 stosw
  mov ax,0x9900
  times 5 stosw
  mov ax,0xaa00
  times 5 stosw
  mov ax,0xbb00
  times 5 stosw
  mov ax,0xcc00
  times 5 stosw
  mov ax,0xdd00
  times 5 stosw
  mov ax,0xee00
  times 5 stosw
  mov ax,0xff00
  times 5 stosw
  loop top2
  hlt
top2:
  jmp top
