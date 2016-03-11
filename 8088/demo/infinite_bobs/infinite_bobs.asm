org 0
cpu 8086

  cli

  ; Clear screen
  mov ax,cs
  mov ds,ax
  mov ax,0xb800
  mov es,ax
  mov cx,0x1000 + 4000
  mov si,data
  xor ax,ax
  xor di,di
  cld
  rep stosw

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

  ; Mode                                                09
  ;      1 +HRES                                         1
  ;      2 +GRPH                                         0
  ;      4 +BW                                           0
  ;      8 +VIDEO ENABLE                                 8
  ;   0x10 +1BPP                                         0
  ;   0x20 +ENABLE BLINK                                 0
  mov dx,0x3d8
  mov al,0x1a
  out dx,al

  ; Palette                                             00
  ;      1 +OVERSCAN B                                   0
  ;      2 +OVERSCAN G                                   2
  ;      4 +OVERSCAN R                                   4
  ;      8 +OVERSCAN I                                   0
  ;   0x10 +BACKGROUND I                                 0
  ;   0x20 +COLOR SEL                                    0
  inc dx
  mov al,0x0f
  out dx,al

  mov dl,0xd4

  ;   0xff Horizontal Total                             71
  mov ax,0x3800
  out dx,ax

  ;   0xff Horizontal Displayed                         50
  mov ax,0x2801
  out dx,ax

  ;   0xff Horizontal Sync Position                     5a
  mov ax,0x2d02
  out dx,ax

  ;   0x0f Horizontal Sync Width                        0d
  mov ax,0x0a03
  out dx,ax

  ;   0x7f Vertical Total                               3d
  mov ax,0x3b04
  out dx,ax

  ;   0x1f Vertical Total Adjust                        00
  mov ax,0x0005
  out dx,ax

  ;   0x7f Vertical Displayed                           02
  mov ax,0x3206
  out dx,ax

  ;   0x7f Vertical Sync Position                       18
  mov ax,0x3807
  out dx,ax

  ;   0x03 Interlace Mode                               02   0 = non interlaced, 1 = interlace sync, 3 = interlace sync and video
  mov ax,0x0308
  out dx,ax

  ;   0x1f Max Scan Line Address                        00
  mov ax,0x0309
  out dx,ax

  ; Cursor Start                                        06
  ;   0x1f Cursor Start                                  6
  ;   0x60 Cursor Mode                                   0
  mov ax,0x060a
  out dx,ax

  ;   0x1f Cursor End                                   07
  mov ax,0x080b
  out dx,ax

  ;   0x3f Start Address (H)                            00
  mov ax,0x000c
  out dx,ax

  ;   0xff Start Address (L)                            00
  mov ax,0x000d
  out dx,ax

  ;   0x3f Cursor (H)                                   03
  mov ax,0x030e
  out dx,ax

  ;   0xff Cursor (L)                                   c0
  mov ax,0xc00f
  out dx,ax

  mov dl,0xda
  waitForVerticalSync
  waitForDisplayEnable

  xor ax,ax
  mov ds,ax
  cli


; Program timer 0 rate to 2
; timer 0 almost immediately fires, now IRQ0 is waiting
; We wait for display enable, now raster is on scanline 0 and timer 0 is firing rapidly
; We program timer 0 for 240*76, it almost immediately starts counting down from this value
; We enable interrupts and IRQ0 (unreprogrammed). The next IRQ0 will be on scanline 240
; We program timer 0 for 22*76, this will be what it counts down from when it gets to scanline 240
; On scanline 240 we get interrupt8a


; With interlace mode 1, we get our blue/black interface moving up. That means the screen is half a scanline longer


  mov al,0x34
  out 0x43,al
  mov al,2
  out 0x40,al
  mov al,0
  out 0x40,al

  mov al,(240*76) & 0xff
  out 0x40,al

  waitForVerticalSync
  waitForDisplayEnable

  mov al,(240*76) >> 8
  out 0x40,al

  times 5 nop
  sti
  times 5 nop      ; IRQ0 will fire in here, then the next IRQ0 after that will be interrupt8a at the right time...
  cli

  mov al,(22*76) & 0xff
  out 0x40,al
  mov al,(22*76) >> 8
  out 0x40,al               ; ... with the right count

  mov ax,[8*4]
  mov [cs:savedInterrupt8],ax
  mov ax,[8*4+2]
  mov [cs:savedInterrupt8+2],ax
  mov ax,interrupt8a
  mov [8*4],ax
  mov [8*4+2],cs
  sti

  ; All video processing is done in the IRQ0 handler!
  mov ah,0
  int 0x16

  cli
  mov ax,[cs:savedInterrupt8]
  mov [8*4],ax
  mov ax,[cs:savedInterrupt8+2]
  mov [8*4+2],ax
  mov al,0x34
  out 0x43,al
  mov al,0
  out 0x40,al
  out 0x40,al
  sti

  ; Set the CGA back to a normal mode so we don't risk breaking anything
  mov ax,3
  int 0x10

  int 0x67
  ret


; interrupt8a runs on scanline 240 and sets up the "short" field for flipping the odd/even bit
interrupt8a:
  push ax
  push dx

  mov dx,0x3d4
  mov ax,0x0206 ; Vertical Displayed
  out dx,ax
  mov ax,0x0404 ; Vertical Total
  out dx,ax
  mov ax,0x0105 ; Vertical Total Adjust
  out dx,ax

  push ds
  xor ax,ax
  mov ds,ax
  mov word[0x20],interrupt8b

  mov al,(240*76) & 0xff
  out 0x40,al
  mov al,(240*76) >> 8
  out 0x40,al

  pop ds
  pop dx
doneInterrupt8:
  mov al,0x20
  out 0x20,al
  pop ax
  iret


; interrupt8b runs on scanline 0 and sets up the "normal" field
interrupt8b:
  push ax
  push dx

  mov dx,0x3d4
  mov ax,0x3206 ; Vertical Displayed
  out dx,ax
  mov ax,0x3b04 ; Vertical Total
  out dx,ax
  mov ax,0x0005 ; Vertical Total Adjust
  out dx,ax

  push ds
  xor ax,ax
  mov ds,ax
  mov word[0x20],interrupt8a

  mov al,(22*76) & 0xff
  out 0x40,al
  mov al,(22*76) >> 8
  out 0x40,al

  pop ds
  pop dx
  add word[cs:timerCount],76*262
  jnc doneInterrupt8
  pop ax
  jmp far [cs:savedInterrupt8]


savedInterrupt8:
  dw 0,0
timerCount:
  dw 0



drawBob:
  ; y in bx
  add bx,bx
  mov di,[bx+yTable]
  ; x in ax
  mov si,ax
  and si,1
  add si,si
  shr ax,1
  add di,ax
  jmp [si+jumpTable]

yTable:

%assign i 0
%rep 100
  dw i*80
  %assign i i+1
%endrep
;%assign i 0
;%rep 100
;  dw i*80 + 0x2000
;  %assign i i+1
;%endrep

jumpTable:

