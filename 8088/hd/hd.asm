org 0

  ; Init add-in ROMs

  mov dx,0xc800
romScanLoop:
  mov ax,dx
  int 0x60

  mov ds,dx
  cmp word[0],0xaa55
  jnz nextRom

  mov ax,0x40
  mov es,ax
  sub ah,ah
  mov al,[2]
  int 0x62

  mov cl,5
  shl ax,cl
  add dx,ax

  mov al,'*'
  int 0x62

  push dx
  mov word[es:0x67],3
  mov [es:0x69],ds
  call far[es:0x67]
  pop dx

  jmp areWeDone
nextRom:
  add dx,0x80
areWeDone:
  mov al,10
  int 0x62

  cmp dx,0xf600
  jl romScanLoop


  ; Get drive parameters
  mov ah,8
  mov dl,0x80
  int 0x13
  int 0x60      ; AX = 0000  no error
  mov ax,bx
  int 0x60      ; BX = 0000
  mov ax,cx
  int 0x60      ; CX = 3051  17 sectors per track (1..17), 305 cylinders (0..304)
  mov ax,dx
  int 0x60      ; DX = 0301  4 heads (0..3), 1 drive. Total = 17*305*4*512 = 10618880 bytes = 10370Kb = 10.12Mb (?)
  mov al,10
  int 0x62


  ;
readLoop:
  mov ax,cs
  mov es,ax
  mov ds,ax
  mov bx,buffer

  mov ah,2
  mov al,1  ; Number of sectors to read
  mov ch,0  ; Track number
  mov cl,1  ; Sector number
  mov dh,0  ; Head number
  mov dl,0x80  ; Drive number
  int 0x13

  mov si,buffer
  mov cx,0x200
outputLoop:
  lodsb
  int 0x62
  loop outputLoop

  retf

buffer:
