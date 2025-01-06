  cmp byte[9],0
  jnz .alreadySplit
  push ax                       ; Split routine is 66 bytes
  push bx
  push di
  push ds                     ; save old node
  xor bx,bx
  mov di,[bx]  ; Colour of unsplit node
  mov ax,[cs:nextFreeNode]
  mov ds,ax                   ; ds is new node
  mov [bx],di
  mov [bx+0x10],di
  mov [bx+0x20],di
  mov [bx+0x30],di
  mov [bx+0x09],bl
  mov [bx+0x19],bl
  mov [bx+0x29],bl
  mov [bx+0x39],bl
  pop ds                      ; old node
  mov di,ds
  mov es,di                   ; old node
  mov di,bx
  stosw
  inc ax
  stosw
  inc ax
  stosw
  inc ax
  stosw
  inc ax
  mov [cs:nextFreeNode],ax
  pop di
  pop bx
  pop ax
.alreadySplit:

;Start at i
;  up up left to b           after up up, divide slope by 2. Add carry on to the first down. Or if slope is horizontal then add carry on to the first left and right.
;  down left to d
;  right to e - recurse
;  right to f - recurse
;  down left to h - recurse
;  right to i - recurse



  shr si,1
  jc .slope1odd
  shr di,1
  jc .slope2odd
  shr bp,1
  jc .slope3odd

  ; all slopes even case, all slopes vertical case
  sub bx,si
  sub cx,di
  sub dx,bp




  mov es,ax                                ;  E b D
  and ax,0x0842                            ;  d e f
  mov cs:[patch1+1],ax                     ;  B h A
  mov ax,es
  and ax,0x2108
  mov cs:[patch2+1],ax
  mov ax,es
  and ax,0x4210
  mov cs:[patch3+1],ax
  mov ax,es
  push ax
  ; "i" was last point computed
  ; assume slope multiplied by vertical
  ; assume slope a to c distance is 0x100
  and ax,0x0421  ; Keep "a" bits in EJO
  sub bx,si  ; move from i to c
  sub bx,0x80 ; move from c to b
  js insideB

  add bx,si
  add bx,0x100

  ;"a" bits are in E,J,O already                     E D c
  ; TODO: Compute "b" in D,I,N                       B A f
  ; TODO: Compute "e" in A,F,K                       g h i
  ; TODO: Compute "d" in B,G,L
  push ds
  mov ds,[0]
  call nextLevelTL
  pop ds
  ;"a" bits no longer needed (mask off E,J,O)                      spare bits C,H,M
  ;Do a lookup to move "b" bits from D,I,N to E,J,O                                        "right" lookup table  (also moves unused 4,9,14 bits to 1,6,11)  shift right by 1        a E D   a 0 1
  ;                    "e" bits from A,F,K to B,G,L                                                                                                                                 C B A   2 3 4
  ;                    "d" bits from B,G,L to C,H,M                                                                                                                                 g h i   g h i
  shr ax,1
  and ax,0xf7bd                                                                      ;  P  O   N   M   L   K  J  I  H  G  F  E  D  C  B  A
patch1:
  or ax,9999    ; Move "c" bits from D,I,N immediate to D,I,N                             10  11  12  13  14  5  6  7  8  9  0  1  2  3  4
  ; TODO: Compute "f" in A,F,K
  push ds
  mov ds,[2]
  call nextLevelTR
  pop ds
  ;"b" and "c" bits no longer needed (mask off E,J,O and D,I,N)    spare bits D,I,N
  ;Do a lookup to move "d" bits from C,H,M to E,J,O                                        "down" lookup table                                              shift right by 2        a b c   a b c
  ;                    "e" bits from B,G,L to D,I,N                                        2->1 3->4->0 7->6 8->9->5 13->14                                 (down and left)         E D C   0 1 2
  ;                    "f" bits from A,F,K to C,H,M                                         e  c f  d  b                                                                            B A i   3 4 i
  shr ax,1
  shr ax,1
  and ax,0xdef7
patch2:
  or ax,9999    ; Move "g" bits from B,G,L immediate to B,G,L
  ; TODO: Compute "h" in A,F,K
  push ds
  mov ds,[4]
  call nextLevelBL
  pop ds
  ;"d" and "g" bits no longer needed (mask off E,J,O and B,G,L)    spare bits B,G,L        "right" lookup table  (also moves 2,7,12 ("g") bits to 4,9,14)   shift right by 1        a b c   a b c
  ;Do a lookup to move "e" bits from 1,6,11 to E,J,O                bits used in lookups: 1,2,3,4,6,7,8,9,11,12,13,14  64kB - doable! But need to permute                           d E D   d 0 1
  ;                    "h" bits from A,F,K to B,G,L                                                                                                                                 C B A   2 3 4
  ;                    "f" bits from C,H,M to D,I,N
  shr ax,1
  and ax,0xbdef
patch3:
  or ax,9999    ;  Move "i" bits from A,F,K immediate to A,F,K
  push ds
  mov ds,[6]
  call nextLevelBR
  pop ds
  pop ax
  ret

