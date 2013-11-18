@echo off
yasm scope.asm -o scope.bin
copy /b scope.bin+q:\sanxion2.raw scope2.bin
