  org 0x100

  mov ax,6
  int 0x10
  mov ax,0xb800
  mov es,ax

  mov di,0
  mov cx,100
yLoop:
  mov byte[es:di],0x80
  mov byte[es:di+79],1
  mov byte[es:di+0x2000],0x80
  mov byte[es:di+79+0x2000],1
  add di,80
  loop yLoop

  mov di,0
  mov ax,0xffff
  mov cx,40
  rep stosw

  mov di,7920 + 0x2000
  mov cx,40
  rep stosw

  mov ax,0
  int 0x16
  ret

