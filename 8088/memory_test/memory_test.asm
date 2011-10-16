org 0

  ; Disable NMI
  xor al,al
  out 0xa0,al

  mov bx,0x4000
segmentLoop:
  mov es,bx
  xor si,si
offsetLoop:
  mov ah,0xff
  mov byte[es:si],ah
  mov al,byte[es:si]
  cmp al,ah
  jz okHigh

  int 0x60
  mov ax,es
  int 0x60
  mov ax,si
  int 0x60
  mov al,10
  int 0x62

okHigh:
  mov ah,0
  mov byte[es:si],ah
  mov al,byte[es:si]
  cmp al,ah
  jz okLow

  int 0x60
  mov ax,es
  int 0x60
  mov ax,si
  int 0x60
  mov al,10
  int 0x62

okLow:
  add si,1
  jnc offsetLoop

  mov ax,bx
  int 0x60
  mov al,10
  int 0x62

  add bx,0x1000
  cmp bx,0xa000
  jne segmentLoop
  retf

