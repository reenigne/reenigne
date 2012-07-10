cpu 8086

%macro disconnect 0
  mov al,26
  int 0x62  ; Write a ^Z character to tell the "run" program to finish
%endmacro

%macro refreshOff 0
  mov al,0x60  ; Timer 1, write LSB, mode 0, binary
  out 0x43,al
  mov al,0x01  ; Count = 0x0001 so we'll stop almost immediately
  out 0x41,al
%endmacro

%macro lockstep 0
  cli

  refreshOff

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
  jz %%shortPath
  times 2 nop
  jmp $+2
%%shortPath:

%endmacro

