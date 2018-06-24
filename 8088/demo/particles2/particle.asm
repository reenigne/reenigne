; Each particle has:
;   Colour
;   Position (fractional)
;   Velocity (fractional)
;   Last address on page 0
;   Last address on page 1
;
; Use ISAV mode
; Pass 1: Erase all old particles (just restore byte from background image)
; Pass 2: Move and redraw
;
; Unroll by particle and by page
; 346 cycles per particle -> ~200 particles
;


; si = particle data pointer
doParticle:
  mov ax,[si]  ; x position
  add ax,[si+2]  ; x velocity
  mov [si],ax
  mov cx,[si+4]  ; y position
  add cx,[si+6]  ; y velocity
  mov [si+4],cx
  mov bl,ch
  mov bh,yTable >> 8
  add bx,bx
  mov di,[bx]
  mov bl,ah
  mov bh,maskTable >> 8
  mov al,[bx]
  mov bh,xTable >> 8
  add bx,bx
  add di,[bx]


%define particleCount 100

; Erase particles on page 0

%assign i 0
%rep particleCount
  eraseOffset%i_0:
    mov di,1234
    mov al,[di]  ; Background image
    stosb        ; Erase
  %assign i i+1
%endrep

; Move/draw particles on page 0
%assign i 0
%rep particleCount
  xPosition%i_0:
    mov ax,1234
  xVelocity%i_0:
    add ax,5678
    mov [xPosition%i_1 + 1],ax
  yPosition%i_0:
    mov cx,1234
  yVelocity%i_0:
    add cx,5678  ; y velocity
    mov [yPosition%i_1 + 1],cx

    mov bl,ch
    mov bh,yTable >> 8
    add bx,bx
    mov di,[bx]
    mov bl,ah
    mov bh,maskTable >> 8
    mov al,[bx]
    mov bh,xTable >> 8
    add bx,bx
    add di,[bx]
    mov [eraseOffset%i_0 + 1],di
    mov ah,[di]  ; Background image
    and ah,al
    not al
    and al,99 ; Colour
    stosb  ; Draw
  %assign i i+1
%endrep

; Erase particles on page 1

%assign i 0
%rep particleCount
  eraseOffset%i_1:
    mov di,1234
    mov al,[di]  ; Background image
    stosb        ; Erase
  %assign i i+1
%endrep

; Move/draw particles on page 1
%assign i 0
%rep particleCount
  xPosition%i_1:
    mov ax,1234
  xVelocity%i_0:
    add ax,[5678
    mov [xPosition%i_1 + 1],ax
  yPosition%i_0:
    mov cx,1234
  yVelocity%i_0:
    add cx,5678  ; y velocity
    mov [yPosition%i_1 + 1],cx

    mov bl,ch
    mov bh,yTable >> 8
    add bx,bx
    mov di,[bx]
    mov bl,ah
    mov bh,maskTable >> 8
    mov al,[bx]
    mov bh,xTable >> 8
    add bx,bx
    add di,[bx]
    mov [eraseOffset%i_0 + 1],di
    mov ah,[di]  ; Background image
    and ah,al
    not al
    and al,99 ; Colour
    stosb  ; Draw
  %assign i i+1
%endrep

