  %include "../defaults_com.asm"

main:
  mov ax,cs
  mov ds,ax

  mov word[cleanup],cleanupFinal

  outputString "fdspeed: Floppy drive rotation speed tester",10,10

  outputString "Please insert a diskette into the drive that you wish to test. Ensure the",10
  outputString "diskette is not write protected and does not contain any data that you care",10
  outputString "about, as the contents of the disk may be wiped by the speed testing process.",10
  outputString "When you have done this, enter the drive number of the drive that you want to",10
  outputString "test (0-3). 0 is drive A, 1 is drive B. Or press escape to end the program",10
  outputString "without performing any tests.",10
  outputString "When you've finished testing, press Esc to exit the program.",10

whichDrive:
  mov ah,1
  int 0x21

  cmp al,27
  jne notEsc
  jmp [cleanup]
notEsc:
  sub al,'0'
  jl whichDrive
  cmp al,3
  jg whichDrive

  mov [driveNumber],al


; Setup interrupts
  mov word[cleanup], cleanupInterrupts

  xor ax,ax
  mov ds,ax
  cli
  getInterrupt 8, oldInterrupt8
  setInterrupt 8, interrupt8
  getInterrupt 0x0e, oldInterruptE
  setInterrupt 0x0e, interruptE
  sti

  ; While we have 0 in DS, get some disk base table variables
  mov es,[0x1e*4 + 2]
  mov si,[0x1e*4]
  mov ax,cs
  mov ds,ax
  mov al,[es:si]
  mov [specify1],al
  mov al,[es:si+1]
  mov [specify2],al
  mov al,[es:si+2]
  mov [motorTimeout],al
;  mov [sectors],al
;  shl al,1
;  mov ah,al
;  mov al,0
;  mov word[dmaLength], ax  ; Enough DMA space for one track


  ; Get RAM size
  int 0x12
  mov cl,6
  shl ax,cl
  mov [dmaHigh+2],ax


  ; Setup DMA space for the saved track. We reserve enough for a full raw track
  ; in extended density format plus one sector so we can make sure we got the
  ; whole track.
  mov word [dmaLow],programEnd
  mov [dmaLow+2],cs
  mov word[dmaLength],49*512
  call setupDMA
  mov al,[dmaPage]
  mov [savedTrack0Page],al
  mov ax,[dmaOffset]
  mov [savedTrack0Offset],ax


  ; Read a sector via the BIOS to ensure speed bits are set correctly for the
  ; disk that is present.
  mov ah,0
  mov dl,[driveNumber]
  int 0x13
  jnc .resetOk
  outputString "Error resetting disk system via BIOS",10
  jmp [cleanup]
.resetOk:
  mov ax,0x0201
  mov bx,[dmaPointer]
  mov es,[dmaPointer+2]
  mov cx,1
  mov dl,[driveNumber]
  mov dh,0
  int 0x21
  jnc .readOk
  outputString "Error reading first sector via BIOS",10
  jmp [cleanup]
.readOk:


  ; Tell the BIOS we've got the motor on (it should be already on from the
  ; BIOS read).
  cli
  mov ax,0x40
  mov es,ax
  mov al,[motorTimeout]
  mov [es:0x40],al
  mov cl,[driveNumber]
  mov al,1
  shl al,cl
  mov [es:0x3f],al
  sti


  outputString "Resetting 765",10
  call resetFDC

  call commandVersion

  call motorWait

  ; Not actually necessary at least for the IBM controller - the USn pins of
  ; the FDC are not connected.
  mov al,[driveNumber]
  mov [commandUnit],al
  ; Recalibrate drive - this should be a no-op since we just read a sector from track 0.
  mov ax,commandRecalibrate
  call synchronousDisk
  test byte[resultST0],0xd8
  jz .ok2
  outputString "Error recalibrating drive",10
  jmp [cleanup]
.ok2:

  call settleWait


  ; Use the Read ID command to determine the sector size.
  mov ax,commandReadID
  call synchronousDisk
  mov al,[resultC]
  mov [commandC],al
  mov al,[resultN]
  mov [commandN],al
  mov al,[resultH]
  mov [commandH],al


  ; Do the read for backup
  outputString "Reading for backup",10
  mov byte[dmaMode],0x46
  call startDMA
  mov byte[commandR],1
  mov al,196
  mov cl,[resultN]
  shr al,cl
  mov byte[commandEOT],al
  mov ax,commandReadData
  call synchronousDisk
  test byte[resultST0],0x98
  jnz .readError
  test byte[resultST1],0x33
  jnz .readError
  test byte[resultST2],0x7f
  jnz .readError
  ; Make sure the read failed - if it succeeded it potentially means that
  ; there is more data on the disk that we didn't allocate space for. That
  ; shouldn't happen unless somebody made a drive with a density higher than
  ; extended.
  test byte[resultST0],0x40
  jnz .readOk2
.readError:
  outputString "Error reading for backup",10
  jmp [cleanup]
.readOk2:
  ; Save the number of sectors we actually read
  mov al,[resultR]
  mov [commandEOT],al
  mov [commandSC],al


  ; Figure out the total number of bytes read, which we'll ues to figure out
  ; the density.
  mov ah,0
  mov cl,[commandN]
  shl ax,cl

  cmp ax,12500
  jge .extended
  cmp ax,6250
  jge .high
  mov byte[density],0
  jmp gotDensity
.high:
  mov byte[density],1
  jmp gotDensity
.extended:
  mov byte[density],2
gotDensity:
  mov cl,[density]
  mov bx,ax  ; This is the number of bytes actually read

  ; Use the density to determine the limits of the search space.
  mov ax,0x2000
  shl ax,cl
  mov word[highestSize],ax
  mov word[dmaLength],ax
  mov word[lowestSize],0
  mov ax,6250
  shl ax,cl
  mov word[trialSize],ax


  ; Make a second DMA region after the first one
  mov ax,[dmaPointer]
  add ax,bx
  mov [dmaLow],ax
  mov ax,[dmaPointer+2]
  jnc .noCarry
  add ax,0x1000
.noCarry:
  mov [dmaLow+2],ax
  call setupDMA

  mov word[cleanup], cleanup2

  mov byte[searchPattern],0


  ; This is the main measurement loop. The measurement process works by
  ; reading the entire track and seeing how long it takes for the data to
  ; repeat. This is more accurate than measuring the rotation speed by using
  ; the PIT, especially as what we really want is the rotation speed relative
  ; to the FDC clock. With this method we can get the rotation speed with a
  ; resolution of half a bit (1 part in 100,000).
  ;
  ; Normally, the FDC will not send any data to the CPU except actual sector
  ; data, so we have to trick it into reading the entire track. We do this by
  ; formatting a sector which is longer than the track. We can't fit in a full
  ; sector of this length, so we have to interrupt the format before it
  ; completes, leaving the disk with a single track header and sector header
  ; but no sector footer. This means that the sector cannot be read
  ; successfully, but it'll still return data to the CPU, as the FDC streams
  ; data instead of buffering it. So by the time it realizes anything is
  ; wrong, we'll have got away with the data.
  ;
  ; As well as a partial format we also want to do a write that is as long as
  ; possible - if we were reading back bits written at a different speed it
  ; would skew our results. We can do this by querying the DMA controller to
  ; learn which byte it is reading from RAM, and resetting the FDC at the right
  ; byte. To find the optimal number of bytes to write, we do a binary search.
  ;
  ; We also use the PIT to measure the rotation speed and data transfer speed
  ; by watching DMA counters and reading the PIT at the right times.

doFormat:
  call breakFormat

  outputString "Writing data",10

  mov byte[dmaMode],0x4a
  call startDMA

  ; Clear buffer to 0
  mov al,0
  mov es,[dmaPointer+2]
  mov di,[dmaPointer]
  mov cx,0x8000
.loop:
  stosb
  inc al
  loop .loop

  mov byte[commandR],1
  call commandWriteData

  ; Wait until we've written trialSize bytes
  ; Turn interrupts off for the last 200 bytes (~10% of the time between
  ; interrupts) to get as close as possible. We don't want to leave it
  ; off for the whole track, though, or we'll miss interrupts and the
  ; clock will drift.
  mov di,[trialSize]
  sub di,200
  call dmaWait
  add di,200
  cli
  call dmaWait
  call resetFDC  ; This wiil turn interrupts back on


testRead:
  outputString "Reading for test",10

  mov byte[dmaMode],0x46
  call startDMA
  mov ax,commandReadData
  call synchronousDisk
  test byte[resultST0],0xb8
  jnz .error
  test byte[resultST1],0x96
  jnz .error
  test byte[resultST2],0x5f
  jz .no_error
.error:
  outputString "Error for test",10
  jmp [cleanup]
.no_error:
  test byte[resultST1],1
  jz success

  ; We were not able to read the sector - we wrote too much and overwrote the
  ; track header.
  cmp byte[searchPattern],0
  je .binarySearch

  cmp byte[searchPattern],1
  je .increase

  ; Bad read, decrease - need to decrease some more.
  mov ax,[highestSize]
  mov bx,[lowestSize]
  add bx,bx
  sub bx,ax
  mov [lowestSize],bx
  mov [trialSize],bx
  jmp doneTrial

.binarySearch:
  ; Bad read, binary search
  mov ax,[trialSize]
  mov [highestSize],ax
  mov bx,[lowestSize]
  add ax,bx
  shr ax,1
  mov [trialSize],ax
  cmp ax,bx
  jne .doneTrial2
  ; lowestSize is good and highestSize is bad. We've just had a bad, so try
  ; decreasing.
  mov byte[searchPattern],2
.doneTrial2:
  jmp doneTrial

.increase:
  ; Bad read, increase - stop increasing and switch to binary search.
  mov byte[searchPattern],0
  jmp doneTrial

success:
  cmp byte[searchPattern],0
  je .binarySearch

  cmp byte[searchPattern],1
  je .increase

  ; Good read, decrease - stop decreasing and switch to binary search.
  mov byte[searchPattern],0
  jmp doScan

.binarySearch:
  ; Good read, binary search
  mov ax,[trialSize]
  mov [lowestSize],ax
  mov bx,ax
  add ax,[highestSize]
  shr ax,1
  mov [trialSize],ax
  cmp ax,bx
  jne doScan
  ; lowestSize is good and highestSize is bad. We've just had a good, so try
  ; increasing.
  mov byte[searchPattern],1
  inc word[trialSize]
  jmp doScan

.increase:
  ; Good read, increase - need to increase some more.
  mov ax,[lowestSize]
  mov bx,[highestSize]
  add bx,bx
  sub bx,ax
  mov [highestSize],bx
  mov [trialSize],bx
  jmp doScan


  ; This is where we scan backwards through the data that the FDC returned to
  ; try to find the end of the sector header that we got from reading the
  ; entire disk. Finding it is equivalent to counting the number of half bits
  ; recorded on the full track, and hence to measuring the rotation speed to
  ; within 1 part in 100000.
doScan:
  mov es,[dmaSegment]
  mov bx,[dmaOffset]
  add bx,[dmaLength]
  dec bx
  mov al,[es:bx]
  cmp al,0
  je .search00
  cmp al,0xff
  je .searchFF2
.unexpected:
  outputString "Unexpected byte read from disk",10
  jmp [cleanup]
.searchFF2:
  jmp .searchFF
.search00:
  dec bx
  mov al,[es:bx]
  cmp al,0
  jne .foundNot00
  cmp bx,[dmaOffset]
  jne .search00
  outputString "Couldn't find sector header",10
  jmp [cleanup]
.foundNot00:
  cmp al,0xfb
  jne .notFB
  mov al,16
  jmp .found
.notFB:
  cmp al,0xf6
  jne .notF6
  mov al,14
  jmp .found
.notF6:
  cmp al,0xec
  jne .notEC
  mov al,12
  jmp .found
.notEC:
  cmp al,0xd8
  jne .notD8
  mov al,10
  jmp .found
.notD8:
  cmp al,0xb0
  jne .notB0
  mov al,8
  jmp .found
.notB0:
  cmp al,0x60
  jne .not60
  mov al,6
  jmp .found
.not60:
  cmp al,0xc0
  jne .notC0
  mov al,4
  jmp .found
.notC0:
  cmp al,0x80
  jne .unexpected2
  mov al,2
  jmp .found
.unexpected2:
  jmp .unexpected

.searchFF:
  dec bx
  mov al,[es:bx]
  cmp al,0xff
  jne .foundNotFF
  cmp bx,[dmaOffset]
  jne .searchFF
  outputString "Couldn't find sector header",10
  jmp [cleanup]
.foundNotFF:
  cmp al,0x00
  jne .not00
  mov al,15
  jmp .found
.not00:
  cmp al,0x01
  jne .not01
  mov al,13
  jmp .found
.not01:
  cmp al,0x03
  jne .not03
  mov al,11
  jmp .found
.not03:
  cmp al,0x07
  jne .not07
  mov al,9
  jmp .found
.not07:
  cmp al,0x0f
  jne .not0F
  mov al,7
  jmp .found
.not0F:
  cmp al,0x1f
  jne .not1F
  mov al,5
  jmp .found
.not1F:
  cmp al,0x3f
  jne .not3F
  mov al,3
  jmp .found
.not3F:
  cmp al,0x7f
  jne .unexpected2
  mov al,1


.found:
  sub bx,[dmaOffset]
  mov cx,0
  shl bx,1
  rcl bx,1
  shl bx,1
  rcl bx,1
  shl bx,1
  rcl bx,1
  shl bx,1
  rcl bx,1
  mov ah,0
  add bx,ax
  adc cx,0
  ; Now CX:BX contains number of half-bits per track
  ; Subtract



doneTrial:
  ; TODO

  ; Check for escape pressed
  mov ah,1
  int 0x16
  cmp al,27
  je cleanup2

  jmp doFormat

cleanup2:
  mov word[cleanup], cleanupInterrupts

  outputString "Formatting track 0",10
  mov byte[dmaMode],0x4a

  mov al,[commandSC]
  mov ah,0
  shl ax,1
  shl ax,1
  mov word[dmaLength],ax

  mov es,[dmaPointer+2]
  mov di,[dmaPointer]
  mov cx,[commandEOT]
  mov bx,0x0201  ; N R
.loop:
  mov ax,0x0000  ; H C
  stosw
  mov ax,bx
  stosw
  inc bx
  loop .loop

  mov byte[commandN],2
  mov ax,commandFormatTrack
  call synchronousDisk
  test byte[resultST0],0xd8
  jnz .error
  test byte[resultST1],0xb7
  jnz .error
  test byte[resultST2],0x7f
  jz .ok
.error:
  outputString "Error formatting track 0",10
  jmp [cleanup]

.ok:
  outputString "Restoring track 0",10
  mov al,[savedTrack0Page]
  mov [dmaPage],al
  mov ax,[savedTrack0Offset]
  mov [dmaOffset],al
  call startDMA
  mov ax,commandWriteData
  call synchronousDisk

  test byte[resultST0],0xd8
  jnz .restoreError
  test byte[resultST1],0xb7
  jnz .restoreError
  test byte[resultST2],0x7f
  jz .restoreOk
.restoreError:
  outputString "Error restoring track 0",10
.restoreOk:

cleanupInterrupts:
  cli
  restoreInterrupt 8, oldInterrupt8
  restoreInterrupt 0x0e, oldInterruptE
  sti
cleanupFinal:
  complete


; Format a track with a single 8192-byte sector.
; Interrupt the format before it is complete so that the track and sector
; headers are not overwritten by data.
breakFormat:
  outputString "Performing break-format",10
  mov byte[dmaMode],0x4a
  call startDMA

  mov es,[dmaPointer+2]
  mov di,[dmaPointer]
  mov ax,0x0000  ; H C
  stosw
  mov ax,0x0801  ; N R
  stosw

  ; Start the format
  mov byte[commandN],8
  mov byte[commandSC],1
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
  mov al,0x10
  mov cl,[driveNumber]
  shl al,cl
  or al,8      ; Motor on, DMA/INT enabled, FDC reset
  or al,cl
  out dx,al
  or al,4      ; Motor on, DMA/INT enabled, FDC not reset
  out dx,al
  ; According to the NEC uPD765A datasheet: "If the RDY input is held high
  ; during reset, the FDC will generate an interrupt within 1.024ms. To
  ; clear this interrupt, use the Sense Interrupt Status command."
  ; The IBM controller has RDY tied to +5V, so we should expect this
  ; interrupt.
  mov word[operationCallback], seekCallback
  mov byte[interrupted], 0
  mov word[completionCallback], synchronousCallback
  call waitInterrupt
  test byte[resultST0],0xd8
  jz .ok
  outputString "Error resetting 765",10
  jmp [cleanup]
.ok:
  ret


  ; Wait until offset DI in the DMA buffer is reached.
dmaWait:
  add di,[dmaOffset]
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
  outputString "Waiting for spinup",10
  mov bx,[time]
.loop:
  mov cx,[time]
  sub cx,bx
  cmp cx,19
  jb .loop
  ret


  ; Wait for 25ms for head to settle
settleWait:
  outputString "Waiting for head to settle",10
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
  outputString "Waiting",10
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
  outputString "Timeout waiting for 765 interrupt",10
  jmp [cleanup]

synchronousComplete:
  ret

synchronousCallback:
  inc byte[cs:interrupted]
  ret


; Interrupt 8 handler
interrupt8:
  ; Ensure motor won't time out while we're testing
  push es
  push ax
  mov ax,0x40
  mov es,ax
  mov al,[cs:motorTimeout]
  mov [es:0x40],al
  pop ax
  pop es

  inc word[cs:time]
  jmp far [cs:oldInterrupt8]
;  push ax
;  mov al,0x20
;  out 0x20,al
;  pop ax
;  iret

oldInterrupt8:
  dw 0, 0
time:
  dw 0


; Interrupt E hanadler
interruptE:
  push ax
  push si
  push cx
  outputString "Interrupt received",10
  mov al,0x20
  out 0x20,al
  call word[cs:operationCallback]
  pop cx
  pop si
  pop ax
  call word[cs:completionCallback]
  iret
;  jmp far [cs:oldInterruptE]

noCompletionCallback:
  ret

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
  jge .alphabetic
  add al,'0'
  jmp .gotCharacter
.alphabetic:
  add al,'A' - 10
.gotCharacter:
  outputCharacter
  ret

printByte:
  mov bx,ax
  mov al,' '
  outputCharacter
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

  outputString 10,"Timeout writing byte to 765"

  in al,dx
  mov [mainStatus],al
  call printMainStatus
  outputNewLine

  jmp [cleanup]
.do:
  mov al,ah
  inc dx
  out dx,al
  dec dx
  ret


; Set up DMA addresses
setupDMA:
  outputString "Setting up DMA.",10

  mov ax,[dmaLow]
  mov dx,[dmaLow+2]
  mov [dmaOffset],ax
  mov bl,dh
  mov cl,4
  shr bl,cl
  shl dx,cl
  add ax,dx
  adc bl,0
  mov [dmaPage],bl
  mov bh,bl
  shl bh,cl
  mov [dmaSegment + 1],bh

  ; First attempt at DMA buffer in BL AX. Check that it does not overlap a page boundary.

  mov dx,[dmaLength]
  cmp dx,0
  je .fullPage
  add ax,dx
  jc .overflow
  adc bl,0
  jmp .checkHigh

.overflow:
  outputString "Trying next page.",10
  mov cl,4

  ; The region immediately after the program overlapped the page boundary.
  ; Try the start of the next page.
  xor ax,ax
  mov [dmaOffset],ax
  mov bl,[dmaPage]
  inc bl
  mov [dmaPage],bl
  mov bh,bl
  shl bh,cl
  mov [dmaSegment + 1],bh

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
  ; check that it does not exceed dmaHigh.
  push ax
  outputString "Checking for DMA high.",10
  mov cl,4
  pop ax

  push ax
  push bx

  mov dx,[dmaHigh + 2]
  mov bh,dh
  shr bh,cl
  shl dx,cl
  add dx,[dmaHigh]
  adc bh,0

  ; Now the 20-bit version of dmaHigh is in BH DX
  sub dx,ax
  sbb bh,bl
  jc .tooHigh

  outputString "DMA buffer at "
  mov al,[dmaPage]
  call printNybble
  mov ax,[dmaOffset]
  outputHex
  outputCharacter '-'
  pop ax
  call printNybble
  pop ax
  outputHex
  outputNewLine

  ret

.tooHigh:
  pop ax
  pop ax
  outputString "Insufficient space for DMA buffer",10
  jmp [cleanup]


startDMA:
  mov al,0
  out 0x0c,al  ; Clear first/last flip-flop
  mov al,[dmaMode]  ; Channel 2, autoinit disabled, upwards, single
  out 0x0b,al  ; Set DMA mode
  mov ax,[dmaOffset]
  out 0x04,al  ; Output DMA offset low
  mov al,ah
  out 0x04,al  ; Output DMA offset high
  mov al,[dmaPage]
  out 0x81,al  ; Output DMA page
  mov ax,[dmaLength]
  dec ax
  out 0x05,al  ; Output DMA length low
  mov al,ah
  out 0x05,al  ; Output DMA length high
  mov al,2
  out 0x0a,al  ; Clear DMA mask bit for channel 2
  ret


commandCode:        db 0x40  ; +0x80 for MT, +0x40 for MF, +0x20 for SK
commandUnit:        db 0     ; unit (0..3) +0x04 for side 1
commandC:           db 0     ; cylinder
commandH:           db 0     ; head
commandR:           db 0     ; sector number
commandN:           db 0     ; sector size = 128 bytes * 2^N, or DTL when N==0
commandEOT:         db 8     ; end of track (last sector number)
commandGPL:         db 0x5d  ; gap length
commandDTL:         db 0     ; data length in bytes for N==0
results:                     ; We should get no more than 7 bytes of results back from the 765.
resultST0:                   ;  IC1  IC0   SE   EC   NR   HD  US1  US0
resultST3:          db 0     ;   FT   WP   RY   T0   TS   HD  US1  US0
resultST1:                   ;   EN    0   DE   OR    0   ND   NW   MA
resultPCN:          db 0     ; present cylinder number
resultST2:          db 0     ;    0   CM   DD   WC   SH   SN   BC   MD
resultC:            db 0     ; cylinder
resultH:            db 0     ; head
resultR:            db 0     ; sector number
resultN:            db 0     ; sector size = 128 bytes * 2^N
specify1:           db 0xcf  ; SRT3 SRT2 SRT1 SRT0 HUT3 HUT2 HUT1 HUT0
specify2:           db 0x02  ; HLT6 HLT5 HLT4 HLT3 HLT2 HLT1 HLT0   ND
commandNCN:         db 0     ; new cylinder number
commandSC:          db 0     ; sectors per track (format command only)
commandD:           db 0xf6  ; fill byte (format command only)
operationCallback:  dw seekCallback  ; operation-specific callback
completionCallback: dw noCompletionCallback  ; post-operation callback (for calling code)
interrupted:        db 0
dmaPage:            db 0
dmaOffset:          dw 0     ; dmaSegment:dmaOffset form a far pointer to our
dmaSegment:         dw 0     ; DMA address.
dmaLength:          dw 0     ; 1 = 1 byte, ..., 0xffff = 65535 bytes, 0 = 65536 bytes
dmaLow:             dw 0,0   ; Lowest address we can start looking for a suitable DMA buffer
dmaHigh:            dw 0,0x8000  ; Ensure our DMA buffer can't overlap the stack at 0x90000-0x9FFFF
dmaPointer:         dw 0,0
dmaMode:            db 0x46  ; DMA write (to RAM) mode (0x46) or read (from RAM) mode (0x4a)
mainStatus:         db 0
driveNumber:        db 0
savedTrack0Page:    db 0
savedTrack0Offset:  dw 0
sectors:            db 0
motorTimeout:       db 0
cleanup:            dw 0
highestSize:        dw 0
lowestSize:         dw 0
trialSize:          dw 0
searchPattern:      dw 0  ; 0 for binary search, 1 for increase, 2 for decrease
density:            db 0  ; 0 = double, 1 = high, 2 = extended
locked:             db 0
writableBytesCur:   dw 0
writableBytesMin:   dw 0
writableBytesMax:   dw 0
writableBytesAvg:   dw 0
halfBitsCur:        dw 0,0
halfBitsMin:        dw 0,0
halfBitsMax:        dw 0,0
halfBitsAvg:        dw 0,0



standardCommand:
  or al,[commandCode]
standardCommand2:
  mov dx,0x03f4
  mov word[operationCallback],standardCallback
  call write765
  mov al,[commandUnit]
  call write765
  mov al,[commandC]
  call write765
  mov al,[commandH]
  call write765
  mov al,[commandR]
  call write765
  mov al,[commandN]
  call write765
  mov al,[commandEOT]
  call write765
  mov al,[commandGPL]
  call write765
  mov al,[commandDTL]
  call write765
  outputString ".",10
  ret


getResults:
  mov ax,cs
  mov es,ax
  mov dx,0x03f4
  outputString "Read"
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
  outputString "Timeout reading byte from 765",10
  jmp [cleanup]
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
  outputString ".",10
  ret


standardCallback:
  push bx
  push dx
  call getResults
  outputString "Result: "
  call printST0
  call printST1
  call printST2
  outputString ", C="
  mov bl,[resultC]
  call printByte2
  outputString ", H="
  mov bl,[resultH]
  call printByte2
  outputString ", R="
  mov bl,[resultR]
  call printByte2
  outputString ", N="
  mov bl,[resultN]
  call printByte2
  outputString ".",10
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
  outputString "Command Read Data: Writing"
  mov al,0x06
  jmp standardCommand

commandReadDeletedData:
  outputString "Command Read Deleted Data: Writing"
  mov al,0x0c
  jmp standardCommand

commandWriteData:
  outputString "Command Write Data: Writing"
  mov al,[commandCode]
  and al,0xc0
  or al,0x05
  jmp standardCommand2

commandWriteDeletedData:
  outputString "Command Write Deleted Data: Writing"
  mov al,[commandCode]
  and al,0xc0
  or al,0x09
  jmp standardCommand2

commandReadTrack:
  outputString "Command Read Track: Writing"
  mov al,[commandCode]
  and al,0x60
  or al,0x02
  jmp standardCommand2

commandReadID:
  outputString "Command Read ID: Writing"
  mov dx,0x03f4
  mov al,[commandCode]
  and al,0x40
  or al,0x0a
  call write765
  mov al,[commandUnit]
  call write765
  outputString ".",10
  ret

commandFormatTrack:
  outputString "Command Format Track: Writing"
  mov dx,0x03f4
  mov al,[commandCode]
  and al,0x40
  or al,0x0d
  call write765
  mov al,[commandUnit]
  call write765
  mov al,[commandN]
  call write765
  mov al,[commandSC]
  call write765
  mov al,[commandGPL]
  call write765
  mov al,[commandD]
  call write765
  outputString ".",10
  ret

commandScanEqual:
  outputString "Command Scan Equal: Writing"
  mov al,0x11
  jmp standardCommand

commandScanLowOrEqual:
  outputString "Command Scan Low Or Equal: Writing"
  mov al,0x19
  jmp standardCommand

commandScanHighOrEqual:
  outputString "Command Scan High Or Equal: Writing"
  mov al,0x1d
  jmp standardCommand

commandRecalibrate:
  outputString "Command Recalibrate: Writing"
  mov word[operationCallback],seekCallback
  mov dx,0x03f4
  mov al,0x07
  call write765
  mov al,[commandUnit]
  and al,0x03
  call write765
  outputString ".",10
  ret

commandSenseInterruptStatus:
  outputString "Command Sense Interrupt Status: Writing"
  mov dx,0x03f4
  mov al,0x08
  call write765
  outputString ". "
  call getResults
  outputString "Result: "
  call printST0
  outputString ", PCN="
  mov bl,[resultPCN]
  call printByte2
  outputString ".",10
  ret

commandSpecify:
  outputString "Command Specify: Writing"
  mov dx,0x03f4
  mov al,0x03
  call write765
  mov al,[specify1]
  call write765
  mov al,[specify2]
  call write765
  outputString ".",10
  ret

commandSenseDriveStatus:
  outputString "Command Sense Drive Status: Writing"
  mov dx,0x03f4
  mov al,0x04
  call write765
  outputString ". "
  call getResults
  outputString "Result: "
  call printST3
  outputNewLine
  ret

commandSeek:
  outputString "Command Seek: Writing"
  mov word[operationCallback],seekCallback
  mov dx,0x03f4
  mov al,0x0f
  call write765
  mov al,[commandUnit]
  call write765
  mov al,[commandNCN]
  call write765
  outputString ".",10
  ret

commandVersion:
  outputString "Command Version: Writing"
  mov dx,0x03f4
  mov al,0x10
  call write765
  outputString ". "
  call getResults
  outputString "Result: "
  cmp byte[resultST0],0x80
  jne .not_v765a
  outputString "765A/A-2",10
  ret
.not_v765a:
  cmp byte[resultST0],0x90
  je .v765b
  outputString "Unknown 765 version",10
  jmp [cleanup]
.v765b:
  outputString "765B",10
  ret


printST0:
  outputString "Unit="
  mov bl,[resultST0]
  mov al,bl
  and al,3
  add al,'0'
  outputCharacter

  outputString ", Head="
  test bl,4
  jnz .head1
  outputCharacter '0'
  jmp .headDone
.head1
  outputCharacter '1'
.headDone:

  test bl,8
  jz .no_nr
  outputString ", Not Ready"
.no_nr:

  test bl,0x10
  jz .no_ec
  outputString ", Equipment Check"
.no_ec:

  test bl,0x20
  jz .no_se
  outputString ", Seek End"
.no_se:

  test bl,0x40
  jnz .bitsx1
  test bl,0x80
  jnz .bits10
  outputString ", Normal Termination"
  ret
.bits10:
  outputString ", Invalid Command"
  ret
.bitsx1:
  test bl,0x80
  jnz .bits11
  outputString ", Abnormal Termination"
  ret
.bits11:
  outputString ", Ready Changed"
  ret


printST1:
  mov bl,[resultST1]
  test bl,1
  jz .no_ma
  outputString ", Missing Address Mark"
.no_ma:

  test bl,2
  jz .no_nw
  outputString ", Not Writable"
.no_nw:

  test bl,4
  jz .no_nd
  outputString ", No Data"
.no_nd:

  test bl,0x10
  jz .no_or
  outputString ", Overrun"
.no_or:

  test bl,0x20
  jz .no_de
  outputString ", Data Error"
.no_de:

  test bl,0x80
  jz .no_en
  outputString ", End of Cylinder"
.no_en:
  ret


printST2:
  mov bl,[resultST2]
  test bl,1
  jz .no_md
  outputString ", Missing Address Mark in Data Field"
.no_md:

  test bl,2
  jz .no_bc
  outputString ", Bad Cylinder"
.no_bc:

  test bl,4
  jz .no_sn
  outputString ", Scan Not Satisfied"
.no_sn:

  test bl,8
  jz .no_sh
  outputString ", Scan Equal Hit"
.no_sh:

  test bl,0x10
  jz .no_wc
  outputString ", Wrong Cylinder"
.no_wc:

  test bl,0x20
  jz .no_dd
  outputString ", Data Error in Data Field"
.no_dd:

  test bl,0x40
  jz .no_cm
  outputString ", Control Mark"
.no_cm:
  ret


printST3:
  outputString "Unit="
  mov bl,[resultST3]
  mov al,bl
  and al,3
  add al,'0'
  outputCharacter

  outputString ", Head="
  test bl,4
  jnz .head1
  outputCharacter '0'
  jmp .headDone
.head1
  outputCharacter '1'
.headDone:

  test bl,8
  jz .no_ts
  outputString ", Two-Side"
.no_ts:

  test bl,0x10
  jz .no_t0
  outputString ", Track 0"
.no_t0:

  test bl,0x20
  jz .no_ry
  outputString ", Ready"
.no_ry:

  test bl,0x40
  jz .no_wp
  outputString ", Write Protected"
.no_wp

  test bl,0x80
  jz .no_ft
  outputString ", Fault"
.no_ft:
  ret


printMainStatus:
  mov bl,[mainStatus]

  test bl,1
  jz .no_d0b
  outputString ", FDD 0 Busy"
.no_d0b:

  test bl,2
  jz .no_d1b
  outputString ", FDD 1 Busy"
.no_d1b:

  test bl,4
  jz .no_d2b
  outputString ", FDD 2 Busy"
.no_d2b:

  test bl,8
  jz .no_d3b
  outputString ", FDD 3 Busy"
.no_d3b:

  test bl,0x10
  jz .no_cb
  outputString ", FDC Busy"
.no_cb:

  test bl,0x20
  jz .no_exm
  outputString ", Execution Mode"
.no_exm:

  test bl,0x40
  jz .no_dio
  outputString ", Read"
  jmp .done_dio
.no_dio:
  outputString ", Write"
.done_dio:

  test bl,0x80
  jz .no_rqm
  outputString " Request for Master"
.no_rqm:
  ret

programEnd:
