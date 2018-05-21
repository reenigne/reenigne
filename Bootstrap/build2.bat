@echo off

if not exist 1.com call build1.bat

echo Building step 2

rem --------------------------------------------------------------------------
rem Step 2: Use the program from step 1 to make an improved hex to binary
rem translator. Note that because the way redirection in DOS/Windows works, we
rem can't output byte 1A (EOF) or byte 09 (tab) because it's expanded into
rem spaces. The improved dumper can take multiple bytes at once on the command
rem line, uses proper hex characters (A-F instead of j-o) and ignores
rem characters not in the range 0-9, a-f and A-F.
rem --------------------------------------------------------------------------

rem 0100 0E        PUSH CS
rem 0101 1F        POP DS
rem 0102 8A0E8000  MOV CL,B[080]
rem 0106 BE8100    MOV SI,081
rem 0109 32ED      XOR CH,CH
rem 010B 32D2      XOR DL,DL
rem looptop:
rem 010D AC        LODSB
rem 010E FEC9      DEC CL
rem 0110 7C39      JL fin     (14B - 112 = 39)
rem 0112 3C30      CMP AL,030
rem 0114 7CF7      JL looptop (10D - 116 = F7)
rem 0116 3C39      CMP AL,039
rem 0118 7E14      JLE number (12E - 11A = 14)
rem 011A 3C41      CMP AL,041
rem 011C 7CEF      JL looptop (10D - 11E = EF)
rem 011E 3C46      CMP AL,046
rem 0120 7E0A      JLE capital (12C - 122 = 0A)
rem 0122 3C61      CMP AL,061
rem 0124 7CE7      JL looptop (10D - 126 = E7)
rem 0126 3C66      CMP AL,066
rem 0128 7FE3      JG looptop (10D - 12A = E3)
rem 012A 2C20      SUB AL,020
rem capital:
rem 012C 2C07      SUB AL,7
rem number:
rem 012E 2C30      SUB AL,030
rem 0130 D0E2      SHL DL,1
rem 0132 D0E2      SHL DL,1
rem 0134 D0E2      SHL DL,1
rem 0136 D0E2      SHL DL,1
rem 0138 08C2      OR DL,AL
rem 013A 80F501    XOR CH,1
rem 013D 75CE      JNZ looptop (10D - 13F = CE)
rem 013F B402      MOV AH,2
rem 0141 51        PUSH CX
rem 0142 52        PUSH DX
rem 0143 56        PUSH SI
rem 0144 CD21      INT 021
rem 0146 5E        POP SI
rem 0147 5A        POP DX
rem 0148 59        POP CX
rem 0149 EBC2      JMP looptop (10D - 14B = C2)
rem fin:
rem 014B B44C      MOV AH,04C
rem 014D CD21      INT 021

rem Hex translation table for 1.com
rem 0123456789ABCDEF
rem 0123456789jklmno

1 0n >2.com
1 1o >>2.com
1 8j >>2.com
1 0n >>2.com
1 80 >>2.com
1 00 >>2.com
1 kn >>2.com
1 81 >>2.com
1 00 >>2.com
1 32 >>2.com
1 nm >>2.com
1 32 >>2.com
1 m2 >>2.com
1 jl >>2.com
1 on >>2.com
1 l9 >>2.com
1 7l >>2.com
1 39 >>2.com
1 3l >>2.com
1 30 >>2.com
1 7l >>2.com
1 o7 >>2.com
1 3l >>2.com
1 39 >>2.com
1 7n >>2.com
1 14 >>2.com
1 3l >>2.com
1 41 >>2.com
1 7l >>2.com
1 no >>2.com
1 3l >>2.com
1 46 >>2.com
1 7n >>2.com
1 0j >>2.com
1 3l >>2.com
1 61 >>2.com
1 7l >>2.com
1 n7 >>2.com
1 3l >>2.com
1 66 >>2.com
1 7o >>2.com
1 n3 >>2.com
1 2l >>2.com
1 20 >>2.com
1 2l >>2.com
1 07 >>2.com
1 2l >>2.com
1 30 >>2.com
1 m0 >>2.com
1 n2 >>2.com
1 m0 >>2.com
1 n2 >>2.com
1 m0 >>2.com
1 n2 >>2.com
1 m0 >>2.com
1 n2 >>2.com
1 08 >>2.com
1 l2 >>2.com
1 80 >>2.com
1 o5 >>2.com
1 01 >>2.com
1 75 >>2.com
1 ln >>2.com
1 k4 >>2.com
1 02 >>2.com
1 51 >>2.com
1 52 >>2.com
1 56 >>2.com
1 lm >>2.com
1 21 >>2.com
1 5n >>2.com
1 5j >>2.com
1 59 >>2.com
1 nk >>2.com
1 l2 >>2.com
1 k4 >>2.com
1 4l >>2.com
1 lm >>2.com
1 21 >>2.com
