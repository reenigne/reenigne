@echo off
set oldpath=%path%
set path=C:\Windows\system32;C:\Windows;C:\Windows\System32\Wbem;D:\t\Utils;c:\msvc\bin
c:\msvc\bin\cl /nologo /W3 /DNDEBUG /Ox /AT /D_DOS /c /Ic:\msvc\include /Fa 3d.cpp
"C:\Program Files (x86)\nasm\nasm.exe" -f obj 3dasm.asm -o 3dasm.obj -l 3dasm.lst
c:\msvc\bin\link c:\msvc\lib\crtcom.lib 3d.obj 3dasm.obj, 3d.com, 3d.map, c:\msvc\lib\slibce.lib c:\msvc\lib\oldnames.lib,  /NOLOGO /ONERROR:NOEXE /NOI /TINY /NOE
set path=%oldpath%
