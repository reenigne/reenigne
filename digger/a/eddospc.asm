; Digger Remastered
; Copyright (c) Andrew Jenner 1998-2004

PUBLIC _mouseinit,_mouseshow,_mousehide,_mousepos,_mouseput,_mousebox,_vgaline
PUBLIC _cgaline
PUBLIC _initkeyb,_restorekeyb,_getkey,_kbhit
PUBLIC _graphicsoff,_gretrace
PUBLIC _cgainit,_cgaclear,_cgapal,_cgainten,_cgaputi,_cgageti,_cgaputim
PUBLIC _cgawrite,_cgawrite
PUBLIC _vgainit,_vgaclear,_vgapal,_vgainten,_vgaputi,_vgageti,_vgaputim
PUBLIC _vgawrite,_vgawrite

_mouseinit:
  XOR AX,AX
  INT 033
  OR AX,AX
  JZ nomouse
  MOV AX,BX
nomouse:
  RET

_mouseshow:
  MOV AX,1
  INT 033
  RET

_mousehide:
  MOV AX,2
  INT 033
  RET

_mousepos:
  PUSH BP
  MOV BP,SP
  MOV AX,3
  INT 033
  MOV AX,BX
  MOV BX,W[BP+4]
  MOV W[BX],CX
  MOV BX,W[BP+6]
  MOV W[BX],DX
  POP BP
  RET

_mouseput:
  PUSH BP
  MOV BP,SP
  MOV AX,4
  MOV CX,W[BP+4]
  MOV DX,W[BP+6]
  INT 033
  POP BP
  RET

_mousebox:
  PUSH BP
  MOV BP,SP
  MOV AX,7
  MOV CX,W[BP+4]
  MOV DX,W[BP+8]
  INT 033
  MOV AX,8
  MOV CX,W[BP+6]
  MOV DX,W[BP+0a]
  INT 033
  POP BP
  RET

_vgaline:
  PUSH BP
  MOV BP,SP
  PUSH SI
  PUSH DI
  MOV CX,4
linectop:
  PUSH CX
  DEC CX
  MOV DX,03ce
  MOV AL,4
  MOV AH,CL
  OUT DX,AX
  MOV DX,03c4
  MOV AX,0102
  SHL AH,CL
  OUT DX,AX
  XOR DX,DX
  TEST B[BP+0c],AH
  JZ gotcol
  MOV DL,1
gotcol:
  PUSH W[BP+0e]
  PUSH DX
  PUSH W[BP+0a]
  PUSH W[BP+08]
  PUSH W[BP+06]
  PUSH W[BP+04]
  CALL drwln
  ADD SP,0c
  POP CX
  LOOP linectop
  POP DI
  POP SI
  POP BP
  RET
drwln:
  PUSH BP
  MOV BP,SP
  SUB SP,8
  PUSH SI
  PUSH DI
  MOV AX,W[_scnseg]
  MOV ES,AX
  MOV AX,W[BP+0a]
  SUB AX,W[BP+6]
  JGE yispos
  NEG AX
yispos:
  MOV W[BP-4],AX
  MOV BX,W[BP+8]
  SUB BX,W[BP+4]
  JGE xispos
  NEG BX
xispos:
  MOV [BP-2],BX
  CMP BX,AX
  JL dl_a
  JMP dl_b
dl_a:
  SHL AX,1
  MOV W[BP-6],AX
  SHL BX,1
  MOV W[BP-8],BX
  MOV DI,W[BP-2]
  NEG DI
  MOV CX,W[BP+4]
  MOV AX,W[BP+6]
  CMP AX,W[BP+0a]
  JLE dontswapa
  MOV DX,W[BP+4]
  MOV AX,W[BP+8]
  MOV W[BP+4],AX
  MOV W[BP+8],DX
  MOV DX,W[BP+6]
  MOV AX,W[BP+0a]
  MOV W[BP+6],AX
  MOV W[BP+0a],DX
dontswapa:
  MOV AX,W[BP+6]
  MOV DX,W[_bytes_line]
  MUL DX
  ADD AX,W[_scroll]
  MOV BX,AX
  MOV AX,W[BP+4]
  MOV CL,3
  SAR AX,CL
  ADD BX,AX
  MOV AX,W[BP+4]
  AND AL,7
  MOV CL,7
  SUB CL,AL
  MOV DX,W[BP-4]
  MOV AX,W[BP-8]
  CS: MOV W[changea1a+2],AX
  CS: MOV W[changea2a+2],AX
  MOV AX,W[BP-6]
  CS: MOV W[changea1b+2],AX
  CS: MOV W[changea2b+2],AX
  MOV CH,B[BP+0c]
  MOV AX,W[BP+8]
  CMP AX,W[BP+4]
  JL dl_a1
  JMP dontchangea2b
dl_a1:
  JMP dontchangea1b
a1looptop:
  CMP W[BP+0e],1
  JZ a1xor
  ES: MOV AL,B[BX]
  MOV AH,1
  SHL AH,CL
  NOT AH
  AND AL,AH
  MOV AH,CH
  SHL AH,CL
  OR AL,AH
  ES: MOV B[BX],AL
  JMP a1finplot
a1xor:
  MOV AL,CH
  SHL AL,CL
  ES: XOR B[BX],AL
a1finplot:
  ADD BX,W[_bytes_line]
  DEC DX
changea1a:
  ADD DI,06789 ;This value to be replaced
  JLE dontchangea1b
  INC CL
  CMP CL,7
  JLE changea1b
  XOR CL,CL
  DEC BX
changea1b:
  SUB DI,06789 ;This value to be replaced
dontchangea1b:
  CMP DX,0
  JGE a1looptop
  JMP dlfin
dl_a2:
  CMP W[BP+0e],1
  JZ a2xor
  ES: MOV AL,B[BX]
  MOV AH,1
  SHL AH,CL
  NOT AH
  AND AL,AH
  MOV AH,CH
  SHL AH,CL
  OR AL,AH
  ES: MOV B[BX],AL
  JMP a2finplot
a2xor:
  MOV AL,CH
  SHL AL,CL
  ES: XOR B[BX],AL
a2finplot:
  ADD BX,W[_bytes_line]
  DEC DX
changea2a:
  ADD DI,06789 ;This value to be replaced
  JLE dontchangea2b
  DEC CL
  CMP CL,0
  JGE changea2b
  MOV CL,7
  INC BX
changea2b:
  SUB DI,06789 ;This value to be replaced
dontchangea2b:
  CMP DX,0
  JGE dl_a2
  JMP dlfin
dl_b:
  SHL BX,1
  MOV W[BP-6],BX
  SHL AX,1
  MOV W[BP-8],AX
  MOV DI,W[BP-4]
  NEG DI
  MOV CX,W[BP+4]
  MOV AX,W[BP+6]
  CMP CX,W[BP+8]
  JLE dontswapb
  MOV DX,W[BP+4]
  MOV AX,W[BP+8]
  MOV W[BP+4],AX
  MOV W[BP+8],DX
  MOV DX,W[BP+6]
  MOV AX,W[BP+0a]
  MOV W[BP+6],AX
  MOV W[BP+0a],DX
dontswapb:
  MOV AX,W[BP+6]
  MOV DX,W[_bytes_line]
  MUL DX
  ADD AX,W[_scroll]
  MOV BX,AX
  MOV AX,W[BP+4]
  MOV CL,3
  SAR AX,CL
  ADD BX,AX
  MOV AX,W[BP+4]
  AND AL,7
  MOV CL,7
  SUB CL,AL
  MOV DX,W[BP-2]
  MOV AX,W[BP-8]
  CS: MOV W[changeb1a+3],AX
  CS: MOV W[changeb2a+3],AX
  MOV AX,W[BP-6]
  CS: MOV W[changeb1b+2],AX
  CS: MOV W[changeb2b+2],AX
  MOV CH,B[BP+0c]
  MOV AX,W[BP+0a]
  CMP AX,W[BP+6]
  JL dl_b1
  JMP dontchangeb2b
dl_b1:
  JMP dontchangeb1b
b1looptop:
  CMP W[BP+0e],1
  JZ b1xor
  ES: MOV AL,B[BX]
  MOV AH,1
  SHL AH,CL
  NOT AH
  AND AL,AH
  MOV AH,CH
  SHL AH,CL
  OR AL,AH
  ES: MOV B[BX],AL
  JMP b1finplot
b1xor:
  MOV AL,CH
  SHL AL,CL
  ES: XOR B[BX],AL
b1finplot:
  DEC CL
  JGE changeb1a
  MOV CL,7
  INC BX
changeb1a:
  DEC DX
  ADD DI,06789 ;This value to be replaced
  JLE dontchangeb1b
  SUB BX,W[_bytes_line]
changeb1b:
  SUB DI,06789 ;This value to be replaced
dontchangeb1b:
  CMP DX,0
  JGE b1looptop
  JMP dlfin
dl_b2:
  CMP W[BP+0e],1
  JZ b2xor
  ES: MOV AL,B[BX]
  MOV AH,1
  SHL AH,CL
  NOT AH
  AND AL,AH
  MOV AH,CH
  SHL AH,CL
  OR AL,AH
  ES: MOV B[BX],AL
  JMP b2finplot
b2xor:
  MOV AL,CH
  SHL AL,CL
  ES: XOR B[BX],AL
b2finplot:
  DEC CL
  CMP CL,0
  JGE changeb2a
  MOV CL,7
  INC BX
changeb2a:
  DEC DX
  ADD DI,06789 ;This value to be replaced
  JLE dontchangeb2b
  ADD BX,W[_bytes_line]
changeb2b:
  SUB DI,06789 ;This value to be replaced
dontchangeb2b:
  CMP DX,0
  JGE dl_b2
dlfin:
  POP DI
  POP SI
  MOV SP,BP
  POP BP
  RET

_cgaline:
  PUSH BP
  MOV BP,SP
  SUB SP,8
  PUSH SI
  PUSH DI
  MOV AX,0b800
  MOV ES,AX
  MOV AX,[BP+0a]
  SUB AX,[BP+6]
  JGE o100
  NEG AX
o100:
  MOV [BP-4],AX
  MOV BX,[BP+8]
  SUB BX,[BP+4]
  JGE o101
  NEG BX
o101:
  MOV [BP-2],BX
  CMP BX,AX
  JL o229
  JMP o36f
o229:
  SHL AX,1
  MOV [BP-6],AX
  SHL BX,1
  MOV [BP-8],BX
  MOV DI,[BP-2]
  NEG DI
  MOV CX,[BP+4]
  MOV AX,[BP+6]
  CMP AX,[BP+0a]
  JLE o284
  MOV DX,[BP+4]
  MOV AX,[BP+8]
  MOV [BP+4],AX
  MOV [BP+8],DX
  MOV DX,[BP+6]
  MOV AX,[BP+0a]
  MOV [BP+6],AX
  MOV [BP+0a],DX
o284:
  MOV BX,[BP+6]
  AND BX,1
  MOV CL,0d
  SHL BX,CL
  MOV AX,[BP+6]
  SAR AX,1
  MOV DX,80
  MUL DX
  ADD BX,AX
  MOV AX,[BP+4]
  SAR AX,1
  SAR AX,1
  ADD BX,AX
  MOV AX,[BP+4]
  AND AL,3
  MOV CL,3
  SUB CL,AL
  SHL CL,1
  MOV DX,[BP-4]
  MOV AX,[BP-8]
  CS: MOV W[o2ef+3],AX
  CS: MOV W[o345+3],AX
  MOV AX,[BP-6]
  CS: MOV W[o3ob+2],AX
  CS: MOV W[o361+2],AX
  MOV CH,[BP+0c]
  MOV AX,[BP+8]
  CMP AX,[BP+4]
  JL o2c1
  JMP o364
o2c1:
  JMP o3oe
o2c3:
  ES: MOV AL,B[BX]
  MOV AH,3
  SHL AH,CL
  NOT AH
  AND AL,AH
  MOV AH,CH
  SHL AH,CL
  ADD AL,AH
  ES: MOV B[BX],AL
  ADD BX,02000
  CMP BX,04000
  JL o2ef
  SUB BX,03fb0
o2ef:
  DEC DX
  ADD DI,06789
  JLE o3oe
  INC CL
  INC CL
  CMP CL,6
  JLE o3ob
  XOR CL,CL
  DEC BX
o3ob:
  SUB DI,06789
o3oe:
  OR DX,DX
  JGE o2c3
  JMP o4af
o319:
  ES: MOV AL,B[BX]
  MOV AH,3
  SHL AH,CL
  NOT AH
  AND AL,AH
  MOV AH,CH
  SHL AH,CL
  ADD AL,AH
  ES: MOV B[BX],AL
  ADD BX,02000
  CMP BX,04000
  JL o345
  SUB BX,03fb0
o345:
  DEC DX
  ADD DI,06789
  JLE o364
  DEC CL
  DEC CL
  JGE o361
  MOV CL,6
  INC BX
o361:
  SUB DI,06789
o364:
  OR DX,DX
  JGE o319
  JMP o4af
o36f:
  SHL BX,1
  MOV [BP-6],BX
  SHL AX,1
  MOV [BP-8],AX
  MOV DI,[BP-4]
  NEG DI
  MOV CX,[BP+4]
  MOV AX,[BP+6]
  CMP CX,[BP+8]
  JLE o3ca
  MOV DX,[BP+4]
  MOV AX,[BP+8]
  MOV [BP+4],AX
  MOV [BP+8],DX
  MOV DX,[BP+6]
  MOV AX,[BP+0a]
  MOV [BP+6],AX
  MOV [BP+0a],DX
o3ca:
  MOV BX,[BP+6]
  AND BX,1
  MOV CL,0d
  SHL BX,CL
  MOV AX,[BP+6]
  SAR AX,1
  MOV DX,80
  MUL DX
  ADD BX,AX
  MOV AX,[BP+4]
  SAR AX,1
  SAR AX,1
  ADD BX,AX
  MOV AX,[BP+4]
  AND AL,3
  MOV CL,3
  SUB CL,AL
  SHL CL,1
  MOV DX,[BP-2]
  MOV AX,[BP-8]
  CS: MOV W[o439+3],AX
  CS: MOV W[o48c+3],AX
  MOV AX,[BP-6]
  CS: MOV W[o44f+2],AX
  CS: MOV W[o4a4+2],AX
  MOV CH,[BP+0c]
  MOV AX,[BP+0a]
  CMP AX,[BP+6]
  JL o4o7
  JMP o4a7
o4o7:
  JMP o452
o4o9:
  ES: MOV AL,B[BX]
  MOV AH,3
  SHL AH,CL
  NOT AH
  AND AL,AH
  MOV AH,CH
  SHL AH,CL
  ADD AL,AH
  ES: MOV B[BX],AL
  DEC CL
  DEC CL
  JGE o439
  MOV CL,6
  INC BX
o439:
  DEC DX
  ADD DI,06789
  JLE o452
  SUB BX,02000
  JGE o44f
  ADD BX,03fb0
o44f:
  SUB DI,06789
o452:
  OR DX,DX
  JGE o4o9
  JMP o4af
o45c:
  ES: MOV AL,B[BX]
  MOV AH,3
  SHL AH,CL
  NOT AH
  AND AL,AH
  MOV AH,CH
  SHL AH,CL
  ADD AL,AH
  ES: MOV B[BX],AL
  DEC CL
  DEC CL
  JGE o48c
  MOV CL,6
  INC BX
o48c:
  DEC DX
  ADD DI,06789
  JLE o4a7
  ADD BX,02000
  CMP BX,04000
  JL o4a4
  SUB BX,03fb0
o4a4:
  SUB DI,06789
o4a7:
  OR DX,DX
  JGE o45c
o4af:
  POP DI
  POP SI
  MOV SP,BP
  POP BP
  RET

;Keyboard routines

_initkeyb:
  PUSH DI
  MOV AX,DS
  CS: MOV W[dssave9],AX
  PUSH DS
  MOV AX,0
  MOV DS,AX
  MOV DI,024
  MOV AX,W[DI]
  MOV BX,W[DI+2]
  MOV CX,offset interrupt9
  MOV DX,CS
  CLI
  MOV W[DI],CX
  MOV W[DI+2],DX
  STI
  POP DS
  CS: MOV W[int9save],AX
  CS: MOV W[int9save+2],BX
  POP DI
  RET

_restorekeyb:
  PUSH DI
  PUSH ES
  MOV AX,0
  MOV ES,AX
  MOV DI,024
  CLI
  CS: MOV AX,W[int9save]
  ES: MOV W[DI],AX
  CS: MOV AX,W[int9save+2]
  ES: MOV W[DI+2],AX
  STI
  POP ES
  POP DI
  RET

int9save:
  DW 0,0
dssave9:
  DW 0

interrupt9:
  PUSH AX
  PUSH BX
  PUSH CX
  PUSH DX
  PUSH DS
  PUSH ES
  CS: MOV AX,W[dssave9]
  MOV DS,AX
  IN AL,060
  XOR AH,AH
  PUSH AX
  CALL _processkey
  POP AX
  POP ES
  POP DS
  POP DX
  POP CX
  POP BX
  POP AX
  CS: JMP D[int9save]

_getkey:
  MOV AH,0
  INT 016
  CMP AL,0
  JNE getlow
  MOV AL,AH
  MOV AH,1
  JMP endgetkey
getlow:
  MOV AH,0
endgetkey:
  RET

_kbhit:
  MOV AH,0b
  INT 021
  CBW
  RET

;Miscellaneous graphics

_graphicsoff:
  MOV AX,3
  INT 010
  RET

_gretrace:
  CMP B[_retrflag],0
  JNZ o513
  RET
o513:
  PUSH DX
  PUSH AX
  MOV DX,03da
o518:
  IN AL,DX
  TEST AL,8
  JNZ o518
oo518:
  IN AL,DX
  TEST AL,8
  JZ oo518
  POP AX
  POP DX
  RET


;CGA graphics

_cgainit:
  MOV B[_paletten],0
  MOV AX,4
  INT 010
  MOV AH,0b
  MOV BX,0
  INT 010
  MOV BX,0100
  INT 010
  RET

_cgaclear:
  PUSH DI
  PUSH ES
  MOV AX,0b800
  MOV ES,AX
  MOV CX,02000
  XOR DI,DI
  XOR AX,AX
  REP STOSW
  POP ES
  POP DI
  RET

_cgapal:
  PUSH BP
  MOV BP,SP
  CALL _gretrace
  CMP W[_biosflag],1
  JZ biospalette
  MOV AL,B[BP+4]
  AND AL,1
  SHL AL,1
  MOV AH,B[_paletten]
  AND AH,0fd
  OR AL,AH
  JMP cgasetpal
biospalette:
  MOV BL,B[BP+4]
  AND BL,1
  MOV AH,0b
  MOV BH,1
  INT 010
  POP BP
  RET

_cgainten:
  PUSH BP
  MOV BP,SP
  CALL _gretrace
  CMP W[_biosflag],1
  JZ biosintensity
  MOV AL,B[BP+4]
  AND AL,1
  MOV AH,B[_paletten]
  AND AH,0fe
  OR AL,AH
  JMP cgasetpal
biosintensity:
  MOV BL,B[BP+4]
  AND BL,1
  MOV CL,4
  SHL BL,CL
  MOV AH,0b
  MOV BH,0
  INT 010
  POP BP
  RET

cgasetpal:
  XOR AH,AH
  MOV BX,AX
  SHL BX,1
  ADD BX,AX
  ADD BX,offset cgacolours
  MOV CL,4
  SHL AL,CL
  MOV DX,03d9
  OUT DX,AX
  MOV DX,03ba
  IN AL,DX
  MOV DX,03da
  IN AL,DX       ;Make port 3c0 the index
  MOV DX,03c0
  MOV AL,1
  OUT DX,AL
  MOV AL,B[BX]
  OUT DX,AL
  MOV AL,2
  OUT DX,AL
  MOV AL,B[BX+1]
  OUT DX,AL
  MOV AL,3
  OUT DX,AL
  MOV AL,B[BX+2]
  OUT DX,AL
  MOV AL,020
  OUT DX,AL
  POP BP
  RET

_cgaputi:
  PUSH BP
  MOV BP,SP
  PUSH SI
  PUSH DI
  PUSH ES
  MOV BX,W[BP+4]
  MOV AX,W[BP+6]
  MOV DI,AX
  AND DI,1
  MOV CL,0d
  SHL DI,CL
  SAR AX,1
  MOV CL,80
  MUL CL
  ADD DI,AX
  SAR BX,1
  SAR BX,1
  ADD DI,BX
  MOV DX,W[BP+0a]
  MOV CX,W[BP+0c]
  MOV SI,W[BP+8]
  MOV AX,0b800
  MOV ES,AX
  CLD
cpiyt:
  MOV BX,CX
  MOV CX,DX
  REP MOVSB
  MOV CX,BX
  SUB DI,DX
  ADD DI,02000
  CMP DI,04000
  JL cpiok
  SUB DI,03fb0
cpiok:
  LOOP cpiyt
  POP ES
  POP DI
  POP SI
  POP BP
  RET

_cgageti:
  PUSH BP
  MOV BP,SP
  PUSH SI
  PUSH DI
  PUSH DS
  PUSH ES
  MOV BX,W[BP+4]
  MOV AX,W[BP+6]
  MOV SI,AX
  AND SI,1
  MOV CL,0d
  SHL SI,CL
  SAR AX,1
  MOV CL,80
  MUL CL
  ADD SI,AX
  SAR BX,1
  SAR BX,1
  ADD SI,BX
  MOV DX,W[BP+0a]
  MOV CX,W[BP+0c]
  MOV DI,W[BP+8]
  MOV ES,DS
  MOV AX,0b800
  MOV DS,AX
  CLD
cgiyt:
  MOV BX,CX
  MOV CX,DX
  REP MOVSB
  MOV CX,BX
  SUB SI,DX
  ADD SI,02000
  CMP SI,04000
  JL cgiok
  SUB SI,03fb0
cgiok:
  LOOP cgiyt
  POP ES
  POP DS
  POP DI
  POP SI
  POP BP
  RET

_cgaputim:
  PUSH BP
  MOV BP,SP
  PUSH SI
  PUSH DI
  PUSH DS
  PUSH ES
  MOV BX,W[BP+4]
  MOV AX,W[BP+6]
  MOV DI,AX
  AND DI,1
  MOV CL,0d
  SHL DI,CL
  SAR AX,1
  MOV CX,80
  IMUL CX
  ADD DI,AX
  SAR BX,1
  SAR BX,1
  ADD DI,BX
  PUSH DI
  MOV AX,0b800
  MOV ES,AX
  CLD
  MOV DX,W[BP+0a]
  MOV CX,W[BP+0c]
  PUSH CX
  MOV SI,W[BP+8]
  MOV AX,seg _cgatable
  MOV DS,AX
  SHL SI,1
  SHL SI,1
  PUSH SI
  ADD SI,2
  MOV AX,W[SI+offset _cgatable]
  MOV SI,AX
cpmiyt:
  MOV BX,CX
  MOV CX,DX
cpmixt:
  LODSB
  ES: AND B[DI],AL
  INC DI
  LOOP cpmixt
  MOV CX,BX
  SUB DI,DX
  ADD DI,02000
  CMP DI,04000
  JL cpmiok
  SUB DI,03fb0
cpmiok:
  LOOP cpmiyt
  POP SI
  MOV AX,W[SI+offset _cgatable]
  MOV SI,AX
  POP CX
  POP DI
cpimyt:
  MOV BX,CX
  MOV CX,DX
cpimxt:
  LODSB
  ES: OR B[DI],AL
  INC DI
  LOOP cpimxt
  MOV CX,BX
  SUB DI,DX
  ADD DI,02000
  CMP DI,04000
  JL cpimok
  SUB DI,03fb0
cpimok:
  LOOP cpimyt
  POP ES
  POP DS
  POP DI
  POP SI
  POP BP
  RET

_cgawrite:
  PUSH BP
  MOV BP,SP
  PUSH SI
  PUSH DI
  PUSH DS
  PUSH ES
  MOV BX,W[BP+4]
  MOV AX,W[BP+6]
  MOV DI,AX
  AND DI,1
  MOV CL,0d
  SHL DI,CL
  SAR AX,1
  MOV CL,80
  MUL CL
  ADD DI,AX
  SAR BX,1
  SAR BX,1
  ADD DI,BX
  MOV DL,B[BP+0a]
  AND DL,3
  XOR DH,DH
  MOV AX,05555
  MUL DX
  MOV DX,AX
  MOV BX,W[BP+8]
  XOR BH,BH
  SUB BX,020
  JL cganochar
  CMP BX,05f
  JGE cganochar
  SHL BX,1
  MOV AX,seg _ascii2cga
  MOV DS,AX
  MOV SI,W[BX+offset _ascii2cga]
  CMP SI,0
  JE cganochar
  MOV BX,020
  MOV AX,0b800
  MOV ES,AX
  MOV CX,12
cytop:
  LODSW
  AND AX,DX
  STOSW
  LODSB
  AND AL,DL
  STOSB
  ADD DI,01ffd
  CMP DI,04000
  JL cgaok
  SUB DI,03fb0
cgaok:
  LOOP cytop
cganochar:
  POP ES
  POP DS
  POP DI
  POP SI
  POP BP
  RET


;VGA graphics

_vgainit:
  PUSH BP
  MOV BP,SP
  PUSH SI
  PUSH DI
  MOV B[_paletten],0
  MOV AX,012
  INT 010
  MOV DX,03c2
  MOV AL,063
  OUT DX,AL
  CLI
  MOV DX,03da
vrdly1:
  IN AL,DX
  TEST AL,8
  JNZ vrdly1
vrdly2:
  IN AL,DX
  TEST AL,8
  JZ vrdly2
  MOV DX,03c4
  MOV AX,0100
  OUT DX,AX
  MOV DX,03c4
  MOV AX,0300
  OUT DX,AX
  MOV DX,03d4
  MOV AL,011
  OUT DX,AL
  INC DX
  IN AL,DX
  DEC DX
  AND AL,070
  MOV AH,0e
  OR AH,AL
  MOV AL,011
  PUSH AX
  OUT DX,AX
  MOV AX,09c10
  OUT DX,AX
  MOV AX,08f12
  OUT DX,AX
  MOV AX,09615
  OUT DX,AX
  MOV AX,0b916
  OUT DX,AX
  MOV AX,0bf06
  OUT DX,AX
  MOV AL,9
  OUT DX,AL
  INC DX
  IN AL,DX
  DEC DX
  AND AL,09f
  OR AL,040
  MOV AH,AL
  MOV AL,9
  OUT DX,AX
  MOV AX,01f07
  OUT DX,AX
  POP AX
  OR AH,080
  OUT DX,AX
  STI
  MOV DX,03c4
  MOV AX,0300
  OUT DX,AX
  XOR DI,DI
  MOV CX,0ff
  MOV SI,offset vgacolours
  MOV DX,03c8
slooptop:
  MOV AX,DI
  OUT DX,AL
  INC DX
  LODSB
  OUT DX,AL
  LODSB
  OUT DX,AL
  LODSB
  OUT DX,AL
  DEC DX
  INC DI
  LOOP slooptop
  POP DI
  POP SI
  MOV AL,020
  MOV B[_paletten],AL
  MOV AX,-1
  POP BP
  RET

_vgaclear:
  PUSH DI
  PUSH ES
  MOV AX,0f02
  MOV DX,03c4
  OUT DX,AX
  MOV AX,0a000
  MOV ES,AX
  XOR AX,AX
  XOR DI,DI
  MOV CX,16000
  CLD
  REP STOSW
  POP ES
  POP DI
  RET

_vgapal:
  PUSH BP
  MOV BP,SP
  CALL _gretrace
  MOV BL,B[BP+4]
  AND BL,1
  MOV CL,3
  SHL BL,CL
  MOV AL,B[_paletten]
  AND AL,0f7
  OR AL,BL
  MOV B[_paletten],AL
  CALL vgasetpal
  POP BP
  RET

_vgainten:
  PUSH BP
  MOV BP,SP
  CALL _gretrace
  MOV BL,B[BP+4]
  AND BL,1
  MOV CL,2
  SHL BL,CL
  MOV AL,B[_paletten]
  AND AL,0fb
  OR AL,BL
  MOV B[_paletten],AL
  CALL vgasetpal
  POP BP
  RET

vgasetpal:
  PUSH AX
  MOV DX,03da
  IN AL,DX
  MOV DX,03ba
  IN AL,DX
  MOV AL,014
  MOV DX,03c0 ;I'm using the VGA's colour select register to provide the
  OUT DX,AL   ;equivalent of the CGA's palette/intensity functions.
  POP AX
  OUT DX,AL
  MOV AL,020
  MOV DX,03c0
  OUT DX,AL
  RET

_vgaputi:
  PUSH BP
  MOV BP,SP
  PUSH SI
  PUSH DI
  PUSH ES
  MOV AX,W[BP+6]
  MOV DI,W[BP+4]
  MOV CL,160
  MUL CL
  SHR DI,1
  SHR DI,1
  ADD DI,AX
  MOV SI,W[BP+8]
  MOV BX,W[BP+0a]
  MOV AX,0a000
  MOV ES,AX
  MOV DX,03c4
  MOV CX,4
  PUSH DI
  CLD
vpipt:
  PUSH CX
  DEC CL
  MOV AX,0102
  SHL AH,CL
  OUT DX,AX
  MOV CX,W[BP+0c]
  SHL CX,1
vpiyt:
  MOV AX,CX
  MOV CX,BX
  REP MOVSB
  MOV CX,AX
  SUB DI,BX
  ADD DI,80
  LOOP vpiyt
  POP CX
  POP DI
  PUSH DI
  LOOP vpipt
  POP DI
  POP ES
  POP DI
  POP SI
  POP BP
  RET

_vgageti:
  PUSH BP
  MOV BP,SP
  PUSH SI
  PUSH DI
  PUSH DS
  PUSH ES
  MOV AX,W[BP+6]
  MOV SI,W[BP+4]
  MOV CL,160
  MUL CL
  SHR SI,1
  SHR SI,1
  ADD SI,AX
  MOV DI,W[BP+8]
  MOV BX,W[BP+0a]
  MOV ES,DS
  MOV AX,0a000
  MOV DS,AX
  MOV DX,03ce
  MOV CX,4
  PUSH SI
  CLD
vgipt:
  PUSH CX
  DEC CL
  MOV AL,4
  MOV AH,CL
  OUT DX,AX
  SS: MOV CX,W[BP+0c]
  SHL CX,1
vgiyt:
  MOV AX,CX
  MOV CX,BX
  REP MOVSB
  MOV CX,AX
  SUB SI,BX
  ADD SI,80
  LOOP vgiyt
  POP CX
  POP SI
  PUSH SI
  LOOP vgipt
  POP SI
  POP ES
  POP DS
  POP DI
  POP SI
  POP BP
  RET

_vgaputim:
  PUSH BP
  MOV BP,SP
  PUSH SI
  PUSH DI
  PUSH DS
  PUSH ES
  MOV AX,W[BP+6]
  MOV DI,W[BP+4]
  MOV CX,160
  IMUL CX
  SHR DI,1
  SHR DI,1
  ADD DI,AX
  MOV BX,W[BP+0a]
  MOV SI,W[BP+8]
  MOV AX,seg _vgatable
  MOV DS,AX
  SHL SI,1
  SHL SI,1
  PUSH SI
  ADD SI,2
  MOV AX,W[SI+offset _vgatable]
  MOV SI,AX
  PUSH SI
  PUSH DI
  MOV AX,0a000
  MOV ES,AX
  MOV CX,4
  MOV DX,03c4
  CLD
vpmipt:
  PUSH CX
  DEC CL
  MOV AX,0102
  SHL AH,CL
  OUT DX,AX
  MOV AL,4
  MOV AH,CL
  MOV DL,0ce
  OUT DX,AX
  MOV DL,0c4
  SS: MOV CX,W[BP+0c]
  SHL CX,1
vpmiyt:
  PUSH CX
  MOV CX,BX
vpmixt:
  LODSB
  ES: AND B[DI],AL
  INC DI
  LOOP vpmixt
  POP CX
  SUB DI,BX
  ADD DI,80
  LOOP vpmiyt
  POP CX
  POP DI
  POP SI
  PUSH SI
  PUSH DI
  LOOP vpmipt
  POP DI
  POP SI
  POP SI
  MOV AX,W[SI+offset _vgatable]
  MOV SI,AX
  PUSH DI
  MOV CX,4
vpimpt:
  PUSH CX
  DEC CL
  MOV AX,0102
  SHL AH,CL
  OUT DX,AX
  MOV AL,4
  MOV AH,CL
  MOV DL,0ce
  OUT DX,AX
  MOV DL,0c4
  SS: MOV CX,W[BP+0c]
  SHL CX,1
vpimyt:
  PUSH CX
  MOV CX,BX
vpimxt:
  LODSB
  ES: OR B[DI],AL
  INC DI
  LOOP vpimxt
  POP CX
  SUB DI,BX
  ADD DI,80
  LOOP vpimyt
  POP CX
  POP DI
  PUSH DI
  LOOP vpimpt
  POP DI
  POP ES
  POP DS
  POP DI
  POP SI
  POP BP
  RET

vganochar:
  JMP vganochar2
_vgawrite:
  PUSH BP
  MOV BP,SP
  PUSH SI
  PUSH DI
  PUSH DS
  PUSH ES
  MOV BX,W[BP+8]
  XOR BH,BH
  SUB BX,020
  JL vganochar2
  CMP BX,05f
  JGE vganochar2
  SHL BX,1
  PUSH DS
  MOV AX,seg _ascii2vga
  MOV DS,AX
  MOV SI,W[BX+offset _ascii2vga]
  POP DS
  CMP SI,0
  JE vganochar2
  MOV BX,020
  MOV AX,W[BP+6]
  MOV DI,W[BP+4]
  MOV CL,160
  MUL CL
  SHR DI,1
  SHR DI,1
  ADD DI,AX
  MOV BL,B[BP+0a]
  XOR BH,BH
  SHL BX,1
  SHL BX,1
  SHL BX,1
  ADD BX,offset _textoffdat
  MOV CX,4
  MOV AX,0a000
  MOV ES,AX
  MOV DX,03c4
  MOV AX,seg _ascii2vga
  MOV DS,AX
planetop:
  PUSH CX
  DEC CX
  MOV AX,0102
  SHL AH,CL
  OUT DX,AX
  ADD SI,W[BX]
  INC BX
  INC BX
  MOV CX,24
ytop:
  MOVSW
  MOVSB
  ADD DI,77
  LOOP ytop
  POP CX
  SUB DI,1920
  LOOP planetop
vganochar2:
  POP ES
  POP DS
  POP DI
  POP SI
  POP BP
  RET


_DATA SEGMENT WORD PUBLIC 'DATA'

_paletten:
  DB 0

cgacolours:
  DB 2,4,6,18,20,22,3,5,7,19,21,23

vgacolours:
  DB  0, 0, 0,  0, 0,32,  0,32, 0,  0,32,32
  DB 32, 0, 0, 32, 0,32, 32,32, 0, 32,32,32
  DB  0, 0,16,  0, 0,63,  0,32,16,  0,32,63
  DB 32, 0,16, 32, 0,63, 32,32,16, 32,32,32
  DB  0,16, 0,  0,16,32,  0,63, 0,  0,63,32
  DB 32,16, 0, 32,16,32, 32,63, 0, 32,63,32
  DB  0,16,16,  0,16,63,  0,63,16,  0,63,63
  DB 32,16,16, 32,16,63, 32,63,16, 32,63,63
  DB 16, 0, 0, 16, 0,32, 16,32, 0, 16,32,32
  DB 63, 0, 0, 63, 0,32, 63,32, 0, 63,32,32
  DB 16, 0,16, 16, 0,63, 16,32,16, 16,32,63
  DB 63, 0,16, 63, 0,63, 63,32,16, 63,32,63
  DB 16,16, 0, 16,16,32, 16,63, 0, 16,63,32
  DB 63,16, 0, 63,16,32, 63,63, 0, 63,63,32
  DB 16,16,16,  0, 0,63,  0,63, 0,  0,63,63
  DB 63, 0, 0, 63, 0,63, 63,63, 0, 63,63,63

  DB  0, 0, 0,  0, 0,63,  0,63, 0,  0,63,63
  DB 63, 0, 0, 63, 0,63, 63,63, 0, 48,48,48
  DB  0, 0,21,  0, 0,63,  0,42,21,  0,42,63
  DB 42, 0,21, 42, 0,63, 42,42,21, 42,42,63
  DB  0,21, 0,  0,21,42,  0,63, 0,  0,63,42
  DB 63,32, 0, 42,21,42, 42,63, 0, 42,63,42
  DB  0,21,21,  0,21,63,  0,63,21,  0,63,63
  DB 42,21,21, 42,21,63, 42,63,21, 42,63,63
  DB 21, 0, 0, 21, 0,42, 21,42, 0, 21,42,42
  DB 63, 0, 0, 63, 0,42, 63,42, 0, 63,42,42
  DB 21, 0,21, 21, 0,63, 21,42,21, 21,42,63
  DB 63, 0,21, 63, 0,63, 63,42,21, 63,42,63
  DB 21,21, 0, 21,21,42, 21,63, 0, 21,63,42
  DB 63,21, 0, 63,21,42, 63,63, 0, 63,63,42
  DB 32,32,32, 32,32,63, 32,63,32, 32,63,63
  DB 63,32,32, 63,32,63, 63,63,32, 63,63,63

  DB  0, 0, 0,  0,32, 0, 32, 0, 0, 32,16, 0
  DB  0, 0,32,  0,32,32, 32, 0,32, 32,32,32
  DB  0,16, 0,  0,63, 0, 32,16, 0, 32,63, 0
  DB  0,16,32,  0,63,32, 32,16,32, 32,32,32
  DB 16, 0, 0, 16,32, 0, 63, 0, 0, 63,32, 0
  DB 32, 0,32, 16,32,32, 63, 0,32, 63,32,32
  DB 16,16, 0, 16,63, 0, 63,16, 0, 63,63, 0
  DB 16,16,32, 16,63,32, 63,16,32, 63,63,32
  DB  0, 0,16,  0,32,16, 32, 0,16, 32,32,16
  DB  0, 0,63,  0,32,63, 32, 0,63, 32,32,63
  DB  0,16,16,  0,63,16, 32,16,16, 32,63,16
  DB  0,16,63,  0,63,63, 32,16,63, 32,63,63
  DB 16, 0,16, 16,32,16, 63, 0,16, 63,32,16
  DB 16, 0,63, 16,32,63, 63, 0,63, 63,32,63
  DB 16,16,16,  0,63, 0, 63, 0, 0, 63,63, 0
  DB  0, 0,63,  0,63,63, 63, 0,63, 63,63,63

  DB  0, 0, 0,  0,63, 0, 63, 0, 0, 63,32, 0
  DB  0, 0,63,  0,63,63, 63, 0,63, 48,48,48
  DB  0,21, 0,  0,63, 0, 42,21, 0, 42,63, 0
  DB  0,21,42,  0,63,42, 42,21,42, 42,63,42
  DB 21, 0, 0, 21,42, 0, 63, 0, 0, 63,42, 0
  DB 63, 0,63, 21,42,42, 63, 0,42, 63,42,42
  DB 21,21, 0, 21,63, 0, 63,21, 0, 63,63, 0
  DB 21,21,42, 21,63,42, 63,21,42, 63,63,42
  DB  0, 0,21,  0,42,21, 42, 0,21, 42,42,21
  DB  0, 0,63,  0,42,63, 42, 0,63, 42,42,63
  DB  0,21,21,  0,63,21, 42,21,21, 42,63,21
  DB  0,21,63,  0,63,63, 42,21,63, 42,63,63
  DB 21, 0,21, 21,42,21, 63, 0,21, 63,42,21
  DB 21, 0,63, 21,42,63, 63, 0,63, 63,42,63
  DB 32,32,32, 32,63,32, 63,32,32, 63,63,32
  DB 32,32,63, 32,63,63, 63,32,63, 63,63,63

