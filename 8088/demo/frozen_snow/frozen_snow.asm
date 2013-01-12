  %include "../defaults_bin.asm"

  lockstep

  initCGA 0x09

  mov ax,cs
  mov ds,ax

  push es

  ; Fill visible video memory with spaces
  mov cx,80*25
  mov ax,0x0720
  xor di,di
  rep stosw

  push di

  mov si,snowRoutine
  mov cx,snowRoutineEnd - snowRoutine
  rep movsb

  mov cl,1
  mov al,0x3f
  mov dx,0x03d

  retf

snowRoutine:
  %rep 261
    times 24 nop
    mul cl
  %endrep


;  28.5 per line

;  17.5 on last line


  mov al,0x07
  times 6 nop
;  times 7456 nop
  mul cl
  times 9
  nop
  mov
  jmp snowRoutine
snowRoutineEnd:
