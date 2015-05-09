P8086
locals @@
text segment public
assume cs:text,ds:text,es:text,ss:text
org 0100h
start:


; Skeleton test

;experimentFoo:          ; Unique label to distinguish local labels.
;  db "Foo$"             ; Name of experiment, $-terminated.
;  db 0                  ; DRAM refresh period (0 for no refresh).
;  dw @@endInit - ($+2)  ; Number of bytes in init section.
;
;                        ; Init section code goes here. Most registers default
;                        ; to 0. DS defaults to 0x8000, ES defaults to 0xb800,
;                        ; SS defaults to 0x7000. CX defaults to number of
;                        ; iterations (so you can profile a non-unrolled loop
;                        ; by just putting it in the init section).
;
;@@endInit:              ; Local label marker to find end of init section.
;  dw @@endCode - ($+2)  ; Number of bytes in test section.
;
;                        ; Test section code goes here.
;                        ; MTD makes 48 or 528 consecutive copies.
;
;@@endCode:              ; Local label marker to find end of init section.


; Notes: If the test code is more than ~120 bytes it will run out of space.
; If the test takes more than 546 +016 CPU cycles the timer will overflow
; and you'll get an answer too small by a multiple of that number.
;
; Reprogramming the PIT may cause MTD to give incorrect results. I suggest
; writing to port 0xe0 instead.


; Example tests

experimentROLCL:
  db "ROLCL$"
  db 0
  dw @@endInit - ($+2)

  mov ax,7000h
  mov ds,ax
  xor si,si
  xor bx,bx
  cld

@@endInit:
  dw @@endCode - ($+2)

  lodsw
  mov cl, 4
  rol ax, cl
  mov [bx], ax
@@endCode:


experimentROLx4:
  db "ROLx4$"
  db 0
  dw @@endInit - ($+2)

  mov ax,7000h
  mov ds,ax
  xor si,si
  xor bx,bx
  cld

@@endInit:
  dw @@endCode - ($+2)

  lodsw
  rol ax, 1
  rol ax, 1
  rol ax, 1
  rol ax, 1
  mov [bx], ax
@@endCode:


experimentROLCL_18:
  db "ROLCL$"
  db 18
  dw @@endInit - ($+2)

  mov ax,7000h
  mov ds,ax
  xor si,si
  xor bx,bx
  cld

@@endInit:
  dw @@endCode - ($+2)

  lodsw
  mov cl, 4
  rol ax, cl
  mov [bx], ax
@@endCode:


experimentROLx4_18:
  db "ROLx4$"
  db 18
  dw @@endInit - ($+2)

  mov ax,7000h
  mov ds,ax
  xor si,si
  xor bx,bx
  cld

@@endInit:
  dw @@endCode - ($+2)

  lodsw
  rol ax, 1
  rol ax, 1
  rol ax, 1
  rol ax, 1
  mov [bx], ax
@@endCode:


text ends
end start
