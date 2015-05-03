  org 0x100
  %include "../../defaults_common.asm"

  mov ax,0
  mov ds,ax
  mov ax,[8*4]
  mov [cs:oldInterrupt8],ax
  mov ax,[8*4+2]
  mov [cs:oldInterrupt8+2],ax
  mov ax,[9*4]
  mov [cs:oldInterrupt9],ax
  mov ax,[9*4+2]
  mov [cs:oldInterrupt9+2],ax
  cli
  mov word[9*4],interrupt9
  mov [9*4+2],cs


%macro delay 1
  %if %1 == 78
    mov al,0x00
    mul cl
  %endif
  %if %1 == 79
    mov al,0x01
    mul cl
  %endif
  %if %1 == 80
    mov al,0x03
    mul cl
  %endif
  %if %1 == 81
    mov al,0x07
    mul cl
  %endif
  %if %1 == 82
    mov al,0x0f
    mul cl
  %endif
  %if %1 == 83
    mov al,0x1f
    mul cl
  %endif
  %if %1 == 84
    mov al,0x3f
    mul cl
  %endif
  %if %1 == 85
    mov al,0x7f
    mul cl
  %endif
  %if %1 == 86
    mov al,0xff
    mul cl
  %endif
  %if %1 == 87
    mov al,0x7f
    mul cl
    nop
  %endif
  %if %1 == 88
    mov al,0xff
    mul cl
    nop
  %endif
  %if %1 == 89
    mov al,0x1f
    mul cl
    nop
    nop
  %endif
  %if %1 == 90
    mov al,0x3f
    mul cl
    nop
    nop
  %endif
  %if %1 == 91
    mov al,0x1f
    mul cl
    nop
    nop
    nop
  %endif
  %if %1 == 92
    mov al,0x3f
    mul cl
    nop
    nop
    nop
  %endif
  %if %1 == 93
    mov al,0x07
    mul cl
    nop
    nop
    nop
    nop
  %endif
%endmacro

%macro black 0
    mov al,0      ;  8
    out dx,al     ;  8
%endmacro

%macro white 0
    mov al,0x0f   ;  8
    out dx,al     ;  8
%endmacro



  cli

  ; Initialize video memory with appropriate patterns
  mov ax,cs
  mov ds,ax
  xor di,di
  mov ax,0xb800
  mov es,ax

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


  mov cx,256
  xor ax,ax
  mov ds,ax
  mov si,ax
  cld
  cli

  ; Increase refresh frequency to ensure all DRAM is refreshed before turning
  ; off refresh.
  mov al,TIMER1 | LSB | MODE2 | BINARY
  out 0x43,al
  mov al,2
  out 0x41,al  ; Timer 1 rate

  ; Delay for enough time to refresh 512 columns
  rep lodsw

  ; We now have about 1.5ms during which refresh can be off
  refreshOff

  ; CRTC should be in lockstep too by now. Set it up for normal video

  initCGA ONE_BPP | BW

  mov dx,0x3da
  waitForVerticalSync
  waitForDisplayEnable

  times 76*4 - (16) nop    ; + 38 + 76


  writePIT16 0, 2, 76*6

  xor ax,ax
  mov ds,ax
  mov word[8*4],interrupt8
  mov [8*4+2],cs

  ; Put palette register address in DX
  mov dx,0x03d9

  mov di,80*100
  mov ax,0xb800
  mov es,ax
  mov ds,ax
  mov di,0x3ffc
  mov si,di
  mov ax,0x0303  ; Found by trial and error
  stosw
  stosw



%macro testCode 1
    mov si,0x3ffc

    black
    delay %1
    white

    mov dl,0xd9  ; 0xda

    mov cl,1
    mov al,0  ; exact value doesn't matter here - it's just to ensure the prefetch queue is filled
    mul cl
    lodsb
    mul cl
    nop
    lodsb
    mul cl
    nop
    lodsb
    mul cl

    in al,dx
    and al,1
    dec ax
    mul cl
    mul cl

    mov dl,0xd9
    black
    white

    times 76*2 - (16+21+8) nop

    mov dl,0xd9
    black
    white

    hlt
%endmacro

%macro testCode2 1
    mov si,0x3ffc

    black
    delay %1
    white

    mov dl,0xd9  ; 0xda

    mov cl,1
    mov al,0  ; exact value doesn't matter here - it's just to ensure the prefetch queue is filled
    mul cl
    lodsb
    mul cl
    nop
    lodsb
    mul cl
    nop
    lodsb
    mul cl

    in al,dx
    and al,1
    dec ax
    mul cl
    mul cl

    mov al,0
    mul cl

    mov dl,0xd9
    black
    white

    times 76*2 - (16+21+8) nop

    mov dl,0xd9

    black
    white

    hlt
%endmacro


  mov ax,0xb800
  mov es,ax
  ; Set argument for MUL
  mov cl,1

  sti

;interrupt8:
frameLoop:

  hlt

%assign i 78
%rep 16
  testCode i
%assign i i+1
%endrep

%assign i 78
%rep 16
  testCode2 i
%assign i i+1
%endrep

  mov al,(76*4) & 0xff
  out 0x40,al
  mov al,(76*4) >> 8
  out 0x40,al
  times 76*6 - 50 nop
  hlt

  mov al,(76*6) & 0xff
  out 0x40,al
  mov al,(76*6) >> 8
  out 0x40,al
  times 76*4 - 50 nop
  hlt

%rep (262-4)/6 - 34
  times 76*6 - 50 nop
  black
  white
  hlt
%endrep

  jmp frameLoop


interrupt8:
  mov al,0x20
  out 0x20,al
  iret


interrupt9:
  push ax
  in al,0x60
  cmp al,1
  jne .noEsc

  mov al,TIMER1 | LSB | MODE2 | BINARY
  out 0x43,al
  mov al,18
  out 0x41,al  ; Timer 1 rate

  writePIT16 0, 2, 0

  mov ax,3
  int 0x10

  in al,0x61
  or al,0x80
  out 0x61,al
  and al,0x7f
  out 0x61,al
  mov al,0x20
  out 0x20,al

  mov ax,0
  mov ds,ax
  mov ax,[cs:oldInterrupt8]
  mov [8*4],ax
  mov ax,[cs:oldInterrupt8+2]
  mov [8*4+2],ax
  mov ax,[cs:oldInterrupt9]
  mov [9*4],ax
  mov ax,[cs:oldInterrupt9+2]
  mov [9*4+2],ax
  sti

  mov ax,0x4c00
  int 0x21

.noEsc:
  pop ax
  jmp far [cs:oldInterrupt9]



oldInterrupt8: dw 0,0
oldInterrupt9: dw 0,0


screenData:
  times 26 db 10010010b,01001001b,00100100b
  db 10010010b,01001001b

  times 40 db 10000000b,00000000b

  times 80 db 11111111b

; We got a stable raster with no NOPs in testCode and 262-3*16 reps of waitOneScanLine
; We got a stable raster with 76*3 NOPs in testCode and 262-9*16 reps of waitOneScanLine
