@echo off
copy moire.asm+tables.asm combined.asm
yasm -o moire.com -f bin combined.asm
copy moire_xtserver.asm+tables.asm combined_xtserver.asm
yasm -o moire_xtserver.bin -f bin combined_xtserver.asm

