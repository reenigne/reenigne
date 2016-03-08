@echo off
yasm infinite_bobs.asm -o 1.bin
copy /b 1.bin+bobs.bin infinite_bobs.bin
