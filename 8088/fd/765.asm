  %include "../defaults_bin.asm"

; Setup interrupts
  cli
  xor ax,ax
  mov ds,ax
  getInterrupt 8, oldInterrupt8
  setInterrupt 8, interrupt8
  getInterrupt 0x0e, oldInterruptE
  setInterrupt 0x0e, interruptE

  mov ax,cs
  mov ds,ax

  print "Floppy disk control program",10

  writePIT16 0, 2, 0

; Reset the 765.
  print "Resetting 765",10
  call resetFDC

  call commandVersion

  call commandSpecify

  call motorWait

  mov ax,commandRecalibrate
  call synchronousDisk

  call settleWait

  mov word[dma_length], 0x2000
  call setupDMA

  call breakFormat


  print "Writing data",10

  mov byte[dma_mode],0x4a
  call startDMA

  mov al,0
  mov es,[dma_pointer+2]
  mov di,[dma_pointer]
  mov cx,8192
loop1:
  stosb
  inc al
  loop loop1

  mov byte[command_r],1
  call commandWriteData

  cli
  mov di,6095+80+55 +80+80
  call dmaWait
  call resetFDC
;  mov ax,commandWriteData
;  call synchronousDisk


  print "Reading data",10

  mov byte[dma_mode],0x46
  call startDMA
  mov ax,commandReadData
  call synchronousDisk


  print "Sending data",10

  mov cx,[dma_length]
  printCharacter 0
  printCharacter cl
  printCharacter ch
  printCharacter 0
  mov si,[dma_pointer]
  mov ds,[dma_pointer+2]
  printString

  print "Data sent",10

  mov dx,0x03f2
  mov al,0x0c  ; Motor A off, DMA/INT enabled, FDC not reset, drive=A
  out dx,al

  complete


; Format a track with a single 8192-byte sector.
; Interrupt the format before it is complete so that the track and sector
; headers are not overwritten by data.
breakFormat:
  print "Performing break-format",10
  mov byte[dma_mode],0x4a
  call startDMA

  mov es,[dma_pointer+2]
  mov di,[dma_pointer]
  mov ax,0x0000  ; H C
  stosw
  mov ax,0x0601  ; N R
  stosw

  ; Start the format
  mov byte[command_n],6
  mov byte[command_sc],1
  call commandFormatTrack

  ; Wait until the FDC retreives the format data
  mov di,4
  call dmaWait

  ; Wait a bit longer so we're sure that we've finished the sector header but
  ; but that we haven't spun all the way around and overwritten the track
  ; header.
  mov bx,[time]
waitLoop:
  mov cx,[time]
  sub cx,bx
  cmp cx,2   ; 54-108ms = 27%-54% of a revolution
  jb waitLoop

  call resetFDC
  ret


resetFDC:
  mov dx,0x03f2
  mov al,0x18  ; Motor A on, DMA/INT enabled, FDC reset, drive=A
  out dx,al
  mov al,0x1c  ; Motor A on, DMA/INT enabled, FDC not reset, drive=A
  out dx,al
  ; According to the NEC uPD765A datasheet: "If the RDY input is held high
  ; during reset, the FDC will generate an interrupt within 1.024ms. To
  ; clear this interrupt, use the Sense Interrupt Status command."
  ; The IBM controller has RDY tied to +5V, so we should expect this
  ; interrupt.
  mov word[operationCallback], seekCallback
  mov byte[interrupted], 0
  mov word[completionCallback], synchronousCallback
  jmp waitInterrupt


  ; Wait until offset DI in the DMA buffer is reached.
dmaWait:
  add di,[dma_offset]
.loop:
  in al,0x04
  mov ah,al
  in al,0x04
  xchg al,ah
  cmp ax,di
  jb .loop
  ret


  ; Wait for 1 second for the motor to spin up
motorWait:
  print "Waiting for spinup",10
  mov bx,[time]
.loop:
  mov cx,[time]
  sub cx,bx
  cmp cx,19
  jb .loop
  ret


  ; Wait for 25ms for head to settle
settleWait:
  print "Waiting for head to settle",10
  readPIT16 0
  mov bx,ax
.loop:
  readPIT16 0
  sub ax,bx
  neg ax
  cmp ax,29830
  jb .loop
  ret


; To do a synchronous disk operation, put the address of the operation in ax
; and call synchronousDisk.
synchronousDisk:
  mov byte[interrupted], 0
  mov word[completionCallback], synchronousCallback
  call ax
waitInterrupt:
  print "Waiting",10
  mov bx,[time]
  sti
.loop:
  cmp byte[interrupted],0
  jnz synchronousComplete
  mov cx,[time]
  sub cx,bx
  cmp cx,37  ; 2 seconds timeout
  jb .loop
  ; We timed out
  print "Timeout waiting for 765 interrupt",10
  complete

synchronousComplete:
  ret

synchronousCallback:
  inc byte[cs:interrupted]
  iret


; Interrupt 8 handler
interrupt8:
  inc word[cs:time]
  jmp far [cs:oldInterrupt8]
  ; push ax; mov al,0x20; out 0x20,al; pop ax; iret

oldInterrupt8:
  dw 0, 0
time:
  dw 0


; Interrupt E hanadler
interruptE:
  push ax
  push si
  push cx
  print "Interrupt received",10
  mov al,0x20
  out 0x20,al
  call word[cs:operationCallback]
  pop cx
  pop si
  pop ax
  jmp word[cs:completionCallback]
noCompletionCallback:
  iret

oldInterruptE:
  dw 0, 0


; The NEC D765AC datasheet says: After each byte of data read or written to
; the data register, CPU should way for 12us before reading main status
; register. This is not really necessary on PC/XT, but may be for faster
; machines.
delay12us:
  push ax
  readPIT16 0
  mov bx,ax
.loop:
  readPIT16 0
  sub ax,bx
  neg ax
  cmp ax,15
  jb .loop
  pop ax
  ret


printNybble:
  cmp al,10
  jge printAlphabetic
  add al,'0'
  jmp printGotCharacter
printAlphabetic:
  add al,'A' - 10
printGotCharacter:
  printCharacter
  ret

printByte:
  mov bx,ax
  mov al,' '
  printCharacter
printByte2:
  mov al,bl
  mov cl,4
  shr al,cl
  call printNybble
  mov al,bl
  and al,0x0f
  call printNybble
  mov al,bl
  ret


; Write a byte to the 765. The byte to write is in AL.
; Assume DX=0x03f4.
write765:
  call printByte
  call delay12us
  mov ah,al
  mov bx,[time]
.wait:
  in al,dx
  and al,0xc0
  cmp al,0x80
  je .do
  mov cx,[time]
  sub cx,bx
  cmp cx,2
  jb .wait

  print 10,"Timeout writing byte to 765"

  in al,dx
  mov [main_status],al
  call printMainStatus
  printNewLine

  complete
.do:
  mov al,ah
  inc dx
  out dx,al
  dec dx
  ret


; Set up DMA addresses
setupDMA:
  print "Setting up DMA.",10

  mov word [dma_low],programEnd
  mov [dma_low+2],cs

  mov ax,[dma_low]
  mov dx,[dma_low+2]
  mov [dma_pointer],ax
  mov [dma_pointer+2],dx
  mov bl,dh
  mov cl,4
  shr bl,cl
  shl dx,cl
  add ax,dx
  adc bl,0
  mov [dma_page],bl
  mov [dma_offset],ax

  ; First attempt at DMA buffer in BL AX. Check that it does not overlap a page boundary.

  mov dx,[dma_length]
  cmp dx,0
  je .fullPage
  add ax,dx
  jc .overflow
  adc bl,0
  jmp .checkHigh

.overflow:
  print "Trying next page.",10
  mov cl,4

  ; The region immediately after the program overlapped the page boundary.
  ; Try the start of the next page.
  xor ax,ax
  mov [dma_low],ax
  mov [dma_pointer],ax
  mov bl,[dma_page]
  inc bl
  mov [dma_page],bl
  mov bh,bl
  shl bh,cl
  mov [dma_pointer + 2],al
  mov [dma_pointer + 3],bh

  cmp dx,0
  je .nextPage
  add ax,dx
  adc bl,0
  jmp .checkHigh

.fullPage:
  cmp ax,0
  jne .overflow
.nextPage:
  inc bl

.checkHigh:
  ; Now the first 20-bit address after the DMA buffer is in BL AX - we need to
  ; check that it does not exceed dma_high.
  push ax
  print "Checking for DMA high.",10
  mov cl,4
  pop ax

  push ax
  push bx

  mov dx,[dma_high+2]
  mov bh,dh
  shr bh,cl
  shl dx,cl
  add dx,[dma_high]
  adc bh,0

  ; Now the 20-bit version of dma_high is in BH DX
  sub dx,ax
  sbb bh,bl
  jc .tooHigh

  print "DMA buffer at "
  mov al,[dma_page]
  call printNybble
  mov ax,[dma_offset]
  printHex
  printCharacter '-'
  pop ax
  call printNybble
  pop ax
  printHex
  printNewLine

  ret

.tooHigh:
  pop ax
  pop ax
  print "Insufficient space for DMA buffer",10
  complete


startDMA:
  mov al,0
  out 0x0c,al  ; Clear first/last flip-flop
  mov al,[dma_mode]  ; Channel 2, autoinit disabled, upwards, single
  out 0x0b,al  ; Set DMA mode
  mov ax,[dma_offset]
  out 0x04,al  ; Output DMA offset low
  mov al,ah
  out 0x04,al  ; Output DMA offset high
  mov al,[dma_page]
  out 0x81,al  ; Output DMA page
  mov ax,[dma_length]
  dec ax
  out 0x05,al  ; Output DMA length low
  mov al,ah
  out 0x05,al  ; Output DMA length high
  mov al,2
  out 0x0a,al  ; Clear DMA mask bit for channel 2
  ret


command_code:       db 0x40  ; +0x80 for MT, +0x40 for MF, +0x20 for SK
command_unit:       db 0     ; unit (0..3) +0x04 for side 1
command_c:          db 0     ; cylinder
command_h:          db 0     ; head
command_r:          db 0     ; sector number
command_n:          db 0     ; sector size = 128 bytes * 2^N, or DTL when N==0
command_eot:        db 8     ; end of track (last sector number)
command_gpl:        db 0x5d  ; gap length
command_dtl:        db 0     ; data length in bytes for N==0
results:                     ; We should get no more than 7 bytes of results back from the 765.
result_st0:                  ;  IC1  IC0   SE   EC   NR   HD  US1  US0
result_st3:         db 0     ;   FT   WP   RY   T0   TS   HD  US1  US0
result_st1:                  ;   EN    0   DE   OR    0   ND   NW   MA
result_pcn:         db 0     ; present cylinder number
result_st2:         db 0     ;    0   CM   DD   WC   SH   SN   BC   MD
result_c:           db 0     ; cylinder
result_h:           db 0     ; head
result_r:           db 0     ; sector number
result_n:           db 0     ; sector size = 128 bytes * 2^N
specify_1:          db 0xcf  ; SRT3 SRT2 SRT1 SRT0 HUT3 HUT2 HUT1 HUT0
specify_2:          db 0x02  ; HLT6 HLT5 HLT4 HLT3 HLT2 HLT1 HLT0   ND
command_ncn:        db 0     ; new cylinder number
command_sc:         db 0     ; sectors per track (format command only)
command_d:          db 0xf6  ; fill byte (format command only)
operationCallback:  dw seekCallback  ; operation-specific callback
completionCallback: dw noCompletionCallback  ; post-operation callback (for calling code)
interrupted:        db 0
dma_page:           db 0
dma_offset:         dw 0
dma_length:         dw 0     ; 1 = 1 byte, ..., 0xffff = 65535 bytes, 0 = 65536 bytes
dma_low:            dw 0,0   ; Lowest address we can start looking for a suitable DMA buffer
dma_high:           dw 0,0x8000  ; Ensure our DMA buffer can't overlap the stack at 0x90000-0x9FFFF
dma_pointer:        dw 0,0   ; Far pointer to address we ended up DMAing to
dma_mode:           db 0x46  ; DMA write (to RAM) mode (0x46) or read (from RAM) mode (0x4a)
main_status:        db 0

standardCommand:
  or al,[command_code]
standardCommand2:
  mov dx,0x03f4
  mov word[operationCallback],standardCallback
  call write765
  mov al,[command_unit]
  call write765
  mov al,[command_c]
  call write765
  mov al,[command_h]
  call write765
  mov al,[command_r]
  call write765
  mov al,[command_n]
  call write765
  mov al,[command_eot]
  call write765
  mov al,[command_gpl]
  call write765
  mov al,[command_dtl]
  call write765
  print ".",10
  ret


getResults:
  mov ax,cs
  mov es,ax
  mov dx,0x03f4
  print "Read"
  mov di,results
  mov cx,7
.getResult:
  call delay12us
  mov bx,[time]
.wait:
  in al,dx
  and al,0xc0
  cmp al,0xc0
  je .do
  cmp al,0x80   ; The IBM BIOS checks bit 4 here instead to see if the read is complete. Not sure if this matters.
  je .done
  mov ax,[time]
  sub ax,bx
  cmp ax,2
  jb .wait
  print "Timeout reading byte from 765",10
  complete
.do:
  inc dx
  in al,dx
  dec dx
  stosb
  push cx
  call printByte
  pop cx
  loop .getResult
.done:
  print ".",10
  ret


standardCallback:
  push bx
  push dx
  call getResults
  print "Result: "
  call printST0
  call printST1
  call printST2
  print ", C="
  mov bl,[result_c]
  call printByte2
  print ", H="
  mov bl,[result_h]
  call printByte2
  print ", R="
  mov bl,[result_r]
  call printByte2
  print ", N="
  mov bl,[result_n]
  call printByte2
  print ".",10
  pop dx
  pop bx
  ret

; "A Sense Interrupt Status command must be sent after a seem or recalibrate
; command, otherwise the FDC will consider the next command to be an Invalid
; command."
seekCallback:
  push bx
  push dx
  call commandSenseInterruptStatus
  pop dx
  pop bx
  ret


commandReadData:
  print "Command Read Data: Writing"
  mov al,0x06
  jmp standardCommand

commandReadDeletedData:
  print "Command Read Deleted Data: Writing"
  mov al,0x0c
  jmp standardCommand

commandWriteData:
  print "Command Write Data: Writing"
  mov al,[command_code]
  and al,0xc0
  or al,0x05
  jmp standardCommand2

commandWriteDeletedData:
  print "Command Write Deleted Data: Writing"
  mov al,[command_code]
  and al,0xc0
  or al,0x09
  jmp standardCommand2

commandReadTrack:
  print "Command Read Track: Writing"
  mov al,[command_code]
  and al,0x60
  or al,0x02
  jmp standardCommand2

commandReadID:
  print "Command Read ID: Writing"
  mov dx,0x03f4
  mov al,[command_code]
  and al,0x40
  or al,0x0a
  call write765
  mov al,[command_unit]
  call write765
  print ".",10
  ret

commandFormatTrack:
  print "Command Format Track: Writing"
  mov dx,0x03f4
  mov al,[command_code]
  and al,0x40
  or al,0x0d
  call write765
  mov al,[command_unit]
  call write765
  mov al,[command_n]
  call write765
  mov al,[command_sc]
  call write765
  mov al,[command_gpl]
  call write765
  mov al,[command_d]
  call write765
  print ".",10
  ret

commandScanEqual:
  print "Command Scan Equal: Writing"
  mov al,0x11
  jmp standardCommand

commandScanLowOrEqual:
  print "Command Scan Low Or Equal: Writing"
  mov al,0x19
  jmp standardCommand

commandScanHighOrEqual:
  print "Command Scan High Or Equal: Writing"
  mov al,0x1d
  jmp standardCommand

commandRecalibrate:
  print "Command Recalibrate: Writing"
  mov word[operationCallback],seekCallback
  mov dx,0x03f4
  mov al,0x07
  call write765
  mov al,[command_unit]
  and al,0x03
  call write765
  print ".",10
  ret

commandSenseInterruptStatus:
  print "Command Sense Interrupt Status: Writing"
  mov dx,0x03f4
  mov al,0x08
  call write765
  print ". "
  call getResults
  print "Result: "
  call printST0
  print ", PCN="
  mov bl,[result_pcn]
  call printByte2
  print ".",10
  ret

commandSpecify:
  print "Command Specify: Writing"
  mov dx,0x03f4
  mov al,0x03
  call write765
  mov al,[specify_1]
  call write765
  mov al,[specify_2]
  call write765
  print ".",10
  ret

commandSenseDriveStatus:
  print "Command Sense Drive Status: Writing"
  mov dx,0x03f4
  mov al,0x04
  call write765
  print ". "
  call getResults
  print "Result: "
  call printST3
  printNewLine
  ret

commandSeek:
  print "Command Seek: Writing"
  mov word[operationCallback],seekCallback
  mov dx,0x03f4
  mov al,0x0f
  call write765
  mov al,[command_unit]
  call write765
  mov al,[command_ncn]
  call write765
  print ".",10
  ret

commandVersion:
  print "Command Version: Writing"
  mov dx,0x03f4
  mov al,0x10
  call write765
  print ". "
  call getResults
  print "Result: "
  cmp byte[result_st0],0x80
  jne .v765b
  print "765A/A-2",10
  ret
.v765b:
  print "765B",10
  ret


printST0:
  print "Unit="
  mov bl,[result_st0]
  mov al,bl
  and al,3
  add al,'0'
  printCharacter

  print ", Head="
  test bl,4
  jnz .head1
  printCharacter '0'
  jmp .headDone
.head1
  printCharacter '1'
.headDone:

  test bl,8
  jz .no_nr
  print ", Not Ready"
.no_nr:

  test bl,0x10
  jz .no_ec
  print ", Equipment Check"
.no_ec:

  test bl,0x20
  jz .no_se
  print ", Seek End"
.no_se:

  test bl,0x40
  jnz .bitsx1
  test bl,0x80
  jnz .bits10
  print ", Normal Termination"
  ret
.bits10:
  print ", Invalid Command"
  ret
.bitsx1:
  test bl,0x80
  jnz .bits11
  print ", Abnormal Termination"
  ret
.bits11:
  print ", Ready Changed"
  ret


printST1:
  mov bl,[result_st1]
  test bl,1
  jz .no_ma
  print ", Missing Address Mark"
.no_ma:

  test bl,2
  jz .no_nw
  print ", Not Writable"
.no_nw:

  test bl,4
  jz .no_nd
  print ", No Data"
.no_nd:

  test bl,0x10
  jz .no_or
  print ", Overrun"
.no_or:

  test bl,0x20
  jz .no_de
  print ", Data Error"
.no_de:

  test bl,0x80
  jz .no_en
  print ", End of Cylinder"
.no_en:
  ret


printST2:
  mov bl,[result_st2]
  test bl,1
  jz .no_md
  print ", Missing Address Mark in Data Field"
.no_md:

  test bl,2
  jz .no_bc
  print ", Bad Cylinder"
.no_bc:

  test bl,4
  jz .no_sn
  print ", Scan Not Satisfied"
.no_sn:

  test bl,8
  jz .no_sh
  print ", Scan Equal Hit"
.no_sh:

  test bl,0x10
  jz .no_wc
  print ", Wrong Cylinder"
.no_wc:

  test bl,0x20
  jz .no_dd
  print ", Data Error in Data Field"
.no_dd:

  test bl,0x40
  jz .no_cm
  print ", Control Mark"
.no_cm:
  ret


printST3:
  print "Unit="
  mov bl,[result_st3]
  mov al,bl
  and al,3
  add al,'0'
  printCharacter

  print ", Head="
  test bl,4
  jnz .head1
  printCharacter '0'
  jmp .headDone
.head1
  printCharacter '1'
.headDone:

  test bl,8
  jz .no_ts
  print ", Two-Side"
.no_ts:

  test bl,0x10
  jz .no_t0
  print ", Track 0"
.no_t0:

  test bl,0x20
  jz .no_ry
  print ", Ready"
.no_ry:

  test bl,0x40
  jz .no_wp
  print ", Write Protected"
.no_wp

  test bl,0x80
  jz .no_ft
  print ", Fault"
.no_ft:
  ret


printMainStatus:
  mov bl,[main_status]

  test bl,1
  jz .no_d0b
  print ", FDD 0 Busy"
.no_d0b:

  test bl,2
  jz .no_d1b
  print ", FDD 1 Busy"
.no_d1b:

  test bl,4
  jz .no_d2b
  print ", FDD 2 Busy"
.no_d2b:

  test bl,8
  jz .no_d3b
  print ", FDD 3 Busy"
.no_d3b:

  test bl,0x10
  jz .no_cb
  print ", FDC Busy"
.no_cb:

  test bl,0x20
  jz .no_exm
  print ", Execution Mode"
.no_exm:

  test bl,0x40
  jz .no_dio
  print ", Read"
  jmp .done_dio
.no_dio:
  print ", Write"
.done_dio:

  test bl,0x80
  jz .no_rqm
  print " Request for Master"
.no_rqm:
  ret

programEnd:
