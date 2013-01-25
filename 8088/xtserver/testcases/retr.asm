org 0
cpu 8086

  mov dl,0
  mov cx,0xffff
  mov ax,cs
  mov ds,ax
  mov si,0
  int 0x66
  int 0x67

