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

slop equ 0

  mov ax,cs
  mov ds,ax
  cli
  mov ss,ax
  mov sp,0xfffe
  sti

;  in al,0x61
;  or al,0x80
;  mov [port61high+1],al
;  and al,0x7f
;  mov [port61low+1],al



  ; Clear screen
  mov ax,0xb800
  mov es,ax
  mov ax,0x100
  xor di,di
  mov cx,80*50
loopTop:
  stosw
  inc ax
  loop loopTop

  mov di,(49*80 + 79)*2 + 1
  std
  mov al,7
  stosb
  dec di
  stosb
  dec di
  stosb
  dec di
  stosb
  dec di
  stosb

  call startISAV

;  xor cx,cx
;delay:
;  loop delay
;  cli
;  xor ax,ax
;  mov ds,ax
;  mov ax,[cs:originalInterrupt8]
;  mov [8*4],ax
;  mov ax,[cs:originalInterrupt8+2]
;  mov [8*4+2],ax
;  mov al,0x34
;  out 0x43,al
;  mov al,0
;  out 0x40,al
;  out 0x40,al
;  sti
;  mov ax,cs
;  mov ds,ax

;  mov al,0xfe
;  out 0x21,al


mainLoop:
;  mov dx,0x3d9
;  mov al,[palette]
;  and al,7
;  out dx,al
;  inc byte[palette]

  mov ax,[delayTotal]
;  mov ax,[delayPCycles]
  mov bx,10
  mov di,(49*80 + 79)*2

  xor dx,dx
  idiv bx
  add dl,'0'
  push ax
  mov al,dl
  stosb
  dec di
  pop ax

  xor dx,dx
  idiv bx
  add dl,'0'
  push ax
  mov al,dl
  stosb
  dec di
  pop ax

  xor dx,dx
  idiv bx
  add dl,'0'
  push ax
  mov al,dl
  stosb
  dec di
  pop ax

  xor dx,dx
  idiv bx
  add dl,'0'
  push ax
  mov al,dl
  stosb
  dec di
  pop ax

  add al,'0'
  stosb

;  mov dx,0x3d9
;  mov al,2
;  out dx,al


;  in al,0x20
;  and al,2    ; Check for IRR bit 1 (IRQ 1) high
;  jz mainLoop2 ; noKey
;  ; Read the keyboard byte and store it
;  in al,0x60
;  xchg ax,bx
;  ; Acknowledge the previous byte
;port61high:
;  mov al,0xcf
;  out 0x61,al
;port61low:
;  mov al,0x4f
;  out 0x61,al
;  xchg ax,bx
;
;  cmp al,44  ;'z'
;  jne notMinus
;  mov word[delayPCycles],-1
;notMinus:
;  cmp al,45  ;'x'
;  jne notPlus
;  mov word[delayPCycles],1
;notPlus:
;  cmp al,30  ;'a'
;  jne notMinus10
;  mov word[delayPCycles],-10
;notMinus10:
;  cmp al,31  ;'s'
;  jne notPlus10
;  mov word[delayPCycles],10
;notPlus10:
;  cmp al,1   ;ESC
;  jne mainLoop2


  mov ah,1
  int 0x16
  jz mainLoop2

;  xor byte[palette],8

  mov ah,0
  int 0x16
  cmp al,'+'
  jne notPlus
  mov word[delayPCycles],1
notPlus:
  cmp al,'-'
  jne notMinus
  mov word[delayPCycles],-1
notMinus:
  cmp al,'z'
  jne notMinus10
  mov word[delayPCycles],-10
notMinus10:
  cmp al,'x'
  jne notPlus10
  mov word[delayPCycles],10
notPlus10:
  cmp al,'a'
  jne notMinus100
  mov word[delayPCycles],-100
notMinus100:
  cmp al,'s'
  jne notPlus100
  mov word[delayPCycles],100
notPlus100:
  cmp al,'q'
  jne notMinus1000
  mov word[delayPCycles],-1000
notMinus1000:
  cmp al,'w'
  jne notPlus1000
  mov word[delayPCycles],1000
notPlus1000:
  cmp al,27
  jne mainLoop2

finishLoop:
  call stopISAV

  int 0x67
;  mov ax,0x4c00
;  int 0x21
  ret
mainLoop2:
  jmp mainLoop


palette:
  db 0
delayPCycles:
  dw 0
delayTotal:
  dw 224*76 + 27

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
  mov al,0x09
  out dx,al

  ; Palette                                             00
  ;      1 +OVERSCAN B                                   0
  ;      2 +OVERSCAN G                                   2
  ;      4 +OVERSCAN R                                   4
  ;      8 +OVERSCAN I                                   0
  ;   0x10 +BACKGROUND I                                 0
  ;   0x20 +COLOR SEL                                    0
  inc dx
  mov al,0
  out dx,al

  mov dl,0xd4

  ;   0xff Horizontal Total                             71
  mov ax,0x7100
  out dx,ax

  ;   0xff Horizontal Displayed                         50
  mov ax,0x5001
  out dx,ax

  ;   0xff Horizontal Sync Position                     5a
  mov ax,0x5a02
  out dx,ax

  ;   0x0f Horizontal Sync Width                        0a
  mov ax,0x0f03 ;0x0a03
  out dx,ax

  ;   0x7f Vertical Total                               01  vertical total = 2 rows
  mov ax,0x0104
  out dx,ax

  ;   0x1f Vertical Total Adjust                        00  vertical total adjust = 0
  mov ax,0x0005
  out dx,ax

  ;   0x7f Vertical Displayed                           01  vertical displayed = 1
  mov ax,0x0106
  out dx,ax

  ;   0x7f Vertical Sync Position                       1c  vertical sync position = 28 rows
  mov ax,0x1c07
  out dx,ax

  ;   0x03 Interlace Mode                               00   0 = non interlaced, 1 = interlace sync, 3 = interlace sync and video
  mov ax,0x0008
  out dx,ax

  ;   0x1f Max Scan Line Address                        00  scanlines per row = 1
  mov ax,0x0009
  out dx,ax

  ; Cursor Start                                        06
  ;   0x1f Cursor Start                                  6
  ;   0x60 Cursor Mode                                   0
  mov ax,0x060a
  out dx,ax

  ;   0x1f Cursor End                                   08
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


  ; Step 0 - don't do anything (we've just completed wait for CRTC stabilization)
int8_oe0:
  mov word[0x20],int8_oe1

;   dec dx
;   mov al,1
;   out dx,al
;   inc dx

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

;   dec dx
;   mov al,2
;   out dx,al
;   inc dx

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

;   dec dx
;   mov al,3
;   out dx,al
;   inc dx

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

;   dec dx
;   mov al,4
;   out dx,al
;   inc dx

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

;   dec dx
;   mov al,5
;   out dx,al
;   inc dx

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

;   dec dx
;   mov al,6
;   out dx,al
;   inc dx

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

;   mov dx,0x3d9
;   mov al,7
;   out dx,al

  mov al,0x20
  out 0x20,al
  iret


  ; Step 7. This occurs on scanline 3 (one timer cycle before the active area starts). The next interrupt will be on scanline 0.
int8_oe7:
  mov word[0x20],int8_oe8

;   mov dx,0x3d9
;   mov al,8
;   out dx,al

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

;   mov dx,0x3d9
;   mov al,9
;   out dx,al

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
  mov ax,0x0309    ; Scanlines per row = 4  (2 on each field)
  out dx,ax
  mov ax,0x0106    ; Vertical displayed = 1 row (actually 2)
  out dx,ax
  mov ax,0x0304    ; Vertical total = 4 rows
  out dx,ax
  mov ax,0x0305    ; Vertical total adjust = 3
  out dx,ax

;   mov dl,0xd9
;   mov al,10
;   out dx,al

  pop dx

  mov al,(223*76 + 27 - slop) & 0xff
  out 0x40,al
  mov al,(223*76 + 27 - slop) >> 8
  out 0x40,al

  mov al,[cs:originalIMR]
  out 0x21,al

  push ds
  xor ax,ax
  mov ds,ax
  mov word[0x20],int8_oe10
  pop ds

  mov al,0x20
  out 0x20,al
  pop ax
  iret


  ; Step 10 - set up CRTC registers for full screen - scanline 0
int8_oe10:
  push ax
  push dx
  mov dx,0x3d4
  mov ax,0x0709  ; Scanlines per row = 8 (4 on each field)
  out dx,ax
  mov ax,0x1906  ; Vertical displayed = 25 rows (actually 50)
  out dx,ax
  mov ax,0x1f04  ; Vertical total = 32 rows (actually 64)
  out dx,ax
  mov ax,0x0605  ; Vertical total adjust = 6
  out dx,ax

;   mov dl,0xd9
;   mov al,10
;   out dx,al

  pop dx

  mov al,(525*76) & 0xff
  out 0x40,al
  mov al,(525*76) >> 8
  out 0x40,al

  push ds
  xor ax,ax
  mov ds,ax
  mov word[0x20],int8_isav
  pop ds


  mov al,0x20
  out 0x20,al
  pop ax
  iret


  ; Final - scanline 224
int8_isav:
  push ax
  push dx
  push bx

  mov dx,0x3d4
  mov ax,0x2102  ; Horizontal sync position early
  out dx,ax

  mov dx,0x40
  mov bx,524*76 + slop
.loopTop1:
  mov al,0x04
  out 0x43,al
  in al,dx
  mov ah,al
  in al,dx
  xchg al,ah
  cmp ax,bx
  jae .loopTop1

  mov dx,0x3d4
  mov ax,0x5a02  ; Horizontal sync position normal
  out dx,ax

  mov dx,0x40
  mov bx,522*76 + slop
.loopTop2:
  mov al,0x04
  out 0x43,al
  in al,dx
  mov ah,al
  in al,dx
  xchg al,ah
  cmp ax,bx
  jae .loopTop2

  mov dx,0x3d4
  mov ax,0x2102  ; Horizontal sync position early
  out dx,ax

  mov dx,0x40
  mov bx,521*76 + slop
.loopTop3:
  mov al,0x04
  out 0x43,al
  in al,dx
  mov ah,al
  in al,dx
  xchg al,ah
  cmp ax,bx
  jae .loopTop3

  mov dx,0x3d4
  mov ax,0x5a02  ; Horizontal sync position normal
  out dx,ax

  pop bx
  pop dx


  mov ax,[cs:delayPCycles]
  add [cs:delayTotal],ax
  mov word[cs:delayPCycles],0
  add ax,525*76
  out 0x40,al
  mov al,ah
  out 0x40,al


  add word[cs:timerCount],76*525
  jnc doneInterrupt8
  pop ax
  jmp far [cs:originalInterrupt8]

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

