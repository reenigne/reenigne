  %include "../../defaults_bin.asm"

  initCGA 9, 6

  mov ax,0x0c03    ; No color burst (we'll use the border as our burst)
  out dx,ax

  mov ax,0xb800
  mov es,ax
  cld
  mov ax,0x00db
  xor di,di
  mov cx,25
rowLoop:
  push cx
  mov cx,16
barLoop:
  push cx
  mov cx,5
  rep stosw
  pop cx
  inc ah
  loop barLoop
  pop cx
  loop rowLoop

frameLoop:
  mov dx,0x3da
  waitForVerticalSync
  waitForNoVerticalSync

  mov cx,100
topLinesLoop:
  waitForDisplayEnable
  waitForDisplayDisable
  loop topLinesLoop

  mov cx,16
  mov ah,0
glitchLinesLoop:
  waitForDisplayEnable
  mov al,ah
  dec dx
  out dx,al
  inc dx
  waitForDisplayDisable
  waitForDisplayEnable
  mov al,6
  dec dx
  out dx,al
  inc dx
  waitForDisplayDisable
  inc ah
  push cx
  mov cx,4
glitchGapLoop:
  waitForDisplayEnable
  waitForDisplayDisable
  loop glitchGapLoop
  pop cx
  loop glitchLinesLoop
  jmp frameLoop


  hlt

