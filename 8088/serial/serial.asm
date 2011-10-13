org 0

  mov dx,0x3f8  ; COM1 (0x3f8 == COM1, 0x2f8 == COM2, 0x3e8 == COM3, 0x2e8 == COM4)

  ; dx + 0 == Transmit/Receive Buffer   (bit 7 of LCR == 0)  Baud Rate Divisor LSB (bit 7 of LCR == 1)
  ; dx + 1 == Interrupt Enable Register (bit 7 of LCR == 0)  Baud Rate Divisor MSB (bit 7 of LCR == 1)
  ; dx + 2 == Interrupt Identification Register IIR (read)   16550 FIFO Control Register (write)
  ; dx + 3 == Line Control Register LCR
  ; dx + 4 == Modem Control Register MCR
  ; dx + 5 == Line Status Register LSR
  ; dx + 6 == Modem Status Register MSR
  ; dx + 7 == Scratch Pad Register

  add dx,3    ; 3
  mov al,0x80
  out dx,al   ; Set LCR bit 7 to 1 to allow us to set baud rate

  dec dx      ; 2
  dec dx      ; 1
  mov al,0x00
  out dx,al   ; Set baud rate divisor high = 0x00

  dec dx      ; 0
  mov al,0x0c
  out dx,al   ; Set baud rate divisor low  = 0x0c = 9600 baud

  add dx,3    ; 3
  ; Line Control Register LCR                                03
  ;      1 Word length -5 low bit                             1
  ;      2 Word length -5 high bit                            2
  ;      4 1.5/2 stop bits                                    0
  ;      8 parity                                             0
  ;   0x10 even parity                                        0
  ;   0x20 parity enabled                                     0
  ;   0x40 force spacing break state                          0
  ;   0x80 allow changing baud rate                           0
  mov al,0x03
  out dx,al

  dec dx      ; 2
  dec dx      ; 1
  ; Interrupt Enable Register                                00
  ;      1 Enable data available interrupt and 16550 timeout  0
  ;      2 Enable THRE interrupt                              0
  ;      4 Enable lines status interrupt                      0
  ;      8 Enable modem status change interrupt               0
  mov al,0x00
  out dx,al

  add dx,3    ; 4
  ; Modem Control Register                                   00
  ;      1 Activate DTR                                       0
  ;      2 Activate RTS                                       0
  ;      4 OUT1                                               0
  ;      8 OUT2                                               0
  ;   0x10 Loop back test                                     0
  out dx,al

  ; Wait for a data byte to be available
  inc dx      ; 5
waitForData:
  in al,dx
  test al,1
  jz waitForData

  ; Read the data byte
  sub dx,5
  in al,dx
  add dx,5

  ; See if we've reached the end of the stream
  cmp al,0x1a
  je finished

  ; Print the character and continue
  int 0x65
  jmp waitForData
finished:
  retf
