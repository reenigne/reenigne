cpu p4
org 0x100

start:
;  times 20 nop

  mov ax,0x13
  int 0x10

  mov cx,0x9000  ; 0x9000
  mov es,cx
;  xor di,di
;  mov ch,0x80
  mov ax,0xb4b4
  rep stosw

  mov si,start
  mov cl,1
xfLoopTop:
  mov ch,1
yfLoopTop:
  lodsw
  push si
  mov si,codeEnd
  mov bx,ax

  mov al,cl
  mul ch
  shl ax,2
  mov [si+2],ax
  mov [si+4],bl
  fild word[si+4]
  fild word[si+2]
  fdivp st1,st0

  xor dx,dx
xyLoopTop:
  mov al,dl
  mul cl
  mov bp,ax
  mov al,dh
  mul ch
  add bp,ax
  add bp,bx
  mov word[si],bp
  fild word[si]
  fmul dword[si - 8] ;tau256] ;si - 4]
  fsin
  fmul st1
  fistp word[si]
  mov al,[si]
  add [es:di],al
  inc dx
  inc di
  test dx,dx
  jnz xyLoopTop
  pop si
  inc ch
  cmp ch,12
  jl yfLoopTop
  inc cl
  cmp cl,12
  jl xfLoopTop

;  mov bp,si
  mov bp,0xff80
  mov ax,0x9000  ; 0x9000
  mov ds,ax
  mov ah,0xa0
  mov es,ax
;  xor cx,cx      ; xp
tLoopTop:
  xor di,di      ; x
xLoopTop:
  mov bx,200*320       ; maxY
  mov ax,di
  add ax,cx
  mov [bp],ax
  fild word[bp]
  fmul dword[cs:tau1280] ;bp - 2]
  fsincos
  mov dx,1       ; r
rLoopTop:
  mov [bp],dx
  push dx
  fild word[bp]
  fld st0
  fmul st0,st2
  fistp word[bp]
  fmul st0,st2
  fistp word[bp+1]
  add [bp],cl
  mov si,[bp]
  mov al,[si]              ; 0-ff
  mov ah,0xff              ; -256 to -1
  imul ax,ax,25            ; -6400 to -25
  push bx
  mov bx,dx
  cwd
;  div dl                  ; -6400 to 0
  idiv bx
  pop bx
  sub ax,20
  neg ax
  cmp ax,200
  jb positive
  mov ax,199
positive:
  imul dx,ax,320
  mov al,[si+0x7f]

fill:
  cmp dx,bx
  jae noFill
  mov [es:bx+di],al
  sub bx,320
  jmp fill
noFill:

  pop dx
  inc dx
;  cmp dl,250
;  jb rLoopTop
  cmp dx,100
  jl rLoopTop
sky:
  cmp bx,200*320
  ja doneSky
  mov byte[es:bx+di],0
  sub bx,320
  jmp sky
doneSky:

  inc di
  cmp di,320
  jl xLoopTop
  inc cx
  jmp tLoopTop
;  loop tLoopTop

;align 4
tau256:
  dd 0.02454369260617025967548940143187
tau1280:
  dd 0.00490873852123405193509788028637
codeEnd:


;times 20 nop
