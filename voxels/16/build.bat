@echo off
set oldpath=%path%
set path=C:\Windows\system32;C:\Windows;C:\Windows\System32\Wbem;c:\t\Utils;c:\msvc\bin
C:\msvc\bin\cl /nologo /W3 /DNDEBUG /Ox /AT /D_DOS /c /Ic:\msvc\include /Fa voxels.cpp
C:\msvc\bin\link voxels.obj, voxels.exe, voxels.map, c:\msvc\lib\slibce.lib c:\msvc\lib\oldnames.lib,  /NOLOGO /ONERROR:NOEXE /NOI /NOE
set path=%oldpath%
