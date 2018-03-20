org 0
cpu 8086

testA:
  dw 0     ; cycle count ignored (computed by emulator)
  db 0     ; MUL queuefiller, no NOPs
  db 0     ; Refresh period
  db 0     ; Refresh phase
  db .preambleEnd - ($+1)
.preambleEnd:
  db .instructionsEnd - ($+1)

  out 0xe0,al

.instructionsEnd:
  db .fixupsEnd - ($+1)
.fixupsEnd:


test0:
  dw 0     ; cycle count ignored (computed by emulator)
  db 0x40  ; No queuefiller, no NOPs
  db 0     ; Refresh period
  db 0     ; Refresh phase
  db .preambleEnd - ($+1)
.preambleEnd:
  db .instructionsEnd - ($+1)

;  mov al,0x99
;  out 0x63,al

  in al,0x61
  and al,0xfc
  out 0x61,al
  or al,3
  out 0x61,al

;  mov al,0xb4
;  out 0x43,al
;  mov al,2
;  out 0x42,al
;  mov al,0
;  out 0x42,al

;  mov al,0xb4
;  out 0x43,al
;  mov al,2
;  out 0x42,al
;  mov al,0
;  out 0x42,al

;  mov al,0xb4
;  out 0x43,al
;  mov al,2
;  out 0x42,al
;  mov al,0
;  out 0x42,al

;  mov al,0xb4
;  out 0x43,al
;  mov al,2
;  out 0x42,al
;  mov al,0
;  out 0x42,al

  mov al,0x94
  out 0x43,al
  mov al,2
  out 0x42,al

  in al,0x62
  mov ah,al
  in al,0x62
  mov bl,al
  in al,0x62
  mov bh,al
  in al,0x62
  mov cl,al
  in al,0x62
  mov ch,al
  in al,0x62
  mov dl,al
  in al,0x62
  mov dh,al
  in al,0x62

  and ax,0x3030
  cmp ax,0x3020
  jne .fail
  and bx,0x3030
  cmp bx,0x1010
  jne .fail
  and cx,0x3030
  cmp cx,0x0000
  jne .fail
  and dx,0x3030
  cmp dx,0x2020
  jne .fail

; Reading from port 0x63 seems to give unpredictable results, so we'll skip
; it for now.
;  in al,0x63
;  mov ah,al
;  in al,0x63
;  mov bl,al
;  in al,0x63
;  mov bh,al
;  in al,0x63
;  mov cl,al
;  in al,0x63
;  mov ch,al
;  in al,0x63
;  mov dl,al
;  in al,0x63
;  mov dh,al
;  in al,0x63
;
;  and ax,0x3030
;  cmp ax,0x1030
;  jne .fail
;  and bx,0x3030
;  cmp bx,0x0010
;  jne .fail
;  and cx,0x3030
;  cmp cx,0x2000
;  jne .fail
;  and dx,0x3030
;  cmp dx,0x2020
;  jne .fail

  mov al,0xb4
  out 0x43,al
  mov al,0x6e  ; low
  out 0x42,al
  mov al,0xf9  ; high
  out 0x42,al


  int 0xff
.fail:
  int 0xfe

.instructionsEnd:
  db .fixupsEnd - ($+1)
.fixupsEnd:

