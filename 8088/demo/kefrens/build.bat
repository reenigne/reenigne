@echo off
copy kefrens.asm+tables.asm combined.asm
yasm -o kefrens.com -f bin combined.asm
copy kefrens_xtserver.asm+tables.asm combined_xtserver.asm
yasm -o kefrens_xtserver.bin -f bin combined_xtserver.asm

