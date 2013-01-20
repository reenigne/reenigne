
%macro test 1
  lahf
  mov [cs:store+0x16],ah

  mov ax,0x0022
  mov ds,ax
  mov ax,0x4466
  mov es,ax
  mov ax,0x88aa
  mov ss,ax

  mov ax,0x0123
  mov bx,0x4567
  mov cx,0x89ab
  mov dx,0xcdef
  mov si,0x1133
  mov di,0x5577
  mov sp,0x99bb
  mov bp,0xddff
  db %1,2
  int 0x65

  mov [cs:store],ax
  mov [cs:store+2],bx
  mov [cs:store+4],cx
  mov [cs:store+6],dx
  mov [cs:store+8],si
  mov [cs:store+0x0a],di
  mov [cs:store+0x0c],sp
  mov [cs:store+0x0e],bp
  mov [cs:store+0x10],ds
  mov [cs:store+0x12],es
  mov [cs:store+0x14],ss

  lahf
  mov [cs:store+0x17],ah


  mov ax,[cs:store]
  int 0x63
  mov ax,[cs:store+2]
  int 0x63
  mov ax,[cs:store+4]
  int 0x63
  mov ax,[cs:store+6]
  int 0x63
  mov ax,[cs:store+8]
  int 0x63
  mov ax,[cs:store+0x0a]
  int 0x63
  mov ax,[cs:store+0x0c]
  int 0x63
  mov ax,[cs:store+0x0e]
  int 0x63
  mov ax,[cs:store+0x10]
  int 0x63
  mov ax,[cs:store+0x12]
  int 0x63
  mov ax,[cs:store+0x14]
  int 0x63
  mov ax,[cs:store+0x16]
  int 0x63

  mov al,10
  int 0x65
%endmacro

  mov ax,0
  sub ax,1

  test 0x60
  test 0x61
  test 0x62
  test 0x63
  test 0x64
  test 0x65
  test 0x66
  test 0x67
  test 0x68
  test 0x69
  test 0x6a
  test 0x6b
  test 0x6c
  test 0x6d
  test 0x6e
  test 0x6f

  test 0x70
  test 0x71
  test 0x72
  test 0x73
  test 0x74
  test 0x75
  test 0x76
  test 0x77
  test 0x78
  test 0x79
  test 0x7a
  test 0x7b
  test 0x7c
  test 0x7d
  test 0x7e
  test 0x7f

  mov al,26
  int 0x65
  hlt
store:





