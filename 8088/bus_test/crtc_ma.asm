  %include "../defaults_bin.asm"

  mov dx,0x3d4
  mov ax,0x000c
  out dx,ax
  inc ax
  out dx,ax

  mov ax,0x0004
  out dx,ax
  mov ax,0x0005
  out dx,ax
  mov al,0x0009
  out dx,ax
  mov al,0x0008
  out dx,ax

  mov ax,cs
  mov es,ax
  mov ds,ax
  mov di,end
  mov si,di
  mov cx,0x4000
  mov dl,0xdc
looptop:
  in al,dx  ; 0x3dc: Activate light pen
  dec dx
  in al,dx  ; 0x3db: Clean light pen strobe
  mov dl,0xd4
  mov al,17
  out dx,al ; 0x3d4<-17: light pen low
  inc dx
  in al,dx  ; 0x3d5: register value
  stosb
  dec dx
  mov al,16
  out dx,al ; 0x3d4<-16: light pen high
  inc dx
  in al,dx  ; 0x3d5: register value
  stosb
  mov dl,0xdc
  loop looptop

  mov cx,0x4000
looptop2:
  lodsw
  printHex
  printCharacter ' '
  dec cx

  lodsw
  printHex
  printCharacter ' '
  dec cx

  lodsw
  printHex
  printCharacter ' '
  dec cx

  lodsw
  printHex
  printNewLine

  loop looptop2

  complete
end:

