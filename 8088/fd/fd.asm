%include "../defaults_bin.asm"

%macro debug 1
  push ax
  pushf
  printCharacter %1
  popf
  pop ax
%endmacro


  xor ax,ax
  mov ds,ax
  setInterrupt 0x0e, disk_int

  mov ax,0x40
  mov ds,ax

  in al,0x21
  and al,0xbf
  out 0x21,al
  sti

  debug '1'

  mov ah,0
  mov dl,ah
  int 0x13
  ;call diskette_io
  test ah,0xff
  jz test2

  debug 'F'
  printHex
  retf

test2:
  debug '2'

  mov dx,0x3f2
  mov al,0x1c
  out dx,al
  sub cx,cx
motorWait1:
  loop motorWait1
motorWait2:
  loop motorWait2
  xor dx,dx
  mov es,dx
  mov ch,1
  mov byte[0x3e],dl
  call seek
  jnc test3

  debug '*'
  retf

test3:
  debug '3'

  mov ch,0x34
  call seek
  jnc test4

  debug '#'
  retf

test4:
  debug '4'

  mov al,0x0c
  mov dx,0x3f2
  out dx,al

  debug '^'
  retf


nec_output:
  push dx
  push cx
  mov dx,0x3f4
  xor cx,cx
j23:
  in al,dx
  test al,0x40
  jz j25
  loop j23
j24:
  debug 'T'
  complete
j25:
  xor cx,cx
j26:
  in al,dx
  test al,0x80
  jnz j27
  loop j26

  debug 't'
  complete
j27:
  mov al,ah
  mov dl,0xf5
  out dx,al
  pop cx
  pop dx
  ret


get_parm:
  push ds
  xor ax,ax
  mov ds,ax
  lds si,[0x78]
  shr bx,1
  mov ah,[si+bx]
  pop ds
  jc nec_output
  ret


seek:
  mov al,1
  push cx
  mov cl,dl
  rol al,cl
  pop cx
  test al,byte[0x3e]
  jnz j28
  or byte[0x3e],al
  mov ah,0x07
  debug 'a'
  call nec_output  ; Recalibrate
  mov ah,dl
  debug 'b'
  call nec_output  ; Drive 0
  call chk_stat_2
  jc j32
j28:
  mov ah,0x0f
  debug 'c'
  call nec_output
  mov ah,dl
  debug 'd'
  call nec_output
  mov ah,ch
  debug 'e'
  call nec_output
  call chk_stat_2

  pushf
  mov bx,18
  call get_parm
  push cx
j29:
  mov cx,550
  or ah,ah
  jz j31
j30:
  loop j30
  dec ah
  jmp j29
j31:
  pop cx
  popf
j32:
  ret


chk_stat_2:
  debug 'C'
  call wait_int
  jc j34
  debug 'C'
  mov ah,8
  call nec_output  ; Sense interrupt status
  call results
  jc j34
  mov al,[0x42]

  push ax
  pushf
  debug ' '
  printHex
  popf
  pop ax

  and al,0x60
  cmp al,0x60
  jz j35
  clc
j34:
  debug 'D'
  ret
j35:
  debug 'S'
  stc
  ret


wait_int:
  sti
  push bx
  push cx
  mov bl,2
  xor cx,cx
j36:
  test byte[0x3e],0x80
  jnz j37
  loop j36
  dec bl
  jnz j36
  debug '7'
  stc
j37:
  pushf
  and byte[0x3e],0x7f
  popf
  pop cx
  pop bx
  ret


results:
  cld
  mov di,0x42
  push cx
  push dx
  push bx
  mov bl,7
j38:
  xor cx,cx
  mov dx,0x3f4
j39:
  in al,dx
  test al,0x80
  jnz j40a
  loop j39
  debug '8'
j40:
  stc
  pop bx
  pop dx
  pop cx
  ret
j40a:
  in al,dx
  test al,0x40
  jnz j42
j41:
  debug 'N'
  jmp j40
j42:
  inc dx
  in al,dx
  mov [di],al

  debug '-'
  pushf
  push ax
  printHex
  pop ax
  popf

  inc di
  mov cx,10
j43:
  loop j43
  dec dx
  in al,dx
  test al,0x10
  jz j44
  dec bl
  jnz j38
  debug '9'
  jmp j40
j44:
  pop bx
  pop dx
  pop cx
  ret


disk_int:
  sti
  push ds
  push ax
  debug '+'
  mov ax,0x40
  mov ds,ax
  or byte[0x3e],0x80
  mov al,0x20
  out 0x20,al
  pop ax
  pop ds
  iret


;diskette_io:
;  sti
;  push bx
;  push cx
;  push ds
;  push si
;  push di
;  push bp
;  push dx
;  mov bp,sp
;
;  mov ax,0x40
;  mov ds,ax
;  call j1
;  mov bx,4
;  call get_parm
;  mov [0x40],ah
;  mov ah,[0x41]
;  cmp ah,1
;  cmc
;
;  pop dx
;  pop bp
;  pop di
;  pop si
;  pop ds
;  pop cx
;  pop bx
;  ret
;
;DISKETTE_IO     PROC    FAR
;        STI                     ;INTERRUPTS BACK ON
;        PUSH    BX              ;SAVE ADDRESS
;        PUSH    CX
;        PUSH    DS              ;SAVE SEGMENT REGISTER VALUE
;        PUSH    SI              ;SAVE ALL REGISTERS DURING OPERATION
;        PUSH    DI
;        PUSH    BP
;        PUSH    DX
;        MOV     BP,SP           ;SET UP POINTER TO HEAD PARM
;        CALL    DDS
;        CALL    J1              ;CALL THE REST TO ENSURE DS RESTORED
;        MOV     BX,4            ;GET THE MOTOR WAIT PARAMETER
;        CALL    GET_PARM
;        MOV     MOTOR_COUNT,AH  ;SET THE TIMER COUNT FOR THE MOTOR
;        MOV     AH,DISKETTE_STATUS ;GET STATUS OF OPERATION
;        CMP     AH,1            ;SET THE CARRY FLAG TO INDICATE
;        CMC                     ; SUCCESS OR FAILURE
;        POP     DX              ;RESTORE ALL REGISTERS
;        POP     BP
;        POP     DI
;        POP     SI
;        POP     DS
;        POP     CX
;        POP     BX              ;RECOVER ADDRESS
;        RET     2               ;THROW AWAY SAVED FLAGS
;DISKETTE_IO     ENDP
;
;J1      PROC    NEAR
;        MOV     DH,AL           ;SAVE # SECTORS IN DH
;        AND     MOTOR_STATUS,07FH ;INDICATE A READ OPERATION
;        OR      AH,AH           ;AH=0
;        JZ      DISK_RESET
;        DEC     AH              ;AH=1
;        JZ      DISK_STATUS
;        MOV     DISKETTE_STATUS,0 ;RESET THE STATUS INDICATOR
;        CMP     DL,4            ;TEST FOR DRIVE IN 0-3 RANGE
;        JAE     J3              ;ERROR IF ABOVE
;        DEC     AH              ;AH=2
;        JZ      DISK_READ
;        DEC     AH              ;AH=3
;        JNZ     J2              ;TEST_DISK_VERF
;        JMP     DISK_WRITE
;J2:                             ;TEST_DISK_VERF
;        DEC     AH              ;AH=4
;        JZ      DISK_VERF
;        DEC     AH              ;AH=5
;        JZ      DISK_FORMAT
;J3:                             ;BAD_COMMAND
;        MOV     DISKETTE_STATUS,BAD_CMD ;ERROR CODE, NO SECTORS TRANSFERRED
;        RET                     ;UNDEFINED OPERATION
;J1      ENDP
;
;;-------------- RESET THE DISKETTE SYSTEM
;
;DISK_RESET      PROC    NEAR
;        MOV     DX,03F2H        ;ADAPTER CONTROL PORT
;        CLI                     ;NO INTERRUPTS
;        MOV     AL,MOTOR_STATUS ;WHICH MOTOR IS ON
;        MOV     CL,4            ;SHIFT COUNT
;        SAL     AL,CL           ;MOVE MOTOR VALUE TO HIGH NYBBLE
;        TEST    AL,20H          ;SELECT CORRESPONDING DRIVE
;        JNZ     J5              ;JUMP IF MOTOR ONE IS ON
;        TEST    AL,40H
;        JNZ     J4              ;JUMP IF MOTOR TWO IS ON
;        TEST    AL,80H
;        JZ      J6              ;JUMP IF MOTOR ZERO IS ON
;        INC     AL
;J4:
;        INC     AL
;J5:
;        INC     AL
;J6:
;        OR      AL,8            ;TURN ON INTERRUPT ENABLE
;        OUT     DX,AL           ;RESET THE ADAPTER
;        MOV     SEEK_STATUS,0   ;SET RECAL REQUIRED ON ALL DRIVES
;        MOV     DISKETTE_STATUS,0 ;SET OK STATUS FOR DISKETTE
;        OR      AL,4            ;TURN OFF RESET
;        OUT     DX,AL           ;TURN OFF THE RESET
;        STI                     ;REENABLE THE INTERRUPTS
;        CALL    CHK_STAT_2      ;DO SENSE INTERRUPT STATUS
;                                ; FOLLOWING RESET
;        MOV     AL,NEC_STATUS   ;IGNORE ERROR RETURN AND DO OWN TEST
;        CMP     AL,0C0H         ;TEST FOR DRIVE READY TRANSITION
;        JZ      J7              ;EVERYTHING OK
;        OR      DISKETTE_STATUS,BAD_NEC ;SET ERROR CODE
;        RET
;
;;-------------- SEND SPECIFY COMMAND TO NEC
;
;J7:                             ;DRIVE_READY
;        MOV     AH,03H          ;SPECIFY COMMAND
;        CALL    NEC_OUTPUT      ;OUTPUT THE COMMAND
;        MOV     BX,1            ;FIRST BYTE PARM IN BLOCK
;        CALL    GET_PARM        ; TO THE NEC CONTROLLER
;        MOV     BX,3            ;SECOND BYTE PARM IN BLOCK
;        CALL    GET_PARM        ; TO THE NEC CONTROLLER
;                                ;RESET_RET
;        RET                     ;RETURN TO CALLER
;DISK_RESET      ENDP
;
