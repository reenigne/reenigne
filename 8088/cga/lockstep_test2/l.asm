  %include "../../defaults_bin.asm"

  cli
  mov ax,cs
  mov ss,ax
  mov sp,stackEnd + 0x100
  sti

  mov bp,0

top:

  in al,0x61
  or al,3
  out 0x61,al

  mov al,TIMER2 | BOTH | MODE2 | BINARY
  out 0x43,al
  mov dx,0x42
  mov al,0
  out dx,al
  out dx,al


;  xor ax,ax
;  mov ds,ax
;  cli
;  mov ax,[8*4]
;  mov [cs:oldInterrupt8],ax
;  mov ax,[8*4+2]
;  mov [cs:oldInterrupt8+2],ax
;  mov word[8*4],interrupt8
;  mov [8*4+2],cs
;  sti
;
;  hlt
;  hlt
;  mov ax,[cs:oldInterrupt8]
;  mov [8*4],ax
;  mov ax,[cs:oldInterrupt8+2]
;  mov [8*4+2],ax


;  in al,0x61
;  or al,3
;  out 0x61,al

;  mov al,TIMER2 | BOTH | MODE2 | BINARY
;  out 0x43,al
;  mov dx,0x42
;  mov al,0
;  out dx,al
;  out dx,al


  mov dx,0x03d8
  mov al,0x0a
  out dx,al

  ; Set up CRTC for 1 character by 2 scanline "frame". This gives us 2 lchars
  ; per frame.
  mov dx,0x3d4
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

  mov cx,256
  xor ax,ax
  mov ds,ax
  mov si,ax
  cld
  cli

  ; Increase refresh frequency to ensure all DRAM is refreshed before turning
  ; off refresh.
  mov al,TIMER1 | LSB | MODE2 | BINARY
  out 0x43,al
  mov al,2
  out 0x41,al  ; Timer 1 rate

  ; Delay for enough time to refresh 512 columns
  rep lodsw

  ; We now have about 1.5ms during which refresh can be off
  refreshOff

  ; Set "lodsb" destination to be CGA memory
  mov ax,0xb800
  mov es,ax
  mov ds,ax
  mov di,0x3ffc
  mov si,di
  mov ax,0x0303  ; Found by trial and error
  stosw
  stosw

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

  nop
  nop

  ; To get the CRTC into lockstep with the CGA and CPU, we need to figure out
  ; which of the two possible CRTC states we're in and switch states if we're
  ; in the wrong one by waiting for an odd number of lchars more in one code
  ; path than in the other. To keep CGA and CPU in lockstep, we also need both
  ; code paths to take the same time mod 3 lchars, so we wait 3 lchars more on
  ; one code path than on the other.
  in al,dx
  and al,1
  dec ax
  mul cl
  mul cl
  jmp $+2

  in al,0x61
  or al,3
  out 0x61,al

  mov ax,0x7000
  mov es,ax
  mov ds,ax
  xor di,di

  mov al,TIMER2 | BOTH | MODE2 | BINARY
  out 0x43,al
  mov dx,0x42
  mov al,0
  out dx,al
  out dx,al
%rep 12
  readPIT16 2
  stosw

%endrep


 ; mov bl,al

  ; Increase refresh frequency to ensure all DRAM is refreshed before turning
  ; off refresh.
  mov al,TIMER1 | LSB | MODE2 | BINARY
  out 0x43,al
  mov al,2
  out 0x41,al  ; Timer 1 rate

  ; Delay for enough time to refresh 512 columns
  mov cx,256
  rep lodsw


  refreshOn

  sti

  mov ax,0x7000
  mov ds,ax
  mov cx,11
  xor si,si
  lodsw
  xchg ax,bx
outputLoopTop:
;  lodsb
;  sub bl,al
;  xchg ax,bx
;  add al,'0'
  lodsw
  sub bx,ax
  xchg ax,bx
  add al,'0'

  outputCharacter
  loop outputLoopTop

  mov al,10
  outputCharacter

  jmp top



  disconnect


  ; Go into +HRES
;  mov dx,0x3d8
;  mov al,9
;  out dx,al

;  refreshOff
;
;  mov dx,0x3da
;  mov ax,0x7000
;  mov es,ax
;  xor di,di
;%rep 128
;  in al,dx
;  in al,dx
;  in al,dx
;  nop
;  stosb
;%endrep
;
;  refreshOn


  mov ax,0x7000
  mov ds,ax
;  xor si,si
;  mov cx,128
;  and bl,1
;loopTop:
;  lodsb
;  test al,1
;  jz zero
;  or bl,2
;  jmp done
;zero:
;  or bl,4
;done:
;  loop loopTop

  xor bl,bl
  mov al, byte[127]
  and al,1
  or bl,al
  mov al, byte[119]
  shl al,1
  and al,2
  or bl,al
  mov al, byte[111]
  shl al,1
  shl al,1
  and al,4
  or bl,al
  mov al, byte[103]
  shl al,1
  shl al,1
  shl al,1
  and al,8
  or bl,al


  ; Compare phases
  dec dx
  mov al,bl
  out dx,al


  refreshOff

  mov ax,3
  int 0x10

  ; Wait for a random amount of time
  lea si,[bp+table]
  mov ax,cs
  mov ds,ax
  mov cl,1
  lodsb
  mul cl
  lodsb
  mul cl
  lodsb
  mul cl
  lodsb
  mul cl
  add bp,[cs:delta]
  add word[cs:delta],4
  and bp,31*4




  jmp top

interrupt8:
  push ax
  mov al,0x20
  out 0x20,al
  pop ax
  iret

oldInterrupt8: dw 0, 0

delta: dw 0

table:
  db   0,  0,  0,  1
  db   0,  0,  1,  1
  db   0,  1,  1,  1
  db   1,  1,  1,  1
  db   1,  1,  1,  3
  db   1,  1,  3,  3
  db   1,  3,  3,  3
  db   3,  3,  3,  3
  db   3,  3,  3,  7
  db   3,  3,  7,  7
  db   3,  7,  7,  7
  db   7,  7,  7,  7
  db   7,  7,  7, 15
  db   7,  7, 15, 15
  db   7, 15, 15, 15
  db  15, 15, 15, 15
  db  15, 15, 15, 31
  db  15, 15, 31, 31
  db  15, 31, 31, 31
  db  31, 31, 31, 31
  db  31, 31, 31, 63
  db  31, 31, 63, 63
  db  31, 63, 63, 63
  db  63, 63, 63, 63
  db  63, 63, 63,127
  db  63, 63,127,127
  db  63,127,127,127
  db 127,127,127,127
  db 127,127,127,255
  db 127,127,255,255
  db 127,255,255,255
  db 255,255,255,255

stackEnd:
