compressedDataPointer: dw 0

vProgram:
  inc word[cs:compressedDataPointer]

  mov es,[cs:compressedDataPointer]

  mov al,[es:0x0]
  mov [ss:w0+3],al

  mov al,[es:0x3]
  mov [ss:w1+3],al

  mov al,[es:0x6]
  mov [ss:w2+3],al

  mov al,[es:0x9]
  mov [ss:w3+3],al

  mov ax,[es:0x1]
  mov [ss:f0+2],ax

  mov ax,[es:0x4]
  mov [ss:f1+2],ax

  mov ax,[es:0x7]
  mov [ss:f2+2],ax

  mov ax,[es:0xa]
  mov [ss:f3+2],ax

  w0: dw 0, 0

  f0: dw 0, 0

  w1: dw 0, 0

  f1: dw 0, 0

  w2: dw 0, 0

  f2: dw 0, 0

  w3: dw 0, 0

  f3: dw 0, 0

  mov sp,[es:0xc]

crtcUpdate:
  mov ax,[es:0xe]
  mov [ss:sa1+7],al

  mov ax,[es:0xf]
  mov [ss:sa2+7],al

  sa1: mov bx,dx
  mov dx,0x3d4
  mov ax,0x990c
  out dx,ax
  mov dx,bx

  sa2: mov bx,dx
  mov dx,0x3d4
  mov ax,0x990d
  out dx,ax
  mov dx,bx

  mov sp,vProgram

teletype:
  inc word[cs:screenUpdate+3]

  inc word[cs:screenUpdate+3]

  mov ax,[es:0xe]
  mov [ss:screenUpdate+5],ax

  mov ax,0xb800
  mov es,ax

  screenUpdate: mov word[es:0x1234],0x5678

  mov sp,vProgram

moveCursor:
  mov ax,[es:0xe]
  mov [ss:screenUpdate+3],ax

  mov sp,vProgram

finish:
  dw 0, 0x9090

