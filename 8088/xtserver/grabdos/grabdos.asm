%include "../../defaults_com.asm"

start:
  initSerial

  mov ax,0
  mov ds,ax
  mov si,ax
  mov cx,ax
  mov dl,0x10

  cld
  mov al,4
  call printCharNoScreen
  mov al,cl
  call printCharEscaped
  mov al,ch
  call printCharEscaped
  mov al,dl
  call printCharEscaped
.loopTop:
  cmp cx,0
  jne .doByte
  cmp dl,0
  je .done
.doByte:
  cmp si,0xffff
  jne .normalized
  mov si,0x000f
  mov ax,ds
  add ax,0xfff
  mov ds,ax
.normalized:
  lodsb
  call printCharEscaped
  dec cx
  cmp cx,0xffff
  jne .loopTop
  dec dl
  jmp .loopTop
.done:

  mov ax,0x4c00
  int 0x21


printCharNoScreen:
  push dx
  mov dx,0x3f8 + 5
  sendByte
  pop dx
  ret

printCharEscaped:
  cmp al,0x5
  ja .checkForComplete
.escapeNeeded:
  push ax
  mov al,0
  call printCharNoScreen
  pop ax
.noEscapeNeeded:
  call printCharNoScreen
  ret
.checkForComplete:
  cmp al,0x1a
  je .escapeNeeded
  jmp .noEscapeNeeded

