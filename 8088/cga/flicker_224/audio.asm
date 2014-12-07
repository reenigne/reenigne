org 0x100
cpu 8086

PERIOD EQU (76*262/4)

  cli
  mov ax,cs
  mov ds,ax
  mov ss,ax
  mov sp,0xfffe

  ; Set up interrupt
  xor ax,ax
  mov ds,ax
  mov bx,[8*4]
  mov cx,[8*4+2]
  mov word[8*4],interrupt8
  mov [8*4+2],cs
  mov ax,cs
  mov ds,ax
  mov [oldInterrupt8],bx
  mov [oldInterrupt8+2],cx

  ; Set up PIT channel 2
  mov al,0xb6
  out 0x43,al
  mov al,2
  out 0x42,al
  mov al,0
  out 0x42,al

  ; Set up speaker
  in al,0x61
  mov [originalPortB],al
  or al,3
  out 0x61,al

  ; Set up PIT channel 0
  mov al,0x34
  out 0x43,al
  mov al,PERIOD & 0xff
  out 0x40,al
  mov al,PERIOD >> 8
  out 0x40,al

  sti

  mov si,musicStart

mainLoop:
  ; Check keyboard
  mov ah,1
  int 0x16
  cmp al,27
  je finish

  cmp byte[timerFired],1
  jne mainLoop
  mov byte[timerFired],0

  lodsw
  test ax,ax
  jz finish
  test si,si
  jz finish

  out 0x42,al
  mov al,ah
  out 0x42,al
  jmp mainLoop

finish:
  ; Restore everything
  cli

  mov bx,[oldInterrupt8]
  mov cx,[oldInterrupt8+2]
  xor ax,ax
  mov ds,ax
  mov [8*4],bx
  mov [8*4+2],cx
  mov ax,cs
  mov ds,ax

  mov al,0x36
  out 0x43,al
  mov al,0
  out 0x40,al
  out 0x40,al

  mov al,[originalPortB]
  out 0x61,al

  sti

  int 0x20
;  ret


interrupt8:
  push ax
  mov byte[cs:timerFired],1

  ; Restore and chain interrupt
  add word[cs:pitCount],PERIOD
  jc pitFallback
  mov al,0x20
  out 0x20,al
  pop ax
  iret
pitFallback:
  pop ax
  jmp far [cs:oldInterrupt8]

timerFired: db 0
oldInterrupt8: dw 0,0
pitCount: dw 0
originalPortB: db 0

musicStart:
