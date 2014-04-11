@echo off
copy moire.asm+tables.asm combined.asm
nasm -o moire.com -f bin combined.asm

