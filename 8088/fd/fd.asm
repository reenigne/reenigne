cpu 8086
org 0


  xor ax,ax
  mov es,ax
  mov word[es:0x18],disk_int
  mov [es:0x1a],cs

  in al,0x21
  and al,0xbf
  out 0x21,al
  int 0x62
  sti

  mov al,'1'
  int 0x62


  mov ah,0
  mov dl,ah
  int 0x13
  test ah,0xff
  jz test2

  push ax
  mov al,'F'
  int 0x62
  pop ax
  int 0x60
  retf

test2:
  mov al,'2'
  int 0x62

  mov dx,0x3f2
  mov al,0x1c
  out dx,al
  sub cx,cx
motorWait1:
  loop motorWait1
motorWait2:
  loop motorWait2
  xor dx,dx
  mov es,dx
  mov ch,1
  mov byte[es:0x3e],dl
  call seek
  jnc test3

  mov al,'*'
  int 0x62
  retf

test3:
  mov al,'3'
  int 0x62

  mov ch,0x34
  call seek
  jnc test4

  mov al,'#'
  int 0x62
  retf

test4:
  mov al,'4'
  int 0x62

  mov al,0x0c
  mov dx,0x3f2
  out dx,al

  mov al,'^'
  int 0x62
  retf


nec_output:
  push dx
  push cx
  mov dx,0x3f4
  xor cx,cx
j23:
  in al,dx
  test al,0x40
  jz j25
  loop j23
j24:
  mov al,'T'
  int 0x62
  int 0x67
j25:
  xor cx,cx
j26:
  in al,dx
  test al,0x80
  jnz j27
  loop j26

  mov al,'t'
  int 0x62
  int 0x67
j27:
  mov al,ah
  mov dl,0xf5
  out dx,al
  pop cx
  pop dx
  ret


get_parm:
  push ds
  xor ax,ax
  mov ds,ax
  lds si,[es:0x78]
  shr bx,1
  mov ah,[si+bx]
  pop ds
  jc nec_output
  ret


seek:
  mov al,1
  push cx
  mov cl,dl
  rol al,cl
  pop cx
  test al,byte[es:0x3e]
  jnz j28
  or byte[es:0x3e],al
  mov ah,0x07

  mov al,'a'
  int 0x62

  call nec_output
  mov ah,dl

  mov al,'b'
  int 0x62

  call nec_output
  call chk_stat_2
  jc j32
j28:
  mov ah,0x0f

  mov al,'c'
  int 0x62

  call nec_output
  mov ah,dl

  mov al,'d'
  int 0x62

  call nec_output
  mov ah,ch

  mov al,'e'
  int 0x62

  call nec_output
  call chk_stat_2

  pushf
  mov bx,18
  call get_parm
  push cx
j29:
  mov cx,550
  or ah,ah
  jz j31
j30:
  loop j30
  dec ah
  jmp j29
j31:
  pop cx
  popf
j32:
  ret


chk_stat_2:
  call wait_int
  jc j34
  mov ah,8
  call nec_output
  call results
  jc j34
  mov al,[es:0x42]
  and al,0x60
  cmp al,0x60
  jz j35
  clc
j34:
  ret
j35:
  mov al,'S'
  int 0x62
  stc
  ret


wait_int:
  sti
  push bx
  push cx
  mov bl,2
  xor cx,cx
j36:
  test byte[es:0x3e],0x80
  jnz j37
  loop j36
  dec bl
  jnz j36
  mov al,'7'
  int 0x62
  stc
j37:
  pushf
  and byte[es:0x3e],0x7f
  popf
  pop cx
  pop bx
  ret


results:
  cld
  mov di,0x42
  push cx
  push dx
  push bx
  mov bl,7
j38:
  xor cx,cx
  mov dx,0x3f4
j39:
  in al,dx
  test al,0x80
  jnz j40a
  loop j39
  mov al,'8'
  int 0x62
j40:
  stc
  pop bx
  pop dx
  pop cx
  ret
j40a:
  in al,dx
  test al,0x40
  jnz j42
j41:
  mov al,'N'
  int 0x62
  jmp j40
j42:
  inc dx
  in al,dx
  mov [es:di],al
  inc di
  mov cx,10
j43:
  loop j43
  dec dx
  in al,dx
  test al,0x10
  jz j44
  dec bl
  jnz j38
  mov al,'9'
  int 0x62
  jmp j40
j44:
  pop bx
  pop dx
  pop cx
  ret


disk_int:
  sti
  push ds
  push ax
  mov al,'+'
  int 0x62
  xor ax,ax
  mov ds,ax
  or byte[0x3e],0x80
  mov al,0x20
  out 0x20,al
  pop ax
  pop ds
  iret

