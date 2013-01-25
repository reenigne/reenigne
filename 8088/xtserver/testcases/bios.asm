org 0
cpu 8086

  mov dl,0
  mov cx,0x2000
  mov ax,0xf000
  mov ds,ax
  mov si,0xe000
  int 0x66
  int 0x67

