@echo off
yasm 1spf.asm -o 1spf.bin
yasm 1spf2.asm -o 1spf2.bin
yasm 1spf_intermittent.asm -o 1spf_intermittent.bin -l 1spf_intermittent.lst
yasm 1spf2_intermittent.asm -o 1spf2_intermittent.bin
yasm visible_lockstep.asm -o visible_lockstep.bin
yasm 1spf3.asm -o 1spf3.bin
yasm 1spf4.asm -o 1spf4.bin
yasm 1spf4_single.asm -o 1spf4_single.bin

