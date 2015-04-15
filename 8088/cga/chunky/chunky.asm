  %include "../../defaults_bin.asm"

  initCGA 8, 0, 2

;  mov ax,0x7f01
;  mov dx,0x3d4
;  out dx,ax

  mov ax,0xb800
  mov es,ax

  mov ax,0x07b0
  mov cx,10
  mov di,0
  cld
  rep stosw

  mov ax,0x0fb0
  mov cx,10
  mov di,80
  cld
  rep stosw

frameLoop:
  mov dx,0x3da
  waitForVerticalSync
  waitForNoVerticalSync
  waitForDisplayEnable

  mov dx,0x3d4

  mov ax,0x7f01
  out dx,ax
  mov ax,0x0009
  out dx,ax

  times 76 nop
  times 76 nop
  times 76 nop
  times 76 nop
  times 76 nop
  times 76 nop
  times 76 nop
  times 76 nop

  mov ax,0x2801
  mov dx,0x3d4
  out dx,ax

  times 76 nop
  times 76 nop
  times 76 nop
  times 76 nop
  times 76 nop
  times 76 nop
  times 76 nop
  times 76 nop

  mov ax,0x0109
  out dx,ax

  add cx,40

  jmp frameLoop


;  mov ax,0x0c03    ; No color burst (we'll use the border as our burst)
;  out dx,ax
;
;  mov ax,0xb800
;  mov es,ax
;  cld
;  mov ax,0x00db
;  xor di,di
;  mov cx,25
;rowLoop:
;  push cx
;  mov cx,16
;barLoop:
;  push cx
;  mov cx,5
;  rep stosw
;  pop cx
;  inc ah
;  loop barLoop
;  pop cx
;  loop rowLoop
;
;frameLoop:
;  mov dx,0x3da
;  waitForVerticalSync
;  waitForNoVerticalSync
;
;  mov cx,100
;topLinesLoop:
;  waitForDisplayEnable
;  waitForDisplayDisable
;  loop topLinesLoop
;
;  mov cx,16
;  mov ah,0
;glitchLinesLoop:
;  waitForDisplayEnable
;  mov al,ah
;  dec dx
;  out dx,al
;  inc dx
;  waitForDisplayDisable
;  waitForDisplayEnable
;  mov al,6
;  dec dx
;  out dx,al
;  inc dx
;  waitForDisplayDisable
;  inc ah
;  push cx
;  mov cx,4
;glitchGapLoop:
;  waitForDisplayEnable
;  waitForDisplayDisable
;  loop glitchGapLoop
;  pop cx
;  loop glitchLinesLoop
;  jmp frameLoop


  hlt

