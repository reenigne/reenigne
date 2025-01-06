  mov bl,112
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
  mov al,0
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

