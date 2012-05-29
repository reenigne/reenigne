push dx
push ax
mov dx,0x3d9
mov al,0x1e
out dx,al
pop ax
pop dx
iret
