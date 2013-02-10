%include "../../defaults_bin.asm"

  ; Non-resident portion

  cld
  mov si,residentPortion
  mov di,0
  mov ax,0x9fc0  ; segment for 640KB - 1KB
  mov es,ax
  mov ax,ds
  mov cx,(1 + residentPortionEnd - residentPortion)/2
  rep movsw

  mov ax,0
  mov ds,ax
  dec word[0x413]  ; Reduce BIOS RAM count by 1KB to reduce chances of DOS stomping on our

%macro setResidentInterrupt 2
  mov word [%1*4], (%2 - residentPortion)
  mov [%1*4 + 2], es
%endmacro

  setResidentInterrupt 0x13, int13routine
  setResidentInterrupt 0x60, captureScreenRoutine
  setResidentInterrupt 0x61, startAudioRoutine
  setResidentInterrupt 0x62, stopAudioRoutine
  setResidentInterrupt 0x63, printHexRoutine
  setResidentInterrupt 0x64, printStringRoutine
  setResidentInterrupt 0x65, printCharacterRoutine
  setResidentInterrupt 0x66, sendFileRoutine
  setResidentInterrupt 0x67, completeRoutine
  setResidentInterrupt 0x68, loadSerialDataRoutine
  setResidentInterrupt 0x69, stopScreenRoutine
  setResidentInterrupt 0x6a, resumeScreenRoutine

  ; TODO: Install ourselves to top of RAM
  ; TODO: Reduce available RAM according to BIOS


  mov ax,0x40
  push ax
  jmp 0xf000:0xe518


  ; Resident portion

residentPortion:

  ; This is basically a copy of the int 0x6X routines in kernel.asm, except
  ; that these ones don't write to the screen.

loadSerialDataRoutine:
  initSerial

packetLoop:
  ; Activate DTR
  mov al,1
  out dx,al
  inc dx      ; 5

  ; Receive packet size in bytes
  receiveByte
  mov cl,al
  mov ch,0

  ; Push a copy to check when we're done and adjust DI for retries
  push cx

  ; Init checksum
  mov ah,0

  ; Receive CX bytes and store them at ES:DI
  jcxz noBytes
byteLoop:
  receiveByte
  add ah,al
  stosb
  loop byteLoop
noBytes:

  ; Receive checksum
  receiveByte
  sub ah,al

  ; Deactivate DTR
  dec dx      ; 4
  mov al,0
  out dx,al

  cmp ah,0
  jne checkSumFailed

  ; Send success byte
  inc dx      ; 5
  mov al,'K'
  sendByte
  dec dx      ; 4


  ; Normalize ES:DI
  mov ax,di
  mov cl,4
  shr ax,cl
  mov bx,es
  add bx,ax
  mov es,bx
  and di,0xf

  pop cx
  jcxz transferComplete
  jmp packetLoop

checkSumFailed:
  ; Send fail byte
  inc dx      ; 5
  mov al,'F'
  sendByte
  dec dx      ; 4

  pop cx
  sub di,cx
  jmp packetLoop

transferComplete:
  iret


completeRoutine:
  disconnect
  cli
  hlt


printNybble:
  cmp al,10
  jge printAlphabetic
  add al,'0'
  jmp printGotCharacter
printAlphabetic:
  add al,'A' - 10
printGotCharacter:
  jmp printChar


printHexRoutine:
  push bx
  push cx
  mov bx,ax
  mov al,bh
  mov cl,4
  shr al,cl
  call printNybble
  mov al,bh
  and al,0xf
  call printNybble
  mov al,bl
  shr al,cl
  call printNybble
  mov al,bl
  and al,0xf
  call printNybble
  pop cx
  pop bx
  iret


printStringRoutine:
  lodsb
  call printChar
  loop printStringRoutine
  iret


printCharacterRoutine:
  call printChar
  iret


printChar:
  push dx
  ; Output the character over serial as well
  mov dx,0x3f8 + 5
  sendByte
  pop dx
  ret


captureScreenRoutine:
  push ax
  mov al,1
  call printChar
  pop ax
  iret


startAudioRoutine:
  push ax
  mov al,2
  call printChar
  pop ax
  iret


stopAudioRoutine:
  push ax
  mov al,3
  call printChar
  pop ax
  iret


printCharEscaped:
  cmp al,0x5
  ja .checkForComplete
.escapeNeeded:
  push ax
  mov al,0
  call printChar
  pop ax
.noEscapeNeeded:
  call printChar
  ret
.checkForComplete:
  cmp al,0x1a
  je .escapeNeeded
  jmp .noEscapeNeeded


sendFileRoutine:
  cld
  mov al,4
  call printChar
  mov al,cl
  call printCharEscaped
  mov al,ch
  call printCharEscaped
  mov al,dl
  call printCharEscaped
.loopTop:
  cmp cx,0
  jne .doByte
  cmp dl,0
  je .done
.doByte:
  cmp si,0xffff
  jne .normalized
  mov si,0x000f
  mov ax,ds
  add ax,0xfff
  mov ds,ax
.normalized:
  lodsb
  call printCharEscaped
  dec cx
  cmp cx,0xffff
  jne .loopTop
  dec dl
  jmp .loopTop
.done:
stopScreenRoutine:
resumeScreenRoutine:
  iret


int13routine:
  sti
  push bx
  push cx
  push dx
  push si
  push di
  push bp
  push ds
  push es

  mov bp,0x40
  mov ds,bp

  cmp al,0
  je .reset
  cmp al,1
  je .status
  mov byte[0x41],0  ; ok
  cmp al,2
  je .read
  cmp al,3
  je .write
  cmp al,4
  je .verify
  cmp al,5
  je .format
  mov byte[0x41],1  ; bad command
  jmp .complete

.reset:
  mov byte[0x41],0  ; ok
  jmp .complete

.status:
  mov al,byte[0x41]
  jmp .complete

.read:
  call sendParameters

  ; Receive the data to read
  mov di,bx
  loadSerialData

  jmp .getResult

.write:
.verify:
  call sendParameters

  ; Send the data to write/verify
  mov cx,ax
  mov ax,es
  mov ds,ax
  mov si,bx
  printString

.getResult:
  ; Receive the status information
  mov ax,cs
  mov es,ax
  mov di,tempBuffer
  loadSerialData

  ; Store the status information
  mov ah,[cs:tempBuffer+2]
  sahf
  mov ax,[cs:tempBuffer]
  mov bp,0x40
  mov ds,bp
  mov [0x41],ah

.complete:
  pop es
  pop ds
  pop bp
  pop si
  pop di
  pop dx
  pop cx
  pop bx
  retf 2  ; Throw away saved flags

.format:
  call sendParameters

  ; Send the formatting data
  mov cl,ah
  mov ch,0
  shl cx,1
  shl cx,1
  mov ax,es
  mov ds,ax
  mov si,bx
  printString

  jmp .getResult


sendParameters:
  push ax        ; Save sector count and command
  mov al,0x05
  printCharacter   ; Host command
  mov al,0x13
  printCharacter   ; Interrupt number
  pop ax
  push ax        ; Save sector count and command
  printCharacter   ; Number of sectors
  pop ax
  push ax        ; Save sector count
  mov al,ah
  printCharacter   ; Command
  mov al,cl
  printCharacter   ; Sector number
  mov al,ch
  printCharacter   ; Track number
  mov al,dl
  printCharacter   ; Drive number
  mov al,dh
  printCharacter   ; Head number

  ; Send the contents of the disk parameter table
  xor ax,ax
  mov ds,ax
  mov si,[0x1e*4]
  mov ds,[0x1e*4 + 2]
  push word[si+3]    ; Bytes-per-sector shift...
  mov cx,0x0b
  printString
  pop cx         ; in CL
  pop ax         ; Saved sector count from above
  mov ah,0
  add cl,7
  shl ax,cl      ; Return number of bytes to write
  ret

tempBuffer:

residentPortionEnd:

