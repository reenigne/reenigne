  print "Floppy disk control program",10

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

  writePIT16 0, 2, 0

; Reset the 765.
  mov dx,0x03f2
  mov al,0x08  ; All motors off, DMA/INT enabled, FDC reset, drive=A
  out dx,al
  mov al,0x0c  ; All motors off, DMA/INT enabled, FDC not reset, drive=A
  out dx,al

  ; According to the NEC uPD765A datasheet: "If the RDY input is held high
  ; during reset, the FDC will generate an interrupt within 1.024ms. To
  ; clear this interrupt, use the Sense Interrupt Status command."
  ; The IBM controller has RDY tied to +5V, so we should expect this
  ; interrupt.
  mov word[operationCallback], noCallback
  call waitInterrupt

  call senseInterruptStatus

  call commandSpecify

  ;TODO: Turn on motor and recalibrate



; To do a synchronous disk operation, put the address of the operation in ax
; and call synchronousDisk.
synchronousDisk:
  mov byte[interrupted], 0
  mov word[completionCallback], synchronousCallback
  call ax
waitInterrupt:
  mov bx,[time]
  sti
waitInterruptLoop:
  cmp byte[interrupted],0
  jnz synchronousComplete
  mov cx,[time]
  sub cx,bx
  cmp cx,37  ; 2 seconds timeout
  jb waitInterruptLoop
  ; We timed out
  print "Timeout waiting for 765 interrupt - hardware fault?",10
  complete

synchronousComplete:
  ret

synchronousCallback:
  inc byte[cs:interrupted]
  iret



  ret


; Interrupt 8 handler
interrupt8:
  inc word[cs:time]
  jmp dword[cs:oldInterrupt8]
  ; push ax; mov al,0x20; out 0x20,al; pop ax; iret

oldInterrupt8:
  dw 0, 0
time:
  dw 0


; Interrupt E hanadler
interruptE:
  push ax
  mov al,0x20
  out 0x20,al
  call word[cs:operationCallback]
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
  readPIT16
  mov bx,ax
delayLoop:
  readPIT16
  mov cx,bx
  sub bx,ax
  cmp bx,15
  jb delayLoop
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
  mov al,bl
  mov cl,4
  shr al,cl
  call printNybble
  mov al,bl
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
write765wait:
  in al,dx
  and al,0xc0
  cmp al,0x80
  mov cx,[time]
  sub cx,bx
  cmp cx,2
  jae timeout765
  jne write765wait
  mov al,ah
  inc dx
  out dx,al
  dec dx
  ret

; Read a byte from the 765. The byte read is returned in AL.
; Assume DX=0x3f4.
read765:
  call delay12us
  mov bx,[time]
write765wait:
  in al,dx
  and al,0xc0
  cmp al,0xc0
  mov cx,[time]
  sub cx,bx
  cmp cx,2
  jae timeout765
  jne write765wait
  inc dx
  in al,dx
  dec dx
  call printByte
  ret

timeout765:
  print "Timeout writing byte to 765 - is controller connected?",10
  complete


command_code:       db 0x40  ; +0x80 for MT, +0x40 for MF, +0x20 for SK
command_unit:       db 0     ; unit (0..3) +0x04 for side 1
command_c:          db 0     ; cylinder
command_h:          db 0     ; head
command_r:          db 0     ; sector number
command_n:          db 0     ; sector size = 128 bytes * 2^N, or DTL when N==0
command_eot:        db 0     ; end of track (last sector number)
command_gpl:        db 0     ; gap length
command_dtl:        db 0     ; data length in bytes for N==0
result_st0:         db 0     ;  IC1  IC0   SE   EC   NR   HD  US1  US0
result_st1:         db 0     ;   EN    0   DE   OR    0   ND   NW   MA
result_st2:         db 0     ;    0   CM   DD   WC   SH   SN   BC   MD
result_c:           db 0     ; cylinder
result_h:           db 0     ; head
result_r:           db 0     ; sector number
result_n:           db 0     ; sector size = 128 bytes * 2^N
specify_1:          db 0xcf  ; SRT3 SRT2 SRT1 SRT0 HUT3 HUT2 HUT1 HUT0
specify_2:          db 0x02  ; HLT6 HLT5 HLT4 HLT3 HLT2 HLT1 HLT0   ND
result_st3:         db 0     ;   FT   WP   RY   T0   TS   HD  US1  US0
result_pcn:         db 0     ; present cylinder number
command_ncn:        db 0     ; new cylinder number
command_sc:         db 0     ; sectors per track (format command only)
command_d:          db 0xf6  ; fill byte (format command only)
operationCallback:  dw noCallback  ; operation-specific callback
completionCallback: dw noCompletionCallback  ; post-operation callback (for calling code)
interrupted:        db 0

standardCommand:
  mov dx,0x03f4
  or al,[command_code]
standardCommand2:
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
  jmp write765

standardCallback:
  push bx
  push cx
  push dx
  push si
  mov dx,0x03f4
  print "Read"
  call read765
  mov [result_st0],al
  call read765
  mov [result_st1],al
  call read765
  mov [result_st2],al
  call read765
  mov [result_c],al
  call read765
  mov [result_h],al
  call read765
  mov [result_r],al
  call read765
  mov [result_n],al
  call printErrors
  pop si
  pop dx
  pop cx
  pop bx
noCallback:
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
  jmp write765

commandFormatTrack:
  print "Command Format Track: Writing"
  mov dx,0x03f4
  mov al,[command_code]
  and al,0x40
  or al,0x0a
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
  jmp write765

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
  mov word[operationCallback],noCallback
  mov dx,0x03f4
  mov al,0x07
  call write765
  mov al,[command_unit]
  and al,0x03
  jmp write765

commandSenseInterruptStatus:
  print "Command Sense Interrupt Status: Writing"
  mov dx,0x03f4
  mov al,0x08
  call write765
  print 10,"Read"
  call read765
  mov [result_st0],al
  call read765
  mov [result_pcn],al
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
  jmp write765

commandSenseDriveStatus:
  print "Command Sense Drive Status: Writing"
  mov dx,0x03f4
  mov al,0x04
  call write765
  print 10,"Read"
  call read765
  mov [result_st3],al
  print 10
  ret

commandSeek:
  print "Command Seek: Writing"
  mov word[operationCallback],noCallback
  mov dx,0x03f4
  mov al,0x0f
  call write765
  mov al,[command_unit]
  call write765
  mov al,[command_ncn]
  jmp write765

commandVersion:
  print "Command Version: Writing"
  mov dx,0x03f4
  mov al,0x10
  call write765
  print 10,"Read"
  call read765
  mov [result_st0],al  ; 0x80 for 765A/A-2, 0x90 for 765B.
  print 10
  ret


