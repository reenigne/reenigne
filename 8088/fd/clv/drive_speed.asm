  org 0
  cpu 8086

; Want to be able to deal with speeds from 180rpm to 330rpm
; =182ms to 333ms = 3.3 to 6.06 timer ticks

  mov al,0x34
  out 0x43,al
  mov al,0
  out 0x40,al
  out 0x40,al
  xor ax,ax
  mov es,ax
  mov di,0x20
  cli
  mov ax,irq0
  stosw
  mov ax,cs
  stosw
  sti

  mov ax,0
  int 0x10

  mov ax,cs
  mov ds,ax
  mov es,ax

  mov al,0xb4
  out 0x43,al
  mov al,0
  out 0x42,al
  out 0x42,al

  in al,0x61
  and al,0xfc
  or al,1
  out 0x61,al



sectorReadLoop:

  mov ax,cs
  mov es,ax
  mov ax,0x0201
  mov cx,0x0001
  mov dx,0x0000
  mov bx,sectorBuffer
  int 0x13

;  mov al,0x80
;  out 0x43,al
;  in al,0x42
;  mov ah,al
;  in al,0x42
;  xchg ah,al

  mov al,0x00
  out 0x43,al
  in al,0x40
  mov ah,al
  in al,0x40
  xchg ah,al

  mov dx,[lastTime]
  mov [lastTime],ax
  sub dx,ax

  mov si,[timerHigh]
  mov ax,[lastTime+2]
  mov [lastTime+2],si
  sbb si,ax

  shr si,1
  rcr dx,1
  shr si,1
  rcr dx,1
  shr si,1
  rcr dx,1
  mov cx,dx

  mov bp,0xb800
  mov es,bp
  xor di,di

  mov dx,0x3556  ; 0x3556e1dc = 894886364 ~= 6000*157500000/11/12/8
  mov ax,0xe1dc
  div cx

  mov bx,ax
  xor dx,dx
  mov cx,10000
  div cx
  add al,'0'
  mov ah,7
  stosw

  mov ax,dx
  xor dx,dx
  mov cx,1000
  div cx
  add al,'0'
  mov ah,7
  stosw

  mov ax,dx
  xor dx,dx
  mov cx,100
  div cx
  add al,'0'
  mov ah,7
  stosw

  mov al,'.'
  stosw

  mov ax,dx
  xor dx,dx
  mov cx,10
  div cx
  add al,'0'
  mov ah,7
  stosw

  mov al,dl
  add al,'0'
  stosw



;  mov ah,7
;  mov bx,hexDigit
;  mov cl,4
;
;  xchg dx,si
;
;  mov al,dh
;  shr al,cl
;  xlatb
;  stosw
;  mov al,dh
;  and al,0x0f
;  xlatb
;  stosw
;  mov al,dl
;  shr al,cl
;  xlatb
;  stosw
;  mov al,dl
;  and al,0x0f
;  xlatb
;  stosw
;
;  mov dx,si
;
;  mov al,dh
;  shr al,cl
;  xlatb
;  stosw
;  mov al,dh
;  and al,0x0f
;  xlatb
;  stosw
;  mov al,dl
;  shr al,cl
;  xlatb
;  stosw
;  mov al,dl
;  and al,0x0f
;  xlatb
;  stosw

  jmp sectorReadLoop

;hexDigit: db "0123456789ABCDEF"
lastTime: dw 0,0
timerHigh: dw 0

irq0:
  push ax
  inc word[cs: timerHigh]
  mov al,0x20
  out 0x20,al
  pop ax
  iret

divideOverflow:
  iret

sectorBuffer:

