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
  mov ax,0xb800
  mov es,ax

  lodsw
  and [es:di],ah
  or [es:di],al
  inc di


  mov ax,es
  mov ds,ax
  push ax

  pop ax
  and [si],ah
  or [si],al
  inc si


  and byte[si],0x12
  or byte[si],0x34
  inc si


  and word[si],0x1234
  or word[si],0x5678


  mov ax,cs
  mov ds,ax

  lodsw
  and ax,[es:di]
  xchg ax,bx
  lodsw
  or ax,bx
  stosw

mov ax,[es:di]
and ax,1234h
or ax,5678h
stosw


  ret








  mov word[8*4],int8_isav0

  pushf
  push cs
  mov ax,after_ints
  push ax

  mov al,0x34
  out 0x43,al
  mov al,76
  out 0x40,al
  mov al,0
  out 0x40,al

  sti
  hlt

int8_isav0:                                           ; Starts 792                            479
  push ax
  push dx
  mov dx,0x3d4
;
;   mov al,11
;   mov dx,0x3d9
;   out dx,al
;
;  pop dx

  mov al,0xfe
  out 0x21,al

  push ds
  xor ax,ax
  mov ds,ax
  mov word[0x20],int8_isav1
;  pop ds

  mov al,0x20
  out 0x20,al
;  pop ax
  sti
  hlt                                                 ; ends 1042 - 250 cycles   ok           733  254 cycles

int8_isav1:                                           ; starts 1401                           783
;  push ax
;  push dx
;  mov dx,0x3d4
  mov ax,0x2102  ; Horizontal sync position early
  out dx,ax

;   mov al,12
;   mov dl,0xd9
;   out dx,al

;  pop dx

;  push ds
;  xor ax,ax
;  mov ds,ax
  mov word[0x20],int8_isav3
;  pop ds

  mov al,0x20
  out 0x20,al
;  pop ax
;  add sp,6
  sti
  hlt                                                 ; ends 1710 - 309 cycles                965  182 cycles

int8_isav2:                                           ; starts 783                           1087
;  push ax
;  push dx
;  mov dx,0x3d4
  mov ax,0x5a02  ; Horizontal sync position normal
  out dx,ax

;   mov al,13
;   mov dl,0xd9
;   out dx,al

;  pop dx

;  push ds
;  xor ax,ax
;  mov ds,ax
  mov word[0x20],int8_isav3
;  pop ds

  mov al,0x20
  out 0x20,al
;  pop ax
;  add sp,6
  sti
  hlt                                                 ; ends 1088 - 305 cycles               1268  181 cycles


  ; Final 3 - scanline 227
int8_isav3:                                           ; starts 1391                          1391
;  push ax
;  push dx

;   mov al,14
;   mov dl,0xd9
;   out dx,al

;  pop dx

;  push ds
;  xor ax,ax
;  mov ds,ax
  mov word[0x20],int8_isav4
;  pop ds

  mov al,0x20
  out 0x20,al
;  pop ax
;  add sp,6
  sti
  hlt                                                 ; ends 1662 - 271 cycles   ok          1549  158 cycles


  ; Final 4 - scanline 228
int8_isav4:                                           ; starts 783                           1695
;  push ax
;  push dx
;  mov dx,0x3d4
  mov ax,0x2102  ; Horizontal sync position early
  out dx,ax

;   mov al,15
;   mov dl,0xd9
;   out dx,al

;  pop dx

; mov ax,[cs:delayPCycles]
; add [cs:delayTotal],ax
; mov word[cs:delayPCycles],0
; mov ax,0 ;-1
; add ax,520*76
; out 0x40,al
; mov al,ah
; out 0x40,al

  mov al,(520*76) & 0xff
  out 0x40,al
  mov al,(520*76) >> 8
  out 0x40,al

;  push ds
;  xor ax,ax
;  mov ds,ax
  mov word[0x20],int8_isav5
;  pop ds

  mov al,0x20
  out 0x20,al
;  pop ax
;  add sp,6
  sti
  hlt                                                 ; ends 1249 - 466 cycles               1930  235 cycles


  ; Final 5 - scanline 229
int8_isav5:                                           ; starts 1391                          1999                1704
  add sp,4*6  ; 6 bytes for each of isav1-isav5. isav0 will be undone with iret
;  push ax
;  push dx
;  mov dx,0x3d4
  mov ax,0x5a02  ; Horizontal sync position normal
  out dx,ax

;   mov al,10
;   mov dl,0xd9
;   out dx,al

;  pop dx

  mov al,(76*2) & 0xff
  out 0x40,al
  mov al,(76*2) >> 8
  out 0x40,al

;  push ds
;  xor ax,ax
;  mov ds,ax
  mov word[0x20],int8_isav0
  pop ds
  pop dx

  mov al,0xfe ;[cs:originalIMR]
  out 0x21,al

;  add word[cs:timerCount],76*525
;  jnc doneInterrupt8
;  pop ax
;  jmp far [cs:originalInterrupt8]

doneInterrupt8:
  mov al,0x20
  out 0x20,al
  pop ax
  iret                                                ; ends 1816 - 425 cycles   ok


delayPCycles: dw 0
delayTotal: dw 0


after_ints:
  ret







  mov dx,0x3d9
  in al,dx
  test al,8
  jnz foo1
foo1:
  in al,dx
  test al,8
  jz foo2
foo2:
  mov dl,0xd9
  mov al,0x0f
  out dx,al
  ret



  mov ax,0xb800
  mov es,ax
  xor di,di
  mov cx,79

  lodsb
  stosb
  add di,cx
  stosb
  add di,cx
  lodsb
  stosb
  add di,cx
  stosb
  add di,cx
  movsb
  add di,cx
  lodsb
  stosb
  add di,cx
  stosb
  add di,cx
  movsb
  add di,cx
  lodsb
  stosb
  add di,cx
  stosb
  add di,cx
  movsb
  add di,cx
  lodsb
  stosb
  add di,cx
  stosb
  add di,cx
  lodsb
  stosb
  add di,cx
  stosb
  add di,cx
  movsb
  add di,cx
  lodsb
  stosb
  add di,cx
  stosb
  add di,cx
  movsb
  add di,cx
  lodsb
  stosb
  add di,cx
  stosb
  add di,cx
  movsb
  add di,cx
  lodsb
  stosb
  add di,cx
  stosb
  add di,cx
  movsb
  add di,cx

  ret








  mov di,[si]  ; x*x
  mov bp,[bx]  ; y*y
  lea ax,[di+bp] ; x*x+y*y
  cmp ax,ax
  jne $+2
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
  cmp ax,ax
  jne $+2
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
  cmp ax,ax
  jne $+2
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
  cmp ax,ax
  jne $+2
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
  cmp ax,ax
  jne $+2
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
  cmp ax,ax
  jne $+2
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
  cmp ax,ax
  jne $+2
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
  cmp ax,ax
  jne $+2
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
  cmp ax,ax
  jne $+2
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
  cmp ax,ax
  jne $+2
  dec cx
  mov bx,[si+bx] ; (x+y)*(x+y)
  sub bx,ax  ; 2*x*y
  add bx,dx  ; 2*x*y+b -> new y
  mov si,es
  add si,di
  sub si,bp  ; x*x-y*y+a -> new x

  ret

  mov al,0xff
  mov bl,0xff
  imul bl
  jo foo
foo:
  push ax
  pop ax

;  mov cx,1000
;self:
;  loop self

  ret


  ; Enable auto-EOI
  mov al,0x13  ; ICW4 needed, not cascaded, call address interval 8, edge triggered
  out 0x20,al  ; Set ICW1
  mov al,0x08  ; Interrupt vector address
  out 0x21,al  ; Set ICW2
  mov al,0x0f  ; 8086/8088 mode, auto-EOI, buffered mode/master, not special fully nested mode
  out 0x21,al  ; Set ICW4
  mov al,0xbc  ; Enable IRQs 0 (timer), 1 (keyboard) and 6 (floppy disk).
  out 0x21,al  ; Leave disabled 2 (EGA/VGA/slave 8259) 3 (COM2/COM4), 4 (COM1/COM3), 5 (hard drive, LPT2) and 7 (LPT1)

  xor ax,ax
  mov ds,ax
  mov word[8*4],testIRQ0

;  mov al,0x54
;  out 0x43,al
;  mov al,86-63
;  out 0x41,al
;  mov al,70
;  out 0x41,al

  mov al,0x14
  out 0x43,al
  mov al,70-63
  out 0x40,al
  mov al,70
  out 0x40,al
  sti

;  mov ax,0x0000
;  mul ah
;  wait
  hlt

  mov ax,0xffff ;0x0000
  mul ah
;  wait
  hlt

  mov ax,0x0101
  mul ah
;  wait
  hlt

  mov ax,0x0303
  mul ah
;  wait
  hlt

  mov ax,0x0707
  mul ah
;  wait
  hlt

  mov ax,0x0f0f
  mul ah
;  wait
  hlt


  mov al,0x34
  out 0x43,al
  mov al,0
  out 0x40,al
  out 0x40,al

  mov word[8*4],irq0
  sti

  ret


testIRQ0:
;  push ax
;  push bx
;  mov al,0x20
;  out 0x20,al
;  mov bx,sp
;  inc word[ss:bx+4]
;  mov ax,[ss:bx+4]
;  pop bx
;  pop ax
;  iret

;  mov bx,sp
;  inc byte[ss:bx]
  iret

