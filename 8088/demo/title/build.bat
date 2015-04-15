@echo off
yasm title.asm -o t.bin
copy /b t.bin+title.bin prog.bin
