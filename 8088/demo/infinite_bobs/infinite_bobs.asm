;org 0x100
org 0
cpu 8086

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

patch3:
  call [bx+0x9999]

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

  call waitForSafeToDraw

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
  mov ax,0x0104
  out dx,ax

  ;   0x1f Vertical Total Adjust                        00
  mov ax,0x0005
  out dx,ax

  ;   0x7f Vertical Displayed                           02
  mov ax,0x0106
  out dx,ax

  ;   0x7f Vertical Sync Position                       18
  mov ax,0x3807
  out dx,ax

  ;   0x03 Interlace Mode                               02   0 = non interlaced, 1 = interlace sync, 3 = interlace sync and video
  mov ax,0x0008
  out dx,ax

  ;   0x1f Max Scan Line Address                        00
  mov ax,0x0009
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
  cli

  mov al,0x34
  out 0x43,al
  mov al,0
  out 0x40,al
  out 0x40,al

  mov al,76*2 + 1
  out 0x40,al
  mov al,0
  out 0x40,al

  xor ax,ax
  mov ds,ax
  mov ax,[0x20]
  mov [cs:originalInterrupt8],ax
  mov ax,[0x22]
  mov [cs:originalInterrupt8+2],ax
  mov word[0x20],int8_oe0
  mov [0x22],cs

  in al,0x21
  mov [cs:originalIMR],al
  mov al,0xfe
  out 0x21,al

  sti
setupLoop:
  hlt
  jmp setupLoop


originalInterrupt8:
  dw 0, 0
originalIMR:
  db 0
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


  ; Step 0 - don't do anything (we've just completed wait for CRTC stabilization)
int8_oe0:
  mov word[0x20],int8_oe1
  mov al,0x20
  out 0x20,al
  iret


  ; Step 1, wait until display is disabled, then change interrupts
int8_oe1:
  in al,dx
  test al,1
  jz .noInterruptChange   ; jump if not -DISPEN, finish if -DISPEN

  mov word[0x20],int8_oe2

.noInterruptChange:
  mov al,0x20
  out 0x20,al
  iret


  ; Step 2, wait until display is enabled - then we'll be at the start of the active area
int8_oe2:
  in al,dx
  test al,1
  jnz .noInterruptChange  ; jump if -DISPEN, finish if +DISPEN

  mov word[0x20],int8_oe3
  mov cx,2

.noInterruptChange:
  mov al,0x20
  out 0x20,al
  iret


  ; Step 3 - this interrupt occurs one timer cycle into the active area.
  ; The pattern of scanlines on the screen is +-+-- As the interrupt runs every other scanline, the pattern of scanlines in terms of what is seen from the interrupt is ++---.
int8_oe3:
  mov dl,0xd4
  mov ax,0x0308  ; Set interlace mode to ISAV
  out dx,ax
  mov dl,0xda

  loop .noInterruptChange
  mov word[0x20],int8_oe4
.noInterruptChange:

  mov al,76*2
  out 0x40,al
  mov al,0
  out 0x40,al

  mov al,0x20
  out 0x20,al
  iret


  ; Step 4 - this interrupt occurs two timer cycles into the active area.
int8_oe4:
  in al,dx
  test al,1
  jnz .noInterruptChange  ; jump if -DISPEN, finish if +DISPEN

  mov word[0x20],int8_oe5

.noInterruptChange:
  mov al,0x20
  out 0x20,al
  iret


  ; Step 5
int8_oe5:
  in al,dx
  test al,1
  jz .noInterruptChange   ; jump if not -DISPEN, finish if -DISPEN (i.e. scanline 4)

  mov word[0x20],int8_oe6

  mov al,76*2 - 3
  out 0x40,al
  mov al,0
  out 0x40,al

.noInterruptChange:
  mov al,0x20
  out 0x20,al
  iret


  ; Step 6. This occurs on scanline 1. The next interrupt will be on scanline 3.
int8_oe6:
  mov word[0x20],int8_oe7

  mov al,76*2
  out 0x40,al
  mov al,0
  out 0x40,al

  mov al,0x20
  out 0x20,al
  iret


  ; Step 7. This occurs on scanline 3 (one timer cycle before the active area starts). The next interrupt will be on scanline 0.
int8_oe7:
  mov word[0x20],int8_oe8
  mov al,0x20
  out 0x20,al
  iret


  ; Step 8 - scanline 0, next interrupt on scanline 2
int8_oe8:
  mov al,(20*76) & 0xff
  out 0x40,al
  mov al,(20*76) >> 8
  out 0x40,al

  mov word[0x20],int8_oe9

  mov dl,0xd4

  mov al,0x20
  out 0x20,al
  add sp,6
  sti
  pop ds
  ret


  ; Step 9 - initial short (odd) field
int8_oe9:
  push ax
  push dx
  mov dx,0x3d4
  mov ax,0x0309
  out dx,ax
  mov ax,0x0106
  out dx,ax
  mov ax,0x0304    ; 0404
  out dx,ax
  mov ax,0x0305
  out dx,ax
  pop dx

  mov al,(242*76) & 0xff
  out 0x40,al
  mov al,(242*76) >> 8
  out 0x40,al

  mov al,[cs:originalIMR]
  out 0x21,al

  push ds
  xor ax,ax
  mov ds,ax
  mov word[0x20],int8_isav0
  pop ds

  mov al,0x20
  out 0x20,al
  pop ax
  iret


  ; Final 0 - scanline 0
int8_isav0:
  push ax
  push dx
  mov dx,0x3d4
  mov ax,0x3206
  out dx,ax
  mov ax,0x3b04
  out dx,ax

  cmp byte[cs:longField],0
  je .noActiveFlip
  mov byte[cs:longField],0
  xor byte[cs:activePage],1
.noActiveFlip:

  mov ax,0x0105
  add ah,[cs:activePage]
  out dx,ax

  cmp byte[cs:needLongField],0
  je .noNewChange

  mov ax,0x4004
  out dx,ax
  mov byte[cs:longField],1
  mov byte[cs:needLongField],0

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
  mov word[0x20],int8_isav1
  pop ds

  mov al,(20*76) & 0xff
  out 0x40,al
  mov al,(20*76) >> 8
  out 0x40,al

  add word[cs:timerCount],76*262
  jnc doneInterrupt8
  pop ax
  jmp far [cs:originalInterrupt8]


int8_isav0_end:
%if ((int8_isav0_end - int8_isav0) & 0xff) == 0
  nop
%endif


  ; Final 1 - scanline 242
int8_isav1:
  push ax
  cmp byte[cs:longField],0
  jne .doneCRTC
  push dx
  mov dx,0x3d4
  mov ax,0x0106
  out dx,ax
  mov ax,0x0304
  out dx,ax
  mov ax,0x0405
  sub ah,[cs:activePage]
  out dx,ax
  pop dx
.doneCRTC:

  push ds
  xor ax,ax
  mov ds,ax
  mov word[0x20],int8_isav0
  pop ds

  mov al,(242*76) & 0xff
  out 0x40,al
  mov al,(242*76) >> 8
  out 0x40,al

doneInterrupt8:
  mov al,0x20
  out 0x20,al
  pop ax
  iret


; Returns the CGA to normal mode
stopISAV:
  cli
  xor ax,ax
  mov ds,ax
  mov ax,[cs:originalInterrupt8]
  mov [8*4],ax
  mov ax,[cs:originalInterrupt8+2]
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

  cmp bl,int8_isav0  ; Warning expected here, doing the obvious "& 0xff" turns it into an error.
  je .done
  cmp ax,76*3
  jl .done

  ; We've got plenty of time until the next IRQ0, but other interrupts could happen in the meantime.
  cli

  mov dx,0x3d4
  mov ax,0x4004
  out dx,ax
  mov ax,0x0105
  add ah,[activePage]
  out dx,ax
  mov byte[longField],1
  mov byte[needLongField],0

  sti

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
  cmp ax,42*76 + 21  ; Round down for safety

  ; Interrupt happens on scanline 242 cycle -1
  ; Safe point is scanline 199 cycle 54
  ; (242*76 + n/12 - 1) - (199*76 + 53 + 4/12)  = 3123 = 42*76 + 22 + (n-4)/12

.done:
  ret
.safe:
  stc
  ret


waitForSafeToDraw:
  call safeToDraw
  jnc waitForSafeToDraw
  ret

cppData:

