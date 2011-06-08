void line(UInt16 x0, UInt16 y0, UInt16 x1, UInt16 y1, UInt8 c)
{
    ES = 0xb800;
    AX = y1-y0;
    if (AX < 0)
        AX = -AX;
    bp_4 = AX;
    BX = x1-x0;
    if (BX < 0)
        BX = -BX;
    bp_2 = BX;
    if (BX < AX) {
        AX <<= 1;
        bp_6 = AX;
        BX <<= 1;
        bp_8 = BX;
        DI = -bp_2;
        CX = x0;
        if (y0 > y1) {
            DX = x0;
            x0 = x1;
            x1 = DX;
            DX = y0;
            y0 = y1;
            y1 = DX;
        }
        BX = ((y0 & 1) << 13) + (y0 >> 1) * 80 + (x0 >> 2);
        CL = (3 - (x0 & 3)) << 1;
        DX = bp_4;
        o2ef = bp_8;
        o345 = bp_8;
        o3ob = bp_6;
        o361 = bp_6;
        CH = c;
        if (


  MOV AX,[BP+8]
  CMP AX,[BP+4]
  JL o2c1
  JMP o364
o2c1:
  JMP o3oe
o2c3:
  CMP W[BP+0e],1
  JZ c1xor
  ES: MOV AL,B[BX]
  MOV AH,3
  SHL AH,CL
  NOT AH
  AND AL,AH
  MOV AH,CH
  SHL AH,CL
  ADD AL,AH
  ES: MOV B[BX],AL
  JMP c1finplot
c1xor:
  MOV AL,CH
  SHL AL,CL
  ES: XOR B[BX],AL
c1finplot:
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
  CMP W[BP+0e],1
  JZ c2xor
  ES: MOV AL,B[BX]
  MOV AH,3
  SHL AH,CL
  NOT AH
  AND AL,AH
  MOV AH,CH
  SHL AH,CL
  ADD AL,AH
  ES: MOV B[BX],AL
  JMP c2finplot
c2xor:
  MOV AL,CH
  SHL AL,CL
  ES: XOR B[BX],AL
c2finplot:
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

    else {

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
  CMP W[BP+0e],1
  JZ d1xor
  ES: MOV AL,B[BX]
  MOV AH,3
  SHL AH,CL
  NOT AH
  AND AL,AH
  MOV AH,CH
  SHL AH,CL
  ADD AL,AH
  ES: MOV B[BX],AL
  JMP d1finplot
d1xor:
  MOV AL,CH
  SHL AL,CL
  ES: XOR B[BX],AL
d1finplot:
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
  CMP W[BP+0e],1
  JZ d2xor
  ES: MOV AL,B[BX]
  MOV AH,3
  SHL AH,CL
  NOT AH
  AND AL,AH
  MOV AH,CH
  SHL AH,CL
  ADD AL,AH
  ES: MOV B[BX],AL
  JMP d2finplot
d2xor:
  MOV AL,CH
  SHL AL,CL
  ES: XOR B[BX],AL
d2finplot:
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
