  %include "../defaults_bin.asm"
;  org 0x100
;  %include "../defaults_common.asm"

  ; time = k*(R + 2.2Kohm)*0.01uF = k*R*0.01uF + k*22us, k = 24.2/22 = 1.1
  ; 0 to 100K is 24.2us to 1124.2us = 28 to 1341 timer values (0.36 to 17.64 scanlines)

  ; 21 cycles = 4.4 microseconds per iteration
  ; 500 iterations = 2.2 milliseconds = 220kOhm
  ; Max is 412 across, 371 down (181.28kOhm horizontal, 163.24kOhm vertical)
ITERS equ 640


  mov ax,4
  int 0x10
  mov dx,0x3d8
  mov al,0x0a
  out dx,al
  inc dx
  mov al,0x30
  out dx,al


mainLoop:
  safeRefreshOff

  mov ax,cs
  mov ds,ax
  mov es,ax
  mov di,joydump
  mov dx,0x0201
  out dx,al
%rep ITERS
  in al,dx
  stosb
%endrep

  safeRefreshOn
  sti

  mov ax,cs
  mov ds,ax
;  mov ax,0xb800
;  mov es,ax
  mov si,joydump
;  mov di,0
;  mov cx,ITERS/2
  cld
;  rep movsw

  mov cx,ITERS
  xor bx,bx
  xor dx,dx
findLoopTop:
  lodsb
  test al,1
  jz gotBit0
  mov bx,cx
gotBit0:
  test al,2
  jz gotBit1
  mov dx,cx
gotBit1:
  loop findLoopTop
  sub bx,ITERS
  sub dx,ITERS
  neg bx
  neg dx
;  cmp bx,[maxX]
;  jle noNewMaxBX
;  mov [maxX],bx
;  push dx
;  push bx
;  mov ax,[maxX]
;  outputHex
;  mov ax,[maxY]
;  outputHex
;  outputCharacter 10
;  pop bx
;  pop dx
;noNewMaxBX:
;  cmp dx,[maxY]
;  jle noNewMaxDX
;  mov [maxY],dx
;  push dx
;  push bx
;  mov ax,[maxX]
;  outputHex
;  mov ax,[maxY]
;  outputHex
;  outputCharacter 10
;  pop bx
;  pop dx
;noNewMaxDX:
  shr bx,1
  shr dx,1
  mov [joydata],bx
  mov [joydata+2],dx



;  jmp mainLoop





%macro readJoy 1
  mov dx,0x201
  mov cx,750
  cli
;  mov al,TIMER0 | BOTH | MODE2 | BINARY
;  out 0x43,al
;  mov al,0x00
;  out 0x40,al
;  out 0x40,al

  out dx,al
%%joyloop:         ; Loop takes 39.25 cycles at refresh 18, so resolution is slightly better than 10 PIT cycles
  in al,dx
  test al,1<<%1
  loopnz %%joyloop

%%joyloop1:
  in al,dx
  and al,3
  cmp al,0
  jne %%joyloop1

;  in al,0x40
;  mov cl,al
;  in al,0x40
;  mov ch,al
  neg cx
  add cx,750
;  shr cx,1
  mov [joydata+%1*2],cx

%endmacro

repeatloop:
;  readJoy 0
;  readJoy 1
;  readJoy 2
;  readJoy 3

;  in al,dx
;  not al
;  mov [es:0],al
;  mov cl,4
;  shr al,cl
;  or al,0x30
;  mov dx,0x3d9
;  out dx,al

  mov ax,[joydata+2]
;  xor dx,dx
;  mov cx,1
;  div cx
  mov bx,ax
  cmp bx,0
  jge noY0Low
  mov bx,0
noY0Low:
  cmp bx,200
  jl noY0High
  mov bx,199
noY0High:
  add bx,bx
  mov di,[bx+yTable]
  mov ax,[joydata]
;  xor dx,dx
;  mov cx,1 ;5
;  div cx
  mov bx,ax
  cmp bx,0
  jge noX0Low
  mov bx,0
noX0Low:
  cmp bx,320
  jl noX0High
  mov bx,319
noX0High:
  mov al,[bx+pTable0]
  add bx,bx
  add di,[bx+xTable]
  push di
  mov di,[lastOffset]
  mov ah,[lastMask]
  not ah
  mov bx,0xb800
  mov es,bx
  and [es:di],ah
  pop di
  mov [lastOffset],di
  mov [lastMask],al
  mov ah,al
  add al,ah
  add al,ah
  or [es:di],al
  shr al,1


;  mov ax,[joydata+6]
;;  xor dx,dx
;;  mov cx,13 ;7
;;  div cx
;  mov bx,ax
;;  cmp bx,0
;;  jge noY1Low
;;  mov bx,0
;;noY1Low:
;;  cmp bx,200
;;  jl noY1High
;;  mov bx,199
;;noY1High:
;  add bx,bx
;  mov di,[bx+yTable]
;  mov ax,[joydata+4]
;;  xor dx,dx
;;  mov cx,8 ;5
;;  div cx
;  mov bx,ax
;;  cmp bx,0
;;  jge noX1Low
;;  mov bx,0
;;noX1Low:
;;  cmp bx,320
;;  jl noX1High
;;  mov bx,319
;;noX1High:
;  mov al,[bx+pTable1]
;  add bx,bx
;  add di,[bx+xTable]
;  or [es:di],al

;  mov ax,[joydata]
;  outputHex
;  mov ax,[joydata+2]
;  outputHex
;  mov ax,[joydata+4]
;  outputHex
;  mov ax,[joydata+6]
;  outputHex
;  outputCharacter 10

;042C 0114 09C0 09C0       1068 276    2496 2496

;  jmp repeatloop
  jmp mainLoop


joydata:
  times 4 dw 0

yTable:
%assign i 0
%rep 100
  dw i*80,0x2000+i*80
  %assign i i+1
%endrep

xTable:
%assign i 0
%rep 80
  dw i,i,i,i
  %assign i i+1
%endrep

pTable0:
%assign i 0
%rep 80
  db 0x40,0x10,0x04,0x01
  %assign i i+1
%endrep

pTable1:
%assign i 0
%rep 80
  db 0x80,0x20,0x08,0x02
  %assign i i+1
%endrep

lastOffset: dw 0
lastMask: db 0
maxX: dw 0
maxY: dw 0

joydump:
