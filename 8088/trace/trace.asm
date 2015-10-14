  %include "../defaults_bin.asm"

FASTSAMPLING EQU 0     ; Set to one to sample at 14.318MHz. Default is 4.77MHz.
LENGTH       EQU 2048  ; Number of samples to capture.

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
;  outputbit dl,1
;  outputbit dl,2
;  outputbit dl,4
;  outputbit dl,8
;  outputbit dl,0x10
;  outputbit dl,0x20
;  outputbit dl,0x40
;  outputbit dl,0x80
  mov dx,LENGTH
  dec dx
;  outputbit dl,1
;  outputbit dl,2
;  outputbit dl,4
;  outputbit dl,8
;  outputbit dl,0x10
;  outputbit dl,0x20
;  outputbit dl,0x40
;  outputbit dl,0x80
;  outputbit dh,1
;  outputbit dh,2
;  outputbit dh,4
;  outputbit dh,8
;  outputbit dh,0x10
;  outputbit dh,0x20
;  outputbit dh,0x40
;  outputbit dh,0x80
  outputByte
  outputByte
  mov dx,18780+204
  outputByte
  outputByte

  mov al,0
  out 0x0c,al  ; Clear first/last flip-flop
  out 0x00,al  ; Output DMA offset low
  out 0x00,al  ; Output DMA offset high
;  safeRefreshOn


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
  refreshOn



  call testRoutine

  mov cx,25*LENGTH
flushLoop2:
  loop flushLoop2
;  mov cx,31*LENGTH
;flushLoop3:
;  loop flushLoop3

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

  mov cx,8
  xor ax,ax
  mov ds,ax
  mov bp,ax
  mov si,ax
  mov di,ax
  mov dx,ax
v:
  times 15 nop
mixPatch:
  add bp,9999  ; 0
  mov bx,bp    ; 4
  mov bl,99    ; 6
  mov al,[bx]  ; 8
  add si,9999  ; 10
  mov bx,si    ; 14
  mov bl,99    ; 16
  add al,[bx]  ; 18
  add di,9999  ; 20
  mov bx,di    ; 24
  mov bl,99    ; 26
  add al,[bx]  ; 28
  add dx,9999  ; 30
  mov bx,dx    ; 34
  mov bl,99    ; 36
  add al,[bx]
  out 0x42,al  ; Output total to speaker
loopPatch:
  loop v


  ; Append the code to test - it should end with a "ret"
  ret