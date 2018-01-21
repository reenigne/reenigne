  %include "../defaults_bin.asm"

  in al,0x61
  and al,0xfc
  or al,1
  out 0x61,al

  ; Enable auto-EOI
  mov al,0x13  ; ICW4 needed, not cascaded, call address interval 8, edge triggered
  out 0x20,al  ; Set ICW1
  mov al,0x08  ; Interrupt vector address
  out 0x21,al  ; Set ICW2
  mov al,0x0f  ; 8086/808 mode, auto-EOI, buffered mode/master, not special fully nested mode
  out 0x21,al  ; Set ICW4
  mov al,0xbc  ; Enable IRQs 0 (timer), 1 (keyboard) and 6 (floppy disk).
  out 0x21,al  ; Leave disabled 2 (EGA/VGA/slave 8259) 3 (COM2/COM4), 4 (COM1/COM3), 5 (hard drive, LPT2) and 7 (LPT1)


  xor ax,ax
  mov ds,ax
  mov word[8*4],irq0
  mov [8*4+2],cs

  xor ax,ax
doTest:
  push ax
  safeRefreshOff
  writePIT16 0, 2, 2    ; Ensure an IRQ0 is pending
  writePIT16 0, 2, 100  ; Queue an IRQ0 to execute from HLT
  sti
  hlt                   ; ACK first IRQ0
  hlt                   ; wait for second IRQ0
  writePIT16 0, 2, 0 ; Queue an IRQ0 for after the test in case of crash
  writePIT16 2, 2, 0        ; ***TIMING START***

  pop ax
  push ax

  mul ah

  mov al,0x80
  out 0x43,al               ; ***TIMING END***
  in al,0x42
  mov bl,al
  in al,0x42
  mov bh,al

  mov al,0x80
  out 0x43,al
  in al,0x42
  mov cl,al
  in al,0x42
  mov ch,al

  mov al,0x80
  out 0x43,al
  in al,0x42
  mov dl,al
  in al,0x42
  mov dh,al

  mov al,0x80
  out 0x43,al
  in al,0x42
  mov ah,al
  in al,0x42
  xchg ah,al

  add bx,ax
  add bx,cx
  add bx,dx

  push bx

  refreshOn

  pop ax
  neg ax
;  add al,'0'
  cmp al,32
  jae noSaturate
  mov al,32
noSaturate:
  outputCharacter

  pop ax
  inc ax
  jz done
  jmp doTest
done:
  complete

irq0:
  iret


