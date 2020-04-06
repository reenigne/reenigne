org 0x100
cpu 8086

  push bp
  push ds
  push si
  push di
  mov bx,-8192
  mov ax,0xb800
  mov es,ax
  mov ds,ax
  ; TODO: set up initial bp, dx, si, di, cx
  ; TODO: jump to appropriate starting routine


vline_0_0:
  dec cx
  jz vline_done
vline_0_0_nocheckdone:
  mov al,[di+bx]
  and al,0x3f
  or al,0x80
  mov [di+bx],al
  add si,bp
  jle vline_0_1
  sub si,dx

vline_1_1:
  dec cx
  jz vline_done
  mov al,[di]
  and al,0xcf
  or al,0x20
  stosb
  add di,79
  add si,bp
  jle vline_1_0
  sub si,dx

vline_2_0:
  dec cx
  jz vline_done
  mov al,[di+bx]
  and al,0xf3
  or al,0x08
  mov [di+bx],al
  add si,bp
  jle vline_2_1
  sub si,dx

vline_3_1:
  dec cx
  jz vline_done
  mov al,[di]
  and al,0xfc
  or al,0x02
  stosb
  add di,79
  add si,bp
  jle vline_3_0
  sub si,dx
  loop vline_0_0_nocheckdone

vline_done:
  pop di
  pop si
  pop ds
  pop bp
  ret

vline_0_1:
  dec cx
  jz vline_done
vline_0_1_nocheckdone:
  mov al,[di]
  and al,0x3f
  or al,0x80
  stosb
  add di,79
  add si,bp
  jle vline_0_0
  sub si,dx

vline_1_0:
  dec cx
  jz vline_done
  mov al,[di+bx]
  and al,0xcf
  or al,0x20
  mov [di+bx],al
  add si,bp
  jle vline_1_1
  sub si,dx

vline_2_1:
  dec cx
  jz vline_done
  mov al,[di]
  and al,0xf3
  or al,0x08
  stosb
  add di,79
  add si,bp
  jle vline_2_0
  sub si,dx

vline_3_0:
  dec cx
  jz vline_done
  mov al,[di+bx]
  and al,0xfc
  or al,0x02
  mov [di+bx],al
  add si,bp
  jle vline_3_1
  sub si,dx
  loop vline_0_1_nocheckdone
  jmp vline_done
