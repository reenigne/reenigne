%include "../../defaults_bin.asm"

  initCGA 9, 6, 2

clear:
  ; Clear screen to white
  mov ax,0xb800
  mov es,ax
  mov ds,ax
  mov ax,0x0fdb
  mov cx,80*100
  xor di,di
  cld
  rep stosw

  ; Draw 15 3x13 coloured blocks (aspect ratio is 3*8*5 : 6*2*6 = 5 : 3) on the right for the palette
  mov di,77*2+1
  mov cx,15
colourLoop:
  push cx
  mov al,16
  sub al,cl
  mov cx,6
yLoop:
  stosb
  inc di
  stosb
  inc di
  stosb
  add di,77*2+1
  loop yLoop
  pop cx
  loop colourLoop

  ; Draw an "all colours" palette entry (10 rows high, aspect ratio of each block is 8*5 : 2*2*6 = 5:3
  mov byte[di],1
  mov byte[di+2],2
  mov byte[di+4],3
  mov byte[di+160],1
  mov byte[di+162],2
  mov byte[di+164],3

  mov byte[di+320],4
  mov byte[di+322],5
  mov byte[di+324],6
  mov byte[di+480],4
  mov byte[di+482],5
  mov byte[di+484],6

  mov byte[di+640],7
  mov byte[di+642],8
  mov byte[di+644],9
  mov byte[di+800],7
  mov byte[di+802],8
  mov byte[di+804],9

  mov byte[di+960],10
  mov byte[di+962],11
  mov byte[di+964],12
  mov byte[di+1120],10
  mov byte[di+1122],11
  mov byte[di+1124],12

  mov byte[di+1280],13
  mov byte[di+1282],14
  mov byte[di+1284],15
  mov byte[di+1440],13
  mov byte[di+1442],14
  mov byte[di+1444],15

  mov si,4

loopTop:

  ; Wait for strobe
  mov dx,0x3da
waitLoop:
  in al,dx
  test al,2
  jz waitLoop

  ; Read light pen position:
  mov dl,0xd4
  mov al,0x10
  out dx,al
  inc dx
  in al,dx
  mov ah,al
  dec dx
  mov al,0x11
  out dx,al
  inc dx
  in al,dx     ; Light pen character position now in AX
  mov bx,ax
  add bx,ax    ; Multiply by 2 to get memory position (offset from 0xb8000)
  inc bx

  mov cl,80
  div cl
  ; Now, al = quotient  = y position
  ;      ah = remainder = x position

  cmp ah,77
  jl plot

  ; Obtain new colour from palette
  mov dl,6
  mov ah,0
  div dl
  inc ax
  mov si,ax
  jmp done

plot:
  mov ax,si
  cmp al,16
  jl normalPlot

  ; Special colour plot
  mov al,byte[bx]
  inc al
  and al,0x0f
  jnz doSpecialPlot
  inc al
doSpecialPlot:
  mov byte[bx],al
  jmp done

normalPlot:
  mov byte[bx],al

done:
  mov dx,0x3da
  waitForVerticalSync

  ; Reset strobe
  inc dx
  out dx,al
  dec dx

  waitForNoVerticalSync

  jmp loopTop
