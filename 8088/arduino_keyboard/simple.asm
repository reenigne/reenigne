  mov ax,0xb800
  mov es,ax
  mov byte[es:0],42
  hlt
