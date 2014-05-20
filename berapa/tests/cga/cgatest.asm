org 0x8000

test1:

;Basic text mode test.
;Set up the equivalent of BIOS mode 01h, and print "PASS" on the screen.
;If the user sees this, the emulator passed. If they don't, the emulator
;failed.

mov dx,0x03d8
mov al,0x28
out dx,al

mov dx,0x03d4
mov ax,0x3800
out dx,ax
mov ax,0x2801
out dx,ax
mov ax,0x2d02
out dx,ax
mov ax,0x0a03
out dx,ax
mov ax,0x1f04
out dx,ax
mov ax,0x0605
out dx,ax
mov ax,0x1906
out dx,ax
mov ax,0x1c07
out dx,ax
mov ax,0x0208
out dx,ax
mov ax,0x0709
out dx,ax
mov ax,0x000c
out dx,ax
mov ax,0x000d
out dx,ax

mov ax,0xB800
mov es,ax
xor di,di

mov al,'P'
stosb
mov al,0x07
stosb
mov al,'A'
stosb
mov al,0x07
stosb
mov al,'S'
stosb
mov al,0x07
stosb
mov al,'S'
stosb
mov al,0x07
stosb

end:
jmp end

times 32752-($-$$) db 0x90

jmp 0xF000:0x8000

times 32768-($-$$) db 0x90