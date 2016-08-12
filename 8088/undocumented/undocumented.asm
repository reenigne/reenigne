  %include "../defaults_bin.asm"

  mov cx,64
loopTop:
  push cx
  mov ah,0
  sub ah,cl
  mov al,0xc4
  push ax
  call trial
  pop ax
  mov al,0xc5
  push ax
  call trial
  pop ax
  pop cx
  loop loopTop

;  mov cx,8
;loopTop2:
;  push cx
;  mov ah,0x40
;  sub ah,cl
;  mov al,0xff
;  push ax
;  call trial
;  pop ax
;  pop cx
;  loop loopTop2

  mov ax,0x38ff
  call trial
  mov ax,0x39ff
  call trial
  mov ax,0x3aff
  call trial
  mov ax,0x3bff
  call trial
  mov ax,0x3cff
  call trial
  mov ax,0x3dff
  call trial
  mov ax,0x3eff
  call trial
  mov ax,0x3fff
  call trial
  mov ax,0x78ff
  call trial
  mov ax,0x79ff
  call trial
  mov ax,0x7aff
  call trial
  mov ax,0x7bff
  call trial
  mov ax,0x7cff
  call trial
  mov ax,0x7dff
  call trial
  mov ax,0x7eff
  call trial
  mov ax,0x7fff
  call trial
  mov ax,0xb8ff
  call trial
  mov ax,0xb9ff
  call trial
  mov ax,0xbaff
  call trial
  mov ax,0xbbff
  call trial
  mov ax,0xbcff
  call trial
  mov ax,0xbdff
  call trial
  mov ax,0xbeff
  call trial
  mov ax,0xbfff
  call trial
  mov ax,0xf8ff
  call trial
  mov ax,0xf9ff
  call trial
  mov ax,0xfaff
  call trial
  mov ax,0xfbff
  call trial
  mov ax,0xfcff
  call trial
  mov ax,0xfdff
  call trial
  mov ax,0xfeff
  call trial
  mov ax,0xffff
  call trial

  mov ax,0x9090
  call trial

  complete

trial:
  cli
  mov [cs:data],ax
  mov [cs:data + 2],ax
  mov [cs:savedStack],sp
  mov [cs:savedStack+2],ss

  mov ax,0x89ab
  mov ds,ax
  mov ax,0x9acd
  mov es,ax

  mov ax,0x1234
  mov bx,0x2345
  mov cx,0x3579
  mov dx,0x4abc
  mov si,0x5678
  mov di,0x6996
  mov bp,0x7adf
  mov [bx+2],bx
  mov [bx],dx
  mov ax,word[bx+1]                ; data = 0x454a  address = 0x2346

;  mov bp,[bx]
  mov ax,0x1234
  mov ax,0x1234
  mov ax,0x1234
data:
  db 0xc5, 0xf1    ; lds si,ax     ; DS = [0x2348] = 0x0023   SI = 0x454a
  db 0xc5, 0xf1    ; lds si,ax     ; DS = [0x234A] = 0x0000   SI = 0x0023

  mov [cs:outRegs],ax
  mov [cs:outRegs + 2],cx
  mov [cs:outRegs + 4],dx
  mov [cs:outRegs + 6],bx
  mov [cs:outRegs + 8],sp
  mov [cs:outRegs + 10],bp
  mov [cs:outRegs + 12],si
  mov [cs:outRegs + 14],di
  mov [cs:outRegs + 16],es
  mov [cs:outRegs + 18],cs
  mov [cs:outRegs + 20],ss
  mov [cs:outRegs + 22],ds

  mov sp,[cs:savedStack]
  mov ss,[cs:savedStack+2]

  pushf
  pop ax
  mov [cs:outRegs + 24],ax
  sti

  mov ax,[cs:outRegs]
  outputHex
  outputCharacter ' '

  mov ax,[cs:outRegs + 2]
  outputHex
  outputCharacter ' '

  mov ax,[cs:outRegs + 4]
  outputHex
  outputCharacter ' '

  mov ax,[cs:outRegs + 6]
  outputHex
  outputCharacter ' '

  mov ax,[cs:outRegs + 8]
  outputHex
  outputCharacter ' '

  mov ax,[cs:outRegs + 10]
  outputHex
  outputCharacter ' '

  mov ax,[cs:outRegs + 12]
  outputHex
  outputCharacter ' '

  mov ax,[cs:outRegs + 14]
  outputHex
  outputCharacter ' '

  mov ax,[cs:outRegs + 16]
  outputHex
  outputCharacter ' '

  mov ax,[cs:outRegs + 18]
  outputHex
  outputCharacter ' '

  mov ax,[cs:outRegs + 20]
  outputHex
  outputCharacter ' '

  mov ax,[cs:outRegs + 22]
  outputHex
  outputCharacter ' '

  mov ax,[cs:outRegs + 24]
  outputHex
  outputCharacter 10

  ret

savedStack: dw 0,0
outRegs: dw 0,0,0,0, 0,0,0,0, 0,0,0,0, 0


; Output: 1234 2345 3579 4ABC 5678 6996 FFFC 7ADF 00A8 89AB 9ACD 9000
;         1234 2345 3579 4ABC F046 6996 FFFC 7ADF 00A8 0000 9ACD 9000
;         AX   BX   CX   DX   SI   DI   SP   BP   CS   DS   ES   SS
