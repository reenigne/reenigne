; Digger Remastered
; Copyright (c) Andrew Jenner 1998-2004

PUBLIC _olddelay,_getkips,_inittimer,_gethrt,_getlrt
PUBLIC _s0initint8,_s0restoreint8,_s0soundoff,_s0setspkrt2,_s0settimer0
PUBLIC _s0timer0,_s0settimer2,_s0timer2,_s0soundinitglob,_s0soundkillglob
PUBLIC _s1initint8,_s1restoreint8,_setsounddevice,_initsounddevice
PUBLIC _killsounddevice
PUBLIC _initkeyb,_restorekeyb,_getkey,_kbhit
PUBLIC _graphicsoff,_gretrace
PUBLIC _cgainit,_cgaclear,_cgapal,_cgainten,_cgaputi,_cgageti,_cgaputim
PUBLIC _cgawrite,_cgagetpix,_cgawrite,_cgatitle
PUBLIC _vgainit,_vgaclear,_vgapal,_vgainten,_vgaputi,_vgageti,_vgaputim
PUBLIC _vgawrite,_vgagetpix,_vgawrite,_vgatitle

_TEXT SEGMENT WORD PUBLIC 'CODE'

;Timing routines

_olddelay:
  PUSH BP
  SUB SP,6
  MOV BP,SP
  PUSH CX
  MOV CX,W[_volume]
delay0ltop:
  MOV W[BP+2],0
o24a:
  MOV AX,W[BP+2]
  CMP AX,W[BP+0a]
  JGE o267
  MOV W[BP+4],0
o257:
  CMP W[BP+4],064
  JGE o262
  INC W[BP+4]
  JMP o257
o262:
  INC W[BP+2]
  JMP o24a
o267:
  LOOP delay0ltop
  POP CX
  ADD SP,6
  POP BP
  RET

_getkips:
  PUSH ES
  PUSH SI
  MOV AX,040
  MOV ES,AX
  XOR AX,AX
  XOR DX,DX
  ES: MOV SI,W[06c]
zerotime:
  ES: CMP SI,W[06c]
  JZ zerotime
  ES: MOV SI,W[06c]
getkloop:
  CALL timekips
  ES: CMP SI,W[06c]
  JNZ donetime
  INC AX
  JNZ getkloop
  INC DX
  JMP getkloop
donetime:
  POP SI
  POP ES
  RET

timekips:
  PUSH AX
  PUSH DX
  PUSH SI
  MOV BX,0
  MOV SI,0
  DW 0a0f7,0
  DW 09801,0,09801,0,09801,0,09801,0,09801,0,09801,0,09801,0,09801,0,09801,0
  DW 09801,0,09801,0,09801,0,09801,0,09801,0,09801,0,09801,0,09801,0,09801,0
  DW 09801,0,09801,0,09801,0,09801,0,09801,0,09801,0,09801,0,09801,0,09801,0
  DW 09801,0,09801,0,09801,0,09801,0,09801,0,09801,0,09801,0,09801,0,09801,0
  DW 09801,0,09801,0,09801,0,09801,0
  POP SI
  POP DX
  POP AX
  RET

_inittimer:
  XOR AX,AX
  MOV W[_hrt],AX
  MOV W[_hrt+2],AX
  RET

_gethrt:
  PUSH SI
  PUSH DI
  LEA SI,W[_hrt]
  LEA DI,W[_hrt+2]
retryhrt:
  MOV AL,4   ;Latch counter 0 value
  CLI
  OUT 043,AL
  MOV BX,W[SI] ;Ideally, these four instructions would be executed
  MOV DX,W[DI] ;simultaneously - no time for an interrupt.
  MOV CX,W[countval] ; NB they probably are on heavily pipelined processors.

  IN AL,040
  MOV AH,AL
  IN AL,040
  STI
  XCHG AL,AH
  SUB CX,AX
  MOV AX,CX
  CMP AX,020
  JB retryhrt
  ADD AX,BX
  ADC DX,0
  POP DI
  POP SI
  RET

_getlrt:
  PUSH ES
  MOV AX,040
  MOV ES,AX
  ES: MOV AX,W[06c]
  ES: MOV DX,W[06e]
  POP ES
  RET

; New sound routines

_s1initint8:
  PUSH DI
  MOV AX,DS
  CS: MOV W[dssave8],AX
  PUSH DS
  MOV AX,0
  MOV DS,AX
  MOV DI,020
  MOV AX,W[DI]
  MOV BX,W[DI+2]
  CS: MOV W[int8save],AX
  CS: MOV W[int8save+2],BX
  MOV CX,offset interrupt8v2
  MOV DX,CS
  CLI
  MOV W[DI],CX
  MOV W[DI+2],DX
  STI
  XOR AX,AX
  PUSH AX
  CALL _s0settimer0
  POP AX
  POP DS
  POP DI
  RET

interrupt8v2:
  PUSH AX
  PUSH BX
  PUSH CX
  PUSH DX
  PUSH DS
  PUSH ES
  CS: MOV AX,W[dssave8]
  MOV DS,AX
  MOV ES,AX

  CALL W[_fillbuffer]

  MOV AX,W[countval]
  DEC AX
  XOR DX,DX
  ADD AX,1
  ADC DX,0
  ADD W[_hrt],AX
  ADC W[_hrt+2],DX

  MOV BX,W[newcount]
  MOV W[countval],BX

  POP ES
  POP DS
  POP DX
  POP CX
  POP BX
  POP AX
  CS: JMP D[int8save]

_setsounddevice:
  PUSH BP
  MOV BP,SP
  MOV AX,W[BP+4] ;Port (0210, 0220, 0230, 0240, 0250 or 0260)
  MOV W[sbport],AX
  MOV W[irqport],021
  MOV AX,W[BP+6] ;IRQ (2, 3, 5, or 7)
  ADD AX,8
  CMP AX,010
  JL lowirq
  ADD AX,060
  MOV W[irqport],0a1
lowirq:
  MOV W[sbint],AX
  MOV AX,W[BP+8] ;DMA channel (0, 1 or 3)
  SHL AX,1
  MOV W[sbdma],AX
  MOV BX,W[BP+0a] ;Transfer frequency in Hz
  MOV DX,0f
  MOV AX,04240
  DIV BX
  MOV DX,256
  SUB DX,AX
  MOV W[sbfrq],DX
  MOV AX,W[BP+0c] ;Length in bytes
  MOV W[sblen],AX
  SHL AX,1
  PUSH AX
  CALL _malloc
  POP CX
  CMP AX,0
  JE endsetsb
  MOV W[sbbuf],AX
endsetsb:
  POP BP
  RET

_initsounddevice:
  PUSH BP
  PUSH SI
  PUSH DI
  MOV DX,W[sbport]
  ADD DX,6
  MOV AL,1
  OUT DX,AL

  PUSH DX
  CALL _gethrt
  MOV SI,AX
  MOV DI,DX
  ADD SI,4      ;3 microseconds
  ADC DI,0
delayloop:
  CALL _gethrt
  SUB AX,SI
  SBB DX,DI
  JC delayloop
  POP DX

  XOR AL,AL
  OUT DX,AL

  PUSH DX
  CALL _gethrt
  MOV SI,AX
  MOV DI,DX
  ADD SI,239  ;200 microseconds
  ADC DI,0
  POP DX
  ADD DX,8
waitloop:
  IN AL,DX
  TEST AL,080
  JNZ gotsb
  PUSH DX
  CALL _gethrt
  SUB AX,SI
  SBB DX,DI
  POP DX
  JC waitloop

nosb:
  XOR AX,AX
  JMP endreset
gotsb:
  SUB DX,4
  IN AL,DX
  CMP AL,0aa
  JNZ nosb
  MOV BX,W[sbint]
  SHL BX,1
  SHL BX,1
  PUSH ES
  XOR AX,AX
  MOV ES,AX
  ES: MOV AX,W[BX]
  MOV W[sboldint],AX
  ES: MOV AX,W[BX+2]
  MOV W[sboldint+2],AX
  CS: MOV W[sbsaveds],DS
  CLI
  MOV AX,offset sbhandler
  ES: MOV W[BX],AX
  MOV AX,CS
  ES: MOV W[BX+2],AX
  STI
  MOV CL,B[sbint]
  AND CL,7
  MOV DX,W[irqport]
  MOV AH,1
  SHL AH,CL
  IN AL,DX
  NOT AH
  AND AL,AH
  OUT DX,AL

  POP ES
  MOV AL,040
  CALL writedsp
  MOV AL,B[sbfrq]
  CALL writedsp
  MOV AL,0d1
  CALL writedsp
  CALL playsample
  MOV AX,-1
endreset:
  POP DI
  POP SI
  POP BP
  RET


_killsounddevice:
  MOV W[sbdonef],0
  MOV W[sbendf],-1
waitsendloop:
  CMP W[sbdonef],0
  JE waitsendloop
  MOV BX,W[sbint]
  SHL BX,1
  SHL BX,1
  PUSH ES
  XOR AX,AX
  MOV ES,AX
  CLI
  MOV AX,W[sboldint]
  ES: MOV W[BX],AX
  MOV AX,W[sboldint+2]
  ES: MOV W[BX+2],AX
  STI
  POP ES
  MOV CL,B[sbint]
  AND CL,7
  MOV DX,W[irqport]
  MOV AH,1
  SHL AH,CL
  IN AL,DX
  OR AL,AH
  OUT DX,AL
  RET


writedsp:
  PUSH AX
  MOV DX,W[sbport]
  ADD DX,0c
waitdsp:
  IN AL,DX
  OR AL,AL
  JS waitdsp
  POP AX
  OUT DX,AL
  RET


playsample:
  PUSH SI
  PUSH DI
  MOV AX,W[sbbuf]

  MOV SI,DS
  MOV DI,DS

  MOV CL,4
  SHL SI,CL
  MOV CL,12
  SHR DI,CL
  ADD SI,AX
  ADC DI,0

  MOV AX,SI
  MOV CX,W[sblen]
  ADD AX,CX
  JNC boundaryokay
  INC DI
  XOR SI,SI
boundaryokay:
  DEC CX
  MOV BX,W[sbdma]
  MOV DX,W[BX+dmamaskregs]
  MOV AL,BL
  SHR AL,1
  OR AL,4
  OUT DX,AL
  MOV DX,W[BX+dmaclearregs]
  XOR AL,AL
  OUT DX,AL
  MOV DX,W[BX+dmamoderegs]
  MOV AL,BL
  SHR AL,1
  OR AL,048
  OUT DX,AL
  MOV DX,W[BX+dmaaddressregs]
  MOV AX,SI
  OUT DX,AL
  MOV AL,AH
  OUT DX,AL
  MOV DX,W[BX+dmapageregs]
  MOV AX,DI
  OUT DX,AL
  MOV DX,W[BX+dmalengthregs]
  MOV AX,CX
  OUT DX,AL
  MOV AL,AH
  OUT DX,AL
  MOV DX,W[BX+dmamaskregs]
  MOV AL,BL
  SHR AL,1
  OUT DX,AL
  MOV AL,014
  CALL writedsp
  MOV AL,CL
  CALL writedsp
  MOV AL,CH
  CALL writedsp
  POP DI
  POP SI
  RET


sbsaveds:
  DW 0

sbhandler:
  PUSH AX
  PUSH BX
  PUSH CX
  PUSH DX
  PUSH DS
  PUSH ES
  PUSH SI
  PUSH DI
  CS: MOV AX,W[sbsaveds]
  MOV DS,AX
  MOV ES,AX
  MOV DX,W[sbport]
  ADD DL,0e
  IN AL,DX
  CMP W[sbendf],0
  JNZ endsbint
  MOV SI,W[_buffer]
  MOV BX,W[_firsts]
  ADD SI,BX
  MOV DI,W[sbbuf]
  MOV DX,W[_size]
  MOV CX,W[sblen]
copylooptop:
  MOVSB
  INC BX
  CMP BX,DX
  JNE contloop
  XOR BX,BX
  MOV SI,W[_buffer]
contloop:
  LOOP copylooptop
  MOV W[_firsts],BX
  CALL playsample
finsbint:
  MOV AL,020
  OUT 020,AL
  POP DI
  POP SI
  POP ES
  POP DS
  POP DX
  POP CX
  POP BX
  POP AX
  IRET
endsbint:
  MOV W[sbdonef],-1
  MOV W[sbendf],0
  JMP finsbint

;Original style sound routines

_s0initint8:
  PUSH DI
  MOV AX,DS
  CS: MOV W[dssave8],AX
  PUSH DS
  MOV AX,0
  MOV DS,AX
  MOV DI,020
  MOV AX,W[DI]
  MOV BX,W[DI+2]
  CS: MOV W[int8save],AX
  CS: MOV W[int8save+2],BX
  MOV CX,offset interrupt8
  MOV DX,CS
  CLI
  MOV W[DI],CX
  MOV W[DI+2],DX
  STI
  POP DS
  POP DI
  RET

_s1restoreint8:
_s0restoreint8:
  PUSH DI
  CLI
  MOV AL,034
  OUT 043,AL
  XOR AL,AL
  OUT 040,AL
  OUT 040,AL
  CS: MOV AX,W[int8save]
  CS: MOV BX,W[int8save+2]
  MOV DI,020
  PUSH DS
  XOR CX,CX
  MOV DS,CX
  MOV W[DI],AX
  MOV W[DI+2],BX
  STI
  POP DS
  POP DI
  RET

int8save:
  DW 0,0
dssave8:
  DW 0

interrupt8:
  PUSH AX
  PUSH BX
  PUSH CX
  PUSH DX
  PUSH DS
  PUSH ES
  CS: MOV AX,W[dssave8]
  MOV DS,AX
  MOV ES,AX

  MOV AX,W[countval]
  DEC AX
  XOR DX,DX
  ADD AX,1
  ADC DX,0
  ADD W[_hrt],AX
  ADC W[_hrt+2],DX

  MOV BX,W[newcount]
  MOV W[countval],BX

  CMP W[_spkrmode],0
  JZ o3ce
  CMP W[_spkrmode],1
  JZ o3be
  IN AL,061
  XOR AL,2
  OUT 061,AL
  JMP o3ce
o3be:
  MOV CX,W[_pulsewidth]
  IN AL,061
  OR AL,2
  OUT 061,AL
o3c9:
  LOOP o3c9
  AND AL,0fd
  OUT 061,AL
o3ce:
  MOV AX,W[_timerrate]
  CMP AX,0
  JZ autoback
  ADD W[_timercount],AX
  PUSHF
  MOV AX,W[_timercount]
  AND AX,0c000
  CS: CMP AX,W[ttbtc]
  JE nointyet
  CS: MOV W[ttbtc],AX
  CALL _soundint
nointyet:
  POPF
  JNC o3ea
back:

  POP ES
  POP DS
  POP DX
  POP CX
  POP BX
  POP AX

  CS: JMP D[int8save]
autoback:
  CALL _soundint
  JMP back
o3ea:
  MOV AL,020
  OUT 020,AL

  POP ES
  POP DS
  POP DX
  POP CX
  POP BX
  POP AX
  IRET

ttbtc: ;Top two bits of _timercount
  DW 0

_s0soundoff:
  IN AL,061
  AND AL,0fc
  OUT 061,AL
  RET

_s0setspkrt2:
  IN AL,061
  OR AL,3
  OUT 061,AL
  RET

_s0settimer0:
  PUSH BP
  MOV BP,SP
  MOV AL,034
  OUT 043,AL
  MOV AX,W[BP+4]
  MOV W[countval],AX
  MOV W[newcount],AX
  OUT 040,AL
  MOV AL,AH
  OUT 040,AL
  XOR AX,AX
  MOV W[_hrt],AX
  MOV W[_hrt+2],AX
  POP BP
  RET

_s0timer0:
  PUSH BP
  MOV BP,SP
  MOV AX,W[BP+4]
  MOV W[newcount],AX
  OUT 040,AL
  MOV AL,AH
  OUT 040,AL
  POP BP
  RET

_s0settimer2:
  PUSH BP
  MOV BP,SP
  MOV AL,0b6
  OUT 043,AL
  MOV AX,W[BP+4]
  OUT 042,AL
  MOV AL,AH
  OUT 042,AL
  POP BP
  RET

_s0timer2:
  PUSH BP
  MOV BP,SP
  MOV AX,W[BP+4]
  OUT 042,AL
  MOV AL,AH
  OUT 042,AL
  POP BP
  RET

_s0soundinitglob:
_s0soundkillglob:
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
  MOV AH,1
  INT 016
  JZ nokeypressed
  MOV AX,1
  JMP donekbhit
nokeypressed:
  XOR AX,AX
donekbhit:
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
  MOV CL,80
  MUL CL
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

_cgagetpix:
  PUSH BP
  MOV BP,SP
  PUSH DI
  MOV AX,0b800
  MOV ES,AX
  MOV BX,W[BP+6] ;y
  AND BX,1
  MOV CL,0d
  SHL BX,CL
  MOV AX,W[BP+6] ;y
  SAR AX,1
  MOV CL,80
  MUL CL
  ADD BX,AX
  MOV AX,W[BP+4] ;x
  SAR AX,1
  SAR AX,1
  ADD BX,AX
  ES: MOV AL,B[BX]
  POP DI
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

_cgatitle:
  PUSH SI
  PUSH DI
  PUSH DS
  PUSH ES
  MOV AX,0b800
  MOV ES,AX
  MOV DI,0
  MOV SI,offset _cgatitledat
  MOV AX,seg _cgatitledat
  MOV DS,AX
ctlt:
  MOV AL,B[SI]
  CMP AL,0fe
  JE ctrle
  ES: MOV B[DI],AL
  INC DI
  INC SI
  CMP DI,04000
  JNZ ctlt
  JMP ctdone
ctrle:
  INC SI
  MOV BL,B[SI]
  INC SI
  MOV AL,B[SI]
  INC SI
ctrlt:
  ES: MOV B[DI],AL
  INC DI
  CMP DI,04000
  JZ ctdone
  DEC BL
  JNZ ctrlt
  JMP ctlt
ctdone:
  POP ES
  POP DS
  POP DI
  POP SI
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
  MOV CL,160
  MUL CL
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

_vgagetpix:
  PUSH BP
  MOV BP,SP
  PUSH DI
  XOR DI,DI
  MOV AX,0a000
  MOV ES,AX
  MOV AX,W[BP+6]
  MOV CX,160
  MUL CX
  MOV CX,AX
  MOV BX,W[BP+4]
  SAR BX,1
  SAR BX,1
  ADD BX,CX
  MOV CX,4
get4ptop:
  PUSH CX
  DEC CL
  MOV DX,03ce
  MOV AL,4
  MOV AH,CL
  OUT DX,AX
  ES: MOV CL,B[BX]
  OR DI,CX
  ES: MOV CL,B[BX+80]
  OR DI,CX
  POP CX
  LOOP get4ptop
  MOV AX,DI
  AND AX,0ee ;Long story, to do with the height of fire going to 16 pixels
  POP DI
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

_vgatitle:
  PUSH ES
  PUSH DS
  PUSH DI
  PUSH SI
  MOV AX,0a000
  MOV ES,AX
  MOV SI,offset _vgatitledat
  MOV AX,seg _vgatitledat
  MOV DS,AX
  MOV CX,4
  MOV DX,03c4
vtplt:
  XOR DI,DI
  DEC CX
  MOV AX,0102
  SHL AH,CL
  OUT DX,AX
  INC CX
vtlt:
  MOV AL,B[SI]
  CMP AL,254
  JE vtrle
  ES: MOV B[DI],AL
  INC DI
  INC SI
  CMP DI,07d00
  JNZ vtlt
  JMP vtdone
vtrle:
  INC SI
  MOV BL,B[SI]
  INC SI
  MOV AL,B[SI]
  INC SI
vtrlt:
  ES: MOV B[DI],AL
  INC DI
  CMP DI,07d00
  JZ vtdone
  DEC BL
  JNZ vtrlt
  JMP vtlt
vtdone:
  LOOP vtplt
  POP SI
  POP DI
  POP DS
  POP ES
  RET

_DATA SEGMENT WORD PUBLIC 'DATA'

_paletten:
  DB 0

_hrt:
  DW 0,0
countval:
  DW 0
newcount:
  DW 0

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

sbport:
  DW 0220
sbint:
  DW 0f
sbdma:
  DW 2
sbfrq:
  DW 200
sblen:
  DW 100
sbbuf:
  DW 0
sboldint:
  DW 0,0
sbendf:
  DW 0
sbdonef:
  DW 0
irqport:
  DW 021

dmapageregs:
  DW 087,083,081,082,08f,08b,089,08a
dmaaddressregs:
  DW 0,2,4,6,0c0,0c4,0c8,0cc
dmalengthregs:
  DW 1,3,5,7,0c2,0c6,0ca,0ce
dmamaskregs:
  DW 0a,0a,0a,0a,0d4,0d4,0d4,0d4
dmamoderegs:
  DW 0b,0b,0b,0b,0d6,0d6,0d6,0d6
dmaclearregs:
  DW 0c,0c,0c,0c,0d8,0d8,0d8,0d8
