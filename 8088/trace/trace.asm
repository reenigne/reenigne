  %include "../defaults_bin.asm"

FASTSAMPLING EQU 0     ; Set to one to sample at 14.318MHz. Default is 4.77MHz.
LENGTH       EQU 2048  ; Number of samples to capture.
REFRESH      EQU 18;0;19   ; Refresh period in cycles, or 0 to disable

  cli
  mov ax,cs
  mov ss,ax
  mov sp,0xfffe

%macro outputByte 0
%rep 8
  rcr dx,1
  sbb bx,bx
  mov bl,[cs:lut+1+bx]
  mov bh,0
  mov ax,[bx]
  times 10 nop
%endrep
%endmacro

;%macro outputbit 2
;  test %1, %2
;  jnz %%notZero
;  mov ax,[8]
;  jmp %%done
;%%notZero:
;  mov ax,[0x88]
;  jmp %%done
;%%done:
;  times 10 nop
;%endmacro

  mov cx,-1
flushLoop:
  loop flushLoop

  outputCharacter 6

waitAckLoop:
  mov al,0x0a  ; OCW3 - no bit 5 action, no poll command issued, act on bit 0,
  out 0x20,al  ;  read Interrupt Request Register
waitKeyboardLoop:
  ; Loop until the IRR bit 1 (IRQ 1) is high
  in al,0x20
  and al,2
  jz waitKeyboardLoop
  ; Read the keyboard byte and store it
  in al,0x60
  mov bl,al
  ; Acknowledge the previous byte
  in al,0x61
  or al,0x80
  out 0x61,al
  and al,0x7f
  out 0x61,al
  cmp bl,'K'
  jne waitAckLoop


%if FASTSAMPLING != 0
  mov cx,48
%else
  mov cx,16
%endif
loopTop:
  mov [cs:savedCX],cx
;  lockstep

;  sti
;  outputCharacter 'A'

  safeRefreshOff
  writePIT16 0, 2, 2
  writePIT16 0, 2, 100
  mov word[8*4],irq0
  mov [8*4+2],cs
  sti
  hlt
  hlt
  writePIT16 0, 2, 0

  mov ax,0xb800
  mov ds,ax
  mov ax,[0]
   lockstep 1

  mov ax,0x8000
  mov ds,ax
  mov ax,[0]      ; Trigger: Start of command load sequence
  times 10 nop
%if FASTSAMPLING != 0
  mov dl,48
%else
  mov dl,16
%endif
  mov cx,[cs:savedCX]
  sub dl,cl

  outputByte
  mov dx,LENGTH
  dec dx
  outputByte
  outputByte
  mov dx,18996-9; + 492*3   ;65534 ;
  outputByte
  outputByte

  mov al,0
  out 0x0c,al  ; Clear first/last flip-flop
  out 0x00,al  ; Output DMA offset low
  out 0x00,al  ; Output DMA offset high

  cli
  cld
  xor ax,ax
  mov ds,ax
  mov si,0x5555

  ; Delay for enough time to refresh 512 columns
  mov cx,256

  ; Increase refresh frequency to ensure all DRAM is refreshed before turning
  ; off refresh.
  mov al,TIMER1 | LSB | MODE2 | BINARY
  out 0x43,al
  mov al,2
  out 0x41,al  ; Timer 1 rate

  rep lodsw
 ; refreshOn
  mov al,TIMER1 | LSB | MODE2 | BINARY
  out 0x43,al
  mov al,REFRESH
  out 0x41,al  ; Timer 1 rate

  call testRoutine

  ; Delay for enough time to refresh 512 columns
  mov cx,256

  ; Increase refresh frequency to ensure all DRAM is refreshed before turning
  ; off refresh.
  mov al,TIMER1 | LSB | MODE2 | BINARY
  out 0x43,al
  mov al,2
  out 0x41,al  ; Timer 1 rate

  rep lodsw
  refreshOn

;  mov cx,3
;flushLoop3:
;  push cx
  mov cx,25*LENGTH
flushLoop2:
  loop flushLoop2
;  pop cx
;  loop flushLoop3

  mov cx,[cs:savedCX]
  loop loopTop2

  sti
  outputCharacter 7

  complete
loopTop2:
  jmp loopTop

irq0:
  push ax
  mov al,0x20
  out 0x20,al
  pop ax
  iret

savedCX: dw 0
lut: db 0x88,8

testRoutine:
  push ds
  xor bx,bx
  mov ds,bx

   mov word[3*4],testRoutineContinue
   mov [3*4+2],cs

  mov byte[0xfe70],0xcc ;0xcb
  mov byte[0xff6f],0xcc ;0xcb
  pop ds
  mov word[bx+2], 0xfff7
  mov cx,0xffff
  mov ax,0x21
  push ds
  push ax
  pop ax
  pop ds
  mov byte[bx], 0
;   mov word[bx], 0xffff

  mov al,0
  mul al
  xchg ax,cx
;  nop

;   mov ax,0xffff
;   mov bx,0xffff
;   mov cx,0xffff
;   mov dx,0xffff
;   mov si,0xffff
;   mov di,0xffff
;   mov sp,0xffff
;   mov bp,0xffff

  db 0xfe,0xe9
;  db 0xfe, 0xe8

testRoutineContinue:
  add sp,6

  ret



  mov dx,0x3d8
  xchg ax,bx
  out dx,ax
  xchg ax,cx
  out dx,ax
  xchg ax,bp
  out dx,ax
  xchg ax,si
  out dx,ax
  xchg ax,di
  out dx,ax
  xchg ax,bx
  out dx,ax
  xchg ax,cx
  out dx,ax
  xchg ax,bp
  out dx,ax
  xchg ax,si
  out dx,ax
  xchg ax,di
  out dx,ax
  xchg ax,bx
  out dx,ax
  xchg ax,cx
  out dx,ax
  xchg ax,bp
  out dx,ax
  xchg ax,si
  out dx,ax
  xchg ax,di
  out dx,ax
  ret


  mov ax,0x7000
  mov ds,ax
  mov es,ax
  mov bp,0x8080
  mov word[bp+2],ax
  mov word[bp+4],ax
  mov word[es:0x0302],emptyRoutine
  xor si,si
  xor bx,bx
  mov word[si],bx
  mov word[si+2],bx
  mov word[si+4],bx
  mov word[si+6],bx

  mov cx,4

  lodsb
  or al,2
ltop:
  mov ah,[bx]
  inc bx
  or ah,3
  mov es,[bp+2]
  mov di,ax
  call word[es:di]
  lodsb
  or al,2
  mov es,[bp+4]
  mov di,ax
  call word[es:di]
  loop ltop
emptyRoutine:
  ret



  mov ax,0xb800
  mov es,ax
  xor di,di
  mov ax,cs
  mov ds,ax

    movsw
    add si,bx
    movsw
    add si,bx
    movsw
    add si,bx
    movsw
    add si,bx
    movsw
    add si,bx
    movsw

    movsw
    movsw
    movsw
    movsw
    movsw

    movsw
    add si,bx
    movsb
    add si,bx
    movsw
    add si,bx
    movsb
    add si,bx
    movsw
    add si,bx
    movsb
    add si,bx
    movsw


    movsb
    add si,bx
    movsb
    add si,bx
    movsb
    add si,bx
    movsb
    add si,bx
    movsb
    add si,bx
    movsb
    add si,bx
    movsb
    add si,bx
    movsb
    add si,bx
    movsb
    add si,bx
    movsb
    add si,bx
    movsb
    add si,bx
    movsb
    add si,bx
    movsb
    add si,bx
    movsb
    add si,bx
    movsb
    add si,bx
    movsb
    add si,bx
    movsb
    add si,bx
    movsb
    add si,bx
    movsb
    add si,bx
    movsb
    add si,bx
    movsb
    add si,bx
    movsb
    add si,bx
    movsb
    add si,bx
    movsb
    add si,bx
    movsb
    add si,bx
    movsb
    add si,bx
    movsb
    add si,bx
    movsb
    add si,bx
    movsb
    add si,bx
    movsb
    add si,bx
    movsb
    add si,bx
    movsb
    add si,bx


   mov al, [si+0x1234]
   stosb
   mov al, [si+0x1234]
   stosb
   mov al, [si+0x1234]
   stosb
   mov al, [si+0x1234]
   stosb
   mov al, [si+0x1234]
   stosb
   mov al, [si+0x1234]
   stosb
   mov al, [si+0x1234]
   stosb
   mov al, [si+0x1234]
   stosb
   mov al, [si+0x1234]
   stosb
   mov al, [si+0x1234]
   stosb
   mov al, [si+0x1234]
   stosb
   mov al, [si+0x1234]
   stosb
   mov al, [si+0x1234]
   stosb
   mov al, [si+0x1234]
   stosb
   mov al, [si+0x1234]
   stosb
   mov al, [si+0x1234]
   stosb
   mov al, [si+0x1234]
   stosb
   mov al, [si+0x1234]
   stosb
   mov al, [si+0x1234]
   stosb
   mov al, [si+0x1234]
   stosb
   mov al, [si+0x1234]
   stosb
   mov al, [si+0x1234]
   stosb
   mov al, [si+0x1234]
   stosb
   mov al, [si+0x1234]
   stosb
   mov al, [si+0x1234]
   stosb
   mov al, [si+0x1234]
   stosb
   mov al, [si+0x1234]
   stosb
   mov al, [si+0x1234]
   stosb
   mov al, [si+0x1234]
   stosb
   mov al, [si+0x1234]
   stosb
   mov al, [si+0x1234]
   stosb
   mov al, [si+0x1234]
   stosb


   mov ah, [si+0x1234]
   mov al, [si+0x1234]
   and ax, bp
   add al, ah
   stosb
   inc di

   mov ah, [si+0x1234]
   mov al, [si+0x1234]
   and ax, bp
   add al, ah
   stosb
   inc di

   mov ah, [si+0x1234]
   mov al, [si+0x1234]
   and ax, bp
   add al, ah
   stosb
   inc di

   mov ah, [si+0x1234]
   mov al, [si+0x1234]
   and ax, bp
   add al, ah
   stosb
   inc di

   mov ah, [si+0x1234]
   mov al, [si+0x1234]
   and ax, bp
   add al, ah
   stosb
   inc di

   mov ah, [si+0x1234]
   mov al, [si+0x1234]
   and ax, bp
   add al, ah
   stosb
   inc di

   mov ah, [si+0x1234]
   mov al, [si+0x1234]
   and ax, bp
   add al, ah
   stosb
   inc di

   mov ah, [si+0x1234]
   mov al, [si+0x1234]
   and ax, bp
   add al, ah
   stosb
   inc di

   mov ah, [si+0x1234]
   mov al, [si+0x1234]
   and ax, bp
   add al, ah
   stosb
   inc di

   mov ah, [si+0x1234]
   mov al, [si+0x1234]
   and ax, bp
   add al, ah
   stosb
   inc di

   mov ah, [si+0x1234]
   mov al, [si+0x1234]
   and ax, bp
   add al, ah
   stosb
   inc di

   mov ah, [si+0x1234]
   mov al, [si+0x1234]
   and ax, bp
   add al, ah
   stosb
   inc di

   mov ah, [si+0x1234]
   mov al, [si+0x1234]
   and ax, bp
   add al, ah
   stosb
   inc di

   mov ah, [si+0x1234]
   mov al, [si+0x1234]
   and ax, bp
   add al, ah
   stosb
   inc di

   mov ah, [si+0x1234]
   mov al, [si+0x1234]
   and ax, bp
   add al, ah
   stosb
   inc di

   mov ah, [si+0x1234]
   mov al, [si+0x1234]
   and ax, bp
   add al, ah
   stosb
   inc di

   mov ah, [si+0x1234]
   mov al, [si+0x1234]
   and ax, bp
   add al, ah
   stosb
   inc di

   mov ah, [si+0x1234]
   mov al, [si+0x1234]
   and ax, bp
   add al, ah
   stosb
   inc di

   mov ah, [si+0x1234]
   mov al, [si+0x1234]
   and ax, bp
   add al, ah
   stosb
   inc di

   mov ah, [si+0x1234]
   mov al, [si+0x1234]
   and ax, bp
   add al, ah
   stosb
   inc di

   mov ah, [si+0x1234]
   mov al, [si+0x1234]
   and ax, bp
   add al, ah
   stosb
   inc di

   mov ah, [si+0x1234]
   mov al, [si+0x1234]
   and ax, bp
   add al, ah
   stosb
   inc di

   mov ah, [si+0x1234]
   mov al, [si+0x1234]
   and ax, bp
   add al, ah
   stosb
   inc di

   mov ah, [si+0x1234]
   mov al, [si+0x1234]
   and ax, bp
   add al, ah
   stosb
   inc di





  add cx,si
  add dx,bp
  mov al,ch
  mov bh,dh
  xlatb
  stosb

  add cx,si
  add dx,bp
  mov al,ch
  mov bh,dh
  xlatb
  stosb

  add cx,si
  add dx,bp
  mov al,ch
  mov bh,dh
  xlatb
  stosb

  add cx,si
  add dx,bp
  mov al,ch
  mov bh,dh
  xlatb
  stosb

  add cx,si
  add dx,bp
  mov al,ch
  mov bh,dh
  xlatb
  stosb

  add cx,si
  add dx,bp
  mov al,ch
  mov bh,dh
  xlatb
  stosb

  add cx,si
  add dx,bp
  mov al,ch
  mov bh,dh
  xlatb
  stosb

  add cx,si
  add dx,bp
  mov al,ch
  mov bh,dh
  xlatb
  stosb

  add cx,si
  add dx,bp
  mov al,ch
  mov bh,dh
  xlatb
  stosb

  add cx,si
  add dx,bp
  mov al,ch
  mov bh,dh
  xlatb
  stosb

  add cx,si
  add dx,bp
  mov al,ch
  mov bh,dh
  xlatb
  stosb

  add cx,si
  add dx,bp
  mov al,ch
  mov bh,dh
  xlatb
  stosb

  add cx,si
  add dx,bp
  mov al,ch
  mov bh,dh
  xlatb
  stosb

  add cx,si
  add dx,bp
  mov al,ch
  mov bh,dh
  xlatb
  stosb

  add cx,si
  add dx,bp
  mov al,ch
  mov bh,dh
  xlatb
  stosb

  add cx,si
  add dx,bp
  mov al,ch
  mov bh,dh
  xlatb
  stosb

  add cx,si
  add dx,bp
  mov al,ch
  mov bh,dh
  xlatb
  stosb

  add cx,si
  add dx,bp
  mov al,ch
  mov bh,dh
  xlatb
  stosb

  add cx,si
  add dx,bp
  mov al,ch
  mov bh,dh
  xlatb
  stosb

  add cx,si
  add dx,bp
  mov al,ch
  mov bh,dh
  xlatb
  stosb

  add cx,si
  add dx,bp
  mov al,ch
  mov bh,dh
  xlatb
  stosb

  add cx,si
  add dx,bp
  mov al,ch
  mov bh,dh
  xlatb
  stosb

  add cx,si
  add dx,bp
  mov al,ch
  mov bh,dh
  xlatb
  stosb

  add cx,si
  add dx,bp
  mov al,ch
  mov bh,dh
  xlatb
  stosb

  add cx,si
  add dx,bp
  mov al,ch
  mov bh,dh
  xlatb
  stosb

  add cx,si
  add dx,bp
  mov al,ch
  mov bh,dh
  xlatb
  stosb

  add cx,si
  add dx,bp
  mov al,ch
  mov bh,dh
  xlatb
  stosb

  add cx,si
  add dx,bp
  mov al,ch
  mov bh,dh
  xlatb
  stosb

  add cx,si
  add dx,bp
  mov al,ch
  mov bh,dh
  xlatb
  stosb

  add cx,si
  add dx,bp
  mov al,ch
  mov bh,dh
  xlatb
  stosb

  add cx,si
  add dx,bp
  mov al,ch
  mov bh,dh
  xlatb
  stosb

  add cx,si
  add dx,bp
  mov al,ch
  mov bh,dh
  xlatb
  stosb





  ret
