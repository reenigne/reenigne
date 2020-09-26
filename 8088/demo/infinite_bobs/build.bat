@echo off
rem yasm infinite_bobs.asm -o 1.bin
nasm infinite_bobs.asm -o 1.bin -l 1.lst
make_bobs\debug\make_bobs 1.bin infinite_bobs.bin

nasm infinite_bobs.asm -o 1.com -l 1.lst
make_bobs\debug\make_bobs 1.com infinite_bobs.com

nasm i.asm -o i1.bin
make_bobs\debug\make_bobs i1.bin i.bin

nasm ib_partial.asm -o p.bin -l p.lst
make_bobs\debug\make_bobs p.bin ib_partial.bin


