@echo off
copy popcorn.asm+tables.asm combined.asm
yasm -o popcorn.com -f bin combined.asm
copy popcorn_xtserver.asm+tables.asm combined_xtserver.asm
yasm -o popcorn_xtserver.bin -f bin combined_xtserver.asm

