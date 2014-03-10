org 0
cpu 8086

  mov ax,cs
  mov ds,ax
  mov bx,foo
  call [bx]
  mov ax,0x1234
  int 0x63
  int 0x67
bar:
  mov ax,0x5678
  int 0x63
  ret

foo: dw bar

