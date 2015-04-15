  %include "../../defaults_bin.asm"

  initCGA 8

  lockstep2

  hlt

  mov cx,1000
  mov di,stash
  mov ax,cs
  mov es,ax

  mov dx,0x3dc
readPlace:
  mov dl,0xdc
  in al,dx  ; 0x3dc: Activate light pen
  dec dx
  in al,dx  ; 0x3db: Clean light pen strobe
  mov dl,0xd4
  mov al,16
  out dx,al ; 0x3d4<-16: light pen high
  inc dx
  in al,dx  ; 0x3d5: register value
  mov ah,al
  dec dx
  mov al,17
  out dx,al ; 0x3d4<-17: light pen low
  inc dx
  in al,dx  ; 0x3d5: register value
  stosw

  mov bl,1
  mov al,cl
  mul bl

  mov dl,0xda
  in al,dx
  stosw
  loop readPlace

  initCGA 8

  mov cx,2000
  mov si,stash
  mov ax,cs
  mov ds,ax
printPlace:
  lodsw
  printHex
  mov al,' '
  printCharacter
  loop printPlace

  hlt

stash:
