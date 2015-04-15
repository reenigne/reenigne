@echo off
set oldpath=%path%
set path=C:\Windows\system32;C:\Windows;C:\Windows\System32\Wbem;T:\t\Utils;T:\msvc\bin
rem copy kefrens.asm+..\popcorn\tables.asm+tables.asm combined.asm
nasm kefrens.asm -o kefrens.obj -f obj -l kefrens.lst
..\tools\2obj\2obj b /s tables.dat table.obj .HUGE:table
T:\msvc\bin\link kefrens.obj table.obj, kefrens.exe, kefrens.map,, /NOLOGO /ONERROR:NOEXE /NOI /NOE
rem T:\msvc\bin\link kefrens.obj, kefrens.exe, kefrens.map,, /NOLOGO /ONERROR:NOEXE /NOI /NOE
set path=%oldpath%
