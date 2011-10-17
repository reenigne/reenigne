org 0
cpu 8086

  mov ax,cs
  mov ds,ax
  mov es,ax

  mov al,0x34
  out 0x43,al
  xor al,al
  out 0x40,al
  out 0x40,al

  mov dx,experiment1Name
  mov cx,experiment1Init
  mov bx,experiment1CodeStart
  mov bp,experiment1CodeEnd
  call doExperiments

  mov dx,experiment2Name
  mov cx,experiment2Init
  mov bx,experiment2CodeStart
  mov bp,experiment2CodeEnd
  call doExperiments

  mov dx,experiment3Name
  mov cx,experiment3Init
  mov bx,experiment3CodeStart
  mov bp,experiment3CodeEnd
  call doExperiments

  mov dx,experiment4Name
  mov cx,experiment4Init
  mov bx,experiment4CodeStart
  mov bp,experiment4CodeEnd
  call doExperiments

  mov dx,experiment5Name
  mov cx,experiment5Init
  mov bx,experiment5CodeStart
  mov bp,experiment5CodeEnd
  call doExperiments

  mov dx,experiment6Name
  mov cx,experiment6Init
  mov bx,experiment6CodeStart
  mov bp,experiment6CodeEnd
  call doExperiments

  mov dx,experiment7Name
  mov cx,experiment7Init
  mov bx,experiment7CodeStart
  mov bp,experiment7CodeEnd
  call doExperiments

  mov dx,experiment8Name
  mov cx,experiment8Init
  mov bx,experiment8CodeStart
  mov bp,experiment8CodeEnd
  call doExperiments

  mov dx,experiment9Name
  mov cx,experiment9Init
  mov bx,experiment9CodeStart
  mov bp,experiment9CodeEnd
  call doExperiments

  mov dx,experimentAName
  mov cx,experimentAInit
  mov bx,experimentACodeStart
  mov bp,experimentACodeEnd
  call doExperiments

  mov dx,experimentBName
  mov cx,experimentBInit
  mov bx,experimentBCodeStart
  mov bp,experimentBCodeEnd
  call doExperiments

  mov dx,experimentCName
  mov cx,experimentCInit
  mov bx,experimentCCodeStart
  mov bp,experimentCCodeEnd
  call doExperiments

  mov dx,experimentDName
  mov cx,experimentDInit
  mov bx,experimentDCodeStart
  mov bp,experimentDCodeEnd
  call doExperiments

  mov dx,experimentEName
  mov cx,experimentEInit
  mov bx,experimentECodeStart
  mov bp,experimentECodeEnd
  call doExperiments

  mov dx,experimentFName
  mov cx,experimentFInit
  mov bx,experimentFCodeStart
  mov bp,experimentFCodeEnd
  call doExperiments

  mov dx,experimentGName
  mov cx,experimentGInit
  mov bx,experimentGCodeStart
  mov bp,experimentGCodeEnd
  call doExperiments

  mov dx,experimentHName
  mov cx,experimentHInit
  mov bx,experimentHCodeStart
  mov bp,experimentHCodeEnd
  call doExperiments

exit:
  int 0x67


experiment1Name:
  db "00*00  $"
experiment1Init:
  mov bl,0
  mov al,0
experiment1CodeStart:
  mul bl
experiment1CodeEnd:

experiment2Name:
  db "00*01  $"
experiment2Init:
  mov bl,1
  mov al,0
experiment2CodeStart:
  mul bl
experiment2CodeEnd:

experiment3Name:
  db "00*c0  $"
experiment3Init:
  mov bl,0xc0
  mov al,0
experiment3CodeStart:
  mul bl
experiment3CodeEnd:

experiment4Name:
  db "00*07  $"
experiment4Init:
  mov bl,7
  mov al,0
experiment4CodeStart:
  mul bl
experiment4CodeEnd:

experiment5Name:
  db "00*f0  $"
experiment5Init:
  mov bl,0xf0
  mov al,0
experiment5CodeStart:
  mul bl
experiment5CodeEnd:

experiment6Name:
  db "00*1f  $"
experiment6Init:
  mov bl,0x1f
  mov al,0
experiment6CodeStart:
  mul bl
experiment6CodeEnd:

experiment7Name:
  db "00*fc  $"
experiment7Init:
  mov bl,0xfc
  mov al,0
experiment7CodeStart:
  mul bl
experiment7CodeEnd:

experiment8Name:
  db "00*7f  $"
experiment8Init:
  mov bl,0x7f
  mov al,0
experiment8CodeStart:
  mul bl
experiment8CodeEnd:

experiment9Name:
  db "00*ff  $"
experiment9Init:
  mov bl,0xff
  mov al,0
experiment9CodeStart:
  mul bl
experiment9CodeEnd:

experimentAName:
  db "01*01  $"
experimentAInit:
  mov bl,1
  mov al,1
experimentACodeStart:
  mul bl
experimentACodeEnd:

experimentBName:
  db "c0*01  $"
experimentBInit:
  mov bl,1
  mov al,0xc0
experimentBCodeStart:
  mul bl
experimentBCodeEnd:

experimentCName:
  db "07*01  $"
experimentCInit:
  mov bl,1
  mov al,7
experimentCCodeStart:
  mul bl
experimentCCodeEnd:

experimentDName:
  db "f0*01  $"
experimentDInit:
  mov bl,1
  mov al,0xf0
experimentDCodeStart:
  mul bl
experimentDCodeEnd:

experimentEName:
  db "1f*01  $"
experimentEInit:
  mov bl,1
  mov al,0x1f
experimentECodeStart:
  mul bl
experimentECodeEnd:

experimentFName:
  db "fc*01  $"
experimentFInit:
  mov bl,1
  mov al,0xfc
experimentFCodeStart:
  mul bl
experimentFCodeEnd:

experimentGName:
  db "7f*01  $"
experimentGInit:
  mov bl,1
  mov al,0x7f
experimentGCodeStart:
  mul bl
experimentGCodeEnd:

experimentHName:
  db "ff*01  $"
experimentHInit:
  mov bl,1
  mov al,0xff
experimentHCodeStart:
  mul bl
experimentHCodeEnd:


print:
  push si
  mov si,dx
printLoop:
  lodsb
  cmp al,'$'
  je donePrint
  int 0x62
  jmp printLoop
donePrint:
  pop si
  ret

printNumber:
  push dx
  push bx
  push cx
  push si
  push di
  mov si,output + 4
  mov bx,10
  mov cx,5
itoaloop:
  sub dx,dx
  div bx
  add dl,'0'
  mov [si],dl
  dec si
  loop itoaloop
  mov dx,output
  call print
  pop di
  pop si
  pop cx
  pop bx
  pop dx
  ret

output:
  db "00000 $"


codeCopy:
  cmp si,dx
  je codeCopyDone
  cmp di,0xffff
  je codeCopyOutOfSpace
  movsb
  jmp codeCopy
codeCopyDone:
  ret
codeCopyOutOfSpace:
  mov dx,outOfSpaceMessage
  call print
  jmp exit

outOfSpaceMessage:
  db "Copy out of space - use fewer iterations$"


init: dw 0
codeStart: dw 0
codeEnd: dw 0

doExperiment:
  mov ax,cx
  call printNumber
  mov dx,colonSpace
  call print
  mov di,codeSpace
  mov si,timerStartStart
  mov dx,timerEndStart
  call codeCopy
  mov si,[init]
  mov dx,[codeStart]
  call codeCopy
doExperimentCopyLoop:
  mov si,[codeStart]
  mov dx,[codeEnd]
  call codeCopy
  loop doExperimentCopyLoop
  mov si,timerEndStart
  mov dx,timerEndEnd
  call codeCopy
  call codeSpace
  call printNumber
  mov dx,newLine
  jmp print

colonSpace:
  db ": $"
newLine:
  db "  $" ; db 0d,0a,"$"
newLine2:
  db 10,'$'


doExperiments:
  call print
  mov [init],cx
  mov [codeStart],bx
  mov [codeEnd],bp
  mov cx,100
doExperimentsLoop:
  cmp cx,1100
  jg doneExperiments
  push cx
  push bx
  call doExperiment
  pop bx
  pop cx
  add cx,1000
  jmp doExperimentsLoop
doneExperiments:
  mov dx,newLine2
  jmp print


startTime: dw 0


  ; The following code isn't executed directly, it's copied elsewhere first
timerStartStart:
  cli
  in al,0x40
  mov ah,al
  in al,0x40
  xchg ah,al
  mov [startTime],ax
  ; Code to be timed will be copied here
timerEndStart:
  in al,0x40
  mov ah,al
  in al,0x40
  xchg ah,al
  mov dx,[startTime]
  sub dx,ax
  sti
  mov ax,dx
  ret
timerEndEnd:


codeSpace:



; Rewrite this as a routine which can be called - passed a structure containing:
;   Start code address
;   End code address
;   Text
;   Number of iterations
