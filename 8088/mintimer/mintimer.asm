  mov ax,cs
  mov ds,ax
  mov es,ax

  mov al,034
  out 043,al
  xor al,al
  out 040,al
  out 040,al

  mov dx,offset experiment1Name
  mov cx,offset experiment1Init
  mov bx,offset experiment1CodeStart
  mov bp,offset experiment1CodeEnd
  call doExperiments

  mov dx,offset experiment2Name
  mov cx,offset experiment2Init
  mov bx,offset experiment2CodeStart
  mov bp,offset experiment2CodeEnd
  call doExperiments

  mov dx,offset experiment3Name
  mov cx,offset experiment3Init
  mov bx,offset experiment3CodeStart
  mov bp,offset experiment3CodeEnd
  call doExperiments

  mov dx,offset experiment4Name
  mov cx,offset experiment4Init
  mov bx,offset experiment4CodeStart
  mov bp,offset experiment4CodeEnd
  call doExperiments

  mov dx,offset experiment5Name
  mov cx,offset experiment5Init
  mov bx,offset experiment5CodeStart
  mov bp,offset experiment5CodeEnd
  call doExperiments

  mov dx,offset experiment6Name
  mov cx,offset experiment6Init
  mov bx,offset experiment6CodeStart
  mov bp,offset experiment6CodeEnd
  call doExperiments

  mov dx,offset experiment7Name
  mov cx,offset experiment7Init
  mov bx,offset experiment7CodeStart
  mov bp,offset experiment7CodeEnd
  call doExperiments

  mov dx,offset experiment8Name
  mov cx,offset experiment8Init
  mov bx,offset experiment8CodeStart
  mov bp,offset experiment8CodeEnd
  call doExperiments

  mov dx,offset experiment9Name
  mov cx,offset experiment9Init
  mov bx,offset experiment9CodeStart
  mov bp,offset experiment9CodeEnd
  call doExperiments

  mov dx,offset experimentAName
  mov cx,offset experimentAInit
  mov bx,offset experimentACodeStart
  mov bp,offset experimentACodeEnd
  call doExperiments

  mov dx,offset experimentBName
  mov cx,offset experimentBInit
  mov bx,offset experimentBCodeStart
  mov bp,offset experimentBCodeEnd
  call doExperiments

  mov dx,offset experimentCName
  mov cx,offset experimentCInit
  mov bx,offset experimentCCodeStart
  mov bp,offset experimentCCodeEnd
  call doExperiments

  mov dx,offset experimentDName
  mov cx,offset experimentDInit
  mov bx,offset experimentDCodeStart
  mov bp,offset experimentDCodeEnd
  call doExperiments

  mov dx,offset experimentEName
  mov cx,offset experimentEInit
  mov bx,offset experimentECodeStart
  mov bp,offset experimentECodeEnd
  call doExperiments

  mov dx,offset experimentFName
  mov cx,offset experimentFInit
  mov bx,offset experimentFCodeStart
  mov bp,offset experimentFCodeEnd
  call doExperiments

  mov dx,offset experimentGName
  mov cx,offset experimentGInit
  mov bx,offset experimentGCodeStart
  mov bp,offset experimentGCodeEnd
  call doExperiments

  mov dx,offset experimentHName
  mov cx,offset experimentHInit
  mov bx,offset experimentHCodeStart
  mov bp,offset experimentHCodeEnd
  call doExperiments

exit:
  mov ah,04c
  int 021


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
  mov bl,0c0
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
  mov bl,0f0
  mov al,0
experiment5CodeStart:
  mul bl
experiment5CodeEnd:

experiment6Name:
  db "00*1f  $"
experiment6Init:
  mov bl,01f
  mov al,0
experiment6CodeStart:
  mul bl
experiment6CodeEnd:

experiment7Name:
  db "00*fc  $"
experiment7Init:
  mov bl,0fc
  mov al,0
experiment7CodeStart:
  mul bl
experiment7CodeEnd:

experiment8Name:
  db "00*7f  $"
experiment8Init:
  mov bl,07f
  mov al,0
experiment8CodeStart:
  mul bl
experiment8CodeEnd:

experiment9Name:
  db "00*ff  $"
experiment9Init:
  mov bl,0ff
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
  mov al,0c0
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
  mov al,0f0
experimentDCodeStart:
  mul bl
experimentDCodeEnd:

experimentEName:
  db "1f*01  $"
experimentEInit:
  mov bl,1
  mov al,01f
experimentECodeStart:
  mul bl
experimentECodeEnd:

experimentFName:
  db "fc*01  $"
experimentFInit:
  mov bl,1
  mov al,0fc
experimentFCodeStart:
  mul bl
experimentFCodeEnd:

experimentGName:
  db "7f*01  $"
experimentGInit:
  mov bl,1
  mov al,07f
experimentGCodeStart:
  mul bl
experimentGCodeEnd:

experimentHName:
  db "ff*01  $"
experimentHInit:
  mov bl,1
  mov al,0ff
experimentHCodeStart:
  mul bl
experimentHCodeEnd:


print:
  push bx
  push cx
  push si
  push di
  push bp
  mov ah,9
  int 021
  pop bp
  pop di
  pop si
  pop cx
  pop bx
  ret

printNumber:
  push dx
  push bx
  push cx
  push si
  push di
  mov si,offset output + 4
  mov bx,10
  mov cx,5
itoaloop:
  sub dx,dx
  div bx
  add dl,'0'
  mov [si],dl
  dec si
  loop itoaloop
  mov dx,offset output
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
  cmp di,0ffff
  je codeCopyOutOfSpace
  movsb
  jmp codeCopy
codeCopyDone:
  ret
codeCopyOutOfSpace:
  mov dx,offset outOfSpaceMessage
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
  mov dx,offset colonSpace
  call print
  mov di,offset codeSpace
  mov si,offset timerStartStart
  mov dx,offset timerEndStart
  call codeCopy
  mov si,[init]
  mov dx,[codeStart]
  call codeCopy
doExperimentCopyLoop:
  mov si,[codeStart]
  mov dx,[codeEnd]
  call codeCopy
  loop doExperimentCopyLoop
  mov si,offset timerEndStart
  mov dx,offset timerEndEnd
  call codeCopy
  call codeSpace
  call printNumber
  mov dx,offset newLine
  jmp print

colonSpace:
  db ": $"
newLine:
  db "  $" ; db 0d,0a,"$"
newLine2:
  db 0d,0a,"$"


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
  mov dx,offset newLine2
  jmp print


startTime: dw 0


  ; The following code isn't executed directly, it's copied elsewhere first
timerStartStart:
  cli
  in al,040
  mov ah,al
  in al,040
  xchg ah,al
  mov [startTime],ax
  ; Code to be timed will be copied here
timerEndStart:
  in al,040
  mov ah,al
  in al,040
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
