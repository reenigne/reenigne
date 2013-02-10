  %include "../../defaults_bin.asm"

  initCGA 0x08

  mov ax,0xb800
  mov es,ax
  mov di,0
  mov ax,0x072d  ; '-'
  stosw


  mov cx,0x0930
  mov bl,0


  mov dx,0x3da
loopTop3:
  waitForVerticalSync
  waitForNoVerticalSync

  mov ax,cx
  stosw

  inc cl
  cmp cl,'9'
  jle loopTop3

  mov cl,'0'
  inc bl
  cmp bl,30
  jne loopTop3


  int 0x60

  mov cx,0x0730
  mov bl,0

  mov dx,0x3da
loopTop:
  waitForVerticalSync
  waitForNoVerticalSync

  mov ax,cx
  stosw

  inc cl
  cmp cl,'9'
  jle loopTop

  mov cl,'0'
  inc bl
  cmp bl,30
  jne loopTop


;  initCGA 0x08
;
;  mov ax,0xb800
;  mov es,ax
;  mov di,0
;  mov ax,0x072d  ; '-'
;  stosw

  int 0x60

  mov al,'-'
  int 0x65
  mov al,'-'
  int 0x65
  mov al,'-'
  int 0x65
  mov al,'-'
  int 0x65
  mov al,'-'
  int 0x65

  mov cx,0x0c30
  mov bl,0

  mov dx,0x3da
loopTop2:
  waitForVerticalSync
  waitForNoVerticalSync

  mov ax,cx
  stosw

  inc cl
  cmp cl,'9'
  jle loopTop2

  mov cl,'0'
  inc bl
  cmp bl,30
  jne loopTop2



  int 0x67



