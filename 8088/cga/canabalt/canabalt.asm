org 0x100
cpu 8086

buildingSize equ 100

  mov ax,4
  int 0x10
  mov dx,0x3d9
  mov al,0x30
  out dx,al

  mov ax,0xb800
  mov es,ax
  xor di,di
  mov si,background
  mov ax,cs
  mov ds,ax
  mov dx,100
  mov ax,0x2000-80
  mov bx,-0x2000
  mov ch,0
yloop:
  mov cl,40
  rep movsw
  add di,ax
  mov cl,40
  rep movsw
  add di,bx
  dec dx
  jnz yloop


timeLoop:
  ; Compute update buffer
  mov ax,[buildingLeftEdge]
  mov di,updateBuffer
  mov bx,cs
  mov es,bx
  mov cx,buildingSize



  mov dx,0x3da
waitForVerticalSync:
  in al,dx
  test al,8
  jz waitForVerticalSync       ;         jump if not +VSYNC, finish if +VSYNC


  mov ah,0
  int 0x16
  mov ax,3
  int 0x10
  ret

buildingLeftEdge: dw 319

backgroundLineTable:
%assign i 0
%rep 200
  dw background + i
  %assign i i+80
%endrep

background:
incbin "../../../../canabaltbg_vram.dat"

updateBuffer:
