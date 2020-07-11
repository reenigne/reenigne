org 0x100
cpu 8086

  mov ax,0x1200
  mov bx,0xff10
  int 0x10


;%if 0
  push bx

  push ax
  and al,0x0f
  add al,'0'
  mov ah,0x0e
  mov bh,0
  int 0x10
  pop ax

  push ax
  shr al,1
  shr al,1
  shr al,1
  shr al,1
  and al,0x0f
  add al,'0'
  mov ah,0x0e
  mov bh,0
  int 0x10
  pop ax

  push ax
  mov al,ah
  and al,0x0f
  add al,'0'
  mov ah,0x0e
  mov bh,0
  int 0x10
  pop ax

  push ax
  mov al,ah
  shr al,1
  shr al,1
  shr al,1
  shr al,1
  and al,0x0f
  add al,'0'
  mov ah,0x0e
  mov bh,0
  int 0x10
  pop ax

  pop bx


  push bx
  push ax
  mov al,bl
  and al,0x0f
  add al,'0'
  mov ah,0x0e
  mov bh,0
  int 0x10
  pop ax
  pop bx

  push bx
  push ax
  mov al,bl
  shr al,1
  shr al,1
  shr al,1
  shr al,1
  and al,0x0f
  add al,'0'
  mov ah,0x0e
  mov bh,0
  int 0x10
  pop ax
  pop bx

  push bx
  push ax
  mov al,bh
  and al,0x0f
  add al,'0'
  mov ah,0x0e
  mov bh,0
  int 0x10
  pop ax
  pop bx

  push bx
  push ax
  mov al,bh
  shr al,1
  shr al,1
  shr al,1
  shr al,1
  and al,0x0f
  add al,'0'
  mov ah,0x0e
  mov bh,0
  int 0x10


  mov al,cl
  and al,0x0f
  add al,'0'
  mov ah,0x0e
  mov bh,0
  int 0x10

  mov al,cl
  shr al,1
  shr al,1
  shr al,1
  shr al,1
  and al,0x0f
  add al,'0'
  mov ah,0x0e
  mov bh,0
  int 0x10

  mov al,ch
  and al,0x0f
  add al,'0'
  mov ah,0x0e
  mov bh,0
  int 0x10

  mov al,ch
  shr al,1
  shr al,1
  shr al,1
  shr al,1
  and al,0x0f
  add al,'0'
  mov ah,0x0e
  mov bh,0
  int 0x10



  mov ah,0
  int 0x16
  pop ax
  pop bx
;%endif

