@echo off
rem copy billions.asm+tables.asm combined.asm
yasm -o 1.bin -f bin billions.asm
copy /b 1.bin+\picture.bin 1024.bin
rem copy billions_xtserver.asm+tables.asm combined_xtserver.asm
rem yasm -o billions_xtserver.bin -f bin combined_xtserver.asm
yasm -o restarts.bin -f bin restarts.asm
