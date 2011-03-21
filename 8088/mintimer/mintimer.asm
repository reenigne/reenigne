  mov ax,cs
  mov ds,ax
  mov es,ax

  mov al,034
  out 043,al
  xor al,al
  out 040,al
  out 040,al

  mov dx,offset experiment1Name
  call print
  mov bx,offset experiment1CodeStart
  mov bp,offset experiment1CodeEnd
  call doExperiments

  mov dx,offset experiment2Name
  call print
  mov bx,offset experiment2CodeStart
  mov bp,offset experiment2CodeEnd
  call doExperiments

exit:
  mov ah,04c
  int 021


experiment1Name:
  db "mov ax,[di]",0d,0a,"$"
experiment1CodeStart:
  mov ax,[di]
experiment1CodeEnd:

experiment2Name:
  db "mov bx,[di]",0d,0a,"$"
experiment2CodeStart:
  mov bx,[di]
experiment2CodeEnd:


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

;  lodsb
;  stosb
;  xor ah,ah
;  call printNumber

  jmp codeCopy
codeCopyDone:
;   mov dx,offset colonSpace
;   call print
  ret
codeCopyOutOfSpace:
  mov dx,offset outOfSpaceMessage
  call print
  jmp exit

outOfSpaceMessage:
  db "Copy out of space - use fewer iterations$"


doExperiment:
  mov ax,cx
  call printNumber
  mov dx,offset colonSpace
  call print
  mov di,offset codeSpace
  mov si,offset timerStartStart
  mov dx,offset timerEndStart
  call codeCopy
doExperimentCopyLoop:
  mov si,bx
  mov dx,bp
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
  db 0d,0a,"$"


doExperiments:
  mov cx,100 ; 0
doExperimentsLoop:
  cmp cx,1000 ; 0
  jg doneExperiments
  push cx
  push bx
  call doExperiment
  pop bx
  pop cx
  add cx,100 ; 0
  jmp doExperimentsLoop
doneExperiments:
  mov dx,offset newLine
  jmp print


startTime:
  dw 0


  ; The following code isn't executed directly, it's copied elsewhere first
timerStartStart:
  cli                  ; 00250
  in al,040            ; 00228 00064
  mov ah,al            ; 00136 00196
  in al,040            ; 00228 00064
  mov [startTime],ax   ; 00163 00036 00002
  ; Code to be timed will be copied here
timerEndStart:
  in al,040            ; 00228 00064
  mov ah,al            ; 00136 00196
  in al,040            ; 00228 00064
  mov dx,[startTime]   ; 00138 00022 00036 00002
  sub dx,ax            ; 00041 00194
  sti                  ; 00251
  ret
timerEndEnd:


codeSpace:



; Rewrite this as a routine which can be called - passed a structure containing:
;   Start code address
;   End code address
;   Text
;   Number of iterations
