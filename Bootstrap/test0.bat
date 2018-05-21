@echo off
echo Testing step 0

rem 100 "%  " - AND AX,2020
rem 103 "%@@" - AND AX,4040      ;zero AX
rem 106 "5:A" - XOR AX,413A
rem 109 "5 @" - XOR AX,4020      ;AX = 11A (address to modify)
rem 10C "P"   - PUSH AX
rem 10D "["   - POP BX           ;MOV BX,AX
rem 10E "5@R" - XOR AX,5240      ;AX = 535A (AH = amount to subtract)
rem 111 ".('" - CS: SUB B[BX],AH ;self-modify (20->CD - INT instruction)
rem 114 "5zS" - XOR AX,537A      ;AX = 0020
rem 117 "5 L" - XOR AX,4C20      ;AX = 4C00
rem 11A " !"  - DB 20,21

rem Percent signs need to be doubled or echo will misinterpret them

echo %%  %%@@5:A5 @P[5@R.('5zS5 L ! >0.com
0
rem del 0.com

echo Step 0 completed successfully.
