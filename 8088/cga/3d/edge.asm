; si = vertex a
; di = vertex b

  mov ax,[si]      ; ax
  mov bx,[si+2]    ; ay
  mov cx,[di]      ; bx
  mov dx,[di+2]    ; by
  sub ax,cx        ; ax-bx
  jl spanXNegative
  sub bx,dx        ; ay-by
  jl spanXPYNegative
  cmp ax,bx
  jl spanXPYPXSmaller

; case spanx >= 0, spany >= 0, spanx > spany
;   intercept = (ax*by - bx*ay)/(ax - bx)
; (ax - bx)*(ay + by) - (ax + bx)*(ay - by) = ax*ay - bx*ay + ax*by - bx*by - ax*ay - bx*ay + ax*by + bx*by = 2*(ax*by - bx*ay)
; (ax*by - bx*ay)/(ax - bx) = ((ax - bx)*(ay + by) - (ax + bx)*(ay - by))/2*(ax - bx) = (ay + by)/2 - (ax + bx)*(ay - by)/2*(ax - bx)
;   = by - bx*(ay - by)/(ax - bx)

  mov di,[bx]             ; log(spany)                                2 2
  xchg ax,si              ; si = spanx                                1 0
  sub di,[si]             ; log(spany)-log(spanx)                     2 2
  es: mov bp,[di+log512]  ; exp(log(spany)-log(spanx)) = spany/spanx  5 2
  mov si,cx               ; bx                                        2 0
  add di,[si]             ; log(spany)-log(spanx)+log(bx)             2 2
  es: sub dx,[di]         ; by - spany*bx/spanx                       3 2
  ; TODO: record horizontalish = true, inverted = false
  jmp edgeDone            ;                                           2 0
spanXPYPXSmaller:
  xchg ax,bx
  mov di,[bx]
  xchg ax,si
  sub di,[si]
  es: mov bp,[di+log512]
  mov si,dx
  add di,[si]
  es: sub cx,[di]
  mov dx,cx
  ; TODO: record horizontalish = false, inverted = false
  jmp edgeDone
spanXPYNegative:
  cmp ax,bx
  jl spanXPYNXSmaller
  mov di,[bx]
  xchg ax,si
  sub di,[si]
  es: mov bp,[di+log512+0x8000]
  mov si,cx
  add di,[si]
  es: sub dx,[di+0x8000]
  ; TODO: record horizontalish = true, inverted = false
  jmp edgeDone
soanXPYNXSmaller:
  xchg ax,bx
  mov di,[bx]
  xchg ax,si
  sub di,[si]
  es: mov bp,[di+log512+0x8000]
  mov si,dx
  add di,[si]
  es: sub cx,[di+0x8000]
  mov dx,cx
  ; TODO: record horizontalish = false, inverted = true
  jmp edgeDone
spanXNegative:
  neg ax
  sub bx,dx
  jl spanXNYNegative
  cmp ax,bx
  jl spanXNYPXSmaller

  mov di,[bx]
  xchg ax,si
  sub di,[si]
  es: mov bp,[di+log512+0x8000]
  mov si,cx
  add di,[si]
  es: sub dx,[di+0x8000]
  ; TODO: record horizontalish = true, inverted = true
  jmp edgeDone
spanXNYPXSmaller:
  xchg ax,bx
  mov di,[bx]
  xchg ax,si
  sub di,[si]
  es: mov bp,[di+log512+0x8000]
  mov si,dx
  add di,[si]
  es: sub cx,[di+0x8000]
  mov dx,cx
  ; TODO: record horizontalish = false, inverted = false
  jmp edgeDone
spanXNYNegative:
  neg bx
  cmp ax,bx
  jl spanXPYNXSmaller
  neg bx
  mov di,[bx]
  xchg ax,si
  sub di,[si]
  es: mov bp,[di+log512]
  mov si,cx
  add di,[si]
  es: sub dx,[di]
  ; TODO: record horizontalish = true, inverted = true
  jmp edgeDone
soanXNYNXSmaller:
  neg bx
  xchg ax,bx
  mov di,[bx]
  xchg ax,si
  sub di,[si]
  es: mov bp,[di+log512]
  mov si,dx
  add di,[si]
  es: sub cx,[di]
  mov dx,cx
  ; TODO: record horizontalish = false, inverted = true


edgeDone:
   shl dx,1               ; copy bit 14 into bit 16 leaving normal values -0x3ffe..0x3ffe but turning overflow 0x8000 into 0x0000 (overflow is wrapped underflow)
   sar dx,1               ;                                           4 0  124 cycles



