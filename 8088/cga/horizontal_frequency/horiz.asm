  %include "../../defaults_bin.asm"

  mov ax,cs
  mov ds,ax

  mov ax,3
  int 0x10
  mov ax,0x0009
  mov dx,0x3d8
  out dx,ax
  mov dl,0xd4
  mov ax,0x0f03
  out dx,ax
  mov ax,0xb800
  mov es,ax
  xor di,di
  mov cx,8192
  mov ax,0x700
init:
  stosw
  inc ax
  loop init

;  mov ax,0x7f04
;  out dx,ax
;  mov ax,0x7f06
;  out dx,ax
;  mov ax,0x7f07
;  out dx,ax
;  mov ax,0x0109
;  out dx,ax

l:
  mov ah,0
  int 0x16

  cmp ax,0x4b00
  jne notLeft
  dec byte[ht]
notLeft:

  cmp ax,0x4d00
  jne notRight
  inc byte[ht]
notRight:

  cmp ax,0x4800
  jne notUp
  dec byte[vta]
notUp:

  cmp ax,0x5000
  jne notDown
  inc byte[vta]
notDown:

  mov ah,[ht]
  mov al,0
  mov dx,0x3d4
  out dx,ax
  sub ah,(114-1)-80
  mov al,1
  out dx,ax
  sub ah,80-90
  mov al,2
  out dx,ax

  mov bl,[ht]
;  mov bh,0
;  mov ah,[vtas-(107-1)+bx]
;  add ah,[vta]
;  mov al,5
;  out dx,ax
;  mov ah,[vts-(107-1)+bx]
;  mov al,4
;  out dx,ax
;  mov al,7
;  sub ah,2
;  out dx,ax
;  mov al,6
;  dec ah
;  out dx,ax

  inc bx
  xor dx,dx
  mov ax,114*262
  div bx
  xchg bx,ax
  mov al,[vta]
  cbw
  add ax,bx
  mov bl,8
  div bl
  mov cl,al
  mov dx,0x3d4
  mov al,5
  out dx,ax
  mov ah,cl
  dec ah
  mov al,4
  out dx,ax
  mov al,7
  sub ah,2
  out dx,ax
  mov al,6
  dec ah
  out dx,ax


  jmp l

ht:
  db 114-1

vta:
  db 0

; Minimum H = 107 16727 (5153 unstable going G->H but stable I->H)    279      27*8+5 = 221     230
;         I   108 16572                                               277
;         J   109 16420                                               274
;         K   110 16271                                               272
;         L   111 16124                                               269
;         M   112 15980                                               267
;         N   113 15839                                               264
; Default O = 114 15700                                               262      27*8 = 216          24624
;         P   115 15563                                               260
;         Q   116 15429                                               257
; Maximum R   117 15297 (on TV)                                       255
;         S   118 15168                                               253
;         T   119 15044 (on 5153 when vsynced)                        251      26*4+4 = 212     206
; Maximum U   120 14915 (on 5153)                                     249

;  107 108 109 110 111 112 113 114 115 116 117 118 119 120
;vtas:
;db  23, 21, 18, 16, 13, 11,  8,  6,  4,  1,  1,  1,  1,  1
;vts:
;db 127,127,127,127,127,127,127,127,127,127,126,125,124,123
vtas:
db   7,  5,  2,  0,  5,  3,  0,  6,  4,  1,  7,  5,  3,  1
vts:
db  33, 33, 33, 33, 32, 32, 32, 31, 31, 31, 30, 30, 30, 30

