@echo off
yasm infinite_bobs.asm -o 1.bin
make_bobs\debug\make_bobs 1.bin infinite_bobs.bin

