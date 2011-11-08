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


;  ; Wait ~55ms
;  mov al,0x34  ; Timer 0, write LSB+MSB, mode 2, binary
;  out 0x43,al
;  mov al,0
;  out 0x40,al
;  out 0x40,al
;
;  ; Initialize from IRQ0 so that's in a known state
;
;  mov ax,0
;  mov ds,ax
;  mov word[0x20],initialize
;  mov [0x22],cs
;
;  ; Enable IRQ0
;  mov al,0xfe  ; Enable IRQ 0 (timer), disable others
;  out 0x21,al
;
;  sti
;  hlt
;
;initialize:
;  ; Acknowledge IRQ0
;  mov al,0x20
;  out 0x20,al
;
;  ; Initialize interrupt vector
;  mov ax,0
;  mov ds,ax
;  mov word[0x20],interrupt8
;  mov [0x22],cs


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

  mov dx,0x03d8
  mov al,0x0a
  out dx,al

  ; Set up CRTC for 1 character by 2 scanline "frame". This gives us 2 lchars
  ; per frame.
  mov dl,0xd4
  ;   0xff Horizontal Total
  mov ax,0x0000
  out dx,ax
  ;   0xff Horizontal Displayed                         28
  mov ax,0x0101
  out dx,ax
  ;   0xff Horizontal Sync Position                     2d
  mov ax,0x2d02
  out dx,ax
  ;   0x0f Horizontal Sync Width                        0a
  mov ax,0x0a03
  out dx,ax
  ;   0x7f Vertical Total                               7f
  mov ax,0x0104
  out dx,ax
  ;   0x1f Vertical Total Adjust                        06
  mov ax,0x0005
  out dx,ax
  ;   0x7f Vertical Displayed                           64
  mov ax,0x0106
  out dx,ax
  ;   0x7f Vertical Sync Position                       70
  mov ax,0x0007
  out dx,ax
  ;   0x03 Interlace Mode                               02
  mov ax,0x0208
  out dx,ax
  ;   0x1f Max Scan Line Address                        01
  mov ax,0x0009
  out dx,ax

  times 512 nop
  nop

  ; To get the CRTC into lockstep with the CGA and CPU, we need to figure out
  ; which of the two possible CRTC states we're in and switch states if we're
  ; in the wrong one by waiting for an odd number of lchars more in one code
  ; path than in the other. To keep CGA and CPU in lockstep, we also need both
  ; code paths to take the same time mod 3 lchars, so we wait 3 lchars more on
  ; one code path than on the other.
  mov dl,0xda
  in al,dx
  jmp $+2
  test al,1
  jz shortPath
  times 2 nop
  jmp $+2
shortPath:


  ; CRTC should be in lockstep too by now. Set it up for normal video

  ; Mode                                                1e
  ;      1 +HRES                                         0
  ;      2 +GRPH                                         2
  ;      4 +BW                                           4
  ;      8 +VIDEO ENABLE                                 8
  ;   0x10 +1BPP                                        10
  ;   0x20 +ENABLE BLINK                                 0
  mov dl,0xd8
  mov al,0x1e
  out dx,al

  inc dx
  mov al,0x0f
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


;  ; Use IRQ0 to go into lockstep with timer 0
;  mov al,0x34  ; Timer 0, write LSB+MSB, mode 2, binary
;  out 0x43,al
;  mov al,(76*262) & 0xff
;  out 0x40,al
;  mov al,(76*262) >> 8
;  out 0x40,al
;
;  sti


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

;  hlt


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

  delay 85
  delay 85
  delay 85
;  delay 85
;  delay 85
  times 15 nop
  waitOneScanline

;interrupt8:
frameLoop:

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

  mov ax,0x8000
  mov es,ax
  mov di,100*80

  delay 78
  delay 78
  times 12 nop

  testCode 78
  delay 78
  times 13 nop

  testCode 79
  delay 81
  times 12 nop

  testCode 80
  delay 80
  times 12 nop

  testCode 81
  delay 79
  times 12 nop

  testCode 82
  delay 78
  times 12 nop

  testCode 83
  delay 81
  times 11 nop

  testCode 84
  delay 80
  times 11 nop

  testCode 85
  delay 79
  times 11 nop

  testCode 86
  delay 78
  times 11 nop

  testCode 87
  delay 81
  times 10 nop

  testCode 88
  delay 80
  times 10 nop

  testCode 89
  delay 79
  times 10 nop

  testCode 90
  delay 78
  times 10 nop

  testCode 91
  delay 81
  times 9 nop

  testCode 92
  delay 80
  times 9 nop

  testCode 93
  delay 79
  times 9 nop

  waitOneScanLine
  waitOneScanLine


  mov ax,0xb800
  mov es,ax
  mov di,100*80

  delay 82
  delay 78
  delay 78
  times 12 nop

  testCode 78
  delay 83
  times 10 nop

  testCode 79
  delay 83
  times 10 nop

  testCode 80
  delay 83
  times 10 nop

  testCode 81
  delay 83
  times 10 nop

  testCode 82
  delay 83
  times 10 nop

  testCode 83
  delay 81
  times 9 nop

  testCode 84
  delay 81
  times 9 nop

  testCode 85
  delay 81
  times 9 nop

  testCode 86
  delay 81
  times 9 nop

  testCode 87
  delay 81
  times 9 nop

  testCode 88
  delay 80
  times 8 nop

  testCode 89
  delay 80
  times 8 nop

  testCode 90
  delay 80
  times 8 nop

  testCode 91
  delay 80
  times 8 nop

  testCode 92
  delay 80
  times 8 nop

  testCode 93
  delay 79
  times 7 nop

  testCode 85
  delay 79
  times 10 nop

%rep 7
  waitOneScanLine
%endrep

  black
  %rep 150
    waitOneScanLine
  %endrep
  white

  delay 85
  delay 85
  delay 85
  delay 85
  times 8 nop

jmp frameLoop

;  mov al,0x20
;  out 0x20,al
;  pop bx
;  pop bx
;  popf
;  hlt




screenData:
  times 26 db 10010010b,01001001b,00100100b
  db 10010010b,01001001b

  times 40 db 10000000b,00000000b

  times 80 db 11111111b


