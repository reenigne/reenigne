@echo off
yasm lockstep_test2.asm -o lockstep_test2.bin
yasm l.asm -o l.bin -l l.lst
yasm lcom.asm -o l.com
