; This code is meant to be loaded to 0500:0000 (make this 0600:0000 as FreeDOS does?)
org 0

  cli

  ; Find end of memory. Memory is always added in 16Kb units. We can't use
  ; the BIOS measurement since it won't have been initialized.
  mov ax,09c00
findRAM:
  mov ds,ax
  mov [0],ax
  cmp [0],ax
  je foundRAM
  sub ax,0400
  jmp findRAM
foundRAM:
  sub ax,0c00
  ; Move the stack right at the end of main RAM.
  mov ss,ax
  xor sp,sp

  ; TODO: relocate the kernel if it was not loaded in the right place
  ; 1) If we're in the final location, continue
  ; 2) If we overlap the final location, relocate to after the final location and jump to this location
  ; 3) Relocate to the final location and jump there.

  ; Set up some interrupts
  ; int 060 == output AX as a 4-digit hex number
  ; int 061 == output CX bytes from DS:SI
  ; int 062 == output AL as a character
  ; int 066 == beep (for debugging)
  xor ax,ax
  mov ds,ax
  mov [0180], writeHex
  mov [0182], cs
  mov [0184], writeString
  mov [0186], cs
  mov [0188], writeCharacter
  mov [018a], cs
  mov [0198], beep
  mov [019a], cs

  ; Push the cleanup address for the program to retf back to.
  mov bp,0500
  push bp
  mov ax,offset complete
  push ax

  ; Find the next segment after the end of the kernel. This is where we'll
  ; load our program.
  mov ax,15 + offset kernelEnd
  mov cl,4
  shr ax,cl
  add ax,bp
  mov ds,ax

  ; Push the address
  push ds
  push di

  ; Set up the 8259 PIC to read the IRR lines
  mov al,0fd
  out 021,al  ; OCW1 - enable keyboard interrupt
  mov al,0a   ; OCW3 - no bit 5 action, no poll command issued, act on bit 0,
  out 020,al  ;  read Interrupt Request Register

  ; Read the non-keyboard bits of port 061
  mov dx,061
  in al,dx
  and al,07f
  or al,040
  mov ah,al

  ; Reads a 3-byte count and then a number of bytes into memory, starting at DS:DI, then runs the loaded code
  call keyboardRead
  mov cl,bl
  call keyboardRead
  mov ch,bl
  call keyboardRead
  mov bh,0
  mov si,bx
  push cx
pagesLoop:
  cmp si,0
  je noFullPages
  xor cx,cx
  call loadBytes
  dec si
  jmp pagesLoop
noFullPages:
  pop cx
  test cx,cx
  jz loadProgramDone
  call loadBytes
loadProgramDone:
  retf


; Reads the next keyboard scancode into BL
keyboardRead:
  ; Loop until the IRR bit 1 (IRQ 1) is high
  in al,020
  and al,2
  jz keyboardRead
  ; Read the keyboard byte and store it
  in al,060
  mov bl,al
  ; Acknowledge the keyboard scancode byte
  mov al,ah
  or al,080
  out dx,al
  and al,07f
  out dx,al
  ret


; Load CX bytes from keyboard to DS:DI (or a full 64Kb if CX == 0)
loadBytes:
  call keyboardRead
  mov [di],bl
  add di,1
  jnc noOverflow
  mov bx,ds
  add bh,010
  mov ds,bx
noOverflow:
  loop loadBytes
  ret


writeBuffer:
  db 0, 0, 0, 0


convertNybble:
  cmp al,10
  jge alphabetic
  add al,48
  jmp gotCharacter
alphabetic:
  add al,55
gotCharacter:
  stosb
  ret


writeHex:
  push ds
  push es
  push di
  push si
  push bx
  push cx
  mov bx,cs
  mov ds,bx
  mov es,bx
  mov di,offset writeBuffer
  mov bx,ax
  mov al,bh
  mov cx,4
  shr al,cl
  call convertNybble
  mov al,bh
  and al,0f
  call convertNybble
  mov al,bl
  shr al,cl
  call convertNybble
  mov al,bl
  and al,0f
  call convertNybble
  mov si,offset writeBuffer
  call sendLoop
  pop cx
  pop bx
  pop si
  pop di
  pop es
  pop ds
  iret


writeString:
  call sendLoop
  iret


writeCharacter:
  push ds
  push si
  push bx
  push cx
  mov bx,cs
  mov ds.bx
  mov si,offset writeBuffer
  mov [si],al
  mov cx,1
  call sendLoop
  pop cx
  pop bx
  pop si
  pop ds
  iret


; Send CX bytes pointed to by DS:SI
sendLoop:
  mov di,cx
  ; Lower clock line to tell the Arduino we want to send data
  mov dx,061
  in al,dx
  and al,0bf
  out dx,al
  ; Wait for 1ms
  mov bx,cx
  mov cx,281   ; 4.77 cycles/us * 1000us / 17 cycles/loop
waitLoop:
  loop waitLoop
  ; Raise clock line again
  or al,040
  out dx,al

  ; Throw away the keystroke that comes from clearInterruptedKeystroke()
  call keyboardRead

  ; Read the number of bytes that we can send
  call keyboardRead
  xor bh,bh
  cmp bx,cx
  jae gotCount
  mov cx,bx      ; Write as many bytes as we can
gotCount:
  sub di,cx

  ; Set up the bh register for sendByte
  in al,dx
  mov bh,al
  rcl bh,1
  rcl bh,1

  mov al,cl
  call sendByte  ; Send the number of bytes we'll be sending
sendByteLoop:
  cmp cx,0
  je doneSend
  lodsb
  call sendByte
  dec cx
  jmp sendByteLoop
doneSend:
  cmp di,0
  jne sendLoop
  ret


sendByte:
  mov bl,al

  clc
  mov al,bh
  rcr al,1
  rcr al,1
  out dx,al

  stc
  mov al,bh
  rcr al,1
  rcr al,1
  out dx,al

  rcr bl,1
  mov al,bh
  rcr al,1
  rcr al,1
  out dx,al

  rcr bl,1
  mov al,bh
  rcr al,1
  rcr al,1
  out dx,al

  rcr bl,1
  mov al,bh
  rcr al,1
  rcr al,1
  out dx,al

  rcr bl,1
  mov al,bh
  rcr al,1
  rcr al,1
  out dx,al

  rcr bl,1
  mov al,bh
  rcr al,1
  rcr al,1
  out dx,al

  rcr bl,1
  mov al,bh
  rcr al,1
  rcr al,1
  out dx,al

  rcr bl,1
  mov al,bh
  rcr al,1
  rcr al,1
  out dx,al

  rcr bl,1
  mov al,bh
  rcr al,1
  rcr al,1
  out dx,al
  ; TODO: Delay long enough for the Arduino to send the byte over serial.
  ret


beep:
  push ax
  push cx
  in al,061
  mov ah,al
  or al,3
  out 061,al
  mov al,0b6
  out 043,al
  mov cx,1193182 / 440
  mov al,cl
  out 042,al
  mov al,ch
  out 042,al
  xor cx,cx
beepLoop:
  loop beepLoop
  mov al,ah
  and al,0fc
  out 061,al
  xor cx,cx
quietLoop:
  loop quietLoop
  pop cx
  pop ax
  iret


complete:
  mov al,26
  int 062  ; Write a ^Z character to tell the "run" program to finish
  jmp 0  ; Restart the kernel


kernelEnd:
