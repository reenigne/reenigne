cpu 8086
org 0x100
;org 0

MULTIPLIER equ 0x600
maxX equ 320
maxY equ 101
initialShift equ 5
initialGrid equ (1 << initialShift)
itersX equ ((maxX + initialGrid - 1)/initialGrid)*initialGrid + 1
itersY equ ((maxY + initialGrid - 1)/initialGrid)*initialGrid + 1

;  mov al,0x34
;  out 0x43,al
;  mov al,0
;  out 0x40,al
;  out 0x40,al
;  xor ax,ax
;  mov ds,ax
;  mov word[0x20],interrupt8
;  mov [0x22],cs

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

  ; Create table of y addresses

  xor ax,ax
  mov bx,0x2000
  mov dx,0x50-0x2000
  mov cx,100
yTableLoop:
  stosw
  add ax,bx
  stosw
  add ax,dx
  loop yTableLoop
  stosw

  ; Create table of itersX multiples

  xor ax,ax
  mov bx,itersX
  mov cx,itersY
itersXTableLoop:
  stosw
  add ax,bx
  loop itersXTableLoop


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

;  mov ax,[cs:timer]
;  int 0x63 ; outputHex

  mov ah,0
  int 0x16
  mov ax,3
  int 0x10
  mov ax,0x4c00
  int 0x21

yLoop2: jmp yLoop
xLoop2: jmp xLoop

;interrupt8:
;  push ax
;  mov al,0x20
;  out 0x20,al
;  inc word[cs:timer]
;  pop ax
;  iret


; assume DS points to iters array
; assume SI is yp*2
; assume BX is xp
; assume ES is 0xb800
; subdivide5:
%macro subdivide 2
subdivide%1_%2:
  %assign z (1 << %i)
  mov ax,bx
  add bx,[si+itersXTable]
  mov al,[bx]
  cmp al,[bx+z]
  jne %%doSubdivide
  cmp al,[bx+z*itersX]
  jne %%doSubdivide
  cmp al,[bx+z*(itersX+1)]
  jne %%doSubdivide
  mov di,ax
  add di,[si+yTable+200]

  times 4 stosw



%endmacro


doSubDivide5:
  lea bx,[di+16]
  call mandelIters
  add si,32
  call mandelIters
  sub bx,16
  call mandelIters
  add bx,32
  call mandelIters
  sub bx,32
  call subdivide4
  add bx,16
  call subdivide4
  cmp si,192+32
  je .noLowerHalf
  add si,64
  call mandelIters
  sub si,32
  call subdivide4
  sub bx,16
  call subdivide4
  sub si,32
  ret
.noLowerHalf:
  sub bx,16
  sub si,32
  ret

subdivide5:
  mov di,bx
  add bx,[si+itersXTable]
  mov al,[bx]
  cmp al,[bx+32]
  jne doSubdivide5
  cmp al,[bx+32*itersX]
  jne doSubdivide5
  cmp al,[bx+32*(itersX+1)]
  jne doSubdivide5

  ; Fill square with colour

  mov bx,di
  add di,[si+yTable+200]
  mov ah,al
  mov dx,0x2000-8
  mov bp,0x50-0x2000-8

  %rep 2
    times 4 stosw
    add di,dx
    times 4 stosw
    add di,bp
  %endrep
  times 4 stosw
  cmp si,192
  je .plotUpper
  %rep 13
    add di,dx
    times 4 stosw
    add di,bp
    times 4 stosw
  %endrep
  add di,dx
  times 4 stosw
  ret

.plotUpper:
  add di,-8000-8
  %rep 2
    times 4 stosw
    add di,dx
    times 4 stosw
    add di,bp
  %endrep
  times 4 stosw
  ret


doSubDivide4:
  lea bx,[di+8]
  call mandelIters
  add si,16
  call mandelIters
  sub bx,8
  call mandelIters
  add bx,16
  call mandelIters
  sub bx,16
  call subdivide4
  add bx,8
  call subdivide4
  cmp si,192+16
  je .noLowerHalf
  add si,32
  call mandelIters
  sub si,16
  call subdivide4
  sub bx,8
  call subdivide4
  sub si,16
  ret
.noLowerHalf:
  sub bx,8
  sub si,16
  ret

subdivide5:
  mov di,bx
  add bx,[si+itersXTable]
  mov al,[bx]
  cmp al,[bx+16]
  jne doSubdivide5
  cmp al,[bx+16*itersX]
  jne doSubdivide5
  cmp al,[bx+16*(itersX+1)]
  jne doSubdivide5

  ; Fill square with colour

  mov bx,di
  add di,[si+yTable+200]
  mov ah,al
  mov dx,0x2000-4
  mov bp,0x50-0x2000-4

  %rep 2
    times 2 stosw
    add di,dx
    times 2 stosw
    add di,bp
  %endrep
  times 2 stosw
  cmp si,192
  je .plotUpper
  %rep 5
    add di,dx
    times 2 stosw
    add di,bp
    times 2 stosw
  %endrep
  add di,dx
  times 2 stosw
  ret

.plotUpper:
  add di,-8000-4
  %rep 2
    times 2 stosw
    add di,dx
    times 2 stosw
    add di,bp
  %endrep
  times 2 stosw
  ret




colourTable:
;  db 0x00,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa
;  db 0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff,0x11,0x22
  db 0x00, 0xff, 0xff, 0xff, 0xff, 0xee, 0xee, 0xee, 0xee, 0xaa, 0xaa
  db 0xaa, 0xbb, 0xbb, 0xbb, 0x99, 0x99, 0x99, 0x88, 0x88, 0x11, 0x11
  db 0x33, 0x33, 0x22, 0x22, 0x66, 0x77, 0x55, 0x44, 0xcc, 0xdd, 0xff
maskTable:
  dw 0xc03f,0x30cf,0x0cf3,0x03fc

section .bss

squareTableSegment: resw 2
aTable:
  resw 320
yTable:
  resw 201
itersXTable:
  resw itersY
;timer: dw 0
stackStart:
  resb 128
stackEnd:
codeEnd:
