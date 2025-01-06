; imul rw is 141 cycles on average


; Transform vertices according to view matrix
; sx = _xx*wx + _xy*wy + _xz*wz
; sy = _yx*wx + _yy*wy + _yz*wz
; sz = _zx*wx + _zy*wy + _zy*wz

_xx: dw 0
_xy: dw 0
_xz: dw 0
_yx: dw 0
_yy: dw 0
_yz: dw 0
_zx: dw 0
_zy: dw 0
_zz: dw 0
loopTop:
  mov bp,savedCX+1-127  ; 3 0
  mov [bp+127],cx       ; 3 0  bp-relative
vp:
  mov si,9999           ; 3 0  vertex pointer
  lodsw                 ; 1 2  wx
  mov bx,ax             ; 2 0
  imul w[_zx]           ; 3 2  bp-relative
  mov cx,dx             ; 2 0
  lodsw                 ; 1 2  wy
  mov di,ax             ; 2 0
  imul w[_zy]           ; 3 2  bp-relative
  add cx,dx             ; 2 0
  lodsw                 ; 1 2  wz
  mov [vp+1],si         ; 3 2  bp-relative
  mov si,ax             ; 2 0
  imul w[_zz]           ; 3 2  bp-relative
  add cx,dx             ; 2 0
  mov es,cx             ; 2 0

  mov ax,bx             ; 2 0
  imul w[_yx]           ; 3 2  bp-relative
  mov bp,ax             ; 2 0
  mov cx,dx             ; 2 0
  mov ax,di             ; 2 0
  imul w[_yy]           ; 4 2  absolute
  add bp,ax             ; 2 0
  adc cx,dx             ; 2 0
  mov ax,si             ; 2 0
  imul w[_yz]           ; 4 2  absolute
  add ax,bp             ; 2 0
  adc dx,cx             ; 2 0
  mov cx,es             ; 2 0
  idiv cx               ; 2 0
  push ax               ; 1 2  saves to [sy]

  mov ax,bx             ; 2 0
  imul w[_xx]           ; 4 2  absolute
  mov bp,ax             ; 2 0
  mov cx,dx             ; 2 0
  mov ax,di             ; 2 0
  imul w[_xy]           ; 4 2  absolute
  add bp,ax             ; 2 0
  adc cx,dx             ; 2 0
  mov ax,si             ; 2 0
  imul w[_xz]           ; 4 2  absolute
  add ax,bp             ; 2 0
  adc dx,cx             ; 2 0
  mov cx,es             ; 2 0
  idiv cx               ; 2 0
  push ax               ; 1 2  saves to [sx]
  push cx               ; 1 2  saves to [sz]
savedCX:                ;      bp+126
  mov cx,9999           ; 3 0
  loop loopTop          ; 2 0

; 556 cycles in IOs, 1269 cycles in IMULs, 349 cycles in IDIVs, -11*3*4 = 132 cycles in preload savings, total = 2042 cycles per vertex = 6.7 scanlines
;   Not counting loop, vertex pointer init etc.  -56 cycles total 1986 cycles
; Stack is directly below transformed vertex buffer so latter grows downwards
; Set DF so the world coordinates go downwards the same way?


; Transform with square tables instead of imul. Reduced range and precision and large table but is it faster?
;   World coordinates go from -0x4000 to 0x3ffc with a granularity of 4
;   Matrix elements go from -0x3ffc to 0x3ffc with a granularity of 4
;   zScale = 0x3ffc  xScale = 0x00a0  yScale =  0x00c0
;   If we have output in pixels instead then we have xScale = 0x0280  yScale = 0x0300
;   If we have different square tables for z than for x and y then we can bake different scale factors into each and retain more precision?
;     Can we? The input can be at whatever the resolution of the rotor but if we're putting the output into idiv it has to be at a resolution that gives zScale/xScale = 102.4
;       But since we don't have the granularity requirement we can use all the bits, giving us an effective xScale of 0x7fff/102.4 = 320
;   A narrower field of view gives finer rotations


;savedSI:
;  mov si,xxx            ; 3 0
  lodsw                 ; 1 2
  xchg di,ax            ; 1 0
  lodsw                 ; 1 2
  xchg bp,ax            ; 1 0
  lodsw                 ; 1 2
;  mov
  xchg si,ax            ; 1 0

;  mov di,[bp+wx]        ; 3 2
  mov bx,_zx            ; 3 0
  mov cx,[di+bx]        ; 2 2
  neg bx                ; 2 0
  sub cx,[di+bx]        ; 2 2
;  mov si,[bp+wy]        ; 3 2
  mov bx,_zy            ; 3 0
  add cx,[si+bx]        ; 2 2
  neg bx                ; 2 0
  sub cx,[si+bx]        ; 2 2
;  mov bp,[bp+wz]        ; 3 2
  xchg bp,si            ; 2 0
  mov bx,_zz            ; 3 0
  add cx,[si+bx]        ; 2 2
  neg bx                ; 2 0
  sub cx,[si+bx]        ; 2 2

  mov bx,_yx            ; 3 0
  les ax,[di+bx]        ; 2 4
  mov dx,es             ; 2 0
  neg bx                ; 2 0
  sub ax,[di+bx]        ; 2 2
  sbb dx,[di+bx+2]      ; 3 2
  xchg si,bp            ; 2 0
  mov bx,_yy            ; 3 0
  add ax,[si+bx]        ; 2 2
  adc dx,[si+bx+2]      ; 3 2
  neg bx                ; 2 0
  sub ax,[si+bx]        ; 2 2
  sbb dx,[si+bx+2]      ; 3 2
  mov bx,_yz            ; 3 0
  xchg si,bp            ; 2 0
  add ax,[si+bx]        ; 2 2
  adc dx,[si+bx+2]      ; 3 2
  neg bx                ; 2 0
  sub ax,[si+bx]        ; 2 2
  sbb dx,[si+bx+2]      ; 3 2
  idiv cx               ; 2 0
  push ax               ; 1 2

  mov bx,_xx            ; 3 0
  les ax,[di+bx]        ; 2 4
  mov dx,es             ; 2 0
  neg bx                ; 2 0
  sub ax,[di+bx]        ; 2 2
  sbb ax,[di+bx+2]      ; 3 2
;  mov si,[bp+wy]        ; 3 2
  xchg si,bp            ; 2 0
  mov bx,_xy            ; 3 0
  add ax,[si+bx]        ; 2 2
  adc dx,[si+bx+2]      ; 3 2
  neg bx                ; 2 0
  sub ax,[si+bx]        ; 2 2
  sbb dx,[si+bx+2]      ; 3 2
  xchg si,bp            ; 2 0
  mov bx,_xz            ; 3 0
  add ax,[si+bx]        ; 2 2
  adc dx,[si+bx+2]      ; 3 2
  neg bx                ; 2 0
  sub ax,[si+bx]        ; 2 2
  sbb dx,[si+bx+2]      ; 3 2
  idiv cx               ; 2 0
  push ax               ; 1 2       ~840 cycles per vertex in IOs, 349 cycles in IDIVs, -2*3*4 = 24 cycles in preload savings, total = 1165 cycles per vertex = 3.8 scanlines
  push cx               ; 1 2


; Can we do the calculations in 16-bit precision and then shift left by 8 bits before division?
;   Suppose yScale = 0x0100, xScale = 0x00d5, zScale = 0x6666 (26214.4)   (could double this if max world coordinate is 0x3ffe)
;   pre-projected error would be +/- 0x7f.  Post-projection, error would be 0.0048 subpixels at maximum distance.
;   If minimum distance is 0x100 times this then that is 1.2 subpixels - not too bad

  lodsw                 ; 1 2
  sub ax,_ex            ; 3 0
  xchg di,ax            ; 1 0
  lodsw                 ; 1 2
  sub ax,_ey            ; 3 0
  xchg bx,ax            ; 1 0
  lodsw                 ; 1 2
  sub ax,_ez            ; 3 0
  xchg si,ax            ; 1 0

  mov cx,[di+_zx]       ; 4 2
  sub cx,[di-_zx]       ; 4 2
  add cx,[bx+_zy]       ; 4 2
  sub cx,[bx-_zy]       ; 4 2
  add cx,[si+_zz]       ; 4 2
  sub cx,[si-_zz]       ; 4 2

  mov ax,[di+_yx]       ; 4 2
  sub ax,[di-_yx]       ; 4 2
  add ax,[bx+_yy]       ; 4 2
  sub ax,[bx-_yy]       ; 4 2
  add ax,[si+_yz]       ; 4 2
  sub ax,[si-_yz]       ; 4 2
                                ; dl is byte 1  dh is byte 2
;  mov al,dh             ; 2 0  ; al is byte 2
;  cbw                   ; 1 0  ; ah is byte 3
;  xchg ax,dx            ; 1 0  ; dh is byte 3, dl is byte 2, al is byte 1, ah is byte 2
;  mov ah,al             ; 2 0  ; ah is byte 1
;  mov al,0              ; 2 0  ; al is byte 0

                        ;      ; al is byte 1  ah is byte 2
  cwd                   ; 1 0  ; dh is byte 3
  mov dl,ah             ; 2 0  ; dl is byte 2
  mov ah,al             ; 2 0  ; ah is byte 1
  mov al,0              ; 2 0  ; al is byte 0
  idiv cx               ; 2 0
  push ax               ; 1 2

  mov ax,[di+_xx]       ; 4 2
  sub ax,[di-_xx]       ; 4 2
  add ax,[bx+_xy]       ; 4 2
  sub ax,[bx-_xy]       ; 4 2
  add ax,[si+_xz]       ; 4 2
  sub ax,[si-_xz]       ; 4 2
                        ;      ; al is byte 1  ah is byte 2
  cwd                   ; 1 0  ; dh is byte 3
  mov dl,ah             ; 2 0  ; dl is byte 2
  mov ah,al             ; 2 0  ; ah is byte 1
  mov al,0              ; 2 0  ; al is byte 0
  idiv cx               ; 2 0
  push ax               ; 1 2
  push cx               ; 1 2     ~588 cycles per vertex in IOs, 349 cycles in IDIVs, -2*3*4 = 24 cycles in preload savings, total = 913 cycles per vertex = 3.0 scanlines


; a/b = exp(log(a) - log(b))

; Using log tables intead of idiv - small speedup possible but lots of other compromises
;  mov di,cx               ; 2 0
;  xchg si,ax              ; 1 0
;  xor cx,si               ; 2 0
;  js negative             ; 2 0  4/2
;  mov bx,[logTable + si]  ; 4 2
;  mov ax,bx               ; 2 0
;  sub bx,[logTable + di]  ; 4 2
;  cmp bx,threshold        ; 4 0
;  jg overflow             ; 2 0
;  push [expTable + bx]    ; 4 4
;  mov di,dx               ; 2 0
;  xchg ax,bx              ; 1 0
;  sub bx,[logTable + di]  ; 4 2
;  cmp bx,threshold        ; 4 0
;  jg overflow2            ; 2 0
;  push [expTable + bx]    ; 4 4  240 cycles compared to 349+12+12-24 = 349 in idiv case, savings 109 cycles = 0.36 scanlines

  cmp cx,0                      ;
  jle zNegativeOrZero           ; 2 0  can do this after the final sub of the z computation
  cmp ax,0                      ;
  jle zPositiveXNegativeOrZero  ; 2 0  can do this after the final sub of the x computation
  xchg si,ax                    ; 1 0
  mov bx,[logTable+si]          ; 4 2
  mov di,cx                     ; 2 0
  mov ax,[logTable+di]          ; 4 2
  sub bx,ax                     ; 2 0
  cmp bx,threshold              ;      can do this by making the threshold zero
  jg overflowX                  ; 2 0
  push [expTable + bx]          ; 4 4
  cmp dx,0                      ;
  jle zPositiveYNegativeOrZero  ; 2 0  can do this after the final sub of the y computation
  mov si,dx                     ; 2 0
  mov bx,[logTable+si]          ; 4 2
  sub bx,ax                     ; 2 0
  cmp bx,threshold              ;      can do this by making the threshold zero
  jg overflowY                  ; 2 0
  push [expTable + bx]          ; 4 4  212 cycles

; How big do the log and exp tables need to be?
;   Let's suppose they're each 32kB (16384 entries)
;     logTable domain from 0 to 0x7ffe, range from 0 to 0x3ffe
;     expTable range from -0x3ffe to 0x3ffe
;     could we eliminate all the explicit negative handling by having two exp tables in the same 64kB segment, and the high bit (which can wrap) corresponds to the sign?
; logTable[x] = k*log(x + 0.5) + p
; expTable[x] = n*exp(x/k)
; expTable[0] = 1
; expTable[0x3ffe] = 0x7fff   m = 0x3ffe/log(0x7fff) = 1575.61997
; logTable[0] = 0             k*log(0.5) = -p    p = 1023.87922
; logTable[0x7ffe] = 0x3ffe   k*log(0x7ffe + 0.5) = 0x3ffe - p
;                             k*(log(0x7ffe + 0.5) - log(0.5)) = 0x3ffe  k = 0x3ffe/(log(0x7ffe + 0.5) - log(0.5))  10.397161930984222269295164385133+0.69314718055994530941723212145818
;                             k = 1477.1454821712394847206996976476
; expTable[0x3ffe] =  41941120
; expTable[0x2000] =   163947.20160097915426859052834905
; expTable[0] = 0x0280  n = 0x0280
; expTable[-0x3ffe] =  0.0097660720553003830131384188119

;  If the possible z values are offset by 0.5 (so goes from -0.5 to 0.5 and zero is unrepresentable) might simplify overflow handling
;

; What is the accuracy of this at the edges of the screen?
;   should be less than half a subpixel: exp(1/k)*640 = 640.43


  lodsw                 ; 1 2
  sub ax,_ex            ; 3 0
  xchg di,ax            ; 1 0
  lodsw                 ; 1 2
  sub ax,_ey            ; 3 0
  xchg bx,ax            ; 1 0
  lodsw                 ; 1 2
  sub ax,_ez            ; 3 0
  xchg si,ax            ; 1 0

  mov bp,[di+_zx]       ; 4 2
  sub bp,[di-_zx]       ; 4 2
  add bp,[bx+_zy]       ; 4 2
  sub bp,[bx-_zy]       ; 4 2
  add bp,[si+_zz]       ; 4 2
  sub bp,[si-_zz]       ; 4 2
  push bp               ; 1 2
  mov dx,[bp+00]        ; 3 2

  mov bp,[di+_yx]       ; 4 2
  sub bp,[di-_yx]       ; 4 2
  add bp,[bx+_yy]       ; 4 2
  sub bp,[bx-_yy]       ; 4 2
  add bp,[si+_yz]       ; 4 2
  sub bp,[si-_yz]       ; 4 2
  mov bp,[bp+00]        ; 3 2
  sub bp,dx             ; 2 0
  es: push w[bp+00]     ; 4 4

  mov bp,[di+_xx]       ; 4 2
  sub bp,[di-_xx]       ; 4 2
  add bp,[bx+_xy]       ; 4 2
  sub bp,[bx-_xy]       ; 4 2
  add bp,[si+_xz]       ; 4 2
  sub bp,[si-_xz]       ; 4 2
  mov si,[bp+00]        ; 3 2
  sub si,dx             ; 2 0
  es: push w[si]        ; 3 4  total 664 cycles = 2.2 scanlines




;With overflow checking:  (cx = 0x8000)

  lodsw                 ; 1 2
  sub ax,_ex            ; 3 0
  xchg di,ax            ; 1 0
  lodsw                 ; 1 2
  sub ax,_ey            ; 3 0
  xchg bx,ax            ; 1 0
overflowRedoZ:
  lodsw                 ; 1 2
  sub ax,_ez            ; 3 0
  xchg si,ax            ; 1 0
overflowPatchZ:

  mov bp,[di+_zx]       ; 4 2
  sub bp,[di-_zx]       ; 4 2
  add bp,[bx+_zy]       ; 4 2
  sub bp,[bx-_zy]       ; 4 2
  add bp,[si+_zz]       ; 4 2
  sub bp,[si-_zz]       ; 4 2
  push bp               ; 1 2
  mov dx,[bp+00]        ; 3 2

overflowRedoY:
  mov bp,[di+_yx]       ; 4 2
  sub bp,[di-_yx]       ; 4 2
  add bp,[bx+_yy]       ; 4 2
  sub bp,[bx-_yy]       ; 4 2
  add bp,[si+_yz]       ; 4 2
  sub bp,[si-_yz]       ; 4 2
  mov bp,[bp+00]        ; 3 2
overflowPatchY:
  sub bp,dx             ; 2 0
  es: mov ax,[bp+00]    ; 4 2
  cmp ax,cx             ; 2 0
  je overflow           ; 2 0
  push ax               ; 1 2

overflowRedoX:
  mov bp,[di+_xx]       ; 4 2
  sub bp,[di-_xx]       ; 4 2
  add bp,[bx+_xy]       ; 4 2
  sub bp,[bx-_xy]       ; 4 2
  add bp,[si+_xz]       ; 4 2
  sub bp,[si-_xz]       ; 4 2
  mov si,[bp+00]        ; 3 2
overflowPatchX:
  sub si,dx             ; 2 0
  es: mov ax,[si]       ; 3 2
  cmp ax,cx             ; 2 0
  je overflowY          ; 2 0
  push ax               ; 1 2  total 704 cycles = 2.3 scanlines

  jmp doneTransform
overflowY:
  pop ax
overflow:
  ; TODO: reset si to vertex z offset
  cs: mov b[overflowPatchZ],"ret"
  call overflowRedoZ
  cs: mov b[overflowPatchZ},"mov rw,rmw"
  ; si is now _rz

  cs: mov b[overflowPatchY],"ret"
  call overflowRedoY
  cs: mov b[overflowPatchY},"sub bp,dx"
  xchg ax,bp  ; ax is now log(_sy)

  cs: mov b[overflowPatchX],"ret"
  call overflowRedoX
  cs: mov b[overflowPatchX},"sub si,dx"
  ; si is now log(_sx)

  cmp si,0
  jl .sxNegative
  cmp ax,0
  jl .sxPositiveSyNegative
  cmp si,ax
  jle .sxPositiveSyPositiveSxSmaller
  sub ax,si
  xchg ax,si
  es: push [si+log0x3ffe]
  mov ax,0x3ffe
  push ax
  jmp doneTransform
.sxPositiveSyPositiveSxSmaller:
  sub si,ax
  mov ax,0x3ffe
  push ax
  es: push [si+log0x3ffe]
  jmp doneTransform
.sxPositiveSyNegative:
  xor ax,0x8000
  cmp si,ax
  jle .sxPositiveSyNegativeSxSmaller
  sub ax,si
  xchg ax,si
  es: push [si+log0x3ffe+0x8000]
  mov ax,0x3ffe
  push ax
  jmp doneTransform
.sxPositiveSyNegativeSxSmaller:
  sub si,ax
  mov ax,-0x3ffe
  push ax
  es: push [si+log0x3ffe]
  jmp doneTransform
.sxNegative:
  xor si,0x8000
  cmp ax,0
  jl .sxNegativeSyNegative
  cmp si,ax
  jle .sxNegativeSyPositiveSxSmaller
  sub ax,si
  xchg ax,si
  es: push [si+log0x3ffe]
  mov ax,-0x3ffe
  push ax
  jmp doneTransform
.sxNegativeSyPositiveSxSmaller:
  sub si,ax
  mov ax,0x3ffe
  push ax
  es: push [si+log0x3ffe+0x8000]
  jmp doneTransform
.sxNegativeSyNegative:
  xor ax,0x8000
  cmp si,ax
  jle .sxNegativeSxNegativeSxSmaller
  sub ax,si
  xchg ax,si
  es: push [si+log0x3ffe+0x8000]
  mov ax,-0x3ffe
  push ax
  jmp doneTransform
.sxNegativeSxNegativeSxSmaller:
  sub si,ax
  mov ax,-0x3ffe
  push ax
  es: push [si+log0x3ffe+0x8000]
doneTransform:

; TODO: check for underflows
; TODO: invert _px and _py if _sz < 0




; Older:

; mov ax,[wx]  3 2
; mov bx,ax    2 0
; imul w[_xx]  3 2
; mov cx,dx    2 0
; mov ax,[wy]  3 2
; mov si,ax    2 0
; imul w[_xy]  3 2
; add cx,dx    2 0
; mov ax,[wz]  3 2
; mov di,ax    2 0
; imul w[_xz]  3 2
; add cx,dx    2 0
; mov [sx],cx  3 2
;
; mov ax,bx    2 0
; imul w[_yx]  3 2
; mov cx,dx    2 0
; mov ax,di    2 0
; imul w[_yy]  3 2
; add cx,dx    2 0
; mov ax,si    2 0
; imul w[_yz]  3 2
; add cx,dx    2 0
; mov [sy],cx  3 2
;
; mov ax,bx    2 0
; imul w[_zx]  3 2
; mov cx,dx    2 0
; mov ax,di    2 0
; imul w[_zy]  3 2
; add cx,dx    2 0
; mov ax,si    2 0
; imul w[_zz]  3 2
; add cx,dx    2 0
; mov [sz],cx  3 2  444 cycles in IOs, 1269 cycles in IMULs, total 1713 = 5.6 scanlines  - 9*3*4 = 108 cycles in preload savings


; For a 9x16 bit multiply, can we beat imul?

;    x*y = (x+y)^2 / 4 - (x-y)^2 / 4
;    x = x0 + 0x100*x1
;    x*y = (x0 + 0x100*x1)*y = x0*y + 0x100*x1*y = (x0+y)^2 / 4 - (x0-y)^2 / 4 + 0x100*(x1+y)^2 / 4 - 0x100*(x1-y)^2 / 4
;
;x in ax
;y in dx
;
;mov bl,al          2 0
;mov bh,0           2 0
;mov si,dx          2 0
;mov cx,[bx+si]     2 2
;neg si             2 0
;sub cx,[bx+si]     2 2
;mov bl,ah          2 0
;add bx,bx          2 0
;neg si             2 0
;add cx,[bx+si+h2]  4 2
;neg si             2 0
;sub cx,[bx+si+h2]  4 2  144 cycles so on par with imul but larger code, less BIU-friendly. Upside is wider choice of registers


;  add si,si
;  mov bx,[sinTable + ninetyDegrees + si]  ; xc
;  mov ax,bx
;  mov di,[_s]
;  mov cx,[bx+di]                          ; (xc+_s)^2/4
;  neg di                                  ; -_s
;  sub cx,[bx+di]                          ; (xc+_s)^2/4 - (xc-_s)^2/4 = xc*_s
;  mov bx,[sinTable + si]                  ; xs
;  mov si,[_zx]
;  sub cx,[bx+si]                          ; xc*_s - (xs+_zx)^2/4
;  neg si                                  ; -_zx
;  add cx,[bx+si]                          ; xc*_s - (xs+_zx)^2/4 + (xs-_zx)^2/4 = xc*_s - xs*_zx = s
;  push cx
;
;  mov cx,[bx+di]                          ; (xs-_s)^2/4
;  neg cx                                  ; -(xs-_s)^2/4   Can we arrange order of evaluation to eliminate this? (maybe zx before s and -xs*_zx before xc*_s)
;  neg di                                  ; _s
;  add cx,[bx+di]                          ; (xs+_s)^2/4 - (xs-_s)^2/4 = xs*_s
;  xchg ax,bx                              ; ax = xs, bx = xc
;  sub cx,[bx+si]                          ; xs*_s - (xc-_zx)^2/4
;  neg si                                  ; _zx
;  add cx,[bx+si]                          ; xs*_s + (xc+_zx)^2/4 - (xc-_zx)^2/4 = xs*_s + xc*_xz = zx
;  push cx
;
;  mov si,[_xy]
;  mov cx,[bx+si]                          ; (xs+_xy)^2/4
;  neg si                                  ; -_xy
;  sub cx,[bx+si]                          ; (xs+_xy)^2/4 - (xs-_xy)^2/4 = xs*_xy
;  xchg ax,bx                              ; ax = xc, bx = xs
;  mov di,[_yz]
;  add cx,[bx+di]                          ; xs*_xy + (xc+_yz)^2/4
;  neg di                                  ; -_yz
;  sub cx,[bx+di]                          ; xs*_xy + (xc+_yz)^2/4 - (xc-_yz)^2/4 = xs*_xy + xc*_yz = yz
;  mov es,cx
;
;  mov cx,[bx+di]                          ; (xs-_yz)^2/4
;  neg di                                  ; _xy
;  sub cx,[bx+di]                          ; (xs-_yz)^2/4 - (xs+_yz)^2/4 = -xs*_yz
;  xchg ax,bx                              ; ax = xs, bx = xc
;  sub cx,[bx+si]                          ; -xs*_yz - (xc-_xy)^2/4
;  neg si                                  ; _xy
;  add cx,[bx+si]                          ; -xs*_yz + (xc+_xy)^2/4 - (xc-_xy)^2/4 = -xs*_yz + xc*_xy = xy
;
;  add dx,dx
;  mov si,dx
;  mov bx,[sinTable + ninetyDegrees + si]  ; yc
;  mov ax,bx
;  mov di,cx                               ; xy
;  mov cx,[bx+di]                          ; (yc+xy)^2/4
;  neg di                                  ; -xy
;  sub cx,[bx+di]                          ; (yc+xy)^2/4 - (yc-xy)^2/4 = yc*xy
;  mov bx,[sinTable + si]                  ; ys
;  pop si                                  ; zx
;  add cx,[bx+si]                          ; yc*xy + (ys+zx)^2/4
;  neg si                                  ; -zx
;  sub cx,[bx+si]                          ; yc*xy + (ys+zx)^2/4 - (ys-zx)^2/4 = yc*xy + ys*zx = _xy
;  mov [_xy],cx
;
;  mov cx,[bx+di]                          ; (ys-xy)^2/4
;  neg di                                  ; xy
;  sub cx,[bx+di]                          ; -(ys+xy)^2/4 + (ys-xy)^2/4 = -ys*xy
;  xchg ax,bx                              ; ax = ys, bx = yc
;  sub cx,[bx+si]                          ; -ys*xy - (yc-zx)^2/4
;  neg si                                  ; zx
;  sub cx,[bx+si]                          ; -ys*xy + (yc+zx)^2/4 - (yc-zx)^2/4 = -ys*xy + yc*zx = _zx
;  mov [_zx],cx
;
;  mov si,es                               ; yz
;  mov cx,[bx+si]                          ; (yc+yz)^2/4
;  neg si                                  ; -yz
;  sub cx,[bx+si]                          ; (yc+yz)^2/4 - (yc-yz)^2/4 = yc*yz
;  xchg ax,bx                              ; ax = yc, bx = ys
;  pop di                                  ; s
;  add cx,[bx+di]                          ; yc*yz + (ys+s)^2/4
;  neg di                                  ; -s
;  sub cx,[bx+di]                          ; yc*yz + (ys+s)^2/4 - (ys-s)^2/4 = yc*yz + ys*s = _yz
;  mov [_yz],cx
;
;  mov cx,[bx+si]                          ; (ys-yz)^2/4
;  neg si                                  ; yz
;  sub cx,[bx+si]                          ; -(ys+yz)^2/4 + (ys-yz)^2/4 = -ys*yz
;  xchg ax,bx                              ; ax = ys, bx = yc
;  sub cx,[bx+di]                          ; -ys*yz - (yc-s)^2/4
;  neg di                                  ; s
;  add cx,[bx+di]                          ; -ys*yz + (yc+s)^2/4 - (yc-s)^2/4 = -ys*yz + yc*s = _s
;  mov [_s],cx


; Update rotor based on mouse delta
; Recalculate view matrix from rotor

; Input in SI and DX - can be positive or negative
rotate:
  mov ds,[quarterSquareTableSegment]
  add si,si
  mov bx,[sinTable + ninetyDegrees + si]  ; xc
  mov ax,bx
  mov di,[_zx]
  mov cx,[bx+di]                          ; (xc+_zx)^2/4
  neg di                                  ; -_zx
  sub cx,[bx+di]                          ; (xc+_zx)^2/4 - (xc-_zx)^2/4 = xc*_zx
  mov bx,[sinTable + si]                  ; xs
  mov si,[_s]
  add cx,[bx+si]                          ; xc*_zx + (xs+_s)^2/4
  neg si                                  ; -_s
  sub cx,[bx+si]                          ; xc*_zx + (xs+_s)^2/4 - (xs-_s)^2/4 = xc*_zx + xs*_s = zx
  mov es,cx

  mov cx,[bx+di]                          ; (xs-_zx)^2/4
  neg di                                  ; _zx
  sub cx,[bx+di]                          ; -(xs+_zx)^2/4 + (xs-_zx)^2/4 = -xs*_zx
  xchg ax,bx                              ; ax = xs, bx = xc
  sub cx,[bx+si]                          ; -xs*_zx - (xc-_s)^2/4
  neg si                                  ; _s
  add cx,[bx+si]                          ; -xs*_zx + (xc+_s)^2/4 - (xc-_s)^2/4 = -xs*_zx + xc*_s = s
  push cx

  mov si,[_xy]
  mov cx,[bx+si]                          ; (xs+_xy)^2/4
  neg si                                  ; -_xy
  sub cx,[bx+si]                          ; (xs+_xy)^2/4 - (xs-_xy)^2/4 = xs*_xy
  xchg ax,bx                              ; ax = xc, bx = xs
  mov di,[_yz]
  add cx,[bx+di]                          ; xs*_xy + (xc+_yz)^2/4
  neg di                                  ; -_yz
  sub cx,[bx+di]                          ; xs*_xy + (xc+_yz)^2/4 - (xc-_yz)^2/4 = xs*_xy + xc*_yz = yz
  push cx

  mov cx,[bx+di]                          ; (xs-_yz)^2/4
  neg di                                  ; _xy
  sub cx,[bx+di]                          ; (xs-_yz)^2/4 - (xs+_yz)^2/4 = -xs*_yz
  xchg ax,bx                              ; ax = xs, bx = xc
  sub cx,[bx+si]                          ; -xs*_yz - (xc-_xy)^2/4
  neg si                                  ; _xy
  add cx,[bx+si]                          ; -xs*_yz + (xc+_xy)^2/4 - (xc-_xy)^2/4 = -xs*_yz + xc*_xy = xy

  add dx,dx
  mov si,dx
  mov bx,[sinTable + ninetyDegrees + si]  ; yc
  mov ax,bx
  mov di,cx                               ; xy
  mov cx,[bx+di]                          ; (yc+xy)^2/4
  neg di                                  ; -xy
  sub cx,[bx+di]                          ; (yc+xy)^2/4 - (yc-xy)^2/4 = yc*xy
  mov bx,[sinTable + si]                  ; ys
  mov si,es                               ; yz
  add cx,[bx+si]                          ; yc*xy + (ys+zx)^2/4
  neg si                                  ; -zx
  sub cx,[bx+si]                          ; yc*xy + (ys+zx)^2/4 - (ys-zx)^2/4 = yc*xy + ys*zx = _xy
  mov [_xy],cx

  mov cx,[bx+di]                          ; (ys-xy)^2/4
  neg di                                  ; xy
  sub cx,[bx+di]                          ; -(ys+xy)^2/4 + (ys-xy)^2/4 = -ys*xy
  xchg ax,bx                              ; ax = ys, bx = yc
  sub cx,[bx+si]                          ; -ys*xy - (yc-zx)^2/4
  neg si                                  ; zx
  add cx,[bx+si]                          ; -ys*xy + (yc+zx)^2/4 - (yc-zx)^2/4 = -ys*xy + yc*zx = _zx
  mov [_zx],cx

  pop si                                  ; yz
  mov cx,[bx+si]                          ; (yc+yz)^2/4
  neg si                                  ; -yz
  sub cx,[bx+si]                          ; (yc+yz)^2/4 - (yc-yz)^2/4 = yc*yz
  pop di                                  ; s
  xchg ax,bx                              ; ax = yc, bx = ys
  add cx,[bx+di]                          ; yc*yz + (ys+s)^2/4
  neg di                                  ; -s
  sub cx,[bx+di]                          ; yc*yz + (ys+s)^2/4 - (ys-s)^2/4 = yc*yz + ys*s = _yz
  mov [_yz],cx

  mov cx,[bx+si]                          ; (ys-yz)^2/4
  neg si                                  ; yz
  sub cx,[bx+si]                          ; -(ys+yz)^2/4 + (ys-yz)^2/4 = -ys*yz
  xchg ax,bx                              ; ax = ys, bx = yc
  sub cx,[bx+di]                          ; -ys*yz - (yc-s)^2/4
  neg di                                  ; s
  add cx,[bx+di]                          ; -ys*yz + (yc+s)^2/4 - (yc-s)^2/4 = -ys*yz + yc*s = _s
  mov [_s],cx

normalize:
  mov ds,[squareTableSegment]
  mov si,cx                               ; _s
  mov ax,si
  mov di,[si]                             ; _s*_s = ss
  mov si,[_xy]                            ; _xy
  mov cx,[si]                             ; _xy*_xy = zz
  add di,cx                               ; ss + zz
  mov si,[_yz]                            ; _yz
  mov cx,[si]                             ; _yz*_yz = xx
  add di,cx                               ; ss + zz + xx
  mov si,[_zx]                            ; _zx
  mov cx,[si]                             ; _zx*_zx = yy
  add di,cx                               ; ss + zz + xx + yy
  mov bx,[di+normaliseTable]              ; normalisation row address
  mov ax,[bx+si]                          ; normalised _zx
  mov [_zx],ax
  mov si,[_yz]                            ; _yz
  mov cx,[bx+si]
  mov [_yz],cx
  mov si,[_xy]
  mov dx,[bx+si]
  mov [_xy],dx
  mov si,[_s]
  mov di,[bx+si]
  mov [_s],di

initViewMatrix:
  mov di,[di]                             ; _s*_s = ss



