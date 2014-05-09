@echo off
yasm memtimer.asm -o memtimer.bin
yasm onetimer.asm -o onetimer.bin
yasm mtd.asm -o mtd.com
yasm refresh_timer.asm -o refresh_timer.bin
yasm refresh_one.asm -o refresh_one.bin
