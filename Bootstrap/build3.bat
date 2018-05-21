@echo off

if not exist 2.com call build2.bat

echo Building step 3

rem --------------------------------------------------------------------------
rem Step 3: Use the program from step 2 to make an improved hex to binary
rem translator. The aim of this one is to use files instead of redirection, so
rem we can use 09 bytes and don't have to have source code in these batch
rem files.
rem --------------------------------------------------------------------------

rem 0100 0E        PUSH CS
rem 0101 1F        POP DS
rem 0102 BE8200    MOV SI,082
rem 0105 89F2      MOV DX,SI
rem fnloop1:
rem 0107 AC        LODSB
rem 0108 3C20      CMP AL,020
rem 010A 75FB      JNE fnloop1 (0107 - 010C = FB)
rem 010C 30DB      XOR BL,BL
rem 010E 885CFF    MOV B[SI-1],BL
rem 0111 89F7      MOV DI,SI
rem fnloop2:
rem 0113 AC        LODSB
rem 0114 3C0D      CMP AL,0d
rem 0116 75FB      JNE fnloop2 (0113 - 0118 = FB)
rem 0118 885CFF    MOV B[SI-1],BL

rem 011B B8003D    MOV AX,03d00
rem 011E CD21      INT 021
rem 0120 7274      JC fin (0196 - 0122 = 74)
rem 0122 89C6      MOV SI,AX

rem 0124 B43C      MOV AH,03c
rem 0126 31C9      XOR CX,CX
rem 0128 89FA      MOV DX,DI
rem 012A CD21      INT 021
rem 012C 7268      JC fin (0196 - 012E = 68)
rem 012E 89C7      MOV DI,AX

rem 0130 32ED      XOR CH,CH
rem 0132 32D2      XOR DL,DL

rem looptop:
rem 0134 52        PUSH DX
rem 0135 51        PUSH CX
rem 0136 56        PUSH SI
rem 0137 57        PUSH DI
rem 0138 B43F      MOV AH,03f
rem 013A 89F3      MOV BX,SI
rem 013C B90100    MOV CX,1
rem 013F BA0001    MOV DX,0100
rem 0142 CD21      INT 021
rem 0144 85C0      TEST AX,AX
rem 0146 744E      JZ fin (0196 - 0148 = 4E)

rem 0148 A00001    MOV AL,B[0100]
rem 014B 5F        POP DI
rem 014C 5E        POP SI
rem 014D 59        POP CX
rem 014E 5A        POP DX

rem 014F 3C30      CMP AL,030
rem 0151 7CE1      JL looptop (134 - 153 = E1)
rem 0153 3C39      CMP AL,039
rem 0155 7E14      JLE number (16B - 157 = 14)
rem 0157 3C41      CMP AL,041
rem 0159 7CD9      JL looptop (134 - 15B = D9)
rem 015B 3C46      CMP AL,046
rem 015D 7E0A      JLE capital (169 - 15F = 0A)
rem 015F 3C61      CMP AL,061
rem 0161 7CD1      JL looptop (134 - 163 = D1)
rem 0163 3C66      CMP AL,066
rem 0165 7FCD      JG looptop (134 - 167 = CD)
rem 0167 2C20      SUB AL,020
rem capital:
rem 0169 2C07      SUB AL,7
rem number:
rem 016B 2C30      SUB AL,030
rem 016D D0E2      SHL DL,1
rem 016F D0E2      SHL DL,1
rem 0171 D0E2      SHL DL,1
rem 0173 D0E2      SHL DL,1
rem 0175 08C2      OR DL,AL
rem 0177 80F501    XOR CH,1
rem 017A 75B8      JNZ looptop (134 - 17C = B8)

rem 017C 88160001  MOV B[0100],DL

rem 0180 52        PUSH DX
rem 0181 51        PUSH CX
rem 0182 56        PUSH SI
rem 0183 57        PUSH DI
rem 0184 B440      MOV AH,040
rem 0186 89FB      MOV BX,DI
rem 0188 B90100    MOV CX,1
rem 018B BA0001    MOV DX,0100
rem 018E CD21      INT 021
rem 0190 5F        POP DI
rem 0191 5E        POP SI
rem 0192 59        POP CX
rem 0193 5A        POP DX

rem 0194 EB9E      JMP looptop (134 - 196 = 9E)
rem fin:
rem 0196 B44C      MOV AH,04C
rem 0198 CD21      INT 021


2 0E       > 3.com
2 1F       >> 3.com
2 BE8200   >> 3.com
2 89F2     >> 3.com
2 AC       >> 3.com
2 3C20     >> 3.com
2 75FB     >> 3.com
2 30DB     >> 3.com
2 885CFF   >> 3.com
2 89F7     >> 3.com
2 AC       >> 3.com
2 3C0D     >> 3.com
2 75FB     >> 3.com
2 885CFF   >> 3.com

2 B8003D   >> 3.com
2 CD21     >> 3.com
2 7274     >> 3.com
2 89C6     >> 3.com

2 B43C     >> 3.com
2 31C9     >> 3.com
2 89FA     >> 3.com
2 CD21     >> 3.com
2 7268     >> 3.com
2 89C7     >> 3.com

2 32ED     >> 3.com
2 32D2     >> 3.com

2 52       >> 3.com
2 51       >> 3.com
2 56       >> 3.com
2 57       >> 3.com
2 B43F     >> 3.com
2 89F3     >> 3.com
2 B90100   >> 3.com
2 BA0001   >> 3.com
2 CD21     >> 3.com
2 85C0     >> 3.com
2 744E     >> 3.com

2 A00001   >> 3.com
2 5F       >> 3.com
2 5E       >> 3.com
2 59       >> 3.com
2 5A       >> 3.com

2 3C30     >> 3.com
2 7CE1     >> 3.com
2 3C39     >> 3.com
2 7E14     >> 3.com
2 3C41     >> 3.com
2 7CD9     >> 3.com
2 3C46     >> 3.com
2 7E0A     >> 3.com
2 3C61     >> 3.com
2 7CD1     >> 3.com
2 3C66     >> 3.com
2 7FCD     >> 3.com
2 2C20     >> 3.com
2 2C07     >> 3.com
2 2C30     >> 3.com
2 D0E2     >> 3.com
2 D0E2     >> 3.com
2 D0E2     >> 3.com
2 D0E2     >> 3.com
2 08C2     >> 3.com
2 80F501   >> 3.com
2 75B8     >> 3.com

2 88160001 >> 3.com

2 52       >> 3.com
2 51       >> 3.com
2 56       >> 3.com
2 57       >> 3.com
2 B440     >> 3.com
2 89FB     >> 3.com
2 B90100   >> 3.com
2 BA0001   >> 3.com
2 CD21     >> 3.com
2 5F       >> 3.com
2 5E       >> 3.com
2 59       >> 3.com
2 5A       >> 3.com

2 EB9E     >> 3.com
2 B44C     >> 3.com
2 CD21     >> 3.com
