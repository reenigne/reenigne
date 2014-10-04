mov ax,[si]
sub al,ah
sub al,[si-1]
sub al,[si+40]
sub al,[si-40]
shr al,1
shr al,1
mov ah,[si+40*25]
add ah,al
mov [si+40*25],ah
shr ah,1
shr ah,1
mov bl,[si+2*40*25]
add bl,ah
mov [si+2*40*25],bl
mov ax,[bx]
stosw

261 cycles per iteration = 305 iterations per frame = 18 fps




mov al,bh   2 0
out dx,al   1 1
mov al,bl   2 0
out dx,al   1 1
in al,0x60  2 1
stosb       1 1  total 13 IOs,


xchg ax,bx    ; bx=?, ax=0xcc
out dx,al     ; dx=0x61
xchg ax,cx    ; cx=0xcc, ax=0x4c
out dx,al
xchg ax,bx    ; ax=?, bx=0x4c
in al,0x60
stosb         ; es:di=destination pointer
xchg ax,cx    ; cx=?, ax=0xcc
out dx,al
xchg ax,bx    ; bx=0xcc, ax=0x4c
out dx,al
xchg ax,cx    ; cx=0x4c, ax=?
in al,0x60
stosb


