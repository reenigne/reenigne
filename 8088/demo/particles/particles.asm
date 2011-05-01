start:
  ; Compute code + data segment
  mov ax,cs
  add ax,01000
  mov ds,ax
  push ax       ; push
  mov ax,8000
  push ax

  ; TODO: set up code and data

  retf


  mov di,9999      ; 3 0 12  4
  stosb            ; 1 1  8 11
  mov bx,[di]      ; 2 2 16 18
  es: mov [bx],ah  ; 3 1 16 16
  mov [8888],bx    ; 4 2 24 19   ; 8888 = address of earlier 9999

