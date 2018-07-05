cpu 8086
%ifdef XTSERVER
org 0
;TIMING equ 1
TIMING equ 0
%else
org 0x100
TIMING equ 0
%endif


MULTIPLIER equ 0x600
maxX equ 320
maxY equ 101
initialShift equ 5
initialGrid equ (1 << initialShift)
itersX equ ((maxX + initialGrid - 1)/initialGrid)*initialGrid + 1
itersY equ ((maxY + initialGrid - 1)/initialGrid)*initialGrid + 1

  ; Set video mode

  mov ax,6 ;4
  int 0x10
  mov dx,0x3d8
  mov al,0x1a ;0x0a
  out dx,al
  mov al,0x0f ;0x30
  inc dx
  out dx,al
  mov dl,0xd4
  mov ax,0x6506
  out dx,ax

%if TIMING != 0
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
%endif

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

  mov ax,ds
  add ax,(stackEnd - 0x1c40) >> 4
  cli
  mov ss,ax
  mov sp,0x1c40
  sti

  ; Create square table in ES

  push ds
  mov ax,cs
  mov ds,ax
  mov cx,0x4000
  mov bx,MULTIPLIER/8
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
  neg di
  mov [es:di],ax
  neg di
  stosw
  loop squareLoop
  pop ds

  ; Initialize iters table
  mov ax,ds
  mov es,ax
  xor di,di
  mov ax,-1
  mov cx,(itersX*itersY + 1) >> 1
  rep stosw

  ; Create table of "a" values

  mov cx,itersX
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

  ; Create table of "b" values

  mov cx,itersY
  xor bx,bx
  mov si,432  ; 3456
  mov bp,25   ; 200
bLoop:
  mov ax,bx
  xor dx,dx
  div bp
  and ax,0xfffe
  stosw
  add bx,si
  loop bLoop

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
;  mov cx,itersY - maxY
;  mov ax,0x2000+8000
;  rep stosw
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
;  mov cx,itersY - maxY
;  mov ax,0x2000+8000
;  rep stosw
  stosw

  ; Create table of itersX multiples

  xor ax,ax
  mov bx,itersX
  mov cx,itersY
itersXTableLoop:
  stosw
  add ax,bx
  loop itersXTableLoop

  ; Compute the initial coarse grid

  xor si,si
  mov cx,(itersY >> initialShift) + 1
coarseYLoop:
  push cx

  xor bx,bx
  mov cx,(itersX >> initialShift) + 1
coarseXLoop:
  push cx
  call mandelIters
  add bx,initialGrid
  pop cx
  loop coarseXLoop

  add si,initialGrid << 1
  pop cx
  loop coarseYLoop

  ; Recursively refine

  xor si,si
  mov cx,itersY >> initialShift
refineYLoop:
  push cx

  xor bx,bx
  mov cx,itersX >> initialShift
refineXLoop:
  push cx
  call subdivide5
  add bx,initialGrid
  pop cx
  loop refineXLoop

  add si,initialGrid << 1
  pop cx
  loop refineYLoop


  ; Clean up

;  xor ax,ax
;  mov ds,ax
;  mov ax,[0x6c]
;  sub ax,[cs:timer]

%if TIMING != 0
  mov ax,[cs:timer]
  int 0x63 ; outputHex
;  int 0x60 ;captureScreen
  int 0x67 ; complete
%endif

  mov ah,0
  int 0x16
  mov ax,3
  int 0x10
  mov ax,0x4c00
  int 0x21

%if TIMING != 0
timer: dw 0

interrupt8:
  push ax
  mov al,0x20
  out 0x20,al
  inc word[cs:timer]
  pop ax
  iret
%endif

savedSP: dw 0
alreadyDone:
  ret
mandelIters:
  mov di,[si+itersXTable]
  cmp byte[di+bx],0xff
  jne alreadyDone

  push di
  push bx
  push si

  add bx,bx
  mov bx,[bx+aTable]
  mov es,bx
  mov si,[si+bTable]
  mov dx,si
  push ds
  mov ds,[squareTableSegment]
  mov cx,32
  mov [cs:savedSP],sp
  mov sp,0x1c00

;  mov bp,[si]             ; y^2
;  cmp bx,-MULTIPLIER*3/4
;  jge .right
;  mov di,[bx+MULTIPLIER]  ; (x+1)^2
;  add di,bp               ; (x+1)^2 + y^2
;  cmp di,MULTIPLIER/16    ; (x+1)^2 + y^2 > MULTIPLIER/16
;  jg .notLake
;  mov cl,33
;  jmp escaped
;.right:
;
;  cmp si,998  ; MULTIPLIER*sqrt(3)*3/8
;  jg .rightNotLake
;  add bp,[bx]             ; c2 = x^2 + y^2
;  mov di,bp               ; c2
;  add di,di               ; 2*c2
;  add di,di               ; 4*c2
;  add di,di               ; 8*c2
;  sub di,3*MULTIPLIER     ; d = 8*c2 - 3*MULTIPLIER
;  mov ax,[ds:bp+di]          ; (c2 + d)^2
;  neg di
;  sub ax,[ds:bp+di]          ; e = (c2 + d)^2 - (c2 - d)^2 = 4*c2*d
;  mov bp,es               ; a
;  add bp,bp
;  add bp,bp               ; 4*a
;  add ax,bp               ; e + 4*a
;  cmp ax,3*MULTIPLIER/8   ; e + 4*a > 3*MULTIPLIER/8
;  jg .rightNotLake
;  mov cl,34
;  jmp escaped

.rightNotLake:
  mov bp,[si]  ; y*y
.notLake:
  mov di,[bx]  ; x*x
  lea ax,[di+bp] ; x*x+y*y
  cmp ax,sp
  jae escaped5

%macro iterate 0
  dec cx
  mov si,[si+bx] ; (x+y)*(x+y)
  sub si,ax  ; 2*x*y
  add si,dx  ; 2*x*y+b -> new y
  mov bx,es
  add bx,di
  sub bx,bp  ; x*x-y*y+a -> new x
  mov di,[bx]  ; x*x
  mov bp,[si]  ; y*y
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
  mov sp,[cs:savedSP]
  pop ds
  pop si
  pop bx
  pop di
  mov [bx+di],cl
  ret


; all the subdivide routines:
; assume DS points to iters array
; assume SI is yp*2
; assume BX is xp

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
  mov dx,0xb800
  mov es,dx
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

   sub bx,8
   call subdivide3
   add bx,8
   call subdivide3
   sub bx,8

;  call subdivide3    ;  (8,8)
;  sub bx,8
;  call subdivide3    ;  (0,8)

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
  mov dx,0xb800
  mov es,dx
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
  add di,[si+yTableUpper+30]
  %rep 7
    times 2 stosw
    add di,bp
    times 2 stosw
    add di,dx
  %endrep
  times 2 stosw
  add di,bp
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
  ja .noLowerHalf
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
  mov ah,[bx+4*itersX]
  cmp ah,0xff
  je .doFill
  cmp ah,al
  jne doSubdivide3
.doFill:

  ; Fill square with colour

  mov bx,colourTable
  xlatb
  mov bx,di
  shr di,1
  shr di,1
  mov cx,di
  add di,[si+yTableLower]
  mov ah,al
  mov dx,0xb800
  mov es,dx
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
  add di,[si+yTableUpper+14]
  %rep 3
    stosw
    add di,bp
    stosw
    add di,dx
  %endrep
  stosw
  add di,bp
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
  jae .noLowerHalf
  add si,8
  call mandelIters   ; (2,4)
  sub si,4
  dec bx
  dec bx
  call subdivide1L   ;  (0,2)
  inc bx
  inc bx
  call subdivide1R   ;  (2,2)
  sub si,4
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
  mov dx,0xb800
  mov es,dx
  mov dx,0x2000-1
  mov bp,80-0x2000-1

  stosb
  cmp si,200
  jae .plotUpper

  add di,dx
  stosb
  add di,bp
  stosb
  add di,dx
  stosb

  mov di,cx
  add di,[si+yTableUpper+6]
  stosb
  add di,bp
  stosb
  add di,dx
  stosb
  add di,bp
  stosb
  ret

.plotUpper:
  add di,-8000-1
  stosb
  ret


doSubdivide1L:
  inc bx
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
  add di,[si+itersXTable]
  mov al,[di]
  cmp al,[di+2]
  jne doSubdivide1L
  cmp al,[di+2*itersX]
  jne doSubdivide1L
  cmp al,[di+2*(itersX+1)]
  jne doSubdivide1L

  ; Don't fill square with colour here - we'll do it in subdivide1R

  mov [di+1],al
  mov ah,al
  mov [di+itersX],ax
  ret


doSubdivide1R:
  push bx
  push di
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
  pop di
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
  mov ah,al
  mov [bx+itersX],ax

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
  mov dx,0xb800
  mov es,dx
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
  stosb

  mov bx,cx
  mov si,bp
  ret





colourTableInit:
;  db 0x00,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa,0xff,0x55,0xaa
;  db 0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff,0x11,0x22
  db 0x00, 0xff, 0xff, 0xff, 0xff, 0xee, 0xee, 0xee, 0xee, 0xaa, 0xaa
  db 0xaa, 0xbb, 0xbb, 0xbb, 0x99, 0x99, 0x99, 0x88, 0x88, 0x11, 0x11
  db 0x33, 0x33, 0x22, 0x22, 0x66, 0x77, 0x55, 0x44, 0xcc, 0xdd, 0xff, 0x00, 0x00
;maskTableInit:
;  dw 0xc03f,0x30cf,0x0cf3,0x03fc
codeEndInit:

absolute 0

iters:
  resb itersX*itersY
  alignb 2
aTable:
  resw itersX
bTable:
  resw itersY
yTableLower:
  resw 102 ;itersY
yTableUpper:
  resw 102 ;itersY
itersXTable:
  resw itersY
colourTable:
  resb 35
;maskTable:
;  resw 4
squareTableSegment:
  resw 1
stackStart:
  resb 128
stackEnd:
codeEnd:
