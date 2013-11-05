@echo off
nasm -o pw.obj -f obj -l pw.lst pw.asm
set oldpath=%path%
set path=C:\Windows\system32;C:\Windows;C:\Windows\System32\Wbem;D:\t\Utils;D:\msvc\bin
D:\msvc\bin\cl /nologo /W3 /DNDEBUG /Ox /AT /D_DOS /c /ID:\msvc\include /Fa playwave.cpp
D:\msvc\bin\link playwave.obj pw.obj, playwave.exe, playwave.map, D:\msvc\lib\slibce.lib D:\msvc\lib\oldnames.lib,  /NOLOGO /ONERROR:NOEXE /NOI /NOE /STACK:4096
set path=%oldpath%
