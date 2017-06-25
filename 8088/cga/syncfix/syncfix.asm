org 0x100
cpu 8086

  JMP loader
_newint10:
  cmp ah,0
  jne done
  pushf
  call far[cs:int10save]
  push ax
  push dx
  mov dx,0x3d4
  mov ax,3
  out dx,ax
  pop dx
  pop ax
  iret
done:
  JMP far[cs:int10save]
int10save:
  DW 0,0
loader:
  mov ax,cs
  mov ds,ax
  XOR AX,AX
  MOV ES,AX
  MOV BX,word[es:0x40]
  MOV AX,word[es:0x42]
  MOV ES,AX
  MOV AX,word[es:BX]
  CMP AX,word[BX]
  JNE load
  MOV AH,9
  MOV DX,loaded
  INT 0x21
  MOV AH,0x4c
  INT 0x21
load:
  CLI
  XOR AX,AX
  MOV ES,AX
  MOV AX,word[es:0x40]
  MOV word[cs:int10save],AX
  MOV AX,word[es:0x42]
  MOV word[cs:int10save+2],AX
  MOV AX,CS
  MOV word[es:0x42],AX
  MOV AX,_newint10
  MOV word[es:0x40],AX
  STI
  MOV DX,loader
  ADD DX,0xf
  MOV CX,4
  SHR DX,CL
  MOV AX,0x3100
  INT 0x21

loaded:
  DB "This TSR is already loaded.",0xd,0xa,"$"
