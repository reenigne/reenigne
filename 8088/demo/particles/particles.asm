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


; Linear Congruential Generation random number

randomSeed:
  dw 0,0

random:  ; Returns random number in range 0..16000 in dx. ~761 cycles
  ; randomSeed = randomSeed*0x15a4e35 + 1
  mov ax,w[randomSeed]      ; 3 2 20 14
  mov cx,04e35              ; 3 0 12  4
  mul cx                    ; 2 0  8  118-133
  mov di,ax                 ; 2 0  8  2
  mov bx,dx                 ; 2 0  8  2
  mov dx,015a               ; 3 0 12  4
  mul dx                    ; 2 0  8  118-133
  add bx,ax                 ; 2 0  8  3
  mov ax,w[randomSeed + 2]  ; 3 2 20 14
  mul cx                    ; 2 0  8  118-133
  add bx,ax                 ; 2 0  8  3
  inc di                    ; 1 0  4  2
  adc bx,0                  ; 3 0 12  4
  mov w[randomSeed],di      ; 3 2 20 19
  mov w[randomSeed + 2],bx  ; 3 2 20 19
  mov ax,bx                 ; 2 0  8  2
  xor dx,dx                 ; 2 0  8  3
  mov bx,16000              ; 3 0 12  4
  div bx                    ; 2 0  8  144-162
  ret                       ; 1 2 12 20


; Additive Lagged Fibonacci Generator random number

randomData:  ; TODO: init with random numbers from random
  dw 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
  dw 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
  dw 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
  dw 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
  dw 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0
randomIndex1:
  dw 0          ; TODO: Add a random number to randomIndex1 and randomIndex2, mod 55
randomIndex2:
  dw 24*4

fastRandom: ; ~462 cycles
  mov bx,[randomIndex1]          ; 3 2 20 18
  mov si,[randomIndex2]          ; 3 2 20 18
  mov ax,[randomData + bx]       ; 3 2 20 21
  add ax,[randomData + si]       ; 3 2 20 22
  mov dx,[randomData + bx + 2]   ; 3 2 20 21
  adc dx,[randomData + si + 2]   ; 3 2 20 22
  mov [randomData + bx], ax      ; 3 2 20 22
  mov [randomData + bx + 2], dx  ; 3 2 20 22
  add bx,4                       ; 3 0 12  4
  cmp bx,55*4                    ; 3 0 12  4
  je zeroRandomIndex1            ; 2 0  8  4
  mov [randomIndex1],bx          ; 3 2 20 19
  add si,4                       ; 3 0 12  4
  cmp si,55*4                    ; 3 0 12  4
  je zeroRandomIndex2            ; 2 0  8  4
  mov [randomIndex2],si          ; 3 2 20 19
  mov bx,16000                   ; 3 0 12  4
  div bx                         ; 2 0  8  144-162
  ret                            ; 1 2 12 20
zeroRandomIndex1:
  mov w[randomIndex1],0
  mov w[randomIndex2],24*4
  mov bx,16000
  div bx
  ret
zeroRandomIndex2:
  mov w[randomIndex2],0
  mov bx,16000
  div bx
  ret









  mov di,9999      ; 3 0 12  4
  stosb            ; 1 1  8 11
  shl di,1         ; 2 0  8  2
  mov bx,[di]      ; 2 2 16 18
  es: mov [bx],ah  ; 3 1 16 16
  mov [8888],bx    ; 4 2 24 19   ; 8888 = address of earlier 9999



