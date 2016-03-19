org 0
cpu 8086

mov dx,0x3d9
mov al,1
out dx,al
cli
hlt
