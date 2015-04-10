@echo off
copy moire.asm+tables.asm combined.asm
yasm -o moire.com -f bin combined.asm

