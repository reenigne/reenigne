org 0
cpu 8086

%macro waitForDisplayEnable 0
  %%waitForDisplayEnable
    in al,dx                       ; 1 1 2
    test al,1                      ; 2 0 2
    jnz %%waitForDisplayEnable     ; 2 0 2
%endmacro

%macro waitForDisplayDisable 0
  %%waitForDisplayDisable
    in al,dx                       ; 1 1 2
    test al,1                      ; 2 0 2
    jz %%waitForDisplayDisable     ; 2 0 2
%endmacro

%macro waitForVerticalSync 0
  %%waitForVerticalSync
    in al,dx
    test al,8
    jz %%waitForVerticalSync
%endmacro

%macro waitForNoVerticalSync 0
  %%waitForNoVerticalSync
    in al,dx
    test al,8
    jnz %%waitForNoVerticalSync
%endmacro


  mov ax,cs
  mov ds,ax

  ; Set up video mode
  mov ax,6
  int 0x10
  mov dx,0x03d8
  mov al,0x1e
  out dx,al
  inc dx
  mov al,0x0f
  out dx,al

  ; Initialize video memory with appropriate patterns
  mov ax,0xb800
  mov es,ax
  cld
  mov di,0x0000

  mov cx, 200/6
sixLinesLoop:
  push cx

  mov si,screenData
  mov cx,40
  rep movsw
  add di,0x2000-80

  mov cx,40
  rep movsw
  sub di,0x2000

  mov cx,40
  rep movsw
  add di,0x2000-80

  mov si,screenData

  mov cx,40
  rep movsw
  sub di,0x2000

  mov cx,40
  rep movsw
  add di,0x2000-80

  mov cx,40
  rep movsw
  sub di,0x2000

  pop cx
  loop sixLinesLoop



  ; Initialize interrupt vector
  mov ax,0
  mov ds,ax
  mov word[0x20],interrupt8_1
  mov [0x22],cs

  ; Enable IRQ0
  mov al,0xfe  ; Enable IRQ 0 (timer), disable others
  out 0x21,al

  ; Wait for vertical sync
  mov dx,0x03da
  waitForVerticalSync
  waitForNoVerticalSync

  ; Turn off refresh
  mov al,0x60  ; Timer 1, write LSB, mode 0, binary
  out 0x43,al
  mov al,0x01  ; Count = 0x0001 so we'll stop almost immediately
  out 0x41,al

  ; Set up second argument to MUL
  mov cl,1

  ; Set "stosb" destination to non-video memory for first 16 lines
  mov di,0x3fff
  mov ax,0x8000
  mov es,ax

  ; We want to start the timer interrupt towards the left of the screen
  waitForDisplayEnable

  nop
  nop
  nop
  nop
  nop
  nop

  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop

  times 76 nop

  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop


  ; Use IRQ0 to go into lockstep with timer 0
  mov al,0x34  ; Timer 0, write LSB+MSB, mode 2, binary
  out 0x43,al
;  mov al,76    ; 76 tcycles == 1 scanline
  mov al,76*2
  out 0x40,al
  mov al,0
  out 0x40,al
  sti

  ; Put palette register address in DX
  mov dx,0x03d9

  hlt

%macro interrupt8 0
    mov al,0x20
    out 0x20,al
    pop bx
    pop bx
    mov word[0x20],%%nextInterrupt8
    popf
    hlt
  %%nextInterrupt8:
%endmacro

%macro testCode 0
    mov al,0
    out dx,al
    jmp $+2
    stosb
    mov al,0x01
    mul cl
    stosb
    mov al,0x7f
    mul cl
    stosb
    mov al,0x0f
    out dx,al
    mov di,0x3fff
%endmacro

; Reduces number of possibilities to 2
;    jmp $+2
;    stosb
;    mov al,0x01  ; Or 0x3f
;    mul cl
;    stosb


interrupt8_1:
  interrupt8

  mov al,0x00
  mul cl
  testCode
  interrupt8

  interrupt8

  interrupt8

  mov al,0x01
  mul cl
  testCode
  interrupt8

  interrupt8

  interrupt8

  mov al,0x03
  mul cl
  testCode
  interrupt8

  interrupt8

  interrupt8

  mov al,0x07
  mul cl
  testCode
  interrupt8

  interrupt8

  interrupt8

  mov al,0x0f
  mul cl
  testCode
  interrupt8

  interrupt8

  interrupt8

  mov al,0x1f
  mul cl
  testCode
  interrupt8

  interrupt8

  interrupt8

  mov al,0x3f
  mul cl
  testCode
  interrupt8

  interrupt8

  interrupt8

  mov al,0x7f
  mul cl
  testCode
  interrupt8

  interrupt8

  interrupt8

  mov al,0xff
  mul cl
  testCode
  interrupt8

  interrupt8

  interrupt8

  mov al,0x7f
  mul cl
  nop
  testCode
  interrupt8

  interrupt8

  interrupt8

  mov al,0xff
  mul cl
  nop
  testCode
  interrupt8

  interrupt8

  interrupt8

  mov al,0x1f
  mul cl
  nop
  nop
  testCode
  interrupt8

  interrupt8

  interrupt8

  mov al,0x3f
  mul cl
  nop
  nop
  testCode
  interrupt8

  interrupt8

  interrupt8

  mov al,0x1f
  mul cl
  nop
  nop
  nop
  testCode
  interrupt8

  interrupt8

  interrupt8

  mov al,0x3f
  mul cl
  nop
  nop
  nop
  testCode
  interrupt8

  interrupt8

  interrupt8

  mov al,0x07
  mul cl
  nop
  nop
  nop
  nop
  testCode
  mov ax,0xb800
  mov es,ax
  interrupt8

  interrupt8

  interrupt8


  mov al,0x00
  mul cl
  testCode
  interrupt8

  interrupt8

  interrupt8

  mov al,0x01
  mul cl
  testCode
  interrupt8

  interrupt8

  interrupt8

  mov al,0x03
  mul cl
  testCode
  interrupt8

  interrupt8

  interrupt8

  mov al,0x07
  mul cl
  testCode
  interrupt8

  interrupt8

  interrupt8

  mov al,0x0f
  mul cl
  testCode
  interrupt8

  interrupt8

  interrupt8

  mov al,0x1f
  mul cl
  testCode
  interrupt8

  interrupt8

  interrupt8

  mov al,0x3f
  mul cl
  testCode
  interrupt8

  interrupt8

  interrupt8

  mov al,0x7f
  mul cl
  testCode
  interrupt8

  interrupt8

  interrupt8

  mov al,0xff
  mul cl
  testCode
  interrupt8

  interrupt8

  interrupt8

  mov al,0x7f
  mul cl
  nop
  testCode
  interrupt8

  interrupt8

  interrupt8

  mov al,0xff
  mul cl
  nop
  testCode
  interrupt8

  interrupt8

  interrupt8

  mov al,0x1f
  mul cl
  nop
  nop
  testCode
  interrupt8

  interrupt8

  interrupt8

  mov al,0x3f
  mul cl
  nop
  nop
  testCode
  interrupt8

  interrupt8

  interrupt8

  mov al,0x1f
  mul cl
  nop
  nop
  nop
  testCode
  interrupt8

  interrupt8

  interrupt8

  mov al,0x3f
  mul cl
  nop
  nop
  nop
  testCode
  interrupt8

  interrupt8

  interrupt8

  mov al,0x07
  mul cl
  nop
  nop
  nop
  nop
  testCode
  mov ax,0x8000
  mov es,ax
  interrupt8

  interrupt8

;%rep 262-(3*2*16+1)
%rep 131-(3*2*16+1)
  interrupt8
%endrep

  mov al,0x20
  out 0x20,al
  pop bx
  pop bx
  mov word[0x20],interrupt8_1
  popf
  hlt



screenData:
  times 26 db 10010010b,01001001b,00100100b
  db 10010010b,01001001b

  times 40 db 10000000b,00000000b

  times 80 db 11111111b


