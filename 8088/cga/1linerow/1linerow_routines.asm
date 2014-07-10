startMode:
  push di
  push ds
  ; Set basic video mode (40-column text)
  mov ax,1
  int 0x10

  ; Wait for vertical sync before reprogramming for consistency
  mov dx,0x3da
waitForVerticalSync:
  in al,dx
  test al,8
  jz waitForVerticalSync

  ; Disable flashing
  mov dl,0xd8
  mov al,0x08
  out dx,al

  ; Initial CRTC reprogramming
  mov dl,0xd4
  mov ax,0x0009 ; Scanlines per row = 1
  out dx,ax
  mov ax,0x7007 ; Vertical sync position = 112
  out dx,ax

  ; Set up the timer interrupt for twice per frame
  xor ax,ax
  mov ds,ax
  mov di,8*4
  mov ax,[di]
  mov [cs:oldInterrupt8],ax
  mov ax,[di+2]
  mov [cs:oldInterrupt8+2],ax

  cli
  mov word[di],interrupt8
  mov [di+2],cs
  sti

  mov al,0x34 ; Timer 0, both bytes, mode 2, binary
  out 0x43,al
  mov al,0xe4
  out 0x40,al
  mov al,0x26
  out 0x40,al ; Count = 0x26e4 = 9956 = 19912/2

  pop ds
  pop di
  ret


stopMode:
  push di
  push ds
  ; Restore screen
  mov ax,3
  int 0x10

  mov al,0x34 ; Timer 0, both bytes, mode 2, binary
  out 0x43,al
  mov al,0
  out 0x40,al
  out 0x40,al

  mov di,8*4
  xor ax,ax
  mov ds,ax
  cli
  mov ax,[cs:oldInterrupt8]
  mov [di],ax
  mov ax,[cs:oldInterrupt8+2]
  mov [di+2],ax
  sti

  pop ds
  pop di
  ret


interrupt8:
  push ax
  push dx

  mov dx,0x3d4

  xor byte[cs:whichHalf],1
  jz .lowerHalf

  mov ax,0x6b04 ; Vertical Total = 108
  out dx,ax
  mov ax,0x0005 ; Vertical Total Adjust = 0
  out dx,ax
  mov ax,0x100c ; Start Address = 40*108 = 0x10e0
  out dx,ax
  mov ax,0xe00d
  out dx,ax
  mov ax,0x6c06 ; Vertical Displayed = 108
  out dx,ax

  jmp .doneHalf
.lowerHalf:

  mov ax,0x7b04 ; Vertical Total = 124
  out dx,ax
  mov ax,0x1e05 ; Vertical Total Adjust = 30
  out dx,ax
  mov ax,0x000c ; Start Address = 0
  out dx,ax
  inc ax
  out dx,ax
  mov ax,0x5c06 ; Vertical Displayed = 92
  out dx,ax

.doneHalf:
  pop dx

  add word[cs:timerCount],0x26e4
  jc .doOld
  mov al,0x20
  out 0x20,al
  pop ax
  iret
.doOld:
  pop ax
  jmp far [cs:oldInterrupt8]

timerCount: dw 0
whichHalf: db 0
oldInterrupt8: dw 0,0




;        for (int y = 0; y < 200; ++y) {
;            console.write("  dw ");
;            for (int x = 0; x < 40; ++x) {
;                int bestPair = 0;
;                int bestScore = 0x7fffffff;
;                for (int pair = 0; pair < 0x10000; ++pair) {
;                    int score = 0;
;                    int error = 0;
;                    int bits = cgaROM[(3*256 + (pair & 0xff))*8];
;                    for (int xx = 0; xx < 8; ++xx) {
;                        int target = data[y*320 + x*8 + xx] + error;
;                        int test = (pair >> 12) & 0xf;
;                        if ((bits & (128 >> xx)) != 0)
;                            test = (pair >> 8) & 0xf;
;                        test *= 17;
;                        score += (test - target)*(test - target);
;                        error = target - test;
;                    }
;                    if (score < bestScore) {
;                        bestScore = score;
;                        bestPair = pair;
;                    }
;                }
;                console.write(String(hex(bestPair, 4)));
;                if (x < 39)
;                    console.write(", ");
;            }
;            console.write("\n");
;        }

