org 0x100
;org 0
cpu 8086


%macro waitForDisplayEnable 0
  %%waitForDisplayEnable:
    in al,dx                       ; 1 1 2
    test al,1                      ; 2 0 2
    jnz %%waitForDisplayEnable     ; 2 0 2
%endmacro

%macro waitForDisplayDisable 0
  %%waitForDisplayDisable:
    in al,dx                       ; 1 1 2
    test al,1                      ; 2 0 2
    jz %%waitForDisplayDisable     ; 2 0 2
%endmacro

%macro waitForVerticalSync 0
  %%waitForVerticalSync:
    in al,dx
    test al,8
    jz %%waitForVerticalSync
%endmacro

%macro waitForNoVerticalSync 0
  %%waitForNoVerticalSync:
    in al,dx
    test al,8
    jnz %%waitForNoVerticalSync
%endmacro


  mov ax,cs
  mov ds,ax
  cli
  mov ss,ax
  mov sp,0xfffe
  sti

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
  mov ax,0x200  ; x frequency
  add [xPhase],ax
  adc word[xPhase+2],0
  mov ax,[cppData]  ; nx
  cmp word[xPhase+1],ax
  jl noXFixup
  sub word[xPhase+1],ax
noXFixup:

  mov ax,0x200  ; y frequency
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
  je firstPage
  or di,0x2000
firstPage:
;    mov dx,0x3d9
;    mov al,2
;    out dx,al

patch3:
  call [bx+0x9999]
;    mov dx,0x3d9
;    mov al,3
;    out dx,al

  mov al,[page]
  xor ah,ah
  push ax
  call setDisplayPage
  mov cx,[delayFrames]
  and cx,63
delayLoop:
  mov ah,1
  int 0x16
  jz noKey
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
;  dec word[delayFrames]
;    mov dx,0x3d9
;    mov al,4
;    out dx,al

  call waitForSafeToDraw
;    mov dx,0x3d9
;    mov al,5
;    out dx,al

  xor byte[page],1

  jmp mainLoop

finishLoop:
  call stopISAV

;  int 0x67
  mov ax,0x4c00
  int 0x21
  ret


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
  db 1
setPage:
  db 1
pageWaiting:
  db 0
longField:
  db 0
needLongField:
  db 0

; Puts the CGA card in ISAV mode
startISAV:
  push ds
  ; Mode                                                09
  ;      1 +HRES                                         1
  ;      2 +GRPH                                         0
  ;      4 +BW                                           0
  ;      8 +VIDEO ENABLE                                 8
  ;   0x10 +1BPP                                         0
  ;   0x20 +ENABLE BLINK                                 0
  mov dx,0x3d8
  mov al,0x1a  ; 0x1a
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
  mov ax,0x0008
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
  waitForVerticalSync
  waitForDisplayEnable

  xor ax,ax
  mov ds,ax

  mov ax,0x0308
  mov dl,0xd4
  out dx,ax
  mov dl,0xda

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

  pop ds
  ret


; Returns the CGA to normal mode
stopISAV:
  cli
  xor ax,ax
  mov ds,ax
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
  mov cl,[bp+4]
  cmp cl,[setPage]
  je .done
  call waitForSafeToDraw
  cmp byte[longField],0
  je .noPageWaiting
  mov byte[pageWaiting],1
  jmp .done
.noPageWaiting:
  cli
  mov [setPage],cl
  mov byte[needLongField],1
  sti

  push ds
  xor bx,bx
  mov ds,bx
  mov al,0x04
  out 0x43,al    ; We latch the count before reading the vector otherwise we could end up
  mov bl,[0x20]  ; on scanline 240 with a timer 0 count indicating scanline 22 or so
  in al,0x40
  mov ah,al
  in al,0x40
  xchg ah,al
  pop ds

  cmp bl,interrupt8b  ; Warning expected here, doing the obvious "& 0xff" turns it into an error.
  je .done
  cmp ax,76*3
  jl .done

  mov dx,0x3d4
  mov ax,0x4004
  out dx,ax
  mov ax,0x0105
  add ah,[activePage]
  out dx,ax
  mov byte[longField],1
  mov byte[needLongField],0

.done:
  pop bp
  ret


; Returns CF=1 if it's safe to draw, i.e. if the drawing page (the one that was
; not requested in the most recent call to setDisplayPage()) will not be used
; as active image data again before the set page becomes the display page.
safeToDraw:
  cmp byte[needLongField],0
  jne .done ; unsafe
  cmp byte[longField],0
  je .safe
  cmp byte[pageWaiting],0
  jne .done ; unsafe

  ; longField == 1, needLongField == 0 is only safe if we're after scanline 200
  mov al,0x04
  out 0x43,al
  in al,0x40
  mov ah,al
  in al,0x40
  xchg ah,al
  cmp ax,40*76

.done:
  ret
.safe:
  stc
  ret
.unsafe:
  clc
  ret


waitForSafeToDraw:
  call safeToDraw
  jnc waitForSafeToDraw
  ret


; interrupt8a runs on scanline 240 and sets up the "short" field for flipping the odd/even bit
interrupt8a:
  push ax

  cmp byte[cs:longField],0
  jne .doneCRTC
  push dx
  mov dx,0x3d4
  mov ax,0x0206 ; Vertical Displayed
  out dx,ax
  mov ax,0x0404 ; Vertical Total
  out dx,ax
  mov ax,0x0105 ; Vertical Total Adjust
  out dx,ax
;    mov dl,0xd9
;    mov al,1
;    out dx,al
  pop dx
.doneCRTC:

  push ds
  xor ax,ax
  mov ds,ax
  mov word[0x20],interrupt8b
  pop ds

  mov al,(240*76) & 0xff
  out 0x40,al
  mov al,(240*76) >> 8
  out 0x40,al

doneInterrupt8:
  mov al,0x20
  out 0x20,al
  pop ax
  iret


interrupt8aEnd:
%if ((interrupt8aEnd - interrupt8a) & 0xff) == 0
  nop
%endif


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
;    mov dl,0xd9
;    mov al,2
;    out dx,al
;    mov dl,0xd4
  cmp byte[cs:longField],0
  je .noActiveFlip
  mov byte[cs:longField],0
  xor byte[cs:activePage],1
.noActiveFlip:

  cmp byte[cs:needLongField],0
  je .noNewChange

  mov ax,0x4004
  out dx,ax
  mov ax,0x0105
  add ah,[cs:activePage]
  out dx,ax
  mov byte[cs:longField],1
  mov byte[cs:needLongField],0

;    mov dl,0xd9
;    mov al,0
;    out dx,al

  cmp byte[cs:pageWaiting],0
  je .noNewChange

  mov byte[cs:pageWaiting],0
  mov byte[cs:needLongField],1
  xor byte[cs:setPage],1

.noNewChange:

  pop dx

  push ds
  xor ax,ax
  mov ds,ax
  mov word[0x20],interrupt8a
  pop ds

  mov al,(22*76) & 0xff
  out 0x40,al
  mov al,(22*76) >> 8
  out 0x40,al

  add word[cs:timerCount],76*262
  jnc doneInterrupt8
  pop ax
  jmp far [cs:savedInterrupt8]


; Page 0, no page change:
;   (0x3b + 1) * 2 * 2 == 240 scanlines main field
;                           0 scanlines main field adjust
;   (0x04 + 1) * 2 * 2 ==  20 scanlines small field
;                           1 scanline small field adjust
;                           1 scanline CRTC extra interlace

; Page 1, no page change:
;   (0x3b + 1) * 2 * 2 == 240 scanlines main field
;                           0 scanlines main field adjust
;                           1 scanline CRTC extra interlace
;   (0x04 + 1) * 2 * 2 ==  20 scanlines small field
;                           1 scanline small field adjust

; Page 0, switch to page 1 for next frame:
;   (0x40 + 1) * 2 * 2 == 260 scanlines
;                           2 scanlines adjust

; Page 1, switch to page 0 for next frame:
;   (0x40 + 1) * 2 * 2 == 260 scanlines
;                           1 scanline adjust
;                           1 scanline CRTC extra interlace

; In order to be able to switch pages before scanline 240, we'll need to do all our drawing on scanlines -21 .. 238
; (we can actually do ~26 scanlines more drawing after the page flip, though really we should start work on the following page by that point)

; To avoid having to change the programmed timer counts, we want interrupt8b to fire on scanline 1, and interrupt8a to run on scanline 241
; (which will be CRTC scanline 0 on page 1 and CRTC scanline 1 on page 0).

cppData:

