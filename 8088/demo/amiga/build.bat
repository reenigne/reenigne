@echo off
copy amiga.asm+tables.asm combined.asm
yasm -o amiga.bin -f bin combined.asm
rem copy billions_xtserver.asm+tables.asm combined_xtserver.asm
rem yasm -o billions_xtserver.bin -f bin combined_xtserver.asm

