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
  add ax,(codeEndInit + 15) >> 4
  mov es,ax
  mov cx,18
  mov si,colourTableInit
  mov di,colourTable
  rep movsw                   ; Want iters array to be at DS:0 so copy the colour table past it
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

  mov ax,ds
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

  mov ax,50*80
  mov bx,0x2000
  mov dx,80-0x2000
  mov cx,50
yTableLoopLower:
  stosw
  add ax,bx
  stosw
  add ax,dx
  loop yTableLoopLower
  stosw
  stosw
  mov ax,50*80
  mov cx,50
yTableLoopUpper:
  stosw
  sub ax,dx
  stosw
  sub ax,bx
  loop yTableLoopUpper
  stosw
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

;  mov ds,[squareTableSegment]
;
;  mov cx,101
;yLoop:
;  push cx
;  mov ax,cx
;  dec ax
;  mov si,ax
;  mov dx,3456
;  imul dx
;  mov cx,200
;  idiv cx
;  and ax,0xfffe
;  mov dx,ax
;
;  add si,si
;  mov ax,[cs:yTableLower+si]
;  mov [cs:yOffsetBottom+2],ax
;  sub ax,[cs:yTableUpper+si]
;  inc ax
;  mov [cs:yOffsetTop+2],ax
;
;  mov cx,320
;xLoop:
;  push cx
;  mov bx,320
;  sub bx,cx
;  add bx,bx
;  mov es,[cs:bx+aTable]
;
;  mov si,es  ; x = a
;  mov bx,dx  ; y = b
;  mov cx,32




mandelIters:
  push bx
  push si


  push ds
  mov ds,[squareTableSegment]




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



  pop si
  pop bx
  pop ds
  ret

;  mov bx,cx
;  mov ah,[cs:colourTable+bx]
;
;  pop di
;  mov cx,di
;  neg di
;  add di,320
;  mov bx,di
;  shr di,1
;  shr di,1
;  mov si,0xb800
;  mov es,si
;
;  and bx,3
;  add bx,bx
;  mov bx,[cs:bx+maskTable]
;
;yOffsetBottom:
;  add di,9999
;  mov al,[es:di]
;  and ax,bx
;  or al,ah
;  stosb
;
;yOffsetTop:
;  sub di,9999
;  mov al,[es:di]
;  and al,bl
;  or al,ah
;  stosb
;
;  loop xLoop2
;  pop cx
;  loop yLoop2

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

;yLoop2: jmp yLoop
;xLoop2: jmp xLoop

;interrupt8:
;  push ax
;  mov al,0x20
;  out 0x20,al
;  inc word[cs:timer]
;  pop ax
;  iret

; all the subdivide routines:
; assume DS points to iters array
; assume SI is yp*2
; assume BX is xp
; assume ES is 0xb800

doSubdivide5:
  lea bx,[di+16]
  call mandelIters   ; (16,0)
  add si,32
  call mandelIters   ; (16,16)
  add bx,16
  call mandelIters   ; (32,16)
  sub bx,32
  call mandelIters   ; (0,16)
  sub si,32
  call subdivide4    ;  (0,0)
  add bx,16
  call subdivide4    ;  (16,0)
  cmp si,192
  je .noLowerHalf
  add si,64
  call mandelIters   ; (16,32)
  sub si,32
  call subdivide4    ;  (16,16)
  sub bx,16
  call subdivide4    ;  (0,16)
  sub si,32
  ret
.noLowerHalf:
  sub bx,16
  ret

plotUpper5:
  add di,-8000-8
  %rep 2
    times 4 stosw
    add di,dx
    times 4 stosw
    add di,bp
  %endrep
  times 4 stosw
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

  mov bx,colourTable
  xlatb
  mov bx,di
  shr di,1
  shr di,1
  mov cx,di
  add di,[si+yTableLower]
  mov ah,al
  mov dx,0x2000-8
  mov bp,80-0x2000-8

  %rep 2
    times 4 stosw
    add di,dx
    times 4 stosw
    add di,bp
  %endrep
  times 4 stosw
  cmp si,192
  je plotUpper5
  %rep 13
    add di,dx
    times 4 stosw
    add di,bp
    times 4 stosw
  %endrep
  add di,dx
  times 4 stosw

  mov di,cx
  add di,[si+yTableUpper+64]
  %rep 15
    times 4 stosw
    add di,dx
    times 4 stosw
    add di,bp
  %endrep
  times 4 stosw
  add di,dx
  times 4 stosw
  ret



doSubdivide4:
  lea bx,[di+8]
  call mandelIters   ; (8,0)
  add si,16
  call mandelIters   ; (8,8)
  add bx,8
  call mandelIters   ; (16,8)
  sub bx,16
  call mandelIters   ; (0,8)
  sub si,16
  call subdivide3    ;  (0,0)
  add bx,8
  call subdivide3    ;  (8,0)
  cmp si,192
  je .noLowerHalf
  add si,32
  call mandelIters   ; (8,16)
  sub si,16
  call subdivide3    ;  (8,8)
  sub bx,8
  call subdivide3    ;  (0,8)
  sub si,16
  ret
.noLowerHalf:
  sub bx,8
  ret

subdivide4:
  mov di,bx
  add bx,[si+itersXTable]
  mov al,[bx]
  cmp al,[bx+16]
  jne doSubdivide4
  cmp al,[bx+16*itersX]
  jne doSubdivide4
  cmp al,[bx+16*(itersX+1)]
  jne doSubdivide4

  ; Fill square with colour

  mov bx,colourTable
  xlatb
  mov bx,di
  shr di,1
  shr di,1
  mov cx,di
  add di,[si+yTableLower]
  mov ah,al
  mov dx,0x2000-4
  mov bp,80-0x2000-4

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

  mov di,cx
  add di,[si+yTableUpper+32]
  %rep 7
    times 2 stosw
    add di,dx
    times 2 stosw
    add di,bp
  %endrep
  times 2 stosw
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


doSubdivide3:
  lea bx,[di+4]
  call mandelIters   ; (4,0)
  add si,8
  call mandelIters   ; (4,4)
  add bx,4
  call mandelIters   ; (8,4)
  sub bx,8
  call mandelIters   ; (0,4)
  sub si,8
  call subdivide2    ;  (0,0)
  add bx,4
  call subdivide2    ;  (4,0)
  cmp si,192
  je .noLowerHalf
  add si,16
  call mandelIters   ; (4,8)
  sub si,8
  call subdivide2    ;  (4,4)
  sub bx,4
  call subdivide2    ;  (0,4)
  sub si,8
  ret
.noLowerHalf:
  sub bx,4
  ret

subdivide3:
  mov di,bx
  add bx,[si+itersXTable]
  mov al,[bx]
  cmp al,[bx+8]
  jne doSubdivide3
  cmp al,[bx+8*itersX]
  jne doSubdivide3
  cmp al,[bx+8*(itersX+1)]
  jne doSubdivide3

  ; Fill square with colour

  mov bx,colourTable
  xlatb
  mov bx,di
  shr di,1
  shr di,1
  mov cx,di
  add di,[si+yTableLower]
  mov ah,al
  mov dx,0x2000-2
  mov bp,80-0x2000-2

  %rep 2
    stosw
    add di,dx
    stosw
    add di,bp
  %endrep
  stosw
  cmp si,192
  je .plotUpper
  add di,dx
  stosw
  add di,bp
  stosw
  add di,dx
  stosw

  mov di,cx
  add di,[si+yTableUpper+16]
  %rep 3
    stosw
    add di,dx
    stosw
    add di,bp
  %endrep
  stosw
  add di,dx
  stosw

  ret

.plotUpper:
  add di,-8000-2
  %rep 2
    stosw
    add di,dx
    stosw
    add di,bp
  %endrep
  stosw
  ret


doSubdivide2:
  lea bx,[di+2]
  call mandelIters   ; (2,0)
  add si,4
  call mandelIters   ; (2,2)
  inc bx
  inc bx
  call mandelIters   ; (4,2)
  sub bx,4
  call mandelIters   ; (0,2)
  sub si,4
  call subdivide1L   ;  (0,0)
  inc bx
  inc bx
  call subdivide1R   ;  (2,0)
  cmp si,200
  je .noLowerHalf
  add si,8
  call mandelIters   ; (2,4)
  dec si
  dec si
  dec bx
  dec bx
  call subdivide1L   ;  (0,2)
  inc bx
  inc bx
  call subdivide1R   ;  (2,2)
  dec si
  dec si
.noLowerHalf:
  dec bx
  dec bx
  ret

subdivide2:
  mov di,bx
  add bx,[si+itersXTable]
  mov al,[bx]
  cmp al,[bx+4]
  jne doSubdivide2
  cmp al,[bx+4*itersX]
  jne doSubdivide2
  cmp al,[bx+4*(itersX+1)]
  jne doSubdivide2

  ; Fill square with colour

  mov bx,colourTable
  xlatb
  mov bx,di
  shr di,1
  shr di,1
  mov cx,di
  add di,[si+yTableLower]
  mov dx,0x2000-1
  mov bp,80-0x2000-1

  stosb
  cmp si,200
  je .plotUpper
  add di,dx
  stosb
  add di,bp
  stosb
  add di,dx
  stosb

  mov di,cx
  add di,[si+yTableUpper+8]
  stosb
  add di,dx
  stosb
  add di,bp
  stosb
  add di,dx
  stosb

.plotUpper:
  add di,-8000-1
  stosb
  ret


doSubdivide1L:
  lea bx,[di+1]
  call mandelIters  ; (1,0)
  inc si
  inc si
  call mandelIters  ; (1,1)
  inc bx
  call mandelIters  ; (2,1)
  dec bx
  dec bx
  call mandelIters  ; (0,1)
  inc bx
  inc si
  inc si
  call mandelIters  ; (1,2)
  sub si,4
  dec bx
  ret

subdivide1L:
  mov di,bx
  add bx,[si+itersXTable]
  mov al,[bx]
  cmp al,[bx+2]
  jne doSubdivide1L
  cmp al,[bx+2*itersX]
  jne doSubdivide1L
  cmp al,[bx+2*(itersX+1)]
  jne doSubdivide1L

  ; Don't fill square with colour here - we'll do it in subdivide1R

  mov [bx+1],al
  mov [bx+itersX],al
  mov [bx+itersX+1],al
  ret


doSubdivide1R:
  push bx
  lea bx,[di+1]
  call mandelIters  ; (1,0)
  inc si
  inc si
  call mandelIters  ; (1,1)
  inc bx
  call mandelIters  ; (2,1)
  dec bx
  dec bx
  call mandelIters  ; (0,1)
  inc bx
  inc si
  inc si
  call mandelIters  ; (1,2)
  sub si,4
  pop bx
  jmp plot1R

subdivide1R:
  mov di,bx
  add bx,[si+itersXTable]
  mov al,[bx]
  cmp al,[bx+2]
  jne doSubdivide1R
  cmp al,[bx+2*itersX]
  jne doSubdivide1R
  cmp al,[bx+2*(itersX+1)]
  jne doSubdivide1R

  mov [bx+1],al
  mov [bx+itersX],al
  mov [bx+itersX+1],al

plot1R:
  mov cx,di
  mov bp,si
  lea si,[bx-2]
  mov bx,colourTable
  lodsb
  xlatb
  and al,0xc0
  mov ah,al
  lodsb
  xlatb
  and al,0x30
  or ah,al
  lodsb
  xlatb
  and al,0x0c
  or ah,al
  lodsb
  xlatb
  and al,0x03
  or al,ah

  shr di,1
  shr di,1
  mov dx,di
  add di,[ds:bp+yTableLower]
  stosb
  mov di,dx
  add di,[ds:bp+yTableUpper]
  stosb

  add si,itersX-4
  lodsb
  xlatb
  and al,0xc0
  mov ah,al
  lodsb
  xlatb
  and al,0x30
  or ah,al
  lodsb
  xlatb
  and al,0x0c
  or ah,al
  lodsb
  xlatb
  and al,0x03
  or al,ah

  mov di,dx
  add di,[ds:bp+yTableLower+2]
  stosb
  mov di,dx
  add di,[ds:bp+yTableUpper+2]

  mov bx,cx
  mov si,bp
  ret







colourTableInit:
;  db 0x00,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa
;  db 0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff,0x11,0x22
  db 0x00, 0xff, 0xff, 0xff, 0xff, 0xee, 0xee, 0xee, 0xee, 0xaa, 0xaa
  db 0xaa, 0xbb, 0xbb, 0xbb, 0x99, 0x99, 0x99, 0x88, 0x88, 0x11, 0x11
  db 0x33, 0x33, 0x22, 0x22, 0x66, 0x77, 0x55, 0x44, 0xcc, 0xdd, 0xff
;maskTableInit:
;  dw 0xc03f,0x30cf,0x0cf3,0x03fc
codeEndInit:

absolute 0

iters: resb itersX*itersY
squareTableSegment: resw 2
aTable:
  resw itersX
bTable:
  resw itersY
yTableLower:
  resw 102
yTableUpper:
  resw 102
itersXTable:
  resw itersY
;timer: dw 0
colourTable:
  resb 35
;maskTable:
;  resw 4
stackStart:
  resb 128
stackEnd:
codeEnd:
