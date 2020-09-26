  %include "defaults_bin.asm"

  mov ax,8
  cwd
  mov cx,3
  idiv cl

  push dx
  outputHex
  outputCharacter ' '
  pop ax
  outputHex
  outputCharacter ' '
  outputCharacter ' '


  mov ax,8
  cwd
  mov cx,-3
  idiv cl

  push dx
  outputHex
  outputCharacter ' '
  pop ax
  outputHex
  outputCharacter ' '
  outputCharacter ' '


  mov ax,-8
  cwd
  mov cx,3
  idiv cl

  push dx
  outputHex
  outputCharacter ' '
  pop ax
  outputHex
  outputCharacter ' '
  outputCharacter ' '


  mov ax,-8
  cwd
  mov cx,-3
  idiv cl

  push dx
  outputHex
  outputCharacter ' '
  pop ax
  outputHex
  outputCharacter ' '
  outputCharacter ' '



  mov ax,8
  cwd
  mov cx,3
  db 0xf2
  idiv cl

  push dx
  outputHex
  outputCharacter ' '
  pop ax
  outputHex
  outputCharacter ' '
  outputCharacter ' '


  mov ax,8
  cwd
  mov cx,-3
  db 0xf2
  idiv cl

  push dx
  outputHex
  outputCharacter ' '
  pop ax
  outputHex
  outputCharacter ' '
  outputCharacter ' '


  mov ax,-8
  cwd
  mov cx,3
  db 0xf2
  idiv cl

  push dx
  outputHex
  outputCharacter ' '
  pop ax
  outputHex
  outputCharacter ' '
  outputCharacter ' '


  mov ax,-8
  cwd
  mov cx,-3
  db 0xf2
  idiv cl

  push dx
  outputHex
  outputCharacter ' '
  pop ax
  outputHex
  outputCharacter ' '
  outputCharacter ' '


  mov ax,8
  cwd
  mov cx,3
  div cl

  push dx
  outputHex
  outputCharacter ' '
  pop ax
  outputHex
  outputCharacter ' '
  outputCharacter ' '


  mov ax,8
  cwd
  mov cx,-3
  div cl

  push dx
  outputHex
  outputCharacter ' '
  pop ax
  outputHex
  outputCharacter ' '
  outputCharacter ' '


  mov ax,-8
  cwd
  mov cx,3
  div cl

  push dx
  outputHex
  outputCharacter ' '
  pop ax
  outputHex
  outputCharacter ' '
  outputCharacter ' '


  mov ax,-8
  cwd
  mov cx,-3
  div cl

  push dx
  outputHex
  outputCharacter ' '
  pop ax
  outputHex
  outputCharacter ' '
  outputCharacter ' '



  mov ax,8
  cwd
  mov cx,3
;  db 0xf2
  rep div cl

  push dx
  outputHex
  outputCharacter ' '
  pop ax
  outputHex
  outputCharacter ' '
  outputCharacter ' '


  mov ax,8
  cwd
  mov cx,-3
  db 0xf2
  div cl

  push dx
  outputHex
  outputCharacter ' '
  pop ax
  outputHex
  outputCharacter ' '
  outputCharacter ' '


  mov ax,-8
  cwd
  mov cx,3
  db 0xf2
  div cl

  push dx
  outputHex
  outputCharacter ' '
  pop ax
  outputHex
  outputCharacter ' '
  outputCharacter ' '


  mov ax,-8
  cwd
  mov cx,-3
;  db 0xf2
  rep div cl
  rep mul cl

  push dx
  outputHex
  outputCharacter ' '
  pop ax
  outputHex
  outputCharacter ' '
  outputCharacter ' '






  complete
