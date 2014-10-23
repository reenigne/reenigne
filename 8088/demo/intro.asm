  add si,9999   ; 4 0
  mov bx,si     ; 2 0
  mov bl,99     ; 3 0
  add al,[bx]   ; 2 1     12


1: Use mod player code - can't synchronize with CGA though
2: Use sample-per-scanline and fake chords
3: Custom square wave player

  add si,9999
  jns p1
  inc ax
p1:


  xor ax,ax

  add si,[bx]   ; 2 2
  lahf          ; 1 0
  rcl ah,1      ; 3 0
  adc ax,bx     ; 2 0    10

  add di,[bx+2] ; 3 2
  lahf          ; 1 0
  rcl ah,1      ; 3 0
  adc ax,bx     ; 2 0    11

  add bp,[bx+4] ; 3 2
  lahf          ; 1 0
  rcl ah,1      ; 3 0
  adc ax,bx     ; 2 0    11

  add cx,[bx+6] ; 3 2
  lahf          ; 1 0
  rcl ah,1      ; 3 0
  adc ax,bx     ; 2 0    11

  xlatb
  out 0x42,al

  pop ax
  out dx,al



100 scanlines of raster bars
162 scanlines for scrolltext

280 bytes to copy per frame





  add si,9999   ; 4 0
  mov bx,si     ; 2 0
  mov bl,99     ; 3 0
  add al,[bx]   ; 2 1     12





; This is the version I emailed on the 26th of August 2014. Three bytes transferred per raster scanline.

  add sp,[bx]   ; 2 2
  lahf          ; 1 0
  rcl ah,1      ; 3 0
  sbb ax,ax     ; 2 0    10

  add di,[bx+2] ; 3 2
  lahf          ; 1 0
  rcl ah,1      ; 3 0
  adc ax,bx     ; 2 0    11

  add bp,[bx+4] ; 3 2
  lahf          ; 1 0
  rcl ah,1      ; 3 0
  adc ax,bx     ; 2 0    11

  add cx,[bx+6] ; 3 2
  lahf          ; 1 0
  rcl ah,1      ; 3 0
  adc ax,bx     ; 2 0    11

  xlatb
  out 0xe0,al

  mov al,[ss:bx+12]
  out dx,al

  xchg ax,di
  mov di,5678
  movsw
  movsb
  xchg ax,di                        ; 303 +104  304 +016  303 +104  304 +016  303 +104



; 5 bytes is too many for non-raster scanlines

  add sp,[bx]   ; 2 2
  lahf          ; 1 0
  rcl ah,1      ; 2 0
  sbb ax,ax     ; 2 0    9

  add di,[bx+2] ; 3 2
  lahf          ; 1 0
  rcl ah,1      ; 2 0
  adc ax,bx     ; 2 0    10

  add bp,[bx+4] ; 3 2
  lahf          ; 1 0
  rcl ah,1      ; 2 0
  adc ax,bx     ; 2 0    10

  add cx,[bx+6] ; 3 2
  lahf          ; 1 0
  rcl ah,1      ; 2 0
  adc ax,bx     ; 2 0    10

  xlatb
  out 0xe0,al

  xchg ax,di
  mov di,5678
  movsw
  movsw
  movsb
  xchg ax,di                        ; 318 +058  +054  +058  +054  +058





; Version using CMP trick

  add sp,[bx]    ; 2 2
  cmp sp,0x8000  ; 4 0
  sbb ax,ax      ; 2 0

  add di,[bx+2]  ; 3 2
  cmp di,0x8000  ; 4 0
  adc ax,bx      ; 2 0    11

  add bp,[bx+4]  ; 3 2
  cmp bp,0x8000  ; 4 0
  adc ax,bx      ; 2 0    11

  add cx,[bx+6]  ; 3 2
  cmp cx,0x8000  ; 4 0
  adc ax,bx      ; 2 0    11

  xlatb
  out 0xe0,al

  mov al,[ss:bx+12]
  out dx,al

  xchg ax,di
  mov di,5678
  movsw
  movsb
  xchg ax,di                        ; 322 +115  323 +004  +004  +004  +004



; Version using CMP trick with DX switching

  mov dx,0x8000

  add sp,[bx]   ; 2 2
  cmp sp,dx
  sbb ax,ax     ; 2 0    10

  add di,[bx+2] ; 3 2
  cmp di,dx
  adc ax,bx     ; 2 0    11

  add bp,[bx+4] ; 3 2
  cmp bp,dx
  adc ax,bx     ; 2 0    11

  add cx,[bx+6] ; 3 2
  cmp cx,dx
  adc ax,bx     ; 2 0    11

  xlatb
  out 0xe0,al

  mov al,[ss:bx+12]
  mov dx,0x3d9
  out dx,al

  xchg ax,di
  mov di,5678
  movsw
  movsb
  xchg ax,di                        ; 313 +096  +096  +096  +096  +096



; Version using CMP trick with DX double-duty (low 10 bits of DX = CGA palette port, high 6 bits of DX = pulse width
; Best so far - 4 bytes per scanline (6 for non-raster lines) = max 1172 bytes transferred per frame

  add sp,[bx]   ; 2 2
  cmp sp,dx
  sbb ax,ax     ; 2 0    10

  add di,[bx+2] ; 3 2
  cmp di,dx
  adc ax,bx     ; 2 0    11

  add bp,[bx+4] ; 3 2
  cmp bp,dx
  adc ax,bx     ; 2 0    11

  add cx,[bx+6] ; 3 2
  cmp cx,dx
  adc ax,bx     ; 2 0    11

  xlatb
  out 0xe0,al

  mov al,[ss:bx+12]
  out dx,al

  xchg ax,di
  mov di,5678
  movsw
  movsw
  xchg ax,di                        ; 304 +000  +000  303 +119  304 +000  +000



; Experiment for Sid player like inner loop - too many cycles for Intro

  xor ax,ax

  add si,[bp]
  mov bx,si
  mov bl,99
  mov al,[bx]

  add di,[bp+2]
  mov bx,di
  mov bl,99
  add al,[bx]

  add cx,[bp+4]
  mov bx,cx
  mov bl,99
  add al,[bx]

  add sp,[bp+6]
  mov bx,sp
  mov bl,99
  add al,[bx]

  out 0xe0,al

  out dx,al                         ; 247 +000  +000  +000  +000  +000






Experiments for using conditional branches. Quite fast, but timing is too unpredictable

  xor ax,ax

  add si,[bx]   ; 2 2
  js .p1
  inc ax
.p1:

  add di,[bx+2] ; 3 2
  js .p2
  inc ax
.p2:

  add bp,[bx+4] ; 3 2
  js .p3
  inc ax
.p3:

  add cx,[bx+6] ; 3 2
  js .p4
  inc ax
.p4:

  xlatb
  out 0xe0,al

  pop ax
  out dx,al                         ; 190 +000  +000  +000  +000  +000
                                    ; 228 +000  +000  +000  +000  +000




