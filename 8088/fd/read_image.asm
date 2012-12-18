cpu 8086
org 0

  mov ax,cs
  mov es,ax
  mov ds,ax


  ; Reset disk system
  mov ah,0
  mov dl,0
  int 0x13


;  ; Get disk parameters
;  mov ax,0x0201  ; Operation 2 = read disk sectors, 1 sector
;  mov bx,buffer
;  mov cx,0x0001  ; Track=0, sector=1
;  mov dx,0x0000  ; Head=0, drive=A:
;  int 0x13

  ; Assume 360K
  mov byte[sectors],9
  mov byte[heads],2
  mov word[cylinders],40


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
  mov dl,0  ; Drive number
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
