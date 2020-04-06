org 0x100
cpu 8086

; bp = scratch
; si = scratch
; es:di = VRAM pointer
; cx = xi
; dx = yi
; bl:al = xs
; bh:ah = ys
; ds = map (2 bytes per tile, tile number in bits 0..3 and 8..11
; sp = 0xf0f0
; ss = tile data (with gaps for stack)
%macro rasterizeLine 0
%rep 80
  add al,cl         ; 2 0 2
  adc bl,ch         ; 2 0 2
  add ah,dl         ; 2 0 2
  adc bh,dh         ; 2 0 2
  mov si,bx         ; 2 0 2
  mov si,[bx+si]    ; 2 2 4
  mov bp,ax         ; 2 0 2
  and bp,sp         ; 2 0 2
  add si,bp         ; 2 0 2
  ss movsb          ; 2 2 4    24 IOs, 20 bytes
%endrep
%endmacro



; angle in BX (0..319)
; x position in DX
; y position in CX
rasterize:
  ; TODO: initialize SS:SP
  ; TODO: intiialize dataSegment
  ; TODO: fix up jumps
  ; TODO: initialize mapSegment


  mov ax,0xb800
  mov es,ax
  mov di,50*80

  cmp bx,160
  jge rasterize23
  cmp bx,80
  jge rasterize1


rasterize0:

;  xs = r*sin(a)                     + xp
;  ys = r*cos(a) = r*sin(80 - a)     + yp
;  xi = r*cos(a)/40 = r*sin(80 - a)/40
;  yi = r*sin(a)/40
;  xs -= 40*xi
;  ys -= 40*yi

  mov [cs:.patchYPos+2],cx
  mov [cs:.patchXPos+2],dx

  add bx,bx
  mov bx,[bx+anglePointers]
  mov cx,50
.yLoop:
  push bx
  push cx
  mov si,[bx]               ; si = sin(a) = xs
.patchXPos:
  add si,9999               ; si = xs + xp
  mov dx,[bx+2]             ; dx = sin(a)/40 = yi
  sub bx,100*80 + bigTable*2
  neg bx                    ; bx = (100*79 - (bx - bigTable)) + bigTable
  mov bp,[bx]               ; bp = cos(a) = ys
.patchYPos:
  add bp,9999               ; bp = ys + yp
  mov cx,[bx+2]             ; cx = cos(a)/40 = xi

  mov bx,cx
  add bx,bx
  mov ax,[bx+mul40table]    ; ax = xi*40
  neg ax
  add ax,si                 ; ax = -xi*40 + xs + xp

  mov bx,dx
  add bx,bx
  mov bx,[bx+mul40table]    ; bx = yi*40
  neg bx
  add bx,bp                 ; bx = -yi*40 + ys + yp

  xchg ah,bl
  mov ds,[mapSegment]
  rasterizeLine
  mov ds,[cs:dataSegment]
  pop bx
  add bx,4
  pop cx
  loop .yLoop
  ret


rasterize1:

;  xs = r*sin(a) = r*sin(80 - (a - 80)) + xp
;  ys = r*cos(a) = -r*sin(a - 80)       + yp
;  xi = r*cos(a)/40 = -r*sin(a - 80)/40
;  yi = r*sin(a)/40 = r*sin(80 - (a - 80))
;  xs -= 40*xi
;  ys -= 40*yi

  mov [cs:.patchYPos+2],cx
  mov [cs:.patchXPos+2],dx

  add bx,bx
  mov bx,[bx+anglePointers - 80*2]
  mov cx,50
.yLoop:
  push bx
  push cx
  mov si,[bx]               ; si = sin(a - 80) = -ys
  neg si                    ; si = ys
.patchYPos:
  add si,9999               ; si = ys + yp
  mov dx,[bx+2]             ; dx = sin(a - 80)/40 = -xi
  sub bx,100*80 + bigTable*2
  neg bx
  mov bp,[bx]               ; bp = sin(80 - (a - 80)) = xs
.patchXPos:
  add bp,9999               ; bp = sin(80 - (a - 80)) + xp
  mov cx,[bx+2]             ; cx = sin(80 - (a - 80))/40 = yi

  mov bx,dx
  neg dx                    ; dx = xi
  add bx,bx
  mov ax,[bx+mul40table]    ; ax = (-xi)*40
  add ax,bp                 ; ax = -xi*40 + xs + xp

  mov bx,cx
  add bx,bx
  mov bx,[bx+mul40table]    ; bx = yi*40
  neg bx
  add bx,si                 ; bx = -yi*40 + ys + yp

  xchg ah,bl
  mov ds,[mapSegment]
  rasterizeLine
  mov ds,[cs:dataSegment]
  pop bx
  add bx,4
  pop cx
  loop .yLoop
  ret


rasterize23:
  cmp bx,240
  jge rasterize3

rasterize2:

;  xs = r*sin(a) = -r*sin(a - 160) + xp
;  ys = r*cos(a) = -r*sin(80 - (a - 160)) + yp
;  xi = r*cos(a)/40 = -r*sin(80 - (a - 160)/40
;  yi = r*sin(a)/40 = -r*sin(a - 160)
;  xs -= 40*xi
;  ys -= 40*yi

  mov [cs:.patchYPos+2],cx
  mov [cs:.patchXPos+2],dx

  add bx,bx
  mov bx,[bx+anglePointers - 80*2*2]
  mov cx,50
.yLoop:
  push bx
  push cx
  mov si,[bx]
  neg si
.patchXPos:
  add si,9999
  mov dx,[bx+2]
  sub bx,100*80 + bigTable*2
  neg bx
  mov bp,[bx]
  neg bp
.patchYPos:
  add bp,9999
  mov cx,[bx+2]

  mov bx,cx
  neg cx
  add bx,bx
  mov ax,[bx+mul40table]
  add ax,si

  mov bx,dx
  neg dx
  add bx,bx
  mov bx,[bx+mul40table]
  add bx,bp

  xchg ah,bl
  mov ds,[mapSegment]
  rasterizeLine
  mov ds,[cs:dataSegment]
  pop bx
  add bx,4
  pop cx
  loop .yLoop
  ret


rasterize3:

;  xs = r*sin(a) = -r*sin(80 - (a - 240)) + xp
;  ys = r*cos(a) = r*sin(a - 240)       + yp
;  xi = r*cos(a)/40 = r*sin(a - 240)/40
;  yi = r*sin(a)/40 = -r*sin(80 - (a - 240))
;  xs -= 40*xi
;  ys -= 40*yi

  mov [cs:.patchYPos+2],cx
  mov [cs:.patchXPos+2],dx

  add bx,bx
  mov bx,[bx+anglePointers - 80*2*4]
  mov cx,50
.yLoop:
  push bx
  push cx
  mov si,[bx]
.patchYPos:
  add si,9999
  mov dx,[bx+2]
  sub bx,100*80 + bigTable*2
  neg bx
  mov bp,[bx]
  neg bp
.patchXPos:
  add bp,9999
  mov cx,[bx+2]

  mov bx,dx
  add bx,bx
  mov ax,[bx+mul40table]
  neg ax
  add ax,bp

  mov bx,cx
  neg cx
  add bx,bx
  mov bx,[bx+mul40table]
  add bx,si

  xchg ah,bl
  mov ds,[mapSegment]
  rasterizeLine
  mov ds,[cs:dataSegment]
  pop bx
  add bx,4
  pop cx
  loop .yLoop
  ret


; Tables needed:
;   anglePointers - 1 word per angle, points to bigTable to avoid multiply (i*200+bigTable)
;   bigTable
;     For each a in 0..80
;       For each y position in 0..49
;         r*sin(a) - 2 bytes
;         r*sin(a)/40 - 2 bytes
;   mul40Table - 1 word per entry, enough to cover all the r*sin(a)/40 values

dataSegment: dw 0

anglePointers:

bigTable:

mul40table:

mapSegment:
