  ; Setup IRQ0

  xor ax,ax
  mov ds,ax
  getInterrupt 8, oldInterrupt8
  setInterrupt 8, interrupt8


  ; No interrupts except timer

  in al,0x21
  mov [oldimr],al
  mov al,0xfe
  out 0x21,al


  ; Set up timer 0 for sync with CRT

  writePIT16 0, 2, 76*262   ; 59.923Hz






  ; Restore IMR

  mov al,[cs:oldimr]
  out 0x21,al


  ; Put timer 0 back to 18.2Hz

  writePIT16 0, 2, 0


  ; Restore IRQ0

  xor ax,ax
  mov ds,ax
  restoreInterrupt 8, oldInterrupt8










oldInterrupt8: dw 0, 0
oldimr: db 0





  ; Synchronize with screen
  sti
  hlt
interrupt8:
  mov al,0x20
  out 0x20,al

