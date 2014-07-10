  cpu 8086
  ;org 0x100
  org 0

  ; Set basic video mode (40-column text)
  mov ax,1
  int 0x10

  ; Put a nice picture in CGA RAM
  mov si,imageData
  mov ax,cs
  mov ds,ax
  mov ax,0xb800
  mov es,ax
  xor di,di
  cld
  mov cx,8000
  rep movsw

  ; Wait for vertical sync before reprogramming for consistency
  mov dx,0x3da
waitForVerticalSync:
  in al,dx
  test al,8
  jz waitForVerticalSync

  ; Disable flashing
  mov dl,0xd8
  mov al,0x08
  out dx,al

  ; Initial CRTC reprogramming
  mov dl,0xd4
  mov ax,0x0009 ; Scanlines per row = 1
  out dx,ax
  mov ax,0x7007 ; Vertical sync position = 112
  out dx,ax

  mov ax,0x0503 ; Horizontal sync width = 5
  out dx,ax


; We can't have more than 127 active scanlines in the top half or we'll be interrupted by vsync somewhere.
; The total number of scanlines (262) is greater than twice the maximum number of active scanlines (127) so we will need to use vertical total adjust
; Active scanlines   0..199
; Lower blank      200..223
; Vsync            224..226
; Upper blank      227..261
; We can't use vertical total adjust for the top half, so this is one of our adjusted registers
; Suppose we use a vertical total adjust of 30. Then we have 232 normal scanlines.
; If we have 100 scanlines in the top half, then the bottom half is too small, so this is one of our adjusted registers

; If we keep the vertical sync position at X and vary the vertical total between Y (<X) and 232-Y we have X = 224-Y (e.g. Y=108, Y'=124, X=116)
; Let's try to keep Start Address L as 0. Then we need 80*Y = 256*N so N must be a multiple of 5 and Y must be a multiple of 16 so Y=112, Y'=120, X=112 should work
; Y=96 is too small
; Y=112 gives us X=Y. If this is a problem let's increase the sync pulse position and move the picture up by 1 scanline
; Y=128 is too large


  ; Set up the timer interrupt for twice per frame
  xor ax,ax
  mov ds,ax
  mov di,8*4
  mov ax,[di]
  mov [cs:oldInterrupt8],ax
  mov ax,[di+2]
  mov [cs:oldInterrupt8+2],ax

  cli
  mov word[di],interrupt8
  mov [di+2],cs
  sti

  mov al,0x34 ; Timer 0, both bytes, mode 2, binary
  out 0x43,al
  mov al,0xe4
  out 0x40,al
  mov al,0x26
  out 0x40,al ; Count = 0x26e4 = 9956 = 19912/2



  ; Do some busywork so we can estimate how much time our interrupts are taking
  mov bx,0
  mov ax,[0x46c]
loopTop:
  mov di,16000    ; 3
  mov cx,96       ; 3
  rep movsw       ; 9.6*96
  mov cx,96       ; 3
  rep stosw       ; 6.9*96
  inc bx          ; 1
  mov dx,[0x46c]  ; 6
  sub dx,ax       ; 2
  cmp dx,1000     ; 4
  jne loopTop     ; 2        ; 25.4+1579.9 = 1605.3

  mov ax,bx
  int 0x63


  foo:
  jmp foo



  ; Restore screen
  mov ax,3
  int 0x10

  mov al,0x34 ; Timer 0, both bytes, mode 2, binary
  out 0x43,al
  mov al,0
  out 0x40,al
  out 0x40,al

  mov di,8*4
  cli
  mov ax,[oldInterrupt8]
  mov [di],ax
  mov ax,[oldInterrupt8+2]
  mov [di+2],ax
  sti



  ; Exit
;  int 0x67
;  ret
  hlt


interrupt8:
  push ax
  push dx

  mov dx,0x3d4

  xor byte[cs:whichHalf],1
  jz .lowerHalf

  mov ax,0x6b04 ; Vertical Total = 108
  out dx,ax
  mov ax,0x0005 ; Vertical Total Adjust = 0
  out dx,ax
  mov ax,0x100c ; Start Address = 40*108 = 0x10e0
  out dx,ax
  mov ax,0xe00d
  out dx,ax
  mov ax,0x6c06 ; Vertical Displayed = 108
  out dx,ax

  jmp .doneHalf
.lowerHalf:

  mov ax,0x7b04 ; Vertical Total = 124
  out dx,ax
  mov ax,0x1e05 ; Vertical Total Adjust = 30
  out dx,ax
  mov ax,0x000c ; Start Address = 0
  out dx,ax
  inc ax
  out dx,ax
  mov ax,0x5c06 ; Vertical Displayed = 92
  out dx,ax

.doneHalf:
  pop dx

  add word[cs:timerCount],0x26e4
  jc .doOld
  mov al,0x20
  out 0x20,al
  pop ax
  iret
.doOld:
  pop ax
  jmp far [cs:oldInterrupt8]

timerCount: dw 0
whichHalf: db 0
oldInterrupt8: dw 0,0

imageData:

