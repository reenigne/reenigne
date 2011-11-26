cpu 8086
org 0

  mov dx,0x3a8
  in al,dx
  int 0x60
  int 0x67

