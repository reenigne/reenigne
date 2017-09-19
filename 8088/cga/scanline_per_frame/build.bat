@echo off
yasm 1spf.asm -o 1spf.bin
yasm 1spf2.asm -o 1spf2.bin
yasm 1spf_intermittent.asm -o 1spf_intermittent.bin -l 1spf_intermittent.lst
yasm 1spf2_intermittent.asm -o 1spf2_intermittent.bin
yasm visible_lockstep.asm -o visible_lockstep.bin
yasm 1spf3.asm -o 1spf3.bin
yasm 1spf4.asm -o 1spf4.bin
yasm 1spf4_single.asm -o 1spf4_single.bin
yasm 1spf_unrolled.asm -o 1spf_unrolled.bin
yasm rose.asm -o r.com -l r.lst
copy /b r.com+\pictures\reenigne\cga2ntsc\rose_out_rgbi.dat rose.com
yasm rose.asm -o r.bin -dbin=1 -l r.lst
copy /b r.bin+\pictures\reenigne\cga2ntsc\rose_out.dat rose.bin
yasm 1spf_newstart.asm -o 1spf_newstart.bin
yasm 1spf_newstart2.asm -o 1spf_newstart2.bin

