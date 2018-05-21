@echo off
echo Building step 1

rem --------------------------------------------------------------------------
rem Stage 1: Use "echo" to dump binary data directly to a COM file we can
rem execute. This is fiddly because we can only use code and data in the range
rem 20 to 7E hex. Fortunately this subset of x86 machine code is Turing
rem complete - we can use self-modifying code to get opcodes we can't get
rem directly, and we can AND, SUB and XOR the acoumulator to get any values we
rem need.
rem Not very often you see an entire x86 machine code program that doesn't
rem have a single MOV in it!
rem --------------------------------------------------------------------------

rem Output an arbitrary character (specified on the command line) to stdout

rem 100 25 20 20  "%  " - AND AX,2020
rem 103 25 40 40  "%@@" - AND AX,4040
rem 106 35 6B 41  "5kA" - XOR AX,416B
rem 109 35 20 40  "5 @" - XOR AX,4020      ; AX = 014B (first address to modify)
rem 10C 50        "P"   - PUSH AX
rem 10D 5B        "["   - POP BX
rem 10E 35 4B 4F  "5KO" - XOR AX,4F4B      ; AX = 4E00 (AH = amount to subtract)
rem 111 2E 28 27  ".('" - CS: SUB B[BX],AH ; self-modify (20->D2 - SHL instruction)
rem 114 35 6B 4F  "5kO" - XOR AX,4F6B      ; AX = 016B (second address to modify)
rem 117 50        "P"   - PUSH AX
rem 118 5B        "["   - POP BX
rem 119 2D 7E 55  "-~U" - SUB AX,557E      ; AX = ABED (amount to subtract)
rem 11C 50        "P"   - PUSH AX
rem 11D 5F        "_"   - POP DI
rem 11E 2E 29 3F  ".)?" - CS: SUB W[BX],DI ; self-modify (7073->C486 - XCHG instruction)
rem 121 43        "C"   - INC BX
rem 122 43        "C"   - INC BX
rem 123 43        "C"   - INC BX
rem 124 43        "C"   - INC BX
rem 125 2D 4C 58  "-LX" - SUB AX,584C      ; AX = 53A1 (AH = amount to subtract)
rem 128 50        "P"   - PUSH AX
rem 129 2E 28 27  ".('" - CS: SUB B[BX],AH ; self-modify (20->CD - INT instruction)
rem 12C 2D 27 52  "-'R" - SUB AX,5227      ; AX = 017A (final address to modify)
rem 12F 50        "P"   - PUSH AX
rem 130 5B        "["   - POP BX
rem 131 58        "X"   - POP AX           ; AX = 53A1
rem 132 2E 28 27  ".('" - CS: SUB B[BX],AH ; self-modify (20->CD - INT instruction)
rem 135 2D 31 23  "-1#" - SUB AX,2331
rem 138 2D 6C 30  "-l0" - SUB AX,306C      ; AX=0004
rem 13B 50        "P"   - PUSH AX
rem 13C 59        "Y"   - POP CX
rem 13D 50        "P"   - PUSH AX
rem 13E 5A        "Z"   - POP DX
rem 13F 2D 2F 40  "-/@" - SUB AX,402F      ; AX=BFD5
rem 142 2D 33 41  "-3A" - SUB AX,4133      ; AX=7EA2
rem 145 35 20 7E  "5 ~" - XOR AX,7E20      ; AX=0082
rem 148 50        "P"   - PUSH AX
rem 149 5B        "["   - POP BX           ; BX=0082
rem 14A 2E 20 27  ". '" - CS: SHL B[BX],CL ; self-mod (2E D2 27)
rem 14D 43        "C"   - INC BX           ; BX=0083
rem 14E 2E 32 37  ".27" - CS: XOR DH,B[BX] ; DH=B[0083]
rem 151 52        "R"   - PUSH DX
rem 152 58        "X"   - POP AX
rem 153 25 20 2F  "% /" - AND AX,2F20
rem 156 25 40 4F  "%@O" - AND AX,4F40
rem 159 50        "P"   - PUSH AX
rem 15A 5A        "Z"   - POP DX           ; AND DH,0F
rem 15B 4B        "K"   - DEC BX           ; BX=0082
rem 15C 2E 30 37  ".07" - CS: XOR B[BX],DH ; B[0082] = byte to output
rem 15F 25 20 20  "%  " - AND AX,2020      ; AX=0
rem 162 35 23 30  "5#0" - XOR AX,3023      ; AX=3022
rem 165 35 21 30  "5!0" - XOR AX,3021      ; AX=0002
rem 168 2E 32 27  ".2'" - CS: XOR AH,B[BX] ; AX=B[0082] = byte to output
rem 16B 73 70     "sp"  - XCHG AL,AH       ; self-mod (86 C4)
rem 16D 50        "P"   - PUSH AX
rem 16E 5A        "Z"   - POP DX
rem 16F 20 21     " !"  - INT 21           ; self-mod (CD 21)
rem 171 25 20 20  "%  " - AND AX,2020
rem 174 25 40 40  "%@@" - AND AX,4040
rem 177 35 20 4C  "5 L" - XOR AX,4C20      ; AX=4C20
rem 17A 20 21     " !"  - INT 21           ; self-mod (CD 21)

rem Percent signs need to be doubled or echo will misinterpret them

echo %%  %%@@5kA5 @P[5KO.('5kOP[-~UP_.)?CCCC-LXP.('-'RP[X.('-1#-l0PYPZ-/@-3A5 ~P[. 'C.27RX%% /%%@OPZK.07%%  5#05!0.2'spPZ !%%  %%@@5 L ! >1.com
