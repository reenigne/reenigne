Edge 1: compute 4 points - if all outside then return. Otherwise, store in/out flags in bits 0-3 of AX      ABDE
Edge 2: compute 4 points - if all outside then return. Otherwise, store in/out flags in bits 5-8 of AX      FGIJ
Edge 3: compute 4 points - if all outside then return. Otherwise, store in/out flags in bits 10-13 of AX    KLNO
If AX is 0xfff (all inside) then fill block with colour and return
We're going to split. See if node is already split. If not then split node.
a b c
d e f
g h i

  mov es,ax                                  E b D
  and ax, D,I,N                              d e f
  mov [(1)],ax                               B h A
  mov ax,es
  and ax, B,G,L
  mov [(2)],ax
  mov ax,es
  and ax, A,F,K
  mov [(3)],ax
  mov ax,es

  "a" bits are in E,J,O already              E D c
  Compute "b" in D,I,N                       B A f
  Compute "e" in A,F,K                       g h i
  Compute "d" in B,G,L
  push ds
  mov ds,[0]
  call nextLevelTL
  pop ds
  "a" bits no longer needed (mask off E,J,O)                      spare bits C,H,M
  Do a lookup to move "b" bits from D,I,N to E,J,O                                        "right" lookup table  (also moves unused 4,9,14 bits to 1,6,11)  shift by 1              a E D
                      "e" bits from A,F,K to B,G,L                                        3->2->4->1->0 8->7->9->6->5 13->12->14->11->10                                           C B A
                      "d" bits from B,G,L to C,H,M                                        10<-11<-14<-12<-13<-5<-6<-9<-7<-8<-0<-1<-4<-2<-3                                         g h i
  and ax, ~(D,I,N)                                                                      P  O   N   M   L   K  J  I  H  G  F  E  D  C  B  A
  or ax,(1)     ; Move "c" bits from D,I,N immediate to D,I,N                             14  13  12  11  10  9  8  7  6  5  4  3  2  1  0

  Compute "f" in A,F,K
  push ds
  mov ds,[2]
  call nextLevelTR
  pop ds
  "b" and "c" bits no longer needed (mask off E,J,O and D,I,N)    spare bits D,I,N
  Do a lookup to move "d" bits from C,H,M to E,J,O                                        "down" lookup table                                              shift by 2              a b c
                      "e" bits from B,G,L to D,I,N                                        2->1 3->4->0 7->6 8->9->5 13->14                                                         E D C
                      "f" bits from A,F,K to C,H,M                                         e  c f  d  b                                                                            B A i
  Move "g" bits from B,G,L immediate to B,G,L
  Compute "h" in A,F,K
  push ds
  mov ds,[4]
  call nextLevelBL
  pop ds
  "d" and "g" bits no longer needed (mask off E,J,O and B,G,L)    spare bits B,G,L        "right" lookup table  (also moves 2,7,12 ("g") bits to 4,9,14)                           a b c
  Do a lookup to move "e" bits from 1,6,11 to E,J,O                bits used in lookups: 1,2,3,4,6,7,8,9,11,12,13,14  64kB - doable! But need to permute                           d E D
                      "h" bits from A,F,K to B,G,L                                                                                                                                 C B A
                      "f" bits from C,H,M to D,I,N
  Move "i" bits from A,F,K immediate to A,F,K
  push ds
  mov ds,[6]
  call nextLevelBR
  pop ds





lookup:
  and ax,0xfff          ; 3
  add ax,lookupSegment  ; 3
  mov es,ax             ; 2
  mov ax,[es:4]         ; 6



