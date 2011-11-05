org 0
cpu 8086

  ; Set CRTC to use +HRES
  mov dx,0x3d8
  mov al,1
  out dx,al

  ; Set up CRTC for 1 character by 2 scanline "frame". This gives us 1 lchar
  ; per frame, which means that going into lchar lockstep (i.e. any CGA memory
  ; access) also puts us into frame lockstep.
  mov dl,0xd4
  ;   0xff Horizontal Total       00
  mov ax,0x0000
  out dx,ax
  ;   0x7f Vertical Total         01
  mov ax,0x0104
  out dx,ax
  ;   0x1f Vertical Total Adjust  00
  mov ax,0x0005
  out dx,ax
  ;   0x1f Max Scan Line Address  00
  mov ax,0x0009
  out dx,ax

  ; Disable interrupts and turn off refresh
  cli
  mov al,0x60  ; Timer 1, write LSB, mode 0, binary
  out 0x43,al
  mov al,0x01  ; Count = 0x0001 so we'll stop almost immediately
  out 0x41,al

  ; Set "stosb" destination to be CGA memory
  mov ax,0xb800
  mov es,ax
  xor di,di

  ; Set argument for MUL
  mov cl,1

  ; Ensure "stosb" won't take us out of video memory
  cld

  ; Go into CGA lockstep. The delays were determined by trial and error.
  jmp $+2      ; Clear prefetch queue
  stosb        ; From 16 down to 3 possible CGA/CPU relative phases.
  mov al,0x01
  mul cl
  stosb        ; Down to 2 possible CGA/CPU relative phases.
  mov al,0x7f
  mul cl
  stosb        ; Down to 1 possible CGA/CPU relative phase: lockstep achieved.


  ; Initialize video memory with appropriate patterns
  mov ax,cs
  mov ds,ax
  xor di,di

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


  ; CRTC should be in lockstep too by now. Set it up for normal video

  ; Mode                                                1a
  ;      1 +HRES                                         0
  ;      2 +GRPH                                         2
  ;      4 +BW                                           0
  ;      8 +VIDEO ENABLE                                 8
  ;   0x10 +1BPP                                        10
  ;   0x20 +ENABLE BLINK                                 0
  mov dl,0xd8
  mov al,0x1a
  out dx,al

  inc dx
  mov al,0x0f
  out dx,al

  ; Palette                                             00
  ;      1 +OVERSCAN B                                   0
  ;      2 +OVERSCAN G                                   0
  ;      4 +OVERSCAN R                                   0
  ;      8 +OVERSCAN I                                   0
  ;   0x10 +BACKGROUND I                                 0
  ;   0x20 +COLOR SEL                                    0
  inc dx
  mov al,0
  out dx,al

  mov dl,0xd4

  ;   0xff Horizontal Total                             38
  mov ax,0x3800
  out dx,ax

  ;   0xff Horizontal Displayed                         28
  mov ax,0x2801
  out dx,ax

  ;   0xff Horizontal Sync Position                     2d
  mov ax,0x2d02
  out dx,ax

  ;   0x0f Horizontal Sync Width                        0a
  mov ax,0x0a03
  out dx,ax

  ;   0x7f Vertical Total                               7f
  mov ax,0x7f04
  out dx,ax

  ;   0x1f Vertical Total Adjust                        06
  mov ax,0x0605
  out dx,ax

  ;   0x7f Vertical Displayed                           64
  mov ax,0x6406
  out dx,ax

  ;   0x7f Vertical Sync Position                       70
  mov ax,0x7007
  out dx,ax

  ;   0x03 Interlace Mode                               02
  mov ax,0x0208
  out dx,ax

  ;   0x1f Max Scan Line Address                        01
  mov ax,0x0109
  out dx,ax

  ; Cursor Start                                        20
  ;   0x1f Cursor Start                                 00
  ;   0x60 Cursor Mode                                  20
  mov ax,0x200a
  out dx,ax

  ;   0x1f Cursor End                                   00
  mov ax,0x000b
  out dx,ax

  ;   0x3f Start Address (H)                            00
  inc ax
  out dx,ax

  ;   0xff Start Address (L)                            00
  inc ax
  out dx,ax

  ;   0x3f Cursor (H)                                   00
  inc ax
  out dx,ax

  ;   0xff Cursor (L)                                   00
  inc ax
  out dx,ax



  ; Set up second argument to MUL
  mov cl,1

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


