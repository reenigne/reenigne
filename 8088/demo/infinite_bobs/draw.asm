  mov si,9999

    ; Start on odd nybble
    mov al,[es:di]
    and al,0xf0
    or al,0x09
    stosb

  mov cx,9999
  rep movsw

    ; End with one byte
    movsb

    ; End with one nybble
    mov al,[es:di]
    and al,0x0f
    or al,0x90
    stosb

  add di,99


  ret
