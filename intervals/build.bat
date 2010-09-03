@echo off
"C:\Program Files\Microchip\MPASM Suite\MPASMWIN.exe" /q /p12F508 "intervals.asm" /l"intervals.lst" /e"intervals.err"
type intervals.err
"C:\Program Files\Microchip\MPASM Suite\mplink.exe" /p12F508 "intervals.o" /o"intervals.cof" /M"intervals.map" /W /x
