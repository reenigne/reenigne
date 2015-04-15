      %include "../defaults_bin.asm"

  mov ax,cs
  mov ds,ax
  mov cx,endMessage-message
  mov si,message

; Send CX bytes pointed to by DS:SI
sendLoop:
  mov di,cx
  ; Lower clock line to tell the Arduino we want to send data
  in al,0x61
  and al,0xbf
  out 0x61,al
  ; Wait for 0.5ms
  mov bx,cx
  mov cx,52500000/(11*54*2000)
waitLoop:
  loop waitLoop
  mov cx,di
  ; Raise clock line again
  in al,0x61
  or al,0x40
  out 0x61,al

  ; Throw away the keystroke that comes from clearInterruptedKeystroke()
  call keyboardRead

  ; Read the number of bytes that we can send
  call keyboardRead
  xor bh,bh
  cmp bx,cx
  jae gotCount
  mov cx,bx      ; Write as many bytes as we can
gotCount:
  sub di,cx                   ; should never carry, so carry should be clear

  ; Set up the bh register for sendByte
  mov dx,0x61
  in al,dx                    ; AL = 0x4c
  mov bh,al
  rcl bh,1                    ; BH = 0x98, CC
  rcl bh,1                    ; BH = 0x30, CS

   push bx
   push cx
   mov cl,4
   mov al,bh
   mov bl,al
   shr al,cl
   call printNybble
   mov al,bl
   and al,0xf
   call printNybble
   mov al,'-'
   call printChar
   pop cx
   pop bx


  mov al,cl
  call sendByteRoutine  ; Send the number of bytes we'll be sending
sendByteLoop:
  cmp cx,0
  je doneSend
  lodsb
  call sendByteRoutine
  dec cx
  jmp sendByteLoop
doneSend:
  cmp di,0
  jne sendLoop

finish:
  jmp finish


sendByteRoutine:
  push ax
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

  rcr bl,1             ; 2 0 8  Each bit takes 45.6 CPU cycles = 9.55us
  mov al,bh            ; 2 0 8  = 153 cycles on the Arduino
  rcr al,1             ; 2 0 8
  rcr al,1             ; 2 0 8
  out dx,al            ; 1 1 8

  rcr bl,1             ; 2 0 8
  mov al,bh            ; 2 0 8
  rcr al,1             ; 2 0 8
  rcr al,1             ; 2 0 8
  out dx,al            ; 1 1 8

  rcr bl,1             ; 2 0 8
  mov al,bh            ; 2 0 8
  rcr al,1             ; 2 0 8
  rcr al,1             ; 2 0 8
  out dx,al            ; 1 1 8

  rcr bl,1             ; 2 0 8
  mov al,bh            ; 2 0 8
  rcr al,1             ; 2 0 8
  rcr al,1             ; 2 0 8
  out dx,al            ; 1 1 8

  rcr bl,1             ; 2 0 8
  mov al,bh            ; 2 0 8
  rcr al,1             ; 2 0 8
  rcr al,1             ; 2 0 8
  out dx,al            ; 1 1 8

  rcr bl,1             ; 2 0 8
  mov al,bh            ; 2 0 8
  rcr al,1             ; 2 0 8
  rcr al,1             ; 2 0 8
  out dx,al            ; 1 1 8

  rcr bl,1             ; 2 0 8
  mov al,bh            ; 2 0 8
  rcr al,1             ; 2 0 8
  rcr al,1             ; 2 0 8
  out dx,al            ; 1 1 8

  rcr bl,1             ; 2 0 8
  mov al,bh            ; 2 0 8
  rcr al,1             ; 2 0 8
  rcr al,1             ; 2 0 8
  out dx,al            ; 1 1 8

  stc
  mov al,bh
  rcr al,1
  rcr al,1
  out dx,al

   pop ax
   push bx
   push cx
   mov cl,4
   mov bl,al
   shr al,cl
   call printNybble
   mov al,bl
   and al,0xf
   call printNybble
   mov al,'-'
   call printChar
   pop cx
   pop bx

  ret


; Reads the next keyboard scancode into BL
keyboardRead:
  ; Loop until the IRR bit 1 (IRQ 1) is high
  in al,0x20
  and al,2
  jz keyboardRead
  ; Read the keyboard byte and store it
  in al,0x60
  mov bl,al
  ; Acknowledge the previous byte
  in al,0x61
  or al,0x80
  out 0x61,al
  and al,0x7f
  out 0x61,al

   push bx
   push cx
   mov cl,4
   mov al,bl
   shr al,cl
   call printNybble
   mov al,bl
   and al,0xf
   call printNybble
   mov al,'*'
   call printChar
   pop cx
   pop bx

  ret

printNybble:
  cmp al,10
  jge printAlphabetic
  add al,'0'
  jmp printGotCharacter
printAlphabetic:
  add al,'A' - 10
printGotCharacter:
  jmp printChar

printChar:
  printCharacter
  ret

message:
  db "Testing 1, 2, 3."
endMessage:
