  mov ax,0x13
  int 0x10


; 28.322MHz 900 dots by 262 scanlines
; 900 dots == 227.5 cycles, 360 is 91,  90 pixels is 91 hdots
; Each cycle is


; BIOS mode                  13
; Bytes per CRTC character    4
;
; Horizontal displayed       80
; Horizontal start blank     80
; Horizontal start retrace   84
; Horizontal end retrace     96
; Horizontal end blank       98
; Horizontal total          100   225
;
; Vertical displayed        400
; Vertical start blank      441
; Vertical start retrace    412
; Vertical end retrace      414
; Vertical end blank        441
; Vertical total            449   262
;
; Horizontal dots           320
;
; Colours on screen         100
;
; total horizontal cycles   800  1800




  mov dx,0x3c4
  ; Reset Register  0x01
  ;      1 Fast Reset Command                               0 = reset.  1 = no reset.
  ;      2 Safe Reset Command                               0 = reset.  2 = no reset.
  mov ax,0100
  out dx,ax   ; Reset sequencer

  ; Clocking Mode Register                              01
  ;      1 Character Dot Clock Select                    1  0 = 9 pixels per character.  1 = 8 pixels per character.
  ;      4 Shift Load Two Control                        0  0 = Reload the video shift registers on each character clock.  1 = Reload the video shift registers on every second character clock permitting use of slow image memory chips.
  ;      8 Dot Clock Divide by Two Enable                0  0 = Normal pixel width.  1 = Double pixel width.
  ;   0x10 Shift Load Four Control                       0  0 = Reload the video shift registers as per bit 1.  1 = Reload the video shift registers on every fourth character clock permitting use of slow image memory chips.
  ;   0x20 Screen Inhibit                                0  0 = Normal.  1 = Blank screen for faster memory access.
  mov ax,0x0001
  out dx,ax

  ; Sequencer Memory Mode Register                      0e
  ;      2 Memory Size Specification                     2  0 = 64K image memory available.  1 = More than 64K memory available.
  ;      4 Processor Image Memory Access Mode Select     4  0 = Odd/even memory access mode.  1 = Normal memory access mode.
  ;      8 Processor Image Memory Access Mapping Select  8  0 = Memory planes accessed determined by Map Mask Register.  1 = Memory plane accessed determined using low 2 bits of address.
  mov ax,0x0604
  out dx,ax

  ; Miscellaneous Output Register                       63
  ;      1 Input Output Address Select                   1  0 = Use ports 3bx.  1 = Use ports 3dx.
  ;      2 Display Memory Access Enable                  2  0 = Disable processor access to display image memory.  1 = Enable processor access to display image memory.
  ;   0x0c Dot Clock Frequency Select                    0  0 = 25.175 MHz clock for 640 total Horizontal Pixels.  1 = 28.322 MHz clock for 720 total Horizontal Pixels.  2/3 = External clock up to a maximum of 65.000 MHz.
  ;   0x20 Address Extension Bit                        20  0 = Select the low 64K page.  1 = Select the high 64K page for the Text Memory Access Mode.
  ;   0xc0 Synchronisation Signal Polarity              c0  0 = 200 lines.  1 = 400 lines.  2 = 350 lines.  3 = 480 lines.
  mov dl,0xc2
  mov al,0xe7
  out dx,al
  mov dl,0xc4
  mov ax,0x300
  out dx,ax   ; Unreset sequencer
  mov dl,0xd4  ; CRTC port address low byte             d4  Corresponds to Input Output Address Select

  ;   0xff Horizontal Total Register                    5f  Horizontal Total - 5
  mov ax,0xdc00
  out dx,ax

  ;   0xff Horizontal Display Enable End Register       4f  Horizontal Display End - 1
  mov ax,0??01
  out dx,ax

  ;   0xff Start Horizontal Blanking Register           50  Start Horizontal Blanking
  mov ax,0??02
  out dx,ax

  ; End Horizontal Blanking Register                    82
  ;   0x1f End Horizontal Blanking                       2  End Horizontal Blanking bits 4..0
  ;   0x60 Display Enable Skew                           0  character clock delay
  ;   0x80 Light Pen Register Enable                    80  0 = Enable Lightpen Registers.  1 = Normal Operation.
  mov ax,0??03
  out dx,ax

  ;   0xff Start Horizontal Retrace Register            54  Start Horizontal Retrace Pulse
  mov ax,0??04
  out dx,ax

  ; End Horizontal Retrace Register                     80
  ;   0x1f End Horizontal Retrace                        0  End Horizontal Retrace  (only bottom 4 bits used?)
  ;   0x60 Horizontal Retrace Skew                       0  Horizontal Retrace Skew
  ;   0x80 End Horizontal Blanking Register bit 5       80  End Horizontal Blanking bit 5
  mov ax,0??05
  out dx,ax

  ;   0xff Vertical Total Register                      bf  Vertical Total - 2 bits 7..0
  mov ax,0x0406
  out dx,ax

  ; CRTC Overflow Register                              1f
  ;      1 Vertical Total bit 8                          1  Vertical Total bit 8
  ;      2 Vertical Display End bit 8                    2  Vertical Display End bit 8
  ;      4 Vertical Retrace Start bit 8                  4  Vertical Retrace Start bit 8
  ;      8 Start Vertical Blanking bit 8                 8  Start Vertical Blanking bit 8
  ;   0x10 Line Compare bit 8                           10  Line Compare bit 8
  ;   0x20 Vertical Total bit 9                          0  Vertical Total bit 9
  ;   0x40 Vertical Display End bit 9                    0  Vertical Display End bit 9
  ;   0x80 Vertical Retrace Start bit 9                  0  Vertical Retrace Start bit 9
  mov ax,0??07
  out dx,ax

  ; Maximum Scan Line Register                          41
  ;   0x1f Maximum Scan Lines                            1  Height of a text mode character.
  ;   0x20 Start Vertical Blanking bit 9                 0  Start Vertical Blanking bit 9
  ;   0x40 Line Compare Register bit 9                  40  Line Compare bit 9
  ;   0x80 Double Scan Line Display                      0  0 = Normal pixel height.  1 = Double pixel height.
  mov ax,0x4109
  out dx,ax

  ;   0xff Vertical Retrace Start Register              9c  Vertical Retrace Start bits 7..0
  mov ax,0??10
  out dx,ax

  ; Vertical Retrace End Register                       8e
  ;   0x0f Vertical Retrace End                         0e  Vertical Retrace End
  ;   0x10 Clear Vertical Interrupt                      0  0 = Clear the vertical interrupt state.  1 = Normal vertical interrupt state operation.
  ;   0x20 Enable Vertical Interrupt                     0  0 = Enable the vertical interrupt.  1 = Disable the vertical interrupt.
  ;   0x40 Refresh Bandwidth Select                      0  0 = Generate 3 image memory refresh cycles per horizontal scan line.  1 = Generate 5.
  ;   0x80 CRTC Register Protect                        80  0 = Enable writes to CRTC Regs at indexes 0..7.  1 = Disable writes to CRTC Regs at indexes 0..7.
  mov ax,0xce11
  out dx,ax

  ;   0xff Vertical Display Enable End Register         8f  Vertical Display Enable End bits 7..0
  mov ax,0??12
  out dx,ax

  ;   0xff Offset Register                              28  Display Image Offset. Image data width.
  mov ax,0x5a13
  out dx,ax

  ; Underline Location Register                         40
  ;   0x1f Underline Location                            0  Underline scan line for monochrome attribute mode.
  ;   0x20 Display Address Dwell Count 4                 0
  ;   0x40 Display Memory Address Unit 4                40
  mov ax,0x0014
  out dx,ax

  ;   0xff Start Vertical Blanking Register             96  Start Vertical Blanking bits 7..0
  mov ax,0??15
  out dx,ax

  ;   0xff End Vertical Blanking Register               b9  End Vertical Blanking
  mov ax,0??16
  out dx,ax

  ; CRTC Mode Control Register                          a3
  ;      1 Display Address 13 Remap Row 0                1  0 = Emulate CGA interlacing.  1 = No interlacing.
  ;      2 Display Address 14 Remap Row 1                2  0 = Emulate Hercules interlacing.  1 = No interlacing.
  ;      4 Horizontal Retrace Divisor                    0  0 = Normal operation.  1 = Double number of scan lines.
  ;      8 Display Address Dwell Count 2                 0
  ;   0x20 Display Address Unit 2 Remap                 20
  ;   0x40 Display Memory Address Unit 2                40
  ;   0x80 Horizontal/Vertical Retrace Reset            80  0 = Reset the horizontal and vertical retrace signals.  1 = Generate horizontal and vertical retrace signals.
  mov ax,0xe317
  out dx,ax

