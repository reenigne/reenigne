cpu 8086

; initCGA m
; m = mode register value:
; 0x08 = 40x25 text, colour, bright background
; 0x09 = 80x25 text, colour, bright background
; 0x0a = 320x200 graphics, colour
; 0x0c = 40x25 text, B/W, bright background
; 0x0d = 80x25 text, B/W, bright background
; 0x0e = 320x200 graphics, B/W
; 0x1a = 640x200 graphics, colour
; 0x1e = 640x200 graphics, B/W
; 0x28 = 40x25 text, colour, blinking
; 0x29 = 80x25 text, colour, blinking
; 0x2c = 40x25 text, B/W, blinking
; 0x2d = 80x25 text, B/W, blinking
%macro initCGA 1
  %if (%1 & 0x10) != 0
    initCGA %1, 0x0f
  %else
    initCGA %1, 0
  %endif
%endmacro

; initCGA m. p
; p = palette register value:
; 0x00..0x0f = background/green/red/brown
; 0x10..0x1f = background/light green/light red/yellow
; 0x20..0x2f = background/cyan/magenta/light grey
; 0x30..0x3f = background/light cyan/light magenta/white
%macro initCGA 2
  %if (%1 & 2) != 0
    initCGA %1, 0x0f, 2
  %else
    initCGA %1, 0, 8
  %endif
%endmacro

; initCGA m, p, l
; l = scanlines per character
%macro initCGA 3
  ; Mode
  ;      1 +HRES
  ;      2 +GRPH
  ;      4 +BW
  ;      8 +VIDEO ENABLE
  ;   0x10 +1BPP
  ;   0x20 +ENABLE BLINK
  mov dx,0x3d8
  mov al,%1
  out dx,al

  ; Palette
  ;      1 +OVERSCAN B
  ;      2 +OVERSCAN G
  ;      4 +OVERSCAN R
  ;      8 +OVERSCAN I
  ;   0x10 +BACKGROUND I
  ;   0x20 +COLOR SEL
  mov dx,0x3d9
  mov al,%2
  out dx,al

  mov dx,0x3d4

  ;   0xff Horizontal Total                             38 71
  %if (%1 & 1) != 0
    mov ax,0x7100
  %else
    mov ax,0x3800
  %endif
  out dx,ax

  ;   0xff Horizontal Displayed                         28 50
  %if (%1 & 1) != 0
    mov ax,0x5001
  %else
    mov ax,0x2801
  %endif
  out dx,ax

  ;   0xff Horizontal Sync Position                     2d 5a
  %if (%1 & 1) != 0
    mov ax,0x5a02
  %else
    mov ax,0x2d02
  %endif
  out dx,ax

  ;   0x0f Horizontal Sync Width                              0a
  mov ax,0x0a03
  out dx,ax

  ;   0x7f Vertical Total                                        1f 7f
  %if %3 == 2
    mov ax,0x7f04
  %else
    mov ax,4 | (((262 / %3) - 1) << 8)
  %endif
  out dx,ax

  ;   0x1f Vertical Total Adjust                              06
  %if %3 == 2
    mov ax,0x0605
  %else
    mov ax,5 | ((262 % %3) << 8)
  %endif
  out dx,ax

  ;   0x7f Vertical Displayed                                    19 64
  %if %3 == 2
    mov ax,0x6406
  %else
    mov ax,6 | ((200 / %3) << 8)
  %endif
  out dx,ax

  ;   0x7f Vertical Sync Position                                1c 70
  %if %3 == 2
    mov ax,0x7007
  %else
    mov ax,7 | ((224 / %3) << 8)
  %endif
  out dx,ax

  ;   0x03 Interlace Mode                                     02
  mov ax,0x0208
  out dx,ax

  ;   0x1f Max Scan Line Address                                 07 01
  mov ax,9 | ((%3 - 1) << 8)
  out dx,ax

  ; Cursor Start                                              06
  ;   0x1f Cursor Start                                        6
  ;   0x60 Cursor Mode                                         0
  mov ax,0x060a
  out dx,ax

  ;   0x1f Cursor End                                         07
  mov ax,0x070b
  out dx,ax

  ;   0x3f Start Address (H)                                  00
  mov ax,0x000c
  out dx,ax

  ;   0xff Start Address (L)                                  00
  mov ax,0x000d
  out dx,ax

  ;   0x3f Cursor (H)                                         03  0x3c0 == 40*24 == start of last line
  mov ax,0x030e
  out dx,ax

  ;   0xff Cursor (L)                                         c0
  mov ax,0xc00f
  out dx,ax
%endmacro


; Assumes DS == 0
%macro setInterrupt 2
  mov word [%1*4], %2
  mov [%1*4 + 2], cs
%endmacro

; Assumes DS == 0
%macro getInterrupt 2
  mov ax, word [%1*4]
  mov [cs:%2], ax
  mov ax, word [%1*4 + 2]
  mov [cs:%2 + 2], ax
%endmacro

; Assumes DS == 0
%macro restoreInterrupt 2
  mov ax, [cs:%2]
  mov word [%1*4], ax
  mov ax, [cs:%2 + 2]
  mov word [%1*4 + 2], ax
%endmacro


%macro initSerial 0
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
  mov al,0x01 ; (0x02 = 57600 baud)
  out dx,al   ; Set baud rate divisor low  = 0x01 = 115200 baud

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
%endmacro


; Receive a byte over serial and put it in AL. DX == port base address + 5
%macro receiveByte 0
    ; Wait until a byte is available
  %%waitForData:
    in al,dx
    test al,1
    jz %%waitForData
    ; Read the data byte
    sub dx,5
    in al,dx
    add dx,5
%endmacro


; Send byte in AL over serial. DX == port base address + 5
%macro sendByte 0
    mov ah,al
  %%waitForSpace:
    in al,dx
    test al,0x20
    jz %%waitForSpace
    inc dx
  %%waitForDSR:
    in al,dx
    test al,0x20
    jz %%waitForDSR
    ; Write the data byte
    sub dx,6
    mov al,ah
    out dx,al
    add dx,5
%endmacro


%macro refreshOff 0
  mov al,0x60  ; Timer 1, write LSB, mode 0, binary
  out 0x43,al
  mov al,0x01  ; Count = 0x0001 so we'll stop almost immediately
  out 0x41,al
%endmacro


%macro lockstep 0
  cli

  refreshOff

  ; Set "stosb" destination to be CGA memory
  mov ax,0xb800
  mov es,ax
  mov di,80*100

  ; Set argument for MUL
  mov cl,1

  ; Ensure "stosb" won't take us out of video memory
  cld

  ; Go into CGA lockstep. The delays were determined by trial and error.
  jmp $+2      ; Clear prefetch queue
  stosb        ; From 16 down to 3 possible CGA/CPU relative phases.
  mov al,0x01
  mul cl
  stosb        ; Down to 2 possible CGA/CPU relative phases.
  mov al,0x7f
  mul cl
  stosb        ; Down to 1 possible CGA/CPU relative phase: lockstep achieved.

  mov dx,0x03d8
  mov al,0x0a
  out dx,al

  ; Set up CRTC for 1 character by 2 scanline "frame". This gives us 2 lchars
  ; per frame.
  mov dl,0xd4
  ;   0xff Horizontal Total
  mov ax,0x0000
  out dx,ax
  ;   0xff Horizontal Displayed                         28
  mov ax,0x0101
  out dx,ax
  ;   0xff Horizontal Sync Position                     2d
  mov ax,0x2d02
  out dx,ax
  ;   0x0f Horizontal Sync Width                        0a
  mov ax,0x0a03
  out dx,ax
  ;   0x7f Vertical Total                               7f
  mov ax,0x0104
  out dx,ax
  ;   0x1f Vertical Total Adjust                        06
  mov ax,0x0005
  out dx,ax
  ;   0x7f Vertical Displayed                           64
  mov ax,0x0106
  out dx,ax
  ;   0x7f Vertical Sync Position                       70
  mov ax,0x0007
  out dx,ax
  ;   0x03 Interlace Mode                               02
  mov ax,0x0208
  out dx,ax
  ;   0x1f Max Scan Line Address                        01
  mov ax,0x0009
  out dx,ax

  times 512 nop
  nop

  ; To get the CRTC into lockstep with the CGA and CPU, we need to figure out
  ; which of the two possible CRTC states we're in and switch states if we're
  ; in the wrong one by waiting for an odd number of lchars more in one code
  ; path than in the other. To keep CGA and CPU in lockstep, we also need both
  ; code paths to take the same time mod 3 lchars, so we wait 3 lchars more on
  ; one code path than on the other.
  mov dl,0xda
  in al,dx
  jmp $+2
  test al,1
  jz %%shortPath
  times 2 nop
  jmp $+2
%%shortPath:

%endmacro


%macro waitForDisplayEnable 0
  %%waitForDisplayEnable
    in al,dx                       ; 1 1 2
    test al,1                      ; 2 0 2
    jnz %%waitForDisplayEnable     ; 2 0 2
%endmacro

%macro waitForDisplayDisable 0
  %%waitForDisplayDisable
    in al,dx                       ; 1 1 2
    test al,1                      ; 2 0 2
    jz %%waitForDisplayDisable     ; 2 0 2
%endmacro

%macro waitForVerticalSync 0
  %%waitForVerticalSync
    in al,dx
    test al,8
    jz %%waitForVerticalSync
%endmacro

%macro waitForNoVerticalSync 0
  %%waitForNoVerticalSync
    in al,dx
    test al,8
    jnz %%waitForNoVerticalSync
%endmacro


; writePIT16 <timer> <mode> <value>
; timer 0 = IRQ0, BIOS time-of-day (default value 0, default mode ?)
; timer 1 = DRAM refresh DMA (default value 18, default mode ?)
; timer 2 = PC speaker/cassette
; mode 0 = interrupt on terminal count
; mode 1 = programmable one-shot
; mode 2 = rate generator
; mode 3 = square wave rate generator
; mode 4 = software triggered strobe
; mode 5 = hardware triggered strobe
; value = 13125000Hz/(11*frequency), or 0 for 18.2Hz
%macro writePIT16 3
  mov al,(%1 << 6) | 0x30 | (%2 << 1)  ; Don't use BCD mode
  out 0x43,al
  mov al,%3 & 0xff
  out 0x40 + %1,al
  mov al,%3 >> 8
  out 0x40 + %1,al
%endmacro

; readPIT16 <timer>
; Value returned in AX
%macro readPIT16 1
  mov al,%1 << 6
  out 0x43,al
  in al,0x40 + %1
  mov ah,al
  in al,0x40 + %1
  xchg ah,al
%endmacro


