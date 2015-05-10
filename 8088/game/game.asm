pitCyclesPerScanline equ 76
overScanLines        equ 62
overScanPerCycles    equ 76*62

offScreenHandler:
  mov al,0x20
  out 0x20,al

