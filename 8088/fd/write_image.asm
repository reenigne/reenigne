cpu 8086
org 0


  mov ah,0  ; Subfunction 0 = Reset Disk System
  mov dl,0  ; Drive 0 (A:)
  int 0x13
  int 0x60  ; Output AX
  mov al,10
  int 0x62  ; New line






  mov ax,cs
  mov es,ax
  mov ds,ax


  ; Get drive parameters
  mov ah,8
  mov dl,0x80
  int 0x13
  pushf
  int 0x60      ; AX = 0000  no error
  mov ax,bx
  int 0x60      ; BX = 0000
  mov ax,cx
  int 0x60      ; CX = 3051  17 sectors per track (1..17), 305 cylinders (0..304)
  mov ax,dx
  int 0x60      ; DX = 0301  4 heads (0..3), 1 drive. Total = 17*305*4*512 = 10618880 bytes = 10370Kb = 10.12Mb (?)
  pop ax
  int 0x60      ; Flags = F246  CF=0 PF=1 AF=0 ZF=1 SF=0 TF=0 IF=1 DF=0 OF=0   Success
  mov al,10
  int 0x62

  mov al,cl
  and al,0x3f
  mov byte[sectors],al

  inc dh
  mov byte[heads],dh

  mov al,ch
  mov ah,cl
  mov cl,6
  shr ah,cl
  inc ax
  mov word[cylinders],ax
  int 0x60


  mov word[cylinder],0
cylinderLoop:

  mov byte[head],0
headLoop:

  mov byte[sector],1
sectorLoop:

  mov bx,buffer

  mov cx,10
retryLoop:
  push cx

  mov dh,byte[head]
  mov ax,word[cylinder]
  mov ch,al
  mov cl,6
  shl ah,cl
  mov cl,ah
  or cl,byte[sector]

  mov ax,word[cylinder]
  int 0x60
  mov al,byte[sector]
  mov ah,dh
  int 0x60
  mov al,10
  int 0x62

  mov al,1  ; Number of sectors to read
  mov dl,0x80  ; Drive number
  mov ah,2
  int 0x13
  pushf
  int 0x60      ; AX = 1000
  pop ax
  push ax
  int 0x60      ; Flags = F217  CF=1 PF=1 AF=1 ZF=0 SF=0 TF=0 IF=1 DF=0 OF=0   Failure
  mov al,10
  int 0x62
  popf
  jnc output

  pop cx
  loop retryLoop
  jmp nextSector

output:
  pop cx
  mov si,buffer
  mov cx,0x200
outputLoop:
  lodsb
  int 0x62
  loop outputLoop

nextSector:
  inc byte[sector]
  mov al,byte[sector]
  cmp al,byte[sectors]
  jle sectorLoop

  inc byte[head]
  mov al,byte[head]
  cmp al,byte[heads]
  jl headLoop

  inc word[cylinder]
  mov ax,word[cylinder]
  mov ax,word[cylinder]
  cmp ax,word[cylinders]
  jge finished
  jmp cylinderLoop
finished:
  retf

cylinders:
  dw 0
sectors:
  db 0
heads:
  db 0
cylinder:
  dw 0
sector:
  db 0
head:
  db 0

buffer:
