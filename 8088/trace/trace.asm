  %include "../defaults_bin.asm"

FASTSAMPLING EQU 0     ; Set to one to sample at 14.318MHz. Default is 4.77MHz.
LENGTH       EQU 2048  ; Number of samples to capture.
REFRESH      EQU 19   ; Refresh period in cycles, or 0 to disable

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
  mov dx,18996    ;65534 ;
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

  mov dx,0x3d4
  mov cx,0xffff
  xor si,si
  mov bp,0x5001
  mov di,0x5a02

%macro innerLoop 0
  mov ax,0x0101  ; b  Horizontal_displayed  right
  out dx,ax

  mov ax,0x1900  ; a  Horizontal_total      right
  out dx,ax

  xchg ax,bp
  ;mov ax,0x5001  ; d  Horizontal_displayed  left
  out dx,ax
  xchg ax,bp

  xchg ax,di
  ;mov ax,0x5a02  ; c  Horizontal_sync       left
  out dx,ax
  xchg ax,di

  mov ax,0x5700  ; e  Horizontal_total      left
  out dx,ax

  mov ax,0x0202  ; f  Horizontal_sync       right
  out dx,ax

  mov ah,bh
  mov al,0x0c
  out dx,ax
  mov ah,bl
  inc ax
  out dx,ax

  mov al,bl
  mov dl,0xd9
  out dx,al
  mov dl,0xd4
  inc bx

  nop
  nop

  loop %%loopTop
%%loopTop:
%endmacro

%rep 5
  innerLoop
%endrep

  ret








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
