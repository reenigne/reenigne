%define particleCount 200

; Erase particles on page 0

origin:

%assign i 0
%rep particleCount
  eraseOffset%[i]_0:
    mov di,1234
    mov al,[di]  ; Background image
    stosb        ; Erase
  %assign i i+1
%endrep

; Move/draw particles on page 0
%assign i 0
%rep particleCount
  xPosition%[i]_0:
    mov ax,1234
  xVelocity%[i]_0:
    add ax,5678
    mov [xPosition%[i]_1 + 1],ax
  yPosition%[i]_0:
    mov cx,1234
  yVelocity%[i]_0:
    add cx,5678  ; y velocity
    mov [yPosition%[i]_1 + 1],cx

    mov bl,ch
    mov bh,(yTable0 - origin) >> 8
    add bx,bx
    mov di,[bx]
    mov bl,ah
    mov bh,(maskTable - origin) >> 8
    mov al,[bx]
    mov bh,(xTable - origin) >> 8
    add bx,bx
    add di,[bx]
    mov [eraseOffset%[i]_0 + 1],di
    mov ah,[di]  ; Background image
    and ah,al
    not al
    and al,99 ; Colour
    or al,ah
    stosb  ; Draw
  %assign i i+1
%endrep

; Erase particles on page 1

%assign i 0
%rep particleCount
  eraseOffset%[i]_1:
    mov di,1234
    mov al,[di]  ; Background image
    stosb        ; Erase
  %assign i i+1
%endrep

; Move/draw particles on page 1
%assign i 0
%rep particleCount
  xPosition%[i]_1:
    mov ax,1234
  xVelocity%[i]_1:
    add ax,5678
    mov [xPosition%[i]_1 + 1],ax

    cmp ax,160*256
    jae rePosition%[i]

  yPosition%[i]_1:
    mov cx,1234
    add cx,[yVelocity%[i]_0 + 2]  ; y velocity
    mov [yPosition%[i]_1 + 1],cx

    cmp cx,100*256
    jae rePosition%[i]

    mov bl,ch
    mov bh,(yTable1 - origin) >> 8
    add bx,bx
    mov di,[bx]
    mov bl,ah
    mov bh,(maskTable - origin) >> 8
    mov al,[bx]
    mov bh,(xTable - origin) >> 8
    add bx,bx
    add di,[bx]
    mov [eraseOffset%[i]_1 + 1],di
    mov ah,[di]  ; Background image
    and ah,al
    not al
    and al,99 ; Colour
    or al,ah
    stosb  ; Draw

    jmp noRePosition%[i]
  rePosition%[i]:
    ; TODO
  noRePosition%[i]:
  %assign i i+1
%endrep

align 256


yTable0:
%assign i 0
%rep 100
    dw i*80
  %assign i i+1
%endrep
times 156 dw 8000

yTable1:
%assign i 0
%rep 100
    dw i*80 + 0x2000
  %assign i i+1
%endrep
times 156 dw 8000

maskTable:
%assign i 0
%rep 80
    db 0xf0, 0x0f
%endrep
times 96 dw 0

xTable:
%assign i 0
%rep 80
    db 0xf0, 0x0f
  %assign i i+1
%endrep
times 96 dw 0

