@echo off
nasm graphical_timer.asm -o graphical_timer.bin -l graphical_timer.lst
nasm gt.asm -o gt.com -l gt.lst
