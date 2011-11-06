org 0
cpu 8086

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


  ; Initialize interrupt vector
  mov ax,0
  mov ds,ax
  mov word[0x20],interrupt8
  mov [0x22],cs

  ; Enable IRQ0
  mov al,0xfe  ; Enable IRQ 0 (timer), disable others
  out 0x21,al



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

;
;  mov dl,0xda
;  in al,dx


;  hlt


  ;Turn off refresh
  mov al,0x60  ; Timer 1, write LSB, mode 0, binary
  out 0x43,al
  mov al,0x01  ; Count = 0x0001 so we'll stop almost immediately
  out 0x41,al

  ; Set "stosb" destination to be CGA memory
  mov ax,0xb800
  mov es,ax
  mov di,80*100

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

  times 512 nop


  ; CRTC should be in lockstep too by now. Set it up for normal video

  ; Mode                                                1e
  ;      1 +HRES                                         0
  ;      2 +GRPH                                         2
  ;      4 +BW                                           4
  ;      8 +VIDEO ENABLE                                 8
  ;   0x10 +1BPP                                        10
  ;   0x20 +ENABLE BLINK                                 0
  mov dl,0xd8
  mov al,0x1a; e
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


  mov dl,0xd9
  mov al,1
  out dx,al


  times 23 nop


  ; Use IRQ0 to go into lockstep with timer 0
  mov al,0x34  ; Timer 0, write LSB+MSB, mode 2, binary
  out 0x43,al
  mov al,(76*262) & 0xff
  out 0x40,al
  mov al,(76*262) >> 8
  out 0x40,al
  sti

  mov al,2
  out dx,al


  ; Set up second argument to MUL
  mov cl,1

  ; Put palette register address in DX
  mov dx,0x03d9

%macro waitOneScanLine 0
  times 76 nop
%endmacro

  ; We can't spend the entire frame sitting in HLT or RAM will decay
%rep 261
  waitOneScanLine
%endrep

  mov al,3
  out dx,al

  hlt


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

%macro testCode 1
    waitOneScanLine
    waitOneScanLine
    black
    white
    delay %1
    black
    stosb
    jmp $+2
    white
%endmacro


interrupt8:

  mov al,4
  out dx,al

  mov ax,0x8000
  mov es,ax
  mov di,100*80

  delay 78
  delay 78

  waitOneScanLine
  nop
  nop
  nop

  testCode 78
  delay 78
  times 16 nop

  testCode 79
  delay 78
  delay 78

  testCode 80
  delay 78
  delay 78

  testCode 81
  delay 78
  delay 78

  testCode 82
  delay 78
  delay 78

  testCode 83
  delay 78
  delay 78

  testCode 84
  delay 78
  delay 78

  testCode 85
  delay 78
  delay 78

  testCode 86
  delay 78
  delay 78

  testCode 87
  delay 78
  delay 78

  testCode 88
  delay 78
  delay 78

  testCode 89
  delay 78
  delay 78

  testCode 90
  delay 78
  delay 78

  testCode 91
  delay 78
  delay 78

  testCode 92
  delay 78
  delay 78

  testCode 93
  delay 78
  delay 78

  waitOneScanLine
  waitOneScanLine


  mov ax,0xb800
  mov es,ax
  mov di,100*80

  delay 78
  delay 78
  delay 78

  testCode 78
  delay 78
  delay 78

  testCode 79
  delay 78
  delay 78

  testCode 80
  delay 78
  delay 78

  testCode 81
  delay 78
  delay 78

  testCode 82
  delay 78
  delay 78

  testCode 83
  delay 78
  delay 78

  testCode 84
  delay 78
  delay 78

  testCode 85
  delay 78
  delay 78

  testCode 86
  delay 78
  delay 78

  testCode 87
  delay 78
  delay 78

  testCode 88
  delay 78
  delay 78

  testCode 89
  delay 78
  delay 78

  testCode 90
  delay 78
  delay 78

  testCode 91
  delay 78
  delay 78

  testCode 92
  delay 78
  delay 78

  testCode 93
  delay 78
  delay 78

  waitOneScanLine
  waitOneScanLine


  black
  %rep 155
    waitOneScanLine
  %endrep
  white

  mov al,0x20
  out 0x20,al
  pop bx
  pop bx
  popf
  hlt




screenData:
  times 26 db 10010010b,01001001b,00100100b
  db 10010010b,01001001b

  times 40 db 10000000b,00000000b

  times 80 db 11111111b


