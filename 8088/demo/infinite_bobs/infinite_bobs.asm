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

  mov dx,[cppData]
  mov ax,cppData + 4
  mov [patch2+2],ax
  add dx,dx
  add ax,dx
  mov [patch3+2],ax
  add ax,dx
  mov [patch1+2],ax


  ; Clear screen
  mov ax,0xb800
  mov es,ax
  mov cx,0x1000 + 4000
  xor ax,ax
  xor di,di
  cld
  rep stosw

  call startISAV

mainLoop:
  mov ax,0x100  ; x frequency
  add [xPhase],ax
  adc word[xPhase+2],0
  mov ax,[cppData]  ; nx
  cmp word[xPhase+1],ax
  jl noXFixup
  sub word[xPhase+1],ax
noXFixup:

  mov ax,0x100  ; y frequency
  add [yPhase],ax
  adc word[yPhase+2],0
  mov ax,[cppData+2]
  cmp word[yPhase+1],ax
  jl noYFixup
  sub word[yPhase+1],ax
noYFixup:

  mov bx,[yPhase+1]
  add bx,bx
patch1:
  mov di,[bx+0x9999]
  mov bx,[xPhase+1]
  add bx,bx
patch2:
  add di,[bx+0x9999]
  cmp byte[page],0
  je patch3
  or di,0x2000
patch3:
  call [bx+0x9999]

  mov al,[page]
  xor ah,ah
  push ax
  call setDisplayPage
  mov cx,[delayFrames]
delayLoop:
  mov ah,1
  int 0x16
  jnz noKey
  mov ah,0
  int 0x16
  cmp al,'+'
  jne notPlus
  dec word[delayFrames]
notPlus:
  cmp al,'-'
  jne notMinus
  inc word[delayFrames]
notMinus:
  cmp al,' '
  jne notSpace
  mov ah,0
  int 0x16
notSpace:
  cmp al,27
  je finishLoop
noKey:
  jcxz doneDelay
  mov dx,0x3da
  waitForVerticalSync
  waitForNoVerticalSync
  dec cx
  jmp delayLoop
doneDelay:
  call waitForSafeToDraw
  xor byte[page],1

  jmp mainLoop

finishLoop:
  call stopISAV

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


xPhase:
  dw 0, 0
yPhase:
  dw 0, 0
xFrequency:
  dw 0x100
yFrequency:
  dw 0x100
page:
  db 0
delayFrames:
  dw 0


; ISAV code starts here.

savedInterrupt8:
  dw 0,0
timerCount:
  dw 0
activePage:
  db 0
setPage:
  db 0

; Puts the CGA card in ISAV mode
startISAV:
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
  ret


; Returns the CGA to normal mode
stopISAV:
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
  ret


; Sets the page to display next (0 or 1)
setDisplayPage:
  push bp
  mov bp,sp
  mov cx,[bp+4]
  push ds

  xor bx,bx
  mov ds,bx
  mov al,0x04
  cli
  out 0x43,al
  mov bx,[bx]
  sti
  in al,0x40
  mov ah,al
  in al,0x40
  xchg ah,al

  ; TODO

  pop bp
  ret


; Returns true if the drawing page (the one that was not requested in the most recent call to setDisplayPage()) is no longer active
safeToDraw:
  ; TODO
  ret


waitForSafeToDraw:
  call safeToDraw
  cmp ax,0
  je waitForSafeToDraw
  ret



cppData:

