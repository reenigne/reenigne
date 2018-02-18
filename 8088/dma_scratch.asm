; Start 4164 block refresh with compressed timing giving 513 cycles every 18432 (2.78% overhead) compared to normal overhead which is probably 4 cycles every 72 (5.56% overhead)

  mov al,TIMER1 | BOTH | MODE2 | BINARY
  out 0x43,al
  mov al,0
  out 0x41,al
  mov al,18
  out 0x41,al

  out 0x0c,al  ; clear byte pointer flip/flop
  mov al,0xff
  out 0x01,al  ;
  mov al,0
  out 0x01,al  ; Set count to 256
  out 0x00,al
  out 0x00,al  ; Set address to 0

  mov al,0x08  ; Memory-to-memory disable, Channel 0 address hold disable, controller enable, compressed timing, fixed priority, late write selection, DREQ sense active high, DACK sense active low
  out 0x08,al  ; DMA command write
  mov al,0x98  ; channel 0, read, autoinit, increment, block
  out 0x0b,al  ; DMA mode write


  ; The following code will crash the machine unless refresh worked
  mov cx,65535
.loop:
  xchg ax,cx
  mov cx,1;8
.loop2:
  loop .loop2
  xchg ax,cx
  loop .loop


; Put refresh back to normal

  mov al,0x00  ; Memory-to-memory disable, Channel 0 address hold disable, controller enable, normal timing, fixed priority, late write selection, DREQ sense active high, DACK sense active low
  out 0x08,al  ; DMA command write
  mov al,0x58  ; channel 0, read, autoinit, increment, single
  out 0x0b,al  ; DMA mode write
  mov al,TIMER1 | BOTH | MODE2 | BINARY
  out 0x43,al
  mov al,18
  out 0x41,al
  mov al,0
  out 0x41,al
  out 0x0c,al  ; clear byte pointer flip/flop
  mov al,0xff
  out 0x01,al  ;
  out 0x01,al  ; Set count to 65536


; block of 4 refresh cycles every scanline - 61 cycles for refresh (1 NOP), normal timing. No speaker PWM, 16 palettes changes 15 cycles (45 hdots) apart - 675 hdot wide active area, 237 hdot overscan

  mov dx,0x3d9
  mov al,TIMER1 | LSB | MODE2 | BINARY
  out 0x43,al
  mov al,76
  out 0x41,al
  out 0x0c,al  ; clear byte pointer flip/flop
  mov al,3
  out 0x01,al  ;
  mov al,0
  out 0x01,al  ; Set count to 4

  mov al,0x00  ; Memory-to-memory disable, Channel 0 address hold disable, controller enable, normal timing, fixed priority, late write selection, DREQ sense active high, DACK sense active low
  out 0x08,al  ; DMA command write
  mov al,0x98  ; channel 0, read, autoinit, increment, block
  out 0x0b,al  ; DMA mode write

  %assign i 0
  %rep 7
    %rep 16
      nop
      out dx,al
    %endrep
    mov al,i
    out 0x00,al
    mov al,(i >> 8)
    out 0x00,al
    %assign i i+4
    nop
  %endrep

  mov al,0x00  ; Memory-to-memory disable, Channel 0 address hold disable, controller enable, normal timing, fixed priority, late write selection, DREQ sense active high, DACK sense active low
  out 0x08,al  ; DMA command write
  mov al,0x58  ; channel 0, read, autoinit, increment, single mode
  out 0x0b,al  ; DMA mode write


; DMA memory-to-memory copy (same page only, have to deal with refresh separately)

  mov al,4     ; Page of source and (unfortunately) also destination
  out 0x83,al

  out 0x0c,al  ; clear byte pointer flip/flop
  mov al,0x1f
  out 0x01,al  ;
  mov al,0
  out 0x01,al  ; Set channel 0 count to 32

  out 0x00,al
  out 0x00,al  ; Set channel 0 address to 0

  mov al,0x40
  out 0x02,al
  mov al,0
  out 0x02,al  ; Set channel 1 address to 0x40

  mov al,0x1f
  out 0x03,al  ;
  mov al,0
  out 0x03,al  ; Set channel 1 count to 32

  mov al,0x0c  ; Set mask for channel 2 and 3, clear mask for channel 0 and 1
  out 0x0f,al

  mov al,0x01  ; Memory-to-memory enable, Channel 0 address hold disable, controller enable, normal timing, fixed priority, late write selection, DREQ sense active high, DACK sense active low
;  mov al,0x03  ; Memory-to-memory enable, Channel 0 address hold enable, controller enable, normal timing, fixed priority, late write selection, DREQ sense active high, DACK sense active low
  out 0x08,al  ; DMA command write
  mov al,0x88  ; channel 0, read, no autoinit, increment, block
  out 0x0b,al  ; DMA mode write
  mov al,0x85  ; channel 1, write, no autoinit, increment, block
  out 0x0b,al

  mov al,4
  out 0x09,al  ; request channel 0

; Put everything back again

  mov al,0x0f  ; Set all masks
  out 0x0f,al

  mov al,0x00  ; Memory-to-memory disable, Channel 0 address hold disable, controller enable, normal timing, fixed priority, late write selection, DREQ sense active high, DACK sense active low
  out 0x08,al  ; DMA command write
  mov al,0x58  ; channel 0, read, autoinit, increment, single
  out 0x0b,al  ; DMA mode write
  out 0x0c,al  ; clear byte pointer flip/flop
  mov al,0xff
  out 0x01,al  ;
  out 0x01,al  ; Set count to 65536

  mov al,0
  out 0x83,al

  mov al,0x0e  ; Set mask for channel 1, 2 and 3, clear mask for channel 0
  out 0x0f,al

