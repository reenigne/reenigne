cpu 8086
;org 0x100
org 0

MULTIPLIER equ 0x600

  mov al,0x34
  out 0x43,al
  mov al,0
  out 0x40,al
  out 0x40,al
  xor ax,ax
  mov ds,ax
  mov word[0x20],interrupt8
  mov [0x22],cs
;  mov ax,[0x6c]
;  mov [cs:timer],ax


  ; Set video mode

  mov ax,6 ;4
  int 0x10
  mov dx,0x3d8
  mov al,0x1a ;0x0a
  out dx,al
  mov al,0x0f ;0x30
  inc dx
  out dx,al

  ; Set up segments and stack

  mov ax,cs
  mov ds,ax
  add ax,(codeEnd + 15) >> 4
  mov es,ax
  mov [squareTableSegment],ax
  ; We want SP to be 0x1c00 + 4 (for the two PUSHed loop variables)
  ; cs*16 + stackEnd + fudge == ss*16 + 0x1c04
  ; ss = cs + (stackEnd + fudge - 0x1c04)/16
  mov ax,cs
  add ax,(stackEnd - 0x1c04) >> 4
  cli
  mov ss,ax
  mov sp,0x1c04
  sti

  ; Create square table in ES

  mov cx,0x4000
  mov bx,MULTIPLIER/2
  xor bp,bp
  xor di,di
  mov si,MULTIPLIER/4
  xor ax,ax
  stosw
squareLoop:
  dw 0xc381,1  ;add bx,0001  not simplified by the assembler since we need to modify it
  adc bp,0
  add word[squareLoop+2],2
  cmp bp,si
  jb noOverflow
  sub bp,si
noOverflow:
  mov ax,bx
  mov dx,bp
  div si
  and ax,0xfffe
  stosw
  neg di
  mov [es:di],ax
  neg di
  loop squareLoop

  ; Create table of "a" values

  mov ax,cs
  mov es,ax
  mov di,aTable
  mov cx,320
  mov bx,-3456*5
  mov si,72
  mov bp,5
aLoop:
  mov ax,bx
  cwd
  idiv bp
  and ax,0xfffe
  stosw
  add bx,si
  loop aLoop

  ; Draw the fractal

  mov ds,[squareTableSegment]

  mov cx,101
yLoop:
  push cx
  mov ax,cx
  dec ax
  mov si,ax
  mov dx,3456
  imul dx
  mov cx,200
  idiv cx
  and ax,0xfffe
  mov dx,ax

  add si,si
  mov ax,[cs:yTable+200+si]
  mov [cs:yOffsetBottom+2],ax
  neg si
  sub ax,[cs:yTable+200+si]
  inc ax
  mov [cs:yOffsetTop+2],ax

  mov cx,320
xLoop:
  push cx
  mov bx,320
  sub bx,cx
  add bx,bx
  mov es,[cs:bx+aTable]

  mov si,es  ; x = a
  mov bx,dx  ; y = b
  mov cx,32

  mov di,[si]  ; x*x
  mov bp,[bx]  ; y*y
  lea ax,[di+bp] ; x*x+y*y
  cmp ax,sp
  jae escaped5

%macro iterate 0
  dec cx
  mov bx,[si+bx] ; (x+y)*(x+y)
  sub bx,ax  ; 2*x*y
  add bx,dx  ; 2*x*y+b -> new y
  mov si,es
  add si,di
  sub si,bp  ; x*x-y*y+a -> new x
  mov di,[si]  ; x*x
  mov bp,[bx]  ; y*y
  lea ax,[di+bp] ; x*x+y*y
  cmp ax,sp
%endmacro

  iterate
  jb notEscaped5
escaped5:
  jmp escaped
notEscaped5:
  iterate
  jae escaped4
  iterate
  jae escaped4
  iterate
  jae escaped4
  iterate
  jae escaped4
  iterate
  jae escaped4
  iterate
  jb notEscaped4
escaped4:
  jmp escaped
notEscaped4:
  iterate
  jae escaped3
  iterate
  jae escaped3
  iterate
  jae escaped3
  iterate
  jae escaped3
  iterate
  jae escaped3
  iterate
  jb notEscaped3
escaped3:
  jmp escaped
notEscaped3:
  iterate
  jae escaped2
  iterate
  jae escaped2
  iterate
  jae escaped2
  iterate
  jae escaped2
  iterate
  jae escaped2
  iterate
  jb notEscaped2
escaped2:
  jmp escaped
notEscaped2:
  iterate
  jae escaped1
  iterate
  jae escaped1
  iterate
  jae escaped1
  iterate
  jae escaped1
  iterate
  jae escaped1
  iterate
  jb notEscaped1
escaped1:
  jmp escaped
notEscaped1:
  iterate
  jae escaped
  iterate
  jae escaped
  iterate
  jae escaped
  iterate
  jae escaped
  iterate
  jae escaped
  iterate
  jae escaped
  dec cx
escaped:
  mov bx,cx
  mov ah,[cs:colourTable+bx]

  pop di
  mov cx,di
  neg di
  add di,320
  mov bx,di
  shr di,1
  shr di,1
  mov si,0xb800
  mov es,si

  and bx,3
  add bx,bx
  mov bx,[cs:bx+maskTable]

yOffsetBottom:
  add di,9999
  mov al,[es:di]
  and ax,bx
  or al,ah
  stosb

yOffsetTop:
  sub di,9999
  mov al,[es:di]
  and al,bl
  or al,ah
  stosb

  loop xLoop2
  pop cx
  loop yLoop2

;  xor ax,ax
;  mov ds,ax
;  mov ax,[0x6c]
;  sub ax,[cs:timer]
  mov ax,[cs:timer]
  int 0x63 ; outputHex

  mov ah,0
  int 0x16
  mov ax,3
  int 0x10
  mov ax,0x4c00
  int 0x21

yLoop2: jmp yLoop
xLoop2: jmp xLoop

interrupt8:
  push ax
  mov al,0x20
  out 0x20,al
  inc word[cs:timer]
  pop ax
  iret

squareTableSegment: dw 0
aTable:
  times 320 dw 0
colourTable:
;  db 0x00,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa
  db 0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff,0x11,0x22
yTable:
  dw 0x0000, 0x2000, 0x0050, 0x2050, 0x00a0, 0x20a0, 0x00f0, 0x20f0
  dw 0x0140, 0x2140, 0x0190, 0x2190, 0x01e0, 0x21e0, 0x0230, 0x2230
  dw 0x0280, 0x2280, 0x02d0, 0x22d0, 0x0320, 0x2320, 0x0370, 0x2370
  dw 0x03c0, 0x23c0, 0x0410, 0x2410, 0x0460, 0x2460, 0x04b0, 0x24b0
  dw 0x0500, 0x2500, 0x0550, 0x2550, 0x05a0, 0x25a0, 0x05f0, 0x25f0
  dw 0x0640, 0x2640, 0x0690, 0x2690, 0x06e0, 0x26e0, 0x0730, 0x2730
  dw 0x0780, 0x2780, 0x07d0, 0x27d0, 0x0820, 0x2820, 0x0870, 0x2870
  dw 0x08c0, 0x28c0, 0x0910, 0x2910, 0x0960, 0x2960, 0x09b0, 0x29b0
  dw 0x0a00, 0x2a00, 0x0a50, 0x2a50, 0x0aa0, 0x2aa0, 0x0af0, 0x2af0
  dw 0x0b40, 0x2b40, 0x0b90, 0x2b90, 0x0be0, 0x2be0, 0x0c30, 0x2c30
  dw 0x0c80, 0x2c80, 0x0cd0, 0x2cd0, 0x0d20, 0x2d20, 0x0d70, 0x2d70
  dw 0x0dc0, 0x2dc0, 0x0e10, 0x2e10, 0x0e60, 0x2e60, 0x0eb0, 0x2eb0
  dw 0x0f00, 0x2f00, 0x0f50, 0x2f50, 0x0fa0, 0x2fa0, 0x0ff0, 0x2ff0
  dw 0x1040, 0x3040, 0x1090, 0x3090, 0x10e0, 0x30e0, 0x1130, 0x3130
  dw 0x1180, 0x3180, 0x11d0, 0x31d0, 0x1220, 0x3220, 0x1270, 0x3270
  dw 0x12c0, 0x32c0, 0x1310, 0x3310, 0x1360, 0x3360, 0x13b0, 0x33b0
  dw 0x1400, 0x3400, 0x1450, 0x3450, 0x14a0, 0x34a0, 0x14f0, 0x34f0
  dw 0x1540, 0x3540, 0x1590, 0x3590, 0x15e0, 0x35e0, 0x1630, 0x3630
  dw 0x1680, 0x3680, 0x16d0, 0x36d0, 0x1720, 0x3720, 0x1770, 0x3770
  dw 0x17c0, 0x37c0, 0x1810, 0x3810, 0x1860, 0x3860, 0x18b0, 0x38b0
  dw 0x1900, 0x3900, 0x1950, 0x3950, 0x19a0, 0x39a0, 0x19f0, 0x39f0
  dw 0x1a40, 0x3a40, 0x1a90, 0x3a90, 0x1ae0, 0x3ae0, 0x1b30, 0x3b30
  dw 0x1b80, 0x3b80, 0x1bd0, 0x3bd0, 0x1c20, 0x3c20, 0x1c70, 0x3c70
  dw 0x1cc0, 0x3cc0, 0x1d10, 0x3d10, 0x1d60, 0x3d60, 0x1db0, 0x3db0
  dw 0x1e00, 0x3e00, 0x1e50, 0x3e50, 0x1ea0, 0x3ea0, 0x1ef0, 0x3ef0
  dw 0x1f40
maskTable:
  dw 0xc03f,0x30cf,0x0cf3,0x03fc
timer: dw 0
stackStart:
  times 128 db 0
stackEnd:
codeEnd:
