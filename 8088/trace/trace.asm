  %include "../defaults_bin.asm"

FASTSAMPLING EQU 0     ; Set to one to sample at 14.318MHz. Default is 4.77MHz.
LENGTH       EQU 2048  ; Number of samples to capture.
REFRESH      EQU 0;19   ; Refresh period in cycles, or 0 to disable

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
  mov dx,18996 ;+ 492*3   ;65534 ;
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

  mov cx,25*LENGTH
flushLoop2:
  loop flushLoop2

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
;  lockstep

  mov dx,0x03d8
  mov al,0
  out dx,al

  ; Set up CRTC for 1 character by 2 scanline "frame". This gives us 2 lchars
  ; per frame.
  mov dx,0x3d4
  ;   0xff Horizontal Total
  mov ax,0x0100
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

  ; 256 lchars (horizonta) + 256 lchars (vertical) = 2731 CPU cycles = 114 iterations of "rep lodsw"

  cli
  cld

  xor ax,ax
  mov ds,ax
  mov si,ax

  ; Delay for enough time to refresh 512 columns
  mov cx,16

  ; Increase refresh frequency to ensure all DRAM is refreshed before turning
  ; off refresh.
  mov al,TIMER1 | LSB | MODE2 | BINARY
  out 0x43,al
  mov al,2
  out 0x41,al  ; Timer 1 rate

  ; Each iteration takes 24 cycles and refreshes 2 locations.
  rep lodsw

  mov al,TIMER1 | LSB | MODE0 | BINARY
  out 0x43,al
  mov al,0x01  ; Count = 0x0001 so we'll stop almost immediately
  out 0x41,al

  ; Set "lodsb" destination to be CGA memory
  mov ax,0xb800
  mov es,ax
  mov ds,ax
  mov di,0x3ffc
  mov si,di
  mov ax,0x0303  ; Found by trial and error
  stosw
  mov al,0x00
  stosb

  mov dl,0xda

  ; Set argument for MUL
  mov cl,1

  ; Go into CGA/CPU lockstep.
  jmp $+2
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

  ; To get the CRTC into lockstep with the CGA and CPU, we need to figure out
  ; which of the four possible CRTC states we're in and switch states (by
  ; waiting for 2*N+1 lchars) until we're in a single state. We do this by
  ; waiting for the display enable bit to go low in a loop that takes 144
  ; cycles per iteration. This will loop at most 3 times.
  mov ax,1
  test al,1
  jnz .loopTop1   ; Always jump to clear the prefetch queue.
.loopTop1:
  mov al,1
  div cl
  times 6 nop
  in al,dx
  test al,1
  jnz .loopTop1


  ret


  mov [cs:savedSP],sp

  xor ax,ax
  mov ds,ax
  mov word[0x20],irq0a
  writePIT16 0, 2, 2

  mov dx,0x3d9
  in al,dx
  test al,1
  jnz .loop1
.loop1:
  in al,dx
  test al,1
  jnz .loop2
.loop2:
  in al,dx
  test al,1
  jz .loop3
.loop3:
  writePIT16 0, 2, 31
  sti
  hlt
irq0a:
  mov al,75
  out 0x40,al
  mov al,0
  out 0x40,al
  mov word[0x20],irq0b
  mov al,0x20
  out 0x20,al
  sti
  hlt
irq0b:
  in al,dx

  mov al,0x20
  out 0x20,al
  writePIT16 0, 2, 0
  mov word[0x20],irq0

  mov sp,[cs:savedSP]
  ret


  mov al,TIMER1 | LSB | MODE2 | BINARY
  out 0x43,al
  mov al,REFRESH
  out 0x41,al  ; Timer 1 rate

  xor ax,ax
  mov ds,ax
  mov word[0x20],irq0setup
  writePIT16 0, 2, 2         ; Ensure we have a pending IRQ0
  writePIT16 0, 2, 40
  sti
  hlt     ; Should never be hit
irq0setup:
  mov al,0x20
  out 0x20,al
  mov word[0x20],irq0test
  sti
  hlt

irq0test:
  mov ax,cs
  mov ds,ax
  mov ss,ax
  mov sp,1234
  mov dx,0x3d4
  mov bp,0x5001
  mov di,0x1900
  mov ax,0x5702
  mov si,12345
  mov bx,123
  mov es,ax

  ; Scanlines 0-199

%macro scanline 1
  mov al,0x00
  out dx,ax        ; e  Horizontal Total         left  0x5700  88
  mov ax,0x0202
  out dx,ax        ; f  Horizontal Sync Position right 0x0202   2

  pop cx
  mov al,0x0c
  mov ah,ch
  out dx,ax
  inc ax
  mov ah,cl
  out dx,ax

  lodsb
  out 0xe0,al

  %if %1 == -1
    mov ax,0x0104
    out dx,ax      ;    Vertical Total
    times 3 nop
  %elif %1 == 198
    mov ax,0x3f04
    out dx,ax      ;    Vertical Total                 0x3f04  64  (1 for scanlines -1 and 198, 62 for scanlines 199-260)
    times 3 nop
  %else
    mov al,[bx+si]
    mov dl,0xd9
    out dx,al
    mov dl,0xd4
  %endif

  mov ax,0x0101
  out dx,ax        ; b  Horizontal Displayed     right 0x0101   1
  xchg ax,di
  out dx,ax        ; a  Horizontal Total         right 0x1900  26
  xchg ax,di
  xchg ax,bp
  out dx,ax        ; d  Horizontal Displayed     left  0x5001  80
  xchg ax,bp
  mov ax,es
  out dx,ax        ; c  Horizontal Sync Position left  0x5702  88
%endmacro
%assign i 0
;%rep 200
;  scanline i
;  %assign i i+1
;%endrep
  scanline -1
  scanline 0
  scanline 1
  scanline 2
  scanline 3
  scanline 198

  ; Scanline 199

  mov ax,0x7100
  out dx,ax        ; e  Horizontal Total         left  0x7100 114
  mov ax,0x5a02
  out dx,ax        ; f  Horizontal Sync Position right 0x5a02  90

  lodsb
  out 0xe0,al


  mov sp,[cs:savedSP]

  xor ax,ax
  mov ds,ax
  mov word[0x20],irq0
  writePIT16 0, 2, 0

  mov al,0x20
  out 0x20,al


  ret
savedSP:







  aaa
  aaa
  aaa
  aaa
  aaa
  aaa
  aaa
  aaa
  aaa
  aaa
  ret


  mov dx,0x3d9
%rep 10
  in al,dx
  test al,8
  jz $+2
%endrep
%rep 10
  in al,dx
  test al,8
  jnz $+2
%endrep



  mov di,data
  mov ax,cs
  mov es,ax
  mov dx,0x3da
  in al,dx
  stosb
  in al,dx
  stosb
  in al,dx
  stosb
  in al,dx
  stosb
  in al,dx
  stosb
  in al,dx
  stosb
  in al,dx
  stosb
  in al,dx
  stosb
  in al,dx
  stosb
  in al,dx
  stosb

;  readPIT16 0
;  stosw
;  readPIT16 0
;  stosw
;  readPIT16 0
;  stosw
;  readPIT16 0
;  stosw
;  readPIT16 0
;  stosw
;  ret




data:
