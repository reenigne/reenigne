cpu 8086
org 0x100

  pop ax            ; 58
  xor ax,[bp+12]    ; 33 46 xx
  stosb             ; AA
  inc di            ; 47
  mov al,ah         ; 88 E0
  stosb             ; AA
  inc di            ; 47

  add sp,10         ; 83 C4 xx
  add bp,10         ; 83 C4 xx


To remove both bytes, replace "xor ax,[bp+12]" with "mov ax,0"


